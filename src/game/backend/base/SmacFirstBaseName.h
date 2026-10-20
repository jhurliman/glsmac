#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace game::backend::base {
// FIRSTBASE's23-byte edit result followed by50cc80's exact duplicate count.
// The suffix is one pass, even when that suffixed name is already in use.
inline std::string NormalizeSmacFirstBaseName(std::string_view supplied,const std::vector<std::string_view>& others) {
    std::string name(supplied.substr(0,std::min(size_t(23),supplied.find('\0'))));
    const auto matches=std::count(others.begin(),others.end(),std::string_view(name));
    if(matches) {name=name.substr(0,21)+"-"+std::to_string(matches+1);}
    return name;
}
}
