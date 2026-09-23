#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
namespace buildinginitial {
// TLO GetMessage copies raw[20..86] to this+5ac. Fields +5d7/+5db
// are signed DWORD percentages, normalized at 004ba2a7.
inline int percent(int32_t value) {return value<0 || value>100?100:int(value);}
struct State {int hpPercent=100,energyPercent=100;};
// STGameObjC::GetMessage 0041af40 copies record +16 to object +2c.
// RegisterOnMap positioning 00417a20 uses any nonzero value as a 2x2 footprint.
inline int footprint(const std::vector<uint8_t> &raw,int fallback=1) {
    if(raw.size()<20) return fallback;
    int32_t size=0;std::memcpy(&size,raw.data()+16,4);
    return size?2:1;
}
inline State read(const std::vector<uint8_t> &raw) {
    State out;
    if(raw.size()<87) return out;
    int32_t mode=0,hp=0,energy=0;
    std::memcpy(&mode,raw.data()+12,4);
    // Mode 2 appends a runtime object dump, not the map initialization branch.
    if(mode!=0 && mode!=1 && mode!=3) return out;
    std::memcpy(&hp,raw.data()+63,4);std::memcpy(&energy,raw.data()+67,4);
    return {percent(hp),percent(energy)};
}
inline int hitpoints(int maximum,int32_t savedPercent) {
    return int(int64_t(maximum)*percent(savedPercent)/100);
}
}
