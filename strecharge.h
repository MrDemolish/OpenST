#pragma once
#include <algorithm>
#include <cstdlib>
namespace recharge {
// 006aadd0, used by CheckForReplenisher 00493340; strict distance < 7.
inline int distance(int dx,int dy,int dz) {
    dx=std::abs(dx);dy=std::abs(dy);dz=std::abs(dz);
    return (dx+dy+dz+2*std::max(dx,std::max(dy,dz)))/3;
}
struct Transfer { int spent,percent; };
// 004d6df0 / 004d6f70 / 004d6eb0: percentage storage, integer truncation.
inline Transfer transfer(int percent,int capacity,int available) {
    if(capacity<=0 || available<0 || percent<0 || percent>100) return {0,percent};
    int spent=std::min((100-percent)*capacity/100,available);
    return {spent,std::min(100,percent+spent*100/capacity)};
}
}
