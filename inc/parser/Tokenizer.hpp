#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <exception>
#include <vector>
#include <stack>

#include "Config.hpp"

typedef std::vector<std::string> strings_t;

class Tokenizer
{
	public:
		Tokenizer(void);
		virtual	~Tokenizer();
		Tokenizer(const Tokenizer& copy);
		Tokenizer&	operator=(const Tokenizer& assign);

		void					fillConfig(const std::string& file);
		bool					clearEmpty(void);
		strings_t				getFileContent(void);
		std::vector<strings_t>	divideContent(void);

	private:
		size_t	_doComment(size_t &i);
		size_t	_doSpace(size_t& i);
		void	_doQuote(size_t& i, size_t& j);
		void	_doToken(size_t& i, size_t& j);
		void	_doExceptions(size_t& i);
		void	_doClean(void);
		void	_readFile(const std::string& file_path);
		void	_tokenizeFile(void);
		void	_checkBrackets(void);
		void	_printContent(void);

		strings_t	file_content;
		std::string raw_input;
};
