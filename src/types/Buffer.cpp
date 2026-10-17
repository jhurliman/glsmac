#include <cstring>
#include <new>
#include <utility>

#include "Buffer.h"

namespace types {

Buffer::Buffer() {
	allocated_len = 0;
	lenw = 0;
	lenr = 0;
	data = nullptr;
	dw = nullptr;
	dr = nullptr;
}

Buffer::Buffer( const std::string& val )
	: Buffer() {
	if ( val.empty() ) { return; }
	allocated_len = val.size();
	lenw = val.size();
	data = (data_t*)malloc( lenw );
	memcpy( ptr( data, 0, lenw ), val.data(), lenw );
	dw = data + lenw;
	dr = data;
}

Buffer::~Buffer() {
	if ( data ) {
		free( data );
	}
}

Buffer::Buffer( const Buffer& other ) {
	allocated_len = other.allocated_len;
	lenw = other.lenw;
	lenr = other.lenr;
	if ( other.data ) {
		data_t* newptr = (data_t*)malloc( allocated_len );
		data = newptr;
		memcpy( ptr( data, 0, lenw ), ptr( other.data, 0, other.lenw ), lenw );
	}
	else {
		data = nullptr;
	}
	dw = data ? data + lenw : nullptr;
	dr = data ? data + lenr : nullptr;
}

Buffer& Buffer::operator=( const Buffer& other ) {
	if ( this != &other ) {
		Buffer copy( other );
		std::swap( data, copy.data );
		std::swap( dw, copy.dw );
		std::swap( dr, copy.dr );
		std::swap( allocated_len, copy.allocated_len );
		std::swap( lenw, copy.lenw );
		std::swap( lenr, copy.lenr );
	}
	return *this;
}

void Buffer::Alloc( uint32_t size ) {
	const uint32_t new_size = lenw + size;
	if ( new_size > allocated_len ) {
		while ( new_size > allocated_len ) {
			allocated_len += BUFFER_ALLOC_CHUNK;
		}
		if ( data ) {
			//Log( "Reallocating " + to_string( allocated_len ) + " bytes" );
			data = (data_t*)realloc( data, allocated_len );
		}
		else {
			//Log( "Allocating " + to_string( allocated_len ) + " bytes" );
			data = (data_t*)malloc( allocated_len );
		}
		dw = ptr( data, lenw, 0 );
		dr = ptr( data, lenr, 0 );
	}
	lenw = new_size;
}

// note: mostly THROWs instead of ASSERTs, because we need that validation in release mode too to prevent buffer overflows
void Buffer::WriteImpl( type_t type, const char* s, const uint32_t sz ) {
	ASSERT( type > T_NONE && type < T_MAX, "invalid buffer write type " + std::to_string( type ) );
	//Log( "Writing " + to_string( sz ) + " bytes (type=" + to_string( type ) + ")" );
	checksum_t c = 0;
	Alloc( sizeof( type ) + sizeof( sz ) + sz + sizeof( c ) );
	memcpy( dw, &type, sizeof( type ) );
	dw += sizeof( type );
	memcpy( dw, &sz, sizeof( sz ) );
	dw += sizeof( sz );

	for ( uint32_t i = 0 ; i < sz ; i++ ) {
		c ^= ( *( dw++ ) = *( s++ ) );
	}

	//Log( "Writing checksum (" + to_string( c ) + ")" );
	*( dw++ ) = c;

	ASSERT( dw - data == lenw, "buffer write bytes count mismatch ( " + std::to_string( dw - data ) + " != " + std::to_string( lenw ) + " )" );
	//Log( "Written successfully" );
}

char* Buffer::ReadImpl( type_t need_type, char* s, uint32_t* sz, const std::optional< uint32_t > need_sz ) {
	ASSERT( need_type > T_NONE && need_type < T_MAX, "invalid buffer read type " + std::to_string( need_type ) );
	// Parse and validate the complete frame before allocating, changing the
	// caller's output, or advancing the cursor. Subtract from bytes remaining
	// instead of adding an untrusted size to the uint32_t read offset.
	constexpr auto header_size = sizeof( type_t ) + sizeof( uint32_t );
	if ( lenr > lenw || lenw - lenr < header_size ) {
		THROW( "buffer ends prematurely (while reading header)" );
	}
	type_t type = T_NONE;
	memcpy( &type, dr, sizeof( type ) );
	if ( type != need_type ) {
		THROW( "unexpected type on buffer read ( " + std::to_string( need_type ) + " != " + std::to_string( type ) + " )" );
	}
	uint32_t payload_size = 0;
	memcpy( &payload_size, dr + sizeof( type ), sizeof( payload_size ) );
	const auto body_available = lenw - lenr - header_size;
	if ( body_available < sizeof( checksum_t ) || payload_size > body_available - sizeof( checksum_t ) ) {
		THROW( "buffer ends prematurely (while reading data)" );
	}
	if ( need_sz && *need_sz != payload_size ) {
		THROW( "buffer read size mismatch ( " + std::to_string( *need_sz ) + " != " + std::to_string( payload_size ) + " )" );
	}

	const auto* payload = dr + header_size;
	checksum_t need_c = 0;
	for ( uint32_t i = 0 ; i < payload_size ; i++ ) {
		need_c ^= payload[ i ];
	}
	const checksum_t c = payload[ payload_size ];
	if ( need_c != c ) {
		THROW( "buffer read checksum mismatch ( " + std::to_string( need_c ) + " != " + std::to_string( c ) + " )" );
	}
	if ( payload_size > 0 ) {
		if ( s == nullptr ) {
			s = (char*)malloc( payload_size );
			if ( !s ) {
				throw std::bad_alloc();
			}
		}
		memcpy( s, payload, payload_size );
	}
	const auto consumed = header_size + payload_size + sizeof( checksum_t );
	*sz = payload_size;
	lenr += consumed;
	dr += consumed;
	ASSERT( dr - data == lenr, "buffer read bytes count mismatch ( " + std::to_string( dr - data ) + " != " + std::to_string( lenr ) + " )" );

	return s;
}

void Buffer::WriteBool( const bool val ) {
	const uint8_t bval = val
		? 1
		: 0;
	WriteImpl( T_BOOL, (const char*)&bval, sizeof( bval ) );
}

const bool Buffer::ReadBool() {
	uint8_t boolval = 0;
	uint32_t sz = 0;
	ReadImpl( T_BOOL, (char*)&boolval, &sz, sizeof( boolval ) );
	if ( boolval > 1 ) {
		THROW( "invalid boolean value in buffer: " + std::to_string( boolval ) );
	}
	return boolval != 0;
}

void Buffer::WriteInt( const long long int val ) {
	WriteImpl( T_INT, (const char*)&val, sizeof( val ) );
}

const long long int Buffer::ReadInt() {
	long long int val = 0;
	uint32_t sz = 0;
	ReadImpl( T_INT, (char*)&val, &sz, sizeof( val ) );
	return val;
}

void Buffer::WriteFloat( const float val ) {
	WriteImpl( T_FLOAT, (const char*)&val, sizeof( val ) );
}

const float Buffer::ReadFloat() {
	float val = 0;
	uint32_t sz = 0;
	ReadImpl( T_FLOAT, (char*)&val, &sz, sizeof( val ) );
	return val;
}

void Buffer::WriteString( const std::string& val ) {
	WriteImpl( T_STRING, val.data(), val.size() );
}

const std::string Buffer::ReadString() {
	uint32_t sz = 0;
	char* res_data = ReadImpl( T_STRING, nullptr, &sz );
	if ( sz > 0 ) {
		std::string result = std::string( res_data, sz );
		free( res_data );
		return result;
	}
	else {
		return "";
	}
}

void Buffer::WriteVec2u( const Vec2< uint32_t > val ) {
	WriteImpl( T_VEC2U, (const char*)&val, sizeof( val ) );
}

const Vec2< uint32_t > Buffer::ReadVec2u() {
	types::Vec2< uint32_t > val = {
		0,
		0
	};
	uint32_t sz = 0;
	ReadImpl( T_VEC2U, (char*)&val, &sz, sizeof( val ) );
	return val;
}

void Buffer::WriteVec2f( const Vec2< float > val ) {
	WriteImpl( T_VEC2F, (const char*)&val, sizeof( val ) );
}

const Vec2< float > Buffer::ReadVec2f() {
	types::Vec2< float > val = {
		0,
		0
	};
	uint32_t sz = 0;
	ReadImpl( T_VEC2F, (char*)&val, &sz, sizeof( val ) );
	return val;
}

void Buffer::WriteVec3( const types::Vec3 val ) {
	WriteImpl( T_VEC3, (const char*)&val, sizeof( val ) );
}

const types::Vec3 Buffer::ReadVec3() {
	types::Vec3 val;
	uint32_t sz = 0;
	ReadImpl( T_VEC3, (char*)&val, &sz, sizeof( val ) );
	return val;
}

void Buffer::WriteColor( const Color& val ) {
	WriteImpl( T_COLOR, (const char*)&val.value, sizeof( val.value ) );
}

void Buffer::ReadColor( Color& val ) {
	uint32_t sz = 0;
	ReadImpl( T_COLOR, (char*)&val.value, &sz, sizeof( val.value ) );
}

void Buffer::WriteData( const void* data, const uint32_t len ) {
	WriteImpl( T_DATA, (const char*)data, len );
}

const void* Buffer::ReadData( const uint32_t len ) {
	uint32_t sz = 0;
	return ReadImpl( T_DATA, nullptr, &sz, len );
}

const std::string Buffer::ToString() const {
	return data
		? std::string( (const char*)data, lenw )
		: "";
}

}
