#pragma once
#include "SmacWorkedSquareCollection.h"
#include <functional>
#include <optional>
#include <vector>

namespace game::backend::base {
struct SmacNativeSpawnInput {
    SmacWorkerRadiusMap map; // selected bloom square, after terrain publication is staged
    int32_t actor,history,retained_base_count,population;
};
struct SmacNativeSpawnBase {
    std::array<int32_t,2> coordinate;
    int32_t owner;
};
struct SmacNativeSpawnTile {
    int32_t altitude_class;
    bool fungus;
    uint8_t region,actor_regional_base_count;
    bool unit_or_base;
    uint8_t cached_owner; // low nibble; invalid15 differs from native0
};
class SmacNativeSpawnPlan {
public:
    struct Request {
        std::array<int32_t,2> coordinate;
        int32_t design,sweep_attempts,special_counter;
        // Sweep success writes the actor tag; fallback leaves initializer tag0.
        std::optional<uint8_t> ecology_owner;
    };
    SmacNativeSpawnPlan(const SmacNativeSpawnPlan&)=default;
    SmacNativeSpawnPlan(SmacNativeSpawnPlan&&)=default;
    SmacNativeSpawnPlan& operator=(const SmacNativeSpawnPlan&)=default;
    SmacNativeSpawnPlan& operator=(SmacNativeSpawnPlan&&)=default;
    const SmacNativeSpawnInput& Input() const noexcept {return m_input;}
    const std::vector<Request>& Requests() const noexcept {return m_requests;}
    int32_t SweepAttempts() const noexcept {return m_sweep_attempts;}
    int32_t SpecialCounter() const noexcept {return m_special_counter;}
private:
    explicit SmacNativeSpawnPlan(const SmacNativeSpawnInput& input):m_input(input) {}
    SmacNativeSpawnInput m_input;
    std::vector<Request> m_requests;
    int32_t m_sweep_attempts=0,m_special_counter=0;
    friend SmacNativeSpawnPlan PrepareSmacNativeSpawn(const SmacNativeSpawnInput&,
        const std::vector<SmacNativeSpawnBase>&,const std::function<SmacNativeSpawnTile(int32_t,int32_t)>&);
};
// Call after human/AI admission, one-shot status and H/Voice holds. Bases retain
// global record order. Query supplies the staged post-terrain map, without writes.
// Produces all factory attempts, including ones that later fail capacity. No RNG,
// allocation success, AI assignment, live Unit or warming/climate publication.
SmacNativeSpawnPlan PrepareSmacNativeSpawn(const SmacNativeSpawnInput& input,
    const std::vector<SmacNativeSpawnBase>& bases,const std::function<SmacNativeSpawnTile(int32_t,int32_t)>& query);
}
