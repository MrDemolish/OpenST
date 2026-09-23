#pragma once
#include <array>
#include <cstdint>

namespace loading {
// STBoatC::PrepareForLoading 00491240. Physical coordinates use original
// world units (201 in XY, 200 in Z), not renderer mesh coordinates.
struct Preparation {
    int x=0,y=0,z=0,heading=0;
    std::array<int,5> levels{{-1,-1,-1,-1,-1}};
    int stage=0; // 0 depth route, 1 XY, 3 physical Z, 5 heading, 6 neutral bob
};
inline bool Prepare(uint32_t reservedCarrier,uint32_t carrier,int gridZ,
                    int physicalX,int physicalY,int physicalZ,int heading,
                    int x,int y,int z,int targetHeading,Preparation &out) {
    if(reservedCarrier!=carrier || gridZ<0 || gridZ>4 || z<0 || z>4) return false;
    Preparation p;p.x=x;p.y=y;p.z=z;p.heading=targetHeading;
    int step=z>gridZ?1:-1,n=0;
    for(int level=gridZ;level!=z;) {level+=step;p.levels[n++]=level;}
    if(n) p.stage=0;
    else if(physicalX!=x*201+100 || physicalY!=y*201+100) p.stage=1;
    else if(physicalZ!=z*200+100) p.stage=3;
    else if(heading!=targetHeading) p.stage=5;
    else p.stage=6;
    out=p;return true;
}
// WaitLoad 004749c0, preparation substate 6. The heading stage switches to
// 6 on an earlier update; phase 23 is also visually neutral but is NOT ready.
inline bool FinishPreparation(Preparation &p,int bobPhase,int &bobMode) {
    if(p.stage!=6 || bobPhase!=47) return false;
    bobMode=0;p.stage=7;return true;
}
struct CarrierHandshake {
    int command=14,phase=2,substage=0;
    uint32_t passenger=0;
    int targetX=0,targetY=0,targetZ=0;
};
// ReadyForLoading 00491fb0 refreshes the passenger's GRID position at receipt.
inline bool Ready(CarrierHandshake &c,uint32_t passenger,int x,int y,int z) {
    if((c.command!=14 && c.command!=15) || c.passenger!=passenger || c.phase!=2) return false;
    c.targetX=x;c.targetY=y;c.targetZ=z;c.phase=3;c.substage=0;return true;
}
}
