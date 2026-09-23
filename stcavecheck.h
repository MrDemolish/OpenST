inline int RunCaveCheck()
{
    int failures=0;
    auto check=[&](bool ok,const char *text) {
        std::printf("cavecheck: %s: %s\n",text,ok?"OK":"FAIL"); if(!ok) ++failures;
    };
    if(!g_menu.OpenEditor(0)) return 1;
    auto tile=g_menu.edit.contours.at(g_menu.edit.contourFamily)[15];
    auto &t=g_menu.terr;
    t.bw=8;t.bh=8;t.cells.clear();t.objects.clear();t.decor.clear();
    g_menu.blds.clear();g_menu.units.clear();g_menu.occStamp=-1;
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        maps::Cell c;c.x=uint8_t(x*2);c.y=uint8_t(y*2);c.texA=1;c.mesh=4352;
        t.cells.push_back(c);
        if(x==4) for(int z=1;z<=5;++z) {
            if(y==3 && z==1) continue; // the only opening, under four rock slabs
            c=tile;c.x=uint8_t(x*2);c.y=uint8_t(y*2);c.level=uint8_t(z);
            t.cells.push_back(c);
        }
    }
    g_menu.EdTouched();
    check(g_menu.WaterAt(4,3,0) && !g_menu.WaterAt(4,3,1)
        && !g_menu.WaterAt(4,2,0),"original masks distinguish tunnel, roof and wall");
    auto field=g_menu.navigationCells;
    bool distanceOk=loading::DistanceField(t.bw*2,t.bh*2,5,{2,6,0},field);
    auto at=[&](int x,int y,int z){return (z*t.bh*2+y)*t.bw*2+x;};
    check(distanceOk && field[at(12,6,0)]>0 && field[at(8,6,1)]<0,
          "original-cost field crosses the real mesh tunnel while preserving its roof code");
    std::vector<uint8_t> occupants(field.size(),0);
    occupants[at(2,6,0)]=1;
    loading::Cell approach,passenger;
    check(loading::SelectCell(t.bw*2,t.bh*2,field,occupants,{12,6,0},approach,passenger)==0
          && approach.z==0 && passenger.z==0,
          "loading selector uses decoded mesh terrain to approach through the tunnel");
    {
        Menu::Unit carrier,cargo;carrier.type=7;carrier.hp=cargo.hp=100;
        carrier.x=2;carrier.y=6;carrier.z=0;cargo.x=12;cargo.y=6;cargo.z=0;
        g_menu.units={carrier,cargo};
        auto plan=g_menu.PlanLoadingApproach(0,1);
        bool through=false,water=true;
        for(auto p:plan.route) {
            through=through || ((p.x==8 || p.x==9) && (p.y==6 || p.y==7) && p.z==0);
            water=water && g_menu.navigationCells[at(p.x,p.y,p.z)]==0;
        }
        check(plan.result==0 && !plan.route.empty() && through && water && !plan.occupied,
              "carrier receives a cell-by-cell approach through a cave to distant cargo");
        auto last=plan.route.empty()?loading::Cell{-1,-1,-1}:plan.route.back();
        check(!plan.route.empty() && last.x==plan.carrier.x && last.y==plan.carrier.y && last.z==plan.carrier.z
              && std::abs(last.x-12)+std::abs(last.y-6)==1,
              "loading route ends beside cargo rather than inside its occupied cell");
        cargo.x=2;cargo.y=6;carrier.x=3;carrier.y=6;
        g_menu.units={carrier,cargo};plan=g_menu.PlanLoadingApproach(0,1);
        check(plan.result==0 && plan.route.empty() && plan.carrier.x==3 && plan.carrier.y==6,
              "carrier already beside cargo needs no artificial movement waypoint");
        g_menu.units[1].hp=0;
        check(g_menu.PlanLoadingApproach(0,1).result==-2,
              "approach cannot be planned to a dead cargo target");
        g_menu.units.clear();
    }
    {
        std::vector<int16_t> encoded(20,0);
        uint32_t masks[4]={8,0x88,0x8008,0};
        navigation::ApplyTile(encoded,2,2,0,0,1,1,false,masks);
        check(encoded[0]==-16385 && encoded[1]==-1 && encoded[2]==-2 && encoded[3]==0,
              "mesh corners retain distinct original navigation codes instead of one blocked flag");
    }
    std::vector<float> depths;
    auto path=g_menu.FindPath(1,3,6,3,4,&depths,0);
    bool crossed=false,clear=path.size()==depths.size() && !path.empty();
    for(size_t i=0;i<path.size();++i) {
        clear=clear && g_menu.WaterAt(path[i].x,path[i].y,int(depths[i]));
        crossed=crossed || (path[i].x==4 && path[i].y==3 && depths[i]==0);
    }
    check(clear && crossed,"3D path passes through the opening below the mountain");
    auto precise=g_menu.FindPath(1,3,6,3,4,&depths,0,2);
    bool preciseTunnel=false;
    for(size_t i=0;i<precise.size();++i)
        preciseTunnel|=precise[i].x==4 && precise[i].y==3 && depths[i]==0;
    check(!precise.empty() && depths.back()==2 && preciseTunnel,
          "exact-depth route passes below the roof then rises to the requested destination");
    check(g_menu.FindPath(1,3,4,3,4,&depths,0,1).empty() && depths.empty(),
          "exact destination inside the roof is rejected instead of moved below it");
    check(g_menu.FindPath(1,3,6,3,1,&depths,0,2).empty(),
          "exact destination above the boat depth limit is rejected");
    check(g_menu.FindPath(1,3,8,3,4,&depths,0,0).empty(),
          "exact docking target outside the map is not replaced with a nearby cell");
    check(g_menu.FindPath(4,3,6,3,4,&depths,1,2).empty(),
          "exact route cannot teleport its starting point out of rock");
    check(g_menu.ReachableDepth(4,3,0,4,4)==0,"rise order cannot cross the roof");
    Menu::Unit pushed;pushed.x=8.5f;pushed.y=6.1f;pushed.z=0;
    g_menu.Nudge(pushed,0,-.3f);
    check(pushed.y==6.1f,"separation cannot push a boat sideways into the tunnel wall");
    auto tunnel=maps::serializeCells(t);
    maps::Cell seal=tile;seal.x=8;seal.y=6;seal.level=1;t.cells.push_back(seal);g_menu.EdTouched();
    check(g_menu.FindPath(1,3,6,3,4,&depths,0).empty(),"sealed wall cannot be traversed at any depth");
    g_menu.edit.layer=1;check(g_menu.EdEraseTerrain(4,3),"erase cuts only the selected rock layer");
    check(maps::serializeCells(t)==tunnel && g_menu.WaterAt(4,3,0),"cut restores opening without deleting roof or floor");
    check(g_menu.EdUndo() && !g_menu.WaterAt(4,3,0) && g_menu.EdRedo() && g_menu.WaterAt(4,3,0),
          "undo and redo rebuild collision masks");
    g_menu.edit.layer=-1;g_menu.edit.on=false;g_menu.screen=SCR_TERRAIN;g_menu.aiOn=false;
    Menu::Unit boat;boat.type=12;boat.owner=g_menu.me;boat.x=2.5f;boat.y=6.5f;
    g_menu.SetUnitStats(boat);boat.z=boat.wantZ=0;boat.maxLevel=4;
    boat.tx=12.5f;boat.ty=6.5f;boat.path=g_menu.FindPath(1,3,6,3,4,&boat.pathZ,0);
    boat.at=0;boat.moving=true;boat.dir=g_menu.DirFrame(1,0);g_menu.units.push_back(boat);
    g_menu.lastUnitStep=1000;bool hitRoof=false;
    for(DWORD now=1100;now<=31100;now+=100) {
        g_menu.StepUnits(now);
        if(!g_menu.units.empty()) {
            const auto &u=g_menu.units.front();
            if(int(u.x)/2==4 && (!g_menu.WaterAt(4,int(u.y)/2,int(std::lround(u.z))) || u.z>.1f)) hitRoof=true;
        }
    }
    check(!g_menu.units.empty() && g_menu.units.front().x>11.5f && !hitRoof,
          "live simulation swims under the roof and reaches the other side");
    {
        Menu::Bld depot;depot.tobj=59;depot.owner=g_menu.me;depot.hp=depot.hpMax=1000;
        depot.x=12;depot.y=6;depot.z=0;depot.span=2;g_menu.blds={depot};g_menu.occStamp=-1;
        Menu::Unit freighter;freighter.type=8;freighter.owner=g_menu.me;
        g_menu.SetUnitStats(freighter);freighter.hp=freighter.hpMax;
        freighter.x=2.5f;freighter.y=6.5f;freighter.z=freighter.wantZ=0;
        freighter.hauling=true;freighter.haulPhase=2;freighter.cargo.metal=20;
        g_menu.units={freighter};g_menu.aiOn=false;g_menu.lastUnitStep=1000;
        const auto bank=g_menu.BankOf(g_menu.me).metal;bool through=false,solid=false,aligned=false;
        for(DWORD now=1040;now<=61000 && g_menu.units[0].cargo.total();now+=40) {
            g_menu.StepUnits(now);
            const auto &u=g_menu.units[0];
            through|=int(u.x)/2==4 && int(u.y)/2==3 && u.z<.1f;
            solid|=!g_menu.WaterAt(int(u.x)/2,int(u.y)/2,int(std::lround(u.z)));
            if(u.haulPhase==3)aligned=std::fabs(u.z-1)<.01f
                && std::fabs(u.x-(12+101.f/201.f))<.01f && std::fabs(u.y-(6+101.f/201.f))<.01f;
        }
        check(through && !solid && aligned && !g_menu.units[0].cargo.total()
              && g_menu.BankOf(g_menu.me).metal==bank+20,
              "live freight crosses tunnel, rises above depot and unloads at the original physical centre");
        check(g_menu.units[0].freightStage==4 && g_menu.units[0].haulTo==-1
              && g_menu.blds[0].freight.reservation==-1,
              "last crate releases hatch and starts a separate departure without losing cargo state");
        freighter.x=14;freighter.y=10;freighter.z=freighter.wantZ=1;
        g_menu.units.push_back(freighter);
        const auto berth=g_menu.units[0].freightBerth;
        bool returned=false,departed=false;
        DWORD departureNow=g_menu.lastUnitStep;
        for(int tick=0;tick<1500;++tick) {
            departureNow+=40;g_menu.StepUnits(departureNow);
            const auto &u=g_menu.units[0];
            returned|=std::fabs(u.x-berth.x)<.01f && std::fabs(u.y-berth.y)<.01f && std::fabs(u.z-berth.z)<.01f;
            solid|=!g_menu.WaterAt(int(u.x)/2,int(u.y)/2,int(std::lround(u.z)));
            if(!u.freightStage && !u.moving) {departed=true;break;}
        }
        check(returned && departed && !solid
              && (int(g_menu.units[0].x)<12 || int(g_menu.units[0].x)>=14
                  || int(g_menu.units[0].y)<6 || int(g_menu.units[0].y)>=8),
              "empty hauler reverses to its saved berth and clears the hatch even without a mine");
        for(int tick=0;tick<2000 && (g_menu.units[1].cargo.total() || g_menu.units[1].freightStage);++tick) {
            departureNow+=40;g_menu.StepUnits(departureNow);
        }
        check(!g_menu.units[1].cargo.total() && !g_menu.units[1].freightStage
              && g_menu.BankOf(g_menu.me).metal==bank+40,
              "second hauler can unload and depart after the first parks without a next job");
        g_menu.units={freighter};auto &waiting=g_menu.units[0];waiting.haulTo=0;
        g_menu.blds[0].freight.busy=1;g_menu.blds[0].freight.reservation=999;
        g_menu.StepHaul(waiting,.04f);
        check(waiting.freightWaiting && waiting.moving && !waiting.path.empty()
              && waiting.haulPhase==2 && waiting.cargo.metal==20,
              "busy hatch sends the next hauler to a reachable waiting cell without transferring cargo");
        const auto waitingSeed=waiting.freightSeed;const auto waitingPath=waiting.path.size();
        g_menu.StepHaul(waiting,.04f);
        check(waiting.freightWaiting && waiting.freightSeed==waitingSeed && waiting.path.size()==waitingPath,
              "waiting order keeps its route instead of rerolling a destination every simulation step");
        g_menu.blds[0].freight={};g_menu.StepHaul(waiting,.04f);
        check(!waiting.freightWaiting && waiting.freightStage==1 && waiting.cargo.metal==20,
              "freed hatch replaces the waiting route with the exact-depth docking approach");
        g_menu.blds[0].freight.busy=1;g_menu.blds[0].freight.reservation=999;
        g_menu.StepHaul(waiting,.04f);g_menu.ReleaseHaulTarget(waiting);
        check(!waiting.freightWaiting && !waiting.moving && waiting.path.empty() && waiting.cargo.metal==20,
              "cancelled queue clears its route while preserving cargo");
        g_menu.blds[0].z=4;g_menu.blds[0].freight={};freighter.x=12;freighter.y=6;
        g_menu.units={freighter};g_menu.StepHaul(g_menu.units[0],.04f);
        check(g_menu.units[0].haulPhase==2 && g_menu.units[0].cargo.metal==20 && !g_menu.units[0].moving,
              "top-level building cannot dock a transport outside the five-level world");
        g_menu.blds.clear();g_menu.occStamp=-1;
    }
    // Independent roof and floor sheets: sculpting either must preserve the other.
    t.cells.clear();g_menu.units.clear();g_menu.edit.on=true;g_menu.screen=SCR_EDITOR;
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        maps::Cell c;c.x=uint8_t(x*2);c.y=uint8_t(y*2);c.texA=1;c.mesh=4352;t.cells.push_back(c);
        c=tile;c.x=uint8_t(x*2);c.y=uint8_t(y*2);c.level=3;t.cells.push_back(c);
    }
    g_menu.EdTouched();
    auto layer=[&](int z) { maps::Terrain q;for(const auto &c:t.cells) if(c.level==z) q.cells.push_back(c);return maps::serializeCells(q); };
    auto floor=layer(0),roof=layer(3);
    g_menu.edit.layer=-1;check(g_menu.EdRaise(3,3) && layer(0)==floor && g_menu.WaterAt(3,3,0),
          "raising roof preserves lower floor and the tunnel");
    g_menu.edit.layer=0;check(g_menu.EdRaise(3,3) && layer(3)==roof,
          "editing lower floor preserves original roof records");
    check(g_menu.terr.hasLevel(3,3,1) && g_menu.terr.hasLevel(3,3,3),"floor and roof remain separate layers");
    // Paint an elevated sheet over a flat floor, leaving two water levels below.
    t.cells.erase(std::remove_if(t.cells.begin(),t.cells.end(),[](const auto &c){return c.level!=0;}),t.cells.end());
    g_menu.EdTouched();floor=layer(0);
    g_menu.edit.layer=-1;g_menu.edit.platformLevel=3;g_menu.edit.brush=0;
    g_menu.EdPushUndo();
    check(g_menu.EdPlatform(3,3,false)>0 && layer(0)==floor
        && g_menu.WaterAt(3,3,0) && g_menu.WaterAt(3,3,1) && !g_menu.WaterAt(3,3,2),
        "platform at level 3 adds a roof without filling the water below");
    auto platform=maps::serializeCells(t);
    check(g_menu.EdPlatform(3,3,false)==0 && maps::serializeCells(t)==platform,
          "painting the same platform twice is idempotent");
    check(g_menu.EdUndo() && layer(3).empty() && g_menu.EdRedo() && maps::serializeCells(t)==platform,
          "platform undo and redo preserve all sheets");
    check(g_menu.EdPlatform(4,3,false)>0,"adjacent stroke extends the platform");
    bool joined=true;
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        int a=g_menu.EdCellIndex(x,y,3);if(a<0) continue;
        auto z=land::Set::corners(*g_menu.landSet.mesh(t.cells[size_t(a)].mesh));
        int b=g_menu.EdCellIndex(x+1,y,3),c=g_menu.EdCellIndex(x,y+1,3);
        if(x<7 && b>=0) {
            auto q=land::Set::corners(*g_menu.landSet.mesh(t.cells[size_t(b)].mesh));
            joined=joined && std::fabs(z[1]-q[0])<.15f && std::fabs(z[2]-q[3])<.15f;
        }
        if(y<7 && c>=0) {
            auto q=land::Set::corners(*g_menu.landSet.mesh(t.cells[size_t(c)].mesh));
            joined=joined && std::fabs(z[3]-q[0])<.15f && std::fabs(z[2]-q[1])<.15f;
        }
    }
    check(joined,"platform edges share corner heights after multiple strokes");
    seal=tile;seal.level=5;seal.x=6;seal.y=6;t.cells.push_back(seal);g_menu.EdTouched();
    auto upper=layer(5);
    check(g_menu.EdPlatform(3,3,true)>0 && !t.hasLevel(3,3,3) && layer(0)==floor && layer(5)==upper
        && g_menu.WaterAt(3,3,2),"platform eraser opens a hole without touching floor or higher roof");
    g_menu.edit.tool=ed::T_PLATFORM;g_menu.edit.view=ed::V_ISO;g_menu.terrLevel=-1;
    g_clientW=1280;g_clientH=860;g_menu.FitCanvas();g_menu.EdCentre();g_menu.ComposeEditor();
    RECT down=g_menu.edit.controlRect[10],up=g_menu.edit.controlRect[11];
    g_menu.EditorClick((down.left+down.right)/2,(down.top+down.bottom)/2,false);
    check(g_menu.edit.platformLevel==2,"platform height decrease button");
    g_menu.EditorClick((up.left+up.right)/2,(up.top+up.bottom)/2,false);
    check(g_menu.edit.platformLevel==3,"platform height increase button");
    int sx,sy;g_menu.CellToScreen(5.f,11.f,sx,sy);
    sy+=(std::max(0,t.topAt(2,5))-g_menu.edit.platformLevel)*g_menu.LevelStep();
    g_menu.EditorMove(sx,sy);
    check(g_menu.edit.curBX==2 && g_menu.edit.curBY==5,"platform cursor hits its selected height plane");
    g_menu.EditorClick(sx,sy,false);g_menu.EditorRelease();
    check(t.hasLevel(2,5,3),"mouse paints platform at the cursor location");
    g_menu.EditorMove(sx,sy);g_menu.EditorClick(sx,sy,true);g_menu.EditorRelease();
    check(!t.hasLevel(2,5,3) && layer(0)==floor,"right mouse cuts selected platform only");
    FILE *shot=std::fopen("editor_platform.raw","wb");
    if(shot){std::fwrite(g_menu.canvas.data(),4,g_menu.canvas.size(),shot);std::fclose(shot);}
    // Exact launch cells are temporary obstacles to the existing 2x2 path graph.
    t.cells.clear();
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        maps::Cell c;c.x=uint8_t(x*2);c.y=uint8_t(y*2);c.texA=1;c.mesh=4352;
        t.cells.push_back(c);
    }
    g_menu.EdTouched();g_menu.blds.clear();g_menu.units.clear();
    Menu::Bld reservation;reservation.hp=1000;reservation.tobj=50;reservation.x=reservation.y=0;
    reservation.dockJob=1;reservation.dockPhase=1;reservation.dockX=6;reservation.dockY=6;reservation.dockZ=0;
    g_menu.blds={reservation};g_menu.occStamp=-1;
    auto route=g_menu.FindPath(1,3,6,3,0,&depths,0);
    bool avoids=!route.empty();
    for(const auto &p:route) if(p.x==3 && p.y==3) avoids=false;
    check(avoids,"route detours around a dock reservation at the same depth");
    auto blockedGoal=g_menu.FindPath(1,3,3,3,0,&depths,0);
    check(blockedGoal.empty(),"destination exception cannot bypass a reserved launch cell");
    auto escape=g_menu.FindPath(3,3,6,3,0,&depths,0);
    check(!escape.empty(),"boat already inside a newly reserved block can plan an escape");
    g_menu.blds[0].dockZ=1;
    route=g_menu.FindPath(1,3,6,3,0,&depths,0);
    bool under=false;for(const auto &p:route) if(p.x==3 && p.y==3) under=true;
    check(under,"reservation above the route does not seal water below it");
    g_menu.blds[0].dockZ=0;g_menu.blds[0].hp=0;
    route=g_menu.FindPath(1,3,6,3,0,&depths,0);
    bool freed=false;for(const auto &p:route) if(p.x==3 && p.y==3) freed=true;
    check(freed,"destroyed dock immediately reopens its reserved block to pathfinding");
    g_menu.blds.clear();g_menu.occStamp=-1;
    auto vertical=g_menu.FindPath(2,2,2,2,4,&depths,0,2);
    check(vertical.size()==2 && depths==std::vector<float>({1,2})
          && vertical.back().x==2 && vertical.back().y==2,
          "same-XY docking route includes the two required vertical steps");
    g_menu.blds={reservation};g_menu.blds[0].dockX=g_menu.blds[0].dockY=4;g_menu.blds[0].dockZ=2;
    check(g_menu.FindPath(1,2,2,2,4,&depths,0,2).empty(),
          "exact-depth endpoint still respects another dock's launch reservation");
    g_menu.blds.clear();g_menu.occStamp=-1;
    Menu::Unit rising;rising.type=12;rising.owner=g_menu.me;
    g_menu.SetUnitStats(rising);rising.hp=rising.hpMax;rising.maxLevel=4;
    rising.x=rising.y=rising.tx=rising.ty=4.5f;rising.z=0;rising.wantZ=2;
    rising.path=g_menu.FindPath(2,2,2,2,4,&rising.pathZ,0,2);rising.moving=true;
    g_menu.units={rising};g_menu.lastUnitStep=1000;
    for(DWORD now=1040;now<=5040;now+=40)g_menu.StepUnits(now);
    check(!g_menu.units[0].moving && std::fabs(g_menu.units[0].z-2)<.01f
          && std::fabs(g_menu.units[0].x-4.5f)<.01f && std::fabs(g_menu.units[0].y-4.5f)<.01f,
          "live movement consumes a vertical-only docking route without drifting sideways");
    std::printf("cavecheck: %d failures\n",failures);
    return failures?1:0;
}
