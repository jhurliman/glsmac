#include "SmacEconomyState.h"
#include "game/backend/faction/SmacEconomyState.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include "fixtures/SmacResearchSubmission.h"

namespace gse::tests {
using game::backend::faction::SmacEconomyState;
namespace {
template< typename F > bool Rejects( F&& operation ) { try { operation(); } catch ( const std::exception& ) { return true; } return false; }
}

std::string CheckSmacEconomyRestore() {
	SmacEconomyState value{ { 1, 7 }, INT32_MIN, INT32_MAX, 99, 0xa5 };
	value.regional_base_counts=SmacEconomyState::RegionalBaseCounts{};
	for(size_t i=0;i<128;++i) {(*value.regional_base_counts)[i]=static_cast<uint8_t>(i*2+1);}
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
