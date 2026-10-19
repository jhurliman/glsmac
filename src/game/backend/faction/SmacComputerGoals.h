#pragma once

#include <array>
#include <cstdint>

namespace game::backend::faction {

// Native goal records are distinct from movement orders and actual paths.
struct SmacComputerGoal {
    int16_t type=-1, priority=0;
    int32_t x=0, y=0, auxiliary=0;
    bool operator==(const SmacComputerGoal& other) const;
};

struct SmacComputerGoals {
    std::array<SmacComputerGoal,75> active{};
    std::array<SmacComputerGoal,25> persistent{};

    // Operate on supplied owned storage; this class does not establish native
    // startup/reset/consumption lifetime or choose a destination for a Unit.
    void InsertActive(int32_t type,int32_t priority,int32_t x,int32_t y,int32_t auxiliary,int32_t width,int32_t height);
    void InsertPersistent(int32_t type,int32_t priority,int32_t x,int32_t y,int32_t width,int32_t height);
    void Renew(int32_t width,int32_t height);
    bool operator==(const SmacComputerGoals& other) const;
};

}
