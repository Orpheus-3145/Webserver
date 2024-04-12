/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Client.hpp                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: itopchu <itopchu@student.42.fr>              +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/11/26 20:16:40 by fra           #+#    #+#                 */
/*   Updated: 2024/03/26 00:46:23 by fra           ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

// #pragma once
#include <unistd.h>           // execve, dup, dup2, pipe, fork, access
#include <cstring>           // strerror
#include <netdb.h>            // gai_strerror, getaddrinfo, freeaddrinfo
#include <cerrno>            // errno
#include <sys/socket.h>       // socketpair, htons, htonl, ntohs, ntohl, select
#include <sys/epoll.h>     // epoll_create, epoll_ctl, epoll_wait
#include <sys/poll.h>     // poll
#include <netinet/in.h>       // socket, accept, listen, bind, connect
#include <arpa/inet.h>        // htons, htonl, ntohs, ntohl
#include <sys/types.h>        // send, recv
#include <sys/socket.h>       // send, recv
#include <sys/types.h>        // chdir
#include <sys/stat.h>         // stat
#include <sys/wait.h>         // waitpid
#include <fcntl.h>            // open
#include <dirent.h>           // opendir, readdir, closedir
#include <signal.h>           // kill, signal
#include <filesystem>           // filesystem::path, 
#include <iostream>
#include <initializer_list>
#include <string>
#include <vector>
#include <map>
#define STD_NOBODY "GET http://Pino:21/home/faru/Documents/Codam/webserv/test.txt HTTP/1.1\r\nkey1: value1\r\nkey2: value2\r\nHost: Pino\r\n\r\n"
#define STD_REQUEST "GET http://Pino:21/home/faru/Documents/Codam/webserv/test.txt HTTP/1.1\r\nHost: example1.com\r\tTransfer-Encoding: chunked\r\n\r\n7\r\nMozilla\r\n11\r\nDeveloper Network\r\n0\r\n\r\n"
#define STD_REQUEST_CHK "GET / HTTP/1.1\r\nHost: example1.com\r\nContent-Type: text/plain\r\nTransfer-Encoding: chunked\r\n\r\n7\r\nMozilla\r\n11\r\nDeveloper Network\r\n0\r\n\r\n"
#define STD_REQUEST_CHK2 "POST http://localhost:8080/cgi-bin/uploadfile.cgi HTTP/1.1\r\nHost: localhost\r\nContent-Type: multipart/form-data; boundary=---------------------------18151306832880448203817778605\r\nTransfer-Encoding: chunked\r\n\r\n14\r\n--------------------\r\n38\r\n---------18151306832880448203817778605\nContent-Dispositi\r\n48\r\non: form-data; name=\"filename\"; filename=\"test_up\"\nContent-Type: applica\r\n17\r\ntion/octet-stream\n\na\na\n\r\n17\r\na\na\n-------------------\r\n29\r\n----------18151306832880448203817778605--\r\n0\r\n\r\n"
#define STD_REQUEST_BODY "GET / HTTP/1.1\r\nHost: example1.com\r\nContent-Length: 5\r\n\r\naaaaa"
#define STD_FULL "GET http://test:21/halo/find/me/here?bcd=123&gasd=255#sectione HTTP/1.1\r\nkey1: value1\r\nkey2: value2\r\n\r\nmuch body very http\r\n\r\n"
#define STD_NOHEADS "GET http://test:21/halo/find/me/here HTTP/1.1\r\n\r\nmuch body very http\r\n\r\n"
#define STD_QUERY "GET http://test:21/halo/find/me/here?amd=123&def=566 HTTP/1.1\r\nkey1: value1\r\nkey2: value2\r\n\r\nmuch body very http\r\n\r\n"

class ClientException : std::exception
{
	public:
		ClientException( std::initializer_list<const char*> prompts) noexcept;
		virtual const char* what() const noexcept override {return (this->_msg.c_str());}
		virtual ~ClientException( void ) noexcept {}
	
	private:
		std::string _msg;
};

class Client
{
	public:
		Client( void ) noexcept;
		~Client( void ) noexcept;

		void		connectTo( const char*, const char* );
		void		sendRequest( const char* ) const ;
		std::string	getAddress( const struct sockaddr_storage*) const noexcept ;

	private:
		int 						_sockfd = -1;
		// const char					*_connectTo, *_port;
		struct sockaddr_storage 	_serverAddr;
		struct addrinfo 			_filter;

		Client( Client const& ) noexcept;
		Client&	operator=( Client const& ) noexcept;
};