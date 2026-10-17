#include "Async.h"

#include "gse/Async.h"
#include "gse/GSE.h"
#include "gse/ExecutionPointer.h"
#include "gse/callable/Native.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Null.h"
#include "gse/value/Undefined.h"
#include "gc/Space.h"
#include "task/gsetests/GSETests.h"
#include "util/FinallyGuard.h"

#include <stdexcept>

namespace gse::tests {

// Only control deadlines and dispatch one real bucket; scheduling, invocation,
// cancellation and repeat handling all remain in Async's production methods.
class AsyncFixture {
public:
	explicit AsyncFixture( Async* async ) : async( async ) {}

	void SetDeadline( timer_id_t id, uint64_t deadline = 0 ) {
		const auto old = async->m_timers_ms.at( id );
		auto timer = async->m_timers.at( old ).extract( id );
		if ( async->m_timers.at( old ).empty() ) { async->m_timers.erase( old ); }
		async->m_timers[ deadline ].insert( std::move( timer ) );
		async->m_timers_ms.at( id ) = deadline;
	}

	void Dispatch( ExecutionPointer& ep ) {
		if ( async->m_timers.empty() ) { throw std::runtime_error( "no timer bucket to dispatch" ); }
		async->ProcessTimers( async->m_timers.begin(), ep );
	}

	size_t Count() const { return async->m_timers_ms.size(); }
	size_t Delay( timer_id_t id ) const {
		return async->m_timers.at( async->m_timers_ms.at( id ) ).at( id ).ms;
	}
	bool IsConsistent() const {
		size_t count = 0;
		for ( const auto& bucket : async->m_timers ) {
			if ( bucket.second.empty() ) { return false; }
			for ( const auto& timer : bucket.second ) {
				const auto found = async->m_timers_ms.find( timer.first );
				if ( found == async->m_timers_ms.end() || found->second != bucket.first ) { return false; }
				++count;
			}
		}
		return count == Count();
	}

	Async* const async;
};

using scenario_t = std::function< std::string( AsyncFixture&, gc::Space*, context::Context*, ExecutionPointer& ) >;

static std::string RunScenario( GSE* gse, context::Context* ctx, const scenario_t& run ) {
	std::string error;
	auto* gc_space = gse->GetGCSpace();
	gc_space->Accumulate( gse, [ & ]() {
		AsyncFixture fixture( gse->GetAsync() );
		ExecutionPointer ep;
		// Always clear pending timers, including after a failed assertion: a
		// cancellation regression must fail promptly instead of hanging teardown.
		util::FinallyGuard cleanup( [ & ]() { fixture.async->StopTimers(); } );
		error = run( fixture, gc_space, ctx, ep );
	} );
	return error;
}

void AddAsyncTests( task::gsetests::GSETests* task ) {
	for ( const bool integer_repeat : { false, true } ) {
		task->AddTest( integer_repeat ? "test async cancels integer repeat later in same bucket" : "test async cancels boolean repeat later in same bucket", GT( integer_repeat ) {
			return RunScenario( gse, ctx, [ & ]( AsyncFixture& f, gc::Space* gc_space, context::Context* ctx, ExecutionPointer& ep ) -> std::string {
				size_t calls = 0;
				bool canceled = false;
				const auto repeating = f.async->StartTimer( 120, NATIVE_CALL( & ) {
					++calls;
					if ( integer_repeat ) { return VALUE( value::Int, , 7 ); }
					return VALUE( value::Bool, , true );
				} ), ctx, {}, ep );
				const auto stop = f.async->StartTimer( 1000, NATIVE_CALL( & ) {
					canceled = f.async->StopTimer( repeating );
					return VALUE( value::Undefined );
				} ), ctx, {}, ep );
				f.SetDeadline( repeating ); f.SetDeadline( stop ); f.Dispatch( ep );
				GT_ASSERT( canceled, "later same-deadline callback could not cancel the repeat" );
				GT_ASSERT( calls == 1 && f.Count() == 0 && f.IsConsistent(), "canceled repeat remained scheduled" );
				GT_OK();
			} );
		} );
	}

	task->AddTest( "test async skips canceled sibling in same bucket", GT() {
		return RunScenario( gse, ctx, [ & ]( AsyncFixture& f, gc::Space* gc_space, context::Context* ctx, ExecutionPointer& ep ) -> std::string {
			timer_id_t sibling = 0;
			size_t calls = 0;
			bool canceled = false;
			const auto stop = f.async->StartTimer( 1, NATIVE_CALL( & ) {
				canceled = f.async->StopTimer( sibling ); return nullptr;
			} ), ctx, {}, ep );
			sibling = f.async->StartTimer( 1, NATIVE_CALL( & ) { ++calls; return nullptr; } ), ctx, {}, ep );
			f.SetDeadline( stop ); f.SetDeadline( sibling ); f.Dispatch( ep );
			GT_ASSERT( canceled && calls == 0, "canceled sibling still ran" );
			GT_ASSERT( f.Count() == 0 && f.IsConsistent(), "cancellation left inconsistent timer indices" );
			GT_OK();
		} );
	} );

	task->AddTest( "test async self cancellation preserves other repeats and siblings", GT() {
		return RunScenario( gse, ctx, [ & ]( AsyncFixture& f, gc::Space* gc_space, context::Context* ctx, ExecutionPointer& ep ) -> std::string {
			timer_id_t self = 0;
			std::vector< int > order;
			bool canceled = false;
			const auto repeat = f.async->StartTimer( 19, NATIVE_CALL( & ) { order.push_back( 1 ); return VALUE( value::Bool, , true ); } ), ctx, {}, ep );
			self = f.async->StartTimer( 1, NATIVE_CALL( & ) { order.push_back( 2 ); canceled = f.async->StopTimer( self ); return VALUE( value::Bool, , true ); } ), ctx, {}, ep );
			const auto sibling = f.async->StartTimer( 1, NATIVE_CALL( & ) { order.push_back( 3 ); return VALUE( value::Null ); } ), ctx, {}, ep );
			f.SetDeadline( repeat ); f.SetDeadline( self ); f.SetDeadline( sibling ); f.Dispatch( ep );
			GT_ASSERT( canceled && order == std::vector< int >( { 1, 2, 3 } ), "self cancellation interrupted sibling dispatch" );
			GT_ASSERT( f.Count() == 1 && f.IsConsistent() && f.Delay( repeat ) == 19, "self cancellation lost another repeat" );
			GT_ASSERT( f.async->StopTimer( repeat ) && !f.async->StopTimer( repeat ), "stopping a repeat was not idempotent" );
			GT_OK();
		} );
	} );

	for ( const bool stop_all : { false, true } ) {
		task->AddTest( stop_all ? "test async stop all preserves subsequently created timer" : "test async preserves nested timer in same deadline bucket", GT( stop_all ) {
			return RunScenario( gse, ctx, [ & ]( AsyncFixture& f, gc::Space* gc_space, context::Context* ctx, ExecutionPointer& ep ) -> std::string {
				size_t nested_calls = 0, sibling_calls = 0;
				if ( stop_all ) {
					const auto repeat = f.async->StartTimer( 1, NATIVE_CALL() { return VALUE( value::Bool, , true ); } ), ctx, {}, ep );
					f.SetDeadline( repeat );
				}
				const auto parent = f.async->StartTimer( 1, NATIVE_CALL( & ) {
					if ( stop_all ) { f.async->StopTimers(); }
					const auto nested = f.async->StartTimer( 1, new callable::Native( gc_space, ctx, [ & ]( GSE_CALLABLE, const value::function_arguments_t& ) -> Value* { ++nested_calls; return nullptr; } ), ctx, {}, ep );
					f.SetDeadline( nested );
					return VALUE( value::Bool, , stop_all );
				} ), ctx, {}, ep );
				const auto sibling = f.async->StartTimer( 1, NATIVE_CALL( & ) { ++sibling_calls; return nullptr; } ), ctx, {}, ep );
				f.SetDeadline( parent ); f.SetDeadline( sibling ); f.Dispatch( ep );
				GT_ASSERT( f.Count() == 1 && f.IsConsistent() && nested_calls == 0, "nested timer was lost or ran in its parent's batch" );
				GT_ASSERT( sibling_calls == ( stop_all ? 0 : 1 ), "stop all did not cancel the pending sibling" );
				f.Dispatch( ep );
				GT_ASSERT( nested_calls == 1 && f.Count() == 0 && f.IsConsistent(), "nested one-shot did not finish exactly once" );
				GT_OK();
			} );
		} );
	}

	task->AddTest( "test async updates repeat delay and finishes one shot", GT() {
		return RunScenario( gse, ctx, [ & ]( AsyncFixture& f, gc::Space* gc_space, context::Context* ctx, ExecutionPointer& ep ) -> std::string {
			size_t calls = 0;
			const auto timer = f.async->StartTimer( 3, NATIVE_CALL( & ) {
				if ( ++calls == 1 ) { return VALUE( value::Int, , 17 ); }
				if ( calls == 2 ) { return VALUE( value::Bool, , true ); }
				return VALUE( value::Bool, , false );
			} ), ctx, {}, ep );
			for ( size_t expected = 1 ; expected <= 3 ; ++expected ) {
				f.SetDeadline( timer ); f.Dispatch( ep );
				GT_ASSERT( calls == expected && f.IsConsistent(), "timer did not execute once per dispatch" );
				GT_ASSERT( f.Count() == ( expected == 3 ? 0 : 1 ), "one-shot/repeat completion changed" );
				if ( expected != 3 ) { GT_ASSERT( f.Delay( timer ) == 17, "repeat delay was not retained" ); }
			}
			GT_OK();
		} );
	} );

	task->AddTest( "test async exception preserves cancellable pending timers", GT() {
		return RunScenario( gse, ctx, [ & ]( AsyncFixture& f, gc::Space* gc_space, context::Context* ctx, ExecutionPointer& ep ) -> std::string {
			std::vector< int > order;
			const auto completed = f.async->StartTimer( 1, NATIVE_CALL( & ) { order.push_back( 1 ); return nullptr; } ), ctx, {}, ep );
			const auto failing = f.async->StartTimer( 1, NATIVE_CALL( & ) { order.push_back( 2 ); throw std::runtime_error( "timer failure" ); } ), ctx, {}, ep );
			const auto sibling = f.async->StartTimer( 1, NATIVE_CALL( & ) { order.push_back( 3 ); return nullptr; } ), ctx, {}, ep );
			f.SetDeadline( completed ); f.SetDeadline( failing ); f.SetDeadline( sibling );
			bool threw = false;
			try { f.Dispatch( ep ); }
			catch ( const std::runtime_error& error ) { threw = std::string( error.what() ) == "timer failure"; }
			GT_ASSERT( threw && order == std::vector< int >( { 1, 2 } ), "timer exception did not propagate immediately" );
			GT_ASSERT( f.Count() == 2 && f.IsConsistent(), "exception lost pending timers or retained a completed one" );
			GT_ASSERT( !f.async->StopTimer( completed ) && f.async->StopTimer( failing ), "pending failing callback was not cancellable" );
			f.Dispatch( ep );
			GT_ASSERT( order == std::vector< int >( { 1, 2, 3 } ) && f.Count() == 0 && f.IsConsistent(), "exception replayed a completed callback or lost a sibling" );
			GT_OK();
		} );
	} );
}

}
