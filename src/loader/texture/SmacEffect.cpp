#include "SmacEffect.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <stdexcept>
namespace loader::texture::smac_effect {
Rows ReadRows(const std::string& system,int bank) {
    if(bank!=0 && bank!=3) {throw std::invalid_argument("Scout effects require bank0 or bank3");}
    Rows rows{};size_t count=0;bool active=false,found=false;
    const auto section="#EXPLOSIONS"+std::to_string(bank);
    std::istringstream input(system);std::string line;
    while(std::getline(input,line)) {
        const auto comment=line.find(';');if(comment!=std::string::npos) {line.resize(comment);}
        const auto first=line.find_first_not_of(" \t\r");if(first==std::string::npos) {continue;}
        const auto last=line.find_last_not_of(" \t\r");line=line.substr(first,last-first+1);
        if(line[0]=='#') {
            active=line==section;if(active && found) {throw std::invalid_argument("Duplicate explosion bank");}found=found || active;continue;
        }
        if(!active) {continue;}
        if(count==rows.size()) {throw std::invalid_argument("Too many explosion rows");}
        std::replace(line.begin(),line.end(),',',' ');std::istringstream fields(line);std::string extra;
        if(!(fields>>rows[count].color>>rows[count].opacity) || (fields>>extra)) {throw std::invalid_argument("Invalid explosion row");}++count;
    }
    if(!found || count!=rows.size()) {throw std::invalid_argument("Missing explosion bank rows");}return rows;
}
Bank Generate(const Palette& palette,const Rows& rows,const std::vector<std::pair<uint8_t,uint8_t>>& ranges) {
    std::array<bool,256> protect{};for(size_t i=0;i<10;++i) {protect[i]=true;protect[246+i]=true;}
    for(const auto range:ranges) {if(range.first>range.second) {throw std::invalid_argument("Inverted protected palette range");}for(int i=range.first;i<=range.second;++i) {protect[i]=true;}}
    for(const auto row:rows) {if(row.color<0 || row.color>=236 || row.opacity<0 || row.opacity>100) {throw std::invalid_argument("Explosion row is outside the native palette/opacity domain");}}
    if(std::all_of(protect.begin(),protect.end(),[](bool value){return value;})) {throw std::invalid_argument("Effect palette has no ordinary colors");}
    Bank bank{};
    for(size_t source=0;source<rows.size();++source) {for(size_t destination=0;destination<256;++destination) {
        if(protect[destination]) {bank[source*256+destination]=destination;continue;}
        const auto row=rows[source];std::array<int,3> mixed{};
        for(size_t channel=0;channel<3;++channel) {mixed[channel]=((100-row.opacity)*palette[destination][channel]+row.opacity*palette[row.color+10][channel])/100;}
        int best=std::numeric_limits<int>::max();uint8_t selected=0;
        for(int candidate=10;candidate<=245;++candidate) {if(protect[candidate]) {continue;}int distance=0;
            for(size_t channel=0;channel<3;++channel) {const int delta=mixed[channel]-palette[candidate][channel];distance+=delta*delta;}
            if(distance<best) {best=distance;selected=candidate;}
        }bank[source*256+destination]=selected;
    }}return bank;
}
uint8_t Composite(const Bank& bank,uint8_t source,uint8_t destination) {
    if(source==253 || source==255) {return destination;}
    if(source>=16) {throw std::invalid_argument("Effect source reads beyond the verified CLUT bank");}
    return bank[size_t(source)*256+destination];
}
}
