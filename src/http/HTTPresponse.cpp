#include "HTTPresponse.hpp"

HTTPresponse::HTTPresponse( int socket, int statusCode, HTTPtype type ) :
	HTTPstruct(socket, statusCode, type) ,
	_HTMLfd(-1),
	_contentLengthWrite(0)
{
	if (isStatic() == true)
		this->_state = HTTP_RESP_HTML_READING;
	else
		this->_state = HTTP_RESP_PARSING;
}

void	HTTPresponse::parseCGI( std::string const& CGIresp )
{
	std::string headers;
	size_t		delimiter;

	if ((isCGI() == false) or (isParsingNeeded() == false))
		throw(ResponseException({"instance in wrong state or type to perfom action"}, 500));
	delimiter = CGIresp.find(HTTP_TERM);
	if (delimiter == std::string::npos)
		throw(ResponseException({"no headers terminator in CGI response"}, 500));
	delimiter += HTTP_TERM.size();
	headers = CGIresp.substr(0, delimiter - HTTP_NL.size());
	this->_tmpBody = CGIresp.substr(delimiter);
	if (this->_tmpBody.empty() == true)
		throw(ResponseException({"CGI didn't provide any body"}, 500));
	_setVersion(HTTP_DEF_VERSION);
	_setHeaders(headers);
	_addHeader(HTTP_HEADER_DATE, _getDateTime());
	HTTPstruct::_setBody(this->_tmpBody);
	this->_state = HTTP_RESP_WRITING;
	this->_strSelf = toString();
}

void	HTTPresponse::parseNotCGI( std::string const& servName )
{
	if ((isCGI() == true) or (isParsingNeeded() == false))
		throw(ResponseException({"instance in wrong state or type to perfom action"}, 500));
	_setVersion(HTTP_DEF_VERSION);
	_addHeader(HTTP_HEADER_DATE, _getDateTime());
	_addHeader(HTTP_HEADER_SERVER, servName);
	if (isDelete())				// DELETE responses are bodyless
		this->_statusCode = 204;
	else
	{
		_addHeader(HTTP_HEADER_CONT_LEN, std::to_string(this->_tmpBody.size()));
		_addHeader(HTTP_HEADER_CONT_TYPE, _getContTypeFromFile(this->_targetFile));
		if (isRedirection() == true)
		{
			if (this->_targetFile.empty() == true)
				throw(ResponseException({"redirect file target not given"}, 500));
			_addHeader(HTTP_HEADER_LOC, this->_targetFile);
		}
		HTTPstruct::_setBody(this->_tmpBody);
	}
	this->_state = HTTP_RESP_WRITING;
	this->_strSelf = toString();
}

void	HTTPresponse::readStaticFile( void )
{
    ssize_t 	readChar = -1;
    char        buffer[HTTP_BUF_SIZE];

	if ((isStatic() == false) or (isDoneReadingHTML() == true))
		throw(ResponseException({"instance in wrong state or type to perfom action2"}, 500));
	std::fill(buffer, buffer + HTTP_BUF_SIZE, 0);
	readChar = read(this->_HTMLfd, buffer, HTTP_BUF_SIZE);
	if (readChar < 0)
	{
		close(this->_HTMLfd);
		if (this->_targetFile.empty() == true)
			throw(ResponseException({"targetFile not set"}, 500));
		else
			throw(ResponseException({"file", this->_targetFile, "not available"}, 500));
	}
	this->_tmpBody += std::string(buffer, buffer + readChar);
	if (readChar < HTTP_BUF_SIZE)
		this->_state = HTTP_RESP_PARSING;
}

// Function to convert file_time_type to string
static std::string fileTimeToString(std::filesystem::file_time_type time)
{
	auto timePoint = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
	std::time_t t = std::chrono::system_clock::to_time_t(timePoint);
	std::stringstream ss;
	ss << std::put_time(std::localtime(&t), "%d/%m/%Y %H:%M:%S");
	return ss.str();
}

static std::string formatSize(uintmax_t size)
{
	const char* suffixes[] = {"bytes", "kB", "MB", "GB"};
	int suffixIndex = 0;
	double size_d = static_cast<double>(size);

	while (size_d >= 1024 && suffixIndex < 3) {
		size_d /= 1024;
		suffixIndex++;
	}

	std::ostringstream oss;
	oss << std::fixed << std::setprecision(2);

	if (size_d - std::floor(size_d) < 0.01) {
		oss << std::setprecision(0);
	}

	oss << size_d << " " << suffixes[suffixIndex];
	return oss.str();
}

void	HTTPresponse::listContentDirectory( void )
{
	// index of ....			[Header]
	// ------------------------	[break]
	// parent dir				[Parent directory]
	// Name Size DateModified	[list]
	// Folders					[folder/ [DD/MM/YYYY, HH:MM::SS]]
	// Files [file 3DigitSize	[DD/MM/YYYY, HH:MM::SS]]
	std::set<std::filesystem::directory_entry> folders;
	std::set<std::filesystem::directory_entry> files;

	// Populating folders and files sets
	if ((isAutoIndex() == false) or (this->_state != HTTP_RESP_PARSING))
		throw(ResponseException({"instance in wrong state or type to perfom action2"}, 500));
	for (const auto& entry : std::filesystem::directory_iterator(this->_targetFile))
	{
		if (std::filesystem::is_directory(entry))
			folders.insert(entry);
		else
			files.insert(entry);
	}
	// Header part of the html:
	_tmpBody += R"(
		<!DOCTYPE html>
		<html lang="en">
		<head>
			<meta charset="UTF-8">
			<meta name="viewport" content="width=device-width, initial-scale=1.0">
			<title>Directory Listing</title>
			<style>
				body {
					font-family: Arial, sans-serif;
					margin: 0;
					padding: 0;
					background-color: #1a1a1a;
					color: #fff;
				}

				.container {
					max-width: 800px;
					margin: 0 auto;
					padding: 20px;
					background-color: #333;
					border-radius: 8px;
				}

				table {
					width: 100%;
					border-collapse: collapse;
					margin-top: 20px;
					background-color: #444;
				}

				table, th, td {
					border: 1px solid #222;
				}

				th, td {
					padding: 8px;
					text-align: left;
				}

				th {
					background-color: #2c2c2c;
				}

				tr:nth-child(even) {
					background-color: #333;
				}

				tr:hover {
					background-color: #555;
				}

				a {
					color: #4bb7ff;
					text-decoration: none;
				}

				a:hover {
					text-decoration: underline;
				}
			</style>
		</head>
		<body>
		<div class="container">
		<h1>Index of )" + this->_targetFile.string().substr(_root.string().length()) + "/" + "</h1><hr>";
	std::string parentDir = this->_targetFile.parent_path().string() + "/";
	std::string tmpRoot = _root.string();
	size_t i = 0;
	while (i < tmpRoot.length() && parentDir[i] == tmpRoot[i])
		i++;
	if (!parentDir.empty())
		_tmpBody += "<tr><td><a href=\"" + parentDir.substr(i) + "\">[Parent directory]</a></td><td></td><td></td></tr>";
	_tmpBody += "<table><thead><tr><th>Name</th><th>Size</th><th>Date Modified</th></tr></thead><tbody>";
	// Inserting folders into HTML
	std::string name;
	std::string path;
	for (const auto& folder : folders)
	{
		name = folder.path().filename().string();
		path = std::filesystem::weakly_canonical(folder).string().substr(_root.string().length());
		_tmpBody += "<tr><td><a href=\"" + path + "/" + "\">" + name + "/" + "</a></td><td>" + "</td><td>" + fileTimeToString(std::filesystem::last_write_time(folder)) + "</td></tr>";
	}
	// Inserting files into HTML
	for (const auto& file : files)
	{
		name = file.path().filename().string();
		path = std::filesystem::weakly_canonical(file).string().substr(_root.string().length());
		_tmpBody += "<tr><td><a href=\"" + path + "\">" + name + "</a></td><td>" + formatSize(std::filesystem::file_size(file)) +  "</td><td>" + fileTimeToString(std::filesystem::last_write_time(file)) + "</td></tr>";
	}
	_tmpBody += "</tbody></table></div></body></html>";
}

void	HTTPresponse::removeFile( void ) const
{
	if ((isDelete() == false) or (this->_state != HTTP_RESP_PARSING))
		throw(ResponseException({"instance in wrong state or type to perfom action2"}, 500));
	if (std::remove(this->_targetFile.c_str()) < 0)
		throw(ResponseException({"resource", this->_targetFile, "could not be deleted"}, 500));
}

void	HTTPresponse::writeContent( void )
{
    ssize_t writtenChars = -1;
	size_t	charsToWrite = 0;

	if (isDoneWriting() == true)
		throw(ResponseException({"instance in wrong state or type to perfom action"}, 500));
	else if (this->_contentLengthWrite == 0)
		_resetTimeout();
	if ((this->_strSelf.size()) < HTTP_BUF_SIZE)
	{
		charsToWrite = this->_strSelf.size();
		this->_state = HTTP_RESP_DONE;
	}
	else
		charsToWrite = HTTP_BUF_SIZE;
	writtenChars = send(this->_socket, this->_strSelf.substr(0, charsToWrite).c_str(), charsToWrite, 0);
	if (writtenChars < 0)
		throw(ServerException({"socket not available"}));
	else if (writtenChars == 0)
		_checkTimeout();
	else
	{
		this->_contentLengthWrite += writtenChars;
		this->_strSelf = this->_strSelf.substr(writtenChars);
		if (this->_strSelf.empty() == true)
			this->_state = HTTP_RESP_DONE;
	}
}

void	HTTPresponse::errorReset( int errorStatus, bool hardCode ) noexcept
{
	this->_statusCode = errorStatus;
	this->_HTMLfd = -1;
	this->_contentLengthWrite = 0;
	this->_targetFile.clear();
	this->_headers.clear();
	this->_root.clear();
	if (hardCode == true)
	{
		this->_targetFile = "500.html";
		this->_tmpBody = ERROR_500_CONTENT;
		this->_state = HTTP_RESP_PARSING;
	}
	else
	{
		this->_tmpBody.clear();
		this->_state = HTTP_RESP_HTML_READING;
	}
	this->_type = HTTP_STATIC;
}

std::string	HTTPresponse::toString( void ) const noexcept
{
	std::string	strResp;

	strResp += this->_version.scheme;
	strResp += "/";
	strResp += std::to_string(this->_version.major);
	strResp += ".";
	strResp += std::to_string(this->_version.minor);
	strResp += HTTP_SP;
	strResp += std::to_string(this->_statusCode);
	strResp += HTTP_SP;
	strResp += this->_mapStatusCode(this->_statusCode);
	strResp += HTTP_NL;
	if (!this->_headers.empty())
	{
		for (auto item : this->_headers)
		{
			strResp += item.first;
			strResp += ":";
			strResp += HTTP_SP;
			strResp += item.second;
			strResp += HTTP_NL;
		}
	}
	strResp += HTTP_NL;
	if (!this->_body.empty())
	{
		strResp += this->_body;
		strResp += HTTP_TERM;
	}
	return (strResp);
}

int		HTTPresponse::getHTMLfd( void ) const noexcept
{
	return (this->_HTMLfd);
}

void	HTTPresponse::setTargetFile( path_t const& targetFile)
{
	if (isStatic() == true)
	{
		if (this->_HTMLfd != -1)
			throw(ResponseException({"already reading file", this->_targetFile}, 500));
		this->_HTMLfd = open(targetFile.c_str(), O_RDONLY);
		if (this->_HTMLfd == -1)
			throw(ResponseException({"invalid file descriptor"}, 500));
	}
	this->_targetFile = targetFile;
}

bool	HTTPresponse::isDoneReadingHTML( void ) const noexcept
{
	return (this->_state > HTTP_RESP_HTML_READING);
}

bool	HTTPresponse::isParsingNeeded( void ) const noexcept
{
	return (this->_state == HTTP_RESP_PARSING);
}

bool	HTTPresponse::isDoneWriting( void ) const noexcept
{
	return (this->_state == HTTP_RESP_DONE);
}

void	HTTPresponse::_setHeaders( std::string const& strHeaders )
{
	size_t	delimiter = 0;
	int		statusCode = -1;

	HTTPstruct::_setHeaders(strHeaders);
	if (this->_headers.count(HTTP_HEADER_STATUS) == 0)
		throw(ResponseException({"missing Status header in CGI response"}, 500));
	delimiter = this->_headers.find(HTTP_HEADER_STATUS)->second.find(HTTP_SP);
	try {
		statusCode = std::stoi(this->_headers.find(HTTP_HEADER_STATUS)->second.substr(0, delimiter));
	}
	catch (const std::exception& e) {
		throw(ResponseException({"invalid status code:", this->_headers.find(HTTP_HEADER_STATUS)->second}, 500));
	}
	if (statusCode >= 400)
		throw(ResponseException({"error while running CGI"}, statusCode));
	if (this->_headers.find(HTTP_HEADER_SERVER) == this->_headers.end()
		|| this->_headers.find(HTTP_HEADER_CONT_TYPE) == this->_headers.end()
		|| this->_headers.find(HTTP_HEADER_CONT_LEN) == this->_headers.end())
	{
		throw(ResponseException({"missing mandatory header(s) in CGI response"}, 500));
	}

	if (this->_type == HTTP_CGI_FILE_UPL)
	{
		if (this->_headers.count(HTTP_HEADER_LOC) == 0)
			throw(ResponseException({"missing Location header in CGI response"}, 500));
		if (statusCode != 201)
			throw(ResponseException({"file upload needs status code 201, given:", std::to_string(this->_statusCode)}, 500));
		this->_targetFile = this->_headers.find(HTTP_HEADER_LOC)->second;
	}
	this->_statusCode = statusCode;
}

std::string	HTTPresponse::_mapStatusCode( int status) const
{
	std::map<int, const char*> mapStatus =
	{
		// Information responses
		{100, "Continue"},				// This interim response indicates that the client should continue the request or ignore the response if the request is already finished.
		{101, "Switching Protocols"},	// This code is sent in response to an Upgrade request header from the client and indicates the protocol the server is switching to.
		{102, "Processing"},			// This code indicates that the server has received and is processing the request, but no response is available yet.
		{103, "Early Hints"},			// This status code is primarily intended to be used with the Link header, letting the user agent start preloading resources while the server prepares a response or preconnect to an origin from which the page will need resources.
		// Successful responses
		{200, "OK"},							// The request succeeded. The result meaning of "success" depends on the HTTP method
		{201, "Created"},						// The request succeeded, and a new resource was created as a result. This is typically the response sent after POST requests, or some PUT requests.
		{202, "Accepted"},						// The request has been received but not yet acted upon. It is noncommittal, since there is no way in HTTP to later send an asynchronous response indicating the outcome of the request. It is intended for cases where another process or server handles the request, or for batch processing.
		{203, "Non-Authoritative Information"},	// This response code means the returned metadata is not exactly the same as is available from the origin server, but is collected from a local or a third-party copy. This is mostly used for mirrors or backups of another resource. Except for that specific case, the 200 OK response is preferred to this status.
		{204, "No Content"},					// There is no content to send for this request, but the headers may be useful. The user agent may update its cached headers for this resource with the new ones.
		{205, "Reset Content"},					// Tells the user agent to reset the document which sent this request.
		{206, "Partial Content"},				// This response code is used when the Range header is sent from the client to request only part of a resource.
		{207, "Multi-Status"},					// Conveys information about multiple resources, for situations where multiple status codes might be appropriate.
		{208, "Already Reported"},				// Used inside a <dav:propstat> response element to avoid repeatedly enumerating the internal members of multiple bindings to the same collection.
		{226, "IM Used"},						// The server has fulfilled a GET request for the resource, and the response is a representation of the result of one or more instance-manipulations applied to the current instance.
		// Redirection messages
		{300, "Multiple Choices"},		// The request has more than one possible response. The user agent or user should choose one of them. (There is no standardized way of choosing one of the responses, but HTML links to the possibilities are recommended so the user can pick.)
		{301, "Moved Permanently"},		// The URL of the requested resource has been changed permanently. The new URL is given in the response.
		{302, "Found"},					// This response code means that the URI of requested resource has been changed temporarily. Further changes in the URI might be made in the future. Therefore, this same URI should be used by the client in future requests.
		{303, "See Other"},				// The server sent this response to direct the client to get the requested resource at another URI with a GET request.
		{304, "Not Modified"},			// This is used for caching purposes. It tells the client that the response has not been modified, so the client can continue to use the same cached version of the response.
		{305, "Use Proxy"},				// Defined in a previous version of the HTTP specification to indicate that a requested response must be accessed by a proxy. It has been deprecated due to security concerns regarding in-band configuration of a proxy.
		{306, "unused"},				// This response code is no longer used; it is just reserved. It was used in a previous version of the HTTP/1.1 specification.
		{307, "Temporary Redirect"},	// The server sends this response to direct the client to get the requested resource at another URI with the same method that was used in the prior request. This has the same semantics as the 302 Found HTTP response code, with the exception that the user agent must not change the HTTP method used: if a POST was used in the first request, a POST must be used in the second request.
		{308, "Permanent Redirect"},	// This means that the resource is now permanently located at another URI, specified by the Location: HTTP Response header. This has the same semantics as the 301 Moved Permanently HTTP response code, with the exception that the user agent must not change the HTTP method used: if a POST was used in the first request, a POST must be used in the second request.
		// Client error responses
		{400, "Bad Request"},						// The server cannot or will not process the request due to something that is perceived to be a client error (e.g., malformed request syntax, invalid request message framing, or deceptive request routing).
		{401, "Unauthorized"},						// Although the HTTP standard specifies "unauthorized", semantically this response means "unauthenticated". That is, the client must authenticate itself to get the requested response.
		{402, "Payment Required"},					// This response code is reserved for future use. The initial aim for creating this code was using it for digital payment systems, however this status code is used very rarely and no standard convention exists.
		{403, "Forbidden"},							// The client does not have access rights to the content; that is, it is unauthorized, so the server is refusing to give the requested resource. Unlike 401 Unauthorized, the client's identity is known to the server.
		{404, "Not Found"},							// The server cannot find the requested resource. In the browser, this means the URL is not recognized. In an API, this can also mean that the endpoint is valid but the resource itself does not exist. Servers may also send this response instead of 403 Forbidden to hide the existence of a resource from an unauthorized client. This response code is probably the most well known due to its frequent occurrence on the web.
		{405, "Method Not Allowed"},				// The request method is known by the server but is not supported by the target resource. For example, an API may not allow calling DELETE to remove a resource.
		{406, "Not Acceptable"},					// This response is sent when the web server, after performing server-driven content negotiation, doesn't find any content that conforms to the criteria given by the user agent.
		{407, "Proxy Authentication Required"},		// This is similar to 401 Unauthorized but authentication is needed to be done by a proxy.
		{408, "Request Timeout"},					// This response is sent on an idle connection by some servers, even without any previous request by the client. It means that the server would like to shut down this unused connection. This response is used much more since some browsers, like Chrome, Firefox 27+, or IE9, use HTTP pre-connection mechanisms to speed up surfing. Also note that some servers merely shut down the connection without sending this message.
		{409, "Conflict"},							// This response is sent when a request conflicts with the current state of the server.
		{410, "Gone"},								// This response is sent when the requested content has been permanently deleted from server, with no forwarding address. Clients are expected to remove their caches and links to the resource. The HTTP specification intends this status code to be used for "limited-time, promotional services". APIs should not feel compelled to indicate resources that have been deleted with this status code.
		{411, "Length Required"},					// Server rejected the request because the Content-Length header field is not defined and the server requires it.
		{412, "Precondition Failed"},				// The client has indicated preconditions in its headers which the server does not meet.
		{413, "Payload Too Large"},					// Request entity is larger than limits defined by server. The server might close the connection or return an Retry-After header field.
		{414, "URI Too Long"},						// The URI requested by the client is longer than the server is willing to interpret.
		{415, "Unsupported Media Type"},			// The media format of the requested data is not supported by the server, so the server is rejecting the request.
		{416, "Range Not Satisfiable"},				// The range specified by the Range header field in the request cannot be fulfilled. It's possible that the range is outside the size of the target URI's data.
		{417, "Expectation Failed"},				// This response code means the expectation indicated by the Expect request header field cannot be met by the server.
		{418, "I'm a teapot"},						// The server refuses the attempt to brew coffee with a teapot.
		{421, "Misdirected Request"},				// The request was directed at a server that is not able to produce a response. This can be sent by a server that is not configured to produce responses for the combination of scheme and authority that are included in the request URI.
		{422, "Unprocessable Content"},				// The request was well-formed but was unable to be followed due to semantic errors.
		{423, "Locked"},							// The resource that is being accessed is locked.
		{424, "Failed Dependency"},					// The request failed due to failure of a previous request.
		{425, "Too Early"},							// Indicates that the server is unwilling to risk processing a request that might be replayed.
		{426, "Upgrade Required"},					// The server refuses to perform the request using the current protocol but might be willing to do so after the client upgrades to a different protocol. The server sends an Upgrade header in a 426 response to indicate the required protocol(s).
		{428, "Precondition Required"},				// The origin server requires the request to be conditional. This response is intended to prevent the 'lost update' problem, where a client GETs a resource's state, modifies it and PUTs it back to the server, when meanwhile a third party has modified the state on the server, leading to a conflict.
		{429, "Too Many Requests"},					// The user has sent too many requests in a given amount of time ("rate limiting").
		{431, "Request Header Fields Too Large"},	// The server is unwilling to process the request because its header fields are too large. The request may be resubmitted after reducing the size of the request header fields.
		{451, "Unavailable For Legal Reasons"},		// The user agent requested a resource that cannot legally be provided, such as a web page censored by a government.
		// Server error responses
		{500, "Internal Server Error"},				// The server has encountered a situation it does not know how to handle.
		{501, "Not Implemented"},					// The request method is not supported by the server and cannot be handled. The only methods that servers are required to support (and therefore that must not return this code) are GET and HEAD.
		{502, "Bad Gateway"},						// This error response means that the server, while working as a gateway to get a response needed to handle the request, got an invalid response.
		{503, "Service Unavailable"},				// The server is not ready to handle the request. Common causes are a server that is down for maintenance or that is overloaded. Note that together with this response, a user-friendly page explaining the problem should be sent. This response should be used for temporary conditions and the Retry-After HTTP header should, if possible, contain the estimated time before the recovery of the service. The webmaster must also take care about the caching-related headers that are sent along with this response, as these temporary condition responses should usually not be cached.
		{504, "Gateway Timeout"},					// This error response is given when the server is acting as a gateway and cannot get a response in time.
		{505, "HTTP Version Not Supported"},		// The HTTP version used in the request is not supported by the server.
		{506, "Variant Also Negotiates"},			// The server has an internal configuration error: the chosen variant resource is configured to engage in transparent content negotiation itself, and is therefore not a proper end point in the negotiation process.
		{507, "Insufficient Storage"},				// The method could not be performed on the resource because the server is unable to store the representation needed to successfully complete the request.
		{508, "Loop Detected"},						// The server detected an infinite loop while processing the request.
		{510, "Not Extended"},						// Further extensions to the request are required for the server to fulfill it.
		{511, "Network Authentication Required"},	// Indicates that the client needs to authenticate to gain network access.
	};

	try
	{
		return (std::string(mapStatus.at(status)));
	}
	catch(const std::out_of_range& e) {
		throw(ResponseException({"Unknown HTTP response code:", std::to_string(status)}, 500));
	}
}

std::string	HTTPresponse::_getDateTime( void ) const noexcept
{
	std::time_t rawtime;
	std::tm* timeinfo;
	char buffer[80];

	std::time(&rawtime);
	timeinfo = std::gmtime(&rawtime);
	std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	return (std::string(buffer));
}

std::string	HTTPresponse::_getContTypeFromFile( path_t const& fileName) const noexcept
{
	if ((fileName.extension() == ".html") or isAutoIndex())
		return (HTML_CONTENT_TYPE);
	else if (fileName.extension() == ".css")
		return (CSS_CONTENT_TYPE);
	else if (fileName.extension() == ".js")
		return (JS_CONTENT_TYPE);
	else if ((fileName.extension() == ".jpg") or (fileName.extension() == ".jpeg"))
		return (JPG_CONTENT_TYPE);
	else if (fileName.extension() == ".png")
		return (PNG_CONTENT_TYPE);
	else if (fileName.extension() == ".ico")
		return (ICO_CONTENT_TYPE);
	else
		return (PLAIN_CONTENT_TYPE);
}
