#pragma once
#include <cstdint>
#include <vector>
#include "strecharge.h"
namespace recharge {
struct Cell {int x=0,y=0,z=0;};
struct Occupancy {bool object=false,reservation=false;};
struct Pod { Cell cell; uint32_t id=0; int status=0; bool present=true; };
// Own-player list in original order. vtable+f8 (004be140) is status != 1.
// This picks a building, not a reachable path or a free charging position.
inline int nearestPod(Cell boat,const std::vector<Pod> &pods) {
    int best=1000000,chosen=-1;
    for(size_t i=0;i<pods.size();++i) {
        const auto &p=pods[i];if(!p.present || p.status==1) continue;
        int d=distance(p.cell.x-boat.x,p.cell.y-boat.y,p.cell.z-boat.z);
        if(d<best) {best=d;chosen=int(i);}
    }
    return chosen;
}
// 00493610: nearest clear recharge cell, Z/Y/X order breaks equal distances.
// Both original object-grid slots matter. The footprint is 4x4x3 cells.
inline bool podCell(int w,int h,int levels,Cell boat,Cell pod,
                    const std::vector<int16_t> &terrain,
                    const std::vector<Occupancy> &occupied,Cell &out) {
    if(w<=0 || h<=0 || levels<=0 || terrain.size()!=size_t(w)*h*levels
        || occupied.size()!=terrain.size()) return false;
    int best=1000000;bool found=false;
    auto index=[&](int x,int y,int z){return (z*h+y)*w+x;};
    for(int z=pod.z-1;z<=pod.z+1;++z) {
        if(z<0 || z>=levels) continue;
        for(int y=pod.y-1;y<=pod.y+2;++y) {
            if(y<0 || y>=h) continue;
            for(int x=pod.x-1;x<=pod.x+2;++x) {
                if(x<0 || x>=w) continue;
                int i=index(x,y,z);
                if(terrain[i]!=0 || occupied[i].object || occupied[i].reservation) continue;
                if(z>0) {
                    int below=index(x,y,z-1);
                    if(occupied[below].reservation || (occupied[below].object && terrain[below]!=0)) continue;
                }
                int d=distance(x-boat.x,y-boat.y,z-boat.z);
                if(d<best) {best=d;out={x,y,z};found=true;}
            }
        }
    }
    return found;
}
}
