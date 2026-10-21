#include "SmacStartupDesignState.h"
#include "game/backend/unit/SmacStartupDesignState.h"
#include "fixtures/SmacStartupSource.h"
#include "fixtures/SmacRetirementHistory.h"
#include "fixtures/SmacNativeRegistry.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include "task/gsetests/GSETests.h"

#include <functional>
#include <utility>

namespace gse::tests {
namespace {
using namespace game::backend;
using State = unit::SmacStartupDesignState;
bool Rejects( const std::function< void() >& fn ) { try { fn(); } catch ( const std::exception& ) { return true; } return false; }
faction::SmacTechnologyState Technologies( const std::vector< faction::SmacSimulationAdmission >& roster ) {
	return faction::SmacTechnologyState::NewWorld( roster, smac_rules_fixture::Technologies(), smac_startup_fixture::Factions() );
}
State Saved() {
	auto result = State::NewWorld( Technologies( { { 7, "MORGANITES" }, { 2, "SPARTANS" } } ), smac_rules_fixture::Technologies(), smac_startup_fixture::Units(), 5 );
    result.retirement_history={}; // explicit V1 fixture has no retained history authority
	auto& d = result.owners[ 0 ].designs[ 0 ]; d.active = false; d.prototyped = false; d.cost = 255; d.plan = 255; d.carry = 254; d.abilities = 0xffffff; d.faction_exclusion = UINT32_MAX;
	result.owners[ 0 ].starting_scout = d.id;
	result.owners[ 1 ].designs[ 0 ].cost = 0;
	return result;
}
// Deliberately bypass typed validation to distinguish every framed field.
std::string Wire( const State& value, size_t index = SIZE_MAX, int64_t replacement = 0 ) {
	types::Buffer b; size_t n = 0; const auto write = [ & ]( int64_t v ) { b.WriteInt( n++ == index ? replacement : v ); };
	write( 0x534d5344 ); write( 1 ); write( value.global_difficulty ); write( value.owners.size() );
	for ( const auto& owner : value.owners ) {
		write( owner.simulation_id ); write( owner.starting_scout ); write( owner.designs.size() );
		for ( const auto& d : owner.designs ) {
			write( d.id ); write( d.chassis ); write( d.weapon ); write( d.defense ); write( d.reactor ); write( d.cost ); write( d.plan ); write( d.carry );
			write( d.abilities ); write( d.faction_exclusion ); b.WriteBool( d.active ); b.WriteBool( d.prototyped );
		}
	}
	return b.ToString();
}
}

void AddSmacStartupDesignStateTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC native registry matches twenty-eight complete original startup pools",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto read=[&](const char* name) {std::ifstream file(std::string(assets)+"/"+name,std::ios::binary);if(!file) {throw std::runtime_error("Missing owned registry source");}return std::string(std::istreambuf_iterator<char>(file),{});};
        const auto source=read("alpha.txt");const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto rules=settings::SmacPredefinedUnitRules::Parse(source,tech);rules.catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(read("labels.txt"));
        const std::array<const char*,7> ids{"GAIANS","HIVE","UNIVERSITY","MORGANITES","SPARTANS","BELIEVERS","PEACEKEEPERS"};
        const std::array<int,7> grants{6,43,2,1,5,4,0};
        for(const auto& test:native_registry_fixture::CASES) {
            faction::SmacTechnologyState ownership;faction::SmacFactionTechnologyState owner;
            owner.admission={test.owner,ids.at(test.owner-1)};owner.owned.at(grants.at(test.owner-1))=true;ownership.factions.push_back(owner);
            const auto state=State::NewWorld(ownership,tech,rules,test.difficulty);
            GT_ASSERT(state.native_registry && state.Find(test.owner)->starting_scout==test.scout,"Startup omitted native pool or changed Scout selection");
            uint64_t hash=UINT64_C(14695981039346656037);for(const auto& row:*state.native_registry) {for(const auto byte:row) {hash=(hash^byte)*UINT64_C(1099511628211);}}
            GT_ASSERT(hash==test.pool,"Whole native startup registry differs: owner="+std::to_string(test.owner)+" difficulty="+std::to_string(test.difficulty)+" hash="+std::to_string(hash));
            const auto wire=state.Serialize(rules.catalog).ToString();GT_ASSERT(State::Deserialize(wire,rules.catalog)==state,"Native startup pool or typed views changed on restore");
        }GT_OK();
    });
    task->AddTest("SMAC native registry preserves inactive slots tails and opaque fields through typed edits",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;State state;state.owners={{1,{},2}};
        unit::SmacOriginalDesignPool pool{};
        for(size_t id=0;id<512;++id) {for(size_t i=0;i<52;++i) {pool[id][i]=uint8_t(id*31+i*17);}pool[id][48]&=uint8_t(~1u);}
        auto& row=pool[65];std::fill_n(row.begin(),32,0);const std::string name="Retained";std::copy(name.begin(),name.end(),row.begin());row[31]=0xa5;
        std::fill(row.begin()+32,row.begin()+43,0);row[39]=1;row[41]=1;row[42]=3;row[44]=0;row[48]=5;row[49]=0;
        row[43]=0x31;row[45]=0x20;row[46]=0xff;row[47]=0xa5;row[50]=0x34;row[51]=0x12;
        state.RetainNativeRegistry(pool,catalog);const auto original=state;
        const auto wire=state.Serialize(catalog).ToString();types::Buffer header(wire);header.ReadInt();GT_ASSERT(header.ReadInt()==7,"Retained registry omitted V7 framing");
        GT_ASSERT(State::Deserialize(wire,catalog)==state,"Inactive/global record bytes changed on restore");
        state.owners[0].designs[0].cost=7;
        GT_ASSERT(Rejects([&]{state.Validate(catalog);}),"Stale typed/record mirror passed validation");state.SynchronizeNativeRegistry();state.Validate(catalog);
        auto expected=pool;expected[65][41]=7;
        GT_ASSERT(state.native_registry==std::optional<unit::SmacOriginalDesignPool>(expected),"Typed edit touched name tail, inactive slot or unmodeled field");
        auto bad=pool;bad[65][37]=255;
        GT_ASSERT(Rejects([&]{state.RetainNativeRegistry(bad,catalog);}) && state.native_registry==std::optional<unit::SmacOriginalDesignPool>(expected),"Invalid imported record partially replaced authority");
        for(const auto& badwire:{wire.substr(0,wire.size()-1),wire+"x"}) {GT_ASSERT(Rejects([&]{State::Deserialize(badwire,catalog);}),"Malformed native pool frame accepted");}
        auto missing=original;missing.intrinsic_overrides[2].reset();GT_ASSERT(Rejects([&]{missing.Validate(catalog);}),"Missing intrinsic mirror silently used frozen definitions");GT_OK();
    });
    task->AddTest("SMAC intrinsic overlays preserve V6 words bytes and legacy catalog authority",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;auto state=Saved();const auto legacy=state.Serialize(catalog).ToString();
        GT_ASSERT(State::Deserialize(legacy,catalog)==state && State::Deserialize(legacy,catalog).Serialize(catalog).ToString()==legacy,"legacy design wire changed without overlays");
        for(size_t i=0;i<64;++i) {state.intrinsic_overrides[i]=unit::SmacIntrinsicDesignState{uint16_t(i*1021),uint8_t(i*37)};}
        state.intrinsic_overrides[2]=unit::SmacIntrinsicDesignState{0x205,0x86};const auto snapshot=state;
        const auto current=state.EffectiveIntrinsic(catalog.units.at(2));
        GT_ASSERT(current.active && current.prototyped && current.faction_exclusion==0x86 && catalog.units.at(2).faction_exclusion==0 && state==snapshot,"effective view changed catalog or overlay");
        const auto wire=state.Serialize(catalog).ToString();GT_ASSERT(State::Deserialize(wire,catalog)==state && State::Deserialize(wire,catalog).Serialize(catalog).ToString()==wire,"complete intrinsic overlay changed on restore");
        for(const auto& bad:{wire.substr(0,wire.size()-1),wire+"x"}) {GT_ASSERT(Rejects([&]{State::Deserialize(bad,catalog);}),"malformed V6 frame accepted");}
        auto v5=State::NewWorld(Technologies({{1,"GAIANS"}}),smac_rules_fixture::Technologies(),smac_startup_fixture::Units(),3).Serialize(catalog).ToString();
        types::Buffer read(v5);read.ReadInt();GT_ASSERT(read.ReadInt()==5,"V5 boundary fixture changed");types::Buffer header;header.WriteInt(0x534d5344);header.WriteInt(6);
        const auto candidate=[&](int64_t flags,int64_t exclusion) {types::Buffer tail;
            for(size_t i=0;i<64;++i) {tail.WriteBool(i==2);if(i==2) {tail.WriteInt(flags);tail.WriteInt(exclusion);}}return header.ToString()+v5.substr(read.lenr)+tail.ToString();};
        GT_ASSERT(State::Deserialize(candidate(65535,255),catalog).intrinsic_overrides[2]->flags==65535,"valid full-width intrinsic state rejected");
        for(const auto fields:{std::pair<int64_t,int64_t>{-1,0},{65536,0},{0,-1},{0,256}}) {GT_ASSERT(Rejects([&]{State::Deserialize(candidate(fields.first,fields.second),catalog);}),"overflowed intrinsic fields accepted");}GT_OK();
    });
    task->AddTest("SMAC retirement history matches eight original complete row and cursor controls",GT(){
        for(const auto& row:retirement_history_fixture::FRAMES) {
            std::array<unit::SmacRetirementHistory,8> histories;std::array<uint8_t,4096> input;
            for(size_t i=0;i<input.size();++i) {input[i]=uint8_t(row.start+row.step*i);}
            for(const auto& patch:row.patches) {input.at(patch.first)=patch.second;}
            for(size_t owner=0;owner<8;++owner) {histories[owner].cursor=row.before_cursor[owner];
                for(size_t i=0;i<512;++i) {histories[owner].slots[i/8][i%8]=input[owner*512+i];}}
            if(row.kind==0) {histories.at(row.owner).Clear();}
            else if(row.kind==1) {histories.at(row.owner).Record(row.tuple);}
            else {GT_ASSERT(histories.at(row.owner).Contains(row.tuple,row.human)==row.found,"native complete history lookup differs");}
            uint64_t digest=UINT64_C(14695981039346656037);
            for(size_t owner=0;owner<8;++owner) {GT_ASSERT(histories[owner].cursor==row.after_cursor[owner],"native retirement cursor changed");
                for(const auto& slot:histories[owner].slots) {for(const auto byte:slot) {digest=(digest^byte)*UINT64_C(1099511628211);}}}
            GT_ASSERT(digest==row.after,"complete 4096-byte native history differs");
        }GT_OK();
    });
    task->AddTest("SMAC retirement history startup codec preserves all owners and absent legacy authority",GT(){
        const auto units=smac_startup_fixture::Units();const auto tech=smac_rules_fixture::Technologies();
        auto state=State::NewWorld(Technologies({{1,"GAIANS"},{7,"SPARTANS"}}),tech,units,3);
        GT_ASSERT(state.retirement_history[1] && state.retirement_history[7] && !state.retirement_history[0] && !state.retirement_history[2],"startup invented inactive history authority");
        for(const auto owner:{1,7}) {GT_ASSERT(state.retirement_history[owner]->cursor==0,"startup failed to reset cursor");
            for(const auto& slot:state.retirement_history[owner]->slots) {for(const auto byte:slot) {GT_ASSERT(byte==255,"startup history is not FF");}}}
        for(size_t owner=0;owner<8;++owner) {auto history=unit::SmacRetirementHistory::NewWorld();history.cursor=int32_t(owner*9);
            for(size_t i=0;i<512;++i) {history.slots[i/8][i%8]=uint8_t(i*37+owner*19);}state.retirement_history[owner]=history;}
        const auto wire=state.Serialize(units.catalog).ToString();
        GT_ASSERT(State::Deserialize(wire,units.catalog)==state && State::Deserialize(wire,units.catalog).Serialize(units.catalog).ToString()==wire,"history/cursors changed during restore");
        for(const int cursor:{-1,64,INT32_MAX}) {auto bad=state;bad.retirement_history[7]->cursor=cursor;
            GT_ASSERT(Rejects([&]{bad.Serialize(units.catalog);}) && Rejects([&]{bad.retirement_history[7]->Record({});}),"invalid native cursor accepted");}
        auto legacy=state;legacy.retirement_history={};const auto old=legacy.Serialize(units.catalog).ToString();
        GT_ASSERT(State::Deserialize(old,units.catalog)==legacy && State::Deserialize(old,units.catalog).Serialize(units.catalog).ToString()==old,"legacy load invented or reset retirement history");
        // V4's design layout is unchanged in V5; append independently framed
        // history fields to exercise reader admission separately from writer.
        legacy.owners.back().designs.front().auxiliary_flags=8;const auto v4=legacy.Serialize(units.catalog).ToString();types::Buffer input(v4);
        input.ReadInt();GT_ASSERT(input.ReadInt()==4,"legacy control did not select V4");types::Buffer header;header.WriteInt(0x534d5344);header.WriteInt(5);
        const auto candidate=[&](int64_t cursor,size_t extent) {types::Buffer tail;
            for(int owner=0;owner<8;++owner) {tail.WriteBool(owner==7);if(owner==7) {tail.WriteInt(cursor);tail.WriteString(std::string(extent,'x'));}}
            return header.ToString()+v4.substr(input.lenr)+tail.ToString();};
        GT_ASSERT(State::Deserialize(candidate(63,512),units.catalog).retirement_history[7]->cursor==63,"valid boundary history rejected");
        for(const int64_t cursor:{int64_t(-1),int64_t(64),INT64_MAX}) {GT_ASSERT(Rejects([&]{State::Deserialize(candidate(cursor,512),units.catalog);}),"invalid history cursor decoded");}
        for(const size_t extent:{0,511,513}) {GT_ASSERT(Rejects([&]{State::Deserialize(candidate(0,extent),units.catalog);}),"invalid history extent decoded");}GT_OK();
    });
    task->AddTest("SMAC full custom banks and retained history fit the bounded design codec",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;State full;full.global_difficulty=3;
        for(int owner=1;owner<8;++owner) {unit::SmacDesignOwner bank;bank.simulation_id=owner;
            for(int index=0;index<64;++index) {unit::SmacCustomDesign row;row.id=64*owner+index;row.name=std::string(31,'X');
                row.explicitly_named=true;row.auxiliary_flags=0xfff8;row.active=index%2!=0;bank.designs.push_back(row);}full.owners.push_back(bank);}
        for(auto& row:full.retirement_history) {row=unit::SmacRetirementHistory::NewWorld();}
        const auto wire=full.Serialize(catalog).ToString();
        GT_ASSERT(wire.size()>16*1024 && wire.size()<State::MAX_WIRE_BYTES && State::Deserialize(wire,catalog)==full,"full custom banks exceed budget or lose records");
        full.native_registry.emplace();for(auto& row:full.intrinsic_overrides) {row=unit::SmacIntrinsicDesignState{};}full.SynchronizeNativeRegistry();
        const auto native=full.Serialize(catalog).ToString();
        GT_ASSERT(native.size()>wire.size()+512*52 && native.size()<State::MAX_WIRE_BYTES && State::Deserialize(native,catalog)==full,
            "Full V7 registry, maximum custom banks and history exceed budget or lose data");GT_OK();
    });
    task->AddTest("SMAC human riot prototype honors retained retired tuples without changing design authority",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;
        auto state=State::NewWorld(Technologies({{1,"GAIANS"},{2,"SPARTANS"}}),smac_rules_fixture::Technologies(),smac_startup_fixture::Units(),5);
        const auto tuple=unit::SmacDesignRetirementTuple(state.Find(2)->designs.front());
        state.retirement_history[1]->slots[3]=tuple;state.retirement_history[1]->cursor=42;
        const auto before=state.Serialize(catalog).ToString();
        GT_ASSERT(state.RequestRiotPrototype(128,1,false,catalog,true)==-1 && state.Serialize(catalog).ToString()==before,"retired human tuple was recreated or changed state");
        auto computer=State::NewWorld(Technologies({{1,"SPARTANS"},{2,"SPARTANS"}}),smac_rules_fixture::Technologies(),smac_startup_fixture::Units(),5);
        computer.retirement_history[1]->slots[3]=tuple;computer.retirement_history[1]->cursor=42;
        const auto ai_before=computer.Serialize(catalog).ToString();
        GT_ASSERT(computer.RequestRiotPrototype(128,1,false,catalog,false)==64 && computer.Serialize(catalog).ToString()==ai_before,"AI existing design was blocked by human retirement history");
        computer.retirement_history[1].reset();
        GT_ASSERT(computer.RequestRiotPrototype(128,1,false,catalog,false)==64,"AI reuse required human history authority");
        state.retirement_history[1].reset();const auto legacy=state.Serialize(catalog).ToString();
        GT_ASSERT(Rejects([&]{state.RequestRiotPrototype(128,1,false,catalog,true);}) && state.Serialize(catalog).ToString()==legacy,"missing history became an empty history");GT_OK();
    });
    task->AddTest("SMAC human riot Morgan armor prototypes retain their distinct measured payload",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;
        const std::array<uint8_t,5> expected_aux{2,0,2,255,0};
        for(const auto* recipient:{"GAIANS","MORGANITES","SPARTANS"}) {
            auto state=State::NewWorld(Technologies({{1,recipient},{2,"MORGANITES"}}),smac_rules_fixture::Technologies(),smac_startup_fixture::Units(),5);
            const auto source=state.Find(2)->designs[0];const auto before=state.Find(1)->designs;
            const bool exact=std::string(recipient)=="MORGANITES";const int32_t expected=exact ? -1:std::string(recipient)=="SPARTANS" ? 65:64;
            GT_ASSERT(state.Find(2)->starting_scout==2,"Armored custom source was incorrectly treated as the Morgan starting Unit");
            GT_ASSERT(state.RequestRiotPrototype(128,1,exact,catalog,true)==expected && state.Find(2)->designs[0]==source,
                "Morgan source allocation branch or retained source changed");
            if(exact) {GT_ASSERT(state.Find(1)->designs==before,"Negative human match modified recipient block");}
            else {
                const auto& added=state.Find(1)->designs.back();GT_ASSERT(added.id==expected && added.chassis==0 && added.weapon==0 && added.defense==1
                    && added.reactor==1 && added.cost==2 && added.plan==2 && added.carry==0 && added.riot
                    && added.riot->source_design==128 && added.riot->flags==197 && added.riot->prerequisite==0
                    && added.riot->auxiliary==expected_aux && !added.name,
                    "Morgan allocated payload borrowed Rover bytes or invented its unknown name");
                if(!before.empty()) {GT_ASSERT(state.Find(1)->designs[0]==before[0],"Morgan allocation overwrote existing Spartan64");}
            }
            GT_ASSERT(State::Deserialize(state.Serialize(catalog).ToString(),catalog)==state,"Morgan prototype metadata did not survive codec");
        }GT_OK();
    });

    task->AddTest("SMAC human riot Rover prototype retains measured result fields and legacy unknown names",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;
        const std::array<uint8_t,5> expected_aux{10,0,2,255,0};
        const auto create=[&](const char* recipient) {return State::NewWorld(Technologies({{1,recipient},{2,"SPARTANS"}}),smac_rules_fixture::Technologies(),smac_startup_fixture::Units(),5);};
        auto empty=create("GAIANS");const auto source=empty.owners[1].designs[0];
        GT_ASSERT(empty.RequestRiotPrototype(128,1,false,catalog,true)==64,"Empty human block did not allocate measured global64");
        const auto& allocated=empty.Find(1)->designs[0];
        GT_ASSERT(allocated.riot && allocated.riot->source_design==128 && allocated.riot->flags==197 && allocated.riot->prerequisite==0
            && allocated.riot->auxiliary==expected_aux && !allocated.name && empty.Find(2)->designs[0]==source,
            "Allocated definition dropped known result bytes, invented a name or changed the source");
        const auto retained=empty;
        GT_ASSERT(empty.RequestRiotPrototype(128,1,false,catalog,true)==-1 && empty==retained,"Exact human prototype was incorrectly reused or changed");
        auto morgan=create("MORGANITES");morgan.owners[0].designs[0].name="Startup design";morgan.owners[1].designs[0].name="Startup design";
        const auto old_morgan=morgan.owners[0].designs[0],old_source=morgan.owners[1].designs[0];
        GT_ASSERT(morgan.RequestRiotPrototype(128,1,true,catalog,true)==65 && morgan.Find(1)->designs[0]==old_morgan
            && morgan.Find(2)->designs[0]==old_source && morgan.Find(1)->designs[1].name==std::optional<std::string>("Startup design*"),
            "Measured Morgan block allocation/name collision changed retained definitions");
        for(const auto& state:{empty,morgan}) {
            const auto wire=state.Serialize(catalog).ToString();GT_ASSERT(State::Deserialize(wire,catalog)==state,"Known prototype fields were lost on decode");
            for(size_t n=0;n<wire.size();++n) {GT_ASSERT(Rejects([&]{State::Deserialize(wire.substr(0,n),catalog);}),"Truncated prototype metadata accepted");}
            GT_ASSERT(Rejects([&]{State::Deserialize(wire+"x",catalog);}),"Trailing prototype metadata accepted");
        }
        auto legacy=create("GAIANS");legacy.retirement_history={};GT_ASSERT(legacy.Serialize(catalog).ToString()==Wire(legacy) && State::Deserialize(Wire(legacy),catalog)==legacy,
            "Legacy design frame changed or gained invented metadata");
        auto bad=empty;bad.owners[0].designs[0].riot->source_design=64;
        GT_ASSERT(Rejects([&]{bad.Serialize(catalog);}),"Cyclic prototype source accepted");GT_OK();
    });
	task->AddTest( "SMAC startup designs separate generation from marking for explicit owners and difficulty", GT() {
		const auto units = smac_startup_fixture::Units(); const auto technologies = smac_rules_fixture::Technologies();
		const char* names[] = { "Citizen", "Specialist", "Talent", "Librarian", "Thinker", "Transcend" };
		for ( int32_t difficulty = 0; difficulty < 6; ++difficulty ) {
			GT_ASSERT( State::OriginalDifficulty( names[ difficulty ] ) == difficulty, "canonical difficulty mapping changed" );
			for ( int32_t id = 1; id <= 7; ++id ) {
				for ( const auto& binding : settings::SmacFactionStartupRules::Bindings() ) {
					const auto tech = Technologies( { { id, binding.port_id } } );
					auto owner = State::GenerateEmptyOwner( tech.factions[ 0 ], technologies, units.catalog, difficulty );
					const bool morgan = std::string( binding.port_id ) == "MORGANITES", sparta = std::string( binding.port_id ) == "SPARTANS";
					GT_ASSERT( owner.simulation_id == id && owner.starting_scout == 2 && owner.designs.size() == ( morgan || sparta ? 1u : 0u ), "generation inferred a slot or selected before marking" );
					if ( !owner.designs.empty() ) {
						const auto& d = owner.designs[ 0 ];
						GT_ASSERT( d.id == 64 * id && d.chassis == ( sparta ? 1u : 0u ) && d.weapon == 0 && d.defense == ( morgan ? 1u : 0u )
							&& d.reactor == 1 && !d.abilities && d.cost == 2 && d.plan == ( sparta ? 3 : 2 ) && !d.carry && d.active
							&& d.prototyped == ( difficulty <= 1 ) && !d.faction_exclusion, "generated component/cache or premark state differs" );
					}
					State::MarkAndSelect( owner, units.catalog );
					GT_ASSERT( owner.starting_scout == ( sparta ? 64 * id : 2 ) && ( owner.designs.empty() || owner.designs[ 0 ].prototyped ), "startup marking/selection differs" );
				}
			}
		}
		for ( const auto* name : { "Captain", "citizen", "", "7" } ) { GT_ASSERT( Rejects( [ & ]() { State::OriginalDifficulty( name ); } ), "unknown difficulty identity accepted" ); }
		for ( const auto difficulty : { INT32_MIN, -1, 6, INT32_MAX } ) { GT_ASSERT( Rejects( [ & ]() { State::NewWorld( Technologies( { { 1, "MORGANITES" } } ), technologies, units, difficulty ); } ), "invalid original difficulty accepted" ); }
		GT_OK();
	} );
	task->AddTest( "SMAC startup design admission rejects extra ownership and changed capabilities without cosmetic guesses", GT() {
		const auto technology = Technologies( { { 4, "MORGANITES" } } ).factions[ 0 ]; const auto tech = smac_rules_fixture::Technologies();
		const auto original = smac_startup_fixture::Units().catalog;
		for ( size_t scenario = 0; scenario < 13; ++scenario ) {
			auto catalog = original; auto state = technology;
			if ( scenario == 0 ) { state.owned[ 88 ] = true; }
			if ( scenario == 1 ) { state.owned[ 1 ] = false; }
			if ( scenario == 2 ) { catalog.chassis.chassis[ 2 ].prerequisite = "None"; }
			if ( scenario == 3 ) { catalog.weapons.weapons[ 1 ].prerequisite = "None"; }
			if ( scenario == 4 ) { catalog.defenses.defenses[ 1 ].prerequisite = "Unknown"; }
			if ( scenario == 5 ) { catalog.reactors.reactors[ 1 ].prerequisite = "Tech1"; }
			if ( scenario == 6 ) { catalog.abilities.abilities[ 0 ].prerequisite = "None"; }
			if ( scenario == 7 ) { catalog.chassis.chassis[ 0 ].speed = 2; }
			if ( scenario == 8 ) { catalog.defenses.defenses[ 1 ].rating = 3; }
			if ( scenario == 9 ) { catalog.weapons.weapons[ 0 ].offense = 2; }
			if ( scenario == 10 ) { catalog.units[ 2 ].plan = 0; }
			if ( scenario == 11 ) { state.admission.faction_id = "Other"; }
			if ( scenario == 12 ) { state.admission.simulation_id = 8; }
			GT_ASSERT( Rejects( [ & ]() { State::GenerateEmptyOwner( state, tech, catalog, 3 ); } ), "unsupported startup capability admitted " + std::to_string( scenario ) );
		}
		auto compatible = original;
		compatible.chassis.chassis[ 0 ].names[ 0 ].name = "Cosmetic"; compatible.defenses.defenses[ 1 ].name = "Another name";
		compatible.defenses.defenses[ 1 ].cost = 2000;
		const auto expected_cost = unit::SmacPredefinedUnitCatalog::CalculateCost( compatible.chassis.chassis[ 0 ], compatible.weapons.weapons[ 0 ], compatible.defenses.defenses[ 1 ], 1, 0, compatible.abilities );
		const auto generated = State::GenerateEmptyOwner( technology, tech, compatible, 3 );
		GT_ASSERT( expected_cost > 255 && generated.designs[ 0 ].cost == static_cast< uint8_t >( expected_cost ), "startup used a hardcoded cost or cosmetic classifier" );
		GT_OK();
	} );
	task->AddTest( "SMAC startup design codec owns cached and inactive records without replay", GT() {
		const auto units = smac_startup_fixture::Units(); const auto expected = Saved();
		auto value = State::Deserialize( expected.Serialize( units.catalog ).ToString(), units.catalog );
		GT_ASSERT( value == expected && value.Find( 7 )->designs[ 0 ].cost == 255 && !value.Find( 7 )->designs[ 0 ].active && !value.Find( 7 )->designs[ 0 ].prototyped, "restore regenerated cached or prototype state" );
		auto copied = value; auto moved = std::move( copied ); value.owners.clear();
		GT_ASSERT( moved == expected && State::Deserialize( Wire( moved ), units.catalog ) == expected, "design state borrowed prior containers or wire" );
		GT_OK();
	} );
	task->AddTest( "SMAC startup design codec rejects bounded malformed fields and foreign selection", GT() {
		const auto catalog = smac_startup_fixture::Units().catalog; const auto saved = Saved();
		for ( size_t index = 0; index < 17; ++index ) {
			for ( const auto value : { INT64_MIN, int64_t( -1 ), INT64_MAX } ) {
				GT_ASSERT( Rejects( [ & ]() { State::Deserialize( Wire( saved, index, value ), catalog ); } ), "out-of-range design field accepted" );
			}
		}
		for ( const auto& item : std::vector< std::pair< size_t, int64_t > >{ {1,2}, {2,6}, {3,8}, {4,0}, {5,64}, {6,65}, {7,447}, {8,9}, {9,21}, {10,10}, {11,0}, {11,5}, {12,256}, {13,256}, {14,256}, {15,0x1000000}, {16,int64_t(UINT32_MAX)+1} } ) {
			GT_ASSERT( Rejects( [ & ]() { State::Deserialize( Wire( saved, item.first, item.second ), catalog ); } ), "invalid bounded design field accepted" );
		}
		const auto wire = Wire( saved );
		for ( size_t i = 0; i < wire.size(); ++i ) { GT_ASSERT( Rejects( [ & ]() { State::Deserialize( wire.substr( 0, i ), catalog ); } ), "truncated design record accepted" ); }
		GT_ASSERT( Rejects( [ & ]() { State::Deserialize( wire + "x", catalog ); } ) && Rejects( [ & ]() { State::Deserialize( std::string( State::MAX_WIRE_BYTES + 1, 'x' ), catalog ); } ), "trailing or over-budget design payload accepted" );
		for ( size_t scenario = 0; scenario < 5; ++scenario ) {
			auto bad = saved;
			if ( scenario == 0 ) { bad.owners.push_back( bad.owners[ 0 ] ); }
			if ( scenario == 1 ) { bad.owners[ 0 ].designs.push_back( bad.owners[ 0 ].designs[ 0 ] ); }
			if ( scenario == 2 ) { bad.owners[ 0 ].starting_scout = 128; }
			if ( scenario == 3 ) { bad.owners[ 0 ].designs[ 0 ].id = 511; bad.owners[ 0 ].starting_scout = 448; }
			if ( scenario == 4 ) { bad.owners.clear(); }
			GT_ASSERT( Rejects( [ & ]() { bad.Serialize( catalog ); } ), "invalid design ownership serialized" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC startup marking keeps ascending active Recon selection and skips inactive records", GT() {
		const auto catalog = smac_startup_fixture::Units().catalog;
		auto owner = Saved().owners[ 1 ]; auto second = owner.designs[ 0 ]; second.id += 3; owner.designs.push_back( second );
		auto inactive = second; inactive.id += 1; inactive.active = false; inactive.prototyped = false; owner.designs.push_back( inactive );
		State::MarkAndSelect( owner, catalog );
		GT_ASSERT( owner.starting_scout == second.id && owner.designs[ 0 ].prototyped && owner.designs[ 1 ].prototyped && !owner.designs[ 2 ].prototyped, "marking changed ordered/active selection semantics" );
		GT_OK();
	} );
}
}
