// Included inside Menu. A platform edits one sheet, never the volume below it.
    int EdPlatform(int bx, int by, bool erase)
    {
        if (bx<0 || by<0 || bx>=terr.bw || by>=terr.bh) return 0;
        auto patterns=edit.contours.find(edit.texGroup);
        if (patterns==edit.contours.end()) patterns=edit.contours.find(edit.contourFamily);
        if (patterns==edit.contours.end()) { edit.lastSave="brak kompletu siatek zboczy"; return 0; }
        int level=std::clamp(edit.platformLevel,1,5), w=terr.bw+1;
        const int dx[4]={0,1,1,0}, dy[4]={0,0,1,1};
        std::vector<uint8_t> high(size_t(w)*(terr.bh+1),0), touched(high.size(),0);
        std::vector<int> indices(size_t(terr.bw)*terr.bh,-1);
        for (size_t i=0;i<terr.cells.size();++i) {
            const auto &c=terr.cells[i];
            if(c.level!=level) continue;
            int x=c.x/2,y=c.y/2;
            indices[size_t(y)*terr.bw+x]=int(i);
            const auto *mesh=landSet.mesh(c.mesh);
            if(!mesh || !mesh->ok()) continue;
            auto z=land::Set::corners(*mesh);
            for(int k=0;k<4;++k) if(z[k]>-.15f)
                high[size_t(y+dy[k])*w+x+dx[k]]=1;
        }
        for(int y=std::max(0,by-edit.brush);y<=std::min(terr.bh-1,by+edit.brush);++y)
            for(int x=std::max(0,bx-edit.brush);x<=std::min(terr.bw-1,bx+edit.brush);++x) {
                if(!EdInBrush(x-bx,y-by)) continue;
                for(int k=0;k<4;++k) {
                    size_t i=size_t(y+dy[k])*w+x+dx[k];
                    high[i]=erase?0:1; touched[i]=1;
                }
            }
        std::vector<uint8_t> remove(terr.cells.size(),0);
        int changed=0;
        for(int y=0;y<terr.bh;++y) for(int x=0;x<terr.bw;++x) {
            int mask=0;bool editHere=false;
            for(int k=0;k<4;++k) {
                size_t i=size_t(y+dy[k])*w+x+dx[k];
                if(high[i]) mask|=1<<k;
                editHere=editHere || touched[i];
            }
            if(!editHere) continue;
            int at=indices[size_t(y)*terr.bw+x];
            if(!mask) { if(at>=0) { remove[size_t(at)]=1; ++changed; } continue; }
            maps::Cell c=patterns->second[size_t(mask)];
            c.x=uint8_t(x*2);c.y=uint8_t(y*2);c.level=uint8_t(level);
            if(at>=0) {
                auto &old=terr.cells[size_t(at)];
                if(old.mesh==c.mesh && old.texA==c.texA && old.texB==c.texB) continue;
                old=c;
            } else { terr.cells.push_back(c); remove.push_back(0); }
            ++changed;
        }
        if(changed) {
            size_t out=0;
            for(size_t i=0;i<terr.cells.size();++i) if(!remove[i]) terr.cells[out++]=terr.cells[i];
            terr.cells.resize(out); EdTouched();
            edit.lastSave=erase?"wycieto otwor na wybranej wysokosci":"namalowano platforme; dolne warstwy zachowane";
        }
        return changed;
    }
