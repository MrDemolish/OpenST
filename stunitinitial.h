#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include "stcargotransfer.h"
namespace unitinitial {
using Cargo = cargotransfer::Cargo;
inline Cargo cargo(int32_t corium,int32_t metal) {
    int c=corium<0?0:corium>120?40:corium/3;
    int m=metal<0?0:metal>800?40:metal/20;
    if(c+m>40) { c=c*40/(c+m); m=40-c; }
    Cargo out;out.corium=c*3;out.metal=m*20;return out;
}
inline Cargo readCargo(const std::vector<uint8_t> &raw) {
    if(raw.size()<105) return {};
    int32_t mode=0,c=0,m=0;
    std::memcpy(&mode,raw.data()+12,4);
    if(mode!=0) return {};
    std::memcpy(&c,raw.data()+42,4);std::memcpy(&m,raw.data()+46,4);
    return cargo(c,m);
}
// STBoat GetMessage 0044f196..0044f1d6: this differs from building loading.
inline int hitpoints(int maximum,int32_t percent) {
    if(percent<0) return 1;
    if(percent>=100) return maximum;
    return int(int64_t(maximum)*percent/100);
}
inline int readHitpoints(const std::vector<uint8_t> &raw,int maximum) {
    if(raw.size()<105) return maximum;
    int32_t mode=0,percent=0;
    std::memcpy(&mode,raw.data()+12,4);
    if(mode!=0) return maximum; // other creation/save modes follow another branch
    std::memcpy(&percent,raw.data()+38,4);
    return hitpoints(maximum,percent);
}
}
