#include <iostream>
#include "PackageManager.h"

bool
PackageManager::ReadPKGJson( std::string filename )
{
	std::cout << "Loading pkg.json..." << std::endl;

	std::ifstream file( filename.c_str(), std::ios::binary );
	if( !file.is_open() )
	{
		return false;
	}

	try
	{
		nlohmann::json reader = nlohmann::json::parse( file );

		_kxrList.clear();
		_kxrFileList.clear();
		_currentPath.clear();
		_packageReady = false;

		BeginParseJson( reader );
	}
	catch( std::exception e )
	{
		std::cout << e.what() << std::endl;
		return false;
	}

	return true;
}

void
PackageManager::BeginParseJson( nlohmann::json root )
{
	if( root.end() == root.find( "kxrlist" ) )
	{
		throw std::exception( "pkg.json missing 'kxrlist' entry!" );
	}

	uint32_t kxrId = 0;
	for( auto kxr : root[ "kxrlist" ] )
	{
		_kxrList[ kxr.at( "kxrname" ) ] = kxrId++;
	}

	RecursiveParseJson( root[ "entries" ] );

	_packageReady = true;
}

void
PackageManager::RecursiveParseJson( nlohmann::json list )
{
	for( auto entry : list )
	{
		std::string name = entry.at( "name" );

		if( entry.find( "list" ) != entry.end() )
		{
			// Is a folder
			_currentPath.append( name );
			RecursiveParseJson( entry[ "list" ] );
			_currentPath = _currentPath.parent_path();
		}
		else
		{
			// Is a file
			uint32_t kxr = entry.at( "kxr" );
			uint32_t eid = entry.at( "eid" );

			_kxrFileList[ kxr ].push_back( { kxr, eid, ( _currentPath / name ) } );
		}
	}
}

std::filesystem::path
PackageManager::FindFileName( std::string kxrName, uint32_t eid )
{
	if( _packageReady )
	{
		uint32_t kxr = _kxrList[ kxrName ];

		if( eid < _kxrFileList[ kxr ].size() )
		{
			std::vector< KXRJsonEntry >::const_iterator iter = ( _kxrFileList[ kxr ].begin() + eid );
			if( ( *iter ).eid == eid )
			{
				return ( *iter ).name;
			}
		}

		for( auto i : _kxrFileList[ kxr ] )
		{
			if( eid == i.eid )
			{
				return i.name;
			}
		}
	}

	return kxrName + "/" + std::to_string( eid ) + ".bin";
}