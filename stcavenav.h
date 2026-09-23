// Included inside Menu. BuildTerrainGrid (00575640), _mfTMapSetMeshParam
// (006ef530): four masks at MAPMESH +4 describe occupied cells BELOW a tile.
// A high roof does not imply that everything underneath it is solid.
    mutable uint64_t waterRevision = ~uint64_t(0);
    mutable std::string waterLand;
    mutable std::vector<uint8_t> rockCells;
    mutable std::vector<int16_t> navigationCells;

    void EnsureWaterGrid() const
    {
        if (waterRevision==terr.revision && waterLand==landName) return;
        rockCells.assign(size_t(terr.bw)*terr.bh*5,0);
        navigationCells.assign(size_t(terr.bw)*terr.bh*20,0);
        auto &set=const_cast<land::Set &>(landSet);
        for (const auto &c:terr.cells) {
            if (!c.level || (c.mesh & 0x4000)) continue;
            const auto *m=set.mesh(c.mesh);
            if (!m) continue;
            int count=((c.mesh>>8)&15)-((c.mesh&0x1000)?1:0);
            navigation::ApplyTile(navigationCells,terr.bw*2,terr.bh*2,c.x,c.y,
                                  c.level,count,false,m->collision.data());
            for(int d=0;d<count && d<4;++d) {
                int z=int(c.level)-1-d;
                if(z<0 || z>=5) continue;
                size_t i=(size_t(z)*terr.bh+c.y/2)*terr.bw+c.x/2;
                for(int k=0;k<4;++k)
                    if(m->collision[size_t(k)] & (8u>>d)) rockCells[i]|=uint8_t(1u<<k);
            }
        }
        waterRevision=terr.revision; waterLand=landName;
    }

    bool WaterAt(int bx,int by,int z) const
    {
        if(bx<0 || by<0 || bx>=terr.bw || by>=terr.bh || z<0 || z>=5) return false;
        EnsureWaterGrid();
        // Routes use 2x2 blocks; require space for the whole footprint.
        return !rockCells[(size_t(z)*terr.bh+by)*terr.bw+bx];
    }

    bool Passable(int bx,int by,int maxLevel) const
    {
        for(int z=0;z<=std::min(4,maxLevel);++z) if(WaterAt(bx,by,z)) return true;
        return false;
    }
    bool Passable(int bx,int by) const { return Passable(bx,by,passLevel); }

    float ReachableDepth(int bx,int by,float current,float wanted,int maxLevel) const
    {
        int hi=std::clamp(maxLevel,0,4), at=std::clamp(int(std::lround(current)),0,hi);
        if(!WaterAt(bx,by,at)) {
            for(int d=1;d<5;++d) {
                if(at+d<=hi && WaterAt(bx,by,at+d)) { at+=d; break; }
                if(at-d>=0 && WaterAt(bx,by,at-d)) { at-=d; break; }
            }
        }
        int lo=at, up=at;
        while(lo>0 && WaterAt(bx,by,lo-1)) --lo;
        while(up<hi && WaterAt(bx,by,up+1)) ++up;
        return std::clamp(wanted,float(lo),float(up));
    }

    // 3D A*: each waypoint carries its depth. No diagonal may clip the
    // intervening rock. Existing diagnostic callers can ignore the depths.
    std::vector<POINT> FindPath(int sx,int sy,int gx,int gy,int maxLevel,
                               std::vector<float> *depths=nullptr,float startZ=-1,int goalZ=-1)
    {
        std::vector<POINT> out;
        if(depths) depths->clear();
        int W=terr.bw,H=terr.bh,Z=std::clamp(maxLevel+1,1,5),area=W*H;
        if(!area || sx<0 || sy<0 || sx>=W || sy>=H) return out;
        const bool exact=goalZ>=0;
        // Docking must fail rather than silently substituting another XY or Z.
        if(exact && (goalZ>=Z || !WaterAt(gx,gy,goalZ))) return out;
        if(exact && startZ>=0 && (std::lround(startZ)>=Z
            || !WaterAt(sx,sy,int(std::lround(startZ))))) return out;
        SyncOcc();
        if(!exact && !Passable(gx,gy,Z-1)) {
            int best=INT_MAX,tx=gx,ty=gy;
            for(int r=1;r<8 && best==INT_MAX;++r)
                for(int y=-r;y<=r;++y) for(int x=-r;x<=r;++x)
                    if(Passable(gx+x,gy+y,Z-1) && x*x+y*y<best) {
                        best=x*x+y*y;tx=gx+x;ty=gy+y;
                    }
            if(best==INT_MAX) return out;
            gx=tx;gy=ty;
        }
        int sz=int(ReachableDepth(sx,sy,startZ<0?float(TopAt(sx,sy)):startZ,
                                 startZ<0?float(TopAt(sx,sy)):startZ,Z-1));
        // Navigation uses 2x2 blocks. Reserve the occupied block at the launch
        // depth, leaving other depths available (including water below roofs).
        std::vector<uint8_t> reserved(size_t(area*Z),0);
        for(const Bld &b:blds) {
            if(b.hp<=0 || b.dockJob<=0 || (b.dockPhase!=1 && b.dockPhase!=2)) continue;
            int x=b.dockX/2,y=b.dockY/2,z=b.dockZ;
            if(b.dockX>=0 && b.dockY>=0 && x<W && y<H && z>=0 && z<Z)
                reserved[size_t((z*H+y)*W+x)]=1;
        }
        auto walk=[&](int x,int y,int z) {
            if(z>=Z || !WaterAt(x,y,z)) return false;
            bool startCell=x==sx && y==sy && z==sz;
            if(!startCell && reserved[size_t((z*H+y)*W+x)]) return false;
            return (x==gx && y==gy)||(x==sx && y==sy)||!BldAt(x,y);
        };
        if(!walk(sx,sy,sz)) return out;
        auto id=[&](int x,int y,int z){ return (z*H+y)*W+x; };
        auto heuristic=[&](int x,int y,int z) {
            int a=std::abs(gx-x),b=std::abs(gy-y),c=exact?std::abs(goalZ-z):0;
            return a+b+c+2*std::max({a,b,c});
        };
        int start=id(sx,sy,sz),goal=-1;
        std::vector<int> costs(size_t(area*Z),INT_MAX),from(size_t(area*Z),-1);
        std::vector<uint8_t> closed(size_t(area*Z),0);
        using Node=std::pair<int,int>;
        std::priority_queue<Node,std::vector<Node>,std::greater<Node>> open;
        costs[size_t(start)]=0;open.push({heuristic(sx,sy,sz),start});
        while(!open.empty()) {
            int at=open.top().second;open.pop();
            if(closed[size_t(at)]) continue;
            closed[size_t(at)]=1;
            int x=at%W,y=(at/W)%H,z=at/area;
            if(x==gx && y==gy && (!exact || z==goalZ)) {goal=at;break;}
            for(int dz=-1;dz<=1;++dz) for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx) {
                int axes=(dx!=0)+(dy!=0)+(dz!=0);
                if(!axes || !walk(x+dx,y+dy,z+dz)) continue;
                bool clear=true;
                for(int k=1;k<8 && clear;++k) {
                    int a=(k&1)?dx:0,b=(k&2)?dy:0,c=(k&4)?dz:0;
                    if((a||b||c) && !walk(x+a,y+b,z+c)) clear=false;
                }
                if(!clear) continue;
                int next=id(x+dx,y+dy,z+dz),cost=costs[size_t(at)]+axes+2;
                if(cost>=costs[size_t(next)]) continue;
                costs[size_t(next)]=cost;from[size_t(next)]=at;
                open.push({cost+heuristic(x+dx,y+dy,z+dz),next});
            }
        }
        if(goal<0) return out;
        std::vector<float> zs;
        for(int at=goal;at!=start;at=from[size_t(at)]) {
            out.push_back(POINT{at%W,(at/W)%H});zs.push_back(float(at/area));
        }
        std::reverse(out.begin(),out.end());std::reverse(zs.begin(),zs.end());
        if(depths) *depths=std::move(zs);
        return out;
    }
