#pragma once
#include <string>
#include <cstdint>
#include <vector>

namespace task::gsetests { class GSETests; }
namespace gse::tests { void AddCvrScoutRendererTests( task::gsetests::GSETests* task ); }
namespace gse::tests { std::string MakeScoutTestAsset( bool reactor ); }
namespace gse::tests { std::string MakeScoutTestAssetWithGeometry( const std::vector< uint8_t >& geometry ); }
