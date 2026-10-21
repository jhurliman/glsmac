#pragma once
#include "game/backend/unit/SmacWorkshop.h"
namespace gse::tests::workshop_alias_fixture {
// Self-authored native input/output projections from complete441e20 receipts.
// Manifest870aa01c5b1cacc8ed8840801ad7125900cd3c60fecd41f0bc97d3996e71da67.
struct Case {const char* id;game::backend::unit::SmacWorkshopDraft draft;std::vector<game::backend::unit::SmacCustomDesign> before,after;int32_t selected;bool warning;};
inline std::vector<Case> Cases() {return {
{"match_reuse_named",{64,0,0,2,0,1,"Existing Guard"},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0},{70,0,0,2,1,2,2,0,0,0,true,false,std::string("Other Guard"),true,std::nullopt,0}},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0},{70,0,0,2,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0}},70,false},
{"match_reuse_auto_draft",{64,0,0,2,0,1,""},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),false,std::nullopt,0},{70,0,0,2,1,2,2,0,0,0,true,false,std::string("Other Guard"),true,std::nullopt,0}},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),false,std::nullopt,0},{70,0,0,2,1,2,2,0,0,0,true,false,std::string("Other Guard"),true,std::nullopt,0}},70,false},
{"match_builtin_name_warning",{64,0,0,0,0,1,"Existing Guard"},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0},{70,0,0,0,1,1,0,0,0,0,true,false,std::string("Other Guard"),true,std::nullopt,0}},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0},{70,0,0,0,1,1,0,0,0,0,true,false,std::string("Other Guard"),true,std::nullopt,0}},2,true},
{"same_tuple_named",{64,0,0,1,0,1,"New Guard"},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0}},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("New Guard"),true,std::nullopt,0}},64,false},
{"same_tuple_auto_ignores_direct_draft",{64,0,0,1,0,1,"New Guard"},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),false,std::nullopt,0}},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),false,std::nullopt,0}},64,false},
{"match_reuse_obsolete",{64,0,0,2,0,1,"Existing Guard"},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0},{70,0,0,2,1,2,2,0,0,2,true,false,std::string("Other Guard"),true,std::nullopt,0}},{{64,0,0,1,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,0},{70,0,0,2,1,2,2,0,0,0,true,false,std::string("Existing Guard"),true,std::nullopt,8}},70,false},
};}
}
