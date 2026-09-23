#pragma once
#include <cstdint>
#include <cstddef>
#include <vector>

namespace navigation {
// BuildTerrainGrid 00575640, normal mode. All four MAPMESH masks are needed:
// presence bits 8..1, solid bits 0x80..0x10, edge bits 0x8000..0x1000.
inline void ApplyTile(std::vector<int16_t> &grid,int w,int h,int x,int y,
                      int level,int count,bool skip,const uint32_t *masks)
{
    if(skip || level==0 || w<1 || h<1 || grid.size()!=std::size_t(w)*h*5) return;
    for(int d=0;d<count && d<4;++d) {
        int z=level-1-d;if(z<0 || z>=5) continue;
        for(int k=0;k<4;++k) {
            int cx=x+(k&1),cy=y+(k>>1);
            if(cx<0 || cx>=w || cy<0 || cy>=h || !(masks[k]&(8u>>d))) continue;
            int16_t value=(masks[k]&(0x80u>>d)) ? -1 :
                ((masks[k]&(0x8000u>>d)) ? -2 : -16385);
            grid[(std::size_t(z)*h+cy)*w+cx]=value;
        }
    }
}
}
