#pragma once

#include "gse/Value.h"
#include <array>
#include <cstdint>

namespace gse {
class GCWrappable;
namespace value {
class Object;
}
}

namespace game {
namespace backend {

namespace resource {
class ResourceManager;
}

class ResourceRelated {
protected:
	// Explicit SMAC N/M/E port resource identities; other defined resource fields
	// retain the callback API's zero-fill behavior. No display-name inference.
	gse::value::Object* GetSmacResourceValues( GSE_CALLABLE, resource::ResourceManager* rm, const std::array< int64_t, 3 >& values );
	gse::value::Object* const GetResourcesFromCallback( GSE_CALLABLE, gse::GCWrappable* const object, resource::ResourceManager* const rm, const std::string& event, const gse::f_args_t& f_args );

};

}
}
