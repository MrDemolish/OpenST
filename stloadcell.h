#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace loading {
struct Cell { int x=0,y=0,z=0; };
// Selection half of STBoatC::GetCellForLoading (004919c0), after 006ab090.
// distance: positive reachable cost, zero unreachable, negative terrain.
// occupant: 0 empty, 1 this carrier, 2 another object (first grid slot).
// Output includes the passenger's proposed level, not a completed transfer.
inline int SelectCell(int w,int h,const std::vector<int16_t> &distance,
                      const std::vector<uint8_t> &occupant,Cell target,
                      Cell &approach,Cell &passenger)
{
    if(w<=0 || h<=0 || target.x<0 || target.x>=w || target.y<0 || target.y>=h
        || target.z<0 || target.z>=5 || distance.size()!=std::size_t(w)*h*5
        || occupant.size()!=distance.size()) return -2;
    auto idx=[&](int x,int y,int z){return (z*h+y)*w+x;};
    if(distance[idx(target.x,target.y,target.z)]<1) return -2;
    constexpr int order[5][5]={{0,1,2,3,4},{1,2,0,3,4},{2,3,1,4,0},
                              {3,4,2,1,0},{4,3,2,1,0}};
    const int *levels=order[target.z];
    const int dx[4]={0,-1,1,0},dy[4]={-1,0,0,1};
    int status[5]={},pick[5]={-1,-1,-1,-1,-1};
    for(int l=0;l<5;++l) {
        int z=levels[l],best=1000000000;
        if(distance[idx(target.x,target.y,z)]<1) continue;
        for(int n=0;n<4;++n) {
            int x=target.x+dx[n],y=target.y+dy[n];
            if(x<0 || x>=w || y<0 || y>=h) continue;
            int i=idx(x,y,z),cost=distance[i];
            if(cost<1) continue;
            if(occupant[i]==1) {pick[l]=n;status[l]=2;break;}
            if(occupant[i]==0 && status[l]==0) {
                pick[l]=n;status[l]=1;best=cost;continue;
            }
            if(occupant[i]!=0 && status[l]!=0) continue;
            if(cost<best) {pick[l]=n;best=cost;}
        }
    }
    // A reachable level around a ceiling is not a vertical passage for cargo.
    for(int z=target.z+1;z<5;++z) if(distance[idx(target.x,target.y,z)]<1)
        for(int l=0;l<5;++l) if(levels[l]>z) pick[l]=-1;
    for(int z=target.z-1;z>=0;--z) if(distance[idx(target.x,target.y,z)]<1)
        for(int l=0;l<5;++l) if(levels[l]<z) pick[l]=-1;
    int chosen=-1,best=1000000000;
    for(int l=0;l<5;++l) {
        int n=pick[l];if(n<0) continue;
        int cost=distance[idx(target.x+dx[n],target.y+dy[n],levels[l])];
        if(chosen<0 || (status[l]==1 && status[chosen]==0)
            || ((status[l]!=0 || status[chosen]!=1) && cost<best)) {
            chosen=l;best=cost;
        }
    }
    if(chosen<0) return -1;
    int n=pick[chosen];
    approach={target.x+dx[n],target.y+dy[n],levels[chosen]};
    passenger={target.x,target.y,levels[chosen]};
    return 0;
}
}
