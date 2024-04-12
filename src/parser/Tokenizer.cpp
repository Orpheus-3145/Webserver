#include "Tokenizer.hpp"
#include "Config.hpp"

Tokenizer::Tokenizer(void) {}

Tokenizer::~Tokenizer(void) {}

void	Tokenizer::fillConfig(const std::string& file)
{
	_readFile(file);
	_tokenizeFile();
	if (clearEmpty())
		throw ParserException({"config file is empty"});
	_checkBrackets();
}

std::vector<strings_t>	Tokenizer::divideContent(void)
{
	std::vector<strings_t>	ret;
	strings_t tmp;
	int	bracks = 0;
	for (strings_t::iterator it = file_content.begin(); it != file_content.end(); it++)
	{
		tmp.push_back(*it);
		if (*it == "{")
			bracks++;
		else if (*it == "}")
		{
			bracks--;
			if (bracks == 0)
			{
				ret.push_back(std::move(tmp));
				tmp.clear();
			}
		}
	}
	return (ret);
}

Tokenizer& Tokenizer::operator=(const Tokenizer& assign)
{
	raw_input = assign.raw_input;
	file_content.clear();
	file_content = assign.file_content;
	return (*this);
}

Tokenizer::Tokenizer(const Tokenizer& copy) :
	file_content(copy.file_content),
	raw_input(copy.raw_input)
{

}

strings_t	Tokenizer::getFileContent(void)
{
	return (file_content);
}

void	Tokenizer::_readFile(const std::string& file_path)
{
	std::ifstream inputFile(file_path);
	if (!inputFile.is_open())
		throw ParserException({"error opening file:", file_path});

	std::ostringstream	fileContentStream;
	fileContentStream << inputFile.rdbuf();
	if (inputFile.bad())
		throw ParserException({"error reading file:", file_path});

	raw_input = fileContentStream.str();
	inputFile.close();
	if (raw_input.empty())
		throw ParserException({"empty file", file_path,});
}

size_t	Tokenizer::_doComment(size_t &i)
{
	if (raw_input[i] != '#')
		return (i);
	while (raw_input[i] != '\n' && i < raw_input.size())
		i++;
	return (i);
}

void	Tokenizer::_doQuote(size_t& i, size_t& j)
{
	if (raw_input[i] != '\'' && raw_input[i] != '"')
		return ;
	char	type = raw_input[i];
	j = i + 1;
	while (raw_input[j] && raw_input[j] != type)
		j++;
	if (raw_input[j] == type)
	{
		j++;
		file_content.push_back(raw_input.substr(i, j - i));
	}
	else
		throw ParserException({"non-matching quote"});
	i = j;
}

size_t	Tokenizer::_doSpace(size_t& i)
{
	if (!std::isspace(raw_input[i]))
		return (i);
	while (i < raw_input.size() && std::isspace(raw_input[i]))
		i++;
	if (file_content.size() != 0)
		file_content.push_back(" ");
	return (i);
}

void	Tokenizer::_doExceptions(size_t& i)
{
	if ((raw_input[i] >= 1 && 8 <= raw_input[i]) || (raw_input[i] >= 14 && raw_input[i] <= 31))
		throw ParserException({"invalid character in file"});
}

void	Tokenizer::_doToken(size_t& i, size_t& j)
{
	if (j >= raw_input.size()
		|| std::isspace(raw_input[j])
		|| raw_input[j] == '"'
		|| raw_input[j] == '\''
		|| raw_input[j] == '#')
		return ;
	while (j < raw_input.size()
		&& !std::isspace(raw_input[j])
		&& raw_input[j] != '"'
		&& raw_input[j] != '\''
		&& raw_input[j] != '#'
		&& raw_input[j] != ';')
			j++;
	if (j - 1 >= i)
		file_content.emplace_back(raw_input.cbegin() + i, raw_input.cbegin() + j);
	if (raw_input[j] == ';')
	{
		j++;
		file_content.emplace_back(";");
	}
	i = j;
}

void	Tokenizer::_doClean()
{
	for (strings_t::iterator it = file_content.begin(); it != file_content.end();)
	{
		if (*it == " ")
			it = file_content.erase(it);
		else
			it++;
	}
	size_t pos = 0;
	while (pos < file_content.size())
	{
		if (file_content[pos].front() == '\'' || file_content[pos].front() == '\"')
			file_content[pos].erase(file_content[pos].begin());
		if (!file_content[pos].empty()
			&& (file_content[pos].back() == '\'' || file_content[pos].back() == '\"'))
			file_content[pos].pop_back();
		if (file_content[pos].empty())
			file_content.erase(file_content.begin() + pos);
		else
			pos++;
	}
}

void	Tokenizer::_tokenizeFile(void)
{
	size_t	i = 0, j = 0;
	while (i < raw_input.size())
	{
		j = _doSpace(i);
		j = _doComment(i);
		j = _doSpace(i);
		_doQuote(i, j);
		j = _doSpace(i);
		_doToken(i, j);
	}
	if (file_content.size() == 0)
		throw ParserException({"no valuable input found in given config file"});
	_doClean();
}

void	Tokenizer::_checkBrackets(void)
{
	int bracks = 0;
	strings_t::iterator it;
	for (it = file_content.begin(); it != file_content.end(); ++it)
	{
		if (*it == "{")
			bracks++;
		else if (*it == "}")
		{
			if (--bracks < 0)
				throw ParserException({"mismatched brackets"});
		}
	}
	if (bracks)
		throw ParserException({"missing brackets"});
}

void	Tokenizer::_printContent(void)
{
	size_t i = 0;
	while (i < file_content.size())
	{
		std::cout << file_content[i];
		i++;
	}
	std::cout << std::endl;
}

bool	Tokenizer::clearEmpty(void)
{
	for (strings_t::iterator it = file_content.begin(); it != file_content.end();)
	{
		if (*it == " ")
			it = file_content.erase(it);
		else
			++it;
	}
	return file_content.empty();
}