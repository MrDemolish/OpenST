#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
namespace silicon {
constexpr int SET_PER_CELL_OPCODE=0x584;
constexpr uint8_t INITIAL_PER_CELL=80; // 00430d28 mov eax,0x50505050; rep stos.
inline std::vector<uint8_t> initialGrid(int width,int height) {
    if(width<=0 || height<=0) return {};
    return std::vector<uint8_t>(size_t(width)*size_t(height),INITIAL_PER_CELL);
}
// 004d83d0, after the script wrapper's coordinate conversion.
// Negative origin is clamped BEFORE clipping the supplied extent.
inline bool fill(std::vector<uint8_t> &grid,int width,int height,int value,
                 int x,int y,int spanX,int spanY) {
    if(width<=0 || height<=0 || grid.size()!=size_t(width)*size_t(height)
       || spanX<0 || spanY<0) return false;
    x=std::max(0,x);y=std::max(0,y);
    // Original assumes valid starts; reject invalid script rectangles safely.
    if(x>width || y>height) return false;
    spanX=std::min(spanX,width-x);spanY=std::min(spanY,height-y);
    for(int row=y;row<y+spanY;++row)
        std::fill_n(grid.begin()+size_t(row)*width+x,spanX,uint8_t(value));
    return true;
}
// ActSetSiliconPerCell: 0064f620 supplies 0,0,-1,-1 for omitted arguments.
// 0064e5c0 and 006756d0 shift overflowing rectangles back inside the map.
inline bool setPerCell(std::vector<uint8_t> &grid,int width,int height,int value,
                       int x=0,int y=0,int spanX=-1,int spanY=-1) {
    x=int16_t(x);y=int16_t(y);spanX=int16_t(spanX);spanY=int16_t(spanY);
    if(spanX<1) { x=0;spanX=width; }
    if(spanY<1) { y=0;spanY=height; }
    x=std::max(0,x);y=std::max(0,y);
    if(x+spanX>width) x=std::max(0,width-spanX);
    if(y+spanY>height) y=std::max(0,height-spanY);
    return fill(grid,width,height,value,x,y,spanX,spanY);
}
struct State { uint32_t last=0;int cursor=0,stored=0; };
struct Result { int taken=0,credited=0;bool ran=false; };
// 004e46f0, with bank/UI notifications represented by credited.
// Original scan is 29x29 at offsets -15..13 (841 cells), not a symmetric 30x30.
inline Result step(State &s,std::vector<uint8_t> &grid,int width,int height,
                   int x,int y,uint32_t tick) {
    Result r;
    if(width<=0 || height<=0 || grid.size()!=size_t(width)*size_t(height)
       || s.cursor<0 || s.cursor>840 || uint32_t(s.last+25)>tick) return r;
    s.last=tick;r.ran=true;int i=s.cursor;
    do {
        int cx=i%29-15+x,cy=i/29-15+y;
        if(cx>=0 && cx<width && cy>=0 && cy<height) {
            auto &cell=grid[size_t(cy)*width+cx];
            int n=std::min(int(cell),10);cell=uint8_t(cell-n);
            r.taken+=n;s.stored+=n;
            if(n && s.stored>=100) { r.credited+=s.stored;s.stored=0; }
        }
        if(++i>840) i=0;
    } while(i!=s.cursor && r.taken<10);
    s.cursor=i;return r;
}
}
