#include "HTTPstruct.hpp"

HTTPtype	HTTPstruct::getType( void ) const noexcept
{
	return(this->_type);
}

int		HTTPstruct::getSocket( void ) const noexcept
{
	return (this->_socket);
}

int	HTTPstruct::getStatusCode( void ) const noexcept
{
	return (this->_statusCode);
}

std::string const&	HTTPstruct::getTmpBody( void ) const noexcept
{
	return (this->_tmpBody);
}

void	HTTPstruct::setTmpBody( std::string const& tmpBody ) noexcept
{
    this->_tmpBody = tmpBody;
}

path_t const&	HTTPstruct::getRoot( void ) const noexcept
{
	return (this->_root);
}

void	HTTPstruct::setRoot( path_t const& newRoot ) noexcept
{
	this->_root = newRoot;
}

bool	HTTPstruct::isStatic( void ) const noexcept
{
	return (this->_type == HTTP_STATIC);
}

bool	HTTPstruct::isRedirection( void ) const noexcept
{
	return (this->_type == HTTP_REDIRECTION);
}

bool	HTTPstruct::isAutoIndex( void ) const noexcept
{
	return (this->_type == HTTP_AUTOINDEX);
}

bool	HTTPstruct::isFastCGI( void ) const noexcept
{
	return (this->_type == HTTP_CGI_STATIC);
}

bool	HTTPstruct::isFileUpload( void ) const noexcept
{
	return (this->_type == HTTP_CGI_FILE_UPL);
}

bool	HTTPstruct::isDelete( void ) const noexcept
{
	return (this->_type == HTTP_FILE_DEL);
}

bool	HTTPstruct::isCGI( void ) const noexcept
{
	return (isFastCGI() || this->_type == HTTP_CGI_FILE_UPL);
}

void	HTTPstruct::_setHeaders( std::string const& headers )
{
	size_t 		nextHeader, delimHeader;
	std::string key, value, tmpHeaders=headers;

	if (tmpHeaders.empty())
		return ;
	nextHeader = tmpHeaders.find(HTTP_NL);
	while (nextHeader != std::string::npos)
	{
		delimHeader = tmpHeaders.find(": ");
		if (delimHeader == std::string::npos)
			throw(HTTPexception({"invalid header format:", tmpHeaders.substr(0, nextHeader)}, 400));
		key = tmpHeaders.substr(0, delimHeader);
		value = tmpHeaders.substr(delimHeader + 2, nextHeader - delimHeader - 2);
		tmpHeaders = tmpHeaders.substr(nextHeader + HTTP_NL.size());
		_addHeader(key, value);
		nextHeader = tmpHeaders.find(HTTP_NL);
	}
}

void	HTTPstruct::_setBody( std::string const& tmpBody )
{
    this->_body = tmpBody;
}

void	HTTPstruct::_setVersion( std::string const& strVersion )
{
	size_t		del1, del2;
	std::string scheme;
	int			major=0, minor=0;

	del1 = strVersion.find('/');
	if (del1 == std::string::npos)
		throw(HTTPexception({"invalid version:", strVersion}, 400));
	scheme = strVersion.substr(0, del1);
	std::transform(scheme.begin(), scheme.end(), scheme.begin(), ::toupper);
	if (scheme != HTTP_DEF_SCHEME)
		throw(HTTPexception({"invalid scheme:", strVersion}, 400));
	del2 = strVersion.find('.');
	if (del2 == std::string::npos)
		throw(HTTPexception({"invalid version:", strVersion}, 400));
	try {
		major = std::stoi(strVersion.substr(del1 + 1, del2 - del1 - 1));
		minor = std::stoi(strVersion.substr(del2 + 1));
	}
	catch (std::exception const& e) {
		throw(HTTPexception({"invalid version numbers:", strVersion}, 400));
	}
	if (major !=1 or minor != 1)
		throw(HTTPexception({"unsupported HTTP version:", strVersion}, 505));
	this->_version.scheme = scheme;
	this->_version.major = major;
	this->_version.minor = minor;
}

void	HTTPstruct::_resetTimeout( void ) noexcept
{
	this->_lastActivity = steady_clock::now();
}

void	HTTPstruct::_checkTimeout( void )
{
	duration<double> 	time_span;

	time_span = duration_cast<duration<int>>(steady_clock::now() - this->_lastActivity);
	if (time_span.count() > HTTP_MAX_TIMEOUT)
		throw(RequestException({"timeout request"}, 408));
}

void	HTTPstruct::_addHeader(std::string const& name, std::string const& content) noexcept
{
	this->_headers.insert({name, content});
}
