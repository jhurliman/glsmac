#include "TerrainRenderUpdate.h"

#include "types/texture/Texture.h"
#include "util/random/Random.h"
#include <stdexcept>

namespace game::backend::map {

void TerrainRenderUpdate::ApplyTexture( types::texture::Texture& atlas ) const {
	// Validate the whole payload before the first atlas write.
	for ( const auto& p : patches ) {
		if ( !p.pixels || !p.pixels->GetWidth() || !p.pixels->GetHeight() ||
			p.x > atlas.GetWidth() || p.y > atlas.GetHeight() ||
			p.pixels->GetWidth() > atlas.GetWidth() - p.x || p.pixels->GetHeight() > atlas.GetHeight() - p.y )
			throw std::runtime_error( "invalid terrain render patch" );
	}
	for ( const auto& p : patches ) atlas.AddFrom( p.pixels.get(), types::texture::AM_DEFAULT,
		0, 0, p.pixels->GetWidth() - 1, p.pixels->GetHeight() - 1, p.x, p.y );
}

void TerrainCoastRecipes::Record( Key destination, Paint paint ) {
	if ( !paint.pixels ) throw std::invalid_argument( "missing coast source" );
	if ( paint.land ) m_land[ *paint.land ] = paint.pixels;
	m_recipes[ destination ].push_back( std::move( paint ) );
    m_fungus.try_emplace(destination, std::nullopt);
}

void TerrainCoastRecipes::EnsureWater(Key destination, size_t width, size_t height) {
    if (m_recipes.count(destination)) return;
    if (!m_blank_water) m_blank_water=std::make_shared<types::texture::Texture>(width,height,types::texture::TF_CPU_ONLY);
    if (m_blank_water->GetWidth()!=width || m_blank_water->GetHeight()!=height) throw std::invalid_argument("water canvas dimensions differ");
    Record(destination,{m_blank_water,std::nullopt,types::texture::AM_DEFAULT,0,1.0f,{1,2,3,4},nullptr});
}

void TerrainCoastRecipes::RecordFungus(Key destination, Paint paint) {
    if (!paint.pixels || !m_recipes.count(destination)) throw std::invalid_argument("missing water base recipe");
    m_fungus.at(destination) = std::move(paint);
}

std::vector< TerrainRenderUpdate::Patch > TerrainCoastRecipes::Prepare( const Land& replacements, const Fungus& fungus ) const {
    for (const auto& [key, paint] : fungus) {
        if (!m_recipes.count(key) || (paint && (!paint->pixels || paint->land))) throw std::invalid_argument("invalid water fungus replacement");
    }
	std::vector< TerrainRenderUpdate::Patch > result;
	for ( const auto& [destination, recipe] : m_recipes ) {
		bool affected = fungus.count(destination) != 0;
		for ( const auto& p : recipe ) if ( p.land && replacements.count( *p.land ) ) affected = true;
		if ( !affected ) continue;
		const auto& first = *recipe.front().pixels;
		auto canvas = std::make_shared< types::texture::Texture >( first.GetWidth(), first.GetHeight(), types::texture::TF_CPU_ONLY );
		const auto apply = [&](const Paint& p) {
			const auto* source = p.pixels.get();
			if ( p.land ) {
				const auto changed = replacements.find( *p.land );
				source = changed != replacements.end() ? changed->second.get() : m_land.at( *p.land ).get();
			}
			if ( source->GetWidth() != canvas->GetWidth() || source->GetHeight() != canvas->GetHeight() )
				throw std::invalid_argument( "coast canvas dimensions differ" );
			util::random::Random random( 1 ); random.SetState( p.random );
			canvas->AddFrom( source, p.flags, 0, 0, source->GetWidth() - 1, source->GetHeight() - 1,
				0, 0, p.rotate, p.alpha, &random, p.perlin );
        };
        for (const auto& paint : recipe) apply(paint);
        const auto changed = fungus.find(destination);
        const auto& overlay = changed == fungus.end() ? m_fungus.at(destination) : changed->second;
        if (overlay) apply(*overlay);
		result.push_back( { destination.first, destination.second, std::move( canvas ) } );
	}
	return result;
}

void TerrainCoastRecipes::Commit( const Land& replacements, const Fungus& fungus ) noexcept {
	for ( const auto& [key, pixels] : replacements ) {
		const auto it = m_land.find( key );
		if ( it != m_land.end() ) it->second = pixels;
	}
    for (const auto& [key, paint] : fungus) {
        const auto found = m_fungus.find(key);
        if (found != m_fungus.end()) found->second = paint;
    }
}
void TerrainCoastRecipes::Clear() { m_recipes.clear(); m_land.clear(); m_fungus.clear(); m_blank_water.reset(); }

}
