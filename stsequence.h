// Type 29 timing descriptor, decoded by original mfTSprLoad (00716850).
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "stark.h"

namespace sequence {
struct Timing {
    std::string sprite;
    std::vector<uint32_t> ticks;
    bool load(const std::vector<uint8_t> &bytes, int frames) {
        sprite.clear(); ticks.clear();
        if(bytes.size()<80 || frames<=0) return false;
        uint32_t count=ark::rd32(bytes.data()+76);
        if(count>(bytes.size()-80)/12) return false;
        size_t end=0; while(end<32 && bytes[end]) ++end;
        if(end==0 || end==32) return false;
        sprite.assign(reinterpret_cast<const char*>(bytes.data()),end);
        ticks.assign(size_t(frames),ark::rd32(bytes.data()+72));
        for(uint32_t i=0;i<count;++i) {
            const uint8_t *entry=bytes.data()+80+size_t(i)*12;
            uint32_t frame=ark::rd32(entry);
            if(frame<ticks.size()) ticks[frame]=ark::rd32(entry+8);
        }
        return true;
    }
    double duration() const {
        double total=0; for(uint32_t t:ticks) total+=std::max(1u,t);
        return total;
    }
    int frame(double elapsed, bool backwards, bool loop) const {
        if(ticks.empty()) return 0;
        return frameRange(elapsed,backwards?int(ticks.size()-1):0,
                          backwards?0:int(ticks.size()-1),loop);
    }
    int frameRange(double elapsed,int first,int last,bool loop) const {
        if(ticks.empty()) return 0;
        first=std::clamp(first,0,int(ticks.size()-1));
        last=std::clamp(last,0,int(ticks.size()-1));
        int count=std::abs(last-first)+1,step=last<first?-1:1;
        double total=0;
        for(int i=0;i<count;++i) total+=std::max(1u,ticks[size_t(first+i*step)]);
        elapsed=std::max(0.0,elapsed);
        if(loop) elapsed=std::fmod(elapsed,total);
        for(int i=0;i<count;++i) {
            size_t index=size_t(first+i*step);
            double hold=std::max(1u,ticks[index]);
            if(elapsed+1e-6<hold) return int(index);
            elapsed-=hold;
        }
        return last;
    }
};
}
