#pragma once
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

#include "colors.hpp"
#include "Parameters.hpp"
#include "Location.hpp"
#include "Listen.hpp"
#include "Exceptions.hpp"

#define DEF_CONF_PATH std::string("default/defaultConfig.conf")

typedef std::vector<std::string> strings_t;

class Config
{
	public:
		// Form
		Config(void) {};
		Config(const Config& copy);
		Config&	operator=(const Config& assign);
		virtual ~Config(void);

		void							parseBlock(strings_t& block);
		const std::vector<Listen>& 		getListens(void) const;
		std::vector<Listen>& 			getListensNonConst(void);
		const strings_t&	getNames(void) const;
		const std::string&				getPrimaryName(void) const;
		const Parameters&				getParams(void) const;
		const std::vector<Location>&	getLocations(void) const;

	private:
		std::vector<Listen> 		listens; // Listens
		strings_t	names; // is the given "server_name".
		Parameters					params; // Default parameters for whole server block
		std::vector<Location>		locations; // declared Locations

		void	_parseListen(strings_t& block);
		void	_parseServerName(strings_t& block);
		void	_parseLocation(strings_t& block);
		void	_fillServer(strings_t& block);
};
