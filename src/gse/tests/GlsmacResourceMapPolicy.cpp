#include "GlsmacResourceMapPolicy.h"

#include "game/backend/map/GlsmacResourceMapPolicy.h"
#include "game/backend/map/tile/Tile.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/map/SmacTerrainAuthority.h"

#include <functional>
#include <limits>

namespace gse::tests {
namespace {
using Policy = game::backend::map::GlsmacResourceMapPolicy;
using Band = game::backend::resource::smac::AltitudeBand;
bool Rejects( const std::function< void() >& operation ) { try { operation(); } catch ( const std::exception& ) { return true; } return false; }
}

void AddGlsmacResourceMapPolicyTests( task::gsetests::GSETests* task ) {

    task->AddTest("GLSMAC generated resource policy retains origin metadata opaque gameplay fields and bounded codec",GT(){
        using Authority=game::backend::map::SmacTerrainAuthority;namespace tile=game::backend::map::tile;
        Authority::Metadata meta;meta.width=16;meta.height=8;meta.horizontal_wrap=true;meta.pattern=11446;meta.sea_offset=-19;meta.climate_extra=-3;
        for(size_t i=0;i<7;++i) {meta.categories[i]=int32_t(i%3);}meta.generation_primary_rules=0x3a0e;
        std::string raw(16/2*8*44,0);
        for(size_t i=0;i<raw.size()/44;++i) {for(size_t k=0;k<44;++k) {raw[44*i+k]=char(uint8_t(i*17+k*11));}
            raw[44*i]=96;raw[44*i+3]=1;raw[44*i+5]=0;
        }
        // Explicit original forced energy and Jungle; legacy Tile fields below
        // intentionally disagree. Presence, visibility and opaque bytes stay raw.
        const auto word=[&](size_t off,uint32_t value){for(unsigned k=0;k<4;++k) {raw[off+k]=char(uint8_t(value>>(8*k)));}};
        word(8,0x20010403);word(12,4);raw[4]=char(255);
        auto policy=Policy::GeneratedTerrainV3(Authority::GeneratedRetained(meta,raw));const auto retained=policy.NativeTerrainSource();
        GT_ASSERT(!policy.OriginalPlanet() && !policy.OriginalPlanetSource() && retained && !retained->InstalledSource()
            && retained->SourceOrigin()==Authority::Origin::Generated && retained->RecordBytes()==raw,"Generated policy fabricated installed provenance or changed raw fields");
        const auto wire=policy.Serialize().ToString();const auto restored=Policy::Deserialize(wire);
        GT_ASSERT(restored==policy && restored.Serialize().ToString()==wire && restored.NativeTerrainSource()->RecordBytes()==raw
            && !(restored==Policy::DisplayedTerrainV1()) && !(Policy::DisplayedTerrainV1()==restored),"Generated policy codec or equality lost its source");
        const auto& info=restored.NativeTerrainSource()->Info();
        GT_ASSERT(info.width==16 && info.height==8 && info.horizontal_wrap && info.pattern==11446 && info.sea_offset==-19 && info.climate_extra==-3
            && info.categories==meta.categories && info.generation_primary_rules==meta.generation_primary_rules && restored.AutomaticLandResources(),"Generated policy lost producer metadata");
        tile::Tile cell;cell.coord={0,0};cell.bonus=tile::BONUS_MINERALS;cell.features=0;cell.is_water_tile=true;
        GT_ASSERT(restored.Altitude(cell)==Band::Land3 && restored.Bonus(cell)==game::backend::resource::smac::ResourceBonus::Energy
            && restored.LandmarkBonuses(cell)[0],"Generated resource queries fell back to displayed terrain");
        for(size_t end=0;end<wire.size();++end) {GT_ASSERT(Rejects([&]{Policy::Deserialize(wire.substr(0,end));}),"Truncated generated payload accepted");}
        GT_ASSERT(Rejects([&]{Policy::Deserialize(wire+"x");}),"Generated trailing fields accepted");
        auto changed=meta;changed.climate_extra=7;
        GT_ASSERT(!(Policy::GeneratedTerrainV3(Authority::GeneratedRetained(changed,raw))==policy),"Generated equality ignored metadata");
        changed=meta;changed.categories[3].reset();GT_ASSERT(Rejects([&]{Authority::GeneratedRetained(changed,raw);}),"Missing generated category accepted");
        changed=meta;changed.categories[3]=-1;GT_ASSERT(Rejects([&]{Authority::GeneratedRetained(changed,raw);}),"Unresolved generated category accepted");
        changed=meta;changed.generation_primary_rules.reset();GT_ASSERT(Rejects([&]{Authority::GeneratedRetained(changed,raw);}),"Missing generation rules accepted");
        changed=meta;changed.width=UINT32_MAX;GT_ASSERT(Rejects([&]{Authority::GeneratedRetained(changed,raw);}),"Wide geometry accepted");
        GT_ASSERT(Rejects([&]{Authority::GeneratedRetained(meta,raw.substr(1));}),"Mismatched raw record count accepted");
        const auto malformed=[&](int64_t width,int64_t version,int64_t kind,bool automatic){types::Buffer b;b.WriteInt(version);b.WriteInt(kind);b.WriteBool(automatic);
            b.WriteInt(width);b.WriteInt(meta.height);b.WriteBool(meta.horizontal_wrap);b.WriteInt(meta.pattern);b.WriteInt(meta.sea_offset);b.WriteInt(meta.climate_extra);
            for(const auto c:meta.categories) {b.WriteInt(c.value());}b.WriteInt(meta.generation_primary_rules.value());b.WriteString(raw);return b.ToString();};
        for(const auto width:{int64_t(-1),int64_t(17),int64_t(514),INT64_MAX}) {GT_ASSERT(Rejects([&]{Policy::Deserialize(malformed(width,3,3,true));}),"Invalid generated wire dimensions accepted");}
        GT_ASSERT(Rejects([&]{Policy::Deserialize(malformed(16,3,3,false));}) && Rejects([&]{Policy::Deserialize(malformed(16,3,2,true));})
            && Rejects([&]{Policy::Deserialize(malformed(16,1,3,true));}),"Contradictory generated options or origin tags accepted");
        types::Buffer nested;Policy::WriteOptional(nested,policy,true);nested.WriteInt(99);
        GT_ASSERT(Policy::ReadOptional(nested,true)==policy && nested.ReadInt()==99,"Generated nested codec consumed its following frame");
        policy=Policy::DisplayedTerrainV1();GT_ASSERT(retained->RecordBytes()==raw,"Retired generated policy invalidated queued raw ownership");
        changed=meta;changed.width=512;changed.height=256;std::string maximum(size_t(512/2)*256*44,0);
        const auto large=Policy::GeneratedTerrainV3(Authority::GeneratedRetained(changed,maximum));
        GT_ASSERT(Policy::Deserialize(large.Serialize().ToString())==large,"Maximum supported generated geometry did not roundtrip");GT_OK();
    });
	task->AddTest( "GLSMAC displayed resource policy preserves exact visible depth and land tiers", GT() {
		const auto policy = Policy::DisplayedTerrainV1();
		for ( const auto row : { std::pair< int64_t, Band >{ -3500, Band::Trench }, { -2001, Band::Trench }, { -2000, Band::DeepOcean },
			{ -1001, Band::DeepOcean }, { -1000, Band::Shelf }, { -1, Band::Shelf }, { 0, Band::Shelf }, { 100, Band::Shelf } } ) {
			GT_ASSERT( policy.Altitude( true, row.first ) == row.second, "depth boundary differs from UI strict comparisons" );
		}
		for ( const auto row : { std::pair< int64_t, Band >{ -100, Band::Land3 }, { 0, Band::Land3 }, { 999, Band::Land3 }, { 1000, Band::Land4 },
			{ 1999, Band::Land4 }, { 2000, Band::Land5 }, { 2999, Band::Land5 }, { 3000, Band::Land6 }, { 3500, Band::Land6 } } ) {
			GT_ASSERT( policy.Altitude( false, row.first ) == row.second, "declared land tier or mixed-sign coast changed" );
		}
		for ( const auto value : { INT64_MIN, int64_t( -3501 ), int64_t( 3501 ), INT64_MAX } ) {
			GT_ASSERT( Rejects( [ & ] { policy.Altitude( true, value ); } ) && Rejects( [ & ] { policy.Altitude( false, value ); } ), "unsupported elevation narrowed or clamped" );
		}

		// Use the real five-sample classifier, not just independently supplied
		// water booleans, for both signs of the coast disagreement.
		game::backend::map::tile::Tile coast;
		game::backend::map::tile::elevation_t center = 0, a = 100, b = 100, c = 100, d = -1000;
		coast.elevation.center = &center; coast.elevation.left = &a; coast.elevation.top = &b;
		coast.elevation.right = &c; coast.elevation.bottom = &d; coast.elevation.corners = { &a, &b, &c, &d };
		coast.Update();
		GT_ASSERT( !coast.is_water_tile && center < 0 && policy.Altitude( coast.is_water_tile, center ) == Band::Land3, "majority-land coast interpreted by center sign" );
		a = b = c = -100; d = 1000; coast.Update();
		GT_ASSERT( coast.is_water_tile && center > 0 && policy.Altitude( coast.is_water_tile, center ) == Band::Shelf, "majority-water coast interpreted by center sign" );
		GT_OK();
	} );
	task->AddTest( "GLSMAC displayed resource policy resolves visible bonus types without hidden pattern", GT() {
		using namespace game::backend;
		const auto policy = Policy::DisplayedTerrainV1(); map::tile::Tile tile; map::tile::elevation_t elevation = 0;
		tile.elevation.center = &elevation; tile.coord = { 2, 2 };
		for ( const auto row : { std::pair< uint8_t, resource::smac::ResourceBonus >{ map::tile::BONUS_NONE, resource::smac::ResourceBonus::None },
			{ map::tile::BONUS_NUTRIENT, resource::smac::ResourceBonus::Nutrients }, { map::tile::BONUS_MINERALS, resource::smac::ResourceBonus::Minerals }, { map::tile::BONUS_ENERGY, resource::smac::ResourceBonus::Energy } } ) {
			for ( const bool water : { false, true } ) { tile.is_water_tile = water; tile.bonus = row.first; GT_ASSERT( policy.Bonus( tile ) == row.second, "named bonus identities or forced override differ" ); }
		}
		elevation = -1001; tile.bonus = map::tile::BONUS_ENERGY;
		GT_ASSERT( Rejects( [ & ] { policy.Bonus( tile ); } ) && tile.bonus == map::tile::BONUS_ENERGY, "deep-water visible bonus was silently changed" );
		tile.bonus = 255; GT_ASSERT( Rejects( [ & ] { policy.Bonus( tile ); } ), "unknown bonus accepted" );
		tile.bonus = 0; tile.coord.x = 1; GT_ASSERT( Rejects( [ & ] { policy.Bonus( tile ); } ), "invalid parity accepted" );
		tile.coord.x = size_t( INT32_MAX ) + 1; GT_ASSERT( Rejects( [ & ] { policy.Bonus( tile ); } ), "wide coordinate narrowed" );
		GT_OK();
	} );
	task->AddTest( "GLSMAC map resource policy codec preserves absence and bounds before allocation", GT() {
		const auto policy = Policy::DisplayedTerrainV1(); const auto wire = policy.Serialize().ToString();
		GT_ASSERT( wire.size() == 28 && Policy::MAX_WIRE_BYTES == 4*1024*1024 && Policy::Deserialize( wire ) == policy, "policy framing differs" );
		for ( size_t end = 0; end < wire.size(); ++end ) { GT_ASSERT( Rejects( [ & ] { Policy::Deserialize( wire.substr( 0, end ) ); } ), "truncated policy accepted" ); }
		for ( int field = 0; field < 2; ++field ) { for ( const auto value : { int64_t( -1 ), int64_t( 0 ), int64_t( 2 ), INT64_MAX } ) {
			types::Buffer bad; bad.WriteInt( field == 0 ? value : 1 ); bad.WriteInt( field == 1 ? value : 1 );
			GT_ASSERT( Rejects( [ & ] { Policy::Deserialize( bad.ToString() ); } ), "unknown policy version or tag accepted" );
		} }
		for ( const bool smac : { false, true } ) {
			types::Buffer empty; Policy::WriteOptional( empty, {}, smac ); empty.WriteInt( 99 );
			GT_ASSERT( !Policy::ReadOptional( empty, smac ) && empty.ReadInt() == 99, "absent policy inferred or tail consumed" );
		}
		types::Buffer actual; Policy::WriteOptional( actual, policy, true ); actual.WriteInt( 99 );
		GT_ASSERT( Policy::ReadOptional( actual, true ) == policy && actual.ReadInt() == 99, "owned policy roundtrip consumed tail" );
		types::Buffer out; out.WriteInt( 99 ); const auto before = out.ToString();
		GT_ASSERT( Rejects( [ & ] { Policy::WriteOptional( out, policy, false ); } ) && out.ToString() == before, "SMAX writer appended unsupported policy" );

		for ( const bool checksum : { false, true } ) {
			auto bad = wire; if ( checksum ) { bad.back() ^= 1; } else { types::Buffer replacement; replacement.WriteInt( 3 ); bad.replace( 0, replacement.lenw, replacement.ToString() ); }
			types::Buffer input; input.WriteBool( true ); input.WriteString( bad ); const auto expected_cursor = input.lenw;
			std::string error; try { Policy::ReadOptional( input, true ); } catch ( const std::runtime_error& e ) { error = e.what(); }
			GT_ASSERT( input.lenr == expected_cursor && error.find( checksum ? "checksum mismatch" : "Unsupported GLSMAC resource map policy" ) != std::string::npos,
				"exact-cap nested error confused with outer budget rejection" );
		}
		types::Buffer unexpected; Policy::WriteOptional( unexpected, policy, true );
		types::Buffer presence; presence.WriteBool( true );
		GT_ASSERT( Rejects( [ & ] { Policy::ReadOptional( unexpected, false ); } ) && unexpected.lenr == presence.lenw, "SMAX policy accepted or copied before rejection" );
		for ( const auto count : { Policy::MAX_WIRE_BYTES+1, Policy::MAX_WIRE_BYTES*2 } ) {
			types::Buffer input; input.WriteBool( true ); const auto length_offset = input.lenw; input.WriteString( std::string( count, 'x' ) );
			GT_ASSERT( Rejects( [ & ] { Policy::ReadOptional( input, true ); } ) && input.lenr == length_offset, "oversized complete frame copied or advanced before budget rejection" );
		}
		GT_OK();
	} );
	for ( const auto scenario : { 0, 1, 2 } ) {
		task->AddTest( std::string( "GLSMAC map resource policy actual " ) + ( scenario == 0 ? "generator draw order" : scenario == 1 ? "snapshot continuation" : "generation publication cancellation" ), GT( scenario ) {
			return CheckResourceMapPolicyBackend( scenario );
		} );
	}
}

}
