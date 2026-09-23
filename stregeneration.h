#pragma once
#include <algorithm>
#include <cstdint>
#include "stregendata.h"

namespace regeneration {
struct Quote { int hp=0, interval=0; };
// 004d7040 and 006b12cc/12bc/12a8/1280, including fixed-point rounding.
inline Quote quote(int type,int percent,int conservation,bool speedUpgrade) {
    if(percent<=0 || percent>100) return {};
    const Parameters *parameters=nullptr;
    if(type>=1 && type<=40) parameters=&boats[type-1];
    else if(type>=50 && type<=115) parameters=&buildings[type-50];
    if(!parameters) return {};
    const auto &p=*parameters;
    int hp=p.amount[std::clamp(conservation,0,3)]/1500;
    int rate=p.rate;
    if(!hp || !rate) return {};
    if(speedUpgrade) rate+=rate*10/100;
    int scaledRate=int(int64_t(rate)*fixedScale*percent/100);
    if(!scaledRate) return {};
    int period=int(int64_t(1500)*fixedScale*fixedScale/scaledRate);
    int interval=period/fixedScale+(period%fixedScale>=fixedScale/2);
    return {hp,interval};
}
// The original uses a strict unsigned deadline, and spends ONE percentage point.
inline bool step(int type,int conservation,bool speedUpgrade,uint32_t tick,
                 uint32_t &last,int &hp,int hpMax,int &percent) {
    if(hp<=0 || hp>=hpMax || hpMax<=0) return false;
    const auto q=quote(type,percent,conservation,speedUpgrade);
    if(q.hp<=0 || uint32_t(last+q.interval)>=tick) return false;
    last=tick;hp=std::min(hpMax,hp+q.hp);--percent;return true;
}
}
