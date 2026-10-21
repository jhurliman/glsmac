#pragma once
#include <array>
namespace live_ground_render_fixture {
struct Case {const char* name;int owner,chassis,weapon,armor,reactor,facing,zoom;std::array<int,2> anchor,target;int kernels;const char* color;const char* depth;};
inline constexpr std::array<Case,9> Cases{{
    {"scout_map",1,0,0,0,1,0,0,{128,128},{178,178},65,"850dc4daadd5c4a0f13ea7c4832397de7b5153e384240bf3c05cf297b735a2b5","ce09c6e31936e8918dbeeae07297c3ec761e93279463d44c58fa2c7a3a0927e8"},
    {"guard_map",1,0,0,1,1,0,0,{128,128},{178,178},66,"4d6a0bec62d04cd9877b56df6ad482d3220d5d53626de01654714cb3662c1e75","bbbe87d01600dd7b9d046d94720c0c9bf44d9fddce414d3d65280d9e197de4be"},
    {"laser_guard_map",1,0,1,1,1,0,0,{128,128},{178,178},71,"a9a4c993e02a602bd1e869544d67cdc0d637e27c9328eb2149f73fc28fd49ac5","1df378e9265de56fdd90fbefd1dd582ab04cf3e15e9e518d9d8aa6f236db2808"},
    {"former_map",1,0,15,0,1,0,0,{128,128},{178,178},33,"90e43baf12a81807a673ef7fe91109c965eedc26369d1231f55334a6245d2d5b","ddfa5ced2aebc84e353712a1d24daa0518c0b4ddd59ee62d87b8695ef5fba252"},
    {"colony_map",1,0,14,0,1,0,0,{128,128},{178,178},20,"3e8be8e51f40bc698719baa003eed51a6c64e4623f2d9a768e3de5280981c547","58cfcde7f052634b8c506866c1b00ac26e1aace5075cdfda670a325cd5f8254b"},
    {"rover_map",1,1,0,0,1,0,0,{128,128},{178,178},24,"19a1036bce318185fe385a453ccc61390689c6cb8418601e62c7ea17e826f478","ec95b58c4521076b56dc2d48407ae8163543a8f77b26c58d9975f8a3f9322a13"},
    {"guard_selected",1,0,0,1,1,0,-4,{34,66},{71,103},66,"7d9579b90eec79736ea1a07f65f4d72d9cdcd2149dab82084d06777c1de65a21","aa2f3b29bbd340267117bbf367a0b90cb992a9cc57089e49d00881a88096f953"},
    {"guard_roster",1,0,0,1,1,0,-8,{156,197},{181,222},66,"969ff33d8ee7f147e8416adf1ac4b73daf08868d4b4658eb7eb8c0809badb66c","15cb1709296a4f7ab0d1bdee2085f98826bbb7a8861ddb46b434c0e806925a45"},
    {"laser_guard_gaian_owner2",2,0,1,1,1,0,0,{128,128},{178,178},71,"f39aa85c203f5a75e4d8d05e536ca4b3c23cad14a3f5dd3aca305b5908689b53","1df378e9265de56fdd90fbefd1dd582ab04cf3e15e9e518d9d8aa6f236db2808"},
}};
}
