#pragma once

#include <vector>
#include <string>
#include <iostream>
#include "math.h"

#define _CRT_SECURE_NO_WARNINGS
#include "zlib.h"

class KBuffer
{
private:
	uint32_t	_pos;
	bool		_isWriteBuffer;
	std::vector< unsigned char > _raw;

public:
	KBuffer()
	{
		_pos = 0;
		_isWriteBuffer = false;
	}

	~KBuffer()
	{
		_pos = 0;
		_isWriteBuffer = false;
	}

	KBuffer( unsigned char* src_buffer, uint32_t src_pos, uint32_t src_size )
	{
		_pos = 0;
		_raw.insert( _raw.begin(), src_buffer + src_pos, src_buffer + src_pos + src_size );
		_isWriteBuffer = false;
	}

	KBuffer operator=( const KBuffer& bb )
	{
		this->_pos = bb._pos;
		this->_raw.resize( bb._raw.size() );
		std::copy( bb._raw.begin(), bb._raw.end(), this->_raw.begin() );
		return *this;
	}

	size_t size() const
	{
		return ( _isWriteBuffer ) ? _pos : _raw.size();
	}

	unsigned char* data()
	{
		return _raw.data();
	}

	void crypt( uint32_t magic )
	{
		for( uint32_t i = 0; i < _raw.size(); )
		{
			if( ( i % 4 == 0 ) && ( i > 0 ) )
			{
				magic = magic * 2 | ~( ( magic >> 3 ^ magic ) >> 0x0D ) & 1;
			}

			if( i + 4 < _raw.size() )
			{
				*( uint32_t* )&_raw[ i ] ^= magic;
				i += 4;
			}
			else
			{
				_raw[ i ] ^= ( ( magic >> ( ( 8 * ( i % 4 ) ) & 0xFF ) ) );
				i++;
			}
		}
	}

	void compress()
	{
		size_t source_length = _raw.size();
		uLongf destination_length = compressBound( source_length );

		char* destination_data = ( char* )malloc( destination_length );
		if( destination_data == nullptr )
		{
			return;
		}

		Bytef* source_data = ( Bytef* )_raw.data();
		int return_value = compress2( ( Bytef* )destination_data, &destination_length, source_data, source_length, Z_BEST_COMPRESSION );

		_raw.clear();
		_raw.insert( _raw.end(), destination_data, destination_data + destination_length );

		_pos = destination_length;

		free( destination_data );
	}

	void decompress()
	{
		z_stream zs;
		memset( &zs, 0, sizeof( zs ) );

		if( inflateInit( &zs ) != Z_OK )
			throw( std::runtime_error( "inflateInit failed while decompressing." ) );

		zs.next_in = ( Bytef* )_raw.data();
		zs.avail_in = _raw.size();

		int ret;
		char outbuffer[ 10240 ];
		std::vector< uint8_t > decomp_buffer;

		do
		{
			zs.next_out = reinterpret_cast< Bytef* >( outbuffer );
			zs.avail_out = sizeof( outbuffer );

			ret = inflate( &zs, 0 );

			if( decomp_buffer.size() < zs.total_out )
			{
				std::copy( outbuffer, outbuffer + ( zs.total_out - decomp_buffer.size() ), std::back_inserter( decomp_buffer ) );
			}

		} while( ret == Z_OK );

		inflateEnd( &zs );

		if( ret != Z_STREAM_END )
		{
			// TODO: Probably some kind of error and/or halt...
		}
		else
		{
			_raw.swap( decomp_buffer );
		}
	}

	inline void resize( size_t write_size )
	{
		if( _pos + write_size > _raw.size() )
		{
			_raw.resize( _pos + write_size + 256 );
		}
	}

	template < typename Type >
	Type read()
	{
		if( _pos + sizeof( Type ) > _raw.size() )
		{
			std::cerr << "End of stream" << std::endl;
			return { 0 };
		}

		Type ret = *( Type* )&_raw.data()[ _pos ];
		_pos += sizeof( Type );

		return ret;
	}

	uint8_t u8()
	{
		return read< uint8_t >();
	}
	uint16_t u16()
	{
		uint16_t val = read< uint16_t >();
		return ( ( val & 0x00FF ) << 8 ) | ( ( val & 0xFF00 ) >> 8 );
	}
	uint32_t u32()
	{
		uint32_t val = read< uint32_t >();
		return	( ( val & 0x000000FF ) << 24 ) |
			( ( val & 0x0000FF00 ) << 8 ) |
			( ( val & 0x00FF0000 ) >> 8 ) |
			( ( val & 0xFF000000 ) >> 24 );
	}

	std::string string()
	{
		uint16_t str_length = u16();

		if( _pos + str_length > _raw.size() )
		{
			std::cerr << "End of stream" << std::endl;
			return "";
		}

		std::vector< unsigned char >::const_iterator l = _raw.begin() + _pos;
		std::vector< unsigned char >::const_iterator r = _raw.begin() + ( _pos += ( str_length ) );

		return std::string( l, r );
	}
};