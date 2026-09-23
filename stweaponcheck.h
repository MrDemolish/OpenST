// Behavioral checks: exercise combat, not just generated table values.
inline int RunWeaponCheck()
{
    int failures = 0;
    auto check = [&](bool ok, const char *what) {
        std::printf("weaponcheck: %s: %s\n", what, ok ? "OK" : "FAIL");
        if (!ok) ++failures;
    };
    if (!g_menu.OpenEditor(0)) return 1;
    auto &m = g_menu;
    int mediaCount = 0, mediaFailures = 0;
    for (int id = 0x96; id <= 0xbf; ++id) {
        auto media = projmedia::get(id);
        if (!media.sprite) continue;
        ++mediaCount;
        auto strip = m.unitSet.misc(media.sprite);
        bool ok = strip && strip->count();
        auto explosion = m.unitSet.misc(media.explosion);
        ok = ok && explosion && explosion->count();
        for (int sound : {media.launch, media.impact}) {
            auto names = m.sfx.resolveAll(sound);
            // These three IDs are requested by ST.exe but absent from this
            // installation's SOUNDS.DKX (verified with ark.py). Preserve silence.
            bool absentInOriginal = sound == 1149 || sound == 1167 || sound == 1170;
            ok = ok && (absentInOriginal ? names.empty() : !names.empty());
            for (const auto &name : names) {
                std::vector<uint8_t> wav;
                if (!m.sfx.wrap(name, wav) || wav.size() < 44) ok = false;
            }
        }
        if (!ok) {
            ++mediaFailures;
            std::printf("weaponcheck: missing media for 0x%x (%s)\n", id, media.sprite);
        }
    }
    check(mediaCount == 25 && mediaFailures == 0,
          "25 sprites and 47 audio events resolve; 3 original absent IDs remain silent");
    m.terr.bw = m.terr.bh = 12;
    m.terr.cells.clear(); m.terr.objects.clear(); m.terr.decor.clear();
    for (int y = 0; y < 12; ++y) for (int x = 0; x < 12; ++x) {
        maps::Cell c; c.x = uint8_t(x*2); c.y = uint8_t(y*2);
        c.texA = 1; c.mesh = 4352; m.terr.cells.push_back(c);
    }
    m.EdTouched(); m.units.clear(); m.blds.clear(); m.shots.clear();
    m.me = 0; m.players[0].civ = 0; m.players[1].civ = 1;
    m.players[0].active = m.players[1].active = true;
    Menu::Unit target; target.type = 12; target.owner = 0;
    target.x = 7; target.y = 4; target.z = 2; target.hp = target.hpMax = 10000;
    target.tgtOrder = true; // prevent the separate automatic dodge mechanic
    m.units.push_back(target);
    Menu::Bld tower; tower.owner = 1; tower.tobj = 71; tower.x = tower.y = 4;
    tower.z = 2; tower.hp = tower.hpMax = 10000; tower.fireFree = false;
    tower.tgt = tower.tgtSecond = 0;
    m.blds.push_back(tower); m.occStamp = -1;
    m.StepCombat(0.01f);
    check(m.shots.size() == 2 && m.shots[0].projectile == 0x99
        && m.shots[0].dmgU == 120 && !m.shots[0].guided
        && m.shots[1].projectile == 0xab && m.shots[1].dmgU == 350
        && m.shots[1].guided, "enemy MML fires both original weapon slots");
    check(m.shots.size() == 2 && m.shots[0].z == 2.5f,
          "tower launches from its saved level");
    m.shots.clear(); m.StepCombat(2.0f);
    check(m.shots.size() == 1 && m.shots[0].projectile == 0x99,
          "cassette reloads in two seconds independently");
    m.shots.clear(); m.StepCombat(1.0f);
    check(m.shots.size() == 1 && m.shots[0].projectile == 0xab,
          "magnetic mine reloads in three seconds independently");
    m.shots.clear(); m.blds[0].tgt = m.blds[0].tgtSecond = -1;
    m.StepCombat(4.0f);
    check(m.shots.empty(), "fire-at-will off stops both slots without a command");

    m.blds.clear(); m.units[0].owner = 1;
    Menu::Unit bomber = target; bomber.owner = 0; bomber.type = 4;
    bomber.x = 4; bomber.tgt = 0; bomber.tgtOrder = true;
    m.units.push_back(bomber); m.occStamp = -1;
    m.StepCombat(0.01f);
    check(m.shots.size() == 1 && m.shots[0].projectile == 0x96
        && m.shots[0].dmgU == 50, "DC Bomber normal attack fires one small torpedo");
    check(wep::bombGun(4).shots == 15 && wep::bombGun(4).dmgUnit == 500
        && !wep::bombGun(1).armed(), "bomb command keeps its separate ammunition stats");
    m.units[1].bombing = true; m.units[1].bombX = target.x;
    m.units[1].bombY = target.y; m.units[1].x = target.x;
    m.ally[0] = m.ally[1] = 0;
    int before = m.units[0].hp;
    m.StepBombing(0.1f);
    check(m.units[0].hp == before - 500, "bomb command does not use torpedo damage");

    m.shots.clear(); m.units.resize(1); m.units[0].z = 3;
    m.Shoot(wep::unitGun(4), 4, 4, 0.8f, 0, false, 0, 0, 0, 0x96);
    m.StepShots(0.05f);
    check(m.shots.size() == 1 && m.shots[0].z > 0.8f && m.shots[0].z < 3.8f,
          "projectile interpolates altitude along its path");
    m.units[0].z = 0; before = m.units[0].hp;
    m.StepShots(10.0f);
    check(m.shots.empty() && m.units[0].hp == before,
          "unguided projectile misses a target that changed depth");
    m.units.clear(); m.blds.clear(); m.shots.clear();
    target.owner = 0; target.x = 9; target.y = 4; target.z = 0;
    m.units.push_back(target);
    tower.x = 4; tower.y = 4; tower.z = 0; tower.turret = true;
    tower.tgt = tower.tgtSecond = 0;
    int heading = m.DirFrame(5, 0);
    tower.dir = tower.want = (heading + 12) % 24;
    m.blds.push_back(tower); m.occStamp = -1;
    m.StepCombat(0.04f);
    check(m.shots.empty() && m.blds[0].dir == tower.dir,
          "turret cannot snap to a rear target or fire before aiming");
    m.StepCombat(0.04f);
    check(std::abs(turret::difference(tower.dir, m.blds[0].dir)) == 1,
          "original tracking interval turns exactly one 15-degree step");
    for (int i=0; i<10; ++i) m.StepCombat(0.04f);
    check(m.shots.size() == 1 && m.shots[0].projectile == 0xab,
          "MML secondary can fire within 90 degrees while primary still aims");
    for (int i=0; i<12; ++i) m.StepCombat(0.04f);
    check(m.shots.size()==1 && m.blds[0].fireActive,
          "aligned primary starts its firing animation before creating a projectile");
    for(int i=0;i<4;++i) m.StepCombat(0.04f);
    check(m.shots.size() == 2 && m.shots[1].projectile == 0x99,
          "primary cassette fires only when the barrel faces its target");
    Menu::Bld coarse=tower, fine=tower;
    coarse.want=fine.want=heading;
    m.TurnTurret(coarse,0.8f);
    for(int i=0;i<20;++i) m.TurnTurret(fine,0.04f);
    check(coarse.dir == fine.dir, "turret turn rate is independent of simulation step size");
    m.units.clear(); m.blds.clear(); m.shots.clear();
    auto roof=m.edit.contours.at(m.edit.contourFamily)[15];
    roof.x=8; roof.y=4; roof.level=2; m.terr.cells.push_back(roof); m.EdTouched();
    check(!m.WaterAt(4,2,1), "projectile collision fixture contains rock at its flight depth");
    Menu::Shot shot; shot.x=7.95f; shot.y=4.5f; shot.z=1.5f;
    float hx,hy,hz;
    check(m.ShotRockHit(shot,8.05f,4.5f,1.5f,hx,hy,hz),
          "short projectile step detects entry into rock");
    check(m.ShotRockHit(shot,10.5f,4.5f,1.5f,hx,hy,hz),
          "long projectile step cannot tunnel through rock");
    shot.thruWall=true;
    check(!m.ShotRockHit(shot,8.05f,4.5f,1.5f,hx,hy,hz),
          "original CheckRay-exempt projectile still bypasses terrain");
    shot.thruWall=false; shot.tx=8.05f; shot.ty=4.5f; shot.tz=1.5f;
    int blocked=m.shotsBlocked; m.shots.push_back(shot); m.StepShots(1.0f);
    check(m.shots.empty() && m.shotsBlocked==blocked+1,
          "arrival segment is collision-tested before applying an impact");
    m.terr.cells.pop_back(); m.EdTouched();
    m.units.clear(); m.blds.clear(); m.shots.clear();
    Menu::Shot mother;
    mother.projectile=0x99; mother.shooterType=16; mother.owner=0;
    mother.x=4; mother.y=4; mother.z=2; mother.tx=15; mother.ty=4; mother.tz=2;
    mother.speed=Menu::ShotSpeed(0,0x99);
    m.shots.push_back(mother); m.StepShots(0.19f);
    check(m.shots.size()==1 && m.shots[0].projectile==0x99,
          "cassette remains intact before original travel threshold");
    m.StepShots(0.02f);
    bool six=m.shots.size()==6;
    for(const auto &s:m.shots) six=six && s.projectile==0xb4 && s.dmgU==70 && s.owner==0;
    check(six,"Invader cassette splits into six original child torpedoes");
    check(six && std::abs(m.shots[0].x-(4+432.0f/201))<0.001f,
          "cassette splits at the original tick-distance boundary");
    m.shots.clear(); mother.shooterType=71; m.shots.push_back(mother); m.StepShots(0.25f);
    check(m.shots.size()==8,"MML cassette creates eight children without invalidating shot iteration");
    m.shots.clear(); mother.projectile=0xb7; mother.shooterType=32;
    m.shots.push_back(mother); m.StepShots(0.25f);
    bool ions=m.shots.size()==6;
    for(const auto &s:m.shots) ions=ions && s.projectile==0xb9 && s.dmgU==20
        && std::abs(s.speed-Menu::ShotSpeed(0,0xb9))<0.001f;
    check(ions,"ion cassette creates six faster ion projectiles with their own damage");
    m.shots.clear(); mother.projectile=0x99; mother.shooterType=16;
    target.owner=1; target.x=5; target.y=4; target.z=2-Menu::HOVER;
    target.hp=target.hpMax=10000; m.units.push_back(target);
    mother.tx=5; mother.tgt=0; mother.dmgU=120;
    m.shots.push_back(mother); m.StepShots(0.15f);
    check(m.units[0].hp==9880 && m.shots.size()==6,
          "close impact applies mother damage and still releases its children");
    m.StepShots(0.5f);
    check(m.shots.empty(),"children cannot recursively split into another cassette");
    m.units.clear(); m.shots.clear(); mother.tx=15; mother.tgt=-1;
    target.x=7; target.z=2-Menu::HOVER; m.units.push_back(target);
    std::vector<Menu::Shot> children;
    m.SplitCassette(mother,6,4,2,children);
    check(children.size()==6 && children[0].tgt==0 && children[0].tx==7,
          "cassette assigns a forward enemy to one cone projectile");
    m.units[0].owner=0; children.clear(); m.SplitCassette(mother,6,4,2,children);
    check(children.size()==6 && children[0].tgt==-1,
          "cassette target search excludes friendly objects");
    m.units.clear();m.blds.clear();m.shots.clear();
    target.owner=1;target.x=7;target.y=4;target.z=3;target.hp=target.hpMax=1000;
    target.launchStage=2;target.launchSteps=100;target.launchStep=0;
    target.launchStart[0]=4*201+100;target.launchStart[1]=4*201+100;target.launchStart[2]=100;
    target.launchEnd[0]=7*201+100;target.launchEnd[1]=4*201+100;target.launchEnd[2]=700;
    m.units={target};
    float tx=0,ty=0;
    check(m.TargetPos(0,false,tx,ty) && tx==4 && ty==4 && m.TargetDepth(0,false)==0,
          "launch target exposes physical hull coordinates instead of reserved grid coordinates");
    bool building=false;
    check(m.FindTarget(0,3,4,1,building,0)==0 && !building,
          "target acquisition measures launch range and depth at the actual hull");
    Menu::Shot impact;impact.tgt=0;impact.owner=0;impact.dmgU=100;impact.speed=100;
    impact.x=impact.tx=7;impact.y=impact.ty=4;impact.z=impact.tz=3+Menu::HOVER;
    m.shots={impact};m.StepShots(.04f);
    check(m.units[0].hp==1000 && m.shots.empty(),
          "shot at the reserved launch destination misses a hull still inside the dock");
    impact.x=impact.tx=4;impact.z=impact.tz=Menu::HOVER;
    m.shots={impact};m.StepShots(.04f);
    check(m.units[0].hp==900,"shot at the physical emerging hull applies damage");
    m.units[0].launchStep=50;
    check(m.TargetPos(0,false,tx,ty) && std::fabs(tx-(4+301.f/201))<.001f
          && m.TargetDepth(0,false)==1.5f,"combat coordinates follow integer launch interpolation between ticks");
    impact.x=impact.tx=4;impact.z=impact.tz=Menu::HOVER;
    m.shots={impact};m.StepShots(.04f);
    check(m.units[0].hp==900,"unguided shot at the old launch position can miss during emergence");
    m.units[0].launchStage=0;
    check(m.TargetPos(0,false,tx,ty) && tx==7 && m.TargetDepth(0,false)==3,
          "completed launch returns combat targeting to normal unit coordinates");
    m.units[0]=target;m.units[0].hp=0;m.units[0].launchStep=0;
    maps::Cell ceiling;ceiling.x=8;ceiling.y=8;ceiling.level=4;ceiling.mesh=4352;ceiling.texA=1;
    m.terr.cells.push_back(ceiling);m.EdTouched();m.fx.clear();m.wrecks.clear();
    int hullX,hullY;m.UnitToScreen(m.units[0],hullX,hullY);
    m.Reap();
    bool deathEffect=false;
    for(const auto &e:m.fx) if(e.name=="expdeep") {
        int ex,ey;m.FxScreen(e.x,e.y,e.z,ex,ey);
        deathEffect=e.x==4 && e.y==4 && std::abs(ex-hullX)<=1 && std::abs(ey-hullY)<=1;
    }
    check(m.units.empty() && deathEffect,
          "launch destruction explosion stays at physical hull height even below a roof");
    bool debris=!m.wrecks.empty();
    for(const auto &w:m.wrecks) debris=debris && w.x==4 && w.y==4 && w.z==0;
    check(debris,"launch wreck falls below the hull instead of jumping to the reserved cell or cave roof");
    size_t effectCount=m.fx.size(),wreckCount=m.wrecks.size();m.Reap();
    check(m.fx.size()==effectCount && m.wrecks.size()==wreckCount,
          "reaping the same destroyed launch twice does not duplicate debris");
    m.terr.cells.pop_back();m.EdTouched();m.fx.clear();m.wrecks.clear();
    target.launchStage=0;target.x=4;target.y=4;target.z=3;target.hp=0;
    m.units={target};m.Reap();
    check(!m.fx.empty() && std::fabs(m.fx[0].z-(3+Menu::HOVER))<.001f,
          "ordinary boat destruction also preserves its swimming height above a low seabed");
    std::printf("weaponcheck: %d failures\n", failures);
    return failures ? 1 : 0;
}
