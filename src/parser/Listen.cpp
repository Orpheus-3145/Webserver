#include "Listen.hpp"
#include <iostream>

Listen::Listen(const Listen& copy) :
	i_port(copy.i_port),
	i_ip(copy.i_ip),
	s_ip(copy.s_ip),
	s_port(copy.s_port),
	def(copy.def) {}

Listen&	Listen::operator=(const Listen& assign)
{
	if (this != &assign)
	{
		i_ip = assign.i_ip;
		i_port = assign.i_port;
		s_ip = assign.s_ip;
		s_port = assign.s_port;
		def = assign.def;
	}
	return (*this);
}

Listen::Listen(void)
{
	i_ip = {0, 0, 0, 0};
	i_port = std::stoi(DEF_PORT);
	s_ip = "0.0.0.0";
	s_port = DEF_PORT;
	def = false;
}

Listen::~Listen(void)
{
	i_ip.clear();
}

void	Listen::_fillFull(strings_t& block)
{
	if (!std::isdigit(block.front().front()))
		throw ParserException({"first element is not a digit in listen '" + block.front() + "'"});
	uint8_t counter = 0;
	uint16_t	tmp = 0;
	int i = 0;
	while (block.front()[i] != '\0' && std::isdigit(block.front()[i]))
	{
		while (std::isdigit(block.front()[i]))
		{
			tmp = tmp * 10 + block.front()[i] - '0';
			if (tmp > 255)
				throw ParserException({"IP range is too high on '" + block.front() + "'"});
			i++;
		}
		i_ip[counter] = tmp;
		tmp = 0;
		if (block.front()[i] == '.')
		{
			i++;
			counter++;
		}
		if (counter > 3)
			throw ParserException({"'listen' has more '.' than expected on '" + block.front() + "'"});
	}
	if (counter != 3)
		throw ParserException({"'listen' has less '.' than expected on '" + block.front() + "'"});
	if (block.front()[i++] != ':')
		throw ParserException({"unexpected character on: '" + block.front() + "'"});
	if (!std::isdigit(block.front()[i]))
		throw ParserException({"unexpected or missing character on: '" + block.front() + "'"});
	uint32_t port = 0;
	while (std::isdigit(block.front()[i]))
	{
		port = port * 10 + block.front()[i] - '0';
		if (port > 65535)
			throw ParserException({"port is too big on '" + block.front() + "'"});
		i++;
	}
	if (block.front()[i] != '\0')
		throw ParserException({"unexpected character on '" + block.front() + "'"});
	i_port = port;
	s_port = std::to_string(port);
	block.erase(block.begin());
	s_ip = std::to_string(i_ip[0]) + "." + std::to_string(i_ip[1]) + "." + std::to_string(i_ip[2]) + "." + std::to_string(i_ip[3]);
}

void	Listen::_fillIp(strings_t& block)
{
	if (!std::isdigit(block.front().front()))
		throw ParserException({"first element is not a digit in listen '" + block.front() + "'"});
	uint8_t counter = 0;
	uint16_t	tmp = 0;
	int i = 0;
	while (block.front()[i] != '\0' && std::isdigit(block.front()[i]))
	{
		while (std::isdigit(block.front()[i]))
		{
			tmp = tmp * 10 + block.front()[i] - '0';
			if (tmp > 255)
				throw ParserException({"IP range is too high on '" + block.front() + "'"});
			i++;
		}
		i_ip[counter] = tmp;
		tmp = 0;
		if (block.front()[i] == '.')
		{
			i++;
			counter++;
		}
		if (counter > 3)
			throw ParserException({"'listen' has more '.' than expected on '" + block.front() + "'"});
	}
	if (block.front()[i] != '\0')
		throw ParserException({"unexpected character on: '" + block.front() + "'"});
	if (counter != 3)
		throw ParserException({"'listen' has less '.' than expected on '" + block.front() + "'"});
	block.erase(block.begin());
	s_ip = std::to_string(i_ip[0]) + "." + std::to_string(i_ip[1]) + "." + std::to_string(i_ip[2]) + "." + std::to_string(i_ip[3]);
}

void	Listen::_fillPort(strings_t& block)
{
	if (block.front().find_first_of(":") != block.front().find_last_of(":"))
		throw ParserException({"expected 'port' type ':80' or '8080' on '" + block.front() + "'"});
	if (block.front().front() == ':')
		block.front().erase(block.front().begin());
	
	uint32_t port = 0;
	int i = 0;
	while (std::isdigit(block.front()[i]))
	{
		port = port * 10 + block.front()[i] - '0';
		if (port > 65535)
			throw ParserException({"port is too big on '" + block.front() + "'"});
		i++;
	}
	if (block.front()[i] != '\0')
		throw ParserException({"unexpected character on '" + block.front() + "'"});
	i_port = port;
	s_port = std::to_string(port);
	block.erase(block.begin());
}

void	Listen::fillValues(strings_t& block)
{
	if (block.front().find_first_not_of(":.0123456789") != std::string::npos)
		throw ParserException({"unexpected character in listen '" + block.front() + "'"});
	if (!std::isdigit(block.front()[0]) && block.front()[0] != '*')
		throw ParserException({"after 'listen' a digit expected. Fault on '" + block.front() + "'"});
	bool hasDot = false;
	bool hasColumn = false;
	if (block.front().find('.') != std::string::npos)
		hasDot = true;
	if (block.front().find(':') != std::string::npos)
		hasColumn = true;
	if (hasDot && hasColumn)
		_fillFull(block);
	else if (hasDot)
		_fillIp(block);
	else
		_fillPort(block);
	if (block.front() == "default_server")
		this->def = true;
}

const std::string&	Listen::getIpString(void) const
{
	return (s_ip);
}

const std::vector<uint8_t>&	Listen::getIpInt(void) const
{
	return (i_ip);
}

const std::string&	Listen::getPortString(void) const
{
	return (s_port);
}

const uint16_t&	Listen::getPortInt(void) const
{
	return (i_port);
}

const bool&	Listen::getDef(void) const
{
	return (def);
}

void	Listen::setDef(bool	status)
{
	def = status;
}

bool Listen::operator==(const Listen& other) const
{
	return ((this->i_ip == other.getIpInt()) && (this->i_port == other.getPortInt()));
}

bool Listen::operator!=(const Listen& other) const
{
	return(!(*this == other));
}
