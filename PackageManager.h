#pragma once

#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>

#include <json.hpp>

class PackageManager
{
	struct KXRJsonEntry
	{
		uint32_t kxr;
		uint32_t eid;
		std::filesystem::path name;
	};

	std::filesystem::path _currentPath;

	std::map< std::string, uint32_t >	_kxrList;
	std::map< uint32_t, std::vector< KXRJsonEntry > >	_kxrFileList;
	bool _packageReady;

public:
	

	PackageManager()
	{
		_packageReady = false;
		_currentPath.clear();
		_kxrList.clear();
		_kxrFileList.clear();
	}

	~PackageManager()
	{

	}

	bool ReadPKGJson( std::string filename );
	std::filesystem::path FindFileName( std::string kxrName, uint32_t eid );

private:
	void BeginParseJson( nlohmann::json root );
	void RecursiveParseJson( nlohmann::json list );
};