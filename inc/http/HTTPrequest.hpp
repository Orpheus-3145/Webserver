#pragma once

#include <sys/types.h>        // send, recv
#include <sys/socket.h>       // send, recv
#include <fstream>

#include "HTTPstruct.hpp"
#include "Config.hpp"
#include "RequestValidate.hpp"

#define MAX_HEADER_SIZE	8192

typedef enum HTTPreqState_f
{
	HTTP_REQ_HEAD_READING,
	HTTP_REQ_HEAD_PARSING,
	HTTP_REQ_BODY_READING,
	HTTP_REQ_DONE,
}	HTTPreqState;

typedef struct HTTPurl_f
{
	std::string	scheme;
	std::string	host;
	std::string	port;
	path_t		path;
	t_dict		query;
	std::string queryRaw;
	std::string fragment;

} HTTPurl;

typedef std::vector<Config> t_serv_list;

class HTTPrequest : public HTTPstruct
{
	public:
		HTTPrequest( int socket, t_serv_list const& servers ) :
			HTTPstruct(socket, 200, HTTP_STATIC),
			_state(HTTP_REQ_HEAD_READING),
			_method(HTTP_GET),
			_validator(servers),
			_contentLength(0) ,
			_maxBodySize(-1) {};
		virtual ~HTTPrequest( void ) override {};

		void		parseHead( void );
		void		parseBody( void );
		std::string	toString( void ) const noexcept override;
		void		updateErrorCode( int ) ;

		std::string		 	getMethod( void ) const noexcept;
		std::string			getHost( void ) const noexcept;
		std::string		 	getPort( void ) const noexcept;
		size_t			 	getContentLength( void ) const noexcept;
		std::string	const&	getQueryRaw( void ) const noexcept;
		std::string	const	getCookie( void ) const noexcept;
		std::string			getContentTypeBoundary( void ) const noexcept;
		std::string const&	getServName( void ) const noexcept;
		path_t const&		getRealPath( void ) const noexcept;
		path_t const&		getRedirectPath( void ) const noexcept;
		path_t const&		getRoot( void ) const noexcept;

		bool	isEndConn( void ) noexcept;
		bool	isChunked( void ) const noexcept;
		bool	isDoneReadingHead( void ) const noexcept;
		bool	isDoneReadingBody( void ) const noexcept;
		bool	hasBodyToRead( void ) const noexcept;

	protected:
		HTTPreqState	_state;
		HTTPmethod		_method;
		HTTPurl			_url;

		RequestValidate	_validator;

		std::string _tmpHead;
		size_t		_contentLength, _maxBodySize;

		void		_setHead( std::string const& ) override;
		void		_setHeaders(std::string const& ) override;
		void		_setVersion( std::string const& ) override;
		void		_setBody( std::string const& ) override;
		void		_readHead( void );
		void		_readBody( void );
		void		_updateTypeAndState( void );
		void		_checkMaxBodySize( void );
		std::string	_encodeSpaces( std::string const&) const noexcept;
		std::string	_decodeSpaces( std::string const&) const noexcept;

		void	_setMethod( std::string const& );
		void	_setURL( std::string const& );
		void	_setScheme( std::string const& );
		void	_setHostPort( std::string const& );
		void	_setPath( std::string const& );
		void	_setQuery( std::string const& );
		void	_setFragment( std::string const& );
	
		std::string	_unchunkBody( std::string const& );
};
