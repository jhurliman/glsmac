#include "SmacWorkshop.h"
#include "game/backend/unit/SmacWorkshop.h"
#include "loader/txt/SmacRulesTXTLoader.h"
#include "fixtures/SmacStartupSource.h"
#include "fixtures/SmacPrototypePrice.h"
#include "fixtures/WorkshopAlias.h"
#include "fixtures/WorkshopGeneral.h"
#include "fixtures/WorkshopHuman.h"
#include "fixtures/WorkshopDomain.h"
#include "fixtures/WorkshopPicker.h"
#include "fixtures/WorkshopStats.h"
#include "fixtures/WorkshopData.h"
#include "fixtures/WorkshopEdit.h"
#include "ui/dom/SmacEditState.h"
#include "ui/dom/SmacWorkshopModalText.h"
#include "fixtures/WorkshopAbilities.h"
#include "fixtures/WorkshopSavedCommits.h"
#include "fixtures/WorkshopUpgrade.h"
#include "fixtures/WorkshopUpgradeStrings.h"
#include "game/backend/unit/SmacWorkshopUpgrade.h"
#include "fixtures/WorkshopAllocation.h"
#include "fixtures/SmacIntrinsicOverlays.h"
#include "task/gsetests/GSETests.h"
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iterator>

namespace gse::tests {
namespace {
using namespace game::backend;
using Kind=unit::SmacWorkshopComponent;
bool Rejects(const std::function<void()>& fn) {try {fn();}catch(const std::exception&) {return true;}return false;}
faction::SmacFactionTechnologyState Owner() {faction::SmacFactionTechnologyState owner;owner.admission={1,"GAIANS"};owner.owned[1]=true;return owner;}
unit::SmacStartupDesignState Empty() {unit::SmacStartupDesignState state;state.global_difficulty=3;state.owners={{1,{},2},{2,{},2}};return state;}
}
void AddSmacWorkshopTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC Workshop Upgrade stored strings match11 native stats and three prompt pairs",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream file(std::string(assets)+"/alpha.txt",std::ios::binary);const std::string source(std::istreambuf_iterator<char>(file),{});
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        const std::array<std::string,3> words{{"Psi","Sea","Air"}};
        for(const auto& c:workshop_upgrade_strings::Cases) {
            const auto stats=unit::FormatSmacWorkshopStats({64,c.chassis,c.weapon,c.armor,c.mask,c.reactor,{}},catalog);
            GT_ASSERT(::ui::dom::SmacWorkshopStoredModalStats({{},stats,{},c.domain,c.reactor,c.mask},words)==c.expected,"Original5935c0 string differs: "+std::string(c.id));
        }
        const auto label=[&](const std::string& name,uint32_t weapon,uint32_t armor,uint32_t mask,uint8_t reactor) {
            const auto stats=unit::FormatSmacWorkshopStats({64,0,weapon,armor,mask&0xffffff,reactor,{}},catalog);
            std::string tokens;for(size_t i=0;i<24;++i) {if(mask&(1u<<i)) {tokens+=catalog.abilities.abilities[i].abbreviation;}}
            return ::ui::dom::SmacWorkshopModalDesignLabel({name,stats,tokens,0,reactor,mask},words);
        };
        GT_ASSERT(label("Upgrade Source",0,1,0x94,1)=="Upgrade Source (1^-2-1 CloakedDropCarrier)"
            && label("Upgrade Target 65",1,2,0x40208,1)=="Upgrade Target 65 (2~-3+t-1 AmphibiousECMTrance)","Upgrade ability modifiers lost bit order or concatenation");
        GT_ASSERT(label("Upgrade Source",0,1,0,2)=="Upgrade Source (1-2-1*2)" && label("Upgrade Target 65",1,2,0,2)=="Upgrade Target 65 (2-3-1*2)","Upgrade prompt reactor suffix differs");
        GT_ASSERT(label("Upgrade Source",0,1,2,1)=="Upgrade Source (1-2-1 )" && label("Upgrade Target 65",1,2,0x80000000u,1)=="Upgrade Target 65 (2-3-1 )","Empty ability suffix lost its raw-mask space");
        GT_ASSERT(::ui::dom::SmacWorkshopStoredModalStats({{},"?-?-4*2",{},1,2,0},{"Mind","Ocean","Sky"})=="Mind-Mind-4 Ocean*2","Stored-stat localization changed reactor/domain order");GT_OK();
    });
    task->AddTest("SMAC Workshop Upgrade modal templates preserve owned text and emphasis word boundaries",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream file(std::string(assets)+"/script.txt",std::ios::binary);const std::string source(std::istreambuf_iterator<char>(file),{});
        using namespace ::ui::dom;
        const auto confirm=ReadSmacWorkshopModalTemplate(source,"UPGRADEPROTO");
        GT_ASSERT(confirm.caption=="Operations Director" && confirm.rows.size()==2,"Original Upgrade confirmation text missing");
        const auto one=SubstituteSmacWorkshopModal(confirm.body,{"Source ($NUM0)","Target"},{100,2000,1});
        const auto many=SubstituteSmacWorkshopModal(confirm.body,{"Source","Target"},{640,2000,2});
        GT_ASSERT(one.find("our single")!=std::string::npos && one.find("Source ($NUM0) unit ")!=std::string::npos
            && one.find("100 energy")!=std::string::npos && many.find("all of our")!=std::string::npos && many.find("Source units ")!=std::string::npos,
            "Upgrade plural/numeric substitutions recursed into stored text or lost singular form");
        for(const auto& tag:{"WHICHUPGRADE2","UPGRADEPROTOCOST","UPGRADEPROTOCOST2","YESUPGRADE","NOUPGRADE"}) {
            GT_ASSERT(ReadSmacWorkshopModalTemplate(source,tag).rows.empty(),"Informational/chooser template acquired invented choice rows");}
        const auto unprototyped=ReadSmacWorkshopModalTemplate(source,"UPGRADEPROTO2");
        const auto text=SubstituteSmacWorkshopModal(unprototyped.body,{"Source","Target"},{640,2000,2});
        const auto words=SmacWorkshopModalWords(text);std::string flat,emphasis;
        for(const auto& word:words) {std::string actual;for(const auto& span:word.spans) {actual+=span.text;if(span.emphasis) {emphasis+=span.text;}}
            GT_ASSERT(actual==word.text,"Highlight split lost its indivisible wrap word");flat+=word.text;}
        GT_ASSERT(flat.find('{')==std::string::npos && flat.find('}')==std::string::npos && emphasis=="not yet been prototyped,",
            "Prototype warning highlight escaped or acquired neighboring spaces");
        const auto overwrite=ReadSmacWorkshopModalTemplate(source,"OVERWRITEPROTO");
        const auto replacement=SubstituteSmacWorkshopModal(overwrite.body,{"Guard $NUM0",""},{3,0,0});
        GT_ASSERT(overwrite.caption==confirm.caption && overwrite.rows.size()==2
            && replacement.find("\"Guard $NUM0\" type (3 units)")!=std::string::npos
            && overwrite.rows[0]=="Never mind." && overwrite.rows[1]=="Proceed with redesign.",
            "Redesign text did not preserve installed choices or single-pass prototype substitution");
        for(const auto* tag:{"INTRINSICTYPE","INTRINSICNAME"}) {
            const auto info=ReadSmacWorkshopModalTemplate(source,tag);
            const auto body=SubstituteSmacWorkshopModal(info.body,{"Scout $NUM0",""},{7,0,0});
            GT_ASSERT(info.rows.empty() && info.caption==confirm.caption && body.find("Scout $NUM0")!=std::string::npos
                && body.find("$PROTONAME0")==std::string::npos,"Intrinsic informational text lost its installed name or acquired choices");
        }
        GT_ASSERT(Rejects([&]{ReadSmacWorkshopModalTemplate(source+"\n#YESUPGRADE\n#caption Duplicate\nRepeated\n","YESUPGRADE");}),"Duplicate modal template admitted");GT_OK();
    });
    task->AddTest("SMAC Workshop Upgrade matches25 native candidate and cached quote controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream file(std::string(assets)+"/alpha.txt",std::ios::binary);const std::string source(std::istreambuf_iterator<char>(file),{});
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        for(const auto& c:workshop_upgrade_fixture::Cases) {
            auto pool=workshop_general_fixture::BasePool();for(int id=64;id<128;++id) {pool[id]={};}
            for(size_t i=0;i<3;++i) {std::copy(c.records[i].begin(),c.records[i].end(),pool[64+i].begin()+32);
                const std::string name="Design"+std::to_string(64+i);std::copy(name.begin(),name.end(),pool[64+i].begin());}
            const auto before=pool;
            GT_ASSERT(unit::SmacWorkshopUpgradeCandidates(pool,1,64,catalog)==c.candidates,"Native Upgrade candidate mismatch: "+std::string(c.id));
            if(c.quote>=0) {GT_ASSERT(unit::SmacWorkshopUpgradeQuoteRows(pool,65,64,catalog,c.nano)==c.quote,"Native Upgrade quote mismatch: "+std::string(c.id));}
            GT_ASSERT(pool==before,"Upgrade query mutated retained registry");
            if(c.applied) {
                auto state=Empty();state.retirement_history[1]=unit::SmacRetirementHistory::NewWorld();state.RetainNativeRegistry(pool,catalog);const auto original=state;
                const int target=std::string(c.id)=="upgrade_many_choose66" ? 66:65;
                const auto result=unit::PrepareSmacWorkshopUpgrade(state,1,target,64,catalog,uint8_t(c.count),c.nano);
                auto expected=pool;expected[64][48]&=254;if(c.charged) {expected[target][48]|=4;}
                const int morale=int(bool(pool[target][33]&0x40))-int(bool(pool[64][33]&0x40));
                GT_ASSERT(state==original && result.total_energy==c.charged && result.cached_source_count==c.count
                    && result.morale_delta==morale && *result.state.native_registry==expected && result.state.retirement_history==state.retirement_history,
                    "Native Upgrade detached conversion mismatch: "+std::string(c.id));
                GT_ASSERT(unit::SmacStartupDesignState::Deserialize(result.state.Serialize(catalog).ToString(),catalog)==result.state,"Upgrade design flags failed restore");
            }
        }
        auto pool=workshop_general_fixture::BasePool();pool[64]=pool[2];pool[65]=pool[2];pool[65][37]=1;pool[65][38]=1;pool[65][48]=7;
        for(const auto row:std::array<std::array<int,4>,4>{{{{1,1,0,3}},{{1,1,1,1}},{{1,0,0,8}},{{2,0,0,14}}}}) {
            pool[65][41]=uint8_t(row[0]);pool[65][48]=uint8_t(row[1] ? 7:3);
            GT_ASSERT(unit::SmacWorkshopUpgradeQuoteRows(pool,65,64,catalog,bool(row[2]))==row[3],"Native Upgrade scalar quote mismatch");
        }
        // Retained scan predicates deliberately differ from new construction:
        // unrelated ability/reactor changes do not disqualify a stored target.
        auto special=pool;special[65][33]|=0x40;special[65][39]=4;special[65][48]=1;
        GT_ASSERT(unit::SmacWorkshopUpgradeCandidates(special,1,2,catalog)==std::vector<int32_t>({64,65}),"Intrinsic source or retained ability/reactor was filtered");
        special[64][48]=0;special[65][44]=2;
        GT_ASSERT(unit::SmacWorkshopUpgradeCandidates(special,1,2,catalog).empty(),"Owner exclusion was ignored");
        special[65][44]=4;
        GT_ASSERT(unit::SmacWorkshopUpgradeCandidates(special,1,2,catalog)==std::vector<int32_t>({65}),"Another owner's exclusion hid target");
        auto intrinsic=Empty();intrinsic.RetainNativeRegistry(special,catalog);const auto intrinsic_before=intrinsic;
        const auto upgraded=unit::PrepareSmacWorkshopUpgrade(intrinsic,1,65,2,catalog,2,false);
        auto intrinsic_expected=special;intrinsic_expected[65][48]|=4;
        GT_ASSERT(intrinsic==intrinsic_before && *upgraded.state.native_registry==intrinsic_expected && upgraded.morale_delta==1
            && upgraded.state.retirement_history==intrinsic.retirement_history,"Intrinsic Upgrade changed source or history");
        GT_ASSERT(unit::SmacWorkshopUpgradeQuoteRows(pool,2,64,catalog,false)==1,"Intrinsic quote gained a prototype transform");
        GT_ASSERT(Rejects([&]{unit::SmacWorkshopUpgradeCandidates(pool,1,128,catalog);}),"Upgrade admitted a foreign source bank");
        auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto before=state;
        GT_ASSERT(Rejects([&]{unit::PrepareSmacWorkshopUpgrade(state,1,2,64,catalog,1,false);}) && state==before,"Upgrade admitted an intrinsic target");GT_OK();
    });
    task->AddTest("SMAC Workshop direct Retire preserves intrinsic records and records custom old tuples",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream file(std::string(assets)+"/alpha.txt",std::ios::binary);const std::string source(std::istreambuf_iterator<char>(file),{});
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        auto pool=workshop_general_fixture::BasePool();pool[64]=pool[2];pool[64][48]=0x87;
        auto state=Empty();state.retirement_history[1]=unit::SmacRetirementHistory::NewWorld();state.owners[0].starting_scout=64;state.RetainNativeRegistry(pool,catalog);const auto before=state;
        GT_ASSERT(unit::RetireSmacWorkshopDesign(state,2,catalog,1)==before,"Intrinsic Retire changed shared record or history");
        auto expected=pool;expected[64][48]=0x86;const auto next=unit::RetireSmacWorkshopDesign(state,64,catalog,1);
        GT_ASSERT(state==before && *next.native_registry==expected && next.retirement_history[1]->cursor==1
            && next.retirement_history[1]->slots[0]==unit::SmacRetirementTuple({0,0,0,0,0,0,0,1}),"Custom Retire changed more than active flag and old tuple history");
        GT_ASSERT(unit::SmacStartupDesignState::Deserialize(next.Serialize(catalog).ToString(),catalog)==next,"Direct Retire did not survive restore");
        const auto retained=next.RetainedCustomDesign(64,catalog);
        GT_ASSERT(!retained.active && retained.id==64 && retained.chassis==pool[64][36] && retained.weapon==pool[64][37]
            && retained.defense==pool[64][38] && retained.reactor==pool[64][39] && retained.cost==pool[64][41],"Inactive retained record lost its catalog identity");
        GT_ASSERT(Rejects([&]{next.RetainedCustomDesign(65,catalog);}),"Uninitialized empty slot became a catalog design");
        auto foreign_start=next;foreign_start.owners[1].starting_scout=64;
        GT_ASSERT(Rejects([&]{foreign_start.Validate(catalog);}),"Inactive retained record admitted a foreign starting selection");
        GT_ASSERT(Rejects([&]{unit::RetireSmacWorkshopDesign(state,128,catalog,1);}) && state==before,"Direct Retire admitted a foreign bank");GT_OK();
    });
    task->AddTest("SMAC Workshop edit translates printable SDL letters and space before virtual keys",GT(){
        auto result=::ui::dom::StepSmacInput({"Scout Infantry",0,14},input::K_P,'p',true,input::KM_NONE,31);
        GT_ASSERT(result.handled && result.state.text=="p" && result.state.anchor==1 && result.state.caret==1,"SDL letter did not replace selection");
        result=::ui::dom::StepSmacInput(result.state,input::K_SPACE,' ',true,input::KM_NONE,31);
        GT_ASSERT(result.handled && result.state.text=="p " && result.state.anchor==2 && result.state.caret==2,"SDL space took the virtual-key path");
        result=::ui::dom::StepSmacInput(result.state,input::K_G,'G',true,input::KM_SHIFT,31);
        GT_ASSERT(result.handled && result.state.text=="p G","Shift letter lost its printable byte");
        result=::ui::dom::StepSmacInput(result.state,input::K_LEFT,0,false,input::KM_SHIFT,31);
        GT_ASSERT(result.handled && result.state.anchor==3 && result.state.caret==2,"Editing key lost its selection modifier");
        result=::ui::dom::StepSmacInput(result.state,input::K_BACKSPACE,0,false,input::KM_NONE,31);
        GT_ASSERT(result.handled && result.state.text=="p " && result.state.caret==2,"Selected deletion lost mapped range");GT_OK();
    });
    task->AddTest("SMAC Workshop edit matches39 native byte selection navigation and limit controls",GT(){
        for(const auto& row:workshop_edit_fixture::Cases) {
            const auto result=::ui::dom::StepSmacEdit({row.text,row.anchor,row.caret},row.key,row.character);
            GT_ASSERT(result.state.text==row.expected && result.state.anchor==row.end_anchor && result.state.caret==row.end_caret
                && result.handled==row.handled && result.beep==row.beep,"Native edit differs: "+std::string(row.id));
        }GT_OK();
    });
    task->AddTest("SMAC Workshop Rename matches five native draft and pre-dialog flag controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto read=[&](const char* name){std::ifstream file(std::string(assets)+"/"+name,std::ios::binary);if(!file) {throw std::runtime_error("Missing owned Rename source");}return std::string(std::istreambuf_iterator<char>(file),{});};
        const auto source=read("alpha.txt");const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(read("labels.txt"));
        for(size_t i=0;i<5;++i) {
            auto pool=workshop_general_fixture::BasePool();pool[64]=pool[2];pool[64][38]=1;pool[64][48]=i<3 ? 3:1;
            const int selected=i==4 ? 2:64;if(i==4) {pool[2][48]=1;}
            auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto original=state;
            const unit::SmacWorkshopDraft draft{selected,0,0,2,0,1,""};
            const auto opened=unit::OpenSmacWorkshopRename(state,draft,catalog,1);
            GT_ASSERT(opened.default_name=="Plasma Sentinels" && state==original && opened.draft.selected_design==selected
                && opened.draft.armor==2 && opened.draft.name.empty(),"Rename default ignored unsaved draft or mutated caller");
            const auto submitted=i==1 ? opened.default_name:i==3 ? "Draft Guard":i==4 ? "Named Scout":"";
            const auto result=unit::AnswerSmacWorkshopRename(opened.state,opened.draft,opened.default_name,i!=0,submitted,catalog,1);
            auto expected=pool;expected[selected][48]=i>=3 ? 3:1;
            GT_ASSERT(*result.state.native_registry==expected && result.draft.name==(i>=3 ? submitted:"")
                && result.draft.armor==2 && result.draft.selected_design==selected,"Native Rename flags, deferred name or selection differs");
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(result.state.Serialize(catalog).ToString(),catalog)==result.state,"Rename flag effect did not survive restore");
        }
        // A nonempty actor name is the exact default; cancel/same text do not
        // clear its saved named flag, and a case-only edit is a new name.
        auto pool=workshop_general_fixture::BasePool();pool[64]=pool[2];pool[64][48]=3;auto state=Empty();state.RetainNativeRegistry(pool,catalog);
        const unit::SmacWorkshopDraft named{64,0,0,0,0,1,"Existing Guard"};const auto opened=unit::OpenSmacWorkshopRename(state,named,catalog,1);
        GT_ASSERT(opened.default_name==named.name && opened.state==state,"Nonempty Rename default cleared named flag");
        const auto same=unit::AnswerSmacWorkshopRename(opened.state,named,opened.default_name,true,named.name,catalog,1);
        const auto lower=unit::AnswerSmacWorkshopRename(opened.state,named,opened.default_name,true,"existing guard",catalog,1);
        GT_ASSERT(same.state==state && same.draft.name==named.name && lower.draft.name=="existing guard","Rename lost bytewise case/default comparison");
        pool[2][48]|=2;unit::SmacWorkshopDraft intrinsic{2,0,0,0,0,1,"Named Scout"};
        GT_ASSERT(unit::DisplaySmacWorkshopName(pool,intrinsic,catalog,1)=="Scout Patrol","Unchanged intrinsic title incorrectly used draft name");
        intrinsic.armor=2;GT_ASSERT(unit::DisplaySmacWorkshopName(pool,intrinsic,catalog,1)=="Named Scout","Changed intrinsic title lost named draft");
        pool[2][48]&=uint8_t(~2u);GT_ASSERT(unit::DisplaySmacWorkshopName(pool,intrinsic,catalog,1)=="Plasma Sentinels","Unnamed title incorrectly trusted supplied draft string");
        auto foreign=named;foreign.selected_design=128;GT_ASSERT(Rejects([&]{unit::OpenSmacWorkshopRename(state,foreign,catalog,1);}),"Rename admitted foreign bank");GT_OK();
    });
    task->AddTest("SMAC Workshop Obsolete matches native toggles navigation and retained draft names",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream file(std::string(assets)+"/alpha.txt",std::ios::binary);if(!file) {throw std::runtime_error("Missing owned Obsolete rules");}
        const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        auto owner=Owner();owner.owned.fill(false);faction::SmacDesignCounts counts{};
        // Action6 controls in freeze471358e392d3dd4b3238bed3a2186aadd43764f251a54563630ce13e006b7b58.
        for(size_t i=0;i<3;++i) {
            auto pool=workshop_general_fixture::BasePool();pool[127]={};pool[128]={};
            const int selected=i==2 ? 2:64;
            if(i!=2) {pool[64]=pool[2];pool[64][38]=1;pool[64][48]=0xb;pool[64][49]=i==0 ? 2:0;pool[64][44]=i==1 ? 2:0;}
            auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto before=state;
            const std::string name=i==2 ? "":"Unsaved Guard";
            auto listed=state;auto listed_pool=pool;if(i==1) {listed_pool[64][44]=0;listed.RetainNativeRegistry(listed_pool,catalog);}
            const auto old_list=unit::BrowseSmacWorkshopDesigns(listed,catalog,tech,owner,counts);
            const auto result=unit::PrepareSmacWorkshopObsolete(state,selected,name,old_list,0,catalog,tech,owner,counts);
            const auto& after=*result.selection.state.native_registry;auto expected=pool[selected];
            expected[44]^=2;if(i==0) {expected[48]=3;expected[49]=0;}if(i==1) {expected[48]=0xb;expected[49]=2;}
            GT_ASSERT(after[selected]==expected && state==before,"Obsolete changed saved name/components or mutated its input");
            GT_ASSERT(result.selection.draft.selected_design==(i==2 ? 64:65) && result.selection.draft.armor==0
                && result.selection.draft.name==name,"Obsolete saved an unsaved tuple or lost inactive reselection name");
            const auto list=unit::BrowseSmacWorkshopDesigns(result.selection.state,catalog,tech,owner,counts);
            GT_ASSERT(list.at((result.strip_offset+5)%list.size())==result.selection.draft.selected_design,"Obsolete did not recenter in rebuilt list");
            auto rest=after;rest[selected]=pool[selected];GT_ASSERT(rest==pool,"Obsolete touched another saved record");
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(result.selection.state.Serialize(catalog).ToString(),catalog)==result.selection.state,"Obsolete did not survive save/restore");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop status matches nine native selection transitions without saving",GT(){
        // Native controls.json, frozen92d5a04d299620a805bf710345bc6fdc6e50079dbf8e095c6e3888958dc47d07.
        using K=unit::SmacWorkshopStatusKind;
        const std::array<K,9> kinds{K::Basic,K::BasedOnOld,K::Previous,K::New,K::New,K::BasedOnOld,K::Modifying,K::BasedOnOld,K::BasedOnOld};
        const std::array<int32_t,9> selections{2,2,64,64,64,67,64,66,68};
        for(size_t i=0;i<kinds.size();++i) {
            auto pool=workshop_general_fixture::BasePool();faction::SmacDesignCounts units{},production{};units.fill(1);
            for(size_t id=64;id<128;++id) {pool[id]=pool[2];pool[id][48]=1;pool[id][44]=0;}
            if(i==3) {pool[64][48]=0;}if(i==4) {pool[64][44]=2;}if(i==5) {pool[67][48]=0;}
            if(i>=7) {for(size_t id:{66u,68u}) {pool[id][44]=2;units[id]=0;production[id]=i==7 ? 1:0;}}
            const auto before=pool;
            unit::SmacWorkshopDraft draft{i<2 ? 2:64,0,0,uint32_t(i==0 || i==2 ? 0:1),0,1,"ignored name"};
            const auto result=unit::QuerySmacWorkshopStatus(pool,draft,1,units,production);
            GT_ASSERT(result.selected==selections[i] && result.kind==kinds[i] && pool==before,"Native status transition differs: "+std::to_string(i));
        }
        auto pool=workshop_general_fixture::BasePool();faction::SmacDesignCounts zero{};
        for(size_t id=64;id<128;++id) {pool[id]=pool[2];pool[id][48]=1;pool[id][44]=0;}
        unit::SmacWorkshopDraft changed{64,0,0,1,0,1,""};
        pool[66][44]=2;pool[66][43]=uint8_t(pool[64][43]+1);auto queued=zero;queued[66]=1;
        GT_ASSERT(unit::QuerySmacWorkshopStatus(pool,changed,1,zero,queued).kind==K::Modifying,"Queued wrong-role record became selected");
        GT_ASSERT(Rejects([&]{unit::QuerySmacWorkshopStatus(pool,changed,1,std::nullopt,queued);}),"Missing required cached counts became zero");
        pool[65][48]=0;
        GT_ASSERT(unit::QuerySmacWorkshopStatus(pool,changed,1,std::nullopt,std::nullopt).selected==65,"Inactive slot unnecessarily required caches");
        GT_ASSERT(Rejects([&]{unit::QuerySmacWorkshopStatus(pool,changed,2,zero,zero);}),"Foreign status bank accepted");GT_OK();
    });
    task->AddTest("SMAC Workshop ability chooser matches three native lists and nine cost fragments",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned ability rules");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        for(const auto& row:workshop_ability_fixture::Rows) {
            auto owner=Owner();owner.owned.fill(row.alltech);const auto context=unit::QuerySmacWorkshopEditContext(catalog,tech,owner,false);
            const unit::SmacWorkshopEditor editor{{64,0,1,2,262656,1,"Retained Draft"},row.slots};
            const auto result=unit::QuerySmacWorkshopAbilities(editor,row.slot,context,catalog);
            GT_ASSERT(result.guard==unit::SmacWorkshopAbilityMenu::Guard::None && result.choices==row.values,"Native ability row order/admission differs");
        }
        for(const auto& row:workshop_ability_fixture::Costs) {
            const auto cost=unit::QuerySmacWorkshopAbilityCost(row.code,0x400,0,2,3,1);GT_ASSERT(cost.has_value(),"Native cost fragment omitted");
            const std::array<const char*,7> formulas{"Attack/Defense","Attack-1","Defense-1","Speed-1","Attack+Defense-2","Attack+Speed-2","Defense+Speed-2"};
            auto text="Cost: "+std::to_string(cost->quantity);
            if(cost->formula<0) {text+=" ("+std::string(formulas.at(size_t(-cost->formula-1)))+")";}
            else if(cost->domain>=0) {text+=" (Land)";}
            GT_ASSERT(text==row.text,"Native ability cost differs: "+text);
        }
        auto owner=Owner();owner.owned.fill(false);auto context=unit::QuerySmacWorkshopEditContext(catalog,tech,owner,false);
        const unit::SmacWorkshopEditor missile{{64,8,20,0,0,1,"Missile"},{-1,-1}};
        GT_ASSERT(unit::QuerySmacWorkshopAbilities(missile,1,context,catalog).guard==unit::SmacWorkshopAbilityMenu::Guard::SecondTechnology
            && unit::QuerySmacWorkshopAbilities(missile,0,context,catalog).guard==unit::SmacWorkshopAbilityMenu::Guard::Missile,"Ability entry guard precedence differs");
        GT_ASSERT(!unit::QuerySmacWorkshopAbilityCost(-8,0,0,2,3,1)
            && Rejects([&]{unit::QuerySmacWorkshopAbilityCost(-1,0,0,2,0,1);}),"Invalid ratio/unknown annotation was normalized");GT_OK();
    });
    task->AddTest("SMAC Workshop stat formatter matches24 native strings four captions and eleven DATA movements",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned stat rules");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        for(const auto& row:workshop_data_fixture::Cases) {
            auto declared=catalog;
            declared.chassis.chassis.at(row.draft.chassis).speed=row.speed;
            declared.chassis.chassis.at(row.draft.chassis).triad=loader::txt::ChassisTriad(row.triad);
            declared.weapons.weapons.at(row.draft.weapon).mode=loader::txt::WeaponMode(row.mode);
            GT_ASSERT(unit::SmacWorkshopMovement(row.draft,declared)==row.expected,"Native DATA movement differs");
        }
        for(const auto& row:workshop_stats_fixture::Cases) {
            const auto actual=unit::FormatSmacWorkshopStats(row.draft,catalog);
            GT_ASSERT(actual==row.text,"Native stat string differs: "+actual+" expected "+row.text);
        }
        for(const auto& row:workshop_stats_fixture::Captions) {
            GT_ASSERT(unit::FormatSmacWorkshopStats(row.draft,catalog,true)==row.text,"Main stat ability caption differs");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop accepted picker mutations match fifteen native drafts and two entry guards",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);
        if(!input) {throw std::runtime_error("Missing owned picker rules");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;auto owner=Owner();owner.owned.fill(true);
        const auto equal=[](const unit::SmacWorkshopEditor& a,const unit::SmacWorkshopEditor& b) {
            return a.draft.selected_design==b.draft.selected_design && a.draft.chassis==b.draft.chassis && a.draft.weapon==b.draft.weapon
                && a.draft.armor==b.draft.armor && a.draft.reactor==b.draft.reactor && a.draft.abilities==b.draft.abilities
                && a.draft.name==b.draft.name && a.ability_slots==b.ability_slots;
        };
        for(const auto& row:workshop_picker_fixture::Cases) {
            const auto before=row.before;auto context=unit::QuerySmacWorkshopEditContext(catalog,tech,owner,row.automatic);
            context.second_ability=row.second;auto actual=before;bool admitted=true;
            if(row.kind==4) {auto result=unit::SetSmacWorkshopAbilitySlot(before,uint32_t(row.slot),row.choice,context,catalog);
                actual=std::move(result.editor);admitted=result.admitted;}
            else {actual=unit::ChangeSmacWorkshopComponent(before,static_cast<Kind>(row.kind),uint32_t(row.choice),context,catalog);}
            GT_ASSERT(equal(actual,row.after) && admitted==row.admitted && equal(before,row.before),"Native picker draft differs: "+std::string(row.name));
        }GT_OK();
    });
    task->AddTest("SMAC Workshop modal return keeps cancellation and reactor rating semantics",GT(){
        GT_ASSERT(unit::ResolveSmacWorkshopPickerResult(2,-1,true,Kind::Chassis)==2,"Cancellation installed last hover");
        GT_ASSERT(unit::ResolveSmacWorkshopPickerResult(2,4,false,Kind::Chassis)==2,"Unavailable card replaced original choice");
        GT_ASSERT(unit::ResolveSmacWorkshopPickerResult(2,2,true,Kind::Reactor)==3,"Reactor ordinal was not converted to rating");GT_OK();
    });
    task->AddTest("SMAC Workshop domain and PSI names match68 native strings and12 complete saved pools",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto read=[&](const char* name) {std::ifstream file(std::string(assets)+"/"+name,std::ios::binary);if(!file) {throw std::runtime_error("Missing owned domain naming rules");}return std::string(std::istreambuf_iterator<char>(file),{});};
        const auto source=read("alpha.txt");const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(read("labels.txt"));
        const auto pool_for=[](const auto& groups) {auto pool=workshop_general_fixture::BasePool();for(const auto& group:groups) {for(const auto id:group.first) {pool.at(id)=group.second;}}return pool;};
        for(const auto& test:workshop_domain_fixture::Names()) {const auto pool=pool_for(test.records);auto inputs=catalog;
            if(test.weapon_strength!=INT32_MIN) {inputs.weapons.weapons.at(test.draft.weapon).offense=test.weapon_strength;}
            if(test.armor_strength!=INT32_MIN) {inputs.defenses.defenses.at(test.draft.armor).rating=test.armor_strength;}
            GT_ASSERT(unit::NameSmacWorkshopDesign(pool,test.draft.selected_design,test.draft,inputs,test.owner)==test.expected,"Native domain name differs: "+std::string(test.id));}
        auto owner=Owner();owner.owned.fill(true);faction::SmacDesignCounts counts{};
        for(const auto& test:workshop_domain_fixture::Commits()) {
            auto state=Empty();state.RetainNativeRegistry(pool_for(test.records),catalog);const auto before=state;
            const auto result=unit::PrepareSmacWorkshopCommit(state,test.draft,catalog,tech,owner,counts,counts);
            GT_ASSERT(result.success && !result.confirmation && !result.retirement && result.result.design_id==test.selected && state==before,"Native domain commit changed selection/authority");
            const auto& pool=*result.state.native_registry;const auto& row=pool.at(test.selected);
            const std::string name(row.begin(),std::find(row.begin(),row.begin()+32,uint8_t(0)));
            uint64_t digest=UINT64_C(14695981039346656037);for(const auto& record:pool) {for(const auto byte:record) {digest=(digest^byte)*UINT64_C(1099511628211);}}
            GT_ASSERT(name==test.expected && digest==test.digest,"Native domain saved pool differs: "+std::string(test.id));
        }
        auto legacy=catalog;legacy.workshop_name_labels->at(7).clear();bool sea_checked=false;
        for(const auto& test:workshop_domain_fixture::Names()) {if(std::string(test.id)=="foil_former") {
            GT_ASSERT(Rejects([&]{unit::NameSmacWorkshopDesign(pool_for(test.records),test.draft.selected_design,test.draft,legacy,test.owner);}),"Legacy profile guessed a missing Sea label");sea_checked=true;}}
        GT_ASSERT(sea_checked,"Missing native Sea Former control");
        GT_OK();
    });
    task->AddTest("SMAC Workshop human proposal producer matches414 original submissions and all policy transitions",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream file(std::string(assets)+"/alpha.txt",std::ios::binary);if(!file) {throw std::runtime_error("Missing owned human Workshop rules");}
        const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        for(const auto& test:workshop_human_fixture::Cases()) {
            auto owner=Owner();owner.owned.fill(false);for(const auto id:test.technologies) {owner.owned.at(id)=true;}
            auto pool=workshop_general_fixture::BasePool();for(const auto& group:test.records) {for(const auto id:group.first) {pool.at(id)=group.second;}}
            auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto before=state;
            faction::SmacDesignCounts units{},production{};for(const auto& entry:test.units) {units.at(entry.first)=uint8_t(entry.second);}for(const auto& entry:test.production) {production.at(entry.first)=uint8_t(entry.second);}
            unit::SmacWorkshopDraft provisional{65,1,1,1,0,1,"Retained draft"};
            const auto result=unit::PrepareSmacWorkshopHumanPreview(state,provisional,catalog,tech,owner,test.context,units,production);
            GT_ASSERT(state==before && result.steps.size()==test.steps.size(),"Human producer changed caller or proposal count: "+std::string(test.name));
            const auto fields=[](const auto& draft) {return std::array<int32_t,5>{{int32_t(draft.chassis),int32_t(draft.weapon),int32_t(draft.armor),int32_t(draft.abilities),draft.reactor}};};
            for(size_t i=0;i<test.steps.size();++i) {const auto& actual=result.steps[i];const auto& expected=test.steps[i];
                GT_ASSERT(fields(actual.proposal)==expected.proposal && fields(actual.published)==expected.published && actual.category==expected.category
                    && actual.alias==expected.alias && actual.mode==expected.mode && actual.filter==expected.filter,"Native human proposal/policy differs: "+std::string(test.name)+" step="+std::to_string(i));
                GT_ASSERT(actual.published.selected_design==65 && actual.published.name=="Retained draft","Proposal changed UI selector/name");
            }
            uint64_t digest=UINT64_C(14695981039346656037);for(const auto& row:*result.state.native_registry) {for(const auto byte:row) {digest=(digest^byte)*UINT64_C(1099511628211);}}
            GT_ASSERT(digest==test.digest,"Human producer complete native pool differs: "+std::string(test.name));
            GT_ASSERT(result.state.owners.size()==state.owners.size() && result.state.retirement_history==state.retirement_history,"Preview changed owner/history authority");
            auto missing=test.context;missing.fields.erase(0x95d1ac);
            GT_ASSERT(Rejects([&]{unit::PrepareSmacWorkshopHumanPreview(state,provisional,catalog,tech,owner,missing,units,production);}) && state==before,"Missing faction authority became zero or mutated caller");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop browse keeps native raw IDs and retained-count ordering",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream file(std::string(assets)+"/alpha.txt",std::ios::binary);if(!file) {throw std::runtime_error("Missing owned Workshop browse rules");}
        const std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;auto owner=Owner();owner.owned.fill(false);
        auto pool=workshop_general_fixture::BasePool();pool[64]=pool[2];pool[66]=pool[2];pool[67]=pool[2];pool[128]=pool[2];
        pool[66][38]=1;pool[66][44]=2;pool[67][44]=2;pool[68][44]=2;pool[2][44]=2;
        auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto before=state;
        std::optional<faction::SmacDesignCounts> counts=faction::SmacDesignCounts{};counts->at(66)=1;
        std::vector<int32_t> expected{0,64};for(int id=65;id<128;++id) {if(id!=66 && id!=67) {expected.push_back(id);}}expected.insert(expected.end(),{66,67,2});
        GT_ASSERT(unit::BrowseSmacWorkshopDesigns(state,catalog,tech,owner,counts)==expected && state==before,"Browse changed inactive/excluded/intrinsic order, consulted custom technology or mutated registry");
        counts->at(66)=0;counts->at(67)=255;expected[expected.size()-3]=67;expected[expected.size()-2]=66;
        GT_ASSERT(unit::BrowseSmacWorkshopDesigns(state,catalog,tech,owner,counts)==expected,"Unsigned retained count did not determine excluded-own priority");
        GT_ASSERT(Rejects([&]{unit::BrowseSmacWorkshopDesigns(state,catalog,tech,owner,std::nullopt);}) && state==before,"Missing count authority was silently recounted or fabricated");
        pool[66][44]=0;pool[67][44]=0;state.RetainNativeRegistry(pool,catalog);
        const auto independent=unit::BrowseSmacWorkshopDesigns(state,catalog,tech,owner,std::nullopt);
        GT_ASSERT(independent[0]==0 && independent[1]==64 && independent[2]==66 && independent[3]==67 && independent.back()==2
            && std::find(independent.begin(),independent.end(),128)==independent.end(),"Included custom rows depended on cache/technology or admitted a foreign bank");
        owner.owned.fill(true);const auto all=unit::BrowseSmacWorkshopDesigns(state,catalog,tech,owner,std::nullopt);
        for(int id:{8,9,10}) {GT_ASSERT(std::find(all.begin(),all.end(),id)==all.end(),"PSI intrinsic entered conventional browse list");}GT_OK();
    });
    task->AddTest("SMAC Workshop active and New reselection preserves the original draft and raw-record rules",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto read=[&](const char* name) {std::ifstream file(std::string(assets)+"/"+name,std::ios::binary);if(!file) {throw std::runtime_error("Missing owned Workshop selection rules");}return std::string(std::istreambuf_iterator<char>(file),{});};
        const auto source=read("alpha.txt");const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(read("labels.txt"));
        auto pool=workshop_general_fixture::BasePool();pool[64]=pool[2];pool[64][38]=1;pool[64][48]=0x83;pool[64][49]=2;pool[64][47]=0xa5;pool[64][50]=0x34;pool[64][51]=0x12;
        const std::string stored="Stored Guard";std::fill_n(pool[64].begin(),32,0);std::copy(stored.begin(),stored.end(),pool[64].begin());
        pool[65]=pool[127];pool[65][48]=0x82;pool[65][49]=2;
        auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto before=state;
        const auto active=unit::PrepareSmacWorkshopReselection(state,64,"Unsaved",catalog,1);auto expected=pool;expected[64][48]&=uint8_t(~0x80u);
        GT_ASSERT(state==before && *active.state.native_registry==expected && active.draft.selected_design==64 && active.draft.armor==1
            && active.draft.name==stored && active.draft.chassis==0 && active.draft.weapon==0 && active.draft.reactor==1,"Active selection lost components/name, opaque bytes or raw flags");
        for(int requested:{-1,65,127}) {
            const auto fresh=unit::PrepareSmacWorkshopReselection(state,requested,"Old Draft",catalog,1);
            GT_ASSERT(fresh.state==before && fresh.draft.selected_design==65 && fresh.draft.name=="Old Draft" && fresh.draft.chassis==0 && fresh.draft.weapon==0
                && fresh.draft.armor==0 && fresh.draft.reactor==1 && fresh.draft.abilities==0 && fresh.ability_slots[0]==-1 && fresh.ability_slots[1]==-1,"New selection copied stale fields, changed flags/name or kept the clicked empty ID");
        }
        auto with_mask=pool;const uint32_t mask=(1u<<3)|(1u<<9)|(1u<<18);for(unsigned i=0;i<4;++i) {with_mask[64][32+i]=uint8_t(mask>>(8*i));}
        state.RetainNativeRegistry(with_mask,catalog);const auto abilities=unit::PrepareSmacWorkshopReselection(state,64,"Ignored",catalog,1);
        GT_ASSERT(abilities.draft.abilities==mask && abilities.ability_slots[0]==3 && abilities.ability_slots[1]==9,"Selection incorrectly cleaned the saved mask or preserved old selector order");
        auto intrinsic=pool;intrinsic[2][48]|=0x80;state.RetainNativeRegistry(intrinsic,catalog);
        const auto scout=unit::PrepareSmacWorkshopReselection(state,2,"Ignored",catalog,1);intrinsic[2][48]&=uint8_t(~0x80u);
        GT_ASSERT(*scout.state.native_registry==intrinsic && scout.draft.selected_design==2 && scout.draft.name.empty(),"Intrinsic reselect copied a name or altered other records");
        const auto psi=unit::PrepareSmacWorkshopReselection(state,8,"Old Draft",catalog,1);
        GT_ASSERT(psi.draft.selected_design==65 && psi.draft.name=="Old Draft" && psi.state==state,"Negative-power intrinsic did not fall back to New");
        auto automatic=pool;const std::string name="Synthmetal Garrison";std::fill_n(automatic[64].begin(),32,0);std::copy(name.begin(),name.end(),automatic[64].begin());
        state.RetainNativeRegistry(automatic,catalog);const auto normalized=unit::PrepareSmacWorkshopReselection(state,64,"Ignored",catalog,1);automatic[64][48]&=uint8_t(~0x82u);
        GT_ASSERT(normalized.draft.name.empty() && *normalized.state.native_registry==automatic,"Automatic stored name did not clear only named/selection flags");
        for(int id=64;id<128;++id) {pool[id]=pool[2];}state.RetainNativeRegistry(pool,catalog);
        const auto full=unit::PrepareSmacWorkshopReselection(state,-1,"Ignored",catalog,1);
        GT_ASSERT(full.draft.selected_design==64 && full.draft.name.empty(),"Exhausted New search did not fall back to the first owner record");
        state.owners.push_back({7,{},2});state.SynchronizeNativeRegistry();const auto seven=unit::PrepareSmacWorkshopReselection(state,511,"Seven",catalog,7);
        GT_ASSERT(seven.draft.selected_design==448 && seven.draft.name=="Seven" && seven.state==state,"Owner7 New selection used another bank");
        GT_ASSERT(Rejects([&]{unit::PrepareSmacWorkshopReselection(state,128,"",catalog,1);}) && Rejects([&]{unit::PrepareSmacWorkshopReselection(state,64,std::string(32,'x'),catalog,1);}),"Foreign selection or overlong retained draft was admitted");GT_OK();
    });
    task->AddTest("SMAC Workshop retained common commits match seven complete native saved pools",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto read=[&](const char* name) {std::ifstream file(std::string(assets)+"/"+name,std::ios::binary);if(!file) {throw std::runtime_error("Missing owned Workshop rules");}return std::string(std::istreambuf_iterator<char>(file),{});};
        const auto source=read("alpha.txt");const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(read("labels.txt"));
        auto owner=Owner();owner.owned.fill(true);faction::SmacDesignCounts counts{};
        auto pool=workshop_general_fixture::BasePool();pool[127]={};pool[128]={70,111,114,101,105,103,110,32,71,117,97,114,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,2,2,2,0,4,255,0,3,0,0,0};
        auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto original=state;
        for(const auto& test:workshop_commit_fixture::CASES) {
            const auto result=unit::PrepareSmacWorkshopCommit(state,test.draft,catalog,tech,owner,counts,counts);
            GT_ASSERT(result.success && !result.confirmation && !result.retirement && result.result.design_id==test.selected && state==original,"Native slot decision or detached authority changed");
            uint64_t hash=UINT64_C(14695981039346656037);for(const auto& row:*result.state.native_registry) {for(const auto byte:row) {hash=(hash^byte)*UINT64_C(1099511628211);}}
            GT_ASSERT(hash==test.digest,"Native saved pool differs: "+std::string(test.name)+" hash="+std::to_string(hash));
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(result.state.Serialize(catalog).ToString(),catalog)==result.state,"Prepared general commit lost state on restore");
        }
        // Full own bank: the cached live count, not queue references alone, gates retirement.
        for(size_t id=64;id<128;++id) {pool[id]=pool[128];}state=Empty();state.retirement_history[1]=unit::SmacRetirementHistory::NewWorld();state.RetainNativeRegistry(pool,catalog);
        unit::SmacWorkshopDraft draft;draft.selected_design=64;draft.armor=2;counts[64]=1;const auto full=state;
        const auto pending=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts);
        GT_ASSERT(!pending.success && pending.confirmation && !pending.retirement && pending.state==full && state==full,"Pending overwrite changed saved records/history");
        const auto accepted=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts,true);
        unit::SmacRetirementTuple old{};std::copy_n(pool[64].begin()+32,8,old.begin());
        GT_ASSERT(accepted.success && accepted.retirement==64 && accepted.state.retirement_history[1]->cursor==1 && accepted.state.retirement_history[1]->slots[0]==old
            && accepted.state.native_registry->at(128)==pool[128] && state==full,"Accepted retirement omitted old tuple or changed foreign/source authority");
        auto prototyped=state;prototyped.owners[0].designs[0].prototyped=true;prototyped.SynchronizeNativeRegistry();
        auto reactor_edit=draft;reactor_edit.armor=1;reactor_edit.reactor=2;
        const auto replacement=unit::PrepareSmacWorkshopCommit(prototyped,reactor_edit,catalog,tech,owner,counts,counts,true);
        GT_ASSERT(replacement.retirement==64 && !replacement.state.owners[0].designs[0].prototyped,
            "Retired source remained active during the new prototype-coverage scan");
        counts[64]=0;faction::SmacDesignCounts queued{};queued[64]=9;
        const auto silent=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,queued);
        GT_ASSERT(silent.success && !silent.confirmation && !silent.retirement && silent.state.retirement_history==state.retirement_history,"Queue-only silent edit incorrectly retired/history-recorded a design");
        // Matching a custom tuple copies only the source-controlled name and NUL.
        auto rename_pool=pool;rename_pool[65][38]=2;state=Empty();state.RetainNativeRegistry(rename_pool,catalog);draft.name="X";owner.owned.fill(false); // existing own aliases do not require component technologies
        const auto alias=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,queued);
        auto expected=rename_pool[65];expected[0]='X';expected[1]=0;
        GT_ASSERT(alias.success && alias.result.design_id==65 && alias.state.native_registry->at(65)==expected && alias.state.native_registry->at(64)==rename_pool[64],"Short alias rename erased its native name tail or source");
        // Fresh prototype construction has its own full-name clear policy.
        auto riot_pool=workshop_general_fixture::BasePool();riot_pool[127]={};riot_pool[128]=pool[128];riot_pool[128][48]=7;
        riot_pool[64].fill(0xa5);riot_pool[64][48]=0;riot_pool[64][49]=0;
        state=Empty();state.retirement_history[1]=unit::SmacRetirementHistory::NewWorld();state.RetainNativeRegistry(riot_pool,catalog);
        GT_ASSERT(state.RequestRiotPrototype(128,1,false,catalog,true)==64,"Fresh retained riot prototype selected another slot");
        std::array<uint8_t,32> cleared{};std::copy_n(riot_pool[128].begin(),13,cleared.begin());
        GT_ASSERT(std::equal(cleared.begin(),cleared.end(),state.native_registry->at(64).begin()) && state.native_registry->at(64)[47]==0xa5
            && state.native_registry->at(64)[50]==0xa5 && state.native_registry->at(64)[51]==0xa5,"Fresh constructor did not clear name or lost destination opaque/prerequisite bytes");GT_OK();
    });
    task->AddTest("SMAC Workshop intrinsic warnings distinguish admitted matches and exhausted-scan fallback",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        const auto read=[&](const char* name) {std::ifstream file(std::string(assets)+"/"+name,std::ios::binary);return std::string(std::istreambuf_iterator<char>(file),{});};
        const auto source=read("alpha.txt");const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(read("labels.txt"));
        auto owner=Owner();owner.owned.fill(true);faction::SmacDesignCounts counts{};
        auto pool=workshop_general_fixture::BasePool();for(size_t id=64;id<128;++id) {pool[id]=pool[2];}
        auto state=Empty();state.RetainNativeRegistry(pool,catalog);const auto before=state;
        using Notice=unit::SmacWorkshopNotice;unit::SmacWorkshopDraft draft;draft.armor=2;
        const auto blocked=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts);
        GT_ASSERT(!blocked.success && !blocked.confirmation && blocked.notice==Notice::IntrinsicType && blocked.state==before && !blocked.retirement,"Exhausted intrinsic replacement did not preserve its typed warning and state");
        draft.armor=0;draft.name="Different intrinsic";
        const auto named=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts);
        GT_ASSERT(named.success && named.notice==Notice::IntrinsicName && named.state==before && named.result.design_id==2,"Admitted selected intrinsic warning incorrectly rejected its successful match");
        for(const auto& name:{std::string(),catalog.source.units.at(2).name}) {
            draft.name=name;const auto result=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts);
            GT_ASSERT(result.success && result.notice==Notice::None && result.state==before,"Unchanged admitted intrinsic acquired an invented warning");
        }
        // The distinct return-zero branch requires exhausting the entire alias
        // scan. A disabled retained intrinsic and an empty own bank establish it.
        for(auto& row:catalog.units) {if(row.source_id==2) {row.prerequisite=tech.ResolveToken("Disable");}}
        pool=workshop_general_fixture::BasePool();for(size_t id=64;id<128;++id) {pool[id]={};}pool[2][50]=254;pool[2][51]=255;
        state=Empty();state.RetainNativeRegistry(pool,catalog);
        for(const auto& name:{std::string(),std::string("Different intrinsic")}) {
            draft.name=name;const auto result=unit::PrepareSmacWorkshopCommit(state,draft,catalog,tech,owner,counts,counts);
            GT_ASSERT(!result.success && result.notice==(name.empty() ? Notice::UnchangedIntrinsic:Notice::IntrinsicName) && result.state==state && !result.retirement,"Exhausted unchanged intrinsic lost its return-zero/name-warning distinction");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop intrinsic reactivation matches four native shared-record controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned alpha for intrinsic controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        for(const auto& test:intrinsic_overlay_fixture::CASES) {
            auto state=Empty();for(const auto& row:test.custom) {state.owners.at(row.id/64-1).designs.push_back(row);}
            for(size_t i=0;i<64;++i) {state.intrinsic_overrides[i]=test.before[i];}
            auto expected=state;for(size_t i=0;i<64;++i) {expected.intrinsic_overrides[i]=test.after[i];}
            auto owner=Owner();owner.admission.simulation_id=test.owner;unit::SmacWorkshopDraft draft;draft.selected_design=test.selected;
            const auto result=unit::ReuseSmacWorkshopDesign(state,draft,catalog,tech,owner);
            GT_ASSERT(result && result->design_id==2 && !result->intrinsic_name_warning && state==expected,"native global intrinsic overlay or unrelated bank changed");
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(state.Serialize(catalog).ToString(),catalog)==state,"intrinsic flags/exclusions lost on restore");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop picker keeps unknown cards and source identities",GT(){
        auto catalog=smac_startup_fixture::Units().catalog;auto tech=smac_rules_fixture::Technologies();auto owner=Owner();
        for(auto& row:catalog.chassis.chassis) {row.prerequisite="Tech5";}catalog.chassis.chassis[0].prerequisite="None";
        auto choices=unit::SmacWorkshopChoices(Kind::Chassis,catalog,tech,owner);
        GT_ASSERT(choices.size()==9 && choices[0].technology_available && !choices[1].technology_available,"Unknown-tech cards were filtered out");
        catalog.chassis.chassis[1].prerequisite="Disable";
        choices=unit::SmacWorkshopChoices(Kind::Chassis,catalog,tech,owner);
        GT_ASSERT(choices.size()==8 && choices[1].component_id==2,"Visible ordinal replaced component identity");
        catalog.reactors.reactors[1].prerequisite="Disable";catalog.reactors.reactors[2].prerequisite="Tech1";
        choices=unit::SmacWorkshopChoices(Kind::Reactor,catalog,tech,owner);
        GT_ASSERT(choices.size()==2 && choices[0].component_id==1 && choices[1].component_id==3 && choices[1].technology_available,"Reactor mapping lost its one-based identity");
        tech.technologies[1].prerequisites={"Disable","None"};
        GT_ASSERT(!unit::SmacWorkshopChoices(Kind::Reactor,catalog,tech,owner)[1].technology_available,"Raw ownership bypassed effective technology query");
        catalog.chassis.chassis[2].prerequisite="Unknown";
        GT_ASSERT(Rejects([&]{unit::SmacWorkshopChoices(Kind::Chassis,catalog,tech,owner);}),"Unresolved source silently admitted");
        owner.admission.simulation_id=32;
        GT_ASSERT(Rejects([&]{unit::SmacWorkshopChoices(Kind::Armor,catalog,tech,owner);}),"Invalid owner reached bit shift");GT_OK();
    });
    task->AddTest("SMAC Workshop first commit reuses Scout or retains named Synthmetal in last slot",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();const auto owner=Owner();
        auto state=Empty();const auto untouched=state;unit::SmacWorkshopDraft draft;draft.name="Different intrinsic name";
        const auto reuse=unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);
        GT_ASSERT(reuse.design_id==2 && reuse.intrinsic_name_warning && state==untouched,"Renaming an intrinsic tuple allocated a duplicate");
        for(const std::string name:{"","Test Guard"}) {
            state=Empty();draft.armor=1;draft.name=name;
            const auto result=unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);
            const auto& design=state.Find(1)->designs.at(0);
            // Original 441e20 controls: ids 2/127, raw costs 1/2, plan 2,
            // flags 1/3 for automatic/supplied names. Production cost is separate.
            GT_ASSERT(result.design_id==127 && !result.intrinsic_name_warning && design.id==127 && design.cost==2 && design.plan==2
                && design.chassis==0 && design.weapon==0 && design.defense==1 && design.reactor==1 && !design.abilities
                && design.active && !design.prototyped && !design.faction_exclusion && !design.carry && !design.riot
                && design.name && design.explicitly_named==!name.empty() && state.Find(2)->designs.empty() && state.Find(1)->starting_scout==2,
                "Native first-path identity, cached fields or unrelated bank changed");
            const auto applied=state;
            GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==127 && state==applied,"Repeated Apply allocated another copy");
            const auto wire=state.Serialize(catalog).ToString();
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(wire,catalog)==state,"Committed name/flag lost on save");
            for(size_t n=0;n<wire.size();++n) {GT_ASSERT(Rejects([&]{unit::SmacStartupDesignState::Deserialize(wire.substr(0,n),catalog);}),"Truncated named design accepted");}
            GT_ASSERT(Rejects([&]{unit::SmacStartupDesignState::Deserialize(wire+"x",catalog);}),"Trailing named design accepted");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop reuses unnamed startup tuples and preserves supplied-name alias rules",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();const auto owner=Owner();
        auto state=Empty();unit::SmacCustomDesign existing;existing.id=64;existing.defense=1;existing.cost=2;existing.plan=2;existing.prototyped=true;
        state.owners[0].designs.push_back(existing);const auto before=state;
        unit::SmacWorkshopDraft draft;draft.armor=1;draft.name="Different direct draft";
        GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==64 && state==before,"Stored-name requirement duplicated a startup tuple or direct draft renamed it");
        const auto label=unit::SmacWorkshopDesignName(existing,catalog,1);
        GT_ASSERT(!label.empty() && state==before,"Read-only legacy name query mutated registry");
        state.owners[0].designs[0].name="Retained Guard";state.owners[0].designs[0].explicitly_named=true;draft.selected_design=64;
        GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==64
            && state.owners[0].designs[0].name==std::optional<std::string>("Different direct draft") && state.owners[0].designs[0].prototyped,"Named same-tuple alias rebuilt prototype metadata");
        const auto named=state;draft.selected_design=191;
        GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);}) && state==named,"Foreign selected design changed registry");GT_OK();
    });
    task->AddTest("SMAC Workshop V4 retains auxiliary prototype flags without changing V1 V2 V3 bytes",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();const auto owner=Owner();
        auto state=Empty();unit::SmacWorkshopDraft draft;draft.armor=1;unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);
        const auto legacy=state.Serialize(catalog).ToString();state.owners[0].designs[0].auxiliary_flags=0xa508;
        const auto wire=state.Serialize(catalog).ToString();const auto restored=unit::SmacStartupDesignState::Deserialize(wire,catalog);
        GT_ASSERT(restored==state && wire!=legacy,"Auxiliary flags were lost or changed known prototype fields");
        auto clear=restored;clear.owners[0].designs[0].auxiliary_flags=0;
        GT_ASSERT(clear.Serialize(catalog).ToString()==legacy,"V4-only metadata changed earlier codec bytes");
        for(size_t n=0;n<wire.size();++n) {GT_ASSERT(Rejects([&]{unit::SmacStartupDesignState::Deserialize(wire.substr(0,n),catalog);}),"Truncated V4 accepted");}
        state.owners[0].designs[0].auxiliary_flags=1;
        GT_ASSERT(Rejects([&]{state.Serialize(catalog);}),"Auxiliary flags duplicated active/name/prototype authority");GT_OK();
    });
    task->AddTest("SMAC Workshop failed edits preserve previously applied design",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;const auto tech=smac_rules_fixture::Technologies();auto owner=Owner();
        auto state=Empty();unit::SmacWorkshopDraft draft;draft.armor=1;
        owner.owned[1]=false;const auto empty=state;
        GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);}) && state==empty,"Unavailable armor mutated registry");
        owner.owned[1]=true;unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner);const auto applied=state;
        for(unsigned change=0;change<7;++change) {
            auto bad=draft;
            switch(change) {case 0:bad.chassis=1;break;case 1:bad.reactor=2;break;case 2:bad.abilities=1;break;
                case 3:bad.armor=2;break;case 4:bad.name=std::string(32,'x');break;case 5:bad.name=std::string("a\0b",3);break;case 6:bad.selected_design=191;break;}
            GT_ASSERT(Rejects([&]{unit::CommitSmacWorkshopFirstDesign(state,bad,catalog,tech,owner);}) && state==applied,"Unsupported edit changed applied design");
        }
        auto invalid=state;invalid.owners[0].designs[0].explicitly_named=true;invalid.owners[0].designs[0].name.reset();
        GT_ASSERT(Rejects([&]{invalid.Serialize(catalog);}),"Custom-name flag without a retained name accepted");GT_OK();
    });
    task->AddTest("SMAC Workshop free prototype uses presence beyond generic record budget",GT(){
        settings::SmacFactionStartupRule faction;
        const auto technologies=smac_rules_fixture::Technologies();
        GT_ASSERT(!faction.FreePrototype(),"Absent keyword granted prototype exemption");
        for(int n=0;n<8;++n) {faction.bonuses.push_back(settings::SmacFactionStartupBonus::Parse("FACILITY","17",technologies));}
        for(const std::string value:{"0","-4","1","not a number"}) {
            faction.bonuses.push_back(settings::SmacFactionStartupBonus::Parse("FREEPROTO",value,technologies));
            GT_ASSERT(faction.FreePrototype() && faction.Records().size()==8,"FREEPROTO used numeric value or consumed a generic slot");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop prototype price guards legacy metadata and ability-free basis",GT(){
        const auto catalog=smac_startup_fixture::Units().catalog;
        unit::SmacCustomDesign design;design.id=127;design.defense=1;design.cost=2;
        unit::SmacPrototypePriceContext context{3,false,false,std::array<int32_t,3>{{50,50,50}}};
        const auto ordinary=unit::SmacDesignProductionRows(design,catalog,context);
        GT_ASSERT(ordinary.rows==3 && ordinary.surcharge_present,"First Synthmetal prototype lost surcharge");
        design.abilities=0xffffffffu;design.cost=20;
        GT_ASSERT(unit::SmacDesignProductionRows(design,catalog,context).rows==21,"Surcharge used ability-inflated stored cost");
        context.percentages.reset();GT_ASSERT(Rejects([&]{unit::SmacDesignProductionRows(design,catalog,context);}),"Old profile invented percentages");
        context.free_prototype=true;GT_ASSERT(unit::SmacDesignProductionRows(design,catalog,context).rows==20,"Free prototype used absent percentages");
        context.free_prototype=false;design.prototyped=true;
        GT_ASSERT(!unit::SmacDesignProductionRows(design,catalog,context).surcharge_present,"Completed prototype required new metadata");
        design.prototyped=false;context.percentages=std::array<int32_t,3>{{-1,50,50}};
        GT_ASSERT(Rejects([&]{unit::SmacDesignProductionRows(design,catalog,context);}),"Negative percentage admitted");
        context.percentages=std::array<int32_t,3>{{INT32_MAX,50,50}};
        GT_ASSERT(Rejects([&]{unit::SmacDesignProductionRows(design,catalog,context);}),"Overflowed native signed32 product admitted");GT_OK();
    });
    task->AddTest("SMAC Workshop owned allocation scans and overwrite prompts match original decisions",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned allocation rules");}
        const auto technologies=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(std::string(std::istreambuf_iterator<char>(input),{}));const auto owner=Owner();
        for(const auto& row:workshop_allocation_fixture::Cases()) {
            auto pool=workshop_allocation_fixture::BasePool();for(const auto& patch:row.patches) {pool.at(patch.first/52).at(patch.first%52)=patch.second;}
            faction::SmacDesignCounts units{},production{};for(const auto& item:row.units) {units.at(item.first)=item.second;}for(const auto& item:row.production) {production.at(item.first)=item.second;}
            const auto before=pool;const auto result=unit::ChooseSmacWorkshopSlot(pool,row.draft,units,production,technologies,owner);
            GT_ASSERT(result.selected==row.selected && result.kind==row.kind && result.confirmation==row.confirmation && pool==before,"Original slot selection differs: " + std::string(row.name));
        }GT_OK();
    });
    task->AddTest("SMAC Workshop owned general builder compatibility and cleanup match native controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);if(!input) {throw std::runtime_error("Missing owned Workshop rules");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto technologies=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        auto catalog=settings::SmacPredefinedUnitRules::Parse(source,technologies).catalog;
        std::ifstream labels_input(std::string(assets)+"/labels.txt",std::ios::binary);if(!labels_input) {throw std::runtime_error("Missing owned Workshop labels");}
        catalog.workshop_name_labels=unit::ReadSmacWorkshopNameLabels(std::string(std::istreambuf_iterator<char>(labels_input),{}));auto owner=Owner();owner.owned.fill(true);
        for(const auto& row:workshop_general_fixture::Builders()) {
            auto pool=workshop_general_fixture::BasePool();for(const auto& patch:row.patches) {pool.at(patch.first/52).at(patch.first%52)=patch.second;}
            unit::BuildSmacWorkshopRecord(pool,127,row.draft,catalog,technologies,owner);
            uint64_t digest=UINT64_C(14695981039346656037);for(const auto& record:pool) {for(const auto byte:record) {digest=(digest^byte)*UINT64_C(1099511628211);}}
            GT_ASSERT(unit::NameSmacWorkshopDesign(pool,127,row.draft,catalog,1)==row.native_name,"Original automatic name differs: " + std::string(row.name));
            GT_ASSERT(digest==row.digest,"Complete original design pool differs: " + std::string(row.name));
        }
        for(const auto& row:workshop_general_fixture::COMPATIBILITY) {
            unit::SmacWorkshopDraft draft;draft.chassis=row.chassis;draft.weapon=row.weapon;draft.armor=row.armor;
            GT_ASSERT(unit::SmacWorkshopAbilityCompatible(row.ability,draft,catalog)==row.compatible,"Original ability compatibility differs");
        }
        for(const auto& row:workshop_general_fixture::Cleanups()) {
            const auto actual=unit::CleanSmacWorkshopAbilities(row.draft,{22,23},row.second,catalog);
            GT_ASSERT(actual.mask==row.mask && actual.slots==row.slots,"Original ability cleanup differs: " + std::string(row.name));
        }
        auto pool=workshop_general_fixture::BasePool();const auto before=pool;unit::SmacWorkshopDraft invalid;invalid.weapon=999;
        GT_ASSERT(Rejects([&]{unit::BuildSmacWorkshopRecord(pool,127,invalid,catalog,technologies,owner);}) && pool==before,"Invalid builder input changed pool");
        GT_OK();
    });
    task->AddTest("SMAC Workshop owned original 927 prototype price receipts",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);
        if(!input) {throw std::runtime_error("Missing owned alpha.txt for prototype controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        size_t ordinal=0;
        for(const auto& row:smac_prototype_price_fixture::CASES) {
            unit::SmacCustomDesign design;design.id=row.id;design.chassis=row.chassis;design.weapon=row.weapon;
            design.defense=row.armor;design.reactor=row.reactor;design.cost=row.cost;design.prototyped=row.prototyped;
            const auto result=unit::SmacDesignProductionRows(design,catalog,{row.difficulty,row.free,row.skunkworks,row.percentages});
            GT_ASSERT(result.rows==row.rows && result.surcharge_present==row.surcharge,"Original prototype price differs at receipt "+std::to_string(ordinal));++ordinal;
        }GT_OK();
    });
    task->AddTest("SMAC Workshop owned original six complete custom alias and name transfers",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);
        if(!input) {throw std::runtime_error("Missing owned alpha.txt for Workshop alias controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;
        const auto owner=Owner();
        for(const auto& row:workshop_alias_fixture::Cases()) {
            auto state=Empty();state.owners[0].designs=row.before;auto expected=state;expected.owners[0].designs=row.after;
            const auto result=unit::ReuseSmacWorkshopDesign(state,row.draft,catalog,tech,owner);
            GT_ASSERT(result && result->design_id==row.selected && result->intrinsic_name_warning==row.warning && state==expected,"Complete native alias differs for "+std::string(row.id));
            GT_ASSERT(unit::SmacStartupDesignState::Deserialize(state.Serialize(catalog).ToString(),catalog)==state,"Native alias metadata lost on save");
        }GT_OK();
    });
    task->AddTest("SMAC Workshop owned original component and commit controls",GT(){
        const auto* assets=std::getenv("GLSMAC_STARTUP_ASSETS");if(!assets || !*assets) {GT_OK();}
        std::ifstream input(std::string(assets)+"/alpha.txt",std::ios::binary);
        if(!input) {throw std::runtime_error("Missing owned alpha.txt for Workshop controls");}
        const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto tech=loader::txt::SmacRulesTXTLoader::ParseTechnologyRules(source);
        const auto catalog=settings::SmacPredefinedUnitRules::Parse(source,tech).catalog;const auto owner=Owner();
        const std::array<size_t,4> sizes{9,20,10,4};size_t kind=0;
        for(const auto picker:{Kind::Chassis,Kind::Weapon,Kind::Armor,Kind::Reactor}) {
            const auto choices=unit::SmacWorkshopChoices(picker,catalog,tech,owner);
            GT_ASSERT(choices.size()==sizes[kind],"Original visible-card count differs");
            for(size_t i=0;i<choices.size();++i) {
                const bool expected=i==0 || (kind==1 && i==14) || (kind==2 && i==1);
                GT_ASSERT(choices[i].technology_available==expected,"Original card admission differs");
            }++kind;
        }
        for(const std::string name:{"","Test Guard"}) {
            auto state=Empty();unit::SmacWorkshopDraft draft;draft.armor=1;draft.name=name;
            GT_ASSERT(unit::CommitSmacWorkshopFirstDesign(state,draft,catalog,tech,owner).design_id==127,"Original allocation differs");
            const auto& row=state.Find(1)->designs[0];
            GT_ASSERT(row.cost==2 && row.plan==2 && row.explicitly_named==!name.empty(),"Original Synthmetal record differs");
        }GT_OK();
    });
}
}
