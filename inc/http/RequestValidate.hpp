#pragma once
#include <iostream>
#include <fstream>
#include "Exceptions.hpp"

#include "HTTPstruct.hpp"
#include "Config.hpp"
typedef std::filesystem::perms t_perms;

typedef enum PermType_s
{
	PERM_READ,
	PERM_WRITE,
	PERM_EXEC,
} PermType;

typedef std::vector<Config> t_serv_list;

class RequestValidate
{
	public:
		RequestValidate( t_serv_list const& );
		virtual	~RequestValidate( void ) {};

		void	solvePath( HTTPmethod, path_t const&, std::string const& );
		void	solveErrorPath( int );

		path_t const&		getRealPath( void ) const;
		path_t const&		getRedirectRealPath( void ) const;
		std::string const&	getServName( void ) const;
		std::uintmax_t		getMaxBodySize( void ) const;
		int					getStatusCode( void ) const;
		path_t const&		getRoot( void ) const;
		bool				isAutoIndex( void ) const;
		bool				isFile( void ) const;
		bool				isCGI( void ) const;
		bool				isRedirection( void ) const;
		bool				solvePathFailed( void ) const;

	private:
		t_serv_list	_servers;
		Config		*_defaultServer, *_handlerServer;
		HTTPmethod	_requestMethod;

		size_t	_statusCode;
		bool	_autoIndex, _isCGI,_isRedirection;
		path_t	_requestPath, _realPath, _redirectRealPath, targetDir, targetFile;

		Location const*		_validLocation;
		Parameters const*	_validParams;

		void			_resetValues( void );
		void			_setConfig( std::string const& );
		void			_setDefaultServ ( void ) noexcept;
		void			_setMethod( HTTPmethod );
		void			_setPath( path_t const& );
		bool			_hasValidIndex( void ) const;

		bool			_checkPerm(path_t const& path, PermType type);
		void			_separateFolders(std::string const& input, strings_t& output);
		Location const*	_diveLocation(Location const& cur, strings_t::iterator itDirectory, strings_t& folders);

		void	_initValidLocation( void );
		void	_initTargetElements( void );

		bool	_handleFolder( void );
		bool	_handleFile( void );
		bool	_handleReturns( void );
		void	_handleIndex( void );

		void	_setStatusCode(const size_t& code);
};
