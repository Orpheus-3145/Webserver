#pragma once
#include <vector>
#include <string>
#include <unordered_set>
#include <map>
#include <climits>
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <bitset>
#include <filesystem>

#include "Exceptions.hpp"
#include "HTTPstruct.hpp"

#define METHOD_AMOUNT 3u // amount of methodes used in our program
#define DEF_SIZE 10
#define DEF_ROOT path_t("/var/www")
#define MAX_SIZE 20
#define DEF_CGI_ALLOWED false
#define DEF_CGI_EXTENTION ".cgi"
#define DEF_SIZE_VALUE 'B'

typedef	std::filesystem::path	path_t;
typedef std::map<size_t, path_t> path_t_map;
typedef std::vector<std::string> strings_t;

class Parameters
{
	public:
		Parameters(void);
		virtual ~Parameters(void);
		Parameters(const Parameters& copy);
		Parameters&	operator=(const Parameters& assign);

		void	fill(strings_t& block);
		void	setRoot(path_t val);
		void	setSize(uintmax_t val, char *c);
		void	setAutoindex(bool status);

		void								inherit(Parameters const&);
		const std::pair<size_t, path_t>& 	getReturns(void) const;
		const std::vector<path_t>&	 		getIndex(void) const;
		std::uintmax_t						getMaxSize(void) const;
		const path_t_map& 					getErrorPages(void) const;
		const bool& 						getAutoindex(void) const;
		const path_t& 						getRoot(void) const;
		const std::bitset<METHOD_AMOUNT>&	getAllowedMethods(void) const;
		const std::string& 					getCgiExtension(void) const;
		const bool& 						getCgiAllowed(void) const;

	private:
		std::uintmax_t				max_size;	// Will be overwriten by last found
		bool						autoindex;	// FALSE in default, will be overwriten.
		std::vector<path_t>			index;	// Will be searched in given order
		path_t						root;		// Last found will be used.
		path_t_map					error_pages;	// Same status codes will be overwriten
		std::pair<size_t, path_t>	returns;	// Overwritten by the last
		std::bitset<METHOD_AMOUNT>	allowedMethods;	// Allowed methods
		std::string					cgi_extension;	// extention .py .sh
		bool						cgi_allowed;	// Check for permissions

		void	_parseRoot(strings_t& block);
		void	_parseBodySize(strings_t& block);
		void	_parseAutoindex(strings_t& block);
		void	_parseIndex(strings_t& block);
		void	_parseErrorPage(strings_t& block);
		void	_parseReturn(strings_t& block);
		void	_parseAllowMethod(strings_t& block);
		void	_parseDenyMethod(strings_t& block);
		void	_parseCgiExtension(strings_t& block);
		void	_parseCgiAllowed(strings_t& block);
};
