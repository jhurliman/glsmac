#include "CvrScoutRenderer.h"

#include "types/texture/Texture.h"

#include <cstring>
#include <stdexcept>

namespace loader::cvr {

void CvrScoutTextureDelete::operator()( types::texture::Texture* texture ) const {
	if ( texture ) { DELETE( texture ); }
}

CvrScoutTexture CvrScoutImage::CreateTexture() const {
	if ( indexed.width != 256 || indexed.height != 256 || rgba.size() != 256 * 256 * 4 ) {
		throw std::invalid_argument( "CVR Scout: invalid owned texture extent" );
	}
	NEWV( texture, types::texture::Texture, "CVR Scout idle", 256, 256 );
	CvrScoutTexture result( texture );
	std::memcpy( result->GetBitmap(), rgba.data(), rgba.size() );
	result->FullUpdate();
	return result;
}

}
