#pragma once
namespace gse::tests::workshop_newborn_fixture {
// Native17 complete Base returns, manifest c6e44e15a2d36876fa568fca0fb3933c14af2de9270e23697eb173de2a58f257.
struct Case {bool first,high,command,bio,selected;int current;uint32_t governor;uint8_t morale;uint32_t state;uint16_t flags;};
inline const Case CASES[]{
{false,false,false,false,false,0,0x0u,1,0x2000u,0x0u},
{false,false,true,true,false,0,0x0u,5,0x2000u,0x0u},
{false,false,true,true,false,-2,0x0u,3,0x2000u,0x0u},
{false,true,false,false,false,0,0x0u,2,0x2000u,0x0u},
{false,true,true,true,false,0,0x0u,6,0x2000u,0x0u},
{false,true,true,true,false,-2,0x0u,4,0x2000u,0x0u},
{true,false,false,false,false,0,0x0u,2,0x2000u,0x0u},
{true,false,true,true,false,0,0x0u,6,0x2000u,0x0u},
{true,false,true,true,false,-2,0x0u,4,0x2000u,0x0u},
{true,true,false,false,false,0,0x0u,3,0x2000u,0x0u},
{true,true,true,true,false,0,0x0u,6,0x2000u,0x0u},
{true,true,true,true,false,-2,0x0u,5,0x2000u,0x0u},
{true,true,true,true,true,-1,0x0u,6,0x2000u,0x1000u},
{false,true,true,false,true,-2,0x0u,3,0x2000u,0x1000u},
{false,false,false,false,true,0,0x80000080u,1,0x2200u,0x1000u},
};
}
