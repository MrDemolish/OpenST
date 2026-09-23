// Included inside Menu. Terrain costs and destination selection follow the
// original; route tie breaking is the remake's priority queue order.
    struct LoadingApproach {
        int result=-2;
        bool occupied=false;
        loading::Cell carrier,passenger;
        std::vector<loading::Cell> route;
    };

    LoadingApproach PlanLoadingApproach(int who,int target) const
    {
        LoadingApproach plan;
        if(who<0 || target<0 || who>=int(units.size()) || target>=int(units.size()) || who==target) return plan;
        const Unit &r=units[size_t(who)],&c=units[size_t(target)];
        if(r.hp<=0 || c.hp<=0 || r.carried || c.carried || r.launchStage || c.launchStage) return plan;
        EnsureWaterGrid();
        int w=terr.bw*2,h=terr.bh*2;
        auto cell=[](const Unit &u){return loading::Cell{int(std::floor(u.x)),int(std::floor(u.y)),int(std::lround(u.z))};};
        auto valid=[&](loading::Cell p){return p.x>=0 && p.x<w && p.y>=0 && p.y<h && p.z>=0 && p.z<5;};
        auto index=[&](loading::Cell p){return (p.z*h+p.y)*w+p.x;};
        auto start=cell(r),goal=cell(c);
        if(!valid(start) || !valid(goal)) return plan;
        auto distances=navigationCells;
        std::vector<int> parents;
        if(!loading::DistanceField(w,h,5,start,distances,&parents)) return plan;
        std::vector<uint8_t> occupied(distances.size(),0);
        // Current remake occupancy adapter; two original object-grid slots
        // and moving-unit reservations still need their own port.
        for(const Bld &b:blds) if(b.hp>0) {
            int span=BldSpan(b);
            for(int y=b.y;y<b.y+span;++y) for(int x=b.x;x<b.x+span;++x) {
                loading::Cell p{x,y,BldLevel(b)};if(valid(p)) occupied[index(p)]=2;
            }
        }
        for(size_t j=0;j<units.size();++j) {
            const Unit &u=units[j];if(u.hp<=0 || u.carried) continue;
            auto p=cell(u);if(valid(p)) occupied[index(p)]=(int(j)==who)?1:2;
        }
        plan.result=loading::SelectCell(w,h,distances,occupied,goal,plan.carrier,plan.passenger);
        if(plan.result!=0) return plan;
        plan.occupied=occupied[index(plan.carrier)]==2;
        plan.route=loading::TraceRoute(w,h,5,start,plan.carrier,parents);
        if(plan.route.empty() && index(start)!=index(plan.carrier)) plan.result=-2;
        return plan;
    }
