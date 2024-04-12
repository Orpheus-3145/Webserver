/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   Client.cpp                                         :+:    :+:            */
/*                                                     +:+                    */
/*   By: fra <fra@student.codam.nl>                   +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/11/26 20:22:27 by fra           #+#    #+#                 */
/*   Updated: 2024/01/16 12:44:18 by faru          ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

ClientException::ClientException( std::initializer_list<const char*> prompts) noexcept 
	: std::exception()
{
	this->_msg = "WebClient exception: ";
	for (const char *prompt : prompts)
		this->_msg += std::string(prompt) + " ";
}

Client::Client( void ) noexcept
{
	memset(&this->_filter, 0, sizeof(this->_filter));
	this->_filter.ai_family = AF_UNSPEC;
	this->_filter.ai_protocol = IPPROTO_TCP;
	this->_filter.ai_socktype = SOCK_STREAM;
}

Client::~Client( void ) noexcept
{
	if (this->_sockfd != -1)
	{
		shutdown(this->_sockfd, SHUT_RDWR);
		close(this->_sockfd);
	}
}

void	Client::connectTo( const char* host, const char* port )
{
	struct addrinfo *list, *tmp;

	if (getaddrinfo(host, port, &this->_filter, &list) != 0)
		throw(ClientException({"error: failed to find ", host, ":", port}));
	for (tmp=list; tmp!=nullptr; tmp=tmp->ai_next)
	{
		this->_sockfd = socket(tmp->ai_family, tmp->ai_socktype, tmp->ai_protocol);
		if (this->_sockfd == -1)
			continue;
		if (connect(this->_sockfd, tmp->ai_addr, tmp->ai_addrlen) == 0)
			break;
		shutdown(this->_sockfd, SHUT_RDWR);
		close(this->_sockfd);
	}
	if (tmp == nullptr)
	{
		freeaddrinfo(list);
		throw(ClientException({"no available IP host found for port", port}));
	}
	memmove(&this->_serverAddr, tmp->ai_addr, std::min(sizeof(struct sockaddr), sizeof(struct sockaddr_storage)));
	std::cout << "connected to: "<< this->getAddress(&this->_serverAddr) << "\n";
	freeaddrinfo(list);
}

void	Client::sendRequest( const char *request ) const
{
	ssize_t	sendBytes, recvBytes;
	char 	buf[1024];
	size_t	sizeBuf=1024;

	sendBytes = send(this->_sockfd, request, strlen(request), 0);
	if (sendBytes < (ssize_t) strlen(request))
		throw(ClientException({"error: failed to send message to", this->getAddress(&this->_serverAddr).c_str()}));
	recvBytes = recv(this->_sockfd, buf, sizeBuf, 0);
	if (recvBytes < 0)
		throw(ClientException({"error: failed to read message from", this->getAddress(&this->_serverAddr).c_str()}));
	shutdown(this->_sockfd, SHUT_RDWR);
	close(this->_sockfd);
}

std::string	Client::getAddress( const struct sockaddr_storage *addr ) const noexcept
{
	std::string ipAddress;
	if (addr->ss_family == AF_INET)
	{
		char ipv4[INET_ADDRSTRLEN];
		struct sockaddr_in *addr_v4 = (struct sockaddr_in*) addr;
		inet_ntop(addr_v4->sin_family, &(addr_v4->sin_addr), ipv4, sizeof(ipv4));
		ipAddress = std::string(ipv4) + std::string(":") + std::to_string(ntohs(addr_v4->sin_port));
	}
	else if (addr->ss_family == AF_INET6)
	{
		char ipv6[INET6_ADDRSTRLEN];
		struct sockaddr_in6 *addr_v6 = (struct sockaddr_in6*) addr;
		inet_ntop(addr_v6->sin6_family, &(addr_v6->sin6_addr), ipv6, sizeof(ipv6));
		ipAddress = std::string(ipv6) + std::string(":") + std::to_string(ntohs(addr_v6->sin6_port));
	}
	return (ipAddress);
}

Client::Client( Client const& other ) noexcept
{
	(void) other;
}

Client&	Client::operator=( Client const& other ) noexcept
{
	(void) other;
	return (*this);
}