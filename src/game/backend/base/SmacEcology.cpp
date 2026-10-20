#include "SmacTerraformingPoints.h"
#include "SmacEcology.h"
#include "SmacEcologyOriginal.h"
#include "game/backend/SmacOriginalArithmetic.h"
#include "SmacEcologyNativeSpawn.h"
#include <algorithm>
#include <stdexcept>
#include <set>
#include <map>
#include <cstdlib>

namespace game::backend::base {
namespace {
int32_t Checked(int64_t value) {
    if(value<INT32_MIN || value>INT32_MAX) {throw std::overflow_error("SMAC ecology exceeds measured signed32 arithmetic");}
    return static_cast<int32_t>(value);
}
int32_t FloorEighth(int32_t value) {
    const auto quotient=value/8;
    return quotient-(value<0 && value%8!=0 ? 1 : 0);
}
}

SmacTerraformingEcology CalculateSmacTerraformingEcology(const SmacWorkerRadiusMap& map,
    bool tree_farm,bool hybrid_forest,const std::function<SmacEcologySquare(int32_t,int32_t)>& query) {
    if(map.width<8 || !query) {throw std::invalid_argument("Ecology requires a nonaliasing radius and square query");}
    int32_t total=0;
    for(int32_t i=0;i<21;++i) {
        const auto coordinate=SmacWorkedSquareCollection::Resolve(map,i);
        if(!coordinate) {continue;}
        const auto tile=query((*coordinate)[0],(*coordinate)[1]);
        const auto points=CalculateSmacTerraformingSquarePoints(tile,tree_farm,hybrid_forest);
        total+=points;
    }
    return {total,FloorEighth(total)};
}

static SmacEcologyBurden EcologyBurden(const SmacEcologyBurdenInput& in,bool original) {
    const auto narrow=[&](int64_t value){return original?SmacWrap32(value):Checked(value);};
    if(!original && in.history<0) {throw std::invalid_argument("Ecology requires nonnegative clean-mineral history");}
    const auto scaled=FloorEighth(in.raw_terraforming_points);
    auto residual=scaled;auto clean=narrow(int64_t(16)+in.history);
    if(residual>0) {const auto used=std::min(residual,clean);residual=narrow(int64_t(residual)-used);clean=narrow(int64_t(clean)-used);}
    const int32_t divisor=1+(in.nanoreplicator || in.singularity_inductor)+in.centauri_preserve+in.temple_of_planet+in.pholus_mutagen;
    const auto satellites=std::max(0,std::min(in.mining_satellites,in.population));
    const auto mineral_term=narrow(int64_t(in.processed_minerals)-satellites-clean)/divisor;
    return {scaled,residual,clean,divisor,satellites,narrow(int64_t(residual)+mineral_term)};
}

static SmacEcologyDamage EcologyDamage(const SmacEcologyBurden& burden,const SmacEcologyScalingInput& in,bool original) {
    const auto narrow=[&](int64_t value){return original?SmacWrap32(value):Checked(value);};
    if(burden.divisor<1 || burden.divisor>5 || (!original && (in.human_difficulty<0 || in.human_difficulty>5
        || in.global_difficulty<0 || in.global_difficulty>5 || in.native_life<0 || in.native_life>2 || in.major_atrocities<0))) {
        throw std::invalid_argument("Ecology scaling exceeds the measured input domain");
    }
    auto after_human=burden.burden;
    if(in.human) {
        const auto denominator=std::clamp(in.sea_level_offset,0,100)/std::clamp(in.sea_level_rises_rule,1,100)+burden.divisor;
        after_human=narrow(int64_t(after_human)+narrow(int64_t(5)*in.major_atrocities)/denominator);
    }
    auto doubled=std::max(0,after_human);
    if(in.voice_of_planet_exists && in.transcendence_victory) {doubled=narrow(int64_t(doubled)*2);}
    if(in.perihelion) {doubled=narrow(int64_t(doubled)*2);}
    const auto difficulty=in.human ? (in.human_difficulty==0 ? 2 : in.human_difficulty<4 ? 3 : 5) : std::clamp(narrow(int64_t(6)-in.global_difficulty),1,3);
    auto product=narrow(int64_t(in.technology_score)-in.future_technology_counter);
    product=narrow(int64_t(product)*(3-std::clamp(in.planet,-3,2)));
    product=narrow(int64_t(product)*narrow(int64_t(in.native_life)+1));
    product=narrow(int64_t(product)*doubled);
    product=narrow(int64_t(product)*difficulty);
    const auto sixth=product/6;
    return {after_human,doubled,difficulty,sixth,narrow(int64_t(sixth)+50)/100};
}
SmacEcologyBurden CalculateSmacEcologyBurden(const SmacEcologyBurdenInput& in) {return EcologyBurden(in,false);}
SmacEcologyBurden CalculateOriginalSmacEcologyBurden(const SmacEcologyBurdenInput& in) {return EcologyBurden(in,true);}
SmacEcologyDamage CalculateSmacEcologyDamage(const SmacEcologyBurden& burden,const SmacEcologyScalingInput& in) {return EcologyDamage(burden,in,false);}
SmacEcologyDamage CalculateOriginalSmacEcologyDamage(const SmacEcologyBurden& burden,const SmacEcologyScalingInput& in) {return EcologyDamage(burden,in,true);}

int32_t SmacEcologyResearchContribution(int32_t damage,int32_t current_planet) {
    return Checked(int64_t(4)*damage)/static_cast<int32_t>(std::clamp(int64_t(4)-current_planet,int64_t(1),int64_t(99)));
}

namespace {
constexpr uint32_t BLOOM_IMPROVEMENTS=SmacEcologySquare::Mine|SmacEcologySquare::Solar|SmacEcologySquare::Farm
    |SmacEcologySquare::SoilEnricher|SmacEcologySquare::Forest;
constexpr std::array<std::array<int32_t,2>,48> BLOOM_OFFSETS{{
    {1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2},
    {2,-2},{2,2},{-2,2},{-2,-2},{1,-3},{3,-1},{3,1},{1,3},{-1,3},{-3,1},{-3,-1},{-1,-3},
    {4,0},{-4,0},{0,4},{0,-4},
    {1,-5},{2,-4},{3,-3},{4,-2},{5,-1},{5,1},{4,2},{3,3},{2,4},{1,5},{-1,5},{-2,4},
    {-3,3},{-4,2},{-5,1},{-5,-1},{-4,-2},{-3,-3},{-2,-4},{-1,-5},{0,6},{6,0},{0,-6},{-6,0}
}};
int32_t EcologyCrt(const std::function<int32_t()>& draw) {
    const auto raw=draw();
    if(raw<0 || raw>32767) {throw std::runtime_error("Ecology requires a raw CRT draw in0..32767");}
    return raw;
}
std::optional<std::array<int32_t,2>> BloomCoordinate(const SmacWorkerRadiusMap& map,int64_t x,int64_t y) {
    if(map.horizontal_wrap) {if(x<0) {x+=map.width;} else if(x>=map.width) {x-=map.width;}}
    if(x<0 || x>=map.width || y<0 || y>=map.height) {return std::nullopt;}
    return std::array<int32_t,2>{static_cast<int32_t>(x),static_cast<int32_t>(y)};
}
int64_t BloomDistance(const SmacWorkerRadiusMap& map,const std::array<int32_t,2>& a,const std::array<int32_t,2>& b) {
    auto dx=std::abs(int64_t(a[0])-b[0]);const auto dy=std::abs(int64_t(a[1])-b[1]);
    if(map.horizontal_wrap) {dx=std::min(dx,int64_t(map.width)-dx);}
    return std::max(dx,dy)-(((dx+dy)/2-std::min(dx,dy)+1)/2);
}
SmacBloomTile BloomQuery(const std::function<SmacBloomTile(int32_t,int32_t)>& query,const std::array<int32_t,2>& p) {
    const auto tile=query(p[0],p[1]);
    if(tile.altitude_class<0 || tile.altitude_class>7 || (tile.removable_improvements&~BLOOM_IMPROVEMENTS)) {
        throw std::invalid_argument("Bloom query requires altitude0..7 and semantic removable improvements");
    }
    return tile;
}
}
bool AdmitSmacEcologyBloom(int32_t damage,bool perihelion,uint32_t primary_rules,
    uint32_t secondary_state,const std::function<int32_t()>& draw) {
    if(damage<0 || !draw) {throw std::invalid_argument("Bloom admission requires nonnegative damage and CRT authority");}
    const auto roll=EcologyCrt(draw)%100;
    return roll<damage/(perihelion ? 2 : 1) && !(primary_rules&0x00200000u)
        && !((secondary_state&1u) && !(secondary_state&8u));
}
std::optional<SmacBloomSelection> SelectSmacEcologyBloom(const SmacWorkerRadiusMap& map,
    int32_t base_id,const std::vector<SmacBloomBase>& bases,
    const std::function<SmacBloomTile(int32_t,int32_t)>& query,const std::function<int32_t()>& draw) {
    SmacWorkedSquareCollection::Resolve(map,0); // shared dimension/center/parity admission
    if(!query || !draw) {throw std::invalid_argument("Bloom selection requires terrain and CRT authorities");}
    bool found=false;std::set<int32_t> ids;
    for(const auto& base:bases) {
        const auto& p=base.coordinate;
        if(base.id<0 || !ids.insert(base.id).second || p[0]<0 || p[0]>=map.width || p[1]<0 || p[1]>=map.height || ((p[0]+int64_t(p[1]))&1)) {
            throw std::invalid_argument("Bloom selection requires valid ordered Base records");
        }
        if(base.id==base_id) {
            if(p!=map.center) {throw std::invalid_argument("Bloom Base differs from the resource center");}
            found=true;
        }
    }
    if(!found) {throw std::invalid_argument("Bloom Base is absent from global records");}
    std::optional<SmacBloomSelection> best;
    for(size_t i=0;i<BLOOM_OFFSETS.size();++i) {
        if(i>=24 && best) {continue;}
        const auto& offset=BLOOM_OFFSETS[i];
        const auto p=BloomCoordinate(map,int64_t(map.center[0])+offset[0],int64_t(map.center[1])+offset[1]);
        if(!p) {continue;}
        const auto tile=BloomQuery(query,*p);
        if((tile.fungus && tile.altitude_class>=2) || tile.real_base) {continue;}
        int32_t weight=i<24 ? 0 : 1;
        if(i>=24) {
            int32_t nearest=-1;int64_t distance=INT64_MAX;
            for(const auto& base:bases) {
                const auto candidate=BloomDistance(map,*p,base.coordinate);
                if(candidate<=distance) {nearest=base.id;distance=candidate;}
            }
            if(nearest!=base_id) {continue;}
        }
        for(const auto& delta:std::array<std::array<int32_t,2>,4>{{{1,-1},{1,1},{-1,1},{-1,-1}}}) {
            const auto neighbor=BloomCoordinate(map,int64_t((*p)[0])+delta[0],int64_t((*p)[1])+delta[1]);
            if(neighbor) {const auto v=BloomQuery(query,*neighbor);if(v.fungus && v.altitude_class>=2) {weight+=2;}}
        }
        if(!weight) {continue;}
        if(weight>1) {
            auto bits=tile.removable_improvements;while(bits) {bits&=bits-1;++weight;}
            if(i<8) {++weight;}
        }
        if(tile.altitude_class<3) {weight=(weight+1)/2;}
        weight*=10;
        const auto score=weight>1 ? EcologyCrt(draw)%weight : weight;
        if(score>(best ? best->score : 0)) {best=SmacBloomSelection{*p,static_cast<int32_t>(i+1),score};}
    }
    return best;
}


namespace {
constexpr std::array<std::array<int32_t,2>,121> BLOOM_SHAPER_OFFSETS{{
    {0,0},{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},
    {0,-2},{2,-2},{2,2},{-2,2},{-2,-2},{1,-3},{3,-1},{3,1},
    {1,3},{-1,3},{-3,1},{-3,-1},{-1,-3},{4,0},{-4,0},{0,4},
    {0,-4},{1,-5},{2,-4},{3,-3},{4,-2},{5,-1},{5,1},{4,2},
    {3,3},{2,4},{1,5},{-1,5},{-2,4},{-3,3},{-4,2},{-5,1},
    {-5,-1},{-4,-2},{-3,-3},{-2,-4},{-1,-5},{0,6},{6,0},{0,-6},
    {-6,0},{0,-8},{1,-7},{2,-6},{3,-5},{4,-4},{5,-3},{6,-2},
    {7,-1},{8,0},{7,1},{6,2},{5,3},{4,4},{3,5},{2,6},
    {1,7},{0,8},{-1,7},{-2,6},{-3,5},{-4,4},{-5,3},{-6,2},
    {-7,1},{-8,0},{-7,-1},{-6,-2},{-5,-3},{-4,-4},{-3,-5},{-2,-6},
    {-1,-7},{0,-10},{1,-9},{2,-8},{3,-7},{4,-6},{5,-5},{6,-4},
    {7,-3},{8,-2},{9,-1},{10,0},{9,1},{8,2},{7,3},{6,4},
    {5,5},{4,6},{3,7},{2,8},{1,9},{0,10},{-1,9},{-2,8},
    {-3,7},{-4,6},{-5,5},{-6,4},{-7,3},{-8,2},{-9,1},{-10,0},
    {-9,-1},{-8,-2},{-7,-3},{-6,-4},{-5,-5},{-4,-6},{-3,-7},{-2,-8},
    {-1,-9},
}};
constexpr std::array<size_t,6> BLOOM_RING_BOUNDARIES{{1,9,25,49,81,121}};
}
SmacBloomClassChange ChangeSmacBloomClassOnly(SmacBloomTerrainCell cell,int32_t altitude,bool nonzero_turn,
    const std::function<int32_t()>& draw) {
    if(cell.altitude_class<0 || cell.altitude_class>7 || altitude<0 || altitude>7 || cell.rockiness<0 || cell.rockiness>2
        || cell.affiliation>15 || (cell.altitude_class>=3)!=(altitude>=3) || !draw) {
        throw std::invalid_argument("Class-only ecology setter requires valid same-category terrain and CRT authority");
    }
    uint32_t dirty=4;
    if(nonzero_turn && cell.altitude_class!=altitude) {
        if(cell.landmarks) {
            auto high=cell.landmarks>>24;if(high<128) {high^=255;}
            cell.landmarks=(cell.landmarks&0x00ffff7fu)|(high<<24);
        }
        bool changed=false;
        if(altitude>=3) {
            if(altitude<cell.altitude_class && cell.rockiness>0 && cell.rockiness>altitude-3) {
                if(EcologyCrt(draw)%(cell.rockiness+1)!=0) {--cell.rockiness;changed=true;}
            } else if(altitude>cell.altitude_class && cell.rockiness<2 && cell.rockiness+1<=altitude-3) {
                if(EcologyCrt(draw)%(cell.rockiness+4)==0) {++cell.rockiness;changed=true;}
            }
        }
        if(changed) {cell.landmarks|=0x00400000u;dirty|=1;}
    }
    cell.altitude_class=altitude;return {cell,dirty};
}
SmacBloomTerrainPlan PrepareSmacBloomTerrain(const SmacWorkerRadiusMap& map,int32_t history,int32_t difficulty,bool nonzero_turn,
    const std::function<SmacBloomTerrainCell(int32_t,int32_t)>& query,const std::function<int32_t()>& draw) {
    SmacWorkedSquareCollection::Resolve(map,0);
    if(history<0 || difficulty<0 || difficulty>5 || !query || !draw) {throw std::invalid_argument("Bloom terrain requires history, difficulty and terrain/CRT authorities");}
    SmacBloomTerrainPlan plan(Checked(int64_t(history)+1),nonzero_turn);
    using Coordinate=std::array<int32_t,2>;
    struct Staged {SmacBloomTerrainCell before,after;};
    std::map<Coordinate,Staged> staged;std::vector<Coordinate> changed;
    const auto get=[&](const Coordinate& coordinate)->SmacBloomTerrainCell& {
        auto found=staged.find(coordinate);
        if(found==staged.end()) {
            const auto value=query(coordinate[0],coordinate[1]);
            if(value.altitude_class<0 || value.altitude_class>7 || value.rockiness<0 || value.rockiness>2 || value.affiliation>15) {
                throw std::invalid_argument("Bloom terrain cell exceeds measured semantic bounds");
            }
            found=staged.emplace(coordinate,Staged{value,value}).first;
        }
        return found->second.after;
    };
    const auto mark=[&](const Coordinate& coordinate) {
        if(std::find(changed.begin(),changed.end(),coordinate)==changed.end()) {changed.push_back(coordinate);}
    };
    const auto set_class=[&](const Coordinate& coordinate,int32_t altitude) {
        auto& cell=get(coordinate);
        if(cell.altitude_class==altitude) {return;}
        const auto change=ChangeSmacBloomClassOnly(cell,altitude,nonzero_turn,draw);
        cell=change.cell;plan.m_native_dirty_mask|=change.native_dirty_mask;
        mark(coordinate);
    };
    const auto shape=[&](const Coordinate& center) {
        set_class(center,2);
        for(size_t i=1;i<9;++i) {
            const auto& offset=BLOOM_SHAPER_OFFSETS[i];
            const auto p=BloomCoordinate(map,int64_t(center[0])+offset[0],int64_t(center[1])+offset[1]);
            if(p && get(*p).altitude_class<1) {set_class(*p,1);}
        }
        for(size_t ring=1;ring<=5;++ring) {
            bool altered=false;
            for(size_t i=BLOOM_RING_BOUNDARIES[ring-1];i<BLOOM_RING_BOUNDARIES[ring];++i) {
                const auto& offset=BLOOM_SHAPER_OFFSETS[i];
                const auto p=BloomCoordinate(map,int64_t(center[0])+offset[0],int64_t(center[1])+offset[1]);
                const auto ceiling=static_cast<int32_t>(2+ring);
                if(p && get(*p).altitude_class>ceiling) {set_class(*p,ceiling);altered=true;}
            }
            if(!altered) {break;}
        }
    };
    if(get(map.center).altitude_class<3 && plan.m_history>=4) {
        const auto denominator=10-plan.m_history/8-difficulty;
        if(denominator<=1 || EcologyCrt(draw)%denominator==0) {plan.m_nominal_cell_budget=9;}
    }
    for(int32_t i=0;i<plan.m_nominal_cell_budget;++i) {
        const auto& offset=BLOOM_SHAPER_OFFSETS[i];
        const auto p=BloomCoordinate(map,int64_t(map.center[0])+offset[0],int64_t(map.center[1])+offset[1]);
        if(!p) {continue;}
        auto& cell=get(*p);
        if((i>0 && cell.altitude_class>=3) || ((cell.items&0x20u) && cell.altitude_class>=2)
            || ((cell.items&1u) && cell.affiliation<=7) || (cell.landmarks&0x80000002u)==2u) {continue;}
        cell.items=(cell.items|0x20u)&~0x00288050u;mark(*p);plan.m_fungus_cell=*p;
        if(cell.altitude_class<2) {shape(*p);}
        // Native nineteen-entry notice scan overwrites the bloom-loop index.
        // The next increment ends either budget, even with no improvements.
        break;
    }
    for(const auto& coordinate:changed) {
        const auto& value=staged.at(coordinate);
        if(!(value.before==value.after)) {plan.m_changes.push_back({coordinate,value.before,value.after});}
    }
    return plan;
}
SmacNativeSpawnPlan PrepareSmacNativeSpawn(const SmacNativeSpawnInput& input,
    const std::vector<SmacNativeSpawnBase>& bases,const std::function<SmacNativeSpawnTile(int32_t,int32_t)>& query) {
    const auto& map=input.map;SmacWorkedSquareCollection::Resolve(map,0);
    if(!query || input.actor<1 || input.actor>7 || input.history<7 || input.population < -128 || input.population>127) {
        throw std::invalid_argument("Native spawn requires post-hold history, actor and signed population authority");
    }
    for(const auto& base:bases) {
        const auto& p=base.coordinate;
        if(base.owner<0 || base.owner>7 || p[0]<0 || p[0]>=map.width || p[1]<0 || p[1]>=map.height || ((int64_t(p[0])+p[1])&1)) {
            throw std::invalid_argument("Native spawn requires ordered canonical Base coordinates and owners");
        }
    }
    const auto cell=[&](const std::array<int32_t,2>& p) {
        const auto value=query(p[0],p[1]);
        if(value.altitude_class<0 || value.altitude_class>7 || value.cached_owner>15) {throw std::invalid_argument("Native spawn tile exceeds original field bounds");}
        return value;
    };
    const auto blocked=[](const SmacNativeSpawnTile& tile) {return tile.unit_or_base && tile.cached_owner>=1 && tile.cached_owner<=7;};
    SmacNativeSpawnPlan plan(input);plan.m_requests.reserve(127);
    constexpr std::array<std::array<int32_t,2>,9> offsets{{{0,0},{1,-1},{2,0},{1,1},{0,2},{-1,1},{-2,0},{-1,-1},{0,-2}}};
    for(int32_t ordinal=0;ordinal<std::min(input.history,9);++ordinal) {
        const auto p=BloomCoordinate(map,int64_t(map.center[0])+offsets[ordinal][0],int64_t(map.center[1])+offsets[ordinal][1]);
        if(!p) {continue;}
        const auto tile=cell(*p);
        if(!tile.fungus || tile.altitude_class<2) {continue;}
        int32_t nearest=-1;int64_t distance=INT64_MAX;
        for(const auto& base:bases) {
            const auto candidate=BloomDistance(map,*p,base.coordinate);
            if(candidate<=distance) {nearest=base.owner;distance=candidate;}
        }
        if(nearest!=input.actor || blocked(tile)) {continue;}
        const auto design=tile.region<64 ? (tile.actor_regional_base_count ? 8 : 10)
            : (plan.m_special_counter%3==0 ? 10 : 9);
        if(design!=8) {
            ++plan.m_special_counter;
            const auto base_limit=Checked(int64_t(2)*input.retained_base_count)/3;
            if(plan.m_special_counter>=base_limit || plan.m_special_counter>=input.history/3) {continue;}
        }
        ++plan.m_sweep_attempts;
        plan.m_requests.push_back({*p,design,plan.m_sweep_attempts,plan.m_special_counter,static_cast<uint8_t>(input.actor)});
    }
    const auto selected=cell(map.center);
    if(!blocked(selected)) {
        const auto design=selected.actor_regional_base_count ? (selected.region<64 ? 8 : 9) : 10;
        for(auto index=plan.m_sweep_attempts;index<input.history/3 && index<input.population;++index) {
            plan.m_requests.push_back({map.center,design,plan.m_sweep_attempts,plan.m_special_counter,std::nullopt});
        }
    }
    return plan;
}

}
