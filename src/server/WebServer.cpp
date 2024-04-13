#include "WebServer.hpp"

WebServer::WebServer( t_serv_list const& servers )
{
	std::vector<Listen>	distinctListeners;

	if (servers.empty() == true)
		throw(ServerException({"no Servers provided for configuration"}));
	this->_servers = servers;
	for (auto const& server : this->_servers)
	{
		for (auto const& address : server.getListens())
		{
			auto isPresent = std::find(distinctListeners.begin(), distinctListeners.end(), address);
			if ( isPresent == distinctListeners.end() )
				distinctListeners.push_back(address);
		}
	}
	for (auto const& listener : distinctListeners)
	{
		try {
			this->_listenTo(listener.getIpString(), listener.getPortString());
		}
		catch (const ServerException& e) {
			std::cout << C_RED << e.what() << '\n' << C_RESET;
		}
	}
	if (this->_pollfds.empty() == true)
		throw(ServerException({"no available host:port in the configuration provided"}));
}

WebServer::~WebServer ( void ) noexcept
{
	for (auto &item : this->_requests)
		delete item.second;
	for (auto &item : this->_responses)
		delete item.second;
	for (auto &item : this->_cgi)
		delete item.second;
	for (auto &item : this->_pollitems)
	{
		if ((item.second->pollType == LISTENER) or
			(item.second->pollType == CLIENT_CONNECTION))
			shutdown(item.first, SHUT_RDWR);
		close(item.first);
		delete item.second;
	}
}

void	WebServer::run( void )
{
	int				nConn = -1;
	struct pollfd 	pollfdItem;

	while (true)
	{
		nConn = poll(this->_pollfds.data(), this->_pollfds.size(), 0);
		if (nConn < 0)
		{
			if ((errno != EAGAIN) and (errno != EWOULDBLOCK))
				throw(ServerException({"poll failed"}));
			continue;
		}
		else if (nConn == 0)
			continue;
		for(size_t i=0; i<this->_pollfds.size(); i++)
		{
			pollfdItem = this->_pollfds[i];
			try {
				if ((pollfdItem.revents & POLLIN) and (this->_pollitems[pollfdItem.fd]->pollType != CGI_RESPONSE_PIPE_READ_END))
					_readData(pollfdItem.fd);
				if ((pollfdItem.revents & POLLOUT) and !(pollfdItem.revents & POLLERR))	// POLLERR is expected when upload pipe is closed by CGI script
					_writeData(pollfdItem.fd);
				if (pollfdItem.revents & (POLLHUP | POLLERR | POLLNVAL)) 	// client-end side was closed / error / socket not valid
				{
					if ((pollfdItem.revents & POLLHUP) and (this->_pollitems[pollfdItem.fd]->pollType == CGI_RESPONSE_PIPE_READ_END))
					{
						if (this->_pollitems[pollfdItem.fd]->pollState == WAIT_FOR_CGI)
							this->_pollitems[pollfdItem.fd]->pollState = READ_CGI_RESPONSE;
						_readData(pollfdItem.fd);
					}
					else
						_dropConn(pollfdItem.fd);
				}
				if (!(pollfdItem.revents & POLLIN) and (this->_pollitems[pollfdItem.fd]->pollType == CLIENT_CONNECTION))
					_checkTimeout(pollfdItem.fd);
			}
			catch (const HTTPexception& e) {
				std::cout << C_RED << e.what()  << C_RESET << '\n';
				_redirectToErrorPage(pollfdItem.fd, e.getStatus());
			}
			catch (const EndConnectionException& e) {
				_dropConn(pollfdItem.fd);
			}
			catch (const std::exception& e) {
				std::cerr << C_RED << e.what() << C_RESET << '\n';
				_dropConn(pollfdItem.fd);
			}
		}
		_clearEmptyConns();
	}
}

void	WebServer::_listenTo( std::string const& hostname, std::string const& port )
{
	struct addrinfo *tmp, *list, filter;
	int 			yes=1, listenSocket=-1;

	filter.ai_flags = AI_PASSIVE;
	filter.ai_family = AF_UNSPEC;
	filter.ai_socktype = 0;
	filter.ai_protocol = IPPROTO_TCP;
	filter.ai_addrlen = 0;
	filter.ai_addr = 0;
	filter.ai_canonname = 0;
	filter.ai_next = 0;

	if (getaddrinfo(hostname.c_str(), port.c_str(), &filter, &list) != 0)
		throw(ServerException({"failed to get addresses for", hostname, ":", port}));
	for (tmp=list; tmp!=nullptr; tmp=tmp->ai_next)
	{
		listenSocket = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
		if (listenSocket == -1)
			continue;
		fcntl(listenSocket, F_SETFL, O_NONBLOCK);
		if (setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) != 0)
			std::cout << C_RED << "failed to update socket, trying to bind anyway... \n" << C_RESET;
		if (bind(listenSocket, tmp->ai_addr, tmp->ai_addrlen) == 0)
			break;
		close(listenSocket);
	}
	if (tmp == nullptr)
	{
		freeaddrinfo(list);
		std::cout << C_RED << "no available IP for " << hostname << ":" << port << '\n' << C_RESET;
		return ;
	}
	freeaddrinfo(list);
	if (listen(listenSocket, BACKLOG) != 0)
	{
		close(listenSocket);
		std::cout << C_RED << "failed listen on: " << hostname << ":" << port << "\n" << C_RESET;
	}
	else
	{
		this->_addConn(listenSocket, LISTENER, WAITING_FOR_CONNECTION, hostname, port);
		std::cout << C_GREEN << "listening on: " << hostname << ":" << port << "\n" << C_RESET;
	}
}

void	WebServer::_readData( int readFd )
{
	switch (this->_pollitems[readFd]->pollState)
	{
		case WAITING_FOR_CONNECTION:
			_handleNewConnection(readFd);
			break;

		case READ_REQ_HEADER:
			_readRequestHead(readFd);
			_resetTimeout(readFd);			// timeout between different requests, goes in idle mode after CONN_MAX_TIMEOUT seconds
			break;

		case READ_STATIC_FILE:
			_readStaticFile(readFd);
			break;

		case READ_REQ_BODY:
			_readRequestBody(readFd);
			break;

		case READ_CGI_RESPONSE:
			_readCGIresponse(readFd);
			break;

		default:
			break;
	}
}

void	WebServer::_writeData( int writeFd )
{
	switch (this->_pollitems[writeFd]->pollState)
	{
		case WRITE_TO_CGI:
			_writeToCGI(writeFd);
			break;

		case WRITE_TO_CLIENT:
			_writeToClient(writeFd);
			break;

		default:
			break;
	}
}

void	WebServer::_addConn( int newSocket , fdType typePollItem, fdState statePollItem, std::string const& ip, std::string const& port )
{
	t_PollItem *newPollitem = nullptr;

	if (newSocket == -1)
		throw(ServerException({"invalid file descriptor"}));
	this->_pollfds.push_back({newSocket, POLLIN | POLLOUT, 0});
	newPollitem = new PollItem;
	newPollitem->pollType = typePollItem;
	newPollitem->pollState = statePollItem;
	newPollitem->IPaddr = ip;
	newPollitem->port = port;
	this->_pollitems[newSocket] = newPollitem;
	_resetTimeout(newSocket);
}

void	WebServer::_dropConn(int toDrop) noexcept
{
	this->_emptyConns.push_back(toDrop);
}

void	WebServer::_clearEmptyConns( void ) noexcept
{
	int fdToDrop = -1;
	while (this->_emptyConns.empty() == false)
	{
		fdToDrop = this->_emptyConns.back();
		if ((this->_pollitems[fdToDrop]->pollType == LISTENER) or
			(this->_pollitems[fdToDrop]->pollType == CLIENT_CONNECTION))		// it's a socket
			shutdown(fdToDrop, SHUT_RDWR);
		close(fdToDrop);
		for (auto curr=this->_pollfds.begin(); curr != this->_pollfds.end(); curr++)
		{
			if (curr->fd == fdToDrop)
			{
				this->_pollfds.erase(curr);
				break;
			}
		}
		if (this->_pollitems[fdToDrop]->pollType == CLIENT_CONNECTION)
			std::cout << C_GREEN << "CLOSED CONNECTION - " << fdToDrop << C_RESET << std::endl;
		delete this->_pollitems[fdToDrop];
		this->_pollitems.erase(fdToDrop);
		_clearStructs(fdToDrop);
		this->_emptyConns.pop_back();
	}
}

void	WebServer::_clearStructs( int toDrop) noexcept
{
	if (this->_requests.count(toDrop) > 0)
	{
		delete this->_requests[toDrop];
		this->_requests.erase(toDrop);
	}
	if (this->_responses.count(toDrop) > 0)
	{
		delete this->_responses[toDrop];
		this->_responses.erase(toDrop);
	}
	if (this->_cgi.count(toDrop) > 0)
	{
		delete this->_cgi[toDrop];
		this->_cgi.erase(toDrop);
	}
}

int		WebServer::_getSocketFromFd( int fd )
{
	if (this->_pollitems[fd]->pollType == CGI_REQUEST_PIPE_WRITE_END)
	{
		for (auto& item : this->_cgi)
		{
			if (item.second->getUploadPipe()[1] == fd)
				return (item.second->getRequestSocket());
		}
	}
	else if (this->_pollitems[fd]->pollType == CGI_RESPONSE_PIPE_READ_END)
	{
		for (auto& item : this->_cgi)
		{
			if (item.second->getResponsePipe()[0] == fd)
				return (item.second->getRequestSocket());
		}
	}
	else if (this->_pollitems[fd]->pollType == STATIC_FILE)
	{
		for (auto& item : this->_responses)
		{
			if (item.second->getHTMLfd() == fd)
				return (item.first);
		}
	}
	else if (fd != -1)
		return (fd);
	throw(std::out_of_range("invalid file descriptor or not found:"));	// entity not found, this should not happen
}

t_serv_list	WebServer::_getServersFromIP( std::string const& ip, std::string const& port) const noexcept
{
	t_serv_list	matchingServers;

	for (auto const& server : this->_servers)
	{
		for (auto const& address : server.getListens())
		{
			if ((address.getIpString() == ip) and (address.getPortString() == port))
			{
				matchingServers.push_back(server);
				break ;
			}
		}
	}
	return (matchingServers);
}

path_t	WebServer::_getDefErrorPage( int statusCode ) const
{
	try
	{
		for (auto const& dir_entry : std::filesystem::directory_iterator{SERVER_DEF_PAGES})
		{
			if (dir_entry.path().stem() == std::to_string(statusCode))
				return (dir_entry.path());
		}
	}
	catch(const std::exception& e) {
		throw(ServerException({"path", SERVER_DEF_PAGES,"is not valid -", e.what()}));
	}
	throw(ServerException({"no default error page found for code", std::to_string(statusCode)}));
}

void	WebServer::_resetTimeout( int fd )
{
	this->_pollitems[fd]->lastActivity = steady_clock::now();
}

void	WebServer::_checkTimeout( int fd )
{
	duration<double> 	time_span;

	time_span = duration_cast<duration<int>>(steady_clock::now() - this->_pollitems[fd]->lastActivity);
	if (time_span.count() > CONN_MAX_TIMEOUT)
		throw(EndConnectionException());
}

void	WebServer::_handleNewConnection( int listenerFd )
{
	struct sockaddr_storage client;
	unsigned int 			sizeAddr = sizeof(client);
	int 					connFd = -1;
	std::string				ip, port;

	ip = this->_pollitems[listenerFd]->IPaddr;
	connFd = accept(listenerFd, (struct sockaddr *) &client, &sizeAddr);
	if (client.ss_family == AF_INET)
		port = std::to_string(ntohs(((struct sockaddr_in*) &client)->sin_port));
	else if (client.ss_family == AF_INET6)
		port = std::to_string(ntohs(((struct sockaddr_in6*) &client)->sin6_port));
	if (connFd == -1)
		std::cerr << C_RED  << "connection with: " << ip << ":" << port << " failed" << C_RESET << '\n';
	else
	{
		fcntl(connFd, F_SETFL, O_NONBLOCK);
		this->_addConn(connFd, CLIENT_CONNECTION, READ_REQ_HEADER, this->_pollitems[listenerFd]->IPaddr, this->_pollitems[listenerFd]->port);
		std::cout << C_GREEN << "connected to " << ip << ":" << port << C_RESET << '\n';
	}
}

void	WebServer::_readRequestHead( int clientSocket )
{
	HTTPrequest 	*request = nullptr;
	HTTPresponse	*response = nullptr;
	CGI				*cgi = nullptr;
	fdState			nextStatus;

	if (this->_requests[clientSocket] == nullptr)
		this->_requests[clientSocket] = new HTTPrequest(clientSocket, _getServersFromIP(this->_pollitems[clientSocket]->IPaddr, this->_pollitems[clientSocket]->port));
	request = this->_requests[clientSocket];
	request->parseHead();
	if (request->isDoneReadingHead())
	{
		response = new HTTPresponse(request->getSocket(), request->getStatusCode(), request->getType());
		this->_responses[clientSocket] = response;
		response->setTargetFile(request->getRealPath());
		response->setRoot(request->getRoot());
		if (request->isCGI())		// GET cgi, POST
		{
			cgi = new CGI(*request);
			if (request->isFastCGI() == true)
			{
				close(cgi->getUploadPipe()[0]);
				close(cgi->getUploadPipe()[1]);
			}
			else if (request->isFileUpload())
				this->_addConn(cgi->getUploadPipe()[1], CGI_REQUEST_PIPE_WRITE_END, WRITE_TO_CGI);
			this->_addConn(cgi->getResponsePipe()[0], CGI_RESPONSE_PIPE_READ_END, READ_CGI_RESPONSE);
			this->_cgi[clientSocket] = cgi;
			cgi->run();
		}
		else if (request->isStatic())		// GET static
			_addConn(response->getHTMLfd(), STATIC_FILE, READ_STATIC_FILE);
		if (request->isAutoIndex() or request->isRedirection() or request->isDelete())		// nothing more to do, send response
			nextStatus = WRITE_TO_CLIENT;
		else if (request->isFastCGI())														// run CGI
			nextStatus = WAIT_FOR_CGI;
		else if (request->hasBodyToRead())													// read request body (file upload)
			nextStatus = READ_REQ_BODY;
		else if (request->isStatic())														// read static file
			nextStatus = READ_STATIC_FILE;
		else																				// request body already read, run CGi (file upload)
			nextStatus = READ_CGI_RESPONSE;
		this->_pollitems[clientSocket]->pollState = nextStatus;
	}
}

void	WebServer::_readStaticFile( int staticFileFd )
{
	int 			socket = _getSocketFromFd(staticFileFd);
	HTTPresponse	*response = this->_responses.at(socket);

	if (this->_pollitems[staticFileFd]->pollType != STATIC_FILE)
		return ;
	response->readStaticFile();
	if (response->isDoneReadingHTML() == true)
	{
		_dropConn(staticFileFd);
		this->_pollitems[socket]->pollState = WRITE_TO_CLIENT;
	}
}

void	WebServer::_readRequestBody( int clientSocket )
{
	HTTPrequest *request = this->_requests.at(clientSocket);
	if (request->getTmpBody() == "")
		request->parseBody();
}

void	WebServer::_writeToCGI( int cgiPipe )
{
	int socket = _getSocketFromFd(cgiPipe);
	HTTPrequest *request = this->_requests.at(socket);
	ssize_t		readChars = -1;

	close(this->_cgi.at(request->getSocket())->getUploadPipe()[0]); // close read end of cgi upload pipe
	std::string tmpBody = request->getTmpBody();
	if (tmpBody != "")
	{
		readChars = write(cgiPipe, tmpBody.data(), tmpBody.length());
		if (readChars < 0)
			throw(ServerException({"unavailable socket"}));
		request->setTmpBody("");
		if (request->isDoneReadingBody())
		{
			_dropConn(cgiPipe);
			this->_pollitems.at(request->getSocket())->pollState = WAIT_FOR_CGI;
		}
	}
}

void	WebServer::_readCGIresponse( int cgiPipe )
{
	int 	socket = _getSocketFromFd(cgiPipe);
	CGI		*cgi = this->_cgi.at(socket);
	ssize_t	readChars = -1;
	char 	buffer[HTTP_BUF_SIZE];

	if (cgi->waitCGIproc() == true)  // check if CGI is done
	{
		std::fill(buffer, buffer + HTTP_BUF_SIZE, 0);
		readChars = read(cgiPipe, buffer, HTTP_BUF_SIZE);
		if (readChars < 0)
			throw(ServerException({"unavailable socket"}));
		cgi->appendResponse(std::string(buffer, buffer + readChars));
		if (readChars < HTTP_BUF_SIZE)
		{
			_dropConn(cgiPipe);
			this->_pollitems[socket]->pollState = WRITE_TO_CLIENT;
		}
	}

}

void	WebServer::_writeToClient( int clientSocket )
{
	HTTPrequest 	*request = this->_requests.at(clientSocket);
	HTTPresponse 	*response = this->_responses.at(clientSocket);

	if (response->isParsingNeeded())
	{
		if (response->isCGI())
			response->parseCGI(this->_cgi.at(clientSocket)->getResponse());
		else
		{
			if (response->isAutoIndex())
				response->listContentDirectory();
			else if (response->isDelete())
				response->removeFile();
			response->parseNotCGI(request->getServName());
		}
	}
	response->writeContent();
	if (response->isDoneWriting())
	{
		if ((request->isEndConn()) or (request->getStatusCode() == 444))		// NGINX custom behaviour, if code == 444 connection is closed as well
			_dropConn(clientSocket);
		else
		{
			_clearStructs(clientSocket);
			this->_pollitems[clientSocket]->pollState = READ_REQ_HEADER;
		}
	}
}

void	WebServer::_redirectToErrorPage( int genericFd, int statusCode ) noexcept
{
	int				clientSocket = _getSocketFromFd(genericFd);
	HTTPrequest		*request = this->_requests[clientSocket];
	HTTPresponse	*response = nullptr;
	path_t			HTMLerrPage;

	if (this->_pollitems[genericFd]->pollType > CLIENT_CONNECTION)	// when genericFd refers to a pipe or a static file
		_dropConn(genericFd);
	else if (statusCode == 444)		// NGINX custom behaviour: close connection without sending a response
	{
		_dropConn(clientSocket);
		this->_pollitems[clientSocket]->pollState = READ_REQ_HEADER;
		return ;
	}
	if (this->_responses[clientSocket] == nullptr)
		this->_responses[clientSocket] = new HTTPresponse(request->getSocket(), statusCode);
	response = this->_responses[clientSocket];
	try {
		request->updateErrorCode(statusCode);
		HTMLerrPage = request->getRealPath();
	}
	catch(const RequestException& e1) {
		std::cerr << C_RED << e1.what() << '\n' << C_RESET;
		statusCode = e1.getStatus();
		try {
			HTMLerrPage = _getDefErrorPage(statusCode);
		}
		catch(const ServerException& e) {
			std::cerr<< C_RED << e.what() << '\n' << C_RESET;
			if (statusCode == 500)
			{
				response->errorReset(statusCode, true);
				this->_pollitems[clientSocket]->pollState = WRITE_TO_CLIENT;
			}
			else
				_redirectToErrorPage(clientSocket, 500);
			return ;
		}
	}
	response->errorReset(statusCode, false);
	response->setTargetFile(HTMLerrPage);
	_addConn(response->getHTMLfd(), STATIC_FILE, READ_STATIC_FILE, "", "");
	this->_pollitems[clientSocket]->pollState = READ_STATIC_FILE;
}
