
#include <fstream>
#include <iostream>
#include "KXRUnpacker.h"

void
KXRUnpacker::ExtractKXRFile( std::string filename )
{
	if( std::filesystem::path( filename ).extension() != ".kxr" )
	{
		std::cout << "Skip file " << filename << std::endl;
		return;
	}

	_kxrFileName = std::filesystem::path( filename ).stem().string();

	// Remove the versioning from the file name
	size_t versionPos = _kxrFileName.find_last_of( "-" );
	if( versionPos != std::string::npos )
	{
		_kxrFileName.erase( _kxrFileName.begin() + versionPos, _kxrFileName.end() );
	}

	// Load the file
	std::fstream file_stream( filename.data(), std::ios::binary | std::ios::in );

	if( !file_stream.is_open() )
	{
		std::cerr << "[Failed to load file]" << filename << std::endl;
		return;
	}

	// Get the file size
	file_stream.seekg( 0, file_stream.end );
	size_t fileSize = static_cast< uint32_t >( file_stream.tellg() );
	file_stream.seekg( 0, file_stream.beg );

	// Allocate space on the buffer and read the contents
	std::vector< unsigned char > fileBuffer( fileSize, 0 );
	file_stream.read( reinterpret_cast< char* >( fileBuffer.data() ), fileSize );

	// Close the file stream
	file_stream.close();

	_kxrContentBuffer = KBuffer( fileBuffer.data(), 0, fileBuffer.size());

	uint32_t file_identity = _kxrContentBuffer.u32();

	if( file_identity != KXRF_Identifier )
	{
		std::cerr << "Invalid KXR File header for input file: " << filename << std::endl;
		return;
	}

	_kxrContentBuffer.u32();
	uint32_t _kxrContentTableOffset = _kxrContentBuffer.u32();
	uint32_t _kxrContentTableSize = _kxrContentBuffer.u32();

	_kxrContentTable = KBuffer( fileBuffer.data(), _kxrContentTableOffset, _kxrContentTableSize);
	_kxrContentTable.crypt( _kxrContentTableOffset );

	RecursiveKXRExtract();
}

void
KXRUnpacker::RecursiveKXRExtract()
{
	std::string entryName = _kxrContentTable.string();
	uint8_t unknown_a = _kxrContentTable.u8();
	uint32_t unknown_b = _kxrContentTable.u32();
	uint32_t entryType = _kxrContentTable.u32();

	switch( entryType )
	{
		case TypeDirectory: UnpackFolder( entryName ); break;
		case TypeFileEncrypted: UnpackEncryptedFile( entryName ); break;
		case TypeFileCompressed: UnpackCompressedFile( entryName ); break;
	}
}

void
KXRUnpacker::UnpackFolder( std::filesystem::path folder_name )
{
	_currentDirectory += folder_name;

	uint16_t nbFilesInFolder = _kxrContentTable.u16();
	for( int i = 0; i < nbFilesInFolder; i++ )
	{
		RecursiveKXRExtract();
	}

	_currentDirectory = _currentDirectory.parent_path();
}

void
KXRUnpacker::UnpackEncryptedFile( std::filesystem::path file_name )
{
	uint32_t offset = _kxrContentTable.u32();
	uint32_t size = _kxrContentTable.u32();

	KBuffer fileBuffer( _kxrContentBuffer.data(), offset, size);
	fileBuffer.crypt( offset );

	std::filesystem::path lookup_name;
	if( false == file_name.has_extension() )
	{
		lookup_name = Package.FindFileName( _kxrFileName, std::atoi( file_name.stem().generic_string().c_str() ) );
	}
	else
	{
		lookup_name = _currentDirectory / file_name;
	}

	SaveToDisk( lookup_name, fileBuffer );

	std::cout << "\t" << lookup_name << std::endl;
}

void
KXRUnpacker::UnpackCompressedFile( std::filesystem::path file_name )
{
	uint32_t offset = _kxrContentTable.u32();
	uint32_t size = _kxrContentTable.u32();

	KBuffer fileBuffer( _kxrContentBuffer.data(), offset, size );

	fileBuffer.decompress();

	std::filesystem::path lookup_name;
	if( false == file_name.has_extension() )
	{
		lookup_name = Package.FindFileName( _kxrFileName, std::atoi( file_name.stem().generic_string().c_str() ) );
	}
	else
	{
		lookup_name = _currentDirectory / file_name;
	}

	SaveToDisk( lookup_name, fileBuffer );

	std::cout << "\t" << lookup_name << std::endl;
}

bool
KXRUnpacker::SetOutputDirectory( std::filesystem::path path )
{
	if( path.is_absolute() )
	{
		_outputDirectory = path;
	}
	else
	{
		_outputDirectory = std::filesystem::current_path() / path;
	}

	return std::filesystem::create_directories( _outputDirectory );
}

void
KXRUnpacker::SaveToDisk( std::filesystem::path file_path, KBuffer buffer )
{
	std::filesystem::path save_path = _outputDirectory / file_path;

	std::filesystem::create_directories( save_path.parent_path() );

	std::fstream save_file( save_path, std::ios::out | std::ios::binary );

	if( save_file.good() )
	{
		save_file.write( ( char* )&buffer.data()[0], buffer.size());
		save_file.close();
	}
}