#pragma once
#include <array>
#include <cstdint>
// Literal original capture/restart outputs: RE-SMAC-SUCCESSFUL-CAPTURE-RESTART-001.
// Semantic JSON SHA256: 140d10e333ad0c2105be511269c49bc356d978b7b1348351d9b9a3c89cb0ed0b
namespace gse::tests::smac_capture_restart_fixture {
inline constexpr uint32_t final_crt=1632431389u;
inline constexpr std::array<int32_t,2> site{{44,8}};
inline constexpr std::array<std::array<int32_t,2>,9> reveal{{{{44,8}},{{45,7}},{{46,8}},{{45,9}},{{44,10}},{{43,9}},{{42,8}},{{43,7}},{{44,6}}}};
inline constexpr std::array<uint8_t,4> visibility{{0,0,2,0}};
}
