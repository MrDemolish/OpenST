#pragma once
#include <algorithm>
#include <cstdint>

// Original seawater gold reservoir, 004d8110..004d83ab.
// Fixed-point operations use 10000 and retain the x86 low dword.
namespace gold {
constexpr int OBJECT_LEVELS=5; // 004959f0, copied to g_wObjGridZ at 00495aef.
// 004d8760, table 007bf594: low/normal/high concentration.
inline int32_t initialAmount(int setting,int16_t levels,int16_t stride) {
    if(setting<0 || setting>2) return 0;
    return int32_t((int64_t(setting+1)*5*levels*stride)/10);
}
// Distance used only inside the extractor's bounded 26x26 neighborhood.
inline int neighborDistance(int dx,int dy) {
    const int squared=dx*dx+dy*dy;
    int result=0;while((result+1)*(result+1)<=squared) ++result;
    return result+(squared>result*result+result?1:0);
}
struct Neighborhood { int count=0,distanceSum=0; };
// isOtherRig reports occupied grid cells, including repeated cells of one rig.
// Ownership does not affect this scan; caller excludes the extractor itself.
template<class IsOtherRig>
Neighborhood neighbors(int x,int y,int width,int height,int levels,IsOtherRig isOtherRig) {
    Neighborhood result;
    for(int cy=std::max(0,y-13);cy<std::min(height,y+13);++cy)
        for(int cx=std::max(0,x-13);cx<std::min(width,x+13);++cx) {
            const int distance=neighborDistance(cx-x,cy-y);
            if(distance>13) continue;
            for(int z=0;z<levels;++z) if(isOtherRig(cx,cy,z)) {
                ++result.count;result.distanceSum+=distance;
            }
        }
    return result;
}
inline int32_t narrow(int64_t n) { return int32_t(uint32_t(n)); }
inline int32_t fixed(int32_t n) { return narrow(int64_t(n)*10000); }
inline int32_t mul(int32_t a,int32_t b) { return narrow(int64_t(a)*b/10000); }
inline int32_t divide(int32_t a,int32_t b) { return narrow(int64_t(a)*10000/b); }
inline int32_t rounded(int32_t n) {
    return n/10000+(n%10000>=5000?1:n%10000<=-5000?-1:0);
}
inline int32_t interval(int32_t volume,int32_t remaining) {
    if(!remaining) return 0x0fffffff;
    const int32_t ratio=divide(fixed(volume),fixed(remaining));
    if(rounded(ratio)>=450) return 0x0fffffff;
    const int64_t square=int64_t(ratio)*ratio/10000;
    const int64_t scaled=25*square;
    // Original idiv traps outside int32. Suspend instead of crashing the game.
    if(square>INT32_MAX || scaled>INT32_MAX || square<0) return 0x0fffffff;
    return narrow(int64_t(rounded(int32_t(scaled)))*2);
}
struct Reservoir {
    int32_t initial=0,remaining=0,volume=0,period=0,initialPeriod=0;
    uint32_t last=0;
    void reset(int32_t amount,int32_t cells,uint32_t tick) {
        initial=remaining=amount;volume=cells;
        initialPeriod=period=interval(volume,remaining);last=tick;
    }
    void take(int32_t amount) {
        if(remaining>=volume/100 && amount) {
            remaining=std::max(0,narrow(int64_t(remaining)-amount));
            period=interval(volume,remaining);
        }
    }
    void step(uint32_t tick) {
        if(remaining<initial && uint32_t(last+uint32_t(period)*5)<=tick) {
            remaining=narrow(int64_t(remaining)+2);last=tick;
            period=interval(volume,remaining);
        }
    }
};
// 004d99a9: all occupied neighboring cells contribute, not only the nearest rig.
inline int32_t crowdedInterval(int32_t base,int count,int distanceSum) {
    if(!count || base>=0x0fffffff) return base;
    const int deficit=std::max(0,count*13-distanceSum);
    const int64_t scale=int64_t(divide(fixed(deficit),fixed(13)))+10000;
    const int64_t product=int64_t(base)*scale;
    if(product>INT32_MAX || product<0) return 0x0fffffff;
    return rounded(int32_t(product));
}
struct Extractor {
    int32_t stored=0,period=0;
    uint32_t last=0,lastScan=0;
    Neighborhood nearby;
    // 004d9790: advance the object's own RNG, not a shared random stream.
    void reset(uint32_t tick,uint32_t &seed) {
        seed=seed*0x41c64e6du+0x3039u;
        stored=period=0;last=tick;lastScan=tick+(seed>>16)%76;
        nearby={};
    }
    // 004d9b20: an empty manual transfer does not reset the production timer.
    int transfer(uint32_t tick) {
        const int amount=stored;
        if(amount) { stored=0;last=tick; }
        return amount;
    }
    template<class Scan>
    int step(uint32_t tick,Reservoir &water,Scan scan) {
        if(uint32_t(lastScan+75)<=tick) { lastScan=tick;nearby=scan(); }
        period=crowdedInterval(water.period,nearby.count,nearby.distanceSum);
        if(uint32_t(last+uint32_t(period))>tick) return 0;
        last=tick;stored=narrow(int64_t(stored)+2);water.take(2);
        return stored>=100?transfer(tick):0;
    }
};
}
