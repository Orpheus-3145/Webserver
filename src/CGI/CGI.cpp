#include "CGI.hpp"

#include <string>
#include <iostream>
#include <sstream>
#include <string.h>

CGI::CGI(const HTTPrequest &req)
	: _req(req),
	  _CGIEnvArr(this->_createCgiEnv(req)),
	  _CgiEnvCStyle(this->_createCgiEnvCStyle())
{
	pipe(_uploadPipe);
	pipe(_responsePipe);
}

CGI::~CGI() {
	delete[] this->_CgiEnvCStyle;
}

std::array<std::string, CGI_ENV_SIZE> CGI::_createCgiEnv(const HTTPrequest &req)
{
	std::string CGIfileName = _req.getRealPath().filename();
	std::array<std::string, CGI_ENV_SIZE> CGIEnv {
		"AUTH_TYPE=",
		"CONTENT_LENGTH=" + std::to_string(this->_req.getContentLength()),
		"CONTENT_TYPE=multipart/form-data; boundary=" + this->_req.getContentTypeBoundary(),
		"GATEWAY_INTERFACE=CGI/1.1", // fixed
		"PATH_INFO=",
		"PATH_TRANSLATED=",
		"QUERY_STRING=" + req.getQueryRaw(),
		"REMOTE_ADDR=" + req.getHost(),
		"REMOTE_HOST=",
		"REMOTE_IDENT=",
		"REMOTE_USER=",
		"REQUEST_METHOD=" + req.getMethod(),
		"SCRIPT_NAME=" + req.getRealPath().filename().string(), // script name, e.g. myScript.cgi
		"SCRIPT_FILENAME=" + req.getRealPath().string(),
		"SERVER_NAME=" + req.getServName(),
		"SERVER_PORT=" + req.getPort(),
		"SERVER_PROTOCOL=HTTP/1.1", // fixed
		"SERVER_SOFTWARE=WebServServer/1.0", // fixed
		"HTTP_COOKIE=" + req.getCookie(),
	};
	return CGIEnv;
}

char **CGI::_createCgiEnvCStyle(void)
{
	int i = 0;
	char **CgiEnv = new char*[CGI_ENV_SIZE + 1];
	while (i < CGI_ENV_SIZE)
	{
		CgiEnv[i] = (char *)this->_CGIEnvArr[i].c_str();
		i++;
	}
	CgiEnv[i] = NULL;
	return CgiEnv;
}

void CGI::run()
{
	this->_pid = fork();
	if (this->_pid == 0) {
		std::string CGIfilePath = _req.getRealPath().string();
		std::string CGIfileName = _req.getRealPath().filename().string(); // fully stripped, only used for execve
		close(this->_responsePipe[0]);
		dup2(this->_responsePipe[1], STDOUT_FILENO); // write to pipe
		close(this->_uploadPipe[1]);
		dup2(this->_uploadPipe[0], STDIN_FILENO); // read from pipe
		char *argv[2] = {(char*)CGIfileName.c_str(), NULL};
		int res = execve(CGIfilePath.c_str(), argv, this->_CgiEnvCStyle);
		if (res != 0)
		{
			close(this->_responsePipe[1]);
			std::cerr << "Error in running CGI script!" << std::endl;
			std::cerr << "path: " << CGIfilePath.c_str() << std::endl;
			perror("");
			exit(EXIT_FAILURE);
		}
	}
	else {
		close(this->_responsePipe[1]); // close write end of cgi response pipe
	}
}

bool	CGI::waitCGIproc() const 
{
	int cgiExitCode = -1;
	int waitStatus = waitpid(this->_pid, &cgiExitCode, WNOHANG);
	if (waitStatus == -1)
		throw(CGIexception({"error while waiting CGI process, pid", std::to_string(this->_pid)}, 500));
	else if (waitStatus == 0)		// if it's 0 the child is not done yet
		return (false);
	else
	{
		if (cgiExitCode != EXIT_SUCCESS)
			throw(CGIexception({"error while running CGI"}, 500));
		return (true);
	}
}

int CGI::getRequestSocket() const {
	return this->_req.getSocket();
}

const std::array<int, 2> CGI::getUploadPipe() const {
	return std::array<int, 2> {this->_uploadPipe[0], this->_uploadPipe[1]};
}

const std::array<int, 2> CGI::getResponsePipe() const {
	return std::array<int, 2> {this->_responsePipe[0], this->_responsePipe[1]};
}

std::string CGI::getResponse() const {
	return this->_response;
}

void CGI::appendResponse(std::string additionalResponsePart) {
	this->_response = this->_response + additionalResponsePart;
}
