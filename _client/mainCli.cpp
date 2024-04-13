/* ************************************************************************** */
/*                                                                            */
/*                                                        ::::::::            */
/*   mainCli.cpp                                        :+:    :+:            */
/*                                                     +:+                    */
/*   By: fra <fra@student.codam.nl>                   +#+                     */
/*                                                   +#+                      */
/*   Created: 2023/11/26 20:50:27 by fra           #+#    #+#                 */
/*   Updated: 2024/03/26 00:47:49 by fra           ########   odam.nl         */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include <ctime>
#include <iomanip>

void runClient( const char *host, const char *port )
{
	try
	{
		std::cout << "running client on process: " << getpid() << "\n" << STD_POST_REQUEST << '\n'; 
		Client webClient;
		webClient.connectTo(host, port);
		webClient.sendRequest(STD_POST_REQUEST);
	}
	catch(ClientException const& e)
	{
		std::cout << e.what() << "\n";
	}
}

int main( int argc, char** argv)
{
	if (argc != 3)
	{
		std::cout << "wrong parameters: host and port needed\n";
		return(EXIT_FAILURE);
	}
	std::time_t rawtime;
    std::tm* timeinfo;
    char buffer[80];

    std::time(&rawtime);
    timeinfo = std::gmtime(&rawtime);
    std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
	runClient(argv[1], argv[2]);
	return(EXIT_SUCCESS);
}