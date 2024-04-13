#include "RequestValidate.hpp"

// ╔════════════════════════════════╗
// ║		CONSTRUCTION PART		║
// ╚════════════════════════════════╝
RequestValidate::RequestValidate(t_serv_list const& servers) : _servers(servers)
{
	_setDefaultServ();	
	this->_handlerServer = this->_defaultServer;
	_resetValues();
}

// ╔════════════════════════════════╗
// ║			GETTER PART			║
// ╚════════════════════════════════╝
path_t const&	RequestValidate::getRealPath( void ) const
{
	return (_realPath);
}

path_t const&	RequestValidate::getRedirectRealPath( void ) const
{
	return (_redirectRealPath);
}

std::string const&	RequestValidate::getServName( void ) const
{
	return(this->_handlerServer->getPrimaryName());
}

int	RequestValidate::getStatusCode( void ) const
{
	return (_statusCode);
}

std::uintmax_t	RequestValidate::getMaxBodySize( void ) const
{
	return (_validParams->getMaxSize());
}

bool	RequestValidate::isAutoIndex( void ) const
{
	return (_autoIndex);
}

bool	RequestValidate::isFile( void ) const
{
	return (_realPath.has_filename());
}

bool	RequestValidate::isCGI( void ) const
{
	return (_isCGI);
}

bool	RequestValidate::isRedirection( void ) const
{
	return (_isRedirection);
}

bool	RequestValidate::solvePathFailed( void ) const
{
	return (this->_statusCode >= 400);
}

path_t	const& RequestValidate::getRoot( void ) const
{
	return (this->_validParams->getRoot());
}

// ╔════════════════════════════════╗
// ║			SETTER PART			║
// ╚════════════════════════════════╝
void	RequestValidate::_resetValues( void )
{
	this->_validLocation = nullptr;
	this->_validParams = &this->_handlerServer->getParams();

	this->_autoIndex = false;
	this->_isCGI = false;
	this->_isRedirection = false;
	this->_realPath = "/IAMEMPTY";
	this->_requestMethod = HTTP_GET;
	this->_statusCode = 200;
	this->_realPath.clear();
	this->_redirectRealPath.clear();
}

void	RequestValidate::_setMethod( HTTPmethod method )
{
	this->_requestMethod = method;
}

void	RequestValidate::_setConfig( std::string const& hostName )
{
	std::string tmpHostName = hostName;

	std::transform(tmpHostName.begin(), tmpHostName.end(), tmpHostName.begin(), ::tolower);
	for (auto& server : this->_servers)
	{
		for (std::string servName : server.getNames())
		{
			std::transform(servName.begin(), servName.end(), servName.begin(), ::tolower);
			if ((servName == tmpHostName))
			{
				this->_handlerServer = &server;
				this->_validParams = &(this->_handlerServer->getParams());
				return ;
			}
		}
	}
}

void	RequestValidate::_setDefaultServ ( void ) noexcept
{
	this->_defaultServer = nullptr;
	for (auto& server : this->_servers)
	{
		for (auto const& address : server.getListens())
		{
			if (address.getDef() == true)
			{
				this->_defaultServer = &server;
				return ;
			}
		}
	}
	this->_defaultServer = &this->_servers.front();
}

void	RequestValidate::_setPath( path_t const& newPath )
{
	this->_requestPath = std::filesystem::weakly_canonical(newPath);
}

bool	RequestValidate::_hasValidIndex( void ) const
{
	return (_validParams->getIndex().empty() == false);
}

void	RequestValidate::_setStatusCode(const size_t& code)
{
	_statusCode = code;
}

// ╔════════════════════════════════╗
// ║			SOLVING PART		║
// ╚════════════════════════════════╝
// ╭───────────────────────────╮
// │ RECURSIVE LOCATION SEARCH │
// ╰───────────────────────────╯
Location const*	RequestValidate::_diveLocation(Location const& cur, strings_t::iterator itDirectory, strings_t& folders)
{
	strings_t curURL;
	strings_t::iterator itFolders;
	Location const*	valid;

	_separateFolders(std::filesystem::weakly_canonical(cur.getURL()).string(), curURL);
	itFolders = curURL.begin();
	while (itFolders != curURL.end() && itDirectory != folders.end())
	{
		if (*itFolders != *itDirectory)
			break ;
		itFolders++;
		itDirectory++;
	}
	if (itFolders == curURL.end() && itDirectory == folders.end())
		return (&cur);
	else if (itFolders == curURL.end())
	{
		for (auto& nest : cur.getNested())
		{
			valid = _diveLocation(nest, itDirectory, folders);
			if (valid)
				return (valid);
		}
	}
	return (NULL);
}

void	RequestValidate::_initValidLocation(void)
{
	Location const*						valid;
	strings_t			folders;
	strings_t::iterator	it;

	_separateFolders(targetDir.string(), folders);
	for (auto& location : _handlerServer->getLocations())
	{
		it = folders.begin();
		valid = _diveLocation(location, it, folders);
		if (valid)
			break ;
	}
	if (!valid)
		return ;
	_validLocation = valid;
}

void	RequestValidate::_separateFolders(std::string const& input, strings_t& output)
{
	std::istringstream iss(input);
	std::string folder;
	while (std::getline(iss, folder, '/'))
	{
		if (!folder.empty())
			output.push_back(folder);
	}
}

// ╭───────────────────────────╮
// │     FILE/FOLDER PERMS     │
// ╰───────────────────────────╯
bool	RequestValidate::_checkPerm(path_t const& path, PermType type)
{
	t_perms perm = std::filesystem::status(path).permissions();
	switch (type)
	{
		case PERM_READ:
			return ((perm & (t_perms::owner_read | t_perms::group_read | t_perms::others_read)) != t_perms::none);
		case PERM_WRITE:
			return ((perm & (t_perms::owner_write | t_perms::group_write | t_perms::others_write)) != t_perms::none);
		case PERM_EXEC:
			return ((perm & (t_perms::owner_exec | t_perms::group_exec | t_perms::others_exec)) != t_perms::none);
		default:
			return ((perm & (t_perms::owner_all | t_perms::group_all | t_perms::others_all)) != t_perms::none);
	}
}

// ╭───────────────────────────╮
// │     ASSIGN BASIC PATHS    │
// ╰───────────────────────────╯
void	RequestValidate::_initTargetElements(void)
{
	_requestPath = std::filesystem::weakly_canonical(_requestPath);
	if (_requestPath.has_filename())
	{
		targetDir = _requestPath.parent_path();
		targetFile = _requestPath.filename();
	}
	else
	{
		targetDir = _requestPath;
		targetFile = "";
	}
}

// ╭───────────────────────────╮
// │GENERAL CHECK FOR THE PATH │
// ╰───────────────────────────╯
bool	RequestValidate::_handleFolder(void)
{
	path_t dirPath = _validParams->getRoot();
	dirPath += targetDir;
	dirPath = std::filesystem::weakly_canonical(dirPath);
	_realPath = dirPath;
	_autoIndex = false;
	if (!std::filesystem::exists(dirPath) ||
	!std::filesystem::is_directory(dirPath))
		return (_setStatusCode(404), false);
	if (!_validParams->getAutoindex())
		return (_setStatusCode(404), false);
	if (!_checkPerm(dirPath, PERM_READ))
		return (_setStatusCode(403), false);
	_autoIndex = true;
	return(true);
}

bool	RequestValidate::_handleFile(void)
{
	path_t dirPath = _validParams->getRoot();
	dirPath += targetDir;
	path_t filePath = dirPath;
	filePath /= std::string(targetFile.filename());
	_isCGI = false;
	if (!std::filesystem::exists(filePath))
		return (_setStatusCode(404), false);
	if (std::filesystem::is_directory(filePath))
		return (_setStatusCode(404), false);
	_realPath = std::filesystem::weakly_canonical(filePath);
	if (_validParams->getCgiAllowed() &&
		filePath.has_extension() &&
		filePath.extension() == _validParams->getCgiExtension())
	{
		if (!_checkPerm(filePath, PERM_EXEC))
			return (_setStatusCode(403), false);
		_isCGI = true;
	}
	else if (!_checkPerm(filePath, PERM_READ))
		return (_setStatusCode(403), false);
	return (true);
}

void	RequestValidate::_handleIndex( void )
{
	Parameters const	indexParam = *_validParams;
	path_t				indexFilePath;

	if (this->_requestMethod != HTTP_GET)	//index but method is not GET
		return (_setStatusCode(400));
	for (auto indexFile : indexParam.getIndex())
	{
		if (indexFile.is_absolute())
			indexFilePath = indexFile;
		else
		{
			indexFilePath = "";
			if (_validLocation != nullptr)
				indexFilePath = _validLocation->getFullPath();
			if (*indexFile.begin() == "/")
				indexFilePath += indexFile;
			else
				indexFilePath /= indexFile;
			indexFilePath = std::filesystem::weakly_canonical(indexFilePath);
		}
		solvePath(this->_requestMethod, indexFilePath, this->_handlerServer->getPrimaryName());
		if (solvePathFailed() == false)
			return ;
	}
}

// ╭───────────────────────────╮
// │  STATUS CODE REDIRECTION  │
// ╰───────────────────────────╯
bool	RequestValidate::_handleReturns(void)
{
	auto const& local = _validParams->getReturns();
	if (local.first)
	{
		_setStatusCode(local.first);
		if (local.second != "")	// file redirect name not provided in return directive, usually an error 40X
		{
			_realPath = local.second;
			_isRedirection = true;
		}
		return (true);
	}
	return (false);
}

// ╭───────────────────────────╮
// │   MAIN FUNCTION TO START  │
// ╰───────────────────────────╯
void	RequestValidate::solvePath( HTTPmethod method, path_t const& path, std::string const& hostName )
{
	_resetValues();
	_setMethod(method);
	_setPath(path);
	_setConfig(hostName);
	_initTargetElements();		// Clean up the _requestPath, Set targetDir and targetFile based on _requestPath
	if (!targetDir.empty() || targetDir == "/")	// if directory is not root check for location
	{
		_initValidLocation();
		if (_validLocation == nullptr)
			return (_setStatusCode(404));
		_validParams = &(_validLocation->getParams());
	}
	if (!_validParams->getAllowedMethods()[_requestMethod])
		return (_setStatusCode(405));	// 405 error, method not allowed
	if (_handleReturns())	// handle return
		return ;

	if ((targetFile.empty() || targetFile == "/") and _hasValidIndex())	// set indexfile if necessarry
		return (_handleIndex());
	targetFile = std::filesystem::weakly_canonical(targetFile);
	if (targetFile.empty() || targetFile == "/")
		_handleFolder();
	else
		_handleFile();
}

void	RequestValidate::solveErrorPath( int statusCode )
{
	path_t	errorPage;

	try {
		if (this->_validLocation != nullptr)
			errorPage = path_t(this->_validLocation->getFullPath());
		errorPage += this->_validParams->getErrorPages().at(statusCode);
	}
	catch(const std::out_of_range& e1) {
		try {
			_resetValues();
			errorPage = this->_handlerServer->getParams().getErrorPages().at(statusCode);
		}
		catch(const std::out_of_range& e2) {
			try {
				this->_handlerServer = this->_defaultServer;
				errorPage = this->_handlerServer->getParams().getErrorPages().at(statusCode);
			}
			catch(const std::out_of_range& e3) {
				throw(RequestException({"config doesn't provide a page for code:", std::to_string(statusCode)}, statusCode));
			}
		}
	}
	_setPath(errorPage);
	_setStatusCode(200);
	_initTargetElements();
	_handleFile();
}
