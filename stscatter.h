// Included inside Menu. Deterministic vegetation placement on actual flat sheets.
    static uint32_t EdRandom(uint32_t value)
    {
        value^=value>>16;value*=0x7feb352du;value^=value>>15;
        value*=0x846ca68bu;return value^(value>>16);
    }

    int EdScatterDecor()
    {
        std::vector<maps::Decor> templates;
        for(const auto &choice:edit.decChoices) {
            if(!edit.scatterMixed && (choice.name!=edit.placeDecor || choice.frame!=edit.placeDecorFrame)) continue;
            for(const auto &d:edit.decorTemplates) if(d.name==choice.name && d.raw.size()==maps::DECOR_STRIDE) {
                const auto *strip=landSet.strip(choice.name);
                if(strip && strip->count()) { templates.push_back(d);templates.back().frame=choice.frame; }
                break;
            }
        }
        if(templates.empty()) {edit.lastSave="wybierz rosline z palety";return 0;}
        int W=terr.bw,H=terr.bh,spacing=std::clamp(edit.scatterSpacing,1,3);
        std::vector<uint8_t> occupied(size_t(W)*H*maps::LEVELS,0);
        auto mark=[&](int x,int y,int z) {
            if(z<0 || z>=maps::LEVELS) return;
            for(int yy=std::max(0,y-spacing);yy<=std::min(H-1,y+spacing);++yy)
                for(int xx=std::max(0,x-spacing);xx<=std::min(W-1,x+spacing);++xx)
                    occupied[(size_t(z)*H+yy)*W+xx]=1;
        };
        for(const auto &d:terr.decor)
            mark(d.x/(2*maps::WORLD_PER_CELL),d.y/(2*maps::WORLD_PER_CELL),
                 int(std::lround(float(d.z)/maps::WORLD_PER_LEVEL)));
        // Keep room around every object and player start, regardless of depth.
        for(const auto &o:terr.objects) if(!o.spawned)
            for(int z=0;z<maps::LEVELS;++z) mark(o.x/2,o.y/2,z);
        for(const auto &s:edit.slots) if(s.id!=0xff)
            for(int z=0;z<maps::LEVELS;++z) mark(s.x/2,s.y/2,z);
        struct Candidate { uint32_t rank; int x,y,z; };
        std::vector<Candidate> candidates;
        for(int y=0;y<H;++y) for(int x=0;x<W;++x) {
            int z=edit.layer>=0?edit.layer:terr.topAt(x,y);
            if(z<0 || !terr.hasLevel(x,y,z)) continue;
            const auto *mesh=landSet.mesh(terr.meshId[(size_t(z)*H+y)*W+x]);
            // Slopes need surface sampling; until then never plant above an abyss.
            if(!mesh || mesh->minZ<-.2f) continue;
            uint32_t h=EdRandom(edit.scatterSeed ^ uint32_t(x)*73856093u ^ uint32_t(y)*19349663u);
            if(h%100>=uint32_t(std::clamp(edit.scatterDensity,0,100))) continue;
            candidates.push_back({h,x,y,z});
        }
        std::sort(candidates.begin(),candidates.end(),[](const auto&a,const auto&b){return a.rank<b.rank;});
        int count=0;
        for(const auto &c:candidates) {
            if(occupied[(size_t(c.z)*H+c.y)*W+c.x]) continue;
            if(!count) EdPushUndo();
            maps::Decor d=templates[EdRandom(c.rank+17u)%templates.size()];
            // A jitter inside the block avoids a visible planting grid.
            d.x=int16_t(c.x*2*maps::WORLD_PER_CELL+30+EdRandom(c.rank+31u)%140);
            d.y=int16_t(c.y*2*maps::WORLD_PER_CELL+30+EdRandom(c.rank+47u)%140);
            d.z=int16_t(c.z*maps::WORLD_PER_LEVEL);
            terr.decor.push_back(std::move(d));mark(c.x,c.y,c.z);++count;
        }
        if(count) {decorOrder.clear();edit.dirty=edit.dirtyObj=true;}
        edit.lastSave="dodano roslin: "+std::to_string(count)+"; Z cofa caly rozsiew";
        return count;
    }

    void EdDrawScatterDialog()
    {
        RECT r=EdDialogRect();char text[128];
        TextAt(fontGrey,r.left+16,EdDialogButton(0).top+5,"GESTOSC ROSLIN");
        const int densities[]={5,20,50,80};
        for(int i=0;i<4;++i) {
            std::snprintf(text,sizeof(text),"%d %%",densities[i]);
            EdButton(EdChoiceRect(EdDialogButton(1),i,4),text,edit.scatterDensity==densities[i]?2:0);
        }
        TextAt(fontGrey,r.left+16,EdDialogButton(2).top+5,"MINIMALNY ODSTEP (bloki)");
        for(int i=0;i<3;++i) {
            std::snprintf(text,sizeof(text),"%d",i+1);
            EdButton(EdChoiceRect(EdDialogButton(3),i,3),text,edit.scatterSpacing==i+1?2:0);
        }
        RECT seed=EdDialogButton(4);
        std::snprintf(text,sizeof(text),"ZIARNO: %u",edit.scatterSeed);
        TextAt(fontCyan,seed.left,seed.top+5,text);
        seed.left=seed.right-180;
        EdButton(EdChoiceRect(seed,0,2),"-",0);EdButton(EdChoiceRect(seed,1,2),"+",0);
        std::string scope=edit.layer<0?"OBSZAR: wierzch mapy":"OBSZAR: warstwa "+std::to_string(edit.layer);
        TextAt(fontGrey,r.left+16,EdDialogButton(5).top+5,scope.c_str());
        EdButton(EdDialogButton(6),edit.scatterMixed?"GATUNKI: MIESZANKA  (klik: wybrana roslina)":"GATUNKI: WYBRANA ROSLINA  (klik: mieszanka)",0);
        EdButton(EdDialogButton(7),"ROZSIEJ ROSLINY",0);
        TextAt(fontGrey,r.left+16,EdDialogButton(8).top+4,"Istniejace rosliny pozostaja. Z cofa caly rozsiew.");
        TextAt(fontGrey,r.left+16,EdDialogButton(9).top+4,"Puste warstwy, zbocza i okolice obiektow sa pomijane.");
    }

    void EdScatterClick(int x,int y)
    {
        const int densities[]={5,20,50,80};
        for(int i=0;i<4;++i) if(EdContains(EdChoiceRect(EdDialogButton(1),i,4),x,y)) edit.scatterDensity=densities[i];
        for(int i=0;i<3;++i) if(EdContains(EdChoiceRect(EdDialogButton(3),i,3),x,y)) edit.scatterSpacing=i+1;
        RECT seed=EdDialogButton(4);seed.left=seed.right-180;
        if(EdContains(EdChoiceRect(seed,0,2),x,y) && edit.scatterSeed>1) --edit.scatterSeed;
        if(EdContains(EdChoiceRect(seed,1,2),x,y) && edit.scatterSeed<999999) ++edit.scatterSeed;
        if(EdContains(EdDialogButton(6),x,y)) edit.scatterMixed=!edit.scatterMixed;
        if(EdContains(EdDialogButton(7),x,y)) {EdScatterDecor();edit.dialog=0;}
    }
