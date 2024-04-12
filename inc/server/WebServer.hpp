#pragma once
#include <unordered_map>
#include <netinet/in.h>     // socket, accept, listen, bind, connect
#include <arpa/inet.h>      // htons, htonl, ntohs, ntohl
#include <sys/poll.h>     	// poll
#include <algorithm>
#include <signal.h>         // kill, signal
#include <iostream>			// I/O streaming
#include <fstream>			// file streaming
#include <netdb.h>          // gai_strerror, getaddrinfo, freeaddrinfo
#include <cerrno>           // errno
#include <string>			// std::string class
#include <vector>
#include <chrono>			// timeout handling

#include "HTTPresponse.hpp"
#include "HTTPrequest.hpp"
#include "Exceptions.hpp"
#include "Config.hpp"
#include "CGI.hpp"

#define BACKLOG 			10		// max pending connection queued up
#define SERVER_DEF_PAGES	path_t("default/errors")
#define CONN_MAX_TIMEOUT	7

using namespace std::chrono;

enum fdType
{
    LISTENER,
    CLIENT_CONNECTION,
    CGI_REQUEST_PIPE_WRITE_END,
    CGI_RESPONSE_PIPE_READ_END,
    STATIC_FILE
};

enum fdState
{
	WAITING_FOR_CONNECTION,			// LISTENER (read)
	READ_REQ_HEADER,				// CLIENT_CONNECTION (read)
	READ_STATIC_FILE,				// STATIC_FILE (read)
	READ_REQ_BODY,					// CLIENT_CONNECTION (read)
	WAIT_FOR_CGI,					// CLIENT_CONNECTION (no action)
	READ_CGI_RESPONSE,				// CGI_RESPONSE_PIPE (read)
	WRITE_TO_CLIENT,				// CLIENT_CONNECTION (write)
	WRITE_TO_CGI					// CGI_REQUEST_PIPE (write)
};

typedef struct PollItem
{
	fdType  					pollType;
    fdState 					pollState;
	std::string					IPaddr;
	std::string					port;
	steady_clock::time_point	lastActivity;
} t_PollItem;

class WebServer
{
	public:
		WebServer ( t_serv_list const& );
		~WebServer ( void ) noexcept;

		void	run( void );

	private:
		t_serv_list	 							_servers;
		std::vector<struct pollfd>	 			_pollfds;
		std::unordered_map<int, t_PollItem*>	_pollitems;
		std::unordered_map<int, HTTPrequest*> 	_requests;
		std::unordered_map<int, HTTPresponse*> 	_responses;
		std::unordered_map<int, CGI*> 			_cgi;	// NOTE: the key is the client socket fd, not any of the cgi-pipes
		std::vector<int>						_emptyConns;

		void		_listenTo( std::string const&, std::string const& );
		void		_readData( int );
		void		_writeData( int );
		void		_addConn( int , fdType , fdState, std::string const& ip="", std::string const& port="" );
		void		_dropConn( int ) noexcept;
		void		_clearEmptyConns( void ) noexcept;
		void		_clearStructs( int, bool ) noexcept;
		int			_getSocketFromFd( int );
		t_serv_list	_getServersFromIP( std::string const&, std::string const& ) const noexcept;
		path_t		_getDefErrorPage( int ) const ;

		void	_resetTimeout( int );
		void	_checkTimeout( int );

		void	handleNewConnections( int ); // keep - DONE
		void	readRequestHeaders( int ); // keep / rework
		void	readStaticFiles( int ); // keep / rework
		void	readRequestBody( int );
		void	readCGIResponses( int ); // keep / rework
		void	writeToCGI( int item );
		void	writeToClients( int );
		void	redirectToErrorPage( int, int ) noexcept;
};
