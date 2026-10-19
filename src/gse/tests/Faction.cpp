#include "Faction.h"

#include "game/backend/faction/Faction.h"
#include "game/backend/faction/FactionManager.h"
#include "task/gsetests/GSETests.h"
#include "types/Buffer.h"
#include "loader/txt/FactionTXTLoader.h"

#include <limits>
#include <memory>
#include <optional>

namespace gse::tests {

using game::backend::faction::Faction;
using game::backend::faction::FactionManager;

static void Populate( Faction& faction, const std::string& id, const uint8_t flags ) {
	faction.m_id = id;
	faction.m_name = "Synthetic " + id;
	faction.m_display_identity = game::backend::faction::DisplayIdentity{"Retained " + id, "Director", "Leader " + id};
	faction.m_flags = flags;
	faction.m_base_names.land = { "Land One", "", "Land One", "Land Two" };
	faction.m_base_names.water = { "Water One", "Water Two" };
	faction.m_colors.text.Set( 0.25f, 0.5f, 0.75f, 1.0f );
	faction.m_colors.text_shadow.Set( 0.125f, 0.25f, 0.5f, 0.75f );
	faction.m_colors.border.Set( 1.0f, 0.75f, 0.5f, 0.25f );
	faction.m_bases_render = { "synthetic.pcx", 3, 7, 11, 13, 5, 6, 2, 1.25f, 0.75f };
}

static std::string Match( const Faction& actual, const Faction& expected ) {
	GT_ASSERT( actual.m_id == expected.m_id && actual.m_name == expected.m_name, "faction identity changed" );
	GT_ASSERT( actual.m_display_identity == expected.m_display_identity, "retained display identity changed" );
	GT_ASSERT( actual.m_flags == expected.m_flags, "faction flags were lost or changed" );
	GT_ASSERT( actual.m_base_names.land == expected.m_base_names.land, "land base names were lost or reordered" );
	GT_ASSERT( actual.m_base_names.water == expected.m_base_names.water, "water base names were lost or reordered" );
	GT_ASSERT( !( actual.m_colors.text != expected.m_colors.text ) && !( actual.m_colors.text_shadow != expected.m_colors.text_shadow ) && !( actual.m_colors.border != expected.m_colors.border ), "faction colors changed" );
	const auto& a = actual.m_bases_render;
	const auto& e = expected.m_bases_render;
	GT_ASSERT( a.file == e.file && a.grid_x == e.grid_x && a.grid_y == e.grid_y && a.cell_width == e.cell_width && a.cell_height == e.cell_height && a.cell_cx == e.cell_cx && a.cell_cy == e.cell_cy && a.cell_padding == e.cell_padding && a.scale_x == e.scale_x && a.scale_y == e.scale_y, "faction render fields changed" );
	GT_OK();
}

// Independent synthetic wire writer makes malformed scalar cases without
// corrupting Buffer checksums or depending on the production serializer.
static types::Buffer Record( const Faction& f, const std::string& marker = "GLSMAC_FACTION", const long long version = 2, const std::optional< long long > flags = {}, const std::optional< long long > land_count = {}, const std::optional< long long > water_count = {}, const std::optional< long long > grid_x = {} ) {
	types::Buffer buf;
	buf.WriteString( marker );
	buf.WriteInt( version );
	buf.WriteString( f.m_id );
	buf.WriteString( f.m_name );
	buf.WriteInt( flags.value_or( f.m_flags ) );
	buf.WriteInt( land_count.value_or( f.m_base_names.land.size() ) );
	for ( const auto& name : f.m_base_names.land ) { buf.WriteString( name ); }
	buf.WriteInt( water_count.value_or( f.m_base_names.water.size() ) );
	for ( const auto& name : f.m_base_names.water ) { buf.WriteString( name ); }
	buf.WriteColor( f.m_colors.text );
	buf.WriteColor( f.m_colors.text_shadow );
	buf.WriteColor( f.m_colors.border );
	buf.WriteString( f.m_bases_render.file );
	buf.WriteInt( grid_x.value_or( f.m_bases_render.grid_x ) );
	buf.WriteInt( f.m_bases_render.grid_y );
	buf.WriteInt( f.m_bases_render.cell_width );
	buf.WriteInt( f.m_bases_render.cell_height );
	buf.WriteInt( f.m_bases_render.cell_cx );
	buf.WriteInt( f.m_bases_render.cell_cy );
	buf.WriteInt( f.m_bases_render.cell_padding );
	buf.WriteFloat( f.m_bases_render.scale_x );
	buf.WriteFloat( f.m_bases_render.scale_y );
	if (version >= 2) {
		buf.WriteBool(f.m_display_identity.has_value());
		if (f.m_display_identity) {
			buf.WriteString(f.m_display_identity->faction_name);buf.WriteString(f.m_display_identity->leader_title);buf.WriteString(f.m_display_identity->leader_name);
		}
	}
	return buf;
}

static std::string RejectRecord( const std::string& bytes ) {
	Faction target, expected;
	Populate( target, "unchanged", Faction::FF_PROGENITOR );
	Populate( expected, "unchanged", Faction::FF_PROGENITOR );
	target.m_base_names.water.clear();
	expected.m_base_names.water.clear();
	bool rejected = false;
	try { target.Deserialize( bytes ); }
	catch ( const std::runtime_error& ) { rejected = true; }
	GT_ASSERT( rejected, "invalid faction record was accepted" );
	return Match( target, expected );
}

void AddFactionTests( task::gsetests::GSETests* task ) {
	task->AddTest("faction display identity parses source headers independently of controller names", GT() {
		using loader::txt::FactionTXTLoader;
		const std::string source="; synthetic records around installed University identity fields\n#UNIV\nUniversity of Planet, The Scientist, University, M, 1, Zakharov, M, 0, 0, 1, 0, 0,\n\nValues, Knowledge, RESEARCH\nPolitics, Fundamentalist, nil\nUniversity, Univ.\nAssistant, Surname, Base\nProvost, brilliant, unethical, talented, reckless\n#BASES\nOne\n";
		const auto identity=FactionTXTLoader::ParseDisplayIdentity(source,"UNIV");
		GT_ASSERT(identity.faction_name=="University of Planet" && identity.leader_title=="Provost" && identity.leader_name=="Zakharov","source display fields were confused with priorities or controller name");
		for(const auto& bad:{source+source, std::string("#UNIV\nIncomplete\n"),std::string("N\0UL",4)+source}) {
			bool rejected=false;try{FactionTXTLoader::ParseDisplayIdentity(bad,"UNIV");}catch(const std::exception&){rejected=true;}
			GT_ASSERT(rejected,"malformed display identity accepted");
		}
		GT_OK();
	});
	task->AddTest("faction snapshot v1 preserves absent display identity and v2 retains explicit metadata", GT() {
		Faction source,restored;Populate(source,"source",Faction::FF_NONE);Populate(restored,"destination",Faction::FF_NONE);
		restored.Deserialize(Record(source,"GLSMAC_FACTION",1));
		GT_ASSERT(!restored.m_display_identity,"old save acquired current installed leader metadata");
		source.m_display_identity.reset();const auto absent=Match(restored,source);GT_ASSERT(absent.empty(),"v1 changed existing faction fields");
		source.m_display_identity=game::backend::faction::DisplayIdentity{"Custom Faction","Custom Title","Custom Leader"};
		restored.Deserialize(source.Serialize());GT_ASSERT(restored.m_display_identity==source.m_display_identity,"custom retained identity was replaced");
		const auto before=restored.Serialize().ToString();Faction malformed;Populate(malformed,"source",Faction::FF_NONE);malformed.m_display_identity->leader_name="";
		bool rejected=false;try{restored.Deserialize(Record(malformed));}catch(const std::runtime_error&){rejected=true;}
		GT_ASSERT(rejected && restored.Serialize().ToString()==before,"bad metadata partially replaced destination");GT_OK();
	});
	task->AddTest( "faction snapshot preserves flags, ordered names and existing fields", GT() {
		for ( uint8_t flags = 0 ; flags <= ( Faction::FF_NAVAL | Faction::FF_PROGENITOR ) ; ++flags ) {
			for ( unsigned empty_lists = 0 ; empty_lists < 4 ; ++empty_lists ) {
				Faction source, restored;
				Populate( source, "source", flags );
				Populate( restored, "old", Faction::FF_NAVAL | Faction::FF_PROGENITOR );
				if ( empty_lists & 1 ) { source.m_base_names.land.clear(); }
				if ( empty_lists & 2 ) { source.m_base_names.water.clear(); }
				restored.Deserialize( source.Serialize() );
				const auto error = Match( restored, source );
				GT_ASSERT( error.empty(), "faction mismatch: " + error );
				GT_ASSERT( source.Serialize().ToString() == Record( source ).ToString(), "faction wire record omitted or reordered fields" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "faction snapshot rejects malformed fields without changing the destination", GT() {
		Faction source;
		Populate( source, "source", Faction::FF_NAVAL );
		std::vector< std::string > bad;
		bad.push_back( Record( source, "unknown" ).ToString() );
		for ( const auto version : { -1LL, 0LL, 3LL } ) { bad.push_back( Record( source, "GLSMAC_FACTION", version ).ToString() ); }
		for ( const auto flags : { -1LL, 4LL, 255LL, 256LL } ) { bad.push_back( Record( source, "GLSMAC_FACTION", 1, flags ).ToString() ); }
		for ( const auto count : { -1LL, std::numeric_limits< long long >::max() } ) {
			bad.push_back( Record( source, "GLSMAC_FACTION", 1, {}, count ).ToString() );
			bad.push_back( Record( source, "GLSMAC_FACTION", 1, {}, {}, count ).ToString() );
		}
		bad.push_back( Record( source, "GLSMAC_FACTION", 1, {}, {}, {}, -1 ).ToString() );
		// A valid legacy prefix is not a supported versioned record.
		types::Buffer legacy;
		legacy.WriteString( source.m_id );
		legacy.WriteString( source.m_name );
		bad.push_back( legacy.ToString() );
		const auto valid = Record( source ).ToString();
		bad.push_back( valid + "trailing" );
		for ( size_t size = 0 ; size < valid.size() ; ++size ) { bad.push_back( valid.substr( 0, size ) ); }
		for ( const auto& bytes : bad ) {
			const auto error = RejectRecord( bytes );
			GT_ASSERT( error.empty(), "faction mismatch: " + error );
		}
		GT_OK();
	} );
	task->AddTest( "faction manager snapshot restores complete records and ordering", GT() {
		FactionManager source, restored;
		for ( const auto& entry : { std::pair< const char*, uint8_t >{ "zeta", Faction::FF_NAVAL }, { "alpha", Faction::FF_PROGENITOR }, { "beta", Faction::FF_NAVAL | Faction::FF_PROGENITOR }, { "none", Faction::FF_NONE } } ) {
			auto faction = std::make_unique< Faction >();
			Populate( *faction, entry.first, entry.second );
			if ( entry.second == Faction::FF_NONE ) { faction->m_base_names = {}; }
			source.Add( faction.release() );
		}
		restored.Add( new Faction( "old", "replaced" ) );
		restored.Deserialize( source.Serialize() );
		GT_ASSERT( !restored.Get( "old" ), "old faction was not replaced" );
		const auto before = source.GetAll(), after = restored.GetAll();
		GT_ASSERT( before.size() == after.size(), "faction count changed" );
		for ( size_t i = 0 ; i < before.size() ; ++i ) {
			GT_ASSERT( after[ i ] != before[ i ] && restored.Get( before[ i ]->m_id ) == after[ i ], "manager did not restore destination ownership/lookup" );
			const auto error = Match( *after[ i ], *before[ i ] );
			GT_ASSERT( error.empty(), "faction mismatch: " + error );
		}
		FactionManager empty;
		restored.Deserialize( empty.Serialize() );
		GT_ASSERT( restored.GetAll().empty(), "empty manager snapshot left old factions" );
		GT_OK();
	} );
	task->AddTest( "faction manager rejects invalid counts and records before publishing each faction", GT() {
		Faction source;
		Populate( source, "source", Faction::FF_NAVAL );
		std::vector< types::Buffer > bad;
		for ( const auto count : { -1LL, std::numeric_limits< long long >::max() } ) {
			types::Buffer buf; buf.WriteInt( count ); bad.push_back( buf );
		}
		for ( const auto& key : { "source", "wrong-id" } ) {
			types::Buffer buf; buf.WriteInt( 1 ); buf.WriteString( key );
			buf.WriteString( std::string( key ) == "source" ? Record( source ).ToString() + "trailing" : Record( source ).ToString() );
			bad.push_back( buf );
		}
		types::Buffer valid;
		valid.WriteInt( 1 ); valid.WriteString( "source" ); valid.WriteString( Record( source ).ToString() );
		const auto bytes = valid.ToString();
		for ( size_t size = 0 ; size < bytes.size() ; ++size ) { bad.emplace_back( bytes.substr( 0, size ) ); }
		for ( const auto& buf : bad ) {
			FactionManager manager;
			bool rejected = false;
			try { manager.Deserialize( buf ); }
			catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected, "invalid manager snapshot was accepted" );
			GT_ASSERT( !manager.Get( "source" ) && !manager.Get( "wrong-id" ) && manager.GetAll().empty(), "failed record was published in the manager" );
		}
		types::Buffer duplicate;
		duplicate.WriteInt( 2 );
		for ( unsigned i = 0 ; i < 2 ; ++i ) { duplicate.WriteString( "source" ); duplicate.WriteString( Record( source ).ToString() ); }
		FactionManager manager;
		bool rejected = false;
		try { manager.Deserialize( duplicate ); }
		catch ( const std::runtime_error& ) { rejected = true; }
		GT_ASSERT( rejected, "duplicate faction ID was accepted" );
		GT_ASSERT( manager.GetAll().size() == 1 && manager.Get( "source" ), "record-local publication did not preserve the prior complete record" );
		const auto error = Match( *manager.Get( "source" ), source );
		GT_ASSERT( error.empty(), "faction mismatch: " + error );
		types::Buffer trailing; trailing.WriteInt( 0 ); trailing.WriteInt( 7 );
		rejected = false;
		try { manager.Deserialize( trailing ); }
		catch ( const std::runtime_error& ) { rejected = true; }
		GT_ASSERT( rejected, "trailing manager data was accepted" );
		GT_OK();
	} );
	task->AddTest( "faction writer rejects unsupported flags and unrepresentable render sizes", GT() {
		Faction source;
		Populate( source, "source", 4 );
		bool rejected = false;
		try { source.Serialize(); }
		catch ( const std::runtime_error& ) { rejected = true; }
		GT_ASSERT( rejected, "writer accepted unknown faction flags" );
		if ( std::numeric_limits< size_t >::max() > static_cast< unsigned long long >( std::numeric_limits< long long >::max() ) ) {
			source.m_flags = Faction::FF_NONE;
			source.m_bases_render.grid_x = std::numeric_limits< size_t >::max();
			rejected = false;
			try { source.Serialize(); }
			catch ( const std::runtime_error& ) { rejected = true; }
			GT_ASSERT( rejected, "writer narrowed an unrepresentable render size" );
		}
		GT_OK();
	} );
}

}
