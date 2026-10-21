#pragma once
#include "util/String.h"
#include <array>
#include <stdexcept>
#include <string>
#include <vector>

namespace ui::dom {
struct SmacPreferenceText {int id;std::string label;};
struct SmacPreferencePage {std::string title;std::vector<SmacPreferenceText> rows;};
using SmacPreferencesText=std::array<SmacPreferencePage,7>;

// Original49bde0 constructor inventory, not a sequential control-ID range.
// In particular Advanced reorders28; Audio skips three template-only rows;
// Warnings has no53 and continues with57..60 on page6.
inline SmacPreferencesText ReadSmacPreferencesText(const std::string& source) {
    if(source.size()>2*1024*1024 || source.find('\0')!=std::string::npos) {
        throw std::invalid_argument("Preferences script exceeds its supported bounds");
    }
    const std::array<std::string,6> tags{{"BASICPREFS","ADVANCEDPREFS","BASEWARNINGS","AUTOMATIONPREFS","AUDIOVISUALPREFS","MAPPREFS"}};
    const std::array<size_t,6> counts{{8,12,17,14,11,8}};
    const std::array<std::vector<int>,7> ids{{
        {7,8,9,10,11,12,13,14}, {15,16,17,18,19,28,20,21,23,24,25,26},
        {43,44,45,46,47,48,49,50,51,52,54,55,56},
        {29,30,31,32,33,34,42,35,36,37,38,39,40,41},
        {64,65,66,71,67,68,69,22}, {72,73,78,74,75,76,77,79}, {57,58,59,60}
    }};
    SmacPreferencesText result;
    for(size_t page=0;page<tags.size();++page) {
        bool found=false,inside=false,items=false;std::string title;std::vector<std::string> labels;
        for(const auto& raw:util::String::Split(source,'\n')) {
            const auto line=util::String::TrimCopy(raw);
            if(line.empty() || line.front()==';' || line.compare(0,2,"##")==0) {continue;}
            if(line=="#itemlist") {if(inside) {items=true;}continue;}
            if(line.front()=='#' && line.find_first_of(" \t")==std::string::npos) {
                inside=line=="#"+tags[page];items=false;
                if(inside) {if(found) {throw std::invalid_argument("Duplicate Preferences template");}found=true;}
            }else if(inside) {
                if(line.compare(0,9,"#caption ")==0) {title=line.substr(9);}
                else if(items && line.front()!='#') {labels.push_back(line);}
            }
        }
        if(!found || title.empty() || title.size()>256 || labels.size()!=counts[page]) {
            throw std::invalid_argument("Incomplete Preferences template: "+tags[page]);
        }
        for(const auto& label:labels) {if(label.empty() || label.size()>512) {throw std::invalid_argument("Preferences label exceeds bounds");}}
        const size_t first=page==4?3:0;
        result[page].title=title;
        for(size_t i=0;i<ids[page].size();++i) {result[page].rows.push_back({ids[page][i],labels.at(first+i)});}
        if(page==2) {
            result[6].title=title;
            for(size_t i=0;i<ids[6].size();++i) {result[6].rows.push_back({ids[6][i],labels.at(13+i)});}
        }
    }
    return result;
}
}
