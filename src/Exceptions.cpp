#include "Exceptions.hpp"

WebservException::WebservException( std::initializer_list<std::string> const& args, std::string const& prompt ) noexcept
	: std::exception() , _prompt(prompt)
{
	for (std::string const& arg : args)
		this->_info += arg + " ";
	this->_fullInfo = std::string(this->_prompt) + " - " + this->_info;
}

HTTPexception::HTTPexception( std::initializer_list<std::string> const& args, int status, std::string const& prompt ) noexcept
	: WebservException(args, prompt) , _status(status)
{
	this->_fullInfo += "- status: " + std::to_string(this->_status);
}

HTTPexception::HTTPexception( HTTPexception const& obj, std::string const& prompt) noexcept
	: WebservException()
{
	if (this != &obj)
	{
		this->_prompt = prompt;
		this->_info = obj.getInfo();
		this->_status = obj.getStatus();
		this->_fullInfo = std::string(this->_prompt) + " - " + this->_info + "- status: " + std::to_string(this->_status);
	}
}
