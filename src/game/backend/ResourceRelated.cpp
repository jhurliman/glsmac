#include "ResourceRelated.h"

#include "game/backend/resource/ResourceManager.h"

namespace game {
namespace backend {

gse::value::Object* ResourceRelated::GetSmacResourceValues( GSE_CALLABLE, resource::ResourceManager* rm, const std::array< int64_t, 3 >& values ) {
	const auto& definitions = rm->GetDefinedResources();
	const std::array< std::string, 3 > ids{ "NUTRIENTS", "MINERALS", "ENERGY" };
	for ( const auto& id : ids ) {
		if ( !definitions.count( id ) ) { GSE_ERROR( gse::EC.GAME_ERROR, "Canonical resource ID is not defined: " + id ); }
	}
	gse::value::object_properties_t result;
	for ( const auto& definition : definitions ) {
		int64_t amount = 0;
		for ( size_t i = 0; i < ids.size(); ++i ) { if ( definition.first == ids[ i ] ) { amount = values[ i ]; break; } }
		result.emplace( definition.first, VALUE( gse::value::Int,, amount ) );
	}
	return VALUE( gse::value::Object,, GSE_CALL_NOGC, result );
}

gse::value::Object* const ResourceRelated::GetResourcesFromCallback( GSE_CALLABLE, gse::GCWrappable* const object, resource::ResourceManager* const rm, const std::string& event, const gse::f_args_t& f_args ) {
	auto* const result = object->Trigger( GSE_CALL, event, f_args );
	ASSERT( result, "GetResources result is null" );
	if ( result->type != gse::VT_OBJECT ) {
		GSE_ERROR( gse::EC.GAME_ERROR, event + " must return object, got: " + result->ToString() );
	}
	const auto& defined_resources = rm->GetDefinedResources();
	auto* obj = ( (gse::value::Object*)result );
	auto& v = obj->value;
	for ( const auto& it : v ) {
		if ( defined_resources.find( it.first ) == defined_resources.end() ) {
			GSE_ERROR( gse::EC.GAME_ERROR, event + " returned unknown resource: " + it.first );
		}
		if ( it.second->type != gse::VT_INT ) {
			GSE_ERROR( gse::EC.GAME_ERROR, event + " returned invalid value for resource " + it.first + ", expected: Int, got " + it.second->GetTypeString() + ": " + it.second->ToString() );
		}
	}
	for ( const auto& it : defined_resources ) {
		if ( v.find( it.first ) == v.end() ) {
			obj->Assign( it.first, VALUE( gse::value::Int, , 0 ) );
		}
	}
	return obj;
}

}
}
