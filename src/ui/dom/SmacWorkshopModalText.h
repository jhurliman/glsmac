#pragma once
#include <array>
#include <cstdint>
#include <utility>
#include <stdexcept>
#include <string>
#include <vector>
#include "util/String.h"

namespace ui::dom {
struct SmacWorkshopModalTemplate {std::string caption,body;std::vector<std::string> rows;};
inline SmacWorkshopModalTemplate ReadSmacWorkshopModalTemplate(const std::string& source,const std::string& tag) {
    if(source.size()>2*1024*1024) {throw std::invalid_argument("Workshop modal source exceeds budget");}
    SmacWorkshopModalTemplate result;bool inside=false,found=false,choices=false;
    for(const auto& raw:util::String::Split(source,'\n')) {
        const auto line=util::String::TrimCopy(raw);
        if(line.empty()) {if(inside && !result.body.empty()) {choices=true;}continue;}
        if(line.front()==';' || line.compare(0,2,"##")==0) {continue;}
        if(line.front()=='#' && line.find_first_of(" \t")==std::string::npos) {
            inside=line=="#"+tag;if(inside) {if(found) {throw std::invalid_argument("Duplicate Workshop modal template");}found=true;}
        }else if(inside) {
            if(line.compare(0,9,"#caption ")==0) {result.caption=line.substr(9);}
            else if(line.front()!='#') {
                if(choices) {result.rows.push_back(line);}
                else {if(!result.body.empty()) {result.body+=' ';}result.body+=line;}
            }
        }
    }
    if(!found || result.caption.empty() || result.caption.size()>256 || result.body.empty() || result.body.size()>2048
        || result.rows.size()>2 || (result.rows.size()==1)) {throw std::invalid_argument("Invalid Workshop modal template");}
    return result;
}
inline std::string SubstituteSmacWorkshopModal(const std::string& source,const std::array<std::string,2>& names,const std::array<int32_t,3>& numbers) {
    std::string result;
    for(size_t i=0;i<source.size();) {
        bool replaced=false;
        for(size_t n=0;n<names.size();++n) {const auto token="$UNITTYPE"+std::to_string(n);
            if(source.compare(i,token.size(),token)==0) {result+=names[n];i+=token.size();replaced=true;break;}}
        if(replaced) {continue;}
        for(size_t n=0;n<numbers.size();++n) {const auto token="$NUM"+std::to_string(n);
            if(source.compare(i,token.size(),token)==0) {result+=std::to_string(numbers[n]);i+=token.size();replaced=true;break;}}
        if(replaced) {continue;}
        if(source.compare(i,3,"$<#")==0 && i+4<source.size() && source[i+3]>='0' && source[i+3]<='2' && source[i+4]==':') {
            const auto middle=source.find(':',i+5),end=source.find('>',i+5);const size_t n=size_t(source[i+3]-'0');
            if(middle==std::string::npos || end==std::string::npos || middle>end) {throw std::invalid_argument("Invalid Workshop modal plural token");}
            result+=numbers[n]==1 ? source.substr(i+5,middle-i-5):source.substr(middle+1,end-middle-1);i=end+1;
        }else {result+=source[i++];}
    }
    if(result.size()>4096 || result.find('\0')!=std::string::npos) {throw std::invalid_argument("Workshop modal output exceeds budget");}return result;
}
struct SmacWorkshopModalSpan {std::string text;bool emphasis;};
struct SmacWorkshopModalWord {std::string text;std::vector<SmacWorkshopModalSpan> spans;};
inline std::vector<SmacWorkshopModalWord> SmacWorkshopModalWords(const std::string& text) {
    std::vector<SmacWorkshopModalWord> words;SmacWorkshopModalWord word;bool emphasis=false;
    for(const char c:text) {
        if(c=='{') {emphasis=true;continue;}if(c=='}') {emphasis=false;continue;}
        word.text+=c;
        if(word.spans.empty() || word.spans.back().emphasis!=emphasis) {word.spans.push_back({{},emphasis});}
        word.spans.back().text+=c;
        if(c==' ') {words.push_back(std::move(word));word={};}
    }
    if(!word.text.empty()) {words.push_back(std::move(word));}return words;
}
}
