#pragma once
#include "stloadpath.h"
#include <array>
#include <cstdint>
#include <cmath>

namespace freightclear {
using loading::Cell;
// 0048dfd0: queue mode 1 and departure mode 2.
// Table order from the original function; allocator details have no gameplay role.
inline constexpr int sectors[8][21]={
    {6,4,8,2,11,17,14,20,16,13,19,1,10,15,12,18,5,3,7,0,9},
    {2,4,14,13,17,1,6,16,12,8,20,0,11,15,19,3,10,5,18,7,9},
    {1,0,2,3,4,13,12,14,16,15,17,5,6,19,18,20,7,8,10,9,11},
    {0,3,12,5,1,15,13,16,7,18,2,14,9,4,19,17,10,6,20,8,11},
    {5,7,3,9,0,15,18,12,16,19,13,10,1,17,20,14,11,2,6,8,4},
    {9,7,18,10,5,19,15,16,11,3,20,12,8,0,17,13,6,1,14,4,2},
    {10,9,11,7,8,19,18,20,16,5,6,15,17,13,3,4,12,14,1,0,2},
    {8,11,20,10,6,19,17,16,9,4,18,14,7,2,15,13,5,1,12,3,0}
};
inline constexpr int levelOrder[5][5]={
    {0,1,2,3,4},
    {1,2,3,0,4},
    {2,3,4,1,0},
    {3,4,2,1,0},
    {4,3,2,1,0}
};
// 004168d0: quantized world-space heading, including short-coordinate wrap.
inline int heading(Cell from,Cell to) {
    float dx=float(int(int16_t(to.x*201+100))-int(int16_t(from.x*201+100)));
    float dy=float(int(int16_t(to.y*201+100))-int(int16_t(from.y*201+100)));
    if(dx==0 && dy==0) dx=1;
    float length=std::sqrt(dx*dx+dy*dy);
    double ratio=double(dy)/length;float f=float(ratio);
    if(ratio<-.991) return 90;
    constexpr double lo[]={-.992,-.925,-.794,-.610,-.384,-.132,.130,.382,.608,.792,.923};
    constexpr double hi[]={-.924,-.793,-.609,-.383,-.131,.131,.383,.609,.793,.924,.992};
    constexpr int right[]={75,60,45,30,15,0,345,330,315,300,285};
    constexpr int left[]={105,120,135,150,165,180,195,210,225,240,255};
    for(int i=0;i<11;++i) if(double(f)>lo[i] && double(f)<hi[i]) return dx>0?right[i]:left[i];
    return 270;
}
inline bool selectNear(int w,int h,const std::vector<int16_t> &terrain,
                   const std::vector<uint8_t> &slots,Cell start,Cell target,int mode,uint32_t &seed,Cell &out)
{
    if(w<1 || h<1 || w>256 || h>256 || terrain.size()!=size_t(w)*h*5
        || slots.size()!=terrain.size() || start.x<0 || start.x>=w || start.y<0
        || start.y>=h || start.z<0 || start.z>=5 || target.x<0 || target.x>=w
        || target.y<0 || target.y>=h || target.z<0 || target.z>=5 || mode<1 || mode>2) return false;
    int margin=mode*4+5;
    int x0=std::max(0,std::min(start.x,target.x)-margin),y0=std::max(0,std::min(start.y,target.y)-margin);
    int x1=std::min(w-1,std::max(start.x,target.x)+margin),y1=std::min(h-1,std::max(start.y,target.y)+margin);
    int cw=x1-x0+1,ch=y1-y0+1;
    auto index=[&](int x,int y,int z){return (z*h+y)*w+x;};
    std::vector<int16_t> field(size_t(cw)*ch*5);
    for(int z=0;z<5;++z)for(int y=0;y<ch;++y)for(int x=0;x<cw;++x)
        field[(z*ch+y)*cw+x]=terrain[index(x+x0,y+y0,z)];
    if(!loading::DistanceField(cw,ch,5,{target.x-x0,target.y-y0,target.z},field)) return false;
    std::vector<int16_t> fromStart;
    if(field[(start.z*ch+start.y-y0)*cw+start.x-x0]<1) {
        fromStart=terrain;field=terrain;x0=y0=0;x1=w-1;y1=h-1;cw=w;ch=h;
        if(!loading::DistanceField(w,h,5,start,fromStart) || !loading::DistanceField(w,h,5,target,field)) return false;
    }
    int direction;
    if(start.x==target.x && start.y==target.y) {
        seed=seed*0x41c64e6du+0x3039u;direction=(seed>>16)&7;
    } else {
        int angle=heading(start,target);direction=((angle+22)/45)%8;
    }
    struct Rect {int l,t,r,b;};
    for(int radius=mode;radius<mode*5;++radius) {
        int l=start.x-radius,r=start.x+radius,t=start.y-radius,b=start.y+radius;
        int third=(radius*2+1)/3,a=l+third-1,c=r-third;
        int mt=t+third,mb=b-third;
        std::array<Rect,21> rects{{
            {l,t,a,t},{a+1,t,c,t},{c+1,t,r,t},
            {l,t+1,l,mt-1},{r,t+1,r,mt-1},
            {l,mt,l,mb},{r,mt,r,mb},
            {l,mb+1,l,b-1},{r,mb+1,r,b-1},
            {l,b,a,b},{a+1,b,c,b},{c+1,b,r,b},
            {l+1,t+1,a,mt-1},{a+1,t+1,c,mt-1},{c+1,t+1,r-1,mt-1},
            {l+1,mt,a,mb},{a+1,mt,c,mb},{c+1,mt,r-1,mb},
            {l+1,mb+1,a,b-1},{a+1,mb+1,c,b-1},{c+1,mb+1,r-1,b-1}
        }};
        for(int zi=0;zi<5;++zi) {
            int z=levelOrder[start.z][zi];
            for(int ri:sectors[direction]) {
                // Small radii disable narrow strips, but retain the central rectangle.
                if(radius<3 && (ri==3 || ri==4 || ri==7 || ri==8 || (ri>=12 && ri!=16))) continue;
                if(ri>=12 && std::abs(start.z-z)<mode) continue;
                const auto &q=rects[ri];
                for(int x=q.l;x<=q.r;++x)for(int y=q.t;y<=q.b;++y) {
                    if(x<x0 || x>x1 || y<y0 || y>y1) continue;
                    int i=index(x,y,z);if(slots[i]) continue;
                    if(z>0) {
                        int below=index(x,y,z-1);
                        if((slots[below]&2) || ((slots[below]&1) && terrain[below]!=0)) continue;
                    }
                    int fi=(z*ch+y-y0)*cw+x-x0,d=field[fi];
                    int reachable=fromStart.empty()?d:fromStart[fi];
                    if(reachable>0 && (d-1)/3>=radius) {out={x,y,z};return true;}
                }
            }
        }
    }
    return false;
}
inline bool select(int w,int h,const std::vector<int16_t> &terrain,
                   const std::vector<uint8_t> &slots,Cell start,uint32_t &seed,Cell &out) {
    return selectNear(w,h,terrain,slots,start,start,2,seed,out);
}
}
