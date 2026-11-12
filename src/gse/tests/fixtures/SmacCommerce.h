#pragma once
#include "game/backend/base/SmacCommerce.h"
namespace gse::tests::smac_commerce_fixture {
using namespace game::backend::base;
struct Case {const char* id;SmacCommerceInputs input;std::array<int32_t,8> bonus{},economy{},known{},ratings{},incoming{},quotes{};std::array<int32_t,8> partners{};size_t rank=0;int32_t total=0;};
inline std::vector<Case> Cases() {std::vector<Case> result;
{Case c;c.id="COMMERCE-001";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-002";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,-1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-003";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[2].treaty_partners|=1u<<1;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-004";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.input.factions[1].pact_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,1,0,0,0,0,0};c.quotes={0,0,1,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=1;result.push_back(c);}
{Case c;c.id="COMMERCE-005";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[2].treaty_partners|=1u<<1;
c.input.factions[2].pact_partners|=1u<<1;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-006";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].pact_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,-1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-007";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=true;
c.input.current=0;c.input.bases={{1,1},{2,1}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.input.factions[1].pact_partners|=1u<<2;
c.bonus[1]=2;c.economy[1]=0;c.known[1]=0;c.input.factions[1].blocked=false;
c.ratings={0,2,0,0,0,0,0,0};c.incoming={0,0,1,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=1;result.push_back(c);}
{Case c;c.id="COMMERCE-008";
c.input.enabled_technologies=6;c.input.governor=1;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,1,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=1;result.push_back(c);}
{Case c;c.id="COMMERCE-009";
c.input.enabled_technologies=6;c.input.governor=2;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,1,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-010";
c.input.enabled_technologies=6;c.input.governor=1;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.bonus[1]=0;c.economy[1]=0;c.known[1]=0;c.input.factions[1].blocked=true;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-011";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.bonus[2]=0;c.economy[2]=0;c.known[2]=0;c.input.factions[2].blocked=true;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-012";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.bonus[1]=4;c.economy[1]=0;c.known[1]=0;c.input.factions[1].blocked=false;
c.bonus[2]=1;c.economy[2]=0;c.known[2]=0;c.input.factions[2].blocked=false;
c.ratings={0,4,1,0,0,0,0,0};c.incoming={0,0,2,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=2;result.push_back(c);}
{Case c;c.id="COMMERCE-013";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.bonus[1]=1;c.economy[1]=5;c.known[1]=6;c.input.factions[1].blocked=false;
c.ratings={0,10,0,0,0,0,0,0};c.incoming={0,0,4,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=4;result.push_back(c);}
{Case c;c.id="COMMERCE-014";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.bonus[1]=0;c.economy[1]=9;c.known[1]=0;c.input.factions[1].blocked=false;
c.ratings={0,3,0,0,0,0,0,0};c.incoming={0,0,1,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=1;result.push_back(c);}
{Case c;c.id="COMMERCE-015";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{1,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,-1,-1,-1,-1,-1,-1};c.rank=1;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-016";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,2,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-017";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{1,40},{2,100},{2,1}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,3,-1,-1,-1,-1,-1};c.rank=1;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-018";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{1,40},{2,100}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,-1,-1,-1,-1,-1,-1};c.rank=1;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-019";
c.input.enabled_technologies=6;c.input.governor=1;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,-1,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-020";
c.input.enabled_technologies=6;c.input.governor=1;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,0},{2,0}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,1,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,1,-1,-1,-1,-1,-1};c.rank=0;c.total=1;result.push_back(c);}
{Case c;c.id="COMMERCE-021";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{3,999},{2,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,0,0,0,0,0,0};c.quotes={0,0,0,0,0,0,0,0};
c.partners={-1,-1,2,-1,-1,-1,-1,-1};c.rank=0;c.total=0;result.push_back(c);}
{Case c;c.id="COMMERCE-022";
c.input.enabled_technologies=6;c.input.governor=0;c.input.trade_doubled=false;
c.input.current=0;c.input.bases={{1,20},{2,20},{3,30}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[1].treaty_partners|=1u<<2;
c.input.factions[1].pact_partners|=1u<<2;
c.input.factions[1].treaty_partners|=1u<<3;
c.input.factions[1].pact_partners|=1u<<3;
c.ratings={0,0,0,0,0,0,0,0};c.incoming={0,0,1,1,0,0,0,0};c.quotes={0,0,1,1,0,0,0,0};
c.partners={-1,-1,1,2,-1,-1,-1,-1};c.rank=0;c.total=2;result.push_back(c);}
{Case c;c.id="COMMERCE-023";
c.input.enabled_technologies=6;c.input.governor=7;c.input.trade_doubled=false;
c.input.current=1;c.input.bases={{2,20},{7,20}};
for(const auto& b:c.input.bases) {++c.input.factions[b.owner].base_count;}
c.input.factions[7].treaty_partners|=1u<<2;
c.input.factions[7].pact_partners|=1u<<2;
c.bonus[7]=2;c.economy[7]=3;c.known[7]=2;c.input.factions[7].blocked=false;
c.ratings={0,0,0,0,0,0,0,5};c.incoming={0,0,5,0,0,0,0,0};c.quotes={0,0,1,0,0,0,0,0};
c.partners={-1,-1,0,-1,-1,-1,-1,-1};c.rank=0;c.total=5;result.push_back(c);}
return result;}
}
