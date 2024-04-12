#include "Config.hpp"

Config&	Config::operator=(const Config& assign)
{
	if (this != &assign)
	{
		listens.clear();
		names.clear();
		locations.clear();
		listens = assign.listens;
		names = assign.names;
		locations = assign.locations;
		params = assign.params;
	}
	return (*this);
}

Config::~Config(void)
{
	listens.clear();
	names.clear();
	names.clear();
	locations.clear();
}

Config::Config(const Config& copy) :
	listens(copy.listens),
	names(copy.names),
	params(copy.params),
	locations(copy.locations)
{

}

void	Config::_parseListen(strings_t& block)
{
	block.erase(block.begin());
	if (block.front() == ";")
		throw ParserException({"Can't use ';' after keyword 'listen'"});
	if (block.front() == "default_server")
		throw ParserException({"Before 'default_server' an ip/port expected"});
	Listen tmp;
	tmp.fillValues(block);
	if (block.front() == "default_server")
	{
		tmp.setDef(true);
		block.erase(block.begin());
	}
	listens.push_back(tmp);
	if (block.front() != ";")
		throw ParserException({"Missing semicolumn on Listen, before: '" + block.front() + "'"});
	block.erase(block.begin());
}

void	Config::_parseServerName(strings_t& block)
{
	block.erase(block.begin());
	for (strings_t::iterator it = block.begin(); it != block.end();)
	{
		if (*it == ";")
		{
			block.erase(block.begin());
			break ;
		}
		if (block.front().find_first_not_of("abcdefghijklmnoprstuvyzwxqABCDEFGHIJKLMNOPRSTUVYZWXQ0123456789-.") != std::string::npos)
			throw ParserException({"Only 'alpha' 'digit' '-' and '.' characters are accepted in 'server_name'"});
		names.push_back(block.front());
		block.erase(block.begin());
	}
}

void	Config::_parseLocation(strings_t& block)
{
	Location	local(block, params, "/");
	locations.push_back(local);
}

void	Config::_fillServer(strings_t& block)
{
	std::vector<strings_t> locationHolder;
	strings_t::iterator index;
	uint64_t size = 0;
	for (strings_t::iterator it = block.begin(); it != block.end();)
	{
		if (*it == "listen")
			_parseListen(block);
		else if (*it == "server_name")
			_parseServerName(block);
		else if (*it == "location")
		{
			index = it;
			while (index != block.end() && *index != "{")
				index++;
			if (index == block.end())
				throw ParserException({"Error on location parsing"});
			index++;
			size++;
			while (size && index != block.end())
			{
				if (*index == "{")
					size++;
				else if (*index == "}")
					size--;
				index++;
			}
			if (size)
				throw ParserException({"Error on location parsing with brackets"});
			strings_t subVector(it, index);
			block.erase(it, index);
			locationHolder.push_back(subVector);
		}
		else
			params.fill(block);
	}
	for (std::vector<strings_t>::iterator it = locationHolder.begin(); it != locationHolder.end(); it++)
		_parseLocation(*it);
}

void	Config::parseBlock(strings_t& block)
{
	if (block.front() != "server")
		throw ParserException({"first arg is not 'server'"});
    block.erase(block.begin());
	if (block.front() != "{")
		throw ParserException({"after a 'server' directive a '{' is expected"});
    block.erase(block.begin());
	if (block[block.size() - 1] != "}")
		throw ParserException({"last element is not a '}"});
	block.pop_back();
	_fillServer(block);
	if (names.empty())
		names.push_back(LOCALHOST);
}

const std::vector<Listen>& Config::getListens(void) const
{
	return (listens);
}

std::vector<Listen>& Config::getListensNonConst(void)
{
	return (listens);
}

const strings_t& Config::getNames(void) const
{
	return (names);
}

const std::string&		Config::getPrimaryName(void) const
{
	return (names[0]);
}

const Parameters&	Config::getParams(void) const
{
	return (params);
}

const std::vector<Location>&	Config::getLocations() const
{
	return (locations);
}
