#pragma once
#include <string>
#include <initializer_list>

#define WEBSERV_ERR_GENERIC "generic error"
#define WEBSERV_ERR_PARSING "config file parsing error"
#define WEBSERV_ERR_SERVER "internal server error"
#define WEBSERV_ERR_HTTP "HTTP generic error"
#define WEBSERV_ERR_HTTP_REQ "HTTP request error"
#define WEBSERV_ERR_HTTP_RESP "HTTP response error"
#define WEBSERV_ERR_HTTP_CGI "HTTP cgi error"

class WebservException : public std::exception
{
	public:
		WebservException( void ) noexcept : std::exception() {};
		WebservException( std::initializer_list<std::string> const&, std::string const& prompt=WEBSERV_ERR_GENERIC ) noexcept;
		virtual ~WebservException( void ) noexcept {};

		virtual const char* what( void ) const noexcept override {return (this->_fullInfo.c_str());};
		std::string const&	getInfo( void ) const noexcept {return (this->_info);};

	protected:
		std::string	_prompt, _info, _fullInfo;
};

class ParserException : public WebservException
{
	public:
		ParserException( std::initializer_list<std::string> const& args) noexcept
			: WebservException(args, WEBSERV_ERR_PARSING) {};
};

class ServerException : public WebservException
{
	public:
		ServerException( std::initializer_list<std::string> const& args) noexcept
			: WebservException(args, WEBSERV_ERR_SERVER) {};
};

class EndConnectionException : public WebservException
{
	public:
		EndConnectionException( void ) noexcept
			: WebservException() {};
};

class HTTPexception : public WebservException
{
	public:
		HTTPexception( std::initializer_list<std::string> const& args, int status, std::string const& prompt=WEBSERV_ERR_HTTP ) noexcept;
		HTTPexception( HTTPexception const&, std::string const& prompt=WEBSERV_ERR_HTTP ) noexcept;

		int	getStatus( void ) const noexcept {return (this->_status);};
	
	protected:
		int	_status;
};

class RequestException : public HTTPexception
{
	public:
		RequestException( std::initializer_list<std::string> const& args, int status ) noexcept
			: HTTPexception(args, status, WEBSERV_ERR_HTTP_REQ) {};
		RequestException( HTTPexception const& instance ) noexcept
			: HTTPexception(instance, WEBSERV_ERR_HTTP_REQ) {};
};

class ResponseException : public HTTPexception
{
	public:
		ResponseException( std::initializer_list<std::string> const& args, int status ) noexcept
			: HTTPexception(args, status, WEBSERV_ERR_HTTP_RESP) {};
		ResponseException( HTTPexception const& instance ) noexcept
			: HTTPexception(instance, WEBSERV_ERR_HTTP_RESP) {};
};

class CGIexception : public HTTPexception
{
	public:
		CGIexception( std::initializer_list<std::string> const& args, int status ) noexcept
			: HTTPexception(args, status, WEBSERV_ERR_HTTP_CGI) {};
		CGIexception( HTTPexception const& instance) noexcept
			: HTTPexception(instance, WEBSERV_ERR_HTTP_CGI) {};
};
