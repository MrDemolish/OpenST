// STBoatC docking candidate selector 0048d7c0; no route is created here.
#pragma once
#include <algorithm>
#include <cstdlib>
#include <cstdint>
namespace freightberth {
struct Cell {int x=0,y=0,z=0;};
inline int distance(Cell a,Cell b) {
    int x=std::abs(a.x-b.x),y=std::abs(a.y-b.y),z=std::abs(a.z-b.z);
    int largest=std::max({x,y,z});
    return (x+y+z+2*largest)/3; // 006aadd0 truncates, it is not Euclidean distance
}
template<class Occupied>
bool select(Cell boat,Cell building,Cell size,Occupied occupied,Cell &chosen) {
    int best=1000000;
    for(int dx=0;dx<2;++dx) for(int dy=0;dy<2;++dy) {
        Cell unwrapped{building.x+dx,building.y+dy,building.z+1};
        Cell candidate{int16_t(unwrapped.x),int16_t(unwrapped.y),int16_t(unwrapped.z)};
        bool inside=candidate.x>=0 && candidate.y>=0 && candidate.z>=0
            && candidate.x<size.x && candidate.y<size.y && candidate.z<size.z;
        // The original considers outside-grid cells empty. The movement adapter
        // must validate the resulting target; this function preserves that quirk.
        if(inside && occupied(candidate)) continue;
        int d=distance(boat,unwrapped);
        if(d<best) {best=d;chosen=candidate;}
    }
    return best!=1000000;
}
}
