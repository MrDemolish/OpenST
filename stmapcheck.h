// Load through the same StepLoad/OpenTerrain path as a match, then inspect
// live state and render/simulate it. Independent archive checks live in Python.
inline int RunMapSaveCheck()
{
    int failures=0,checked=0;
    std::string savedDir=g_mapDir;
    g_mapDir="C:\\ST Reverse\\work\\editor_roundtrip";
    CreateDirectoryA(g_mapDir.c_str(),nullptr);
    for (size_t i=0;i<g_menu.skMaps.size();++i) {
        const auto entry=g_menu.skMaps[i];
        ark::Archive source;
        if (!source.open(entry.dkx,entry.dkd,true)) { ++failures; continue; }
        auto desc=source.read("DESCRIPTOR");
        if (desc.size()!=6553 || ark::rd32(desc.data())!=1) continue;
        std::string name=g_menu.EdFreeName(entry.file);
        if (!g_menu.OpenEditor(int(i))) {
            ++failures; std::printf("mapsavecheck: %s cannot open editor\n",entry.file.c_str()); continue;
        }
        auto err=g_menu.EdSaveAs(name);
        std::string x,d;g_menu.EdTargets(name,x,d);
        ark::Archive output;
        if (!err.empty() || !output.open(x,d,true)) {
            ++failures; std::printf("mapsavecheck: %s: %s (%s)\n",entry.file.c_str(),
                err.empty()?"cannot reopen saved archive":err.c_str(),x.c_str()); continue;
        }
        if (source.names()!=output.names()) {
            ++failures; std::printf("mapsavecheck: %s lost record keys\n",entry.file.c_str());
        }
        for (const auto &key:source.names()) {
            if (key=="SMALL_MAP") continue; // intentionally regenerated
            if (source.read(key)!=output.read(key)) {
                ++failures; std::printf("mapsavecheck: %s changed %s\n",entry.file.c_str(),key.c_str());
            }
        }
        ++checked;
    }
    g_mapDir=savedDir;
    std::printf("mapsavecheck: %d maps saved by editor, %d unexpected changes\n",checked,failures);
    return failures?1:0;
}

inline int RunMapCheck()
{
    int failures = 0, checked = 0;
    size_t cells = 0, objects = 0, decorations = 0;
    g_clientW = 1280; g_clientH = 860;
    for (size_t i = 0; i < g_menu.skMaps.size(); ++i) {
        const maps::Entry entry = g_menu.skMaps[i];
        ark::Archive ar;
        if (!ar.open(entry.dkx,entry.dkd,true)) { ++failures; continue; }
        auto desc=ar.read("DESCRIPTOR");
        // Old broken exports are covered by --maprepair, not valid fixtures.
        if (desc.size()!=6553 || ark::rd32(desc.data())!=1) continue;
        int before=failures;
        auto check=[&](bool ok, const char *what) {
            if (!ok) { ++failures; std::printf("mapcheck: %s: FAIL %s\n",entry.file.c_str(),what); }
        };
        g_menu.skSel=int(i); g_menu.OpenLoad();
        DWORD now=1000;
        for (int k=0;k<10 && g_menu.screen==SCR_LOAD;++k) g_menu.StepLoad(now+=100);
        check(g_menu.screen==SCR_TERRAIN || g_menu.screen==SCR_BRIEF,"game loading screen");
        check(maps::serializeCells(g_menu.terr)==ar.read("3D_MAP"),"terrain bytes / XY / levels");
        int layers=0; for (int n:g_menu.terr.perLevel) layers+=n;
        check(size_t(layers)==g_menu.terr.cells.size(),"layer counters reset between matches");
        check(g_menu.landSet.ok() && g_menu.landName==entry.texture,"texture set");
        std::set<uint16_t> meshes,textures;
        for (const auto &c:g_menu.terr.cells) {
            meshes.insert(c.mesh); textures.insert(c.texA);
            if (c.texB) textures.insert(c.texB);
            check(g_menu.terr.meshAt(c.x/2,c.y/2,c.level)==c.mesh
                && g_menu.terr.texAt(c.x/2,c.y/2,c.level)==c.texA
                && g_menu.terr.tex2At(c.x/2,c.y/2,c.level)==c.texB,"live terrain slot");
        }
        for (auto m:meshes) check(g_menu.landSet.mesh(m)!=nullptr,"missing terrain mesh");
        for (auto t:textures) check(g_menu.landSet.tile(t)!=nullptr,"missing terrain texture");
        size_t unit=0,bld=0;
        for (const auto &o:g_menu.terr.objects) {
            if (!o.spawned) {
                ++objects;
                check(maps::serializeObject(o)==ar.read(o.rec),"object fields preserved");
            }
            if (o.type==maps::OBJ_UNIT && !o.spawned) {
                check(unit<g_menu.units.size(),"missing live boat");
                if (unit<g_menu.units.size()) {
                    const auto &u=g_menu.units[unit++];
                    check(u.type==o.subtype && u.owner==o.owner && u.x==o.x && u.y==o.y
                        && u.z==o.z,"boat type / owner / XYZ");
                    auto bytes=ar.read(o.rec);
                    if(bytes.size()>=105 && ark::rd32(bytes.data()+12)==0) {
                        int percent=int32_t(ark::rd32(bytes.data()+38));
                        int hp=percent<0?1:(percent>=100?u.hpMax:u.hpMax*percent/100);
                        check(u.hp==hp,"boat HP from archived signed DWORD percentage");
                        int c=int32_t(ark::rd32(bytes.data()+42)),m=int32_t(ark::rd32(bytes.data()+46));
                        c=c<0?0:c>120?40:c/3;m=m<0?0:m>800?40:m/20;
                        if(c+m>40) { c=c*40/(c+m);m=40-c; }
                        check(u.cargo.corium==c*3 && u.cargo.metal==m*20,
                              "boat mixed cargo from archived signed DWORD amounts");
                    }
                }
            }
            if (o.type==maps::OBJ_BUILDING) {
                check(bld<g_menu.blds.size(),"missing live building");
                if (bld<g_menu.blds.size()) {
                    const auto &b=g_menu.blds[bld++];
                    check(b.tobj==o.subtype && b.owner==o.owner && b.x==o.x && b.y==o.y
                        && g_menu.BldLevel(b)==o.z,"building type / owner / XYZ");
                    const auto bytes=ar.read(o.rec);
                    if(bytes.size()>=20)
                        check(b.span==(ark::rd32(bytes.data()+16)?2:1),
                              "building footprint from archived common size field");
                    if(bytes.size()>=87 && (ark::rd32(bytes.data()+12)==0
                        || ark::rd32(bytes.data()+12)==1 || ark::rd32(bytes.data()+12)==3)) {
                        int hp=int32_t(ark::rd32(bytes.data()+63));
                        int energy=int32_t(ark::rd32(bytes.data()+67));
                        if(hp<0 || hp>100) hp=100;
                        if(energy<0 || energy>100) energy=100;
                        check(b.hp==b.hpMax*hp/100 && b.energyPercent==energy,
                              "building HP / energy from archived signed DWORD percentages");
                    }
                }
            }
        }
        if (!g_menu.terr.decorRec.empty()) {
            auto bytes = ar.read(g_menu.terr.decorRec);
            check(bytes.size() >= 24 && g_menu.terr.decor.size()==ark::rd32(bytes.data()+20),
                  "decoration count from original +20 field");
            if (bytes.size() >= 24) {
                check(maps::serializeDecor(g_menu.terr.decor,bytes)==bytes,
                      "decoration header, entries and spare tail preserved");
                for(size_t j=0;j<g_menu.terr.decor.size();++j) {
                    const auto &d=g_menu.terr.decor[j]; size_t p=24+j*146;
                    check(p+146<=bytes.size() && d.x==ark::rd16(&bytes[p])
                        && d.y==ark::rd16(&bytes[p+2]) && d.z==ark::rd16(&bytes[p+4])
                        && d.frame==int32_t(ark::rd32(&bytes[p+134])), "original vegetation entry position");
                }
            }
        }
        cells+=g_menu.terr.cells.size(); decorations+=g_menu.terr.decor.size();
        g_menu.screen=SCR_TERRAIN; g_menu.ApplyZoom(4); g_menu.CentreCamera();
        g_menu.FitCanvas(); g_menu.Compose();
        g_menu.lastUnitStep=now;
        for (int k=0;k<20;++k) g_menu.StepUnits(now+=100);
        g_menu.Compose();
        check(g_menu.canvas.size()==size_t(SCREEN_W)*SCREEN_H,"game frame after simulation");
        std::printf("mapcheck: %s: %s\n",entry.file.c_str(),before==failures?"OK":"FAIL");
        std::fflush(stdout); ++checked;
    }
    std::printf("mapcheck: %d maps, %zu cells, %zu objects, %zu decorations; %d failures\n",
        checked,cells,objects,decorations,failures);
    return failures?1:0;
}
