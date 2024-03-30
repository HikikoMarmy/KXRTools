#pragma once

#include <string>
#include <vector>
#include <filesystem>

#include "KBuffer.hpp"
#include "PackageManager.h"

#define KXRF_Identifier ( 'k' << 24 | 'x' << 16 | 'r' << 8 | 'f' )

class KXRUnpacker
{
	enum KType
	{
		TypeFileEncrypted = 0,
		TypeDirectory = 1,
		TypeFileCompressed = 4,
	};

public:
	KXRUnpacker()
	{
		_currentDirectory.clear();
		_outputDirectory.clear();
	}

	KXRUnpacker( std::filesystem::path operating_directory )
	{
		if( operating_directory.is_absolute() )
		{
			_outputDirectory = operating_directory;
		}
		else
		{
			_outputDirectory = std::filesystem::current_path() / operating_directory;
		}
	}

	~KXRUnpacker()
	{

	}

	void ExtractKXRFile( std::string filename );
	void RecursiveKXRExtract();

	void UnpackFolder( std::filesystem::path folder_name );
	void UnpackEncryptedFile( std::filesystem::path file_name );
	void UnpackCompressedFile( std::filesystem::path file_name );

	bool SetOutputDirectory( std::filesystem::path path );
	void SaveToDisk( std::filesystem::path file_path, KBuffer buffer );


	bool LoadPackageJson( std::string filename )
	{
		return Package.ReadPKGJson( filename );
	}
private:
	PackageManager Package;

	std::filesystem::path _outputDirectory;
	std::filesystem::path _currentDirectory;

	KBuffer _kxrContentBuffer;
	KBuffer _kxrContentTable;
	std::string _kxrFileName;
};