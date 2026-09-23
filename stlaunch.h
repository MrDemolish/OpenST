// Included inside Menu. 0046cf20/00415ed0 keep grid and physical XYZ separate.
// Remake coordinates place cell centres at integers: subtract world centre.
    static void UnitPhysicalPosition(const Unit &u,float &x,float &y,float &z)
    {
        x=u.x;y=u.y;z=u.z;
        if(!u.launchStage) return;
        int p[3];
        for(int k=0;k<3;++k)
            p[k]=u.launchStart[k]+(u.launchEnd[k]-u.launchStart[k])*u.launchStep/std::max(1,u.launchSteps);
        x=float(p[0]-100)/201;y=float(p[1]-100)/201;z=float(p[2]-100)/200;
    }

    // CreateGame chooses the route after emergence, using the rally point
    // copied into its creation message. Obstacles may have changed meanwhile.
    bool PlanLaunchDeparture(Unit &u)
    {
        u.path.clear();u.pathZ.clear();u.at=0;u.moving=false;
        u.tx=u.x;u.ty=u.y;
        SyncOcc();
        auto route=[&](int x,int y) {
            if(x<0 || y<0 || x>=terr.bw*2 || y>=terr.bh*2
                || !Free(x/2,y/2,u.maxLevel)) return false;
            std::vector<float> depths;
            auto path=FindPath(int(u.x)/2,int(u.y)/2,x/2,y/2,u.maxLevel,&depths,u.z);
            if(path.empty()) return false;
            u.tx=float(x);u.ty=float(y);u.path=std::move(path);u.pathZ=std::move(depths);
            u.moving=true;return true;
        };
        if(route(u.launchRallyX,u.launchRallyY)) return true;
        // Existing remake neighbourhood search; original 0048dfd0 remains to port.
        static const int dx[8]={1,1,0,-1,-1,-1,0,1},dy[8]={0,1,1,1,0,-1,-1,-1};
        for(int r=2;r<7;++r) for(int k=0;k<8;++k)
            if(route(int(u.x)+dx[k]*r,int(u.y)+dy[k]*r)) return true;
        return false;
    }

    void StepLaunches(float secs)
    {
        for(Unit &u:units) {
            if(!u.launchStage || u.hp<=0 || secs<=0) continue;
            if(u.launchStage==1) {
                int dx=u.launchEnd[0]-u.launchStart[0],dy=u.launchEnd[1]-u.launchStart[1];
                int want=(dx||dy)?DirFrame(float(dx),float(dy)):u.dir;
                TurnBoat(u,want,secs);
                if(u.dir!=want) continue;
                u.launchStage=2;
                continue;
            }
            u.launchClock+=double(secs)*25;
            int ticks=int(u.launchClock+1e-6);u.launchClock-=ticks;
            u.launchStep=std::min(u.launchSteps,u.launchStep+ticks);
            if(u.launchStep==u.launchSteps) {
                // dockUnit is the matching ID reference from 004cf3e0.
                // Reap remaps both directions; losing the dock does not teleport the boat.
                u.launchStage=0;u.launchBld=-1;u.launchClock=0;
                PlanLaunchDeparture(u);
            }
        }
    }
