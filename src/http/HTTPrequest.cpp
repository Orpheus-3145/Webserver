#include "HTTPrequest.hpp"

void	HTTPrequest::parseHead( void )
{
	std::string strHead, strHeaders;
	size_t		endHead=0, endReq=0;

	if (this->_state != HTTP_REQ_HEAD_READING)
		throw(RequestException({"instance in wrong state to parse head"}, 500));
	_readHead();
	if (isDoneReadingHead() == true)
	{
		endReq = this->_tmpHead.find(HTTP_TERM);
		endHead = this->_tmpHead.find(HTTP_NL);		// look for headers
		if (endHead >= endReq)
			throw(RequestException({"no headers"}, 400));
		strHead = this->_tmpHead.substr(0, endHead);
		strHeaders = this->_tmpHead.substr(endHead + HTTP_NL.size(), endReq + HTTP_NL.size() - endHead - 1);
		endReq += HTTP_TERM.size();
		if ((endReq + 1) < this->_tmpHead.size())		// look for the beginning of the body
			this->_tmpBody = this->_tmpHead.substr(endReq);
		_setHead(strHead);
		_setHeaders(strHeaders);
		this->_validator.solvePath(this->_method, this->_url.path, getHost());
		this->_statusCode = this->_validator.getStatusCode();
		this->_root = this->_validator.getRoot();
		_setTypeAndState();
		if (this->_validator.solvePathFailed() == true)
			throw(RequestException({"validation of config file failed"}, this->_validator.getStatusCode()));
		_checkMaxBodySize();
		if (isDoneReadingBody())
			_setBody(this->_tmpBody);
	}
}

void	HTTPrequest::parseBody( void )
{
	if (isDoneReadingBody())
		throw(RequestException({"instance in wrong state or type"}, 500));
	_readBody();
	if (isDoneReadingBody())
		_setBody(this->_tmpBody);
}

std::string	HTTPrequest::toString( void ) const noexcept
{
	std::string	strReq;

	strReq += getMethod();
	strReq += HTTP_SP;
	strReq += this->_url.scheme;
	strReq += "://";
	strReq += this->_url.host;
	strReq += ":";
	strReq += this->_url.port;
	strReq += this->_url.path.generic_string();
	if (!this->_url.queryRaw.empty())
	{
		strReq += "?";
		strReq += this->_url.queryRaw;
	}
	if (!this->_url.fragment.empty())
	{
		strReq += "#";
		strReq += this->_url.fragment;
	}
	strReq += HTTP_SP;
	strReq += this->_version.scheme;
	strReq += "/";
	strReq += std::to_string(this->_version.major);
	strReq += ".";
	strReq += std::to_string(this->_version.minor);
	strReq += HTTP_NL;
	if (!this->_headers.empty())
	{
		for (auto item : this->_headers)
		{
			strReq += item.first;
			strReq += ":";
			strReq += HTTP_SP;
			strReq += item.second;
			strReq += HTTP_NL;
		}
	}
	strReq += HTTP_NL;
	if (this->_body.empty() == false)
	{
		strReq += this->_body;
		strReq += HTTP_NL;
	}
	return (strReq);
}

void	HTTPrequest::updateErrorCode( int errorCode )
{
	this->_statusCode = errorCode;
	this->_validator.solveErrorPath( errorCode );
	_setTypeAndState();
	if (this->_validator.solvePathFailed() == true)
	{
		this->_statusCode = this->_validator.getStatusCode();
		if (errorCode == this->_statusCode)
			throw(RequestException({"endless loop with code:", std::to_string(this->_statusCode)}, errorCode));	// error 404 and lacks 404.html (or same for 403)
		this->_validator.solveErrorPath(this->_statusCode);
		if (this->_statusCode == this->_validator.getStatusCode())
			throw(RequestException({"endless loop with code:", std::to_string(this->_statusCode)}, errorCode));
		else if (this->_validator.solvePathFailed() == true)
		{
			this->_statusCode = this->_validator.getStatusCode();
			this->_validator.solveErrorPath(this->_statusCode);
			if (this->_validator.solvePathFailed() == true)
				throw(RequestException({"endless cross loop with code:", std::to_string(this->_statusCode)}, this->_statusCode));
		}
	}
}

std::string 	HTTPrequest::getMethod( void ) const noexcept
{
	switch (this->_method)
	{
		case (HTTP_GET):
			return ("GET");
		case (HTTP_POST):
			return ("POST");
		case (HTTP_DELETE):
			return ("DELETE");
		default:
			return ("");
	}
}

std::string		HTTPrequest::getHost( void ) const noexcept
{
	try
	{
		std::string	hostPort = this->_headers.find(HTTP_HEADER_HOST)->second;
		size_t	semiColPos = hostPort.find(':');
		return (hostPort.substr(0, semiColPos));
	}
	catch(const std::out_of_range& e) {
		return ("");
	}
}

std::string		HTTPrequest::getPort( void ) const noexcept
{
	try
	{
		std::string	hostPort = this->_headers.find(HTTP_HEADER_HOST)->second;
		size_t	semiColPos = hostPort.find(':');
		if (semiColPos == std::string::npos)
			return (HTTP_DEF_PORT);
		else
			return (hostPort.substr(semiColPos + 1));
	}
	catch(const std::out_of_range& e) {
		return ("");
	}
}

size_t	HTTPrequest::getContentLength( void ) const noexcept
{
	return (this->_contentLength);
}

std::string	const&	HTTPrequest::getQueryRaw( void ) const noexcept
{
	return (this->_url.queryRaw);
}

std::string	const	HTTPrequest::getCookie( void ) const noexcept
{
	if (this->_headers.count(HTTP_HEADER_COOKIE) != 0)
		return (this->_headers.find(HTTP_HEADER_COOKIE)->second);
	else
		return std::string();
}

std::string		HTTPrequest::getContentTypeBoundary( void ) const noexcept
{
	std::string boundary = "";
	if (this->_headers.count(HTTP_HEADER_CONT_TYPE) > 0)
	{
		std::string contentType = this->_headers.find(HTTP_HEADER_CONT_TYPE)->second;
		size_t delim = contentType.find('=');
		boundary = contentType.substr(delim + 1, contentType.length() - delim);
	}
	return (boundary);
}

std::string const&	HTTPrequest::getServName( void ) const noexcept
{
	return(this->_validator.getServName());
}

path_t const&	HTTPrequest::getRealPath( void ) const noexcept
{

	return (this->_validator.getRealPath());
}

path_t const&	HTTPrequest::getRedirectPath( void ) const noexcept
{
	return (this->_validator.getRedirectRealPath());
}

path_t const&	HTTPrequest::getRoot( void ) const noexcept
{
	return (this->_validator.getRoot());
}

bool	HTTPrequest::isEndConn( void ) noexcept
{
	if (this->_headers.count(HTTP_HEADER_CONN) == 0)
		return (false);
	return (this->_headers.find(HTTP_HEADER_CONN)->second == "close");
}

bool	HTTPrequest::isChunked( void ) const noexcept
{
	return ((this->_headers.count(HTTP_HEADER_TRANS_ENCODING) > 0) &&
			(this->_headers.find(HTTP_HEADER_TRANS_ENCODING)->second == "chunked"));
}

bool	HTTPrequest::isDoneReadingHead( void ) const noexcept
{
	return (this->_state != HTTP_REQ_HEAD_READING);
}

bool	HTTPrequest::isDoneReadingBody( void ) const noexcept
{
	return(isFileUpload() and (this->_state == HTTP_REQ_DONE));
}

bool	HTTPrequest::hasBodyToRead( void ) const noexcept
{
	if (isDoneReadingBody())
		return (false);
	else if (isChunked())
		return (this->_tmpBody.find(HTTP_TERM) == std::string::npos);
	else
		return (this->_tmpBody.size() < this->_contentLength);
}

void	HTTPrequest::_setHead( std::string const& header )
{
	std::istringstream	stream(header);
	std::string 		method, url, version;
	char				spaceChar = *HTTP_SP.data();

	if (! std::getline(stream, method, spaceChar))
		throw(RequestException({"invalid header:", header}, 400));
	_setMethod(method);
	if (! std::getline(stream, url, spaceChar))
		throw(RequestException({"invalid header:", header}, 400));
	_setURL(url);
	if (! std::getline(stream, version, spaceChar))
		throw(RequestException({"invalid header:", header}, 400));
	_setVersion(version);
}

void	HTTPrequest::_setHeaders( std::string const& strHeaders )
{
	try {
		HTTPstruct::_setHeaders(strHeaders);
	}
	catch (const HTTPexception& e) {
		throw(RequestException(e));
	}
	if (this->_headers.count(HTTP_HEADER_HOST) == 0)		// missing Host header
		throw(RequestException({"no Host header"}, 444));
	else if (this->_url.host == "")
		_setHostPort(this->_headers.find(HTTP_HEADER_HOST)->second);
	else if (this->_headers.find(HTTP_HEADER_HOST)->second.find(this->_url.host) == std::string::npos)
		throw(RequestException({"hosts do not match"}, 412));
	if ((this->_method == HTTP_GET) or (this->_method == HTTP_DELETE))
		return ;

	if (this->_headers.count(HTTP_HEADER_CONT_TYPE) == 0)
		throw(RequestException({HTTP_HEADER_CONT_TYPE, "required"}, 400));
	if (this->_headers.count(HTTP_HEADER_CONT_LEN) == 0)
	{
		if (this->_headers.count(HTTP_HEADER_TRANS_ENCODING) == 0)
			throw(RequestException({HTTP_HEADER_CONT_LEN, "required"}, 411));
		else if (this->_headers.find(HTTP_HEADER_TRANS_ENCODING)->second != "chunked")
			throw(RequestException({HTTP_HEADER_TRANS_ENCODING, "required"}, 400));
	}
	else
	{
		try {
			this->_contentLength = std::stoull(this->_headers.find(HTTP_HEADER_CONT_LEN)->second);
		}
		catch (const std::exception& e) {
			throw(RequestException({"invalid Content-Length"}, 400));
		}
		if (this->_tmpHead.size() > this->_contentLength)
			this->_tmpBody = this->_tmpBody.substr(0, this->_contentLength);
	}
}

void	HTTPrequest::_setVersion( std::string const& strVersion )
{
	try {
		HTTPstruct::_setVersion(strVersion);
	}
	catch(const HTTPexception& e) {
		throw(RequestException(e));
	}
}

void	HTTPrequest::_setBody( std::string const& body )
{
	size_t		delim = 0;
	std::string	bodyToStore;

	if (isChunked())
	{
		delim = body.find(HTTP_TERM);
		if (delim > this->_maxBodySize)
			throw(RequestException({"content body is longer than the maximum allowed"}, 413));
		delim += HTTP_TERM.size();
		this->_tmpBody = _unchunkBody(body.substr(0, delim));
	}
	else
		this->_tmpBody = body.substr(0, this->_contentLength);
	HTTPstruct::_setBody(this->_tmpBody);
}

void	HTTPrequest::_readHead( void )
{
	char	buffer[HTTP_MAX_HEADER_SIZE];
	ssize_t	charsRead = -1;

	std::fill(buffer, buffer + HTTP_MAX_HEADER_SIZE, 0);
	charsRead = recv(this->_socket, buffer, HTTP_MAX_HEADER_SIZE, 0);
	if (charsRead < 0)
		throw(ServerException({"unavailable socket"}));
	else if (charsRead == 0)
		throw(EndConnectionException({}));
	else if (this->_tmpHead.size() + charsRead > HTTP_MAX_HEADER_SIZE)
		throw(RequestException({"head too large"}, 431));
	_checkTimeout();
	this->_tmpHead += std::string(buffer, buffer + charsRead);
	if (this->_tmpHead.find(HTTP_TERM) != std::string::npos)
		this->_state = HTTP_REQ_HEAD_PARSING;
}

void	HTTPrequest::_readBody( void )
{
    ssize_t charsRead = -1;
    char	buffer[HTTP_BUF_SIZE];

	std::fill(buffer, buffer + HTTP_BUF_SIZE, 0);
	charsRead = recv(this->_socket, buffer, HTTP_BUF_SIZE, 0);
	if (charsRead < 0 )
		throw(ServerException({"unavailable socket"}));
	else if (charsRead == 0)
		throw(EndConnectionException({}));
	_checkTimeout();
	this->_tmpBody += std::string(buffer, buffer + charsRead);
	if (hasBodyToRead() == false)
		this->_state = HTTP_REQ_DONE;
}

void	HTTPrequest::_setTypeAndState( void ) noexcept
{
	if (this->_statusCode >= 300)
	{
		if (this->_validator.isRedirection() == true)		// redirection
			this->_type = HTTP_REDIRECTION;
		else									// request failed, read errorPage
			this->_type = HTTP_STATIC;
		this->_state = HTTP_REQ_DONE;
	}
	else if (this->_headers.count(HTTP_HEADER_CONT_TYPE) > 0)		// request with body
	{
		this->_type = HTTP_CGI_FILE_UPL;
		if (hasBodyToRead())
			this->_state = HTTP_REQ_BODY_READING;
		else
			this->_state = HTTP_REQ_DONE;
	}
	else		// autoindex, bodyless CGI (GET), GET reqs, DELETE
	{
		if (this->_validator.isAutoIndex() == true)
			this->_type = HTTP_AUTOINDEX;
		else if (this->_validator.isCGI() == true)
			this->_type = HTTP_CGI_STATIC;
		else if (this->_method == HTTP_DELETE)
			this->_type = HTTP_FILE_DEL;
		else if (this->_method == HTTP_GET)
			this->_type = HTTP_STATIC;
		
		this->_state = HTTP_REQ_DONE;
	}
}

void	HTTPrequest::_checkMaxBodySize( void )
{
	if (this->_validator.getMaxBodySize() == 0)
		return;
	if (isChunked() == true)
	{
		if (this->_validator.getMaxBodySize() < this->_tmpBody.size())
			throw(RequestException({"Content-Length longer than config max body length"}, 413));
	}
	else if (isFileUpload() == true)
	{
		if (this->_validator.getMaxBodySize() < this->_contentLength)
			throw(RequestException({"Content-Length longer than config max body length"}, 413));
		else if (this->_body.size() > this->_contentLength)
			this->_tmpBody = this->_tmpBody.substr(0, this->_contentLength);
	}
	this->_maxBodySize = this->_validator.getMaxBodySize();
}

std::string	HTTPrequest::_encodeSpaces( std::string const& strWithSpaces) const noexcept
{
	std::string	strNoSpaces = strWithSpaces;
	size_t		spacePos = strNoSpaces.find(HTTP_SP, 0);

	while (spacePos != std::string::npos)
	{
		strNoSpaces.replace(spacePos, 1, "%20");
		spacePos = strNoSpaces.find(HTTP_SP, spacePos + 1);
	}
	return (strNoSpaces);
}

std::string	HTTPrequest::_decodeSpaces( std::string const& strNoSpaces) const noexcept
{
	std::string	strWithSpaces = strNoSpaces;
	size_t		spacePos = strWithSpaces.find("%20", 0);

	while (spacePos != std::string::npos)
	{
		strWithSpaces.replace(spacePos, 3, " ");
		spacePos = strWithSpaces.find("%20", spacePos + 1);
	}
	return (strWithSpaces);
}

void    HTTPrequest::_setMethod( std::string const& strMethod )
{
	if (strMethod == "GET")
		this->_method = HTTP_GET;
	else if (strMethod == "POST")
		this->_method = HTTP_POST;
	else if (strMethod == "DELETE")
		this->_method = HTTP_DELETE;
	else if ((strMethod == "HEAD") or
			(strMethod == "PUT") or
			(strMethod == "PATCH") or
			(strMethod == "OPTIONS") or
			(strMethod == "CONNECT"))
		throw(RequestException({"unsupported HTTP method:", strMethod}, 501));
	else
		throw(RequestException({"unknown HTTP method:", strMethod}, 400));
}

void	HTTPrequest::_setURL( std::string const& strURL )
{
	size_t		delimiter;
	std::string	tmpURL = _decodeSpaces(strURL);

	delimiter = tmpURL.find(":");
	if (delimiter != std::string::npos)		// there is the scheme (always http)
	{
		_setScheme(tmpURL.substr(0, delimiter));
		tmpURL = tmpURL.substr(delimiter + 1);
	}
	else
		_setScheme(HTTP_DEF_SCHEME);
	delimiter = tmpURL.find("//");
	if (delimiter != std::string::npos)		// there is the host (and eventually port)
	{
		if (delimiter != 0)
			throw(RequestException({"bad format URL:", strURL}, 400));
		tmpURL = tmpURL.substr(2);
		delimiter = tmpURL.find("/");
		if ((delimiter == 0) or (delimiter == std::string::npos))
			throw(RequestException({"bad format URL:", strURL}, 400));
		else if (delimiter == 0)
			_setHostPort(HTTP_DEF_HOST);
		else
			_setHostPort(tmpURL.substr(0, delimiter));
		tmpURL = tmpURL.substr(delimiter);
	}
	_setPath(tmpURL);
}

void	HTTPrequest::_setScheme( std::string const& strScheme )
{
	std::string	tmpScheme = strScheme;
	std::transform(tmpScheme.begin(), tmpScheme.end(), tmpScheme.begin(), ::toupper);
	if (tmpScheme != HTTP_DEF_SCHEME)
		throw(RequestException({"unsupported scheme:", strScheme}, 400));
	std::transform(tmpScheme.begin(), tmpScheme.end(), tmpScheme.begin(), ::tolower);
	this->_url.scheme = tmpScheme;
}

void	HTTPrequest::_setHostPort( std::string const& strURL )
{
	size_t 		delimiter = strURL.find(':');
	std::string	port;

	this->_url.host = strURL.substr(0, delimiter);
	if (delimiter != std::string::npos)	// there's the port
	{
		port = strURL.substr(delimiter + 1);
		if (port.empty())		// because    http://ABC.com:/%7esmith/home.html is still valid
		{
			this->_url.port = HTTP_DEF_PORT;
			return ;
		}
		try {
			this->_url.port = port;
		}
		catch(const std::exception& e ) {
			throw(RequestException({"invalid port format:", strURL.substr(delimiter + 1)}, 400));
		}
	}
	else
		this->_url.port = HTTP_DEF_PORT;
}

void	HTTPrequest::_setPath( std::string const& strPath )
{
	size_t		queryPos, fragmentPos;
	std::string	tmpPath=strPath;

	queryPos = tmpPath.find('?');
	fragmentPos = tmpPath.find('#');
	this->_url.path = tmpPath.substr(0, std::min(queryPos, fragmentPos));
	if (queryPos != std::string::npos)
	{
		tmpPath = tmpPath.substr(queryPos);
		_setQuery(tmpPath);
	}
	if (fragmentPos != std::string::npos)
	{
		tmpPath = tmpPath.substr(fragmentPos);
		_setFragment(tmpPath);
	}
}

void	HTTPrequest::_setQuery( std::string const& queries )
{
	std::string			key, value, keyValue=queries;
	size_t 				del1, del2;

	if (queries == "?")
		throw(RequestException({"empty query"}, 400));
	this->_url.queryRaw = keyValue.substr(1);
	while (true)
	{
		keyValue = keyValue.substr(1);	// remove leading '?' or '&'
		del1 = keyValue.find('=');
		if (del1 == std::string::npos)
			throw(RequestException({"invalid query:", keyValue}, 400));
		del2 = keyValue.find('&');
		key = keyValue.substr(0, del1);
		value = keyValue.substr(del1 + 1, del2 - del1 - 1);
		if (key.empty())
			throw(RequestException({"invalid query:", keyValue}, 400));
		this->_url.query.insert({key, value});
		if (del2 == std::string::npos)
			break;
		keyValue = keyValue.substr(del2);
	}
}

void	HTTPrequest::_setFragment( std::string const& strFragment)
{
	this->_url.fragment = strFragment.substr(1);
}

std::string	HTTPrequest::_unchunkBody( std::string const& chunkedBody )
{
	size_t		sizeChunk=0, delimiter=0;
	std::string	tmpChunkedBody, unchunkedBody;

	tmpChunkedBody = chunkedBody;
	do
	{
		delimiter = tmpChunkedBody.find(HTTP_NL);
		if (delimiter == std::string::npos)
			throw(HTTPexception({"bad chunking"}, 400));
		try {
			sizeChunk = std::stoul(tmpChunkedBody.substr(0, delimiter), nullptr, 16);
			unchunkedBody += tmpChunkedBody.substr(delimiter + HTTP_NL.size(), sizeChunk);
			tmpChunkedBody = tmpChunkedBody.substr(delimiter + sizeChunk + HTTP_NL.size() * 2);
		}
		catch(const std::exception& e){
			throw(HTTPexception({"bad chunking"}, 400));
		}
	} while (sizeChunk != 0);
	return (unchunkedBody);
}