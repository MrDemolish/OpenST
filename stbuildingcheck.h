inline int RunBuildingCheck()
{
    int failures = 0;
    auto check = [&](bool ok, const char *name) {
        std::printf("buildingcheck: %s: %s\n", name, ok ? "OK" : "FAIL");
        if (!ok) ++failures;
    };
    auto &m = g_menu;
    if (!m.OpenEditor(0)) return 1;
    m.blds.clear(); m.units.clear(); m.me = 0; m.players[0].civ = 0;
    Menu::Bld b; b.tobj = 57; b.hp = b.hpMax = 1000; b.kind = 221;
    m.blds.push_back(b);
    Menu::Unit u; u.hp = 100; u.hauling = true; u.haulTo = 0; u.haulPhase = 1;
    m.units.push_back(u);
    auto &mine = m.blds[0];
    mine.aniState = 1; m.StepWork(0);
    auto lid = m.WorkStrip(mine);
    if (!lid || lid->count() < 2) return 1;
    int count = int(lid->count());
    const auto *timing=m.WorkTiming(mine);
    check(timing && timing->ticks.size()==size_t(count) && timing->ticks[0]==2,
          "mine hatch timing comes from its type-29 descriptor");
    check(m.WorkFrame(mine, count) == 0, "opening starts at the closed frame");
    m.StepWork(0.04f);
    check(m.WorkFrame(mine,count)==0,"hatch holds frame zero for the first original tick");
    m.StepWork(0.04f);
    check(m.WorkFrame(mine,count)==1,"hatch advances after two original ticks");
    m.StepWork(100);
    check(m.WorkFrame(mine, count) == count - 1, "opening holds its final frame instead of looping");
    mine.aniState = 2; m.StepWork(0);
    auto flow = m.WorkStrip(mine);
    check(flow && m.WorkFrame(mine, int(flow->count())) == int(flow->count()) - 1,
          "mine transfer starts a fresh reverse sequence");
    mine.aniState = 3; m.StepWork(0);
    check(m.WorkFrame(mine, count) == count - 1, "closing starts open, reversed exactly once");
    m.StepWork(0.1f);
    check(m.WorkFrame(mine, count) < count - 1, "closing advances toward the closed frame");
    m.StepWork(100);
    check(m.WorkFrame(mine, count) == 0, "closing reaches the closed frame");
    mine.aniState = 2; m.StepWork(0); m.units[0].hauling = false; m.StepWork(0);
    check(mine.aniState == 3, "cancelled transport closes its hatch");
    m.StepWork(100); check(mine.aniState == 0, "cancelled transfer leaves no running work overlay");
    mine.aniState = 2; mine.buildLeft = 10; m.StepWork(1);
    check(mine.aniStep == 0 && m.DrawWork(mine, 0, 0) == 0, "unfinished building does not play work overlay");
    mine.buildLeft = 0;

    const auto *overrideTiming=m.unitSet.objectTiming("lla_clop");
    check(overrideTiming && overrideTiming->ticks.size()==64
          && std::all_of(overrideTiming->ticks.begin(),overrideTiming->ticks.end(),
                         [](uint32_t t){return t==2;}),
          "original tower descriptor with 64 override records loads intact");
    sequence::Timing fixture;
    fixture.ticks={2,5,1};
    check(fixture.frame(2,false,false)==1 && fixture.frame(6,false,false)==1
          && fixture.frame(7,false,false)==2 && fixture.frame(8,false,true)==0,
          "unequal frame durations and loop boundary are respected");
    check(fixture.frame(1,true,false)==1 && fixture.frame(6,true,false)==0,
          "reverse playback uses the reversed frame's own duration");
    std::vector<uint8_t> bad(80,0); bad[0]='x'; bad[72]=2; bad[76]=1;
    check(!fixture.load(bad,3),"truncated timing overrides are rejected");
    mine.tobj=51; mine.aniState=0; mine.overlayTicks=0;
    m.StepWork(0.08f);
    check(std::abs(mine.overlayTicks-2.0)<0.0001,
          "idle overlay has its own simulation clock");
    mine.moveT=1; m.StepWork(1);
    check(mine.overlayTicks==0 && m.DrawBldAni(mine,0,0)==0,
          "relocation stops the building overlay");
    mine.moveT=0; mine.tobj=57;
    check(fixture.frameRange(0,1,2,true)==0,
          "invalid timing descriptor leaves no playable frames");
    fixture.ticks={2,5,1};
    check(fixture.frameRange(5,1,2,true)==2 && fixture.frameRange(6,1,2,true)==1,
          "overlay loops only within its original frame range");
    mine.tobj=53;
    Menu::Bld secondLab=mine; secondLab.x+=4; m.blds.push_back(secondLab);
    int technologyCount=0,labTech=-1;
    const auto *technologies=tech::list(technologyCount);
    for(int i=0;i<technologyCount;++i)
        if(technologies[i].side==0 && m.BldResearches(53,0,technologies[i].id)) {labTech=i;break;}
    m.players[0].research={{labTech,0,100,0}};
    check(labTech>=0 && m.BldOverlayRuns(m.blds[0]) && !m.BldOverlayRuns(m.blds[1]),
          "only the laboratory doing research plays its work overlay");
    m.StepWork(0.08f); m.players[0].research.clear(); m.StepWork(0.01f);
    check(!m.blds[0].overlayActive && m.blds[0].overlayTicks==0,
          "research completion returns laboratory animation to idle");

    m.winOpen = Menu::WIN_NONE; m.OpenWin(Menu::WIN_TRADE);
    m.rateCor = 10; m.tradeAmt = 100; m.tradeRes = 0;
    m.Me().bank.corium = 1000; m.Me().bank.gold = 1000;
    RECT buy{}, sell{}, w{};
    if (!m.TradeButtonRect(4, buy) || !m.TradeButtonRect(5, sell)
        || !m.WinRect(Menu::WIN_TRADE, w)) return 1;
    check(buy.left-w.left == 172 && sell.left-w.left == 229 && buy.top-w.top == 87,
          "trade buttons occupy original coordinates");
    m.WinClick(buy.left+2, buy.top+2);
    check(m.Me().bank.corium == 1100 && m.Me().bank.gold == 990, "buy button executes purchase");
    m.WinClick(sell.left+2, sell.top+2);
    check(m.Me().bank.corium == 1000 && m.Me().bank.gold == 1000, "sell button executes sale");
    m.WinClick(w.left+180, w.top+54);
    check(m.Me().bank.corium == 1000 && m.Me().bank.gold == 1000,
          "clicking trade information does not execute a transaction");
    m.DrawWin();
    if (FILE *out = std::fopen("building_trade.raw", "wb")) {
        std::fwrite(m.canvas.data(), 4, m.canvas.size(), out); std::fclose(out);
        std::printf("building_trade.raw %dx%d\n", SCREEN_W, SCREEN_H);
    }
    m.OpenResearchFor(0); m.OpenResearchFor(1);
    check(m.winOpen == Menu::WIN_RESEARCH && m.researchBld == 1,
          "another laboratory replaces the research window owner");
    m.OpenResearchFor(0); m.blds[0].hp = 0; m.Reap();
    check(m.winOpen == Menu::WIN_NONE && m.researchBld == -1,
          "destroyed laboratory closes its research window");
    int sequences=0,missingSequences=0;
    for(int type=50;type<=115;++type) for(int side=0;side<3;++side) {
        const char *name=units::buildingAni(type,side);
        if(!name) continue;
        ++sequences;
        if(!m.unitSet.objectTiming(name)) {
            ++missingSequences;
            std::printf("buildingcheck: missing descriptor %s\n",name);
        }
    }
    check(sequences==43 && missingSequences==0,"all 43 building overlays include late Silicon types");
    m.shots.clear();
    Menu::Bld gun; gun.tobj=62; gun.owner=0; gun.hp=1000;
    gun.turret=true; gun.x=gun.y=4; gun.z=0; gun.dir=3;
    gun.tgt=3;
    bool started=m.BeginBldFire(gun,8,4,1);
    gun.tgt=7; // a new command must not retarget the already committed salvo
    check(started && gun.fireActive && m.shots.empty(),"tower firing state starts without an early shot");
    m.StepBldFire(gun,0.12f);
    check(m.shots.empty(),"tower waits through the frame preceding emission");
    m.StepBldFire(gun,0.04f);
    check(m.shots.size()==2 && gun.fireEmitted && m.shots[0].tgt==3
          && std::abs((m.shots[0].tx+m.shots[1].tx)*0.5f-8)<0.001f && m.shots[0].tz==1,
          "tower emits one salvo at frame two and preserves the committed target");
    m.StepBldFire(gun,10);
    check(!gun.fireActive && m.shots.size()==2,"firing animation returns to idle without a second salvo");
    m.BeginBldFire(gun,8,4,1); gun.buildLeft=1; m.StepBldFire(gun,1);
    check(!gun.fireActive && m.shots.size()==2,"busy tower cancels an uncommitted firing animation");
    // Original art at successive firing times, for visual verification.
    gun.buildLeft=0; gun.fireActive=true;
    if(const auto *fire=m.BldFireTiming(gun)) {
        if(const auto *strip=m.unitSet.object(fire->sprite)) {
            std::fill(m.canvas.begin(),m.canvas.end(),0xff102028u);
            for(int stage=0;stage<5;++stage) {
                gun.fireTicks=double(stage*2);
                int frame=m.BldFireFrame(gun,*fire);
                const auto *art=strip->at(frame);
                if(art) m.BlitFrame(*art,80+stage*155,250,180,Menu::WORLD_DEN);
                m.TextAt(m.font,25+stage*155,360,"frame "+std::to_string(frame));
            }
            if(FILE *out=std::fopen("building_fire.raw","wb")) {
                std::fwrite(m.canvas.data(),4,m.canvas.size(),out); std::fclose(out);
                std::printf("building_fire.raw %dx%d\n",SCREEN_W,SCREEN_H);
            }
        }
    }
    m.blds.clear(); m.units.clear(); m.prod.clear(); m.shots.clear();
    m.players[0].bank.oxyNeed=0;
    Menu::Bld yard; yard.tobj=50; yard.hp=yard.hpMax=1000; yard.x=yard.y=4;
    m.blds.push_back(yard); yard.x=10; m.blds.push_back(yard);
    m.prod.push_back({1,10,10,0}); m.prod.push_back({2,10,10,0}); m.prod.push_back({3,10,10,1});
    m.StepProduction(2);
    check(m.prod[0].left==8 && m.prod[1].left==10 && m.prod[2].left==8,
          "each dock advances its own queue head, not all queued boats");
    m.blds[1].moveT=1; m.StepProduction(1);
    check(m.prod[2].left==8,"relocating dock pauses production");
    m.blds[1].moveT=0;
    m.StepProduction(8);
    check(m.prod.size()==1 && m.prod[0].type==2 && m.prod[0].left==10
          && m.blds[0].dockJob==1 && m.blds[1].dockJob==3,
          "completed queue heads enter launch cycle without advancing their successors");
    m.StepProduction(5);
    check(m.prod.size()==1 && m.prod[0].left==10,"occupied launch hatch holds the next order");
    m.prod.clear(); m.blds[0].dockPhase=m.blds[1].dockPhase=0;
    m.blds[0].dockJob=m.blds[1].dockJob=-1;
    m.prod.push_back({2,9,10,0}); m.prod.push_back({3,6,10,1});
    Menu::Unit transport; transport.type=8; transport.hp=100;
    transport.haulTo=1; transport.dockTo=1; transport.haulPhase=1;
    m.units.push_back(transport);
    transport.haulTo=0; transport.dockTo=0; transport.cargo.metal=15;
    m.units.push_back(transport);
    m.blds[0].hp=0; m.Reap();
    check(m.blds.size()==1 && m.prod.size()==1 && m.prod[0].bld==0
          && m.prod[0].type==3 && m.prod[0].left==6,
          "destroyed dock loses its queue while surviving orders retain their building");
    check(m.units[0].haulTo==0 && m.units[0].dockTo==0,
          "building reindexing preserves transport and repair destinations");
    check(m.units[1].haulTo==-1 && m.units[1].dockTo==-1 && m.units[1].cargo.metal==15
          && m.units[1].haulPhase==2,
          "destroyed transfer destination releases the hauler without losing cargo");
    m.blds[0].hp=0; m.blds[0].dockPhase=2; m.blds[0].dockJob=1;
    size_t boats=m.units.size(); m.StepDocks(1);
    check(m.units.size()==boats,"destroyed dock cannot launch a finished boat before reaping");
    m.blds.clear(); m.units.clear(); m.prod.clear();
    Menu::Bld lab; lab.tobj=53; lab.hp=lab.hpMax=1000; lab.x=lab.y=4;
    m.blds.push_back(lab); lab.x=8; m.blds.push_back(lab);
    auto &researcher=m.players[0];
    researcher.research.clear();
    researcher.techDone.assign(size_t(technologyCount),0); researcher.bank.gold=100000;
    int available=-1;
    for(int i=0;i<technologyCount;++i)
        if(technologies[i].side==0 && technologies[i].level<=m.SetupTechCap()
            && m.TechReadyFor(0,i)) {available=i;break;}
    m.OpenResearchFor(1);
    bool researching=available>=0 && m.StartResearch(available);
    check(researching && researcher.research.front().lab==1 && !m.BldOverlayRuns(m.blds[0])
          && m.BldOverlayRuns(m.blds[1]),"research starts and animates in the chosen laboratory");
    m.OpenResearchFor(0);
    check(!m.ResearchInWindow(),"another laboratory does not display or cancel this research");
    m.OpenResearchFor(1);
    check(m.ResearchInWindow(),"working laboratory displays its own research");
    float remaining=researcher.research.front().left;
    m.blds[1].moveT=1; m.StepResearch(1);
    check(researching && researcher.research.front().left==remaining,"relocating laboratory pauses its research");
    m.blds[1].moveT=0; m.blds[0].hp=0; m.Reap();
    check(researching && researcher.research.front().lab==0 && researcher.research.front().left==remaining,
          "research keeps its laboratory and progress after building reindexing");
    m.StepResearch(0.25f);
    check(researching && researcher.research.front().left<remaining,"surviving laboratory resumes research");
    m.blds[0].hp=0; m.StepResearch(100000);
    check(researching && researcher.research.empty()
          && !m.TechHasFor(0,available),"destroyed laboratory cannot complete research before reaping");
    m.blds[0].hp=1000;
    m.OpenResearchFor(0);
    researching=m.StartResearch(available);
    m.blds[0].owner=1; m.StepResearch(100000);
    check(researching && researcher.research.empty() && !m.TechHasFor(0,available),
          "captured laboratory cannot finish its former owner's research");
    m.blds[0].owner=0;
    int gold=researcher.bank.gold;
    researching=m.StartResearch(available); m.AbortResearch();
    check(researching && researcher.bank.gold==gold && researcher.research.empty(),
          "manual cancellation refunds research and releases its laboratory");
    researching=m.StartResearch(available); m.StepResearch(100000);
    check(researching && m.TechHasFor(0,available) && researcher.research.empty(),"completed research unlocks technology and clears its job");
    for(int side=0;side<3;++side) {
        m.blds.clear(); researcher.research.clear(); researcher.civ=side;
        researcher.techDone.assign(size_t(technologyCount),0); researcher.bank.gold=100000;
        int picks[2]={-1,-1}, types[2]={-1,-1}, found=0;
        for(int i=0;i<technologyCount && found<2;++i) {
            if(technologies[i].side!=side || technologies[i].level>m.SetupTechCap()
                || !m.TechReadyFor(0,i)) continue;
            for(int type=50;type<=115;++type) if(m.BldResearches(type,side,technologies[i].id)) {
                picks[found]=i; types[found++]=type; break;
            }
        }
        check(found==2,"race has two initially available research projects");
        if(found!=2) continue;
        for(int k=0;k<2;++k) {lab.tobj=uint32_t(types[k]);lab.owner=0; m.blds.push_back(lab);}
        m.OpenResearchFor(0); bool first=m.StartResearch(picks[0]);
        int afterFirst=researcher.bank.gold;
        check(first && !m.StartResearch(picks[1]) && researcher.bank.gold==afterFirst,
              "busy laboratory rejects another project without charging");
        m.OpenResearchFor(1);
        check(!m.StartResearch(picks[0]) && researcher.bank.gold==afterFirst,
              "same technology cannot be charged or researched twice");
        bool second=m.StartResearch(picks[1]);
        check(first && second && researcher.research.size()==2
              && m.BldOverlayRuns(m.blds[0]) && m.BldOverlayRuns(m.blds[1]),
              "two laboratories research and animate concurrently");
        if(!first || !second || researcher.research.size()!=2) continue;
        float a=researcher.research[0].left,btime=researcher.research[1].left;
        m.StepResearch(1);
        check(researcher.research[0].left==a-1 && researcher.research[1].left==btime-1,
              "concurrent projects each receive a full simulation step");
        m.blds[0].moveT=1; m.StepResearch(1); m.blds[0].moveT=0;
        check(researcher.research[0].left==a-1 && researcher.research[1].left==btime-2,
              "pausing one laboratory does not pause another");
        int spentCancelled=researcher.research[0].paid;
        m.OpenResearchFor(0); m.AbortResearch();
        check(researcher.research.size()==1 && researcher.research[0].tech==picks[1]
              && researcher.research[0].left==btime-2
              && researcher.bank.gold==100000-researcher.research[0].paid-(side==2?spentCancelled:0),
              "cancel refunds only the selected laboratory and preserves other progress");
        first=m.StartResearch(picks[0]); m.blds[0].hp=0; m.Reap();
        check(first && researcher.research.size()==1 && researcher.research[0].lab==0
              && researcher.research[0].tech==picks[1],
              "destroying one active laboratory preserves and remaps the other project");
        m.StepResearch(100000);
        check(m.TechHasFor(0,picks[1]) && !m.TechHasFor(0,picks[0]) && researcher.research.empty(),
              "surviving laboratory completes only its own technology");
        m.blds.clear(); researcher.techDone.assign(size_t(technologyCount),0);
        for(int k=0;k<2;++k) {lab.tobj=uint32_t(types[k]); m.blds.push_back(lab);}
        first=m.StartResearchFor(0,picks[0],false); second=m.StartResearchFor(0,picks[1],false);
        check(first && second && researcher.research.size()==2
              && researcher.research[0].lab!=researcher.research[1].lab,
              "automatic research assigns separate available laboratories");
        m.StepResearch(100000);
        check(researcher.research.empty() && m.TechHasFor(0,picks[0]) && m.TechHasFor(0,picks[1]),
              "two projects can both complete in one simulation step");
    }
    m.blds.clear(); m.units.clear(); m.prod.clear();
    int launchX=-1,launchY=-1;
    for(int y=2;y<m.terr.bh-2 && launchX<0;++y)
        for(int x=2;x<m.terr.bw-2;++x) if(m.WaterAt(x,y,1)) {
            launchX=x*2; launchY=y*2; break;
        }
    check(launchX>=0,"launch test has an underwater cell above the dock");
    if(launchX>=0) {
        Menu::Bld dock; dock.tobj=50; dock.hp=dock.hpMax=1000; dock.span=2;
        dock.x=launchX; dock.y=launchY; dock.z=0; dock.owner=0;
        dock.dockPhase=2; dock.dockT=1; dock.dockJob=1;
        m.blds.push_back(dock); m.occStamp=-1;
        for(int y=0;y<2;++y) for(int x=0;x<2;++x) {
            Menu::Unit blocker; blocker.hp=100;blocker.x=float(launchX+x);
            blocker.y=float(launchY+y); blocker.z=1; m.units.push_back(blocker);
        }
        int built=m.players[0].stBuiltU;
        m.blds[0].dockPhase=5; m.blds[0].dockT=0;
        m.StepDocks(1);
        check(m.blds[0].dockPhase==5 && m.blds[0].dockT==0,
              "blocked dock waits before opening its roof");
        m.units.back().z=0; m.StepDocks(1);
        check(m.blds[0].dockPhase==1,"boat on another depth does not block launch clearance");
        m.units.back().z=1; m.blds[0].dockPhase=2; m.blds[0].dockT=1;
        m.StepDocks(1);
        check(m.units.size()==4 && m.blds[0].dockJob==1 && m.blds[0].dockPhase==2
              && m.players[0].stBuiltU==built,"occupied launch cells hold the completed boat without losing it");
        m.units.pop_back(); m.StepDocks(1);
        check(m.units.size()==4 && m.units.back().x==launchX+1 && m.units.back().y==launchY+1
              && m.units.back().z==1 && m.blds[0].dockJob==-1,
              "dock launches into the free cell one level above its own position");
        m.StepDocks(1);
        check(m.blds[0].dockPhase==4 && m.blds[0].dockT==1 && m.units.size()==4,
              "roof stays open while the launched boat remains in the dock");
        m.units[0].hp=0; m.Reap();
        check(m.blds[0].dockUnit==2,"launched boat reference survives unrelated boat destruction");
        float px,py,pz;m.UnitPhysicalPosition(m.units.back(),px,py,pz);
        check(std::fabs(px-launchX)<.001f && std::fabs(py-launchY)<.001f
              && std::fabs(pz+.45f)<.001f,"physical birth is inside the hall while logical launch cell is occupied");
        m.StepLaunches(.01f);m.StepDocks(1);
        check(m.blds[0].dockPhase==4,"dock cannot close during the initial turn");
        for(int k=0;k<100 && m.units.back().launchStage==1;++k) m.StepLaunches(.04f);
        m.StepLaunches(.04f);
        m.UnitPhysicalPosition(m.units.back(),px,py,pz);
        check(m.units.back().launchStep==1 && pz>-.45f && pz<1,
              "first original 25 Hz motion step raises the hull without jumping to launch depth");
        m.sel={2};m.StopUnits(false);
        check(m.units.back().launchStage==2,"stop command cannot interrupt the emergence manoeuvre");
        m.StepLaunches(20);m.StepDocks(1);
        check(m.blds[0].dockPhase==3 && m.blds[0].dockUnit==-1,
              "roof starts closing on emergence completion even inside the dock footprint");
        m.UnitPhysicalPosition(m.units.back(),px,py,pz);
        check(px==m.units.back().x && py==m.units.back().y && pz==1,
              "completed emergence joins ordinary coordinates without a position discontinuity");
        m.StepDocks(2);
        check(m.blds[0].dockPhase==0 && m.players[0].stBuiltU==built+1,
              "dock closes without emitting a duplicate boat");
        m.blds[0].z=4; m.blds[0].dockJob=1; m.blds[0].dockPhase=2;
        size_t count=m.units.size(); m.StepDocks(1);
        check(m.units.size()==count && m.blds[0].dockJob==1,
              "dock at the highest swimming level cannot launch outside the world");
    }
    Menu::Bld soundDock; soundDock.tobj=92;
    check(m.DockSound(soundDock,false)==923 && m.DockSound(soundDock,true)==924,
          "Protoplasm Generator uses original opening and closing sound IDs");
    soundDock.tobj=64;
    check(m.DockSound(soundDock,false)==596 && m.DockSound(soundDock,true)==597,
          "special dock uses its own original sound IDs");
    std::fill(m.canvas.begin(),m.canvas.end(),0xff102028u);
    for(int side=0;side<3;++side) {
        m.players[0].civ=side;
        Menu::Bld animated; animated.owner=0; animated.tobj=side==2?92:50;
        animated.hp=1000; animated.dockPhase=1;
        const auto *body=m.DockTiming(animated),*cover=m.DockTiming(animated,true);
        auto def=blddock::get(int(animated.tobj),side);
        int expected=side==0?19:(side==1?16:18);
        check(body && cover && def.last==expected && int(body->ticks.size())>expected
              && int(cover->ticks.size())>expected,
              "main dock body and cover resolve their original complete frame range");
        if(!body || !cover) continue;
        check(!m.StepDockSequence(animated,0.04f,false) && m.DockFrame(animated,*body)==0,
              "dock holds its initial frame for the descriptor's first tick");
        check(!m.StepDockSequence(animated,0.04f,false) && m.DockFrame(animated,*body)==1,
              "dock advances after the original two-tick frame duration");
        animated.dockTicks=double((expected-1)*2);
        check(m.StepDockSequence(animated,0.08f,false) && m.DockFrame(animated,*body)==expected
              && m.DockFrame(animated,*cover)==expected,
              "body and roof reach the original final opening frame together");
        animated.dockPhase=3; animated.dockTicks=0;
        check(m.DockFrame(animated,*body)==expected && !m.StepDockSequence(animated,0.08f,true)
              && m.DockFrame(animated,*body)==expected-1,
              "closing runs the original range backwards at descriptor timing");
        check(m.StepDockSequence(animated,10,true) && m.DockFrame(animated,*body)==0,
              "closing completes at the original first frame");
        animated.dockPhase=4;
        const spr::Frame *tint=nullptr;
        check(m.BldTintFrame(animated,"unused",tint) && tint && tint->ok(),
              "dock player color follows its own matching sequence descriptor");
        if(const auto *strip=m.unitSet.object(body->sprite)) {
            const auto *roof=m.unitSet.object(cover->sprite);
            for(int stage=0;stage<3;++stage) {
                animated.dockPhase=1; animated.dockTicks=stage*expected;
                int frame=m.DockFrame(animated,*body);
                if(const auto *f=strip->at(frame)) m.BlitFrame(*f,135+stage*250,80+side*165,180,Menu::WORLD_DEN);
                if(roof) if(const auto *f=roof->at(m.DockFrame(animated,*cover)))
                    m.BlitFrame(*f,135+stage*250,80+side*165,180,Menu::WORLD_DEN);
            }
        }
    }
    if(FILE *out=std::fopen("building_dock_frames.raw","wb")) {
        std::fwrite(m.canvas.data(),4,m.canvas.size(),out); std::fclose(out);
        std::printf("building_dock_frames.raw %dx%d\n",SCREEN_W,SCREEN_H);
    }
    for(int side=0;side<3;++side) {
        m.players[0].civ=side; m.players[0].techDone.assign(size_t(technologyCount),1);
        m.players[0].bank.corium=m.players[0].bank.metal=100000;
        m.units.clear();m.blds.clear();m.prod.clear();m.sel.clear();m.selBld={0};m.palOpen=true;
        Menu::Bld factory;factory.owner=0;factory.tobj=side==2?92:50;
        factory.hp=factory.hpMax=1000;factory.x=launchX;factory.y=launchY;factory.span=2;
        m.blds.push_back(factory);
        int count=0;bool isUnit=false;const int *items=m.Palette(count,isUnit);
        int special=side==0?9:21;
        check(items && isUnit && count>0 && (side==2 || std::find(items,items+count,special)==items+count),
              "normal factory palette excludes boats produced in cyber facilities");
        check(m.QueueSelectedBoat(side==0?1:(side==1?13:25)) && m.prod.size()==1,
              "normal factory accepts its race's ordinary boat");
        if(side==2) {
            m.prod.clear();m.blds[0].tobj=83;int funds=m.players[0].bank.metal;
            check(m.SelectedYard()==-1 && !m.QueueSelectedBoat(25) && m.prod.empty()
                  && m.players[0].bank.metal==funds,"Command Hub cannot produce or charge for submarines");
            m.blds.clear();
            check(m.AiWanted(0)==92,"Silicon AI first requests its actual submarine factory");
            continue;
        }
        m.prod.clear();m.blds[0].tobj=side==0?64:73;m.blds[0].span=1;
        items=m.Palette(count,isUnit);
        check(items && isUnit && count==1 && items[0]==special,
              "cyber facility offers only its own special boat");
        int commandCount=0;const auto *commands=m.CmdsFor(true,commandCount);int button=-1;
        for(int i=0;i<commandCount;++i)
            if(std::strcmp(commands[i].rec,side==0?"BUT_BLDWORM":"BUT_BLDDOLPH")==0) button=i;
        if(button>=0) m.CmdAction(true,button);
        check(button>=0 && m.prod.size()==1 && m.prod[0].type==special,
              "original cyber production button creates the correct order");
        if(m.prod.empty()) continue;
        m.StepProduction(m.prod[0].total+1);
        const auto *timing=m.DockTiming(m.blds[0]);const auto *roof=m.DockTiming(m.blds[0],true);
        auto def=blddock::get(int(m.blds[0].tobj),side);
        check(timing && roof && m.blds[0].dockPhase==6 && m.DockFrame(m.blds[0],*timing)==def.last,
              "special dock starts its reverse preparation sequence");
        m.StepDocks(10);
        check(m.blds[0].dockPhase==5 && timing && m.DockFrame(m.blds[0],*timing)==def.first,
              "special dock finishes preparation before waiting for launch space");
        m.StepDocks(0);m.StepDocks(10);m.StepDocks(0);
        check(m.units.size()==1 && m.units[0].type==uint32_t(special) && m.blds[0].dockPhase==4,
              "special dock opens forward and releases the ordered cyber boat");
        m.StepLaunches(2);m.StepLaunches(20);
        m.StepDocks(0);
        check(m.blds[0].dockPhase==0 && timing && m.DockFrame(m.blds[0],*timing)==def.last,
              "special dock returns directly to idle after departure without a false closing phase");
    }
    m.blds.clear();m.units.clear();m.prod.clear();
    m.players[1].civ=2;m.players[1].bank.corium=m.players[1].bank.metal=100000;
    Menu::Bld aiFactory;aiFactory.tobj=83;aiFactory.owner=1;aiFactory.hp=1000;
    aiFactory.x=launchX;aiFactory.y=launchY;m.blds.push_back(aiFactory);
    int aiFunds=m.players[1].bank.metal;
    m.AiEconomy(1);
    check(m.prod.empty() && m.players[1].bank.metal==aiFunds,
          "AI does not charge for capsule production in a Command Hub");
    m.blds[0].tobj=92;m.AiEconomy(1);
    check(m.prod.size()==1 && m.prod[0].bld==0 && m.prod[0].type==25
          && m.players[1].bank.metal==aiFunds,
          "AI orders its replacement capsule in the Protoplasm Generator");
    m.me=0;m.players[0].civ=2;m.players[0].techDone.assign(size_t(technologyCount),1);
    m.players[0].bank.corium=m.players[0].bank.metal=100000;
    m.blds.clear();m.units.clear();m.prod.clear();m.sel.clear();m.selBld={0};m.palOpen=false;
    int hx=-1,hy=-1,hs=m.BldFootprint(83),ms=m.BldFootprint(84);
    for(int y=2;y<m.terr.bh*2-hs-2 && hx<0;++y)
        for(int x=2;x<m.terr.bw*2-hs-2*ms-2;++x)
            if(m.CanBuildAt(x,y,83) && m.CanBuildAt(x+hs,y,84)
                && m.CanBuildAt(x+hs+ms,y,84) && m.TopAt(x/2,y/2)==m.TopAt((x+hs)/2,y/2)) {
                hx=x;hy=y;break;
            }
    check(hx>=0,"module test finds a flat adjoining building site");
    if(hx>=0) {
        Menu::Bld hub;hub.tobj=83;hub.owner=0;hub.hp=hub.hpMax=2000;
        hub.x=hx;hub.y=hy;hub.z=m.TopAt(hx/2,hy/2);hub.span=hs;m.blds.push_back(hub);
        int count=0;const auto *cmd=m.CmdsFor(true,count);int button=-1;
        for(int i=0;i<count;++i) if(std::strcmp(cmd[i].rec,"BUT_BUILDLAB")==0) button=i;
        if(button>=0) m.CmdAction(true,button);
        bool boats=false;const int *list=m.Palette(count,boats);
        check(button>=0 && m.palOpen && list && !boats && count==7 && list[0]==84 && list[6]==90,
              "Command Hub build button offers all seven research modules");
        check(m.ModuleSite(0,84,hx+hs,hy) && !m.ModuleSite(0,84,hx+hs+ms,hy),
              "module requires direct adjacency to an existing hub or module");
        m.blds[0].owner=1;
        check(!m.PlaceModule(84,hx+hs,hy),"enemy hub cannot authorize module construction");
        m.blds[0].owner=0;m.blds[0].buildLeft=1;
        check(!m.PlaceModule(84,hx+hs,hy),"unfinished hub cannot initiate modules");
        m.blds[0].buildLeft=0;m.players[0].bank.metal=0;
        check(!m.PlaceModule(84,hx+hs,hy) && m.blds.size()==1,
              "insufficient module resources create no building");
        m.players[0].bank.metal=100000;
        int consumed=m.capsulesUsed;auto price=cost::bldPrice(84,2);
        check(m.PlaceModule(84,hx+hs,hy) && m.blds.size()==2 && m.units.empty()
              && m.capsulesUsed==consumed && m.blds[1].buildLeft>0
              && m.blds[1].payCor==price.corium && m.blds[1].payMet==price.metal,
              "hub starts module construction with resource installments and without a capsule");
        check(!m.PlaceModule(84,hx+hs,hy),"module construction reserves its footprint immediately");
        for(int i=0;i<price.secs*3+20;++i) m.StepBuilding(1);
        check(m.blds[1].buildLeft<=0 && m.players[0].bank.corium==100000-price.corium
              && m.players[0].bank.metal==100000-price.metal,
              "module completes and consumes exactly its recorded construction price");
        check(m.ModuleSite(0,84,hx+hs+ms,hy),"finished module extends the available construction edge");
        m.blds[0].hp=0;
        check(!m.PlaceModule(84,hx+hs+ms,hy),"destroyed selected hub cannot authorize another module");
        int technology=-1;
        for(int i=0;i<technologyCount;++i) if(technologies[i].side==2
            && m.BldResearches(84,2,technologies[i].id)) {technology=i;break;}
        check(technology>=0 && m.ResearchBldFor(0,technology)==1,
              "completed module remains a research facility after the hub is destroyed");
    }
    m.blds.clear();m.units.clear();m.prod.clear();m.players[0].civ=2;
    m.players[0].bank.corium=m.players[0].bank.metal=0;
    Menu::Bld unpaid;unpaid.tobj=84;unpaid.owner=0;unpaid.hp=unpaid.hpBuilt=100;
    unpaid.hpMax=1000;unpaid.buildTotal=unpaid.buildLeft=10;
    unpaid.payCor=100;unpaid.payMet=100;unpaid.span=1;
    m.blds.push_back(unpaid);
    m.StepBuilding(20);
    check(m.blds[0].buildLeft==10 && m.blds[0].payAcc==0 && m.blds[0].hp==100,
          "unfunded construction preserves its timer, installments and HP even over a long step");
    m.players[0].bank.corium=10;m.players[0].bank.metal=0;m.StepBuilding(1);
    check(m.players[0].bank.corium==10 && m.blds[0].paidCor==0 && m.blds[0].buildLeft==10,
          "missing one resource does not charge the other or advance work");
    m.players[0].bank.metal=10;m.StepBuilding(1);
    check(m.blds[0].buildLeft==9 && m.blds[0].hp==190 && m.blds[0].paidCor==10
          && m.blds[0].paidMet==10,"one installment resumes work without accumulated idle-time debt");
    m.StepBuilding(20);
    check(m.blds[0].buildLeft==9 && m.blds[0].hp==190 && m.blds[0].paidMet==10,
          "second resource shortage pauses instead of making the building permanently inactive");
    m.players[0].bank.corium=m.players[0].bank.metal=90;
    for(int i=0;i<60;++i) m.StepBuilding(1);
    check(m.blds[0].buildLeft==0 && m.blds[0].hp==1000 && m.blds[0].paidMet==100
          && m.blds[0].paidCor==100 && m.players[0].bank.metal==0 && m.players[0].bank.corium==0,
          "replenished construction completes for exactly the full price");
    m.blds[0]=unpaid;m.blds[0].hp=1000;
    m.players[0].bank.corium=m.players[0].bank.metal=10;m.StepBuilding(1);
    check(m.blds[0].closeT==0 && m.blds[0].buildLeft>0 && m.blds[0].paidMet==10,
          "full HP cannot finish construction while its price remains unpaid");
    m.blds[0]=unpaid;m.blds[0].hp=0;
    m.players[0].bank.corium=m.players[0].bank.metal=100;m.StepBuilding(20);
    check(m.blds[0].hp==0 && m.blds[0].paidMet==0 && m.players[0].bank.metal==100,
          "destroyed construction cannot regenerate or spend resources before reaping");
    m.blds[0].closeT=0.1f;m.StepBuilding(1);
    check(m.blds[0].hp==0 && m.blds[0].buildLeft==10,
          "destroyed closing scaffold cannot resurrect a building");
    m.blds.clear();m.units.clear();m.prod.clear();m.sel.clear();m.selBld={0};
    m.me=0;m.players[0].civ=0;m.players[0].research.clear();
    m.players[0].bank.metal=100;m.players[0].bank.corium=123;
    Menu::Bld repair;repair.owner=0;repair.tobj=50;repair.hpMax=2000;repair.hp=1900;
    m.blds.push_back(repair);
    int repairCmds=0;const auto *repairButtons=m.CmdsFor(true,repairCmds);
    check(repairButtons && std::strcmp(repairButtons[0].rec,"BUT_SELFREP")==0
        && m.CmdEnabled(true,0),"damaged human building offers self-repair in original slot zero");
    m.CmdAction(true,0);
    repairButtons=m.CmdsFor(true,repairCmds);
    check(m.blds[0].repairing && std::strcmp(repairButtons[0].rec,"BUT_BREAK")==0
        && m.CmdEnabled(true,0),"self-repair button starts work and becomes cancel repair");
    check(m.BldBusy(m.blds[0]) && !m.BldOverlayRuns(m.blds[0]) && !m.CmdEnabled(true,1),
        "repairing building suspends normal work animation and other commands");
    m.StepSelfRepair(0.56f);
    check(m.blds[0].hp==1900 && m.players[0].bank.metal==100,
        "SubCenter repair waits the original fifteen-tick interval");
    m.StepSelfRepair(0.04f);
    check(m.blds[0].hp==1920 && m.players[0].bank.metal==96
        && m.players[0].bank.corium==123,"original repair quantum restores twenty HP for four metal");
    m.players[0].bank.metal=0;m.StepSelfRepair(10);
    check(m.blds[0].hp==1920 && m.blds[0].repairing,
        "unfunded repair waits without free health or cancellation");
    m.players[0].bank.metal=16;m.StepSelfRepair(3);
    check(m.blds[0].hp==2000 && !m.blds[0].repairing && m.players[0].bank.metal==0,
        "refunded repair finishes at full health and stops charging");
    check(!m.CmdEnabled(true,0) && !m.StartSelfRepair(m.blds[0]),
        "healthy building disables self-repair");
    m.blds[0]=repair;m.players[0].bank.metal=100;m.StartSelfRepair(m.blds[0]);
    m.StepSelfRepair(0.6f);m.CmdAction(true,0);m.StepSelfRepair(10);
    check(!m.blds[0].repairing && m.blds[0].hp==1920 && m.players[0].bank.metal==96,
        "cancel keeps already repaired HP and does not refund spent metal");
    m.blds[0]=repair;m.blds[0].hp=1999;m.StartSelfRepair(m.blds[0]);m.StepSelfRepair(0.6f);
    check(m.blds[0].hp==2000 && m.players[0].bank.metal==92,
        "final partial HP quantum is clamped but uses original full installment");
    m.blds[0]=repair;m.blds[0].owner=1;
    check(!m.StartSelfRepair(m.blds[0]) && !m.CmdEnabled(true,0),"enemy building cannot receive repair orders");
    m.blds[0]=repair;m.blds[0].buildLeft=1;
    check(!m.StartSelfRepair(m.blds[0]),"unfinished building cannot self-repair");
    m.blds[0]=repair;m.blds[0].raised=true;
    check(!m.StartSelfRepair(m.blds[0]),"raised building cannot start stationary repair");
    m.blds[0]=repair;m.blds[0].dockPhase=1;
    check(!m.StartSelfRepair(m.blds[0]),"dock launch cannot be interrupted by self-repair");
    m.blds[0]=repair;Menu::Job production;production.type=12;production.bld=0;production.left=10;
    m.prod.push_back(production);
    check(!m.StartSelfRepair(m.blds[0]),"production job and self-repair cannot occupy the building together");
    m.prod.clear();m.blds[0].tobj=53;
    Menu::ResearchJob research;research.lab=0;m.players[0].research.push_back(research);
    check(!m.StartSelfRepair(m.blds[0]),"research job must finish or be cancelled before self-repair");
    m.players[0].research.clear();m.blds[0]=repair;m.players[0].bank.metal=100;
    m.StartSelfRepair(m.blds[0]);
    for(int i=0;i<60;++i) m.StepSelfRepair(0.01f);
    check(m.blds[0].hp==1920 && m.players[0].bank.metal==96,
        "repair quantum does not depend on subdivision into smaller simulation steps");
    m.blds[0]=repair;m.StartSelfRepair(m.blds[0]);m.blds[0].hp=0;
    int metalBeforeDeath=m.players[0].bank.metal;m.StepSelfRepair(10);
    check(m.blds[0].hp==0 && !m.blds[0].repairing && m.players[0].bank.metal==metalBeforeDeath,
        "destroyed building stops repair without regeneration or spending");
    m.blds[0]=repair;m.blds[0].tobj=83;m.players[0].civ=2;
    check(!m.StartSelfRepair(m.blds[0]),"Silicon building does not use human metal repair");
    check(bldrepair::get(113,0).hp==0 && bldrepair::get(111,0).hp==700,
        "original category sets exclude Silicon Parcher despite populated human table columns");
    m.players[0].civ=0;m.blds.assign(1,repair);m.blds[0].repairing=true;
    auto repairPoints=bldrepair::points(50);
    check(repairPoints.count==8 && repairPoints.point[0].x==47 && repairPoints.point[0].y==95
        && bldrepair::points(64).count==4,"repair attachment points come from original per-building geometry");
    const auto *repairTiming=m.unitSet.objectTiming("tlo_emb6");
    check(repairTiming && repairTiming->sprite=="_tlo_emb6_" && repairTiming->ticks.size()==38,
        "repair effect resolves original type-29 timing and 38-frame sprite");
    m.StepRepairVisual(0.02f);
    check(m.blds[0].repairPoint==-1,"repair visual waits for a simulation tick instead of a rendered frame");
    m.StepRepairVisual(0.02f);
    check(m.blds[0].repairPoint>=0 && m.blds[0].repairPoint<8 && m.blds[0].repairVisualTicks==0,
        "first repair visual tick chooses one valid attachment and starts frame zero");
    auto visualStart=m.blds[0];m.StepRepairVisual(2.0f);auto coarse=m.blds[0];
    m.blds[0]=visualStart;
    for(int i=0;i<200;++i) m.StepRepairVisual(0.01f);
    check(m.blds[0].repairPoint==coarse.repairPoint && m.blds[0].rnd==coarse.rnd
        && m.blds[0].repairVisualTicks==coarse.repairVisualTicks,
        "repair visual point and animation are independent of simulation step subdivision");
    m.blds[0].repairing=false;m.StepRepairVisual(0);
    check(m.blds[0].repairPoint==-1 && m.blds[0].repairVisualTicks==0,
        "cancelled or completed repair clears its attached effect");
    m.blds[0]=visualStart;m.blds[0].hp=0;m.StepRepairVisual(1);
    check(m.blds[0].repairPoint==-1,"destroyed building cannot retain a repair spark");
    m.blds[0]=visualStart;m.blds[0].moveT=1;m.StepRepairVisual(1);
    check(m.blds[0].repairPoint==-1,"moving building cannot retain a stationary repair effect");
    // Original body canvas and per-type attachment offsets at full world scale.
    std::fill(m.canvas.begin(),m.canvas.end(),0xff102028u);
    int oldWidth=m.tileW;m.tileW=180;
    const int repairTypes[4]={50,53,61,64};
    for(int row=0;row<4;++row) {
        Menu::Bld visual=repair;visual.tobj=repairTypes[row];visual.repairing=true;
        auto points=bldrepair::points(int(visual.tobj));
        const auto *body=m.BldStrip(visual.tobj,0);
        const auto *frame=body?body->at(0):nullptr;
        if(!frame) { check(false,"repair preview body exists");continue; }
        for(int col=0;col<3;++col) {
            int x=130+col*250,y=65+row*130;
            visual.repairPoint=col%points.count;
            visual.repairVisualTicks=repairTiming?int(repairTiming->duration()/2):0;
            m.BlitFrame(*frame,x,y,180,Menu::WORLD_DEN);
            auto before=m.canvas;uint32_t seed=visual.rnd;
            check(m.DrawRepairVisual(visual,*frame,x,y)==1 && before!=m.canvas && visual.rnd==seed,
                "repair effect draws on its body without changing simulation state");
        }
    }
    m.tileW=oldWidth;
    if(FILE *out=std::fopen("building_repair_frames.raw","wb")) {
        std::fwrite(m.canvas.data(),4,m.canvas.size(),out);std::fclose(out);
        std::printf("building_repair_frames.raw %dx%d\n",SCREEN_W,SCREEN_H);
    }
    m.blds.clear();m.units.clear();m.prod.clear();m.shots.clear();m.fx.clear();
    m.players[0].research.clear();m.players[0].civ=0;m.sel.clear();m.selBld={1,2};
    Menu::Bld removable;removable.owner=0;removable.hp=removable.hpMax=1000;
    for(int type:{50,50,53,57}) {removable.tobj=type;m.blds.push_back(removable);}
    m.prod.push_back({1,5,10,0});m.prod.push_back({2,6,10,1});
    Menu::ResearchJob keptResearch;keptResearch.lab=2;keptResearch.left=42;
    m.players[0].research.push_back(keptResearch);
    Menu::Unit linked;linked.hp=100;linked.haulTo=3;linked.dockTo=1;linked.tgt=1;linked.tgtBld=true;
    m.units.push_back(linked);m.blds[0].tgt=2;m.blds[0].tgtBld=true;
    Menu::Shot projectile;projectile.tgt=2;projectile.tgtBld=true;m.shots.push_back(projectile);
    m.researchBld=2;m.lastSelBld=2;m.winOpen=Menu::WIN_RESEARCH;
    m.AddFx("removed",0,0,0,18,false,false,0,true,1);
    m.AddFx("surviving",0,0,0,18,false,false,0,true,2);
    m.AddFx("pending",0,0,0,18,false,false,0,true,4);
    int losses=m.players[0].stLostB;
    check(m.DismantleBuilding(1) && m.blds.size()==3 && m.blds[1].tobj==53,
        "dismantling compacts buildings through the shared removal path");
    check(m.prod.size()==1 && m.prod[0].bld==0 && m.prod[0].type==1,
        "dismantled factory loses its queue without transferring it to the next building");
    check(m.players[0].research.size()==1 && m.players[0].research[0].lab==1
        && m.players[0].research[0].left==42 && m.researchBld==1 && m.lastSelBld==1,
        "dismantling preserves surviving research progress and panel references");
    check(m.units[0].haulTo==2 && m.units[0].dockTo==-1 && m.units[0].tgt==-1
        && m.blds[0].tgt==1 && m.shots[0].tgt==1,
        "dismantling remaps haulers, repair destinations and combat targets");
    check(m.selBld==std::vector<int>({1}) && m.players[0].stLostB==losses,
        "voluntary dismantling preserves other selections and is not a combat loss");
    bool removedFx=false,keptFx=false,pendingFx=false;
    for(const auto &effect:m.fx) {
        if(effect.name=="removed") removedFx=true;
        if(effect.name=="surviving" && effect.tag==1) keptFx=true;
        if(effect.name=="pending" && effect.tag==3) pendingFx=true;
    }
    check(!removedFx && keptFx && pendingFx,
        "attached effects follow surviving buildings and pending embryos keep the next index");
    m.DropFx(1);
    check(std::none_of(m.fx.begin(),m.fx.end(),[](const Menu::Fx &f){return f.name=="surviving";}),
        "finishing a reindexed scaffold removes its own effect");
    m.blds[1].buildLeft=10;m.selBld={1};
    int funds=m.players[0].bank.metal;
    m.CmdAction(true,0);
    check(m.blds.size()==2 && m.players[0].bank.metal==funds
        && m.players[0].research.empty() && m.researchBld==-1 && m.winOpen==Menu::WIN_NONE,
        "construction cancellation through the real button clears jobs and window without refund");
    m.blds[0].owner=1;size_t beforeRefusal=m.blds.size();
    check(!m.DismantleBuilding(0) && !m.DismantleBuilding(-1)
        && !m.DismantleBuilding(999) && m.blds.size()==beforeRefusal,
        "dismantle entry point rejects enemy and invalid building references");
    check(Menu::CanCapture(6) && Menu::CanCapture(18) && Menu::CanCapture(34)
        && !Menu::CanCapture(12) && !Menu::CanCapture(24) && !Menu::CanCapture(25),
        "capture capability follows original unit command tables rather than builders");
    m.blds.clear();m.units.clear();m.prod.clear();m.me=0;
    for(auto &p:m.players) p.research.clear();
    m.players[0].civ=0;m.players[1].civ=1;
    Menu::Bld captured;captured.owner=1;captured.tobj=50;captured.hp=captured.hpMax=1000;
    captured.repairing=true;captured.repairPoint=2;captured.dockJob=13;captured.dockPhase=1;
    captured.tgt=3;captured.fireActive=true;
    m.blds={captured,captured};m.prod={{13,9,10,0},{14,7,10,1}};
    Menu::ResearchJob captureJob;captureJob.lab=0;captureJob.left=18;
    m.players[1].research.push_back(captureJob);captureJob.lab=1;
    m.players[1].research.push_back(captureJob);
    m.players[0].bank.metal=123;m.players[1].bank.metal=456;
    m.players[0].techDone={1,0,1};m.players[1].techDone={0,1,1};
    m.players[0].stShots=78;m.players[1].stLostB=9;
    m.researchBld=0;m.winOpen=Menu::WIN_RESEARCH;
    check(m.TransferBuilding(0,0) && m.blds[0].owner==0 && m.blds.size()==2,
        "capture changes one controller without rebuilding or compacting the world");
    check(m.players[0].bank.metal==123 && m.players[1].bank.metal==456
        && m.players[0].techDone==std::vector<uint8_t>({1,0,1})
        && m.players[1].techDone==std::vector<uint8_t>({0,1,1})
        && m.players[0].stShots==78 && m.players[1].stLostB==9 && m.me==0,
        "capture preserves both players' resources, technologies, statistics and local player");
    check(m.SideOfBld(m.blds[0])==1 && m.players[0].civ==0,
        "captured Black Octopi building retains its body and weapon identity under White Sharks");
    check(m.prod.size()==1 && m.prod[0].bld==1 && m.prod[0].left==7
        && m.players[1].research.size()==1 && m.players[1].research[0].lab==1
        && m.players[1].research[0].left==18,
        "capture cancels only the captured building's jobs, preserving other progress");
    check(!m.blds[0].repairing && m.blds[0].repairPoint==-1 && m.blds[0].dockJob==13
        && m.blds[0].dockPhase==1 && !m.blds[0].fireActive && m.blds[0].tgt==-1,
        "capture clears repair and firing while preserving a committed dock launch");
    check(m.researchBld==-1 && m.winOpen==Menu::WIN_NONE,
        "capture closes a research window bound to the previous controller");
    m.blds[0].dockUnit=0;m.blds[0].dockPhase=4;m.selBld={0};m.lastSelBld=0;
    check(m.TransferBuilding(0,1) && m.SideOfBld(m.blds[0])==1
        && m.blds[0].dockPhase==4 && m.blds[0].dockUnit==0 && m.selBld.empty() && m.lastSelBld==-1,
        "recapture retains original race and lets an already launched boat leave the open dock");
    m.blds[1].hp=0;
    check(!m.TransferBuilding(0,1) && !m.TransferBuilding(1,0)
        && !m.TransferBuilding(-1,0) && !m.TransferBuilding(0,8),
        "ownership transfer rejects unchanged owner, dead building and invalid references");
    m.blds.clear();m.units.clear();m.prod.clear();
    for(auto &p:m.players) {p.research.clear();p.techDone.clear();}
    m.me=0;m.players[0].civ=0;m.players[1].civ=1;m.ally[1]=false;
    Menu::Bld target;target.owner=1;target.tobj=50;target.hp=target.hpMax=1000;
    target.x=10;target.y=10;target.z=1;target.span=2;
    Menu::Unit boarder;boarder.type=6;boarder.owner=0;boarder.hp=1000;
    boarder.x=10;boarder.y=10;boarder.z=2;
    m.blds={target};m.units={boarder};
    check(capture::ticks[0]==125 && capture::ticks[1]==75 && capture::damage(50,0)==200,
        "capture duration and attempt damage match original executable tables");
    int captureWins=0;
    for(uint32_t seed=0;seed<262144;++seed) {uint32_t copy=seed;if(capture::roll(copy)) ++captureWins;}
    check(captureWins==196608,"original capture random branch succeeds for exactly three of four outcomes");
    uint32_t good=0,captureBad=0;
    for(uint32_t seed=0;seed<100;++seed) {uint32_t copy=seed;if(capture::roll(copy)) good=seed;else captureBad=seed;}
    m.blds[0].rnd=good;
    m.units[0].patrol=true;m.units[0].guard=true;m.units[0].wantZ=0;m.units[0].dodge=1;
    check(m.BeginCapture(0,0) && m.blds[0].owner==1 && !m.BeginCapture(0,0),
        "boarding starts one exclusive attempt without instant ownership change");
    check(!m.units[0].patrol && !m.units[0].guard && m.units[0].wantZ==2 && m.units[0].dodge==0,
        "boarding replaces standing orders and holds the required depth");
    m.StepCapture(4.96f);
    check(m.blds[0].owner==1 && m.units[0].hp==1000,"boarding waits until the original 125th tick");
    m.StepCapture(0.04f);
    check(m.blds[0].owner==0 && m.units[0].hp==800 && m.blds[0].captureUnit==-1,
        "successful attempt transfers building and damages the boarding boat exactly once");
    m.StepCapture(10);
    check(m.units[0].hp==800,"completed attempt cannot repeat damage on subsequent ticks");
    m.blds[0]=target;m.blds[0].rnd=captureBad;m.units[0]=boarder;
    m.BeginCapture(0,0);m.StepCapture(5);
    check(m.blds[0].owner==1 && m.units[0].hp==800 && m.blds[0].captureUnit==-1,
        "failed random attempt preserves controller but still damages the boat");
    int upgrade=tech::indexOf(152,1);
    m.players[0].techDone.resize(size_t(upgrade+1));m.players[0].techDone[size_t(upgrade)]=1;
    m.blds[0]=target;m.blds[0].rnd=good;m.units[0]=boarder;
    m.BeginCapture(0,0);m.StepCapture(2.96f);
    check(m.blds[0].owner==1,"capture upgrade does not complete before 75 ticks");
    m.StepCapture(0.04f);
    check(m.blds[0].owner==0,"capture upgrade uses 75 ticks from executable, not guide percentage");
    m.blds[0]=target;m.units[0]=boarder;m.units[0].z=1;
    check(!m.BeginCapture(0,0),"boarding at the building's level is rejected");
    m.units[0].z=2;m.units[0].x=14;
    check(!m.BeginCapture(0,0),"boarding outside the footprint is rejected even inside former six-cell radius");
    m.units[0]=boarder;m.BeginCapture(0,0);m.StepCapture(1);m.units[0].moving=true;m.StepCapture(4);
    check(m.blds[0].captureUnit==-1 && m.blds[0].owner==1 && m.units[0].hp==1000,
        "moving away cancels boarding without rolling or damaging the boat");
    m.units[0]=boarder;m.BeginCapture(0,0);m.sel={0};m.StopUnits(false);
    check(m.blds[0].captureUnit==-1,"STOP cancels an active boarding attempt immediately");
    m.units.insert(m.units.begin(),boarder);m.units[0].hp=0;m.BeginCapture(0,1);m.StepCapture(1);
    m.Reap();
    check(m.blds[0].captureUnit==0 && m.blds[0].captureTicks==25,
        "boarding reference and progress survive removal of an unrelated boat");
    m.units[0].hp=0;m.Reap();
    check(m.blds[0].captureUnit==-1 && m.blds[0].captureTicks==0,
        "death of the boarding boat clears the building's pending attempt");
    check(capture::compatible(50,0,1) && !capture::compatible(50,0,2)
        && capture::compatible(57,0,2) && capture::compatible(94,2,0),
        "original capture matrix distinguishes factories from cross-race extractors");
    m.blds.clear();m.units.clear();m.prod.clear();
    for(auto &p:m.players) p.research.clear();
    m.me=0;m.players[0].civ=2;m.players[1].civ=0;
    Menu::Bld foreign;foreign.tobj=50;foreign.owner=1;foreign.hp=500;foreign.hpMax=1000;
    m.blds={foreign};m.TransferBuilding(0,0);m.selBld={0};
    check(m.blds[0].originalOwner==1 && m.SideOfBld(m.blds[0])==0
        && !m.BldCompatible(m.blds[0]) && !m.BldControlCompatible(m.blds[0]),
        "captured human factory retains original owner and refuses Silicon control");
    int cmdCount=0;m.CmdsFor(true,cmdCount);bool anyEnabled=false;
    for(int i=0;i<cmdCount;++i) anyEnabled|=m.CmdEnabled(true,i);
    check(cmdCount>0 && !anyEnabled && !m.StartSelfRepair(m.blds[0]),
        "incompatible captured panel remains visible but cannot start human repair or commands");
    check(m.SelectedYard()==-1 && !m.QueueSelectedBoat(25) && m.prod.empty(),
        "incompatible factory cannot manufacture through palette or direct queue entry");
    m.blds[0].tobj=57;
    check(m.BldCompatible(m.blds[0]) && !m.BldControlCompatible(m.blds[0]),
        "extractor's autonomous compatibility does not grant cross-category panel control");
    m.blds[0].tobj=53;m.winOpen=Menu::WIN_NONE;m.researchBld=-1;
    m.OpenResearchFor(0);
    check(m.winOpen==Menu::WIN_NONE && m.researchBld==-1 && !m.HasResearchBld(0),
        "incompatible captured laboratory cannot open research through its direct entry point");
    m.TransferBuilding(0,1);m.me=1;m.selBld={0};
    check(m.blds[0].originalOwner==1 && m.BldCompatible(m.blds[0])
        && m.BldControlCompatible(m.blds[0]) && m.CanSelfRepair(m.blds[0]),
        "return to original owner restores control and repair without changing original identity");
    m.players[0].civ=1;m.TransferBuilding(0,0);m.me=0;
    check(m.blds[0].originalOwner==1 && m.BldCompatible(m.blds[0])
        && m.BldControlCompatible(m.blds[0]) && m.HasResearchBld(0),
        "human-to-human captured laboratory retains supported research capability");
    m.blds[0].tobj=50;m.selBld={0};
    check(m.SelectedYard()==0,"human-to-human captured main factory stays available");
    check(!capture::compatible(49,0,0) && !capture::compatible(116,0,0)
        && !capture::compatible(50,-1,0) && !capture::compatible(50,0,3),
        "capture matrix rejects invalid object and civilization indices");
    for(int side=0;side<3;++side) {
        m.blds.clear();m.prod.clear();m.units.clear();m.me=0;m.researchBld=-1;
        auto &payer=m.players[0];payer.civ=side;payer.research.clear();
        payer.techDone.assign(size_t(technologyCount),0);payer.bank.gold=0;
        int pick=-1,labType=-1;
        for(int i=0;i<technologyCount && pick<0;++i)
            if(technologies[i].side==side && technologies[i].gold>0
                && technologies[i].level<=m.SetupTechCap() && m.TechReadyFor(0,i))
                for(int type=50;type<=115;++type) if(m.BldResearches(type,side,technologies[i].id)) {
                    pick=i;labType=type;break;
                }
        if(pick<0) {check(false,"installment test finds a research project");continue;}
        Menu::Bld paymentLab;paymentLab.owner=0;paymentLab.tobj=labType;paymentLab.hp=paymentLab.hpMax=1000;
        m.blds={paymentLab};m.OpenResearchFor(0);
        int cost=technologies[pick].gold,part=(cost+99)/100;
        float interval=float(std::max(1,technologies[pick].ticks/100))/25.0f;
        check(m.StartResearch(pick) && payer.bank.gold==0 && payer.research[0].paid==0,
            "research may start without full funds and does not prepay");
        m.StepResearch(1000);
        check(payer.research.size()==1 && payer.research[0].steps==0
            && payer.research[0].paid==0 && !m.TechHasFor(0,pick),
            "starved research does not advance or complete for free");
        payer.bank.gold=part;m.StepResearch(interval);
        check(payer.research[0].steps==1 && payer.research[0].paid==part && payer.bank.gold==0,
            "one funded research interval pays one rounded installment without idle debt");
        m.AbortResearch();
        check(payer.research.empty() && payer.bank.gold==(side==2?0:part),
            "manual research cancellation refunds paid human gold but not Silicon energy");
        payer.bank.gold=cost;m.StartResearch(pick);m.StepResearch(100000);
        check(payer.bank.gold==0 && payer.research.empty() && m.TechHasFor(0,pick),
            "completed research caps total payments at the exact original cost");
        payer.techDone[size_t(pick)]=0;payer.bank.gold=cost;m.StartResearch(pick);m.StepResearch(interval);
        int beforeCapture=payer.bank.gold;
        m.players[1].civ=side==2?0:1;m.TransferBuilding(0,1);
        int expectedRefund=side==2?0:cost/100;
        check(payer.research.empty() && payer.bank.gold==beforeCapture+expectedRefund,
            "captured lab refunds human progress percentage and does not refund Silicon energy");
    }
    check(production::seconds(1)==20 && production::seconds(12)==28
        && production::seconds(25)==4,"boat production clock uses original integer tick periods");
    for(int side=0;side<3;++side) {
        m.me=0;m.players[0].civ=side;m.players[0].techDone.assign(size_t(technologyCount),1);
        m.players[0].bank.corium=m.players[0].bank.metal=0;m.players[0].bank.oxyNeed=0;
        m.blds.clear();m.units.clear();m.prod.clear();m.sel.clear();m.selBld={0};
        Menu::Bld payDock;payDock.owner=0;payDock.tobj=side==2?92:50;payDock.hp=payDock.hpMax=1000;
        m.blds={payDock};int type=side==0?1:(side==1?13:25);auto price=cost::unitPrice(type);
        check(m.QueueSelectedBoat(type) && m.QueueSelectedBoat(type)
            && m.players[0].bank.metal==0,"boat queue accepts orders without taking full cost upfront");
        m.StepProduction(1000);
        check(m.prod.size()==2 && m.prod[0].steps==0 && m.prod[1].steps==0
            && m.blds[0].dockPhase==0,"unfunded dock neither advances nor launches; queued jobs stay unpaid");
        int cor=(price.corium+99)/100,met=(price.metal+99)/100;
        m.players[0].bank.corium=cor;m.players[0].bank.metal=met;
        float interval=m.prod[0].total/100;
        m.StepProduction(interval);
        check(m.prod[0].steps==1 && m.prod[0].paidCor==cor && m.prod[0].paidMet==met
            && m.prod[1].paidCor==0 && m.prod[1].paidMet==0,
            "only active boat pays one installment after starvation, without backlog");
        check(m.CancelSelectedBoat(type) && m.prod.size()==1 && m.prod[0].steps==1
            && m.players[0].bank.metal==0,"cancel removes last waiting duplicate without refunding unspent money");
        check(m.CancelSelectedBoat(type) && m.prod.empty() && m.players[0].bank.corium==cor
            && m.players[0].bank.metal==met,"cancelling active production returns exactly paid material installments");
        m.players[0].bank.corium=price.corium;m.players[0].bank.metal=price.metal;
        m.QueueSelectedBoat(type);m.StepProduction(10000);
        check(m.prod.empty() && m.blds[0].dockJob==type && m.blds[0].dockPhase==5
            && m.players[0].bank.corium==0 && m.players[0].bank.metal==0,
            "fully funded production charges exact cost and hands one boat to launch state");
        check(m.CancelSelectedBoat(type) && m.blds[0].dockJob==-1 && m.blds[0].dockPhase==0
            && m.players[0].bank.corium==price.corium && m.players[0].bank.metal==price.metal,
            "ordinary dock waiting for free space cancels and refunds its fully paid boat");
    }
    m.blds.clear();m.units.clear();m.prod.clear();m.me=0;m.selBld={0};
    m.players[0].civ=0;m.players[1].civ=1;
    m.players[0].bank.corium=m.players[0].bank.metal=0;
    m.players[1].bank.corium=m.players[1].bank.metal=0;
    Menu::Bld waitingDock;waitingDock.tobj=50;waitingDock.owner=0;
    waitingDock.hp=waitingDock.hpMax=1000;waitingDock.dockJob=1;
    waitingDock.dockPaidCor=40;waitingDock.dockPaidMet=200;waitingDock.dockPhase=5;
    m.blds={waitingDock};
    check(m.TransferBuilding(0,1) && m.blds[0].dockPhase==0 && m.blds[0].dockJob==-1
        && m.players[0].bank.corium==40 && m.players[0].bank.metal==200
        && m.players[1].bank.corium==0 && m.players[1].bank.metal==0,
        "capture cancels uncommitted launch and refunds its old payer rather than captor");
    m.CancelDockLaunch(m.blds[0]);
    check(m.players[0].bank.metal==200 && m.players[1].bank.metal==0,
        "cancelled dock payment cannot be refunded twice");
    for(int phase:{1,2,3,4,6}) {
        m.blds[0]=waitingDock;m.blds[0].dockPhase=phase;m.me=0;m.selBld={0};
        check(!m.CancelSelectedBoat(1) && m.blds[0].dockPhase==phase && m.blds[0].dockJob==1,
            "queue command cannot cancel opening, spawning, departing or special-preparation launch");
    }
    for(int type:{64,73}) {
        m.blds[0]=waitingDock;m.blds[0].tobj=type;
        check(!Menu::CanCancelDockLaunch(m.blds[0]),
            "special dock cannot cancel even while waiting for launch space");
    }
    m.blds[0]=waitingDock;m.blds[0].dockPhase=1;m.blds[0].dockTicks=7;
    m.TransferBuilding(0,1);
    check(m.blds[0].dockJob==1 && m.blds[0].dockPhase==1 && m.blds[0].dockTicks==7
        && m.blds[0].dockPaidMet==200 && m.players[0].bank.metal==200,
        "capture preserves committed launch animation and paid work without a refund");
    if(launchX>=0) {
        m.blds[0].x=launchX;m.blds[0].y=launchY;m.blds[0].z=0;m.blds[0].span=2;
        m.blds[0].dockPhase=2;m.occStamp=-1;
        m.StepDocks(0);
        check(m.units.size()==1 && m.units[0].owner==1 && m.units[0].type==1
            && m.blds[0].dockJob==-1 && m.blds[0].dockPaidMet==0,
            "committed launch after capture creates the preserved boat type for the current controller");
        m.StepDocks(0);
        check(m.units.size()==1,"repeated dock tick cannot duplicate the captured launch");
    }
    if(launchX>=0) {
        m.blds.clear();m.units.clear();m.prod.clear();m.players[0].civ=0;m.me=0;
        Menu::Bld reserved;reserved.tobj=50;reserved.owner=0;reserved.hp=reserved.hpMax=1000;
        reserved.x=launchX;reserved.y=launchY;reserved.z=0;reserved.span=2;
        reserved.dockJob=1;reserved.dockPhase=5;m.blds={reserved,reserved};m.occStamp=-1;
        m.StepDocks(0);
        check(m.blds[0].dockPhase==1 && m.blds[1].dockPhase==1
            && (m.blds[0].dockX!=m.blds[1].dockX || m.blds[0].dockY!=m.blds[1].dockY),
            "concurrent docks reserve different exact launch cells before opening");
        int rx=m.blds[0].dockX,ry=m.blds[0].dockY,rz=m.blds[0].dockZ;
        check(m.DockReserved(rx,ry,rz) && !m.DockReserved(rx,ry,rz+1),
            "dock reservation occupies one cell at its launch depth only");
        Menu::Unit forced;forced.x=float(rx);forced.y=float(ry);forced.z=float(rz);forced.hp=100;
        m.units={forced};m.blds[0].dockPhase=2;
        int pickX=0,pickY=0,pickZ=0;
        check(!m.DockLaunchCell(m.blds[0],pickX,pickY,pickZ),
            "reserved dock does not silently switch launch cell if an external object occupies it");
        m.StepDocks(0);
        check(m.units.size()==1 && m.blds[0].dockJob==1 && m.blds[0].dockX==rx,
            "occupied reserved cell waits without spawning inside another boat");
        m.units.clear();m.blds[1].hp=0;m.Reap();
        check(m.blds.size()==1 && m.DockReserved(rx,ry,rz),
            "reservation stays with its building across unrelated object removal");
        m.TransferBuilding(0,1);
        check(m.DockReserved(rx,ry,rz) && m.blds[0].dockX==rx,
            "committed reservation survives factory capture");
        m.StepDocks(0);
        check(m.units.size()==1 && m.units[0].x==float(rx) && m.units[0].y==float(ry)
            && !m.DockReserved(rx,ry,rz) && m.blds[0].dockX==-1,
            "spawn uses the reserved cell and immediately releases the reservation");
        m.blds={reserved};m.units.clear();m.StepDocks(0);
        rx=m.blds[0].dockX;ry=m.blds[0].dockY;rz=m.blds[0].dockZ;m.blds[0].hp=0;
        check(!m.DockReserved(rx,ry,rz),"dead dock cannot leave a ghost launch reservation before reaping");
        m.Reap();check(m.blds.empty() && !m.DockReserved(rx,ry,rz),
            "removing the dock frees its reservation without a stale global grid entry");
    }
    if(launchX>=0) {
        Menu::Bld yard;yard.tobj=50;yard.x=launchX;yard.y=launchY;
        yard.z=0;yard.span=2;yard.hp=yard.hpMax=1000;yard.dockJob=1;yard.dockPhase=2;
        m.units.clear();m.blds={yard,yard};m.blds[0].hp=0;m.occStamp=-1;
        m.blds[1].dockX=launchX;m.blds[1].dockY=launchY;m.blds[1].dockZ=1;
        m.StepDocks(0);
        check(m.units.size()==1 && m.units[0].launchBld==1,"launch tracks its own producing building");
        m.Reap();
        check(m.units[0].launchBld==0 && m.blds[0].dockUnit==0,
              "removing an earlier building remaps both launch references");
        m.sel={0};m.OrderMove(640,300);
        check(m.units[0].launchStage==1 && m.units[0].launchStep==0,
              "ordinary move order does not replace launch state");
        m.StepLaunches(.04f);m.StepLaunches(.8f);
        int sx,sy;m.UnitToScreen(m.units[0],sx,sy);
        m.camX+=sx-640;m.camY+=sy-400;m.fogOn=false;
        m.ComposeTerrain();
        int hull=-1,cover=-1;
        for(size_t i=0;i<m.drawList.size();++i) {
            const auto &item=m.drawList[i];
            if(item.kind==Menu::DR_UNIT && item.idx==0) hull=int(i);
            if(item.kind==Menu::DR_COVER && item.idx==0) cover=int(i);
        }
        check(hull>=0 && cover>hull,"emerging hull draws before its own dock cover");
        FILE *launchShot=std::fopen("building_launch.raw","wb");
        if(launchShot){std::fwrite(m.canvas.data(),4,m.canvas.size(),launchShot);std::fclose(launchShot);}
        m.blds[0].hp=0;m.Reap();
        check(m.units.size()==1 && m.units[0].launchBld==-1 && m.units[0].launchStage==2,
              "destroyed dock removes the draw link without teleporting the surviving hull");
        m.StepLaunches(20);
        check(m.units[0].launchStage==0 && m.units[0].z==1,
              "surviving hull finishes emergence after its dock is destroyed");
    }
    // Rebuild a flat arena so late obstacles are independent of mission geometry.
    m.terr.bw=m.terr.bh=8;m.terr.cells.clear();m.terr.objects.clear();m.terr.decor.clear();
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        maps::Cell c;c.x=uint8_t(x*2);c.y=uint8_t(y*2);c.mesh=4352;c.texA=1;
        m.terr.cells.push_back(c);
    }
    m.EdTouched();m.units.clear();m.blds.clear();
    Menu::Bld departureYard;departureYard.tobj=50;departureYard.x=4;departureYard.y=4;
    departureYard.z=0;departureYard.span=2;departureYard.hp=departureYard.hpMax=1000;
    departureYard.dockJob=1;departureYard.dockPhase=2;departureYard.rallyX=12;departureYard.rallyY=4;
    m.blds={departureYard};m.occStamp=-1;m.StepDocks(0);
    check(m.units.size()==1 && m.units[0].path.empty() && !m.units[0].moving,
          "newly created boat does not precompute a departure route inside the dock");
    Menu::Bld lateObstacle=departureYard;lateObstacle.tobj=57;lateObstacle.x=8;
    lateObstacle.dockJob=-1;lateObstacle.dockPhase=0;
    m.blds.push_back(lateObstacle);m.occStamp=-1;
    m.blds[0].rallyX=4;m.blds[0].rallyY=12;
    m.StepLaunches(2);m.StepLaunches(20);
    bool detour=m.units[0].moving;
    for(const auto &p:m.units[0].path) if(p.x==4 && p.y==2) detour=false;
    check(detour && m.units[0].tx==12 && m.units[0].ty==4,
          "departure plans around a newly built obstacle using the creation-time rally point");
    m.units.clear();m.blds={departureYard};m.occStamp=-1;m.StepDocks(0);
    lateObstacle.x=12;m.blds.push_back(lateObstacle);m.occStamp=-1;
    m.StepLaunches(2);m.StepLaunches(20);
    check(m.units[0].moving && (m.units[0].tx!=12 || m.units[0].ty!=4),
          "occupied rally point falls back to a reachable exit after emergence");
    {
        Menu::Bld silicon;silicon.tobj=83;silicon.owner=0;silicon.x=silicon.y=8;
        silicon.hp=500;silicon.hpMax=1000;silicon.energyPercent=100;
        m.players[0].civ=2;m.players[0].research.clear();m.me=0;m.aiOn=false;
        std::fill(m.techDoneOf(0).begin(),m.techDoneOf(0).end(),0);
        m.blds={silicon};m.units.clear();m.prod.clear();m.shots.clear();m.sel.clear();m.selBld={0};
        m.BankOf(0).gold=1234;m.regenTick=0;m.regenClock=0;
        check(energy::stored(84)==200 && energy::stored(115)==900 && energy::stored(111)==0,
              "building energy capacities include hub modules and exclude the unused type from EXE");
        m.StepSiRegen(1.04f);
        check(m.blds[0].hp==500 && m.blds[0].energyPercent==100,
              "building regeneration observes its strict 25-tick deadline");
        m.StepSiRegen(.04f);
        check(m.blds[0].hp==510 && m.blds[0].energyPercent==99 && m.BankOf(0).gold==1234,
              "building automatically regenerates from its own energy without charging the bank");
        m.techDoneOf(0)[size_t(tech::indexOf(78,3))]=1;
        m.blds={silicon};m.regenTick=26;m.regenClock=0;m.StepSiRegen(.04f);
        check(m.blds[0].hp==525 && m.blds[0].energyPercent==99,
              "building conservation level three restores 25 HP rather than boat quantum 20");
        m.techDoneOf(0)[size_t(tech::indexOf(102,1))]=1;
        m.blds={silicon};m.regenTick=24;m.regenClock=0;m.StepSiRegen(.04f);
        check(m.blds[0].hp==525 && m.blds[0].regenLastTick==24,
              "building speed research uses original fixed-point rate 66");
        m.blds={silicon};m.blds[0].energyPercent=0;m.regenTick=10000;m.regenClock=0;
        m.StepSiRegen(1);m.StepBuilding(1);
        check(m.blds[0].hp==500 && m.BankOf(0).gold==1234,
              "empty building storage stops healing despite available bank energy");
        m.BankOf(0).gold=7;
        check(m.RechargeBuilding(m.blds[0]) && m.blds[0].energyPercent==0 && m.BankOf(0).gold==0,
              "small building recharge spends the available bank using original percentage truncation");
        m.BankOf(0).gold=1000;
        int rechargeButton=-1,commandCount=0;const auto *commands=m.CmdsFor(true,commandCount);
        for(int i=0;i<commandCount;++i)
            if(std::strcmp(commands[i].rec,"BUT_REPLINISH")==0) rechargeButton=i;
        check(rechargeButton>=0 && m.CmdEnabled(true,rechargeButton),
              "Silicon building exposes its own enabled replenish button");
        if(rechargeButton>=0) m.CmdAction(true,rechargeButton);
        check(m.blds[0].energyPercent==100 && m.BankOf(0).gold==200 && m.blds[0].hp==500,
              "building button replenishes its storage without healing immediately or ordering boats");
        check(rechargeButton>=0 && !m.CmdEnabled(true,rechargeButton),
              "full building energy disables redundant replenish action");
        m.blds[0].energyPercent=0;m.blds[0].buildLeft=1;m.BankOf(0).gold=1000;
        check(!m.RechargeBuilding(m.blds[0]) && m.BankOf(0).gold==1000,
              "unfinished building cannot consume recharge energy");
        m.blds={silicon};m.blds[0].hp=0;m.blds[0].energyPercent=0;
        m.StepSiRegen(1);
        check(!m.RechargeBuilding(m.blds[0]) && m.blds[0].hp==0 && m.BankOf(0).gold==1000,
              "destroyed building cannot regenerate or recharge before removal");
        m.blds={silicon};m.regenTick=0;m.regenClock=0;m.StepSiRegen(12);
        auto whole=m.blds[0];m.blds={silicon};m.regenTick=0;m.regenClock=0;
        for(int i=0;i<300;++i) m.StepSiRegen(.04f);
        check(m.blds[0].hp==whole.hp && m.blds[0].energyPercent==whole.energyPercent
              && m.blds[0].regenLastTick==whole.regenLastTick,
              "building regeneration is stable across long and short time steps");
        std::fill(m.techDoneOf(0).begin(),m.techDoneOf(0).end(),0);
        m.blds={silicon};m.regenTick=0;m.regenClock=0;m.selBld.clear();
        for(int i=0;i<27;++i) m.StepWorldOnce(.04f);
        check(m.blds.size()==1 && m.blds[0].hp==510 && m.blds[0].energyPercent==99,
              "full world simulation runs autonomous building regeneration without a manual repair toggle");
    }
    {
        maps::Object saved;saved.type=maps::OBJ_BUILDING;saved.subtype=83;saved.owner=0;
        saved.x=saved.y=8;saved.raw.assign(87,0);
        int32_t hp=37,energy=12;
        std::memcpy(saved.raw.data()+63,&hp,4);std::memcpy(saved.raw.data()+67,&energy,4);
        m.terr.objects={saved};m.BuildBlds();
        check(m.blds.size()==1 && m.blds[0].hp==m.blds[0].hpMax*37/100
              && m.blds[0].energyPercent==12,"map building preserves stored damage and energy");
        hp=0;energy=0;std::memcpy(saved.raw.data()+63,&hp,4);std::memcpy(saved.raw.data()+67,&energy,4);
        m.terr.objects={saved};m.BuildBlds();
        check(m.blds[0].hp==0 && m.blds[0].energyPercent==0,
              "map zero HP and zero energy are not silently replaced by full values");
        hp=0x101;energy=-1;std::memcpy(saved.raw.data()+63,&hp,4);std::memcpy(saved.raw.data()+67,&energy,4);
        m.terr.objects={saved};m.BuildBlds();
        check(m.blds[0].hp==m.blds[0].hpMax && m.blds[0].energyPercent==100,
              "invalid signed DWORD percentages normalize to 100 exactly as original loading");
        saved.raw.clear();m.terr.objects={saved};m.BuildBlds();
        check(m.blds[0].hp==m.blds[0].hpMax && m.blds[0].energyPercent==100,
              "new editor building without an archived state retains full defaults");
    }
    {
        m.units.clear();m.blds.clear();m.terr.objects.clear();m.players[0].civ=0;
        Menu::Bld depot;depot.tobj=59;depot.owner=0;depot.hp=100;depot.x=depot.y=8;
        Menu::Unit freight;freight.type=8;freight.owner=0;freight.hp=100;
        freight.x=freight.y=8;freight.hauling=true;freight.haulPhase=3;freight.haulTo=0;freight.cargo.metal=20;
        m.blds={depot};m.units={freight};const auto bank=m.BankOf(0).metal;
        m.StepHaul(m.units[0],.04f);
        check(m.BankOf(0).metal==bank+20 && !m.units[0].cargo.total()
              && m.units[0].haulTo==-1 && m.blds[0].freight.busy && m.blds[0].aniState==1,
              "one-crate delivery credits immediately and leaves its opening animation running");
        Menu::Unit waiting=freight;waiting.haulPhase=2;
        m.units.push_back(waiting);m.StepHaul(m.units[1],.04f);
        check(m.units[1].haulPhase==2 && m.HatchBusy(0,m.units[1]),
              "next transport waits while the released hatch is still animating");
        m.StepWork(.04f);
        check(m.blds[0].aniState==1,"ending a short delivery does not cut off opening midway");
        bool closed=false,sawFlow=false;
        for(int n=0;n<300 && m.blds[0].freight.busy;++n) {
            m.StepWork(.04f);closed|=m.blds[0].aniState==3;sawFlow|=m.blds[0].aniState==2;
        }
        check(closed && !sawFlow && !m.blds[0].freight.busy && m.blds[0].aniState==0,
              "short delivery finishes opening then closes without a phantom transfer loop");
        // This test starts the waiting boat at the real delivery position;
        // route/interpolation is covered by the cave integration scenario.
        m.units[1].x=m.units[1].y=8+101.f/201.f;m.units[1].z=1;
        m.StepHaul(m.units[1],.04f);
        check(m.units[1].haulPhase==3,"waiting transport acquires the hatch after both sequences finish");
        m.units[1].hauling=false;m.StepWork(.04f);
        check(m.blds[0].freight.reservation==-1,"cancelling before the first crate releases the reservation");
        Menu::Bld extractor=depot;extractor.tobj=57;extractor.z=0;
        maps::Object ore;ore.type=maps::OBJ_RESOURCE;ore.subtype=221;ore.x=ore.y=8;ore.amount=1200;
        m.terr.objects={ore};m.blds={extractor};freight.haulPhase=1;freight.cargo={};m.units={freight};
        m.StepHaul(m.units[0],.04f);
        check(m.units[0].cargo.corium==3 && m.blds[0].aniState==1,
              "first mined crate starts opening without a fixed pre-transfer delay");
        for(int n=0;n<300;++n)m.StepWork(.04f);
        auto *flow=m.WorkStrip(m.blds[0]);
        check(flow && m.blds[0].aniState==2 && m.WorkFrame(m.blds[0],flow->count())==0,
              "mine flow holds its last reverse frame instead of looping");
        m.units[0].hauling=false;
        for(int n=0;n<300;++n)m.StepWork(.04f);
        check(!m.blds[0].freight.busy && m.blds[0].aniState==0,
              "cancelled mining completes the work and body sequences before becoming available");
        m.blds={depot};freight.haulPhase=3;freight.cargo.metal=800;m.units={freight};
        m.StepHaul(m.units[0],.04f);
        bool restarted=false;double previous=0;
        for(int n=0;n<300;++n) {
            m.StepWork(.04f);
            if(m.blds[0].aniState==2 && m.blds[0].workTicks<previous) restarted=true;
            previous=m.blds[0].workTicks;
        }
        check(restarted && m.blds[0].aniState==2,"active storage repeats its transfer sequence");
        m.players[0].civ=1;m.blds={extractor};auto &metalMine=m.blds[0];
        metalMine.tobj=79;metalMine.freight.busy=1;
        int first,last;const auto *body=m.FreightBodyTiming(metalMine,first,last);
        check(body && first==0 && last==19 && body->ticks.size()>=20,
              "BO metal mine uses its real body descriptor and sequence-14 range");
        m.units.clear();m.StepWork(0);
        check(metalMine.freight.busy==1,"closed work overlay still waits for the mine body to finish");
        for(int n=0;n<300;++n)m.StepWork(.04f);
        check(!metalMine.freight.busy,"completed mine body releases the final busy state");
    }
    std::printf("buildingcheck: %d failures\n", failures);
    return failures ? 1 : 0;
}
