#include "Tokenizer.hpp"
#include "WebServer.hpp"

std::vector<Config>	parseServers(std::string const& fileName)
{
	Tokenizer *config;
	config = new Tokenizer();
	std::vector<Config> servers;
	try {
		config->fillConfig(fileName);
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
		delete config;
		return (servers);
	}
	std::vector<std::vector<std::string>> separated = config->divideContent();
	delete config;
	for (size_t i = 0; i < separated.size(); i++)
	{
		Config tmp;
		try {
			tmp.parseBlock(separated[i]);
			servers.push_back(tmp);
		}
		catch(const std::exception& e) {
			std::cerr << "Failure on Server index " C_RED << i << C_RESET "\n";
			std::cerr << C_RED << e.what() << C_RESET "\n";
			std::cerr << C_YELLOW "Continuing with parsing other servers...\n" C_RESET;
		}
	}
	return (servers);
}

int main(int ac, char **av)
{
	std::vector<Config> servers;
	if (ac > 2)
	{
		std::cerr << C_RED "Wrong amount of arguments - valid usage: ./" << av[0] << " [config_file_path]\n";
		return (EXIT_FAILURE);
	}
	else if (ac == 2)	// custom configuration
	{
		std::cout << "Using config: " << C_GREEN << av[1] << C_RESET << "\n";
		servers = parseServers(av[1]);
	}
	else				// default configuration
	{
		std::cout << "No argument provided, using default config: " C_GREEN << DEF_CONF_PATH << C_RESET << "\n";
		servers = parseServers(DEF_CONF_PATH);
	}
	try
	{
		WebServer	webserv(servers);
		webserv.run();
	}
	catch(const WebservException& e) {
		std::cerr << e.what() << '\n';
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
