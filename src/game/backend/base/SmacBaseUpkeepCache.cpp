#include "SmacBaseUpkeepCache.h"
#include "types/Buffer.h"
#include <limits>
#include <set>
#include <stdexcept>

namespace game::backend::base {
namespace {
int32_t ReadInt32(types::Buffer& buffer) {
    const auto value=buffer.ReadInt();
    if(value<INT32_MIN || value>INT32_MAX) {throw std::runtime_error("Saved Base resource field exceeds signed32");}
    return static_cast<int32_t>(value);
}

void ValidateSupport(const SmacUnitSupportResult& support) {
    if(support.free_allowance<0 || support.free_allowance>127
        || (support.minerals_per_paid_unit!=1 && support.minerals_per_paid_unit!=2)
        || support.units.size()>SmacUnitSupport::MAX_ORDERED_UNITS) {
        throw std::runtime_error("Invalid saved Base support cache");
    }
    int64_t eligible=0,paid=0;
    std::set<uint64_t> ids;
    for(const auto& unit:support.units) {
        if(unit.id>static_cast<uint64_t>(INT64_MAX) || !ids.insert(unit.id).second) {
            throw std::runtime_error("Invalid saved Base support identity");
        }
        switch(unit.disposition) {
            case SmacSupportDisposition::Ignored: case SmacSupportDisposition::Exempt: break;
            case SmacSupportDisposition::Free: ++eligible; break;
            case SmacSupportDisposition::Paid: ++eligible; ++paid; break;
            default: throw std::runtime_error("Invalid saved Base support disposition");
        }
    }
    if(eligible!=support.eligible_count || paid!=support.paid_count
        || paid*support.minerals_per_paid_unit!=support.mineral_support) {
        throw std::runtime_error("Saved Base support totals differ from its ordered markings");
    }
}
}

void WriteSmacResourceCalculation(types::Buffer& buffer,const SmacBaseResourceCalculation& value) {
    for(const auto raw:value.raw_intake) {buffer.WriteInt(raw);}
    buffer.WriteInt(value.nutrients.intake);buffer.WriteInt(value.nutrients.demand);buffer.WriteInt(value.nutrients.surplus);
    buffer.WriteInt(value.nutrients.stored_before);buffer.WriteInt(value.nutrients.normalized_stored);
    buffer.WriteInt(value.minerals.multiplier_steps);buffer.WriteInt(value.minerals.intake);buffer.WriteInt(value.minerals.demand);
    buffer.WriteInt(value.minerals.surplus);buffer.WriteInt(value.minerals.pre_orbital_surplus);
}

SmacBaseResourceCalculation ReadSmacResourceCalculation(types::Buffer& buffer) {
    SmacBaseResourceCalculation value{};
    for(auto& raw:value.raw_intake) {raw=buffer.ReadInt();}
    value.nutrients.intake=ReadInt32(buffer);value.nutrients.demand=ReadInt32(buffer);value.nutrients.surplus=ReadInt32(buffer);
    value.nutrients.stored_before=buffer.ReadInt();value.nutrients.normalized_stored=buffer.ReadInt();
    value.minerals.multiplier_steps=ReadInt32(buffer);value.minerals.intake=ReadInt32(buffer);value.minerals.demand=ReadInt32(buffer);
    value.minerals.surplus=ReadInt32(buffer);value.minerals.pre_orbital_surplus=ReadInt32(buffer);
    return value;
}

types::Buffer SerializeSmacBaseUpkeepCache(const std::optional<SmacBaseUpkeepCache>& cache) {
    if(cache) {ValidateSupport(cache->support);}
    types::Buffer buffer;
    buffer.WriteInt(1);buffer.WriteBool(cache.has_value());
    if(cache) {
        WriteSmacResourceCalculation(buffer,cache->calculation);
        const auto& support=cache->support;
        buffer.WriteInt(support.free_allowance);buffer.WriteInt(support.minerals_per_paid_unit);
        buffer.WriteInt(support.eligible_count);buffer.WriteInt(support.paid_count);buffer.WriteInt(support.mineral_support);
        // A pair is two typed int64 frames. Bound arithmetic before Buffer's
        // uint32 wire offsets, without inventing an original-game unit limit.
        if(support.units.size()>(UINT32_MAX-buffer.lenw-14u)/28u) {throw std::runtime_error("Base support cache exceeds wire capacity");}
        buffer.WriteInt(support.units.size());
        for(const auto& unit:support.units) {buffer.WriteInt(unit.id);buffer.WriteInt(static_cast<int64_t>(unit.disposition));}
    }
    return buffer;
}

std::optional<SmacBaseUpkeepCache> DeserializeSmacBaseUpkeepCache(types::Buffer buffer) {
    if(buffer.ReadInt()!=1) {throw std::runtime_error("Unsupported Base upkeep cache");}
    std::optional<SmacBaseUpkeepCache> cache;
    if(buffer.ReadBool()) {
        cache.emplace();cache->calculation=ReadSmacResourceCalculation(buffer);
        auto& support=cache->support;
        support.free_allowance=ReadInt32(buffer);support.minerals_per_paid_unit=ReadInt32(buffer);
        support.eligible_count=ReadInt32(buffer);support.paid_count=ReadInt32(buffer);support.mineral_support=ReadInt32(buffer);
        const auto count=buffer.ReadInt();
        if(count<0 || count>SmacUnitSupport::MAX_ORDERED_UNITS || buffer.lenr>buffer.lenw
            || static_cast<uint64_t>(count)>(buffer.lenw-buffer.lenr)/28u) {throw std::runtime_error("Invalid Base support cache count");}
        support.units.reserve(static_cast<size_t>(count));
        for(int64_t i=0;i<count;++i) {
            const auto id=buffer.ReadInt(),disposition=buffer.ReadInt();
            if(id<0 || disposition<static_cast<int64_t>(SmacSupportDisposition::Ignored)
                || disposition>static_cast<int64_t>(SmacSupportDisposition::Paid)) {throw std::runtime_error("Invalid Base support cache record");}
            support.units.push_back({static_cast<uint64_t>(id),static_cast<SmacSupportDisposition>(disposition)});
        }
        ValidateSupport(support);
    }
    if(buffer.lenr!=buffer.lenw) {throw std::runtime_error("Trailing Base upkeep cache");}
    return cache;
}
}
