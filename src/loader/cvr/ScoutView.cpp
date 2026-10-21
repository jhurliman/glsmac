#include "ScoutView.h"

#include <charconv>

#include "ScoutArithmetic.h"
#include "ScoutLighting.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>

namespace loader::cvr::scout {

Matrix MultiplyMatrix( const Matrix& a, const Matrix& b ) {
	// Each cell follows its observed641bb0 product and addition schedule.
	static constexpr unsigned order[ 9 ][ 3 ]{
		{ 1, 2, 0 }, { 0, 1, 2 }, { 0, 2, 1 },
		{ 0, 2, 1 }, { 0, 2, 1 }, { 2, 1, 0 },
		{ 0, 1, 2 }, { 2, 1, 0 }, { 0, 1, 2 }
	};
	Matrix result;
	for ( size_t cell = 0; cell < 9; ++cell ) {
		std::array< Number, 3 > terms;
		for ( const auto term : order[ cell ] ) { terms[ term ] = Multiply( Decode( a[ cell / 3 * 3 + term ] ), Decode( b[ term * 3 + cell % 3 ] ) ); }
		result[ cell ] = Store( Add( Add( terms[ order[ cell ][ 0 ] ], terms[ order[ cell ][ 1 ] ] ), terms[ order[ cell ][ 2 ] ] ) );
	}
	return result;
}

Vector MultiplyVector( const Matrix& matrix, const Vector& vector ) {
	Vector result;
	for ( size_t row = 0; row < 3; ++row ) {
		result[ row ] = Store( Add( Add( Multiply( Decode( matrix[ row * 3 + 1 ] ), Decode( vector[ 1 ] ) ),
			Multiply( Decode( matrix[ row * 3 + 2 ] ), Decode( vector[ 2 ] ) ) ),
			Multiply( Decode( matrix[ row * 3 ] ), Decode( vector[ 0 ] ) ) ) );
	}
	return result;
}

Vector AddVector( const Vector& a, const Vector& b ) {
	Vector result;
	for ( size_t i = 0; i < 3; ++i ) { result[ i ] = Store( Add( Decode( a[ i ] ), Decode( b[ i ] ) ) ); }
	return result;
}

namespace {

uint32_t Trig( const uint32_t input, const bool cosine ) {
	const double angle = ToDouble( Decode( input ) );
	const float value = static_cast< float >( cosine ? std::cos( angle ) : std::sin( angle ) );
	uint32_t bits;
	std::memcpy( &bits, &value, sizeof( bits ) );
	(void)Decode( bits );
	return bits;
}

std::string_view Trim( std::string_view value ) {
	const auto whitespace = []( const char c ) { return c == ' ' || c == '\t' || c == '\r'; };
	while ( !value.empty() && whitespace( value.front() ) ) { value.remove_prefix( 1 ); }
	while ( !value.empty() && whitespace( value.back() ) ) { value.remove_suffix( 1 ); }
	return value;
}

}

void ValidateMapLight( const std::string_view source ) {
	if ( source.size() > 64 * 1024 || source.find( '\0' ) != std::string_view::npos ) { throw std::invalid_argument( "CVR Rover: invalid system source" ); }
	bool active = false, found = false, have_row = false;
	std::array< int32_t, 3 > light{};
	for ( size_t start = 0; start < source.size(); ) {
		auto end = source.find( '\n', start ); if ( end == std::string_view::npos ) { end = source.size(); }
		auto line = Trim( source.substr( start, end - start ) ); start = end == source.size() ? end : end + 1;
		line = Trim( line.substr( 0, line.find( ';' ) ) );
		if ( line.empty() ) { continue; }
		if ( line.front() == '#' ) {
			active = line == "#VEH_LIGHT_DIR_MAP";
			if ( active && found ) { throw std::invalid_argument( "CVR Rover: duplicate map light section" ); }
			found = found || active; continue;
		}
		if ( !active ) { continue; }
		if ( have_row ) { throw std::invalid_argument( "CVR Rover: extra map light row" ); }
		for ( size_t i = 0; i < light.size(); ++i ) {
			const auto comma = line.find( ',' );
			if ( ( i < 2 ) != ( comma != std::string_view::npos ) ) { throw std::invalid_argument( "CVR Rover: map light requires three integers" ); }
			const auto field = Trim( line.substr( 0, comma ) );
			const auto parsed = std::from_chars( field.data(), field.data() + field.size(), light[ i ] );
			if ( parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size() ) { throw std::invalid_argument( "CVR Rover: invalid map light integer" ); }
			if ( comma != std::string_view::npos ) { line.remove_prefix( comma + 1 ); }
		}
		have_row = true;
	}
	if ( !have_row || light != std::array< int32_t, 3 >{ 6, 4, 15 } ) { throw std::invalid_argument( "CVR Rover: map light outside the admitted system profile" ); }
}

View MakeView( const CvrScoutViewInput& input, const IdleProfile profile ) {
	RequireNearestRounding();
	if ( input.facing < -1 || input.facing > 7 || input.zoom < -14 || input.zoom > 16 || input.scale_denominator <= 0 ) {
		throw std::invalid_argument( "CVR Scout: view outside bounded idle domain" );
	}
	const auto facing = input.facing == -1 ? 0 : input.facing;
	const auto k = ( facing + 5 ) % 8;
	const auto phi = Store( Multiply( Integer( k ), Decode( 0x3f490e56 ) ) );
	const auto c = Trig( phi, true ), s = Trig( phi, false );
	const Matrix rotation{ c, s ^ 0x80000000U, 0, s, c, 0, 0, 0, 0x3f800000 };
	const auto u = Trig( 0xbf85f92d, true ), v = Trig( 0xbf85f92d, false );
	View result;
    result.orientation=rotation;
	result.auxiliary = { 0x3f800000, 0, 0, 0, u, v, 0x80000000, v, u ^ 0x80000000U };
	Number base;
	int64_t delta;
	if ( input.scale_numerator > 0 ) {
		base = Divide( Multiply( Decode( 0x3d8f5c29 ), Integer( input.scale_numerator ) ), Integer( input.scale_denominator ) );
		delta = int64_t{ 50 } * input.scale_numerator / input.scale_denominator;
	}
	else {
		base = Multiply( Multiply( Decode( 0x3d8f5c29 ), Integer( input.zoom + 16 ) ), Decode( 0x3d800000 ) );
		delta = int64_t{ 50 } * ( input.zoom + 16 ) / 16;
	}
	const auto multiplier = profile == IdleProfile::InitializedSmallInfantry ? 0x3f4ccccdU : 0x3fa00000U;
	result.scales = { profile == IdleProfile::SpartanSpeeder ? Store( base ) : Store( Multiply( Decode( Store( base ) ), Decode( multiplier ) ) ), 0x3f000000 };
	result.projection = MultiplyMatrix( result.auxiliary, rotation );
	for ( size_t i = 0; i < 9; ++i ) { result.projection[ i ] = Store( Multiply( Decode( result.projection[ i ] ), Decode( result.scales[ i < 6 ? 0 : 1 ] ) ) ); }
	result.translation = { 0x43008000, 0x43008000, 0x47138780 };
	result.light = { 0xbf13cd36, 0x3f13cd36, 0x3f13cd36 };
	if ( profile != IdleProfile::Infantry ) {
		// Load validates the installed map-light tuple6,4,15. Normalize those
		// integers here; no captured normalized vector or lighting window.
		const float inverse_length = static_cast< float >( 1.0 / std::sqrt( 6.0 * 6.0 + 4.0 * 4.0 + 15.0 * 15.0 ) );
		for ( size_t i = 0; i < 3; ++i ) {
			const float component = static_cast< float >( ( i == 0 ? 6.0 : i == 1 ? 4.0 : 15.0 ) * inverse_length );
			std::memcpy( &result.light[ i ], &component, sizeof( component ) );
		}
	}
	for ( size_t i = 0; i < 2; ++i ) {
		const int64_t anchor = int64_t( input.anchor[ i ] ) + delta;
		if ( anchor < INT32_MIN || anchor > INT32_MAX ) { throw std::invalid_argument( "CVR Scout: target anchor overflow" ); }
		result.target_anchor[ i ] = static_cast< int32_t >( anchor );
	}
	return result;
}


WorkshopBounds WorkshopObjectBounds(const WorkshopBoundsInput& input) {
    RequireNearestRounding();
    const auto scalar=Store(Multiply(Decode(input.scalar),Decode(input.object_scale)));
    const auto matrix=MultiplyMatrix(input.rotation,input.frame_matrix);
    auto location=MultiplyVector(input.rotation,input.frame_location);
    for(size_t i=0;i<3;++i) {
        location[i]=Store(Add(Decode(Store(Multiply(Decode(location[i]),Decode(input.object_scale)))),Decode(input.position[i])));
    }
    Vector offset;
    for(size_t i=0;i<3;++i) {offset[i]=Store(Multiply(Integer(input.offset[i]),Decode(scalar)));}
    const auto origin=AddVector(MultiplyVector(matrix,offset),location);
    std::array<Vector,3> edges;
    for(size_t column=0;column<3;++column) {
        auto length=Multiply(Integer(input.dimensions[column]),Decode(scalar));
        if(column==2) {length=Decode(Store(length));}
        for(size_t axis=0;axis<3;++axis) {edges[column][axis]=Store(Multiply(Decode(matrix[axis*3+column]),length));}
    }
    WorkshopBounds result{origin,origin};std::array<Number,3> point;
    for(size_t i=0;i<3;++i) {point[i]=Decode(origin[i]);}
    const auto visit=[&] {
        for(size_t i=0;i<3;++i) {
            if(ToDouble(point[i])<ToDouble(Decode(result.minimum[i]))) {result.minimum[i]=Store(point[i]);}
            if(ToDouble(point[i])>ToDouble(Decode(result.maximum[i]))) {result.maximum[i]=Store(point[i]);}
        }
    };
    const auto step=[&](size_t edge,bool subtract) {
        for(size_t i=0;i<3;++i) {point[i]=subtract?Subtract(point[i],Decode(edges[edge][i])):Add(point[i],Decode(edges[edge][i]));}
    };
    // Preserve the original walk's asymmetric spills, not eight independently
    // rounded origin+edge sums. This changes real bounds at binary32 precision.
    step(0,false);for(auto& value:point) {value=Decode(Store(value));}visit();
    step(1,false);visit();step(0,true);point[2]=Decode(Store(point[2]));visit();
    step(2,false);visit();step(0,false);visit();step(1,true);visit();
    step(0,true);point[1]=Decode(Store(point[1]));point[2]=Decode(Store(point[2]));visit();
    return result;
}

View MakeWorkshopView(const Vector& position,const uint32_t yaw_bits,const uint32_t scale_bits,const std::array<int32_t,2> anchor) {
    RequireNearestRounding();
    const auto yaw=Decode(yaw_bits);
    const auto c=Trig(yaw_bits,true),s=Trig(yaw_bits,false);
    View result;result.orientation={c,s^0x80000000u,0,s,c,0,0,0,0x3f800000};
    // Original Workshop constructor camera, not the map's tilt/scale defaults.
    result.auxiliary={0x3f800000,0,0,0,0x3f351765,0xbf34f27f,0,0xbf34f27f,0xbf351765};
    Vector camera,relative;
    for(size_t i=0;i<3;++i) {
        camera[i]=Store(Multiply(Decode(result.auxiliary[6+i]),Integer(-10000)));
        relative[i]=Store(Subtract(Decode(position[i]),Decode(camera[i])));
    }
    const auto transformed=MultiplyVector(result.auxiliary,relative);
    const auto depth_scale=Store(Divide(Integer(65535),Integer(131070)));
    result.scales={scale_bits,depth_scale};
    result.projection=MultiplyMatrix(result.auxiliary,result.orientation);
    for(size_t i=0;i<9;++i) {result.projection[i]=Store(Multiply(Decode(result.projection[i]),Decode(result.scales[i<6?0:1])));}
    for(size_t i=0;i<2;++i) {result.translation[i]=Store(Add(Decode(Store(Multiply(Decode(transformed[i]),Decode(result.scales[0])))),Decode(0x43008000)));}
    const auto depth=Store(Subtract(Decode(transformed[2]),Integer(-65535)));
    result.translation[2]=Store(Multiply(Decode(depth),Decode(depth_scale)));
    const float inverse_length=static_cast<float>(1.0/std::sqrt(277.0));
    for(size_t i=0;i<3;++i) {
        const float component=static_cast<float>((i==0?6.0:i==1?4.0:15.0)*inverse_length);
        std::memcpy(&result.light[i],&component,sizeof(component));
    }
    result.target_anchor=anchor;return result;
}

ShadowScene FlatScoutShadowScene(const CvrScoutViewInput& input) {
    const auto view=MakeView(input);
    const auto c=Decode(view.auxiliary[4]),n=Decode(view.auxiliary[5]);
    const auto q=Decode(Store(Add(Multiply(c,c),Multiply(n,n))));
    const auto iq=Decode(Store(Divide(Integer(1),q)));
    const Matrix inverse{Store(Multiply(q,iq)),0,0,0,Store(Multiply(c,iq)),Store(Multiply(n,iq)),
        0,Store(Multiply(n,iq)),Store(Negate(Multiply(c,iq)))};
    Vector position;
    for(size_t i=0;i<3;++i) {position[i]=Store(Multiply(Decode(view.auxiliary[6+i]),Integer(-10000)));}
    return {view.auxiliary,view.orientation,position,{0,0,0},MultiplyVector(inverse,view.light),
        view.scales[0],0x3f800000,0,{0x3f800000,0x3f800000},{0,0},{0,0},256,256};
}

ShadowView MakeShadowView(const ShadowScene& scene) {
    if(!scene.width || !scene.height || scene.width>4096 || scene.height>4096) {
        throw std::invalid_argument("CVR shadow: unsupported surface dimensions");
    }
    auto dz=Decode(scene.direction[2]);const auto epsilon=Decode(0x358637bd);
    if(std::abs(ToDouble(dz))<ToDouble(epsilon)) {dz=dz.negative && dz.magnitude?Negate(epsilon):epsilon;}
    const auto rx=Divide(Decode(scene.direction[0]),dz),ry=Divide(Decode(scene.direction[1]),dz);
    const Matrix plane{0x3f800000,0,Store(Negate(rx)),0,0x3f800000,Store(Negate(ry)),0,0,0};
    auto position=MultiplyVector(plane,scene.position);
    const Vector ground{Store(Multiply(Decode(Store(rx)),Decode(scene.ground))),
        Store(Multiply(Decode(Store(ry)),Decode(scene.ground))),scene.ground};
    position=AddVector(position,ground);
    for(size_t i=0;i<3;++i) {position[i]=Store(Subtract(Decode(position[i]),Decode(scene.auxiliary_position[i])));}
    const auto translation=MultiplyVector(scene.auxiliary,position);
    ShadowView result{MultiplyMatrix(MultiplyMatrix(scene.auxiliary,plane),scene.orientation),translation};
    const auto scale=Multiply(Decode(scene.object_scale),Decode(scene.auxiliary_scale));
    for(size_t axis=0;axis<2;++axis) {
        const auto scaled=Multiply(scale,Decode(scene.xy_scale[axis]));
        for(size_t column=0;column<3;++column) {const auto i=axis*3+column;
            result.projection[i]=Store(Multiply(Decode(result.projection[i]),scaled));}
        const auto center=Add(Integer(static_cast<int32_t>((axis?scene.height:scene.width)/2)),Decode(0x3f000000));
        auto value=Multiply(Decode(scene.offset[axis]),Decode(scene.xy_scale[axis]));
        value=Add(value,Decode(scene.pixel_offset[axis]));value=Add(value,center);
        value=Add(value,Decode(Store(Multiply(Decode(translation[axis]),scaled))));
        result.translation[axis]=Store(value);
    }
    return result;
}

}
