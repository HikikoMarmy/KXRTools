// https://github.com/HikikoMarmy/KXRTools

#include <iostream>
#include <vector>
#include <windows.h>
#include <shlobj.h>

#include "math.h"
#include <json.hpp>

#include "KXRUnpacker.h"

std::vector< std::string > target_file_list;
std::filesystem::path operating_directory;
std::filesystem::path cobralaunch_path;
std::filesystem::path pkg_path;

bool WaitForConfirmation( const char* prompt )
{
	std::string in;
	std::cout << prompt << std::endl;
	std::cout << "Continue? [y/n]" << std::endl;

	if( std::getline( std::cin, in ) )
	{
		// Only accept single character input
		if( in.length() == 1 )
		{
			if( in[ 0 ] == 'y' || in[ 0 ] == 'Y' ) return true;
		}
	}
	return false;
}

// Gets the cobra cache path from AppData
std::string GetCobraAppData()
{
	std::filesystem::path path;
	PWSTR path_tmp;

	if( S_OK == SHGetKnownFolderPath( FOLDERID_RoamingAppData, 0, nullptr, &path_tmp ) )
	{
		path = std::filesystem::path( path_tmp ) / "cobra" / "cache";
	}

	CoTaskMemFree( path_tmp );

	return path.string();
}

void AddFileToList( std::filesystem::path input_path )
{
	if( false == input_path.has_filename() ) return;

	if( input_path.extension() == ".kxr" )
	{
		// Find Provided launcher either as CobraLaunch.kxr or hashed.kxr
		if( cobralaunch_path.empty() &&
			( input_path.string().find( "cobralaunch" ) != std::string::npos
			  || input_path.stem().string().find_first_not_of( "0123456789abcdefABCDEF" ) == std::string::npos ) )
		{
			cobralaunch_path = input_path;
			std::cout << "Found CobraLaunch as file: " << input_path.filename() << std::endl;
		}
		else
		{
			target_file_list.push_back( input_path.string() );
		}
	}
	else if( input_path.filename() == "pkg.json" )
	{
		std::filesystem::path pkg_path = input_path;
		std::cout << "Found pkg.json as file: " << input_path.filename() << std::endl;
	}
	else
	{
		std::cerr << "Ignoring input file: " << input_path.filename() << std::endl;
	}
}

int main( int argc, const char* argv[] )
{
	if( argc == 1 )
	{
		std::cout << "----------------------------------------------" << std::endl;
		std::cout << "KXRTools Version 1.0" << std::endl;
		std::cout << "----------------------------------------------" << std::endl << std::endl;

		std::cout << "Drag and drop *.kxr files or a folder onto KXRTools to unpack them." << std::endl << std::endl;

		std::cout
			<< "By default KXRTools will use the cobralaunch data in your appdata folder." << std::endl
			<< "To override this behaviour you can either : " << std::endl
			<< "* Provide cobralaunch.kxr in the selection of files." << std::endl
			<< "* Provide a valid 'pkg.json' in the selection of files." << std::endl << std::endl;

		system( "pause" );
		return 0;
	}

	// The operating directory is the path that KXR Tools is in, preferably.
	operating_directory = std::filesystem::path( argv[ 0 ] ).parent_path();

	std::vector< std::string > arg_list( argv + 1, argv + argc );

	// We prefer the cobra launch from AppData, but will be overridden if one is provided in the file list
	arg_list.push_back( GetCobraAppData() );

	// Parse Input Arguments
	for( auto arg = arg_list.begin(); arg != arg_list.end(); arg++ )
	{
		std::filesystem::path input_path = ( *arg );

		if( std::filesystem::is_regular_file( input_path ) )
		{
			AddFileToList( input_path );
		}
		else
		{
			for( std::filesystem::recursive_directory_iterator end, dir( input_path ); dir != end; ++dir )
			{
				if( dir->is_regular_file() )
				{
					AddFileToList( dir->path() );
				}
			}
		}
	}

	KXRUnpacker Unpacker;
	Unpacker.SetOutputDirectory( operating_directory / "output" );

	// Parse CobraLaunch before any other files.
	// We do this for an up to date pkg.json.
	if( cobralaunch_path.has_filename() )
	{
		std::cout << "Attempting to parse CobraLaunch..." << std::endl;
		Unpacker.ExtractKXRFile( cobralaunch_path.string() );

		if( std::filesystem::exists( operating_directory / "output/pkg.json" ) )
		{
			pkg_path = operating_directory / "output/pkg.json";
		}
	}

	// No (other) files to parse
	if( target_file_list.empty() )
	{
		std::cerr << "No valid files were passed to KXRTools." << std::endl;
		system( "pause" );
		return 0;
	}

	// Try to load the pkg.json before parsing files.
	// This will let us name and path files correctly.
	if( false == Unpacker.LoadPackageJson( pkg_path.string() ) )
	{
		if( false == WaitForConfirmation( "Failed to find or parse 'pkg.json'.\nOutput files will NOT be named correctly." ) )
		{
			return 0;
		}
	}

	// Begin parsing the files in the input list.
	for( auto filename : target_file_list )
	{
		Unpacker.ExtractKXRFile( filename );
	}

	// Extraction Complete
	std::cout << "Extraction Complete!" << std::endl;

	system( "pause" );
}