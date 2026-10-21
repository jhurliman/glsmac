#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include "input/Types.h"
namespace ui::dom {
struct SmacEditState {std::string text;size_t anchor,caret;};
struct SmacEditStep {SmacEditState state;bool handled,beep;};
// Original62f180/62f360, narrow bytes and an explicit source maximum.
// One-step Right past the maximum retains its anomalous caret, but later
// uncharacterized out-of-buffer edits never index outside the stored string.
inline SmacEditStep StepSmacEdit(SmacEditState state,uint32_t key,bool character,size_t maximum=31) {
    SmacEditStep result{std::move(state),true,false};auto& edit=result.state;auto& text=edit.text;
    const auto first=std::min(edit.anchor,edit.caret),last=std::max(edit.anchor,edit.caret);
    if(character) {
        if(key<32) {result.handled=false;return result;}
        if(last>text.size() || (maximum && text.size()>=maximum && first==last)) {result.beep=true;return result;}
        text.replace(first,last-first,1,char(uint8_t(key)));edit.anchor=edit.caret=first+1;return result;
    }
    const bool shift=(key&0x10000)!=0,control=(key&0x20000)!=0;const auto code=key&65535;
    if(code==8 || code==46) {
        if(shift || control || last>text.size()) {result.beep=true;return result;}
        if(first!=last) {text.erase(first,last-first);edit.anchor=edit.caret=first;}
        else if(code==8 && edit.caret) {text.erase(edit.caret-1,1);--edit.caret;edit.anchor=edit.caret;}
        else if(code==46 && edit.caret<text.size()) {text.erase(edit.caret,1);}
        else {result.beep=true;}return result;
    }
    if(code==35) {edit.caret=text.size();}
    else if(code==36) {edit.caret=0;}
    else if(code==37 && !control) {if(edit.caret) {--edit.caret;}}
    else if(code==39 && !control) {
        ++edit.caret;
        if(edit.caret==text.size()+1) {
            if(maximum && text.size()>=maximum) {result.beep=true;return result;}
            text+=' ';
        }else if(edit.caret>text.size()) {result.beep=true;return result;}
    }else {result.handled=false;return result;}
    if(!shift) {edit.anchor=edit.caret;}return result;
}
// Translate the engine's separate physical key and printable-byte fields.
// A printable letter/space still has its ordinary SDL scan code.
inline SmacEditStep StepSmacInput(SmacEditState state,input::key_code_t physical,char byte,bool printable,input::key_modifier_t modifiers,size_t maximum) {
    const bool shift=(modifiers&input::KM_SHIFT)!=0,control=(modifiers&input::KM_CTRL)!=0;
    uint32_t code=0;
    if(physical==input::K_HOME) {code=36;}else if(physical==input::K_END) {code=35;}
    else if(physical==input::K_LEFT || physical==input::K_KP_LEFT) {code=37;}
    else if(physical==input::K_RIGHT || physical==input::K_KP_RIGHT) {code=39;}
    else if(physical==input::K_BACKSPACE) {code=8;}else if(physical==input::K_DELETE) {code=46;}
    if(!code) {
        if(control || (modifiers&input::KM_ALT)) {return {std::move(state),printable,false};}
        if(!printable) {return {std::move(state),false,false};}
        return StepSmacEdit(std::move(state),uint8_t(byte),true,maximum);
    }
    return StepSmacEdit(std::move(state),code|(shift ? 0x10000:0)|(control ? 0x20000:0),false,maximum);
}
}
