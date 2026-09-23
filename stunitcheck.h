inline int RunUnitCheck()
{
    int failures=0;
    auto check=[&](bool ok,const char *name) {
        std::printf("unitcheck: %s: %s\n",name,ok?"OK":"FAIL");if(!ok) ++failures;
    };
    auto &m=g_menu;if(!m.OpenEditor(0)) return 1;
    m.blds.clear();m.units.clear();m.me=0;m.aiOn=false;
    Menu::Unit unrelated,carrier,passenger;
    unrelated.hp=100;unrelated.type=1;
    carrier.hp=100;carrier.type=7;carrier.x=8;carrier.y=9;carrier.z=2;carrier.carry=2;
    passenger.hp=100;passenger.type=12;passenger.carried=true;
    m.units={unrelated,carrier,passenger};m.units[0].hp=0;m.Reap();
    check(m.units.size()==2 && m.units[0].carry==1 && m.units[1].carried,
          "unrelated death remaps the carrier to its original passenger");
    m.units[1].path={{1,1}};m.units[1].pathZ={4};m.units[1].moving=true;m.units[1].wantZ=4;
    m.units[1].dodge=3;m.StepCarried();
    check(m.units[1].x==8 && m.units[1].y==9 && m.units[1].z==2,
          "passenger remains attached to the correct carrier after compaction");
    check(!m.units[1].moving && m.units[1].path.empty() && m.units[1].pathZ.empty()
          && m.units[1].wantZ==2 && m.units[1].dodge==0,
          "transport clears stale movement and depth orders on the passenger");
    m.sel={1};m.OrderMove(400,300);m.OrderPatrol(400,300);
    check(!m.units[1].moving && !m.units[1].patrol,
          "carried passenger cannot receive independent move or patrol orders");
    check(m.Docked(m.units[1]),"separation cannot push a carried passenger away from its carrier");
    m.units[1].hp=0;m.Reap();
    check(m.units.size()==1 && m.units[0].carry==-1,
          "passenger death releases the carrier slot without retargeting another unit");
    carrier.carry=1;passenger.carried=true;m.units={carrier,passenger,unrelated};
    m.units[0].hp=0;m.Reap();
    check(m.units.size()==2 && !m.units[0].carried && m.units[0].type==12,
          "surviving passenger cannot remain orphaned in carried state after carrier removal");
    m.units={carrier,passenger};m.units[0].hp=0;m.units[1].x=3;m.StepCarried();
    check(m.units[1].x==3,"dead carrier does not move its passenger before removal");
    carrier.carry=-1;carrier.hp=100;carrier.owner=0;
    passenger.carried=false;passenger.owner=0;passenger.x=8;passenger.y=9;
    m.units={carrier,passenger};
    check(m.TryTakeUnit(0,1) && m.units[0].carry==1 && m.units[1].carried,
          "own boat can be loaded by an available repair submarine");
    check(!m.TryTakeUnit(0,1),"same passenger cannot be loaded twice");
    passenger.owner=1;m.ally[1]=true;m.units={carrier,passenger};
    check(m.TryTakeUnit(0,1),"allied boat can be loaded under current diplomacy");
    m.ally[1]=false;m.units={carrier,passenger};
    check(!m.TryTakeUnit(0,1) && m.units[0].carry==-1 && !m.units[1].carried,
          "hostile boat without paralysis cannot be loaded");
    passenger.owner=0;m.units={carrier,passenger};m.units[0].carried=true;
    check(!m.TryTakeUnit(0,1),"carried repair submarine cannot load another boat");
    m.units={carrier,passenger};m.units[0].launchStage=1;
    check(!m.TryTakeUnit(0,1),"repair submarine cannot load during its own launch");
    m.units={carrier,passenger};m.units[1].hp=0;
    check(!m.TryTakeUnit(0,1),"dead passenger cannot be loaded before Reap");
    m.units={carrier,passenger};
    check(!m.TryTakeUnit(0,0),"repair submarine cannot load itself");
    m.terr.bw=m.terr.bh=8;m.terr.cells.clear();m.terr.objects.clear();m.terr.decor.clear();
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        maps::Cell cell;cell.x=uint8_t(x*2);cell.y=uint8_t(y*2);cell.mesh=4352;cell.texA=1;
        m.terr.cells.push_back(cell);
    }
    m.EdTouched();m.blds.clear();carrier.x=carrier.y=8;carrier.z=1;carrier.carry=1;
    passenger.carried=true;m.units={carrier,passenger};
    m.units[0].dir=7;m.units[0].raw=6;m.units[0].bob.phase=12;
    m.units[0].bob.displacement=boatbob::offset(12);m.units[0].bob.mode=1;
    m.units[1].dir=19;m.units[1].raw=12;m.units[1].bob.phase=30;
    m.units[1].bob.displacement=boatbob::offset(30);m.units[1].bob.mode=0;
    check(m.TryUnloadUnit(0) && m.units[0].carry==-1 && !m.units[1].carried
          && m.units[1].x==8 && m.units[1].y==8 && m.units[1].z==1,
          "unloading leaves the passenger at the carrier position instead of teleporting it sideways");
    check(m.units[1].dir==7 && m.units[1].bob.phase==12
          && m.units[1].bob.displacement==m.units[0].bob.displacement && m.units[1].bob.mode==1,
          "unloading inherits carrier heading and bob without a visual phase jump");
    check(m.units[0].raw==6 && m.units[1].raw==12,
          "unloading preserves both boat speeds while copying motion presentation");
    m.StepBoatBobs(.08f);
    check(m.units[0].bob.phase==m.units[1].bob.phase
          && m.units[0].bob.displacement==m.units[1].bob.displacement,
          "released passenger and carrier resume from the same bob phase");
    check(m.units[0].moving && m.units[0].tx==7 && m.units[0].ty==7 && m.units[0].wantZ==1,
          "carrier departs to the first free same-depth cell in original X-Y search order");
    check(!m.TryTakeUnit(0,1) && m.units[0].carry==-1 && !m.units[1].carried,
          "new load cannot overwrite a still active unload manoeuvre");
    m.StepCarried();
    check(m.Docked(m.units[0]) && m.Docked(m.units[1]),
          "separation waits while carrier leaves the released passenger");
    m.units[0].moving=false;m.StepCarried();
    check(m.units[0].unloadPassenger==-1 && !m.units[0].unloadLocked && !m.units[1].unloadLocked,
          "departure completion releases both separation locks");
    m.units={carrier,passenger};
    for(int x=7;x<=9;++x) for(int y=7;y<=9;++y) if(x!=8 || y!=8) {
        Menu::Unit blocker;blocker.hp=100;blocker.x=float(x);blocker.y=float(y);blocker.z=1;
        m.units.push_back(blocker);
    }
    check(!m.TryUnloadUnit(0) && m.units[0].carry==1 && m.units[1].carried,
          "fully occupied neighbourhood retains cargo on board");
    m.units[2].z=0;
    check(m.TryUnloadUnit(0) && m.units[0].tx==7 && m.units[0].ty==7,
          "boat at another depth does not block unloading clearance");
    m.units={carrier,passenger};m.blds.clear();m.prod.clear();m.shots.clear();m.sel.clear();
    m.units[0].hpMax=m.units[1].hpMax=100;
    m.TryUnloadUnit(0);
    for(int tick=0;tick<100;++tick) m.StepWorldOnce(.04f);
    check(m.units.size()==2 && !m.units[0].moving && std::fabs(m.units[0].x-7)<.01f
          && std::fabs(m.units[0].y-7)<.01f,
          "full simulation moves the unloading carrier to its chosen exit");
    check(m.units.size()==2 && std::fabs(m.units[1].x-8)<.01f && std::fabs(m.units[1].y-8)<.01f
          && !m.units[1].carried && !m.units[1].unloadLocked,
          "full simulation leaves the released passenger in place and clears the transfer lock");
    check(m.TryTakeUnit(0,1),"completed unload permits a subsequent independent load");
    m.units={carrier};m.units[0].carry=-1;m.units[0].bob.phase=0;
    m.units[0].bob.mode=0;m.units[0].bob.displacement=boatbob::offset(0);
    m.bobClock=0;m.bobWorldTick=0;
    m.StepBoatBobs(4);
    check(m.units[0].bob.neutral() && m.units[0].bob.displacement==0,
          "unit simulation finishes the original bob cycle at the loading neutral phase");
    m.StepBoatBobs(1);
    check(m.units[0].bob.neutral(),"neutral bob remains stopped across later simulation ticks");
    m.units[0].bob.mode=1;m.StepBoatBobs(.04f);
    check(m.units[0].bob.neutral(),"bob resume respects the odd world tick instead of advancing early");
    m.StepBoatBobs(.04f);
    check(m.units[0].bob.phase==0,"releasing the bob stop resumes the original cycle");
    auto initial=m.units[0];initial.bob.phase=13;initial.bob.displacement=boatbob::offset(13);
    m.units={initial};m.bobClock=0;m.bobWorldTick=0;m.StepBoatBobs(1.2f);
    auto once=m.units[0].bob;
    m.units={initial};m.bobClock=0;m.bobWorldTick=0;
    for(int n=0;n<30;++n) m.StepBoatBobs(.04f);
    check(m.units[0].bob.phase==once.phase && m.units[0].bob.displacement==once.displacement,
          "bob phase is independent of simulation time chunking");
    m.units[0].carried=true;auto held=m.units[0].bob;m.StepBoatBobs(1);
    check(m.units[0].bob.phase==held.phase,"carried unit has no independent bob animation");
    Menu::Unit turnWhole,turnSplit,turnFine;
    turnWhole.dir=turnSplit.dir=turnFine.dir=0;
    Menu::TurnBoat(turnWhole,11,.4f);
    for(int n=0;n<10;++n) Menu::TurnBoat(turnSplit,11,.04f);
    for(int n=0;n<40;++n) Menu::TurnBoat(turnFine,11,.01f);
    check(turnWhole.dir==10 && turnSplit.dir==turnWhole.dir && turnFine.dir==turnWhole.dir
          && std::fabs(turnWhole.turnAcc-turnSplit.turnAcc)<1e-5f
          && std::fabs(turnWhole.turnAcc-turnFine.turnAcc)<1e-5f,
          "boat turning retains fractions across whole-step and sub-step updates");
    Menu::TurnBoat(turnSplit,11,.04f);
    check(turnSplit.dir==11,"turning reaches the target without overshooting");
    Menu::Unit wrap;wrap.dir=23;Menu::TurnBoat(wrap,1,.08f);
    check(wrap.dir==1,"shared boat turning crosses frame zero by the short route");
    Menu::Unit launch;launch.hp=100;launch.launchStage=1;launch.launchEnd[0]=201;
    int launchWant=m.DirFrame(201,0);launch.dir=(launchWant+24-11)%24;
    auto normal=launch;m.units={launch};
    for(int n=0;n<10;++n) {m.StepLaunches(.04f);Menu::TurnBoat(normal,launchWant,.04f);}
    check(m.units[0].dir==normal.dir && std::fabs(m.units[0].turnAcc-normal.turnAcc)<1e-5f,
          "launch and ordinary boat turning share fractional timing");
    carrier=Menu::Unit{};passenger=Menu::Unit{};
    carrier.hp=carrier.hpMax=400;carrier.type=7;carrier.owner=0;
    passenger.hp=200;passenger.hpMax=400;passenger.type=1;passenger.owner=0;
    carrier.x=passenger.x=8;carrier.y=passenger.y=8;carrier.z=passenger.z=1;
    m.units={carrier,passenger};m.cargoRepairClock=0;m.cargoRepairTick=0;
    m.StepRepSubs(2);
    check(m.units[1].hp==200,"repair submarine does not heal nearby boats for free");
    m.TryTakeUnit(0,1);m.cargoRepairClock=0;m.cargoRepairTick=0;m.StepRepSubs(1);
    check(m.units[1].hp==200,"transport alone does not enable onboard repair");
    m.BankOf(0).metal=100;m.BankOf(0).corium=100;
    check(m.BeginCargoRepair(0,1),"repair command enables repair of the carried passenger");
    m.cargoRepairClock=0;m.cargoRepairTick=0;m.StepRepSubs(.04f);
    check(m.units[1].hp==220 && m.BankOf(0).metal==90 && m.BankOf(0).corium==100,
          "onboard repair uses original HP quantum and metal price without charging corium");
    m.BankOf(0).metal=9;m.StepRepSubs(1);
    check(m.units[1].hp==220 && m.BankOf(0).metal==9,
          "insufficient metal postpones the entire repair quantum without partial spending");
    auto repairStart=m.units;m.BankOf(0).metal=100;m.cargoRepairClock=0;m.cargoRepairTick=0;
    m.StepRepSubs(1.2f);int repairHp=m.units[1].hp,repairMetal=m.BankOf(0).metal;
    m.units=repairStart;m.BankOf(0).metal=100;m.cargoRepairClock=0;m.cargoRepairTick=0;
    for(int n=0;n<30;++n) m.StepRepSubs(.04f);
    check(m.units[1].hp==repairHp && m.BankOf(0).metal==repairMetal,
          "repair ticks and resource spending are independent of time subdivision");
    passenger.owner=1;passenger.carried=false;m.ally[1]=true;m.units={carrier,passenger};
    m.BankOf(0).metal=100;m.BankOf(1).metal=100;m.cargoRepairClock=0;m.cargoRepairTick=0;
    bool alliedRepair=m.BeginCargoRepair(0,1);m.StepRepSubs(.04f);
    check(alliedRepair && m.units[1].hp==220 && m.BankOf(0).metal==100 && m.BankOf(1).metal==90,
          "allied passenger pays repair from its own bank");
    m.units[1].hp=399;m.cargoRepairTick=25;m.StepRepSubs(.04f);
    check(m.units[1].hp==400 && m.BankOf(1).metal==90,
          "last repair quantum clamps HP and truncates its proportional metal cost");
    m.cargoRepairTick=37;m.StepRepSubs(.04f);
    check(m.units[0].carry==-1 && !m.units[0].repairCargo && !m.units[1].carried,
          "completed repair begins automatic unloading on the 37-tick completion check");
    passenger.owner=0;passenger.hp=399;passenger.carried=false;m.units={carrier,passenger};
    m.BankOf(0).metal=100;m.cargoRepairClock=0;m.cargoRepairTick=0;
    m.BeginCargoRepair(0,1);m.sel={0};m.StopUnits(false);m.StepRepSubs(2);
    check(m.units[1].hp==400 && m.units[0].carry==1 && m.units[0].repairCargo
          && !m.units[0].repairOrder && m.units[1].carried,
          "STOP cancels automatic unloading but preserves original onboard repair flag");
    passenger.hp=200;m.units={carrier,passenger};m.BeginCargoRepair(0,1);m.sel={0};
    m.OrderMove(400,300);
    check(!m.units[0].repairOrder && m.units[0].repairCargo && m.units[0].carry==1,
          "replacement movement cancels repair completion command without losing cargo");
    passenger.hp=399;m.units={carrier,passenger};m.BeginCargoRepair(0,1);
    for(int x=7;x<=9;++x) for(int y=7;y<=9;++y) if(x!=8 || y!=8) {
        Menu::Unit block;block.hp=block.hpMax=100;block.x=float(x);block.y=float(y);block.z=1;
        m.units.push_back(block);
    }
    m.cargoRepairClock=0;m.cargoRepairTick=0;m.StepRepSubs(.04f);
    check(m.units[1].hp==400 && m.units[0].carry==1 && m.units[0].repairOrder,
          "completed repair keeps cargo safely aboard when every exit is blocked");
    m.units[2].z=0;m.cargoRepairTick=37;m.StepRepSubs(.04f);
    check(m.units[0].carry==-1 && !m.units[0].repairOrder && m.units[0].tx==7 && m.units[0].ty==7,
          "automatic unload retries after an exit becomes free");
    passenger.hp=399;m.units={carrier,passenger};m.BeginCargoRepair(0,1);
    m.cargoRepairClock=0;m.cargoRepairTick=0;m.sel.clear();m.blds.clear();m.aiOn=false;
    for(int n=0;n<100;++n) m.StepWorldOnce(.04f);
    check(m.units.size()==2 && m.units[1].hp==400 && !m.units[1].carried
          && std::fabs(m.units[1].x-8)<.01f && std::fabs(m.units[1].y-8)<.01f
          && !m.units[0].moving && m.units[0].carry==-1 && !m.units[1].unloadLocked,
          "full world simulation completes repair and carrier departure without moving the passenger");
    check(energy::stored(30)==100,"SHS submarine energy capacity comes from the original table");
    carrier=Menu::Unit{};carrier.type=29;carrier.hp=carrier.hpMax=100;carrier.owner=0;
    carrier.x=carrier.y=8;carrier.z=1;carrier.energy=carrier.energyMax=400;
    passenger=carrier;passenger.type=32;passenger.energy=0;passenger.x=14;
    m.units={carrier,passenger};m.BankOf(0).gold=500;m.blds.clear();
    m.StepRepair(1);
    check(m.units[1].energy==0 && m.BankOf(0).gold==500,
          "Replenisher does not automatically spend bank energy on nearby units");
    m.sel={1};m.OrderRecharge();
    check(m.units[1].energy==400 && m.BankOf(0).gold==100 && m.armed==Menu::ARM_NONE,
          "selected recipient recharges instantly in Replenisher range without targeting another boat");
    m.units[1].energy=0;m.units[1].y=11;m.BankOf(0).gold=500;
    check(!m.TryRechargeUnit(1) && m.BankOf(0).gold==500,
          "original three-dimensional range excludes distance seven");
    m.units[1].y=8;m.units[1].z=4;
    check(!m.TryRechargeUnit(1),"Replenisher range includes depth in the distance metric");
    m.units[1].z=1;m.BankOf(0).gold=101;
    check(m.TryRechargeUnit(1) && m.units[1].energy==100 && m.BankOf(0).gold==0,
          "limited recharge spends available energy with original integer percentage truncation");
    m.units[0].carried=true;m.BankOf(0).gold=500;
    check(!m.TryRechargeUnit(1),"carried Replenisher cannot provide a world recharge source");
    m.units[0].carried=false;m.units[1].owner=1;m.ally[1]=true;m.BankOf(1).gold=500;
    check(!m.TryRechargeUnit(1) && m.BankOf(1).gold==500,
          "allied Replenisher is not in the recipient owner source list");
    // Original regeneration: strict deadline, one percent energy, real research.
    Menu::Unit regen;regen.type=32;regen.hp=100;regen.hpMax=400;
    regen.energy=regen.energyMax=400;regen.owner=0;
    m.players[0].civ=2;
    std::fill(m.techDoneOf(0).begin(),m.techDoneOf(0).end(),0);
    m.units={regen};m.regenClock=0;m.regenTick=0;m.repaired=0;
    m.StepSiRegen(2.04f);
    check(m.units[0].hp==100 && m.units[0].energy==400,
          "regeneration waits through the original strict 50-tick deadline");
    m.StepSiRegen(.04f);
    check(m.units[0].hp==110 && m.units[0].energy==396 && m.units[0].regenLastTick==51,
          "regeneration adds ten HP and spends one percent rather than ten energy units");
    m.units={regen};m.units[0].energy=4;m.regenTick=5001;m.regenClock=0;
    m.StepSiRegen(.04f);
    check(m.units[0].hp==110 && m.units[0].energy==0,
          "last one percent of energy can regenerate instead of being stranded below ten units");
    m.units={regen};m.units[0].hp=399;m.regenTick=51;m.regenClock=0;m.repaired=0;
    m.StepSiRegen(.04f);
    check(m.units[0].hp==400 && m.units[0].energy==396 && m.repaired==1,
          "last regeneration quantum caps HP and records only actual restored health");
    auto enable=[](int id,int level) {int i=tech::indexOf(id,level);if(i>=0) g_menu.techDoneOf(0)[size_t(i)]=1;};
    enable(78,3);m.units={regen};m.regenTick=51;m.regenClock=0;m.StepSiRegen(.04f);
    check(m.units[0].hp==120 && m.units[0].energy==396,
          "energy conservation selects the original upgraded regeneration quantum");
    enable(102,1);m.units={regen};m.regenTick=46;m.regenClock=0;m.StepSiRegen(.04f);
    check(m.units[0].hp==120 && m.units[0].regenLastTick==46,
          "regeneration speed research uses original 33-rate fixed-point interval");
    m.units={regen};m.regenTick=0;m.regenClock=0;m.StepSiRegen(12);
    auto regenWhole=m.units[0];m.units={regen};m.regenTick=0;m.regenClock=0;
    for(int n=0;n<300;++n) m.StepSiRegen(.04f);
    check(m.units[0].hp==regenWhole.hp && m.units[0].energy==regenWhole.energy
          && m.units[0].regenLastTick==regenWhole.regenLastTick,
          "regeneration HP, energy and timestamps agree across time subdivisions");
    m.units={regen};m.units[0].carried=true;m.regenTick=10000;m.regenClock=0;m.StepSiRegen(1);
    check(m.units[0].hp==100 && m.units[0].energy==400,
          "disabled passenger cannot run independent world regeneration");
    m.units={regen};m.units[0].launchStage=1;m.StepSiRegen(1);
    check(m.units[0].hp==100 && m.units[0].energy==400,
          "launching boat waits for normal simulation before regenerating");
    m.units={regen};m.units[0].type=1;m.StepSiRegen(1);
    check(m.units[0].hp==100 && m.units[0].energy==400,
          "human boat does not gain Silicon regeneration from its owner or energy field");
    m.units={regen};m.units[0].energy=0;m.StepSiRegen(1);
    check(m.units[0].hp==100,"empty energy storage stops regeneration");
    m.units={regen};m.units[0].hp=400;m.StepSiRegen(1);
    check(m.units[0].energy==400,"full health consumes no regeneration energy");
    std::fill(m.techDoneOf(0).begin(),m.techDoneOf(0).end(),0);
    regen.x=regen.y=8;regen.z=regen.wantZ=1;
    m.units={regen};m.blds.clear();m.shots.clear();m.prod.clear();m.sel.clear();
    m.regenTick=0;m.regenClock=0;m.aiOn=false;
    for(int n=0;n<52;++n) m.StepWorldOnce(.04f);
    if(m.units.size()!=1 || m.units[0].hp!=110 || m.units[0].energy!=396)
        std::printf("regeneration world: boats=%zu hp=%d energy=%d tick=%u\n",m.units.size(),
                    m.units.empty()?-1:m.units[0].hp,m.units.empty()?-1:m.units[0].energy,m.regenTick);
    check(m.units.size()==1 && m.units[0].hp==110 && m.units[0].energy==396,
          "full world simulation executes original regeneration without a dock or repair order");
    {
        maps::Object saved;saved.type=maps::OBJ_UNIT;saved.subtype=32;saved.owner=0;
        saved.x=saved.y=8;saved.z=1;saved.raw.assign(105,0);
        int32_t percent=37;std::memcpy(saved.raw.data()+38,&percent,4);
        m.terr.objects={saved};m.BuildUnits();
        check(m.units.size()==1 && m.units[0].hp==m.units[0].hpMax*37/100,
              "map boat starts with its stored HP percentage instead of full health");
        percent=-1;std::memcpy(saved.raw.data()+38,&percent,4);m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].hp==1,"negative saved boat percentage means one HP, unlike buildings");
        percent=0;std::memcpy(saved.raw.data()+38,&percent,4);m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].hp==0,"zero saved boat health remains zero");
        percent=257;std::memcpy(saved.raw.data()+38,&percent,4);m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].hp==m.units[0].hpMax,"boat loader reads a full DWORD and caps percentages above 100");
        saved.raw.clear();m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].hp==m.units[0].hpMax,"new boat without a saved record keeps full initial health");
    }
    {
        maps::Object saved;saved.type=maps::OBJ_UNIT;saved.subtype=8;saved.owner=0;
        saved.x=saved.y=8;saved.raw.assign(105,0);
        int32_t hp=100,c=120,metal=800;
        std::memcpy(saved.raw.data()+38,&hp,4);
        std::memcpy(saved.raw.data()+42,&c,4);std::memcpy(saved.raw.data()+46,&metal,4);
        m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].cargo.corium==60 && m.units[0].cargo.metal==400,
              "map cargo keeps both resources within the original 40-crate capacity");
        auto &boat=m.units[0];boat.hauling=true;boat.haulPhase=0;
        m.StepHaul(boat,.04f);
        check(boat.haulPhase==2,"loaded map transport heads to storage before a mine");
        Menu::Bld depot;depot.tobj=59;depot.hp=100;depot.owner=0;m.blds={depot};
        boat.haulTo=0;boat.haulPhase=3;boat.haulTimer=10;
        auto oldCorium=m.BankOf(0).corium,oldMetal=m.BankOf(0).metal;
        for(int n=0;n<2000 && boat.cargo.total();++n) m.StepHaul(boat,.04f);
        check(boat.cargo.total()==0 && m.BankOf(0).corium-oldCorium==60
              && m.BankOf(0).metal-oldMetal==400,
              "mixed cargo unload credits each resource exactly once without dropping metal");
        for(int n=0;n<100;++n) m.StepHaul(boat,.04f);
        check(m.BankOf(0).corium-oldCorium==60 && m.BankOf(0).metal-oldMetal==400,
              "empty hold cannot duplicate resources while closing the hatch");
        c=5;metal=39;std::memcpy(saved.raw.data()+42,&c,4);std::memcpy(saved.raw.data()+46,&metal,4);
        m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].cargo.corium==3 && m.units[0].cargo.metal==20,
              "initial cargo truncates incomplete crates like the original loader");
        c=-1;metal=-1;std::memcpy(saved.raw.data()+42,&c,4);std::memcpy(saved.raw.data()+46,&metal,4);
        m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].cargo.total()==0,"negative map cargo cannot create or subtract resources");
        saved.raw.clear();m.terr.objects={saved};m.BuildUnits();
        check(m.units[0].cargo.total()==0,"new boats start with empty cargo");
    }
    {
        cargotransfer::Cargo hold;hold.gold=5;hold.corium=3;hold.metal=20;
        auto first=cargotransfer::unload(hold);
        auto second=cargotransfer::unload(hold);
        auto third=cargotransfer::unload(hold);
        check(first.kind==220 && first.amount==5 && second.kind==221 && second.amount==3
              && third.kind==222 && third.amount==20 && !hold.total(),
              "unload uses original gold/corium/metal priority and crate sizes");
        int store=2;
        check(!cargotransfer::load(hold,221,store) && store==0 && !hold.total(),
              "depleted deposit remainder is consumed without inventing a full crate");
        hold.corium=117;store=100;
        check(cargotransfer::load(hold,222,store)==20 && store==80
              && !cargotransfer::load(hold,221,store) && hold.crates()==40,
              "loading observes shared crate capacity across different resources");
        cargotransfer::Clock clock;
        check(clock.advance(.04f)==1 && clock.advance(.36f)==0 && clock.advance(.04f)==1,
              "transfer occurs on tick one and eleven, not every frame");
        cargotransfer::Clock small,large;int portions=0;
        for(int n=0;n<250;++n) portions+=small.advance(.04f);
        check(portions==25 && large.advance(10)==25 && small.tick==large.tick,
              "cargo timer preserves transfer count across simulation step sizes");
        Menu::Unit transport;transport.type=8;transport.hp=100;transport.owner=0;
        transport.hauling=true;transport.haulPhase=3;transport.haulTo=0;transport.haulTimer=10;
        transport.cargo.gold=5;transport.cargo.corium=6;transport.cargo.metal=40;
        Menu::Bld depot;depot.tobj=59;depot.hp=100;depot.owner=0;m.blds={depot};
        auto g=m.BankOf(0).gold,c=m.BankOf(0).corium,metal=m.BankOf(0).metal;
        m.StepHaul(transport,.04f);
        check(m.BankOf(0).gold-g==5 && m.BankOf(0).corium==c && m.BankOf(0).metal==metal,
              "live unloading moves exactly one gold crate on its first tick");
        m.StepHaul(transport,.36f);
        check(transport.cargo.corium==6 && transport.cargo.metal==40,
              "live unloading waits nine ticks before the next crate");
        m.StepHaul(transport,.04f);
        check(m.BankOf(0).corium-c==3 && transport.cargo.corium==3,
              "live unloading transfers corium in three-unit crates");
        transport.haulPhase=1;transport.haulTimer=10;transport.cargo={};transport.cargoClock.reset();
        maps::Object deposit;deposit.type=maps::OBJ_RESOURCE;deposit.subtype=222;deposit.amount=800;
        m.terr.objects={deposit};m.blds[0].freight={};m.blds[0].tobj=79;m.blds[0].z=0;m.blds[0].kind=222;m.StepHaul(transport,.04f);
        check(transport.cargo.metal==20 && m.terr.objects[0].amount==780 && m.blds[0].store==0,
              "live loading transfers one twenty-unit metal crate");
    }
    {
        maps::Object ore;ore.type=maps::OBJ_RESOURCE;ore.subtype=221;ore.amount=123;
        ore.x=ore.y=8;m.terr.objects={ore};
        Menu::Bld mine;mine.tobj=57;mine.owner=0;mine.hp=100;mine.z=0;mine.x=mine.y=8;m.blds={mine};
        for(int i=0;i<60;++i) m.StepEconomy(1);
        check(m.terr.objects[0].amount==123 && !m.blds[0].store,
              "idle mine leaves the deposit untouched and never fills an invented buffer");
        check(m.NearestBld(0,8,8,false)==0,"transport can select a mine immediately with no buffer warmup");
        Menu::Unit boat;boat.type=8;boat.owner=0;boat.hp=100;
        check(m.LoadMineCrate(boat,m.blds[0])==3 && boat.cargo.corium==3
              && m.terr.objects[0].amount==120 && !m.blds[0].store,
              "docked loading removes one crate directly from the map deposit");
        m.blds[0].hp=0;
        check(m.LoadMineCrate(boat,m.blds[0])==0 && m.terr.objects[0].amount==120
              && m.NearestBld(0,8,8,false)==-1,"destroyed mine cannot supply cargo or be selected");
        m.blds[0].hp=100;m.blds[0].tobj=79;
        check(m.LoadMineCrate(boat,m.blds[0])==0 && m.terr.objects[0].amount==120,
              "metal extractor cannot consume an overlapping corium deposit");
        m.blds[0].tobj=57;m.terr.objects[0].amount=2;
        check(m.LoadMineCrate(boat,m.blds[0])==0 && !m.terr.objects[0].amount
              && boat.cargo.corium==3 && m.NearestBld(0,8,8,false)==-1,
              "final incomplete crate exhausts deposit without creating extra cargo");
        check(m.LoadMineCrate(boat,m.blds[0])==0 && boat.cargo.corium==3,
              "exhausted deposit cannot be consumed twice");
        m.terr.objects[0].amount=20;boat.cargo.corium=120;
        check(m.LoadMineCrate(boat,m.blds[0])==0 && m.terr.objects[0].amount==20,
              "full transport cannot drain a deposit");
    }
    {
        maps::Object lower;lower.type=maps::OBJ_RESOURCE;lower.subtype=221;
        lower.x=lower.y=8;lower.z=0;lower.amount=100;
        auto upper=lower;upper.z=2;upper.amount=200;
        m.terr.objects={lower,upper};
        Menu::Bld mine;mine.tobj=57;mine.owner=0;mine.hp=100;mine.x=mine.y=8;mine.z=2;
        Menu::Unit boat;boat.type=8;boat.hp=100;
        check(m.MineDepositIndex(mine)==1 && m.LoadMineCrate(boat,mine)==3
              && m.terr.objects[0].amount==100 && m.terr.objects[1].amount==197,
              "mine consumes only the deposit at its exact level, not one below a platform");
        mine.z=1;
        check(m.MineDepositIndex(mine)==-1 && !m.LoadMineCrate(boat,mine),
              "mine between resource levels cannot drain either deposit");
        mine.z=0;mine.x=9;
        check(m.MineDepositIndex(mine)==-1,"adjacent deposit is not a mine's bound resource");
        mine.x=8;m.terr.objects[0].amount=0;
        check(m.MineDepositIndex(mine)==-1,"exhausted resource cannot fall through to a different depth");
        auto metal=upper;metal.subtype=222;metal.amount=1000;
        m.terr.objects={lower,metal};m.terr.objects[0].amount=500;
        Menu::Bld accumulator=mine;accumulator.tobj=97;accumulator.z=2;
        m.blds={accumulator};m.players[0].civ=2;
        m.BankOf(0).oxy=0;m.BankOf(0).oxyCap=1000;
        m.StepSupply(1);
        check(m.terr.objects[0].amount==500 && m.terr.objects[1].amount<1000,
              "energy accumulator consumes metal at its own level and leaves corium untouched");
        m.blds[0].z=0;auto before=m.terr.objects[0].amount;
        m.StepSupply(1);
        check(m.terr.objects[0].amount==before,"energy accumulator cannot consume corium at matching XYZ");
    }
    {
        m.units.clear();m.blds.clear();m.terr.objects.clear();m.aiOn=false;
        m.ResetSilicon();
        check(m.siliconGrid.size()==size_t(m.terr.bw*2)*size_t(m.terr.bh*2)
              && !m.siliconGrid.empty() && m.siliconGrid.front()==80,
              "new game initializes the shared seabed silicon grid to original 80 per cell");
        Menu::Bld extractor;extractor.tobj=100;extractor.owner=0;extractor.hp=100;
        extractor.x=extractor.y=16;extractor.z=0;m.blds={extractor};
        auto bank=m.BankOf(0).metal;auto other=m.BankOf(1).metal;
        for(int n=0;n<225;++n)m.StepSilicon(.04f);
        check(m.BankOf(0).metal==bank && m.blds[0].siliconState.stored==90,
              "silicon extractor accumulates nine portions without premature bank credit");
        for(int n=0;n<25;++n)m.StepSilicon(.04f);
        check(m.BankOf(0).metal==bank+100 && m.BankOf(1).metal==other
              && m.blds[0].siliconState.stored==0,
              "silicon extractor credits its owner after 100 without a deposit or transport");
        m.blds[0].buildLeft=20;auto cursor=m.blds[0].siliconState.cursor;
        m.StepSilicon(2);
        check(m.blds[0].siliconState.cursor==cursor && m.BankOf(0).metal==bank+100,
              "unfinished silicon extractor consumes no seabed resources");
        m.blds[0].buildLeft=0;m.ResetSilicon(true);m.StepSilicon(2);
        check(m.BankOf(0).metal==bank+100 && m.blds[0].siliconState.stored==0,
              "no-minerals silicon grid cannot generate resources");
        m.ResetSilicon();
        check(m.siliconTick==0 && m.siliconGrid.front()==80 && !m.blds[0].siliconStarted,
              "new match resets depleted silicon and extractor timers");
        std::fill(m.siliconGrid.begin(),m.siliconGrid.end(),0);
        m.siliconGrid[size_t(1)*(m.terr.bw*2)+1]=10;
        m.blds.push_back(extractor);m.blds[1].owner=1;m.StepSilicon(1);
        check(m.blds[0].siliconState.stored+m.blds[1].siliconState.stored==10,
              "overlapping extractors share depletion instead of duplicating silicon");
        m.blds={extractor};
        check(!m.SiliconSpacing(31,31,0,0) && m.SiliconSpacing(32,16,0,0),
              "silicon build spacing matches original strict sixteen-cell XY bounds");
        check(m.SiliconSpacing(17,17,0,1) && !Menu::IsExtractor(100) && Menu::ExtractorKind(100)==0,
              "silicon construction is independent of foreign extractors and metal deposits");
        m.ResetSilicon();m.units.clear();m.shots.clear();m.prod.clear();m.sel.clear();
        bank=m.BankOf(0).metal;
        for(int n=0;n<250;++n)m.StepWorldOnce(.04f);
        check(m.BankOf(0).metal==bank+100,"full world update runs silicon extraction and bank credit");
    }
    {
        m.blds.clear();m.units.clear();m.terr.objects.clear();m.me=0;
        m.players[1].civ=2;m.players[1].active=true;
        m.terr.bw=m.terr.bh=32;m.terr.cells.clear();m.terr.decor.clear();
        for(int y=0;y<32;++y) for(int x=0;x<32;++x) {
            maps::Cell cell;cell.x=uint8_t(x*2);cell.y=uint8_t(y*2);cell.mesh=4352;cell.texA=1;
            m.terr.cells.push_back(cell);
        }
        m.EdTouched();m.ResetSilicon();
        int cx=-1,cy=-1;
        for(int y=4;y<m.terr.bh*2-20 && cx<0;++y)
            for(int x=4;x<m.terr.bw*2-20;++x)
                if(m.CanBuildAt(x,y,100,nullptr,1)) {cx=x;cy=y;break;}
        check(cx>=0,"AI silicon fixture finds a valid site without any metal deposit");
        if(cx>=0) {
            Menu::Bld neighbor;neighbor.tobj=100;neighbor.owner=0;neighbor.hp=100;
            neighbor.x=cx+8;neighbor.y=cy+8;neighbor.z=0;m.blds={neighbor};
            check(!m.CanBuildAt(cx,cy,100,nullptr,0) && m.CanBuildAt(cx,cy,100,nullptr,1),
                  "placement uses requested owner instead of the viewing human player");
            m.BankOf(1).metal=10000;m.BankOf(1).corium=10000;
            int created=m.BeginBuilding(1,100,cx,cy);
            check(created>=0 && m.blds[size_t(created)].owner==1 && m.blds[size_t(created)].tobj==100,
                  "AI can actually start silicon construction near a foreign extractor");
            m.blds={neighbor};m.blds[0].owner=1;
            check(!m.CanBuildAt(cx,cy,100,nullptr,1) && m.BeginBuilding(1,100,cx,cy)<0,
                  "AI cannot bypass spacing to its own extractors at construction start");
        }
        m.blds.clear();
        for(int type:{92,83,96}) {
            Menu::Bld base;base.tobj=type;base.owner=1;base.hp=100;
            m.blds.push_back(base);
        }
        check(!m.HasDeposit(222) && m.AiWanted(1)==100,
              "Silicon AI requests its extractor from seabed resources without metal deposits");
        m.ResetSilicon(true);
        check(m.AiWanted(1)!=100,"AI does not request a silicon extractor on an empty silicon grid");
        m.ResetSilicon();m.me=2;
        check(m.AiWanted(1)==100,"AI resource choice does not depend on the observing player");
        m.me=0;
        Menu::Unit builder;builder.type=25;builder.owner=1;builder.hp=100;
        builder.x=builder.y=24;builder.z=1;builder.maxLevel=4;builder.raw=9;
        m.units={builder};m.BankOf(1).metal=m.BankOf(1).corium=10000;
        Menu::Bld foreign;foreign.tobj=100;foreign.owner=0;foreign.hp=100;
        foreign.x=foreign.y=32;m.blds.push_back(foreign);
        m.AiEconomy(1);
        check(m.units[0].bldTobj==100 && m.units[0].moving,
              "AI assigns its capsule a silicon construction order near a foreign extractor");
        const size_t count=m.blds.size();
        m.RaiseBuilding(m.units[0]);
        check(m.blds.size()==count+1 && m.blds.back().tobj==100 && m.blds.back().owner==1
              && m.units[0].hp==0,
              "assigned capsule starts its reserved construction and is consumed");
    }
    {
        m.blds.clear();m.units.clear();m.terr.objects.clear();m.me=0;m.setupOpt[SOPT_GOLD]=1;
        m.ResetGold();
        check(m.goldWater.initial==m.terr.bw*2*m.terr.bh*2*gold::OBJECT_LEVELS && m.goldWater.period==50,
              "normal gold concentration initializes reservoir and original fifty-tick period");
        Menu::Bld rig;rig.tobj=58;rig.owner=1;rig.hp=100;rig.x=rig.y=20;rig.z=0;rig.span=1;
        m.blds={rig};int bank=m.BankOf(1).gold;
        for(int i=0;i<49;++i)m.StepGold(.04f);
        check(m.blds[0].goldState.stored==0,"gold extractor waits for its first production interval");
        m.StepGold(.04f);
        check(m.blds[0].goldState.stored==2 && m.BankOf(1).gold==bank
              && m.goldWater.remaining==m.goldWater.initial-2,
              "gold extraction consumes shared water and buffers two without premature credit");
        m.DumpStore(m.blds[0],false);
        check(m.BankOf(1).gold==bank+2 && m.blds[0].goldState.stored==0,
              "manual gold transfer credits the extractor owner and empties its real buffer");
        m.blds[0].goldState.stored=98;m.blds[0].goldState.last=0;m.StepGold(.04f);
        check(m.BankOf(1).gold==bank+102 && m.blds[0].goldState.stored==0,
              "full gold buffer automatically credits all one hundred units");
        auto neighbor=rig;neighbor.x+=3;neighbor.z=1;neighbor.owner=0;m.blds.push_back(neighbor);
        auto neighborhood=m.GoldNeighbors(m.blds[0]);
        check(neighborhood.count==1 && neighborhood.distanceSum==3,"gold neighborhood includes a foreign extractor on another level");
        m.blds[1].span=2;
        check(m.GoldNeighbors(m.blds[0]).count==4,"large neighboring extractor contributes all four occupied cells");
        m.blds[1].buildLeft=10;
        check(m.GoldNeighbors(m.blds[0]).count==0,"unfinished extractor does not enter the active gold neighborhood");
        m.blds[0].buildLeft=10;int stored=m.blds[0].goldState.stored;m.StepGold(4);
        check(m.blds[0].goldState.stored==stored,"unfinished gold extractor does not produce");
        m.ResetGold();check(!m.blds[0].goldStarted && m.goldTick==0,"new game resets gold extraction timers");
        check(gold::interval(131072,1160)==0x0fffffff,"original gold arithmetic overflow suspends instead of crashing");
        m.blds={rig};m.shots.clear();m.aiOn=false;m.ResetGold();
        for(int i=0;i<50;++i)m.StepWorldOnce(.04f);
        check(m.blds[0].goldState.stored==2,"full world simulation runs the gold extractor timer");
        m.setupOpt[SOPT_GOLD]=0;m.ResetGold();const int low=m.goldWater.period;
        m.setupOpt[SOPT_GOLD]=2;m.ResetGold();
        check(low==200 && m.goldWater.period==22,"match concentration setting changes actual gold extraction timing");
        m.setupOpt[SOPT_GOLD]=1;
    }
    {
        m.units.clear();m.blds.clear();m.terr.objects.clear();
        Menu::Bld neighbor;neighbor.tobj=58;neighbor.owner=0;neighbor.hp=100;
        neighbor.x=neighbor.y=20;neighbor.span=1;m.blds={neighbor};
        check(m.CanBuildAt(21,20,58),"gold extractor may be placed next to another extractor");
        check(!m.CanBuildAt(20,20,58),"adjacent gold construction still rejects an occupied cell");
        m.blds[0].owner=1;
        check(m.CanBuildAt(21,20,58),"foreign gold extractor does not impose an invented spacing radius");
        check(Menu::BldFootprint(50)==2 && Menu::BldFootprint(58)==1,
              "construction footprint is available from original type data without loading artwork");
    }
    {
        m.units.clear();m.blds.clear();m.terr.objects.clear();
        maps::Object ore;ore.type=maps::OBJ_RESOURCE;ore.subtype=222;ore.amount=800;
        ore.x=ore.y=8;ore.z=0;m.terr.objects={ore};
        for(int phase=0;phase<4;++phase) for(int fault=0;fault<4;++fault) {
            Menu::Unit boat;boat.type=8;boat.owner=0;boat.hp=100;
            boat.hauling=true;boat.haulPhase=phase;boat.haulTo=0;boat.haulTimer=10;
            boat.x=boat.y=8;boat.cargo.metal=phase?40:0;
            boat.path={{10,10}};boat.pathZ={2};boat.moving=true;boat.wantZ=2;
            boat.cargoClock.advance(.04f);
            Menu::Bld target;target.tobj=phase>=2?59:79;target.owner=0;target.hp=100;
            target.x=target.y=8;target.z=0;
            m.blds={target};
            check(m.HaulTargetAvailable(0,0,phase>=2,phase==0),
                  "freight target is usable before the simulated loss");
            if(fault==0) target.hp=0;
            if(fault==1) target.owner=1;
            if(fault==2) target.buildLeft=10;
            if(fault==3) target.tobj=50;
            m.blds={target};
            const auto bank=m.BankOf(0).metal,foreign=m.BankOf(1).metal;
            m.StepHaul(boat,.04f);
            std::string label="invalid freight target phase "+std::to_string(phase)+" reason "+std::to_string(fault);
            check(boat.haulTo==-1 && boat.haulPhase==(phase?2:0)
                  && boat.cargo.metal==(phase?40:0) && !boat.moving
                  && boat.path.empty() && boat.pathZ.empty() && boat.wantZ==boat.z
                  && boat.haulTimer==0 && boat.cargoClock.tick==0
                  && m.BankOf(0).metal==bank && m.BankOf(1).metal==foreign,label.c_str());
        }
        Menu::Unit boat;boat.type=8;boat.hp=100;boat.owner=0;boat.hauling=true;
        boat.haulTo=0;boat.haulPhase=2;boat.x=boat.y=8+101.f/201.f;boat.z=1;boat.cargo.metal=40;
        boat.path={{20,20}};boat.pathZ={1};boat.moving=true;
        Menu::Bld lost;lost.tobj=59;lost.hp=0;lost.owner=0;
        Menu::Bld replacement=lost;replacement.hp=100;replacement.x=replacement.y=8;
        m.blds={lost,replacement};m.units={boat};m.Reap();
        check(m.units[0].haulTo==-1 && !m.units[0].moving && m.units[0].path.empty()
              && m.units[0].cargo.metal==40,"building removal clears an approaching transport's stale route");
        m.StepHaul(m.units[0],.04f);
        check(m.units[0].haulTo==0 && m.units[0].haulPhase==3 && m.units[0].cargo.metal==40,
              "interrupted delivery selects a surviving depot after index compaction");
        const auto bank=m.BankOf(0).metal;
        for(int i=0;i<100;++i)m.StepHaul(m.units[0],.04f);
        check(!m.units[0].cargo.total() && m.BankOf(0).metal==bank+40,
              "rerouted delivery credits its original cargo exactly once");
    }
    std::printf("unitcheck: %d failures\n",failures);
    return failures?1:0;
}
