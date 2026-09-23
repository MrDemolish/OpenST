// Included inside Menu: editor dialogs share the canvas and font helpers.
    RECT EdChoiceRect(RECT r, int i, int columns) const
    {
        int width = (r.right - r.left) / columns;
        int x = r.left + (i % columns) * width, y = r.top + (i / columns) * 28;
        return RECT{x, y, x + width - 3, y + 24};
    }

    void EdControl(int id, RECT r, const char *label, bool selected = false)
    {
        edit.controlRect[id] = r;
        EdButton(r, label, selected ? 2 : (EdContains(r, mouse.x, mouse.y) ? 1 : 0));
    }

    void EdDrawQuickControls(const ed::Layout &L)
    {
        const char *labels[] = {"COFNIJ", "PONOW", "-", "+", "CENTRUJ", "MENU"};
        int x = 302;
        for (int i = 0; i < 6; ++i) {
            int w = (i == 2 || i == 3) ? 30 : 74;
            if (x + w > L.top.right - 6) break;
            EdControl(i, RECT{x, 44, x + w, 70}, labels[i]);
            x += w + 4;
        }
    }

    int EdDrawBrushControls(int x, int y, int w)
    {
        char text[80];
        std::snprintf(text, sizeof(text), "ROZMIAR: %d x %d", edit.brush*2+1, edit.brush*2+1);
        TextAt(fontGrey, x, y + 5, text);
        EdControl(6, RECT{x+w-62, y, x+w-34, y+24}, "-");
        EdControl(7, RECT{x+w-28, y, x+w, y+24}, "+");
        y += 28;
        EdControl(8, RECT{x, y, x+w/2-2, y+24}, "OKRAG", edit.brushRound);
        EdControl(9, RECT{x+w/2+2, y, x+w, y+24}, "KWADRAT", !edit.brushRound);
        y += 28;
        if (edit.tool == ed::T_PLATFORM) {
            std::snprintf(text, sizeof(text), "WYSOKOSC: %d", edit.platformLevel);
            TextAt(fontCyan, x, y+5, text);
            EdControl(10, RECT{x+w-62,y,x+w-34,y+24}, "-");
            EdControl(11, RECT{x+w-28,y,x+w,y+24}, "+");
            y += 28;
            TextAt(fontGrey,x,y+5,"LPM: polka   PPM: otwor");
            y += 22;
        }
        TextAt(fontCyan, x, y+5, "ZBOCZA: AUTOMATYCZNE");
        TextAt(fontGrey,x,y+23,"0-5: edytuj warstwe  A: wierzch");
        return y + 46;
    }

    bool EdControlClick(int x, int y)
    {
        for (int i = 0; i < 16; ++i) if (EdContains(edit.controlRect[i], x, y)) {
            switch (i) {
            case 0: EdUndo(); break;
            case 1: EdRedo(); break;
            case 2: EditorKey(VK_OEM_MINUS); break;
            case 3: EditorKey(VK_OEM_PLUS); break;
            case 4: EdCentre(); break;
            case 5: EdRequest(3); break;
            case 6: edit.brush = std::max(0, edit.brush - 1); break;
            case 7: edit.brush = std::min(12, edit.brush + 1); break;
            case 8: edit.brushRound = true; break;
            case 9: edit.brushRound = false; break;
            case 10: edit.platformLevel = std::max(1, edit.platformLevel-1); break;
            case 11: edit.platformLevel = std::min(5, edit.platformLevel+1); break;
            case 12: edit.texAll=!edit.texAll; edit.palScroll=0; break;
            case 13: edit.texMode=(edit.texMode+1)%ed::TEX_MODES; break;
            case 14: edit.dialog=5; edit.painting=false; break;
            }
            return true;
        }
        return false;
    }

    RECT EdDialogCloseRect() const
    {
        RECT r = EdDialogRect();
        return RECT{r.right - 100, r.top + 10, r.right - 12, r.top + 36};
    }
    RECT EdDialogRect() const
    {
        int w = std::min(620, SCREEN_W - 24), h = std::min(480, SCREEN_H - 24);
        return RECT{(SCREEN_W-w)/2, (SCREEN_H-h)/2, (SCREEN_W+w)/2, (SCREEN_H+h)/2};
    }

    RECT EdDialogButton(int row) const
    {
        RECT r = EdDialogRect();
        return RECT{r.left + 16, r.top + 58 + row * 36, r.right - 16, r.top + 88 + row * 36};
    }

    static bool EdContains(const RECT &r, int x, int y)
    {
        return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
    }

    void EdRefreshMaps()
    {
        // Keep original map numbers stable; add/update personal maps by path.
        for (const maps::Entry &m : maps::scan(EdMapDir())) {
            bool found = false;
            for (maps::Entry &old : skMaps)
                if (old.dkx == m.dkx) { old = m; found = true; break; }
            if (!found) skMaps.push_back(m);
        }
    }

    bool EdSaveCurrent()
    {
        std::string name = EdFreeName(terrName.empty() ? "mapa" : terrName);
        std::string err = name.empty() ? "brak wolnej nazwy" : EdSaveAs(name);
        edit.lastSave = err.empty() ? "zapisano: " + name : "BLAD: " + err;
        if (!err.empty()) return false;
        edit.dirty = edit.dirtyObj = false;
        EdRefreshMaps();
        return true;
    }

    void EdProceed(int action)
    {
        edit.pending = 0;
        edit.dialog = action >= 3 ? 0 : action;
        edit.painting = edit.panning = false;
        if (action == 1) { EdRefreshMaps(); edit.mapTop = 0; }
        if (action == 3) CloseEditor();
        if (action == 4) quitRequested = true;
    }

    void EdRequest(int action)
    {
        if (edit.dirty) { edit.pending = action; edit.dialog = 3; }
        else EdProceed(action);
        edit.painting = edit.panning = false;
    }

    int EdDialogRows() const
    {
        return std::max(1, int(EdDialogRect().bottom - EdDialogRect().top - 100) / 36);
    }

    void EdDrawDialog()
    {
        if (!edit.dialog) return;
        Fill(RECT{0, 0, SCREEN_W, SCREEN_H}, 0x000000, 180);
        RECT r = EdDialogRect();
        EdPanel(r, ed::PANEL, ed::ACCENT);
        const char *title = edit.dialog == 1 ? "OTWORZ MAPE" :
                            edit.dialog == 2 ? "NOWA MAPA" :
                            edit.dialog == 5 ? "ROZSIEWANIE ROSLIN" :
                            edit.dialog == 4 ? "WLASCIWOSCI ZAZNACZENIA" : "NIEZAPISANE ZMIANY";
        TextAt(fontCyan, r.left + 16, r.top + 16, title);
        EdButton(EdDialogCloseRect(), "ZAMKNIJ", 0);
        TextAt(fontGrey, r.left + 16, r.bottom - 24, "Esc: anuluj");
        if (edit.dialog == 1) {
            for (int row = 0; row < EdDialogRows(); ++row) {
                int i = edit.mapTop + row;
                if (i >= int(skMaps.size())) break;
                const maps::Entry &m = skMaps[size_t(i)];
                std::string text = m.title.empty() ? m.file : m.title;
                if (m.dkx.find(EdMapDir()) == 0) text = "[MOJA MAPA] " + text;
                RECT b = EdDialogButton(row);
                while (!text.empty() && LineWidth(fontGrey, text.c_str()) > b.right-b.left-16) text.pop_back();
                EdButton(b, text.c_str(), EdContains(b, mouse.x, mouse.y) ? 1 : 0);
            }
            TextAt(fontGrey, r.left + 200, r.bottom - 24, "kolko / PgUp PgDn: lista");
        } else if (edit.dialog == 2) {
            TextAt(fontGrey, r.left + 16, r.top + 58, "SZEROKOSC (komorki)");
            TextAt(fontGrey, r.left + 16, r.top + 130, "WYSOKOSC (komorki)");
            const char *sizes[] = {"32", "64", "128", "256"};
            for (int i = 0; i < 4; ++i) {
                EdButton(EdChoiceRect(EdDialogButton(1), i, 4), sizes[i], edit.newW == (32 << i) ? 2 : 0);
                EdButton(EdChoiceRect(EdDialogButton(3), i, 4), sizes[i], edit.newH == (32 << i) ? 2 : 0);
            }
            EdButton(EdDialogButton(5), "UTWORZ MAPE", 0);
            TextAt(fontGrey, r.left + 16, r.top + 205, "Plaska mapa z dwoma punktami startu.");
        } else if (edit.dialog == 5) {
            EdDrawScatterDialog();
        } else if (edit.dialog == 4) {
            EdButton(EdDialogButton(0), "PRZENIES  (wskaz miejsce na mapie)", 0);
            if (edit.selKind == ed::SEL_OBJ && edit.selIdx >= 0 && edit.selIdx < int(terr.objects.size())) {
                const auto &o = terr.objects[size_t(edit.selIdx)];
                std::string owner = "WLASCICIEL: " + std::to_string(o.owner) + "  (klik: nastepny)";
                std::string level = "POZIOM: " + std::to_string(o.z) + "  (klik: nastepny)";
                if (o.type == maps::OBJ_BUILDING || o.type == maps::OBJ_UNIT || o.type == maps::OBJ_MINE)
                    EdButton(EdDialogButton(1), owner.c_str(), 0);
                if (o.type != maps::OBJ_UNIT && o.type != maps::OBJ_VOLCANO)
                    EdButton(EdDialogButton(2), level.c_str(), 0);
                if (o.type == maps::OBJ_RESOURCE) {
                    std::string amount = "ILOSC: " + std::to_string(o.amount) + "  (klik: +5000)";
                    EdButton(EdDialogButton(3), amount.c_str(), 0);
                    EdButton(EdDialogButton(4), o.subtype == 221 ? "KORIUM  (klik: metal)" : "METAL  (klik: korium)", 0);
                }
            } else if (edit.selKind == ed::SEL_START && edit.selIdx >= 0 && edit.selIdx < int(edit.slots.size())) {
                const auto &slot = edit.slots[size_t(edit.selIdx)];
                static const char *races[] = {"NIEUSTAWIONA", "WHITE SHARKS", "BLACK OCTOPI", "SILICONS"};
                EdButton(EdDialogButton(1), races[std::min(3, int(slot.race))], 0);
                EdButton(EdDialogButton(2), slot.human ? "CZLOWIEK  (klik: komputer)" : "KOMPUTER  (klik: czlowiek)", 0);
            }
            EdButton(EdDialogButton(5), "USUN ZAZNACZENIE", 0);
            TextAt(fontGrey, r.left + 16, r.bottom - 48, "Kazda zmiane mozna cofnac klawiszem Z.");
        } else {
            EdButton(EdDialogButton(0), "ZAPISZ I KONTYNUUJ", 0);
            EdButton(EdDialogButton(1), "PORZUC ZMIANY", 0);
            EdButton(EdDialogButton(2), "ANULUJ", 0);
        }
    }

    bool EdDialogClick(int x, int y)
    {
        if (EdContains(EdDialogCloseRect(), x, y)) {
            edit.dialog = 0; edit.pending = 0; return true;
        }
        if (edit.dialog == 1) {
            for (int row = 0; row < EdDialogRows(); ++row)
                if (EdContains(EdDialogButton(row), x, y)) {
                    int i = edit.mapTop + row;
                    if (i < int(skMaps.size()) && !OpenEditor(i)) edit.lastSave = "BLAD: nie moge otworzyc mapy";
                    return true;
                }
        } else if (edit.dialog == 2) {
            for (int i = 0; i < 4; ++i) {
                if (EdContains(EdChoiceRect(EdDialogButton(1), i, 4), x, y)) edit.newW = 32 << i;
                if (EdContains(EdChoiceRect(EdDialogButton(3), i, 4), x, y)) edit.newH = 32 << i;
            }
            if (EdContains(EdDialogButton(5), x, y)) {
                std::string name = EdFreeName("NowaMapa");
                std::string err = name.empty() ? "brak wolnej nazwy" : EdNewMap(name, edit.newW, edit.newH, 1, 4352);
                if (!err.empty()) { edit.dialog = 0; edit.lastSave = "BLAD: " + err; }
            }
        } else if (edit.dialog == 5) {
            EdScatterClick(x,y);
        } else if (edit.dialog == 4) {
            if (EdContains(EdDialogButton(0), x, y)) { edit.dialog = 0; edit.moving = true; edit.tool = ed::T_SELECT; return true; }
            if (EdContains(EdDialogButton(5), x, y)) { EdDeleteSelection(); edit.dialog = 0; return true; }
            int row = -1;
            for (int i = 1; i <= 4; ++i) if (EdContains(EdDialogButton(i), x, y)) row = i;
            if (row < 0) return true;
            if (edit.selKind == ed::SEL_OBJ && edit.selIdx >= 0 && edit.selIdx < int(terr.objects.size())) {
                auto &o = terr.objects[size_t(edit.selIdx)];
                bool owned = o.type == maps::OBJ_BUILDING || o.type == maps::OBJ_UNIT || o.type == maps::OBJ_MINE;
                if (row == 2 && (o.type == maps::OBJ_UNIT || o.type == maps::OBJ_VOLCANO)) return true;
                if ((row == 1 && !owned) || (row >= 3 && o.type != maps::OBJ_RESOURCE)) return true;
                EdPushUndo();
                if (row == 1) {
                    o.owner = (o.owner + 1) & 7;
                    if (o.type == maps::OBJ_UNIT && o.raw.size() >= 24) ark::wr32(&o.raw[20], o.owner);
                    if (o.type == maps::OBJ_BUILDING && o.raw.size() >= 44) ark::wr32(&o.raw[40], o.owner);
                }
                if (row == 2) o.z = (o.z + 1) % 5;
                if (row == 3) o.amount = o.amount >= 200000 ? 5000 : o.amount + 5000;
                if (row == 4) o.subtype = o.subtype == 221 ? 222 : 221;
            } else if (edit.selKind == ed::SEL_START && edit.selIdx >= 0 && edit.selIdx < int(edit.slots.size()) && row <= 2) {
                EdPushUndo();
                auto &slot = edit.slots[size_t(edit.selIdx)];
                if (row == 1) { slot.race = uint8_t(slot.race % 3 + 1); players[edit.selIdx].civ = slot.race - 1; }
                if (row == 2) slot.human = !slot.human;
            } else return true;
            edit.dirty = edit.dirtyObj = true;
            EdRefreshWorld();
        } else {
            int action = edit.pending;
            if (EdContains(EdDialogButton(0), x, y)) { if (EdSaveCurrent()) EdProceed(action); }
            if (EdContains(EdDialogButton(1), x, y)) EdProceed(action);
            if (EdContains(EdDialogButton(2), x, y)) { edit.dialog = 0; edit.pending = 0; }
        }
        return true;
    }

    void EdDialogScroll(int direction)
    {
        int last = std::max(0, int(skMaps.size()) - EdDialogRows());
        edit.mapTop = std::max(0, std::min(last, edit.mapTop + direction));
    }

    void EdDeleteSelection()
    {
        int i = edit.selIdx;
        if (i < 0) return;
        if (edit.selKind == ed::SEL_OBJ && i < int(terr.objects.size())) {
            EdPushUndo(); terr.objects.erase(terr.objects.begin() + i);
        } else if (edit.selKind == ed::SEL_DECOR && i < int(terr.decor.size())) {
            EdPushUndo(); terr.decor.erase(terr.decor.begin() + i);
        } else if (edit.selKind == ed::SEL_START && i < int(edit.slots.size())) {
            EdPushUndo(); edit.slots[size_t(i)].used = false; edit.slots[size_t(i)].id = 0xFF;
        } else return;
        edit.selKind = ed::SEL_NIC; edit.selIdx = edit.selObj = -1;
        edit.dirty = edit.dirtyObj = true;
        edit.moving = false;
        decorOrder.clear(); EdRefreshWorld();
    }

    bool EdMoveSelection(int bx, int by)
    {
        int i = edit.selIdx;
        if (bx < 0 || by < 0 || bx >= terr.bw || by >= terr.bh || i < 0) return false;
        if (edit.selKind == ed::SEL_OBJ && i < int(terr.objects.size())) {
            for (size_t k = 0; k < terr.objects.size(); ++k)
                if (int(k) != i && !terr.objects[k].spawned && terr.objects[k].x/2 == bx && terr.objects[k].y/2 == by)
                    return false;
            EdPushUndo(); auto &o = terr.objects[size_t(i)]; o.x = bx*2; o.y = by*2;
        } else if (edit.selKind == ed::SEL_DECOR && i < int(terr.decor.size())) {
            EdPushUndo(); auto &d = terr.decor[size_t(i)];
            d.x = uint16_t((bx*2+1) * maps::WORLD_PER_CELL);
            d.y = uint16_t((by*2+1) * maps::WORLD_PER_CELL);
            d.z = uint16_t(std::max(0, terr.topAt(bx,by)) * maps::WORLD_PER_LEVEL);
        } else if (edit.selKind == ed::SEL_START && i < int(edit.slots.size())) {
            EdPushUndo(); auto &slot = edit.slots[size_t(i)]; slot.x = bx*2; slot.y = by*2;
        } else return false;
        edit.dirty = edit.dirtyObj = true;
        edit.moving = false;
        decorOrder.clear(); EdRefreshWorld();
        return true;
    }
