// Included inside Menu. Original selector 0048d7c0, final XYZ from 0046d450.
// The route still uses the remake's 2x2 graph; its occupancy is an adapter.
    bool FreightCellFree(freightberth::Cell p,const Unit &boat) const
    {
        if(p.x<0 || p.y<0 || p.x>=terr.bw*2 || p.y>=terr.bh*2 || p.z<0 || p.z>4)
            return false;
        if(!WaterAt(p.x/2,p.y/2,p.z) || DockReserved(p.x,p.y,p.z)) return false;
        for(const auto &b:blds) if(b.hp>0 && BldLevel(b)==p.z
            && p.x>=b.x && p.x<b.x+BldSpan(b) && p.y>=b.y && p.y<b.y+BldSpan(b)) return false;
        for(const auto &u:units) if(&u!=&boat && u.hp>0 && !u.carried
            && int(std::floor(u.x))==p.x && int(std::floor(u.y))==p.y
            && int(std::lround(u.z))==p.z) return false;
        return true;
    }

    std::vector<uint8_t> FreightSlots() const
    {
        EnsureWaterGrid();
        int w=terr.bw*2,h=terr.bh*2;
        std::vector<uint8_t> slots(navigationCells.size(),0);
        auto mark=[&](int x,int y,int z,uint8_t bits) {
            if(x>=0 && x<w && y>=0 && y<h && z>=0 && z<5)
                slots[(z*h+y)*w+x]|=bits;
        };
        // Live occupancy remains an adapter. The selector itself tests both
        // original slots, including the cell underneath a proposed destination.
        for(const auto &b:blds) if(b.hp>0) {
            for(int dy=0;dy<BldSpan(b);++dy)for(int dx=0;dx<BldSpan(b);++dx)
                mark(b.x+dx,b.y+dy,BldLevel(b),1);
            if(b.dockJob>0 && (b.dockPhase==1 || b.dockPhase==2))
                mark(b.dockX,b.dockY,b.dockZ,2);
        }
        for(const auto &other:units) if(other.hp>0 && !other.carried)
            mark(int(std::floor(other.x)),int(std::floor(other.y)),int(std::lround(other.z)),1);
        return slots;
    }

    void WaitForFreight(Unit &u,const Bld &b)
    {
        if(u.freightWaiting) return; // finish the waiting route, then hold its endpoint
        u.moving=false;u.path.clear();u.pathZ.clear();u.wantZ=u.z;u.freightStage=0;
        auto slots=FreightSlots();
        loading::Cell start{int(std::floor(u.x)),int(std::floor(u.y)),int(std::lround(u.z))};
        loading::Cell goal{b.x,b.y,BldLevel(b)+1},chosen;
        if(!freightclear::selectNear(terr.bw*2,terr.bh*2,navigationCells,slots,start,goal,1,u.freightSeed,chosen)
            || chosen.z>u.maxLevel || !FreightCellFree({chosen.x,chosen.y,chosen.z},u)) return;
        std::vector<float> depths;
        auto path=FindPath(start.x/2,start.y/2,chosen.x/2,chosen.y/2,u.maxLevel,&depths,u.z,chosen.z);
        if(path.empty() && (start.x/2!=chosen.x/2 || start.y/2!=chosen.y/2 || start.z!=chosen.z)) return;
        u.freightBuilding={b.x,b.y,BldLevel(b)};u.freightWaiting=true;u.freightBerthValid=false;
        u.path=std::move(path);u.pathZ=std::move(depths);u.at=0;
        u.tx=float(chosen.x);u.ty=float(chosen.y);u.wantZ=float(chosen.z);u.moving=true;
    }

    bool HaulApproach(Unit &u,Bld &b,float secs)
    {
        using freightberth::Cell;
        Cell building{b.x,b.y,BldLevel(b)},size{terr.bw*2,terr.bh*2,5};
        const int z=building.z+1;
        // Original physical target is (bX+1)*201,(bY+1)*201,bZ*200+300.
        const float gx=b.x+101.f/201.f,gy=b.y+101.f/201.f;
        if((u.freightStage || u.freightWaiting) && (u.freightBuilding.x!=b.x || u.freightBuilding.y!=b.y || u.freightBuilding.z!=building.z)) {
            ReleaseHaulTarget(u);return false;
        }
        if(z>u.maxLevel || z>=5) {u.moving=false;u.path.clear();u.pathZ.clear();return false;}
        if(HatchBusy(u.haulTo,u)) {
            WaitForFreight(u,b);return false;
        }
        if(u.freightWaiting) {
            u.freightWaiting=false;u.moving=false;u.path.clear();u.pathZ.clear();u.freightStage=0;u.wantZ=u.z;
        }
        if(!FreightCellFree({b.x,b.y,z},u)) {
            u.moving=false;u.path.clear();u.pathZ.clear();
            if(u.freightStage==1) u.freightStage=0;
            return false;
        }
        if(std::fabs(u.x-gx)<.02f && std::fabs(u.y-gy)<.02f && std::fabs(u.z-z)<.02f) {
            if(!ClaimFreight(u,b)) return false;
            u.moving=false;u.path.clear();u.pathZ.clear();u.wantZ=u.z;
            u.freightStage=0;return true;
        }
        if(u.freightStage==0) {
            Cell start{int(std::floor(u.x)),int(std::floor(u.y)),int(std::lround(u.z))},chosen;
            // The original selector tests object occupancy, then the movement
            // controller validates the selected destination against terrain.
            auto occupied=[&](Cell p) {
                for(const auto &other:blds) if(other.hp>0 && BldLevel(other)==p.z
                    && p.x>=other.x && p.x<other.x+BldSpan(other)
                    && p.y>=other.y && p.y<other.y+BldSpan(other)) return true;
                for(const auto &other:units) if(other.hp>0 && !other.carried
                    && int(std::floor(other.x))==p.x && int(std::floor(other.y))==p.y
                    && int(std::lround(other.z))==p.z) return true;
                return false;
            };
            if(!freightberth::select(start,building,size,occupied,chosen) || !FreightCellFree(chosen,u)) return false;
            auto path=FindPath(start.x/2,start.y/2,chosen.x/2,chosen.y/2,u.maxLevel,&u.pathZ,u.z,chosen.z);
            if(path.empty() && (start.x/2!=chosen.x/2 || start.y/2!=chosen.y/2 || start.z!=chosen.z)) return false;
            u.freightBerth=chosen;u.freightBuilding=building;u.freightStage=1;u.freightBerthValid=true;
            u.path=std::move(path);u.at=0;u.tx=float(chosen.x);u.ty=float(chosen.y);
            u.wantZ=float(chosen.z);u.moving=true;return false;
        }
        if(u.freightStage==1) {
            if(u.moving) return false;
            if(std::fabs(u.z-z)>.05f || std::fabs(u.x-u.freightBerth.x)>.05f
                || std::fabs(u.y-u.freightBerth.y)>.05f) {u.freightStage=0;return false;}
            if(!ClaimFreight(u,b)) return false;
            u.freightStage=2;u.path.clear();u.pathZ.clear();
        }
        if(u.freightStage==2) {
            int dir=DirFrame(gx-u.x,gy-u.y);TurnBoat(u,dir,secs);
            if(u.dir!=dir) return false;
            u.freightStart[0]=int(std::lround(u.x*201+100));
            u.freightStart[1]=int(std::lround(u.y*201+100));
            u.freightStart[2]=int(std::lround(u.z*200+100));
            u.freightEnd[0]=(b.x+1)*201;u.freightEnd[1]=(b.y+1)*201;u.freightEnd[2]=building.z*200+300;
            double length=0;for(int k=0;k<3;++k) {double d=u.freightEnd[k]-u.freightStart[k];length+=d*d;}
            u.freightSteps=std::max(1,int(std::sqrt(length)+.5)/std::max(1,u.raw));
            u.freightStep=0;u.freightClock=0;u.freightStage=3;return false;
        }
        // 00415b30/00415ed0: integer interpolation at 25 ticks per second.
        u.freightClock+=std::max(0.0,double(secs))*25;
        int ticks=int(u.freightClock+1e-6);u.freightClock-=ticks;
        while(ticks-- > 0 && u.freightStep<u.freightSteps) {
            int next=u.freightStep+1,p[3];
            for(int k=0;k<3;++k)p[k]=u.freightStart[k]+(u.freightEnd[k]-u.freightStart[k])*next/u.freightSteps;
            float nx=float(p[0]-100)/201,ny=float(p[1]-100)/201,nz=float(p[2]-100)/200;
            if(!FreightCellFree({int(std::floor(nx)),int(std::floor(ny)),int(std::lround(nz))},u)) return false;
            u.x=nx;u.y=ny;u.z=u.wantZ=nz;u.freightStep=next;
        }
        if(u.freightStep<u.freightSteps) return false;
        u.freightStage=0;return true;
    }

    void BeginFreightDeparture(Unit &u)
    {
        const bool docked=u.freightBerthValid;
        // 0048d930 releases the building before UnLoadRC phase 6 reverses
        // the final approach to the saved berth. Keep only its coordinates.
        ReleaseHaulTarget(u);
        if(docked) u.freightStage=4;
    }

    void StepFreightDeparture(Unit &u,float secs)
    {
        if(u.freightStage==6) {
            if(!u.moving) u.freightStage=0;
            return;
        }
        if(u.freightStage==4) {
            int dir=DirFrame(float(u.freightBerth.x)-u.x,float(u.freightBerth.y)-u.y);
            TurnBoat(u,dir,secs);if(u.dir!=dir) return;
            u.freightStart[0]=int(std::lround(u.x*201+100));
            u.freightStart[1]=int(std::lround(u.y*201+100));
            u.freightStart[2]=int(std::lround(u.z*200+100));
            u.freightEnd[0]=u.freightBerth.x*201+100;
            u.freightEnd[1]=u.freightBerth.y*201+100;
            u.freightEnd[2]=u.freightBerth.z*200+100;
            double length=0;for(int k=0;k<3;++k){double d=u.freightEnd[k]-u.freightStart[k];length+=d*d;}
            u.freightSteps=std::max(1,int(std::sqrt(length)+.5)/std::max(1,u.raw));
            u.freightStep=0;u.freightClock=0;u.freightStage=5;return;
        }
        const bool returning=u.freightStep<u.freightSteps;
        u.freightClock+=std::max(0.0,double(secs))*25;
        int ticks=int(u.freightClock+1e-6);u.freightClock-=ticks;
        while(ticks-- > 0 && u.freightStep<u.freightSteps) {
            int next=u.freightStep+1,p[3];
            for(int k=0;k<3;++k)p[k]=u.freightStart[k]+(u.freightEnd[k]-u.freightStart[k])*next/u.freightSteps;
            float nx=float(p[0]-100)/201,ny=float(p[1]-100)/201,nz=float(p[2]-100)/200;
            if(!FreightCellFree({int(std::floor(nx)),int(std::floor(ny)),int(std::lround(nz))},u)) return;
            u.x=nx;u.y=ny;u.z=u.wantZ=nz;u.freightStep=next;
        }
        if(returning) return;
        auto slots=FreightSlots();
        int w=terr.bw*2,h=terr.bh*2;
        loading::Cell start{int(std::floor(u.x)),int(std::floor(u.y)),int(std::lround(u.z))},chosen;
        if(!freightclear::select(w,h,navigationCells,slots,start,u.freightSeed,chosen)
            || chosen.z>u.maxLevel || !FreightCellFree({chosen.x,chosen.y,chosen.z},u)) return;
        std::vector<float> depths;
        auto path=FindPath(start.x/2,start.y/2,chosen.x/2,chosen.y/2,u.maxLevel,&depths,u.z,chosen.z);
        if(path.empty() && (start.x/2!=chosen.x/2 || start.y/2!=chosen.y/2 || start.z!=chosen.z)) return;
        u.path=std::move(path);u.pathZ=std::move(depths);u.at=0;
        u.tx=float(chosen.x);u.ty=float(chosen.y);u.wantZ=float(chosen.z);u.moving=true;u.freightStage=6;
    }
