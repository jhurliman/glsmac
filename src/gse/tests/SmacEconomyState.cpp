#include "SmacEconomyState.h"
#include "game/backend/faction/SmacEconomyState.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include "fixtures/SmacResearchSubmission.h"
#include "fixtures/SmacProductionReferences.h"
#include "game/backend/base/ProductionState.h"

namespace gse::tests {
using game::backend::faction::SmacEconomyState;
namespace {
template< typename F > bool Rejects( F&& operation ) { try { operation(); } catch ( const std::exception& ) { return true; } return false; }
}

std::string CheckSmacEconomyRestore() {
	SmacEconomyState value{ { 1, 7 }, INT32_MIN, INT32_MAX, 99, 0xa5 };
	value.regional_base_counts=SmacEconomyState::RegionalBaseCounts{};
	for(size_t i=0;i<128;++i) {(*value.regional_base_counts)[i]=static_cast<uint8_t>(i*2+1);}
    value.design_counts=game::backend::faction::SmacDesignCounts{};
    for(size_t i=0;i<512;++i) {(*value.design_counts)[i]=static_cast<uint8_t>(i*3+1);}
    value.production_reference_counts=game::backend::faction::SmacDesignCounts{};
    for(size_t i=0;i<512;++i) {(*value.production_reference_counts)[i]=static_cast<uint8_t>(i*5+3);}
	const auto wire = value.Serialize().ToString();
	const auto restored = SmacEconomyState::Deserialize( wire );
	if ( wire.size() != SmacEconomyState::MAX_WIRE_BYTES || !( restored == value ) || restored.Serialize().ToString() != wire ) {
		return "economy restore changed raw balances or rates";
	}
	for ( const auto& malformed : { std::string{}, wire.substr( 0, wire.size() - 1 ), wire + "x" } ) {
		if ( !Rejects( [ & ] { SmacEconomyState::Deserialize( malformed ); } ) ) { return "malformed economy extent accepted"; }
	}
	for ( int scenario = 0; scenario < 7; ++scenario ) {
		types::Buffer buffer; buffer.WriteInt( 0x534d4543 ); buffer.WriteInt( scenario == 0 ? 2 : 1 );
		buffer.WriteInt( scenario == 1 ? -1 : scenario == 2 ? 6 : 0 ); buffer.WriteInt( scenario == 2 ? 5 : 0 );
		buffer.WriteInt( scenario == 3 ? int64_t( INT32_MAX ) + 1 : 0 ); buffer.WriteInt( scenario == 4 ? int64_t( INT32_MIN ) - 1 : 0 );
		buffer.WriteInt( scenario == 5 ? 100 : 0 ); buffer.WriteInt( scenario == 6 ? 256 : 0 );
		if ( !Rejects( [ & ] { SmacEconomyState::Deserialize( buffer.ToString() ); } ) ) { return "malformed economy domain accepted"; }
	}
	return {};
}

std::string CheckSmacEconomyPosting() {
	// TWO-BASE-POSTING-001 corrected public JSON SHA256
	// 312a3c68f9301f89855dd9e92914f2483bc5c23cc5a66557c1bfac61c3537b9a.
	// Literal credit/fee/debit projection only, not research or full upkeep.
	struct Case { int32_t maintenance, debit, first, second; };
	for ( const auto row : { Case{ 0, 0, 102, 104 }, Case{ 0, 0, 102, 104 }, Case{ 1, 0, 101, 102 }, Case{ 0, 7, 95, 97 } } ) {
		auto state = SmacEconomyState::NewWorld( { 5, 0 }, 100, 0 ); state.pending_debit = row.debit;
		const auto foreign = SmacEconomyState::NewWorld( { 5, 0 }, 321, 0 ); const auto foreign_wire = foreign.Serialize().ToString();
		for ( int turn = 0; turn < 2; ++turn ) {
			state = state.CreditBase( 1, 0 ).CreditBase( 1, row.maintenance ).SettleDebit();
			if ( state.credits != ( turn == 0 ? row.first : row.second ) || state.pending_debit != 0
				|| foreign.Serialize().ToString() != foreign_wire ) { return "captured per-Base income or faction settlement differs"; }
			state = SmacEconomyState::Deserialize( state.Serialize().ToString() );
		}
	}
	auto excess = SmacEconomyState::NewWorld( { 0, 5 }, 3, 0 ); excess.pending_debit = 7;
	const auto settled = excess.SettleDebit();
	if ( settled.credits != 0 || settled.pending_debit != 0 || !( settled.SettleDebit() == settled ) || excess.credits != 3 || excess.pending_debit != 7 ) {
		return "measured reservation floor/clear or candidate ownership differs";
	}
	return {};
}

std::string CheckSmacEconomyFailure() {
	const auto state = SmacEconomyState::NewWorld( { 0, 5 }, 2, 0 ); const auto before = state.Serialize().ToString();
	if ( !Rejects( [ & ] { state.CreditBase( 0, 3 ); } ) || !Rejects( [ & ] { state.CreditBase( -1, 0 ); } )
		|| !Rejects( [ & ] { state.CreditBase( INT32_MAX, 2 ); } ) || state.Serialize().ToString() != before ) {
		return "failed candidate changed balance or bypassed credit-before-fee overflow";
	}
	return {};
}

std::string CheckSmacResearchSubmission() {
 for ( const auto& row : economy_fixture::submissions ) {
  auto fraction=row.fraction;
  for ( size_t i=0;i<5;++i ) {
   const auto result=game::backend::faction::ScaleSmacResearchSubmission(row.labs,row.rating,row.turn,row.difficulty,row.halved,fraction);
   if(result.increment!=row.increments[i]||result.fraction!=row.fractions[i]) { return "original research submission differs: "+std::string(row.name); }
   fraction=result.fraction;
  }
 }
 if(!Rejects([]{game::backend::faction::ScaleSmacResearchSubmission(INT32_MAX,5,100,3,false,0);})
  ||!Rejects([]{game::backend::faction::ScaleSmacResearchSubmission(1,0,1,3,false,100);})) { return "research input/overflow admission failed"; }
 return {};
}

void AddSmacEconomyStateTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC production references match 13 original summary and manual queue controls",GT(){
        using namespace game::backend::faction;
        for(const auto& row:production_cache_fixture::summaries) {
            GT_ASSERT(RecountSmacProductionReferences(row.owner,row.bases)==row.expected.Expand(),"original summary: "+std::string(row.name));
        }
        for(const auto& row:production_cache_fixture::manual) {
            const auto detached=ChangeSmacProductionReferences(row.before.Expand(),row.old_queue,{});
            GT_ASSERT(detached==row.detached.Expand(),"original detach: "+std::string(row.name));
            GT_ASSERT(ChangeSmacProductionReferences(detached,{},row.new_queue)==row.after.Expand(),"original attach: "+std::string(row.name));
        }
        GT_ASSERT(Rejects([]{RecountSmacProductionReferences(8,{});})
            && Rejects([]{RecountSmacProductionReferences(1,{{1,1,{64}}});})
            && Rejects([]{ChangeSmacProductionReferences({}, {}, {512});}),"invalid reference extent accepted");GT_OK();
    });
    task->AddTest("SMAC production references preserve stale bytes and absent V4 authority",GT(){
        using namespace game::backend::faction;
        auto value=SmacEconomyState::NewWorld({0,5},100,0);
        GT_ASSERT(value.production_reference_counts==std::optional<SmacDesignCounts>(SmacDesignCounts{}),"new faction did not clear cache");
        value.production_reference_counts.reset();const auto legacy=value.Serialize().ToString();types::Buffer header(legacy);header.ReadInt();
        GT_ASSERT(header.ReadInt()==4,"absent cache changed legacy wire version");
        auto restored=SmacEconomyState::Deserialize(legacy);
        GT_ASSERT(!restored.production_reference_counts && restored.Serialize().ToString()==legacy,"legacy load recounted production");
        restored.production_reference_counts=SmacDesignCounts{};
        restored.production_reference_counts->at(0)=255;restored.production_reference_counts->at(2)=251;
        restored.production_reference_counts->at(128)=201;restored.production_reference_counts->at(511)=17;
        const auto wire=restored.Serialize().ToString();
        GT_ASSERT(SmacEconomyState::Deserialize(wire)==restored && SmacEconomyState::Deserialize(wire).Serialize().ToString()==wire,"stale production cache changed on load");
        for(const size_t size:{0,511,513}) {
            types::Buffer malformed;malformed.WriteInt(0x534d4543);malformed.WriteInt(5);
            for(const int n:{0,5,100,0,0,0}) {malformed.WriteInt(n);}
            malformed.WriteBool(false);malformed.WriteBool(false);malformed.WriteBool(false);
            malformed.WriteBool(true);malformed.WriteString(std::string(size,'x'));
            GT_ASSERT(Rejects([&]{SmacEconomyState::Deserialize(malformed.ToString());}),"wrong-sized production row accepted");
        }GT_OK();
    });
    task->AddTest("SMAC production references project current pending duplicates without baseline",GT(){
        using namespace game::backend::base;
        const ProductionItem scout{ProductionItemKind::Unit,"scout",2},former{ProductionItemKind::Unit,"former",1},stockpile{ProductionItemKind::Stockpile,"",38};
        std::optional<ProductionState> state=ProductionState{scout,0,former,0,std::vector<ProductionItem>{scout,former,stockpile}};
        GT_ASSERT(SmacProductionReferenceTargets(state)==std::optional<std::vector<int32_t>>({2,2,1}),"baseline counted or queue duplicates lost");
        state->queue.reset();GT_ASSERT(!SmacProductionReferenceTargets(state),"unknown queue invented empty authority");
        state->queue=std::vector<ProductionItem>{};state->current_item.smac_source_id.reset();
        GT_ASSERT(!SmacProductionReferenceTargets(state) && !SmacProductionReferenceTargets(std::nullopt),"unresolved target fabricated identity");GT_OK();
    });
	task->AddTest("SMAC regional Base byte counts preserve stale values and migrate absent V2 history",GT(){
		auto fresh=SmacEconomyState::NewWorld({0,5},100,0);
		GT_ASSERT(fresh.regional_base_counts && *fresh.regional_base_counts==SmacEconomyState::RegionalBaseCounts{},"fresh faction did not clear all 128 counts");
		types::Buffer legacy;for(const auto v:{int64_t(0x534d4543),int64_t(2),int64_t(0),int64_t(5),int64_t(100),int64_t(0),int64_t(0),int64_t(0)}) {legacy.WriteInt(v);}
		legacy.WriteBool(true);legacy.WriteInt(77);
		const auto migrated=SmacEconomyState::Deserialize(legacy.ToString());
		GT_ASSERT(!migrated.regional_base_counts && migrated.population_aggregate==77
			&& SmacEconomyState::Deserialize(migrated.Serialize().ToString())==migrated,"V2 load invented historical regional counts");
		for(const size_t size:{0,127,129}) {
			types::Buffer malformed;for(const auto v:{int64_t(0x534d4543),int64_t(3),int64_t(0),int64_t(5),int64_t(100),int64_t(0),int64_t(0),int64_t(0)}) {malformed.WriteInt(v);}
			malformed.WriteBool(false);malformed.WriteBool(true);malformed.WriteString(std::string(size,'x'));
			GT_ASSERT(Rejects([&]{SmacEconomyState::Deserialize(malformed.ToString());}),"wrong-sized count array accepted");
		}GT_OK();
	});
	task->AddTest("SMAC economy retains independent population history and marks V1 migration unknown",GT(){
		for(const auto aggregate:{INT32_MIN,0,2,INT32_MAX}) {
			auto value=SmacEconomyState::NewWorld({5,0},100,0);value.population_aggregate=aggregate;
			const auto wire=value.Serialize().ToString();auto restored=SmacEconomyState::Deserialize(wire);
			GT_ASSERT(restored==value && restored.Serialize().ToString()==wire,"population aggregate changed on restore");
			restored.population_aggregate.reset();GT_ASSERT(!(restored==value),"equality omits population history");
		}
		types::Buffer legacy;for(const auto value:{int64_t(0x534d4543),int64_t(1),int64_t(5),int64_t(0),int64_t(100),int64_t(0),int64_t(0),int64_t(0)}) {legacy.WriteInt(value);}
		const auto migrated=SmacEconomyState::Deserialize(legacy.ToString());
		GT_ASSERT(!migrated.population_aggregate && migrated.credits==100 && !SmacEconomyState::Deserialize(migrated.Serialize().ToString()).population_aggregate,"old port save invented a current population aggregate");
		GT_ASSERT(SmacEconomyState::NewWorld({5,0},100,0).population_aggregate==0,"fresh initialization did not clear population aggregate");
		for(const auto invalid:{int64_t(INT32_MIN)-1,int64_t(INT32_MAX)+1}) {
			types::Buffer wire;for(const auto value:{int64_t(0x534d4543),int64_t(2),int64_t(5),int64_t(0),int64_t(100),int64_t(0),int64_t(0),int64_t(0)}) {wire.WriteInt(value);}wire.WriteBool(true);wire.WriteInt(invalid);
			GT_ASSERT(Rejects([&]{SmacEconomyState::Deserialize(wire.ToString());}),"out-of-domain population aggregate accepted");
		}GT_OK();
	});
 task->AddTest( "SMAC research submission matches 60 original scaled increments and retained fractions", GT() {
  const auto error=CheckSmacResearchSubmission(); GT_ASSERT(error.empty(), "research: "+error); GT_OK();
 } );
	task->AddTest( "SMAC economy state preserves exact rates balances and bounded framing", GT() {
		const auto error = CheckSmacEconomyRestore(); GT_ASSERT( error.empty(), "economy: " + error ); GT_OK();
	} );
	task->AddTest( "SMAC economy credits match four original two-Base return sequences", GT() {
		const auto error = CheckSmacEconomyPosting(); GT_ASSERT( error.empty(), "economy: " + error ); GT_OK();
	} );
	task->AddTest( "SMAC economy rejected posting leaves supplied authority unchanged", GT() {
		const auto error = CheckSmacEconomyFailure(); GT_ASSERT( error.empty(), "economy: " + error ); GT_OK();
	} );
}
}
