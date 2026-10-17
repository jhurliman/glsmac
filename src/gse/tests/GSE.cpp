#include "GSE.h"

#include "task/gsetests/GSETests.h"
#include "gse/GSE.h"
#include "gse/Async.h"
#include "gse/Wrappable.h"
#include "gse/value/Null.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Object.h"
#include "gse/value/Callable.h"
#include "gse/ExecutionPointer.h"
#include "gse/Exception.h"
#include "gse/context/GlobalContext.h"
#include "gc/GC.h"
#include "gc/Space.h"
#include "engine/Engine.h"
#include "util/FinallyGuard.h"

#include <atomic>
#include <exception>
#include <memory>
#include <stdexcept>
#include <thread>

namespace gse {
namespace tests {

void AddGSETests( task::gsetests::GSETests* task ) {
	for ( const int kind : { 1, 2, 0 } ) {
		task->AddTest( std::string( "test event callbacks recover after " ) + ( kind == 1 ? "native exceptions" : kind == 2 ? "nonstandard exceptions" : "GSE exceptions" ), GT( kind ) {
			class EventSource : public Wrappable {
			public:
				Value* const Wrap( GSE_CALLABLE ) override { return nullptr; }
			} source;
			auto* gc_space = gse->GetGCSpace();
			const si_t si;
			ExecutionPointer ep;
			std::string error;
			gc_space->Accumulate( nullptr, [ & ]() {
				error = [ & ]() -> std::string {
					std::vector< int > order;
					bool fail = true;
					size_t arguments_built = 0;
					const auto first = source.On( GSE_CALL, "event", NATIVE_CALL( &order, &fail, kind ) {
						order.push_back( 1 );
						if ( fail ) {
							fail = false;
							if ( kind == 1 ) { throw std::runtime_error( "native event failure" ); }
							if ( kind == 2 ) { throw 73; }
							GSE_ERROR( EC.GAME_ERROR, "GSE event failure" );
						}
						return VALUE( value::Int,, 11 );
					} ) );
					source.On( GSE_CALL, "event", NATIVE_CALL( &order ) { order.push_back( 2 ); return VALUE( value::Int,, 22 ); } ) );
					const f_args_t args = [ & ]( GSE_CALLABLE, value::object_properties_t& properties ) {
						++arguments_built;
						properties.emplace( "argument", VALUE( value::Int,, 41 ) );
					};
					bool caught = false;
					try { source.Trigger( GSE_CALL, "event", args, VT_INT ); }
					catch ( const Exception& exception ) { caught = kind == 0 && exception.class_name == EC.GAME_ERROR && exception.reason == "GSE event failure"; }
					catch ( const std::runtime_error& exception ) { caught = kind == 1 && std::string( exception.what() ) == "native event failure"; }
					catch ( const int value ) { caught = kind == 2 && value == 73; }
					GT_ASSERT( caught && order == std::vector< int >{ 1 } && arguments_built == 1, "callback exception or stop order changed" );
					// This real second dispatch hangs with the old unlocked lock_guard.
					auto* result = source.Trigger( GSE_CALL, "event", args, VT_INT );
					GT_ASSERT( VALUE_GET( value::Int, result ) == 22 && order == ( std::vector< int >{ 1, 1, 2 } ) && arguments_built == 2, "subsequent callback order/result changed" );
					source.Off( GSE_CALL, "event", first );
					source.On( GSE_CALL, "event", NATIVE_CALL( &order ) { order.push_back( 3 ); return VALUE( value::Int,, 33 ); } ) );
					result = source.Trigger( GSE_CALL, "event", args, VT_INT );
					GT_ASSERT( VALUE_GET( value::Int, result ) == 33 && order == ( std::vector< int >{ 1, 1, 2, 2, 3 } ) && arguments_built == 3, "handler changes after failure did not take effect" );
					source.ClearHandlers();
					GT_ASSERT( !source.HasHandlers( "event" ), "handler cleanup failed after callback exception" );
					GT_OK();
				}();
			} );
			GT_ASSERT( error.empty(), "event recovery failed: " + error );
			GT_OK();
		} );
	}
	task->AddTest( "test event callbacks keep snapshot order and unlock during reentrant changes", GT() {
		class EventSource : public Wrappable {
		public:
			Value* const Wrap( GSE_CALLABLE ) override { return nullptr; }
		} source;
		auto* gc_space = gse->GetGCSpace();
		const si_t si;
		ExecutionPointer ep;
		std::string error;
		gc_space->Accumulate( nullptr, [ & ]() {
			error = [ & ]() -> std::string {
				std::vector< int > order;
				Wrappable::callback_id_t second;
				bool changed = false;
				auto* third = NATIVE_CALL( &order ) { order.push_back( 3 ); return VALUE( value::Int,, 33 ); } );
				source.On( GSE_CALL, "event", NATIVE_CALL( &source, &order, &changed, &second, third ) {
					order.push_back( 1 );
					if ( !changed ) {
						changed = true;
						source.Off( GSE_CALL, "event", second );
						source.On( GSE_CALL, "event", third );
					}
					return VALUE( value::Int,, 11 );
				} ) );
				second = source.On( GSE_CALL, "event", NATIVE_CALL( &order ) { order.push_back( 2 ); return VALUE( value::Int,, 22 ); } ) );
				auto* args = VALUEEXT( value::Object, GSE_CALL );
				auto* result = source.Trigger( GSE_CALL, "event", args, VT_INT );
				GT_ASSERT( VALUE_GET( value::Int, result ) == 22 && order == ( std::vector< int >{ 1, 2 } ), "callback changes altered the active snapshot" );
				bool rejected = false;
				try { source.Trigger( GSE_CALL, "event", args, VT_STRING ); }
				catch ( const Exception& exception ) { rejected = exception.class_name == EC.INVALID_HANDLER; }
				GT_ASSERT( rejected && order == ( std::vector< int >{ 1, 2, 1 } ), "return-type failure changed callback order" );
				result = source.Trigger( GSE_CALL, "event", args, VT_INT );
				GT_ASSERT( VALUE_GET( value::Int, result ) == 33 && order == ( std::vector< int >{ 1, 2, 1, 1, 3 } ), "callback registry did not survive return-type failure" );
				source.ClearHandlers();
				GT_OK();
			}();
		} );
		GT_ASSERT( error.empty(), "event snapshot validation failed: " + error );
		GT_OK();
	} );

	task->AddTest( "test GSE exception what owns stable formatted storage across copies", GT() {
		std::unique_ptr< Exception > copied;
		std::exception_ptr thrown;
		std::string expected;
		{
			ExecutionPointer ep;
			const si_t outer = { "", { 7, 1 }, { 7, 2 } }, inner = { "", { 11, 2 }, { 11, 3 } };
			ep.WithSI( outer, [ & ]() {
				Exception original( "OwnershipError", "A sufficiently long diagnostic message to exercise allocated storage", nullptr, inner, ep );
				expected = original.ToString();
				copied = std::make_unique< Exception >( original );
				try { throw original; } catch ( ... ) { thrown = std::current_exception(); }
			} );
		}
		GT_ASSERT( expected.find( "OwnershipError" ) != std::string::npos && expected.find( "\tat :11:" ) != std::string::npos && expected.find( "\tat :7:" ) != std::string::npos, "fixture lost its diagnostic or captured stack" );
		const auto check = [ & ]( const std::exception& error ) -> std::string {
			const auto* first = error.what();
			GT_ASSERT( first && std::string( first ) == expected, "what changed the formatted exception text" );
			for ( size_t i = 0 ; i < 32 ; ++i ) {
				GT_ASSERT( error.what() == first, "what allocated another unowned message" );
				GT_ASSERT( std::string( first ) == expected, "what invalidated its earlier result" );
			}
			GT_OK();
		};
		const auto copied_error = check( *copied );
		GT_ASSERT( copied_error.empty(), "copied exception: " + copied_error );
		copied.reset();
		bool caught = false;
		try { std::rethrow_exception( thrown ); }
		catch ( const Exception& error ) {
			caught = true;
			const auto result = check( error ); GT_ASSERT( result.empty(), "retained exception: " + result );
			GT_ASSERT( error.ToString() == expected, "exception copy lost its structured diagnostic" );
		}
		GT_ASSERT( caught, "exception_ptr lost its concrete exception" );
		GT_OK();
	} );

	for ( const bool fail_in_member : { false, true } ) {
		for ( const bool catch_inside : { false, true } ) {
			task->AddTest(
				std::string( "test if GC cancels failed " ) + ( fail_in_member ? "member" : "derived" ) + " construction " + ( catch_inside ? "inside accumulation" : "escaping accumulation" ),
				GT( fail_in_member, catch_inside ) {
					std::atomic< size_t > discarded = 0, kept = 0;
					size_t sentinel_unwinds = 0, member_unwinds = 0, derived_destructions = 0;
					class TrackedObject : public gc::Object {
					public:
						TrackedObject( gc::Space* space, std::atomic< size_t >& destructions )
							: gc::Object( space ), destructions( destructions ) {}
						~TrackedObject() override { destructions++; }
					private:
						std::atomic< size_t >& destructions;
					};
					struct Sentinel {
						size_t& unwinds;
						~Sentinel() { unwinds++; }
					};
					struct Member {
						Member( gc::Space* space, std::atomic< size_t >& discarded, size_t& unwinds, const bool fail )
							: unwinds( unwinds ) {
							// A completed child remains GC-owned even when its parent's construction fails.
							new TrackedObject( space, discarded );
							if ( fail ) { throw std::runtime_error( "member construction failure" ); }
						}
						~Member() { unwinds++; }
						size_t& unwinds;
					};
					class FailingObject : public gc::Object {
					public:
						FailingObject( gc::Space* space, std::atomic< size_t >& discarded, size_t& sentinel_unwinds, size_t& member_unwinds, size_t& destructions, const bool fail_in_member )
							: gc::Object( space ), sentinel{ sentinel_unwinds }, member( space, discarded, member_unwinds, fail_in_member ), destructions( destructions ) {
							throw std::runtime_error( "derived construction failure" );
						}
						~FailingObject() override { destructions++; }
					private:
						Sentinel sentinel;
						Member member;
						size_t& destructions;
					};
					class Root : public gc::Object {
					public:
						Root() : gc::Object( nullptr ) {}
						gc::Object* child = nullptr;
						void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override {
							gc::Object::GetReachableObjects( reachable_objects );
							if ( child ) { GC_REACHABLE( child ); }
						}
					} root;
					gc::Space space( &root );
					space.SetThreadId( std::this_thread::get_id() );
					util::FinallyGuard release_root( [ & ]() {
						space.Accumulate( &root, [ & ]() { root.child = nullptr; } );
					} );
					bool caught = false, still_accumulating = !catch_inside;
					const auto record_error = [ & ]( const std::runtime_error& error ) {
						caught = std::string( error.what() ) == ( fail_in_member ? "member construction failure" : "derived construction failure" );
					};
					const auto fail = [ & ]() {
						new FailingObject( &space, discarded, sentinel_unwinds, member_unwinds, derived_destructions, fail_in_member );
					};
					try {
						space.Accumulate( &root, [ & ]() {
							root.child = new TrackedObject( &space, kept );
							new TrackedObject( &space, discarded );
							if ( catch_inside ) {
								try { space.Accumulate( &root, fail ); }
								catch ( const std::runtime_error& error ) { record_error( error ); }
								still_accumulating = space.IsAccumulating();
							}
							else { fail(); }
						} );
					}
					catch ( const std::runtime_error& error ) { record_error( error ); }
					GT_ASSERT( caught && still_accumulating && !space.IsAccumulating(), "construction failure changed exception or accumulation scope" );
					GT_ASSERT( sentinel_unwinds == 1 && member_unwinds == ( fail_in_member ? 0u : 1u ) && derived_destructions == 0, "construction failure did not unwind exactly once" );
					g_engine->GetGC()->Iterate();
					GT_ASSERT( discarded == 2 && kept == 0, "construction failure lost completed children or a rooted object" );
					space.Accumulate( &root, [ & ]() {
						new TrackedObject( &space, discarded );
						root.child = nullptr;
					} );
					g_engine->GetGC()->Iterate();
					GT_ASSERT( discarded == 3 && kept == 1 && derived_destructions == 0, "subsequent collection duplicated or lost destruction" );
					GT_OK();
				}
			);
		}
	}

	for ( const bool nested : { false, true } ) {
		task->AddTest(
			nested ? "test if nested native exceptions restore GC accumulation" : "test if native exceptions restore GC accumulation",
			GT( nested ) {
				std::atomic< size_t > discarded = 0;
				std::atomic< size_t > kept = 0;
				class TrackedObject : public gc::Object {
				public:
					TrackedObject( gc::Space* space, std::atomic< size_t >& destructions )
						: gc::Object( space ), destructions( destructions ) {}
					~TrackedObject() override { destructions++; }
				private:
					std::atomic< size_t >& destructions;
				};
				class Root : public gc::Object {
				public:
					Root() : gc::Object( nullptr ) {}
					gc::Object* child = nullptr;
					void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override {
						gc::Object::GetReachableObjects( reachable_objects );
						if ( child ) { GC_REACHABLE( child ); }
					}
				} root;
				gc::Space space( &root );
				space.SetThreadId( std::this_thread::get_id() );
				util::FinallyGuard release_root( [ & ]() {
					space.Accumulate( &root, [ & ]() { root.child = nullptr; } );
				} );
				bool caught = false;
				try {
					space.Accumulate( &root, [ & ]() {
						root.child = new TrackedObject( &space, kept );
						new TrackedObject( &space, discarded );
						const auto fail = [ & ]() {
							new TrackedObject( &space, discarded );
							throw std::runtime_error( "native accumulation failure" );
						};
						if ( nested ) { space.Accumulate( &root, fail ); }
						else { fail(); }
					} );
				}
				catch ( const std::runtime_error& error ) {
					caught = std::string( error.what() ) == "native accumulation failure";
				}
				GT_ASSERT( caught, "native exception was not preserved" );
				GT_ASSERT( !space.IsAccumulating(), "native exception left GC accumulation active" );
				g_engine->GetGC()->Iterate();
				GT_ASSERT( discarded == 2 && kept == 0, "failed accumulation did not commit objects for normal collection" );
				space.Accumulate( &root, [ & ]() {
					new TrackedObject( &space, discarded );
					root.child = nullptr;
				} );
				GT_ASSERT( !space.IsAccumulating(), "subsequent accumulation stayed active" );
				g_engine->GetGC()->Iterate();
				GT_ASSERT( discarded == 3 && kept == 1, "subsequent accumulation or root release was not collected" );
				GT_OK();
			}
		);
	}

	task->AddTest(
		"test if event handler return validation handles missing values",
		GT() {
			class EventSource : public Wrappable {
				Value* const Wrap( GSE_CALLABLE ) override { return VALUE( value::Undefined ); }
			} source;
			auto* gc_space = gse->GetGCSpace();
			std::string missing_error, wrong_type_error;
			bool valid_result = false, unconstrained_result = false;
			gc_space->Accumulate( nullptr, [ & ]() {
				ExecutionPointer ep;
				Value* returned = nullptr;
				source.On( gc_space, ctx, {}, ep, "event", NATIVE_CALL( &returned ) { return returned; } ) );
				auto* args = VALUEEXT( value::Object, gc_space, ctx, {}, ep, value::object_properties_t{} );
				try {
					source.Trigger( gc_space, ctx, {}, ep, "event", args, VT_BOOL );
				}
				catch ( const Exception& e ) {
					if ( e.class_name == EC.INVALID_HANDLER ) { missing_error = e.reason; }
				}
				auto* result = source.Trigger( gc_space, ctx, {}, ep, "event", args );
				unconstrained_result = result->type == VT_BOOL && !( (value::Bool*)result )->value;
				returned = VALUE( value::Int, , 7 );
				try {
					source.Trigger( gc_space, ctx, {}, ep, "event", args, VT_BOOL );
				}
				catch ( const Exception& e ) {
					if ( e.class_name == EC.INVALID_HANDLER ) { wrong_type_error = e.reason; }
				}
				returned = VALUE( value::Bool, , true );
				valid_result = source.Trigger( gc_space, ctx, {}, ep, "event", args, VT_BOOL ) == returned;
				source.ClearHandlers();
			} );
			GT_ASSERT( missing_error == "Event handler is expected to return Bool, got no value", + missing_error );
			GT_ASSERT( wrong_type_error == "Event handler is expected to return Bool, got Int: 7", + wrong_type_error );
			GT_ASSERT( valid_result && unconstrained_result, "valid or unconstrained handler result changed" );
			GT_OK();
		}
	);

	task->AddTest(
		"test if GSE teardown excludes concurrent garbage collection",
		GT() {
			std::atomic< bool > stop = false;
			std::atomic< size_t > collections = 0;
			// Exercise the production collector between every lifecycle, without
			// waiting for the normal one-second collection interval.
			std::thread collector( [ &stop, &collections ]() {
				while ( !stop.load() ) {
					g_engine->GetGC()->Iterate();
					collections++;
					std::this_thread::yield();
				}
			} );
			util::FinallyGuard join_collector( [ &stop, &collector ]() {
				stop = true;
				collector.join();
			} );

			for ( size_t cycle = 0 ; cycle < 32 ; cycle++ ) {
				GSE nested;
				auto* space = nested.GetGCSpace();
				space->Accumulate( &nested, [ &nested, space ]() {
					ExecutionPointer ep;
					for ( size_t i = 0 ; i < 16 ; i++ ) {
						auto* context = nested.CreateGlobalContext();
						context->CreateVariable( "kept", new value::Int( space, i ), {}, ep );
					}
					auto* context = nested.CreateGlobalContext();
					nested.RunScript( space, context, {}, ep, "./GLSMAC_data/tests/_gc_lifecycle" );
				} );
				const auto before = collections.load();
				while ( collections.load() == before ) {
					std::this_thread::yield();
				}
				// Roots, cached include results and their contexts must remain
				// stable until an in-flight mark has finished.
			}
			GT_ASSERT( collections.load() >= 32, "collector did not exercise every lifecycle" );
			GT_OK();
		}
	);

	task->AddTest(
		"test if GSE timer drain excludes collection between callbacks",
		GT() {
			std::atomic< bool > draining = false;
			std::atomic< bool > collector_attempted = false;
			std::atomic< size_t > marks_during_drain = 0;
			std::atomic< size_t > executions = 0;
			std::atomic< size_t > destructions = 0;
			class DrainTimer : public value::Callable {
			public:
				DrainTimer( gc::Space* space, context::Context* context, std::atomic< bool >& draining,
					std::atomic< bool >& collector_attempted, std::atomic< size_t >& marks,
					std::atomic< size_t >& executions, std::atomic< size_t >& destructions )
					: value::Callable( space, context ), draining( draining ), collector_attempted( collector_attempted ),
					  marks( marks ), executions( executions ), destructions( destructions ) {}
				~DrainTimer() override { destructions++; }
				void GetReachableObjects( std::unordered_set< gc::Object* >& reachable_objects ) override {
					if ( draining.load() ) {
						marks++;
					}
					value::Callable::GetReachableObjects( reachable_objects );
				}
				Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
					if ( ++executions == 1 ) {
						draining = true;
						while ( !collector_attempted.load() ) {
							std::this_thread::yield();
						}
						// ProcessTimers updates its maps and ProcessAndExit waits here
						// outside the callback's own Accumulate. The outer teardown
						// accumulation must protect both that gap and the next callback.
						return VALUE( value::Int, , 100 );
					}
					return VALUE( value::Bool, , false );
				}
			private:
				std::atomic< bool >& draining;
				std::atomic< bool >& collector_attempted;
				std::atomic< size_t >& marks;
				std::atomic< size_t >& executions;
				std::atomic< size_t >& destructions;
			};
			GSE* nested = nullptr;
			std::thread owner( [ & ]() {
				nested = new GSE();
				auto* space = nested->GetGCSpace();
				space->Accumulate( nested, [ & ]() {
					ExecutionPointer ep;
					auto* context = nested->CreateGlobalContext();
					auto* timer = new DrainTimer( space, context, draining, collector_attempted, marks_during_drain, executions, destructions );
					nested->GetAsync()->CreateTimer( 0, timer, context, {}, ep );
				} );
			} );
			owner.join();
			std::thread collector( [ & ]() {
				while ( !draining.load() ) {
					std::this_thread::yield();
				}
				collector_attempted = true;
				g_engine->GetGC()->Iterate();
			} );
			util::FinallyGuard join_collector( [ & ]() { collector.join(); } );
			delete nested;
			GT_ASSERT( executions.load() == 2, "pending repeat timer was not fully drained" );
			GT_ASSERT( destructions.load() == 1, "drained timer was not collected" );
			GT_ASSERT( marks_during_drain.load() == 0, "collector traversed timer state while teardown was draining it" );
			GT_OK();
		}
	);

	for ( const bool queue_callback : { false, true } ) {
		task->AddTest(
			queue_callback
				? "test if cross-thread GSE teardown releases pending callback owners"
				: "test if cross-thread GSE teardown drains timers and clears roots",
			GT( queue_callback ) {
				std::atomic< size_t > destructions = 0;
				std::atomic< size_t > executions = 0;
				class TrackedCallable : public value::Callable {
				public:
					TrackedCallable( gc::Space* space, context::Context* context, std::atomic< size_t >& destructions, std::atomic< size_t >& executions )
						: value::Callable( space, context ), destructions( destructions ), executions( executions ) {}
					~TrackedCallable() override { destructions++; }
					Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
						executions++;
						return VALUE( value::Bool, , false );
					}
				private:
					std::atomic< size_t >& destructions;
					std::atomic< size_t >& executions;
				};
				GSE* nested = nullptr;
				TrackedCallable* tracked = nullptr;
				std::thread owner( [ & ]() {
					nested = new GSE();
					auto* space = nested->GetGCSpace();
					space->Accumulate( nested, [ & ]() {
						ExecutionPointer ep;
						auto* context = nested->CreateGlobalContext();
						nested->RunScript( space, context, {}, ep, "./GLSMAC_data/tests/_gc_lifecycle" );
						tracked = new TrackedCallable( space, context, destructions, executions );
						nested->AddRootObject( tracked );
						nested->GetAsync()->CreateTimer( 0, tracked, context, {}, ep );
					} );
				} );
				owner.join(); // The original owner must stop using the space before destruction.
				bool callback_ran = false;
				size_t cleanups = 0;
				if ( queue_callback ) {
					nested->GetGCSpace()->Accumulate( tracked, [ & ]() { callback_ran = true; }, [ & ]() { cleanups++; } );
				}
				delete nested;
				GT_ASSERT( executions.load() == 1, "pending timer did not run exactly once during teardown" );
				GT_ASSERT( destructions.load() == 1, "rooted callback was not collected during teardown" );
				GT_ASSERT( !callback_ran, "queued accumulation must not run during teardown" );
				GT_ASSERT( cleanups == ( queue_callback ? 1 : 0 ), "queued callback cleanup did not run exactly once" );
				GT_OK();
			}
		);
	}

	class Sum : public value::Callable {
	public:
		Sum( gc::Space* const gc_space, context::Context* const ctx )
			: value::Callable( gc_space, ctx ) {}
		Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
			int64_t result = 0;
			for ( const auto& it : arguments ) {
				const auto arg = it;
				switch ( arg->type ) {
					case VT_INT: {
						result += ( (value::Int*)arg )->value;
						break;
					}
					default: {
						// TODO: exception
					}
				}
			}
			return VALUE( value::Int, , result );
		}
	};

	task->AddTest(
		"test if modules get assigned and run correctly",
		GT() {
			auto* gc_space = gse->GetGCSpace();

			static std::string modules_run_order = "";

			class TestModuleY : public value::Callable {
			public:
				TestModuleY( gc::Space* const gc_space, context::Context* const ctx )
					: Callable( gc_space, ctx ) {}
				Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
					modules_run_order += 'Y';
					return VALUE( value::Null );
				}
			};
			gc_space->Accumulate(
				nullptr,
				[ &gc_space, &ctx, &gse ]() {
					NEWV( test_module_y, TestModuleY, gc_space, ctx );
					gse->AddModule( "test_module_y", test_module_y );
				}
			);

			class TestModuleX : public value::Callable {
			public:
				TestModuleX( gc::Space* const gc_space, context::Context* const ctx )
					: Callable( gc_space, ctx ) {}
				Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
					modules_run_order += 'X';
					return VALUE( value::Null );
				}
			};
			gc_space->Accumulate(
				nullptr,
				[ &gc_space, &ctx, &gse ]() {
					NEWV( test_module_x, TestModuleX, gc_space, ctx );
					gse->AddModule( "test_module_x", test_module_x );
				}
			);

			gse->Run();

			GT_ASSERT( modules_run_order == "YX", "wrong modules order" );

			GT_OK();
		}
	);

	task->AddTest(
		"test if variables are written and read correctly",
		GT() {
			auto* gc_space = gse->GetGCSpace();

			class SetVariables : public value::Callable {
			public:
				SetVariables( gc::Space* const gc_space, GSE* gse, context::Context* const ctx )
					: value::Callable( gc_space, ctx )
					, gse( gse ) {}
				GSE* gse;
				Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
					gse->SetGlobal( "testvar_null", VALUE( value::Null ) );
					gse->SetGlobal( "testvar_bool_first", VALUE( value::Bool, , true ) );
					gse->SetGlobal( "testvar_bool_second", VALUE( value::Bool, , false ) );
					gse->SetGlobal( "testvar_int_first", VALUE( value::Int, , 3 ) );
					gse->SetGlobal( "testvar_int_second", VALUE( value::Int, , 7 ) );
					gse->SetGlobal( "testvar_int_third", VALUE( value::Int, , 25 ) );
					gse->SetGlobal( "testvar_string_first", VALUE( value::String, , "FIRST" ) );
					gse->SetGlobal( "testvar_string_second", VALUE( value::String, , "SECOND" ) );

					return VALUE( value::Null );
				}
			};
			gc_space->Accumulate(
				nullptr,
				[ &gc_space, &ctx, &gse ]() {
					NEWV( set_variables, SetVariables, gc_space, gse, ctx );
					gse->AddModule( "set_variables", set_variables );
				}
			);

			static std::string errmsg = "";
			class CheckVariables : public value::Callable {
			public:
				CheckVariables( gc::Space* const gc_space, GSE* gse, context::Context* const ctx )
					: value::Callable( gc_space, ctx )
					, gse( gse ) {}
				GSE* gse;
				Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
					const auto validate = [ this ]() -> const std::string {
						Value* t;

#define CHECK( _varname, _expected_type ) \
                        t = gse->GetGlobal( _varname ); \
                        GT_ASSERT( t->type == _expected_type, "unexpected value type" );
#define CHECKV( _varname, _type, _expected_value ) \
                        CHECK( _varname, value::_type::GetType() ); \
                        GT_ASSERT( ((value::_type*)t)->value == _expected_value, "unexpected value" );

						CHECK( "testvar_undefined1", VT_UNDEFINED );
						CHECK( "testvar_undefined2", VT_UNDEFINED );
						CHECK( "testvar_undefined3", VT_UNDEFINED );
						CHECK( "testvar_null", VT_NULL );
						CHECKV( "testvar_bool_first", Bool, true );
						CHECKV( "testvar_bool_second", Bool, false );
						CHECKV( "testvar_int_first", Int, 3 );
						CHECKV( "testvar_int_second", Int, 7 );
						CHECKV( "testvar_int_third", Int, 25 );
						CHECKV( "testvar_string_first", String, "FIRST" );
						CHECKV( "testvar_string_second", String, "SECOND" );
#undef CHECKV
#undef CHECK
						GT_OK();
					};
					errmsg = validate();
					return VALUE( value::Null );
				}
			};
			gc_space->Accumulate(
				nullptr,
				[ &gc_space, &ctx, &gse ]() {
					NEWV( check_variables, CheckVariables, gc_space, gse, ctx );
					gse->AddModule( "check_variables", check_variables );
				}
			);

			gse->Run();

			return errmsg;

		}
	);

	task->AddTest(
		"test if methods are defined and called correctly",
		GT() {
			auto* gc_space = gse->GetGCSpace();

			static bool wasTestMethodCalled = false;

			class SetMethods : public value::Callable {
			public:
				SetMethods( gc::Space* const gc_space, GSE* gse, context::Context* const ctx )
					: value::Callable( gc_space, ctx )
					, gse( gse ) {}
				GSE* gse;

				class TestMethod : public value::Callable {
				public:
					TestMethod( gc::Space* const gc_space, context::Context* const ctx )
						: value::Callable( gc_space, ctx ) {}
					Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
						wasTestMethodCalled = true;
						return VALUE( value::Null );
					}
				};

				Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {
					gse->SetGlobal( "test_method", VALUE( TestMethod, , ctx ) );
					gse->SetGlobal( "sum", VALUE( Sum, , ctx ) );
					return VALUE( value::Null );
				}
			};
			gc_space->Accumulate(
				nullptr,
				[ &gc_space, &ctx, &gse ]() {
					NEWV( set_methods, SetMethods, gc_space, gse, ctx );
					gse->AddModule( "set_methods", set_methods );
				}
			);

			gse->Run();

#define GETMETHOD( _method ) \
    auto* value = gse->GetGlobal( _method ); \
    GT_ASSERT( value->type != VT_UNDEFINED, "method not found" ); \
    GT_ASSERT( value->type == VT_CALLABLE, "method is not callable" ); \
    auto* method = (value::Callable*)value;

#define GETRESULT( _args, _expected_type, _cast_type ) \
    ExecutionPointer ep; \
    auto resultvalue = method->Run( gc_space, nullptr, {}, ep, _args ); \
    auto* resultv = resultvalue; \
    GT_ASSERT( resultv->type == _expected_type, "wrong result type ( " + std::to_string( resultv->type ) + " != " + std::to_string( _expected_type ) + " )" ); \
    auto* result = (_cast_type*)resultv;

#define CHECKRESULT( _args, _expected_type, _cast_type, _expected_value ) \
    { \
        GETRESULT( _args, _expected_type, _cast_type ); \
        GT_ASSERT( result->value == _expected_value, "wrong result value: " + std::to_string( result->value ) ); \
    }

			const auto& run_tests = [ &gse, &gc_space ]() -> std::string {
				{ // test test_method()
					GETMETHOD( "test_method" );
					GT_ASSERT( !wasTestMethodCalled, "method should have not be called yet" );
					GETRESULT( {}, VT_NULL, value::Null );
					GT_ASSERT( wasTestMethodCalled, "method should have been called" );
				}
				{ // test sum()
					GETMETHOD( "sum" );
					CHECKRESULT( {}, VT_INT, value::Int, 0 );
					CHECKRESULT( { VALUE( value::Int, , 3 ) }, VT_INT, value::Int, 3 );
					std::vector< Value* > args = {
						VALUE( value::Int, , 2 ),
						VALUE( value::Int, , 5 ),
					};
					CHECKRESULT( args, VT_INT, value::Int, 7 );
					args = {
						VALUE( value::Int, , 11 ),
						VALUE( value::Int, , 22 ),
						VALUE( value::Int, , 33 ),
					};
					CHECKRESULT( args, VT_INT, value::Int, 66 );
				}
				GT_OK();
			};

#undef GETMETHOD
#undef GETRESULT
#undef CHECKRESULT

			std::string result = "";
			gc_space->Accumulate(
				nullptr,
				[ &run_tests, &result ]() {
					result = run_tests();
				}
			);

			return result;
		}
	);

	task->AddTest(
		"test if variables are assigned and reassigned correctly",
		GT() {
			auto* gc_space = gse->GetGCSpace();

			class SetVariables : public value::Callable {
			public:
				SetVariables( gc::Space* const gc_space, GSE* gse, context::Context* const ctx )
					: value::Callable( gc_space, ctx )
					, gse( gse ) {}
				GSE* gse;
				Value* Run( GSE_CALLABLE, const value::function_arguments_t& arguments ) override {

					auto val1 = VALUE( value::Int, , 1 );
					auto val2 = VALUE( value::Int, , VALUE_GET( value::Int, val1 ) + 1 );

					gse->SetGlobal( "testvar_first", val1 );
					gse->SetGlobal( "testvar_second", val2 );

					VALUE_SET( value::Int, val1, 10 ); // this should update testvar_first

					auto val3 = VALUE_CLONE( value::Int, val2 );
					VALUE_SET( value::Int, val3, 20 ); // this should not update testvar_second
					gse->SetGlobal( "testvar_third", val3 );

					gse->SetGlobal( "testvar_obj1", VALUEEXT( value::Object, GSE_CALL ) );

					auto obj2 = VALUEEXT( value::Object, GSE_CALL );
					auto data2 = VALUE_DATA( value::Object, obj2 );
					data2->Set( "property_int", VALUE( value::Int, , 555 ), GSE_CALL );
					data2->Set( "property_bool", VALUE( value::Bool, , false ), GSE_CALL );
					data2->Set( "property_bool", VALUE( value::Bool, , true ), GSE_CALL ); // this should overwrite previous value
					gse->SetGlobal( "testvar_obj2", obj2 );

					auto properties = value::object_properties_t{
						{ "property_int", val3 },
						{ "property_sum",    VALUE( Sum, , ctx ) },
						{ "property_string", VALUE( value::String, , "STRING" ) },
					};
					gse->SetGlobal( "testvar_obj3", VALUEEXT( value::Object, GSE_CALL, properties ) );

					VALUE_SET( value::Int, val3, 30 ); // this should update testvar_third and testvar_obj3.property_int

					return VALUE( value::Null );
				}
			};

			std::string result = "";

			gc_space->Accumulate(
				nullptr,
				[ &gc_space, &ctx, &gse, &result ]() {
					NEWV( set_variables, SetVariables, gc_space, gse, ctx );
					gse->AddModule( "set_variables", set_variables );

					gse->Run();

					const auto f_asserts = [ &gse, &gc_space ]() {

						GT_ASSERT( VALUE_GET( value::Int, gse->GetGlobal( "testvar_first" ) ) == 10 );
						GT_ASSERT( VALUE_GET( value::Int, gse->GetGlobal( "testvar_second" ) ) == 2 );
						GT_ASSERT( VALUE_GET( value::Int, gse->GetGlobal( "testvar_third" ) ) == 30 );

						const auto obj1 = VALUE_GET( value::Object, gse->GetGlobal( "testvar_obj1" ) );
						GT_ASSERT( obj1.empty() );

						const auto obj2 = VALUE_GET( value::Object, gse->GetGlobal( "testvar_obj2" ) );
						GT_ASSERT( obj2.size() == 2 );
						GT_ASSERT( VALUE_GET( value::Int, obj2.at( "property_int" ) ) == 555 );
						GT_ASSERT( VALUE_GET( value::Bool, obj2.at( "property_bool" ) ) == true );

						const auto obj3 = VALUE_GET( value::Object, gse->GetGlobal( "testvar_obj3" ) );
						GT_ASSERT( obj3.size() == 3 );
						GT_ASSERT( VALUE_GET( value::Int, obj3.at( "property_int" ) ) == 30 );
						GT_ASSERT( VALUE_GET( value::String, obj3.at( "property_string" ) ) == "STRING" );
						const auto sum = VALUE_DATA( value::Callable, obj3.at( "property_sum" ) );

						const auto run_tests = [ &gse, &gc_space, &sum ]() -> const std::string {
							std::vector< Value* > args = {
								VALUE( value::Int, , 2 ),
								VALUE( value::Int, , 5 ),
							};
							{
								ExecutionPointer ep;
								GT_ASSERT( VALUE_GET( value::Int, sum->Run( gc_space, nullptr, {}, ep, args ) ) == 7 );
							}
							GT_OK();
						};

						return run_tests();
					};

					result = f_asserts();

				}
			);
			return result;
		}
	);

}

}
}
