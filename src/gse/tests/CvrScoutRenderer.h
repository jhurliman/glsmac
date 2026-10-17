#pragma once
#include <string>

namespace task::gsetests { class GSETests; }
namespace gse::tests { void AddCvrScoutRendererTests( task::gsetests::GSETests* task ); }
namespace gse::tests { std::string MakeScoutTestAsset( bool reactor ); }
