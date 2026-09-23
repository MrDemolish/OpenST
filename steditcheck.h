// Editor integration checks: mutations must survive a file round trip.
inline int RunEditorCheck(bool keepFiles = false)
{
    int failed = 0;
    auto check = [&](bool ok, const char *what) {
        std::printf("editorcheck: %s: %s\n", what, ok ? "OK" : "FAIL");
        if (!ok) ++failed;
    };
    std::string originalDir = g_mapDir;
    g_mapDir = "C:\\ST Reverse\\remake\\native\\editor_tests";
    CreateDirectoryA(g_mapDir.c_str(), nullptr);
    g_clientW = 1280; g_clientH = 860;
    std::set<std::string> lands;
    for (size_t i=0;i<g_menu.skMaps.size();++i) {
        const auto entry=g_menu.skMaps[i];
        if (!lands.insert(entry.texture).second) continue;
        check(g_menu.OpenEditor(int(i)) && !g_menu.edit.contours.empty(),
              ("complete slopes for " + entry.texture).c_str());
        size_t expectedDecor=0;
        for(const auto &name:g_menu.edit.decList) {
            const auto *strip=g_menu.landSet.strip(name);
            expectedDecor+=g_menu.edit.decorAnimated.count(name)?1:strip->count();
        }
        check(g_menu.edit.decChoices.size()==expectedDecor && expectedDecor>g_menu.edit.decList.size(),
              ("all static decoration variants and animated plants for "+entry.texture).c_str());
        std::printf("editorcheck: %s: %zu strips, %zu decoration choices\n",entry.texture.c_str(),
                    g_menu.edit.decList.size(),g_menu.edit.decChoices.size());
    }
    if (!g_menu.OpenEditor(26)) return 1;
    check(g_menu.EdTemplate(maps::OBJ_BUILDING) && g_menu.EdTemplate(maps::OBJ_UNIT)
        && g_menu.EdTemplate(maps::OBJ_RESOURCE), "templates from real records");
    check(!g_menu.EdNewMap("INVALID_DIMENSION", 512, 32, 1, 4352).empty(), "byte coordinate limit");
    std::string name = g_menu.EdFreeName("integration_blank");
    std::string err = g_menu.EdNewMap(name, 64, 96, 1, 4352);
    if (!err.empty()) std::printf("editorcheck: new map error: %s\n", err.c_str());
    std::printf("editorcheck: new map state: %s %dx%d index %d\n", g_menu.terrName.c_str(), g_menu.terr.bw, g_menu.terr.bh, g_menu.edit.mapNo);
    check(err.empty() && g_menu.terr.bw == 32 && g_menu.terr.bh == 48
        && g_menu.edit.on && g_menu.terrName == name, "new map opens from personal directory");
    if (!err.empty()) return 1;
    {
        const auto *exact=g_menu.EdTemplate(maps::OBJ_BUILDING,58);
        check(exact && exact->subtype==58 && exact->raw.size()>=87,
              "blank editor retains a specific gold extractor template");
        if(exact && exact->subtype==58 && exact->raw.size()>=87) {
            const auto source=*exact;
            // A different local building must not hide an exact archived template.
            auto unrelated=source;unrelated.subtype=50;unrelated.x=0;unrelated.y=0;
            ark::wr32(unrelated.raw.data()+16,1);
            g_menu.terr.objects.push_back(unrelated);
            auto selected=g_menu.EdTemplate(maps::OBJ_BUILDING,58);
            check(selected && selected->subtype==58 && selected->raw==source.raw,
                  "exact archived subtype wins over an unrelated local building");
            auto placed=g_menu.EdPlaceObject(2,2,maps::OBJ_BUILDING,58);
            const auto &object=g_menu.terr.objects.back();
            auto bytes=maps::serializeObject(object);
            check(placed.empty() && object.subtype==58 && bytes.size()==source.raw.size()
                  && ark::rd32(bytes.data()+16)==ark::rd32(source.raw.data()+16),
                  "placing and serializing an extractor preserves its own footprint");
            check(g_menu.EdUndo(),"undo removes the subtype-specific placement");
            g_menu.terr.objects.pop_back();g_menu.EdRefreshWorld();
        }
    }
    {
        auto templates=g_menu.edit.templates;
        auto objects=g_menu.terr.objects;
        const auto *prototype=g_menu.EdTemplate(maps::OBJ_BUILDING);
        check(prototype && prototype->raw.size()>=87,"building fallback record available");
        if(prototype && prototype->raw.size()>=87) {
            const auto source=*prototype;
            for(int target:{50,58}) {
                auto fallback=source;
                fallback.subtype=target==50?58:50;
                ark::wr32(fallback.raw.data()+16,target==50?0:1);
                g_menu.edit.templates={fallback};
                g_menu.terr.objects.clear();
                auto placed=g_menu.EdPlaceObject(2,2,maps::OBJ_BUILDING,target);
                const int span=target==50?2:1;
                check(placed.empty() && g_menu.terr.objects.size()==1
                      && ark::rd32(maps::serializeObject(g_menu.terr.objects.back()).data()+16)==uint32_t(span-1)
                      && g_menu.blds.size()==1 && g_menu.blds.front().span==span,
                      target==50?"small fallback creates a large base":"large fallback creates a small extractor");
                check(g_menu.EdUndo() && g_menu.terr.objects.empty()
                      && g_menu.EdRedo() && g_menu.blds.size()==1 && g_menu.blds.front().span==span,
                      "fallback footprint survives undo and redo");
            }
        }
        g_menu.edit.templates=std::move(templates);
        g_menu.terr.objects=std::move(objects);
        g_menu.EdRefreshWorld();
    }
    check(g_menu.edit.texList.size() > 100 && g_menu.edit.texGroups.size() > 10
        && !g_menu.edit.decList.empty(), "blank map offers full land palette");
    std::printf("editorcheck: palette %zu textures, %zu families, %zu plants, %zu contour families\n",
        g_menu.edit.texList.size(),g_menu.edit.texGroups.size(),g_menu.edit.decList.size(),g_menu.edit.contours.size());
    check(!g_menu.edit.contours.empty(), "original slope pairs discovered automatically");
    auto flat = maps::serializeCells(g_menu.terr);
    g_menu.EdPushUndo();
    check(g_menu.EdRaise(16,16), "raise isolated mountain");
    int slopes = 0;
    for (const auto &c:g_menu.terr.cells)
        if (c.level==1 && g_menu.landSet.mesh(c.mesh)->minZ < -9.f) ++slopes;
    check(slopes == 8 && g_menu.terr.topAt(16,16)==1 && g_menu.terr.topAt(13,13)==0,
        "mountain has eight original slopes and untouched surroundings");
    auto mountain = maps::serializeCells(g_menu.terr);
    check(g_menu.EdUndo() && maps::serializeCells(g_menu.terr)==flat
        && g_menu.EdRedo() && maps::serializeCells(g_menu.terr)==mountain,
        "undo and redo restore all mountain neighbours");
    check(g_menu.EdRaise(16,16) && g_menu.terr.topAt(16,16)==2, "raise mountain twice");
    check(g_menu.EdLower(16,16) && g_menu.terr.topAt(16,16)==1,
        "lower sparse plateau without deleting its floor");
    // Compare shared corners of every top surface, independently of sculpting.
    std::map<std::pair<int,int>,float> corners;
    bool stitched = true;
    for (const auto &c:g_menu.terr.cells) {
        if (c.level != g_menu.terr.topAt(c.x/2,c.y/2)) continue;
        auto zs = land::Set::corners(*g_menu.landSet.mesh(c.mesh));
        int dx[4]={0,1,1,0},dy[4]={0,0,1,1};
        for(int k=0;k<4;++k) {
            auto key=std::make_pair(c.x/2+dx[k],c.y/2+dy[k]);
            float z=c.level*10.f+zs[k];
            auto old=corners.find(key);
            if(old!=corners.end() && std::fabs(old->second-z)>.15f) stitched=false;
            corners[key]=z;
        }
    }
    check(stitched,"sculpted neighbouring corners join without vertical holes");
    g_menu.edit.grid=false; g_menu.ApplyZoom(4); g_menu.CentreCamera();
    g_menu.FitCanvas(); g_menu.Compose();
    if (FILE *f=std::fopen("editor_mountain.raw","wb")) {
        std::fwrite(g_menu.canvas.data(),4,g_menu.canvas.size(),f); std::fclose(f);
    }
    maps::Decor plant; plant.x=3300; plant.y=3300; plant.z=100;
    auto pose=g_menu.DecorPosition(plant);
    check(std::fabs(pose.x-330.f)<.001f && std::fabs(pose.z-12.966f)<.001f && pose.hotY==61,
        "plant uses original XYZ and static hot spot");
    size_t count = g_menu.terr.objects.size();
    check(g_menu.EdPlaceObject(4, 4, maps::OBJ_BUILDING, 50).empty(), "first building on blank map");
    auto building = maps::serializeObject(g_menu.terr.objects.back());
    check(g_menu.EdUndo() && g_menu.terr.objects.size() == count, "undo object placement");
    check(g_menu.EdRedo() && maps::serializeObject(g_menu.terr.objects.back()) == building, "redo preserves object bytes");
    g_menu.edit.selKind = ed::SEL_OBJ;
    g_menu.edit.selIdx = int(g_menu.terr.objects.size()) - 1;
    check(g_menu.EdMoveSelection(5, 5) && g_menu.terr.objects.back().x == 10, "move selection");
    check(g_menu.EdUndo() && maps::serializeObject(g_menu.terr.objects.back()) == building, "undo move");
    g_menu.edit.selKind = ed::SEL_OBJ;
    g_menu.edit.selIdx = int(g_menu.terr.objects.size()) - 1;
    g_menu.EdDeleteSelection();
    check(g_menu.terr.objects.size() == count && g_menu.EdUndo()
        && maps::serializeObject(g_menu.terr.objects.back()) == building, "undo deletion");
    g_menu.edit.objectSide = 1;
    g_menu.edit.placeOwner = 1;
    check(g_menu.EdPlaceObject(8, 8, maps::OBJ_UNIT, 24).empty(), "first boat on blank map");
    check(ark::rd16(&g_menu.terr.objects.back().raw[34]) == 0xffff
        && ark::rd16(&g_menu.terr.objects.back().raw[36]) == 0xfffe,
        "new boat requests free identity and controllable group in original game");
    check(g_menu.EdPlaceObject(10, 10, maps::OBJ_RESOURCE, 221).empty(), "first deposit on blank map");
    size_t decorCount = g_menu.terr.decor.size();
    check(!g_menu.edit.decList.empty()
        && g_menu.EdPlaceDecor(12, 12, "coral",15).empty(), "last coral variant can be placed on a blank map");
    check(g_menu.EdUndo() && g_menu.terr.decor.size() == decorCount
        && g_menu.EdRedo() && g_menu.terr.decor.size() == decorCount + 1, "undo and redo decoration");
    auto beforeSlots = g_menu.edit.slots;
    g_menu.EdPushUndo();
    g_menu.edit.slots[0].x = 22;
    g_menu.edit.slots[0].race = 3;
    check(g_menu.EdUndo() && g_menu.edit.slots[0].x == beforeSlots[0].x
        && g_menu.edit.slots[0].race == beforeSlots[0].race, "undo player properties");
    check(g_menu.EdRedo() && g_menu.edit.slots[0].x == 22 && g_menu.edit.slots[0].race == 3, "redo player properties");
    std::string saved = g_menu.EdFreeName("integration_saved");
    g_menu.edit.platformLevel=3;g_menu.edit.brush=1;
    check(g_menu.EdPlatform(24,24,false)>0 && g_menu.WaterAt(24,24,0)
        && !g_menu.WaterAt(24,24,2),"elevated platform before saving");
    err = g_menu.EdSaveAs(saved);
    check(err.empty(), "save blank map with all added classes");
    maps::Terrain loaded;
    maps::Entry entry;
    bool found = false;
    for (const auto &e : maps::scan(g_mapDir)) if (e.file == saved) { entry = e; found = true; }
    bool read = found && maps::loadTerrain(entry, loaded);
    check(read && maps::serializeCells(loaded) == maps::serializeCells(g_menu.terr), "terrain file round trip");
    check(read && loaded.hasLevel(24,24,0) && loaded.hasLevel(24,24,3)
        && !loaded.hasLevel(24,24,1) && !loaded.hasLevel(24,24,2),"saved platform retains the space below it");
    std::vector<std::vector<uint8_t>> expected, actual;
    for (const auto &o : g_menu.terr.objects) if (!o.spawned) expected.push_back(maps::serializeObject(o));
    for (const auto &o : loaded.objects) if (!o.spawned) actual.push_back(maps::serializeObject(o));
    std::sort(expected.begin(), expected.end()); std::sort(actual.begin(), actual.end());
    check(read && expected == actual && expected.size() == 3, "all object bytes survive reload");
    check(read && maps::serializeDecor(loaded.decor, g_menu.edit.decorSource)
        == maps::serializeDecor(g_menu.terr.decor, g_menu.edit.decorSource)
        && loaded.decor.size() == 1 && loaded.decor[0].name=="coral" && loaded.decor[0].frame==15,
          "selected decoration variant and complete record survive reload");
    check(read && entry.slots[0].x == 22 && entry.slots[0].race == 3, "player properties survive reload");
    check(!g_menu.EdSaveAs(saved).empty(), "existing archive cannot be overwritten");
    check(g_menu.EdPlaceObject(14, 14, maps::OBJ_SHARK, 230).empty()
        && g_menu.EdPlaceObject(16, 16, maps::OBJ_VOLCANO, 1).empty()
        && g_menu.EdPlaceObject(18, 18, maps::OBJ_MINE, 166).empty(), "shark volcano and mine on blank map");
    std::string worldSaved = g_menu.EdFreeName("integration_world");
    err = g_menu.EdSaveAs(worldSaved);
    expected.clear(); actual.clear();
    for (const auto &o : g_menu.terr.objects) if (!o.spawned) expected.push_back(maps::serializeObject(o));
    for (const auto &e : maps::scan(g_mapDir)) if (e.file == worldSaved && maps::loadTerrain(e, loaded))
        for (const auto &o : loaded.objects) if (!o.spawned) actual.push_back(maps::serializeObject(o));
    std::sort(expected.begin(), expected.end()); std::sort(actual.begin(), actual.end());
    check(err.empty() && expected == actual && expected.size() == 6, "world object classes survive reload");
    g_menu.edit.dirty = true;
    g_menu.EdRequest(3);
    check(g_menu.screen == SCR_EDITOR && g_menu.edit.dialog == 3, "unsaved exit requests a decision");
    g_menu.EditorKey(VK_ESCAPE);
    check(g_menu.screen == SCR_EDITOR && g_menu.edit.dirty && !g_menu.edit.dialog, "cancel preserves unsaved work");
    g_menu.edit.tool = ed::T_OBJECT;
    g_menu.edit.objectSide = 2;
    g_menu.edit.objectMode = 1;
    g_menu.EdBuildObjectPalette();
    check(g_menu.edit.objList.size() == 13 && g_menu.edit.objList.front() == 25
        && g_menu.edit.objList.back() == 40, "Silicon boat palette");
    g_clientH = 520;
    g_menu.edit.objectMode = 0; g_menu.EdBuildObjectPalette();
    g_menu.edit.selKind = ed::SEL_NIC;
    g_menu.FitCanvas(); g_menu.ComposeEditor();
    ed::Layout layout = g_menu.EdLayout();
    int w, h, cols; g_menu.EdPalCell(w, h, cols);
    int last = int(g_menu.edit.objList.size()) - 1;
    RECT cell = g_menu.EdPalSlot(layout, last, w, h, cols);
    check(g_menu.EdPalHit(layout, (cell.left+cell.right)/2, (cell.top+cell.bottom)/2) == -1,
        "hidden palette cell cannot be clicked");
    g_menu.mouse = POINT{layout.insp.left + 16, g_menu.edit.palTop + 8};
    for (int i = 0; i < 20; ++i) g_menu.EditorWheel(-1);
    cell = g_menu.EdPalSlot(layout, last, w, h, cols);
    check(g_menu.edit.palScroll > 0 && g_menu.EdPalHit(layout, (cell.left+cell.right)/2,
        (cell.top+cell.bottom)/2) == last, "scroll reaches final palette cell");
    g_clientH = 860;
    auto click = [&](RECT r) { g_menu.EditorClick((r.left+r.right)/2, (r.top+r.bottom)/2, false); };
    g_menu.FitCanvas(); g_menu.ComposeEditor();
    click(g_menu.EdChoiceRect(g_menu.edit.objectModeRect, 3, 3));
    check(g_menu.edit.objectMode == 3, "mouse selects volcano category directly");
    g_menu.ComposeEditor();
    click(g_menu.EdChoiceRect(g_menu.edit.objectModeRect, 1, 3));
    g_menu.ComposeEditor();
    click(g_menu.EdChoiceRect(g_menu.edit.objectSideRect, 0, 3));
    check(g_menu.edit.objectMode == 1 && g_menu.edit.objectSide == 0, "mouse selects boat race directly");
    g_menu.edit.tool = ed::T_RAISE;
    g_menu.ComposeEditor();
    int brush = g_menu.edit.brush;
    click(g_menu.edit.controlRect[7]);
    check(g_menu.edit.brush == brush + 1, "mouse increases brush size");
    click(g_menu.edit.controlRect[6]);
    click(g_menu.edit.controlRect[9]);
    check(g_menu.edit.brush == brush && !g_menu.edit.brushRound, "mouse decreases size and selects square");
    g_menu.edit.dialog = 2;
    click(g_menu.EdChoiceRect(g_menu.EdDialogButton(1), 2, 4));
    click(g_menu.EdChoiceRect(g_menu.EdDialogButton(3), 0, 4));
    check(g_menu.edit.newW == 128 && g_menu.edit.newH == 32, "mouse chooses new map dimensions directly");
    click(g_menu.EdDialogCloseRect());
    check(!g_menu.edit.dialog && g_menu.edit.on, "visible close button preserves editor");
    g_menu.ComposeEditor();
    click(g_menu.edit.controlRect[0]);
    click(g_menu.edit.controlRect[1]);
    check(g_menu.terr.objects.size() == 6, "toolbar undo and redo restore last object");
    g_menu.edit.tool = ed::T_OBJECT;
    g_menu.edit.objectMode = 1; g_menu.EdBuildObjectPalette();
    g_menu.edit.view = ed::V_FLAT;
    g_menu.FitCanvas(); g_menu.ComposeEditor();
    auto dump = [&](const char *filename) {
        g_menu.ComposeEditor();
        FILE *out = std::fopen(filename, "wb");
        if (out) { std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), out); std::fclose(out); }
    };
    dump("editor_objects.raw");
    g_menu.edit.dialog = 1; dump("editor_open.raw");
    g_menu.edit.dialog = 2; dump("editor_new.raw");
    g_menu.edit.dialog = 3; dump("editor_unsaved.raw");
    g_menu.edit.selKind = ed::SEL_OBJ; g_menu.edit.selIdx = 0;
    g_menu.edit.dialog = 4; dump("editor_properties.raw");
    g_menu.edit.dialog=0;g_menu.edit.tool=ed::T_TEXTURE;g_menu.edit.palScroll=0;
    g_menu.ComposeEditor();click(g_menu.edit.controlRect[12]);
    check(g_menu.edit.texAll && g_menu.EdPalCount()==int(g_menu.landSet.textureIds().size()),
          "full texture palette exposes every Land texture");
    bool groundOnly=true;std::set<uint16_t> varied;
    for(const auto &group:g_menu.edit.texGroups) for(const auto &v:group.variants) {
        auto known=g_menu.edit.textureMeshes.find(uint16_t((group.group<<8)|v.first));
        if(known!=g_menu.edit.textureMeshes.end() && g_menu.landSet.mesh(known->second)->minZ<-.2f) groundOnly=false;
    }
    for(const auto &group:g_menu.edit.texGroups) if(group.variants.size()>1) {
        g_menu.edit.texGroup=group.group;g_menu.edit.texMode=ed::TEX_WARIANTY;
        for(int y=0;y<32;++y) for(int x=0;x<32;++x) varied.insert(g_menu.EdPickTile(x,y));
        break;
    }
    check(groundOnly && varied.size()>1,"random terrain varies textures without sampling known rock edges");
    dump("editor_textures.raw");
    g_menu.edit.tool=ed::T_DECOR;g_menu.edit.palScroll=0;g_menu.ComposeEditor();
    int coralChoice=-1;
    for(size_t i=0;i<g_menu.edit.decChoices.size();++i)
        if(g_menu.edit.decChoices[i].name=="coral" && g_menu.edit.decChoices[i].frame==15) coralChoice=int(i);
    check(coralChoice>=0,"last coral variant is visible in the palette");
    if(coralChoice>=0) {
        g_menu.edit.palScroll=coralChoice/6;g_menu.ComposeEditor();
        click(g_menu.EdPalSlot(g_menu.EdLayout(),coralChoice,40,40,6));
        check(g_menu.edit.placeDecor=="coral" && g_menu.edit.placeDecorFrame==15,
              "mouse selects the exact decoration variant shown in its thumbnail");
    }
    g_menu.edit.palScroll=0;g_menu.ComposeEditor();dump("editor_decor_variants.raw");
    click(g_menu.edit.controlRect[14]);
    check(g_menu.edit.dialog==5,"mouse opens vegetation generator");
    click(g_menu.EdChoiceRect(g_menu.EdDialogButton(1),2,4));
    check(g_menu.edit.scatterDensity==50,"mouse selects vegetation density");
    dump("editor_scatter_dialog.raw");
    auto originalPlants=maps::serializeDecor(g_menu.terr.decor,g_menu.edit.decorSource);
    size_t originalCount=g_menu.terr.decor.size();
    click(g_menu.EdDialogButton(7));
    auto scattered=maps::serializeDecor(g_menu.terr.decor,g_menu.edit.decorSource);
    bool placed=g_menu.terr.decor.size()>originalCount;
    std::set<std::pair<std::string,int32_t>> generatedChoices;
    bool grounded=true;
    for(size_t i=originalCount;i<g_menu.terr.decor.size();++i) {
        const auto &d=g_menu.terr.decor[i];
        generatedChoices.insert({d.name,d.frame});
        int x=d.x/(2*maps::WORLD_PER_CELL),y=d.y/(2*maps::WORLD_PER_CELL),z=d.z/maps::WORLD_PER_LEVEL;
        grounded=grounded && g_menu.terr.hasLevel(x,y,z)
            && g_menu.landSet.mesh(g_menu.terr.meshId[(size_t(z)*g_menu.terr.bh+y)*g_menu.terr.bw+x])->minZ>=-.2f;
        for(const auto &o:g_menu.terr.objects) if(!o.spawned)
            grounded=grounded && (std::abs(o.x/2-x)>1 || std::abs(o.y/2-y)>1);
    }
    check(placed && grounded,"generated plants rest on flat terrain and avoid objects");
    check(generatedChoices.size()>g_menu.edit.decList.size(),"scatter uses multiple static variants, not one plant per strip");
    check(g_menu.EdUndo() && maps::serializeDecor(g_menu.terr.decor,g_menu.edit.decorSource)==originalPlants,
          "one undo removes the entire scatter and preserves existing plants");
    check(g_menu.EdScatterDecor()>0 && maps::serializeDecor(g_menu.terr.decor,g_menu.edit.decorSource)==scattered,
          "same seed recreates exactly the same plants");
    std::string scatterSaved=g_menu.EdFreeName("integration_plants");
    err=g_menu.EdSaveAs(scatterSaved);bool plantsRead=false;
    for(const auto &e:maps::scan(g_mapDir)) if(e.file==scatterSaved) plantsRead=maps::loadTerrain(e,loaded);
    check(err.empty() && plantsRead && maps::serializeDecor(loaded.decor,g_menu.edit.decorSource)==scattered,
          "generated plant coordinates and complete records survive file round trip");
    g_menu.edit.view=ed::V_ISO;g_menu.ApplyZoom(3);g_menu.EdCentre();dump("editor_scattered.raw");
    // Remove only the two files created under our isolated test directory.
    if (!keepFiles) for (const auto &base : {name, saved, worldSaved, scatterSaved}) {
        std::string dkx, dkd; g_menu.EdTargets(base, dkx, dkd);
        std::remove(dkx.c_str()); std::remove(dkd.c_str());
    }
    g_mapDir = originalDir;
    std::printf("editorcheck: %d failures\n", failed);
    return failed ? 1 : 0;
}
