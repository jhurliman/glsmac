#include "SmacPretemperatureGeneration.h"
#include "SmacGenerationNoise.h"
#include "SmacShapingOffsets.h"
#include <algorithm>
#include <optional>
#include <stdexcept>

namespace game::backend::map::smac_generation {
namespace {
int32_t Checked(int64_t v) {
    if(v<INT32_MIN || v>INT32_MAX) {throw std::overflow_error("Generation arithmetic exceeds measured signed32 domain");}
    return int32_t(v);
}
void Validate(const State& s,Geometry g) {
    if(g.width<8 || g.height<8 || g.width>65534 || g.height>65534 || (g.width&1)
        || uint64_t(g.width/2)*uint64_t(g.height)!=s.records.size() || s.records.size()>UINT32_MAX/44u) {
        throw std::invalid_argument("Native generation requires a complete admitted owned plane");
    }
}
ContinentRules Continent(const State& s,const WorldRules& r) {return {r[2],r[3],{{r[25],r[26],r[27],r[28],r[29]}},s.primary_rules};}
int64_t Sqrt(size_t n) {
    size_t lo=0,hi=n+1;
    while(hi-lo>1) {const auto mid=lo+(hi-lo)/2;if(mid<=n/mid) {lo=mid;}else {hi=mid;}}
    return int64_t(lo);
}
void Pump(ContinentTrace* t) {if(t) {++t->pumps;if(t->checkpoint) {t->checkpoint();}}}
}
void MorphContours(State& s,Geometry g,const WorldRules& rule,Regions& regions,const GenerationTrace& trace) {
    Validate(s,g);
    if(s.stage!=Stage::ContinentLoop || s.generation_flags!=std::array<int32_t,2>{{1,1}}) {
        throw std::invalid_argument("Contour morph requires turn-zero generation continuation");
    }
    const auto land=s.categories[2],erosion=s.categories[3],ocean=s.categories[1];
    if(land<0 || land>2 || erosion<0 || erosion>2 || ocean<0 || ocean>2) {throw std::invalid_argument("Unresolved generation categories");}
    const int64_t scale_n=land==1?Sqrt(s.records.size()):1,scale_d=land==1?56:1;
    Noise noise(s.random,trace.random);
    const auto index=[&](Coordinate p){return size_t(p[1])*size_t(g.width/2)+size_t(p[0]/2);};
    const auto band=[&](size_t i){return s.records[i][0]>>5;};
    const auto neighbor=[&](Coordinate p,size_t k)->std::optional<size_t> {
        p[0]+=smac_shaping::OFFSETS[k][0];p[1]+=smac_shaping::OFFSETS[k][1];
        if(s.topology==0) {if(p[0]<0) {p[0]+=g.width;}else if(p[0]>=g.width) {p[0]-=g.width;}}
        if(p[0]<0 || p[0]>=g.width || p[1]<0 || p[1]>=g.height) {return std::nullopt;}
        return index(p);
    };
    for(int phase=0;phase<2;++phase) {
        if(phase) {ReconstructAltitudes(s,g,Continent(s,rule),trace.shaping);}
        RebuildRegions(s,g,regions,trace.regions);
        for(int32_t y=0;y<g.height;++y) {
            for(int32_t x=y&1;x<g.width;x+=2) {
                const Coordinate p{{x,y}};const auto a=index(p);const auto b=band(a);const int64_t old=s.records[a][1];
                const int64_t value=std::clamp(((5*noise.Evaluate(16*x,16*y)+2*noise.Evaluate(128*x,128*y))*7)/256,0,100);
                int64_t result=value;
                if((b<=2)!=(phase==0)) {continue;}
                int water=0;bool other=false;
                if(b==2 || b==3) {
                    for(size_t k=1;k<=8;++k) {const auto n=neighbor(p,k);if(n && band(*n)<=2) {++water;}}
                    if(b==3) {
                        for(size_t k=9;k<25;++k) {const auto n=neighbor(p,k);
                            if(n && band(*n)>2 && s.records[*n][3]<64 && s.records[*n][3]!=s.records[a][3]) {other=true;}
                        }
                    } else {
                        int best=-1;
                        for(size_t k=1;k<=8;++k) {
                            const auto n=neighbor(p,k);if(!n || band(*n)<=2) {continue;}
                            const auto r=s.records[*n][3];
                            if(best<0) {best=r;}
                            else if(r!=best) {
                                if(int64_t(regions.Area(best))+regions.Area(r)>=int64_t(50)*int64_t(s.records.size())/3200) {other=true;}
                                if(regions.Area(r)>regions.Area(best)) {best=r;}
                            }
                        }
                    }
                }
                if(b<=1) {result=value-rule[18];}
                else if(b==2) {
                    if(other) {water=std::clamp(water+4,0,8);}
                    if(value<old) {result=value+rule[19];}
                    if(result>=s.working_land_table[3]) {result-=(int64_t(water)-3)*rule[19]/4;}
                } else if(b==3) {
                    if(other) {water=std::clamp(water+4,0,8);}
                    const auto q=8-water,k=std::clamp(ocean,1,2);
                    result=value+int64_t(q)*rule[20]*scale_n/(scale_d*8);
                    if(result<old) {result+=(int64_t(rule[21])+(int64_t(rule[22])*erosion/2)*q/(k*8))*scale_n/scale_d;}
                    else {result=old+(result-old)*(8-std::clamp(water,0,8))*erosion/(k*8);}
                } else if(b==4) {
                    if(value>old) {result=2*value-old;}
                    result+=int64_t(rule[22])*erosion+rule[23];
                } else {result=value+int64_t(rule[22])*erosion;}
                auto contour=(result+old+1)/2;if(erosion==0) {contour=std::min<int64_t>(contour,74);}
                s.records[a][1]=uint8_t(contour);
                if(trace.shaping) {++trace.shaping->writes;if(trace.shaping->on_write) {trace.shaping->on_write(uint32_t(a*44+1),uint8_t(contour));}}
            }
            Pump(trace.shaping);
        }
    }
}
PretemperatureResult GeneratePretemperature(State& s,Geometry g,const WorldRules& rule,Regions& regions,const GenerationTrace& trace) {
    Validate(s,g);
    if(g.height&1) {throw std::invalid_argument("Native generated worlds require even height");}
    PretemperatureResult result;
    while(true) {
        const auto entry=result.attempts.empty()?0:s.continent_attempts;
        Bootstrap(s,{rule[0],rule[1],rule[30]},result.attempts.empty()?Entry::First:Entry::Retry,trace.random);
        for(size_t i=1;i<=3;++i) {if(s.categories[i]<0 || s.categories[i]>2) {throw std::invalid_argument("Unresolved generation categories");}}
        int32_t calls=0;
        while(s.land_progress<s.land_target || s.continent_attempts<s.continent_minimum) {
            if(++s.attempt_iterations>=100) {break;}
            ++s.continent_attempts;++calls;SeedContinent(s,g,Continent(s,rule),s.continent_attempts,trace.shaping);
        }
        int32_t hills=0;
        if(s.categories[3]>=1) {
            hills=Checked((int64_t(rule[4])+(int64_t(s.categories[3])-1)*rule[5])*int64_t(s.records.size())/3200);
            if(s.categories[1]==2) {hills/=2;}
            hills=std::max(hills,0);
        }
        for(int32_t i=0;i<hills;++i) {SeedHill(s,g,Continent(s,rule),rule[6],rule[7],4,trace.shaping);}
        MorphContours(s,g,rule,regions,trace);
        bool polar=s.categories[1]==0;
        if(s.categories[1]==1) {
            const auto before=s.random.Crt();const auto raw=s.random.NextCrtRaw15();polar=raw%3==0;
            if(trace.random) {trace.random({Draw::Kind::Crt,before,s.random.Crt(),raw});}
        }
        if(polar) {SeedPolarContours(s,g,Continent(s,rule),trace.shaping);}
        ReconstructAltitudes(s,g,Continent(s,rule),trace.shaping);RebuildRegions(s,g,regions,trace.regions);
        int32_t largest=0,second=0;
        for(size_t i=1;i<63;++i) {
            const auto a=regions.Area(i);
            if(a>=largest) {second=largest;largest=a;}else if(a>=second) {second=a;}
        }
        const bool rejected=second<(s.categories[2]==1?int64_t(2)*largest/3:largest/2);
        result.attempts.push_back({entry,s.continent_attempts,s.attempt_iterations,calls,hills,rejected});
        if(!rejected) {result.accepted=true;break;}
        if(++result.retries>=4) {break;}
    }
    s.stage=Stage::Pretemperature;return result;
}
}
