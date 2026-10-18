#include "SmacHurry.h"
#include "fixtures/SmacHurry.h"
#include "game/backend/base/SmacHurry.h"
#include "task/gsetests/GSETests.h"
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend::base;
ProductionItem Item(int id) {
    const auto kind=id>=0 ? ProductionItemKind::Unit : id<=-39 ? ProductionItemKind::Project
        : id==-38 ? ProductionItemKind::Stockpile : id<=-34 ? ProductionItemKind::Orbital : ProductionItemKind::Facility;
    return {kind,kind==ProductionItemKind::Stockpile ? "" : std::to_string(id)};
}
template<class F> bool Rejects(F call) {try {call();}catch(const std::exception&) {return true;}return false;}
}
void AddSmacHurryTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC Hurry32 literal native quote payment guard and baseline controls",GT() {
        for(const auto& row:fixtures::hurry_cases) {
            ProductionState state{Item(row.item),row.stored,Item(-1),7};state.hurry_partial_paid=row.marker;
            const auto original=state;const SmacHurryContext context{row.cost,row.factor,row.exemption,row.percent,row.voice,row.blocked,row.riots};
            const auto result=PrepareSmacHurryPayment(state,context,row.treasury,row.reserved,row.reserve_payment,static_cast<SmacHurryChoice>(row.choice),row.entered);
            GT_ASSERT(result.quote==row.quote && result.production.minerals==row.stored_after && result.production.baseline_minerals==row.baseline_after
                && result.production.baseline_item==Item(row.baseline_item_after) && result.production.hurry_partial_paid==row.marker_after
                && result.treasury==row.treasury_after && result.reserved==row.reserved_after && state==original,
                "native Hurry literal mismatch or input mutation: "+std::string(row.id));
        }GT_OK();
    });
    task->AddTest("SMAC Hurry codec retains partial marker and admits explicit pre-command port frames",GT() {
        const ProductionItem item{ProductionItemKind::Unit,"smac:1:2",2};
        ProductionState state{item,20,item,7,std::vector<ProductionItem>{item}};state.hurry_partial_paid=true;
        auto encoded=SerializeProductionState(state);GT_ASSERT(DeserializeProductionState(encoded)==state,"partial marker was not saved");
        types::Buffer old;
        const auto write_item=[&]{old.WriteInt(static_cast<int64_t>(item.kind));old.WriteBool(true);old.WriteInt(2);old.WriteString(item.id);};
        old.WriteBool(true);write_item();old.WriteInt(20);write_item();old.WriteInt(7);old.WriteBool(true);old.WriteInt(1);write_item();
        const auto legacy_bytes=old.ToString();auto decoded=DeserializeProductionState(old);
        state.hurry_partial_paid=false;GT_ASSERT(decoded==state,"pre-Hurry port frame fabricated a partial purchase");
        types::Buffer unknown(legacy_bytes);unknown.WriteInt(2);unknown.WriteBool(true);
        GT_ASSERT(Rejects([&]{DeserializeProductionState(unknown);}),"unknown Hurry extension accepted");
        types::Buffer short_frame(legacy_bytes);short_frame.WriteInt(1);
        GT_ASSERT(Rejects([&]{DeserializeProductionState(short_frame);}),"truncated Hurry marker accepted");
        auto extra=SerializeProductionState(state);extra.WriteBool(false);
        GT_ASSERT(Rejects([&]{DeserializeProductionState(extra);}),"trailing Hurry data accepted");GT_OK();
    });
    task->AddTest("SMAC Hurry checked arithmetic refuses unverified overflowing boundaries",GT() {
        ProductionState state{Item(0),0,Item(-1),0};SmacHurryContext context{INT32_MAX,10,10,100,false,false,false};
        const auto original=state;GT_ASSERT(Rejects([&]{CalculateSmacHurryQuote(state,context);}) && state==original,"overflow wrapped or mutated production");
        context.total_cost=40;context.hurry_percent=INT32_MAX;
        GT_ASSERT(Rejects([&]{CalculateSmacHurryQuote(state,context);}),"percentage multiplication overflow accepted");
        context.hurry_percent=100;context.mineral_factor=0;
        GT_ASSERT(Rejects([&]{CalculateSmacHurryQuote(state,context);}),"missing mineral authority substituted");GT_OK();
    });
}
}
