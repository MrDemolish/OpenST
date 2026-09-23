#pragma once
#include "stloadcell.h"
#include <queue>
#include <utility>
#include <functional>
#include <algorithm>

namespace loading {
// Full-field mode of 006ab090 (destination = -1,-1,-1).
// The signed terrain codes are retained, including their corner-mask bits.
inline bool DistanceField(int w,int h,int levels,Cell start,std::vector<int16_t> &d,
                          std::vector<int> *parents=nullptr)
{
    if(parents) parents->clear();
    if(w<1 || h<1 || levels<1 || w>256 || h>256 || levels>256
        || d.size()!=std::size_t(w)*h*levels || start.x<0 || start.x>=w
        || start.y<0 || start.y>=h || start.z<0 || start.z>=levels) return false;
    auto idx=[&](int x,int y,int z){return (z*h+y)*w+x;};
    if(parents) parents->assign(d.size(),-1);
    using Entry=std::pair<int,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending;
    int first=idx(start.x,start.y,start.z);d[first]=1;pending.push({1,first});
    while(!pending.empty()) {
        auto [cost,i]=pending.top();pending.pop();if(d[i]!=cost) continue;
        int x=i%w,y=(i/w)%h,z=i/(w*h);
        for(int dz=-1;dz<=1;++dz) for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
            int axes=(dx!=0)+(dy!=0)+(dz!=0);if(!axes) continue;
            int nx=x+dx,ny=y+dy,nz=z+dz;
            if(nx<0 || nx>=w || ny<0 || ny>=h || nz<0 || nz>=levels) continue;
            int n=idx(nx,ny,nz),next=cost+axes+2;
            if(d[n]<0 || (d[n]>0 && d[n]<=next)) continue;
            bool blocked=false;
            // Every intermediate corner must avoid a hard (0xc000) barrier.
            for(int mask=1;mask<8;++mask) {
                if(((mask&1) && !dx) || ((mask&2) && !dy) || ((mask&4) && !dz)) continue;
                int ax=(mask&1)?dx:0,ay=(mask&2)?dy:0,az=(mask&4)?dz:0;
                if(ax==dx && ay==dy && az==dz) continue;
                int16_t v=d[idx(x+ax,y+ay,z+az)];
                if((uint16_t(v)&0xc000)==0xc000) blocked=true;
            }
            // Vertical two-axis diagonals have an extra signed-height check.
            if(axes==2 && dz && d[idx(x,y,z+dz)]<0) blocked=true;
            if(blocked || next>32767) continue;
            d[n]=int16_t(next);if(parents) (*parents)[n]=i;
            pending.push({next,n});
        }
    }
    return true;
}

// Retain the predecessor chosen by the cost solver. This avoids reconstructing
// an illegal diagonal by merely looking at lower neighbouring costs.
inline std::vector<Cell> TraceRoute(int w,int h,int levels,Cell start,Cell goal,
                                   const std::vector<int> &parents)
{
    std::vector<Cell> route;
    if(w<1 || h<1 || levels<1 || parents.size()!=std::size_t(w)*h*levels) return route;
    auto valid=[&](Cell c){return c.x>=0 && c.x<w && c.y>=0 && c.y<h && c.z>=0 && c.z<levels;};
    if(!valid(start) || !valid(goal)) return route;
    int first=(start.z*h+start.y)*w+start.x,i=(goal.z*h+goal.y)*w+goal.x;
    for(std::size_t guard=0;i!=first && guard<parents.size();++guard) {
        if(i<0 || i>=int(parents.size())) return {};
        route.push_back({i%w,(i/w)%h,i/(w*h)});i=parents[i];
    }
    if(i!=first) return {};
    std::reverse(route.begin(),route.end());return route;
}
}
