#pragma once
#define CGI_ENV_SIZE 19
#define CGI_READ_BUFFER_SIZE 10000

#include <array>
#include <string>
#include <unistd.h>  // pipe(), fork()
#include <sys/wait.h>  // waitpid()

#include "HTTPrequest.hpp"

class  CGI {
public:
	CGI(const HTTPrequest &req);
	~CGI();

	void						run();
	bool 						waitCGIproc() const;
	const std::array<int, 2> 	getUploadPipe() const;
	const std::array<int, 2> 	getResponsePipe() const;
	int 						getRequestSocket() const;
	std::string 				getResponse() const;
	void 						appendResponse(std::string additionalResponsePart);

private:
	const HTTPrequest                       &_req;
	std::array<std::string, CGI_ENV_SIZE>   _CGIEnvArr;
	char *const                             *_CgiEnvCStyle;
	int                                     _uploadPipe[2];
	int                                     _responsePipe[2];
	std::string                             _response;
	pid_t                                   _pid;

	std::array<std::string, CGI_ENV_SIZE> _createCgiEnv(const HTTPrequest &req);
	char **_createCgiEnvCStyle();
};
