#include "GSE.h"

#include "task/gsetests/GSETests.h"
#include "gse/GSE.h"
#include "gse/Async.h"
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
#include <thread>

namespace gse {
namespace tests {

void AddGSETests( task::gsetests::GSETests* task ) {
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
