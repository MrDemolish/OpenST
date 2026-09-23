// Diagnostic command line modes.
//
// Everything the remake can be asked to do without opening a window: compose a
// frame and dump it, walk a map, time the renderer, list an archive. They live
// here rather than in main() because they are read far more often than they are
// changed, and because main() is then short enough to take in at a glance.
//
//   --smoke                      load everything and report
//   --dump [ticks]               menu frame to compose.raw
//   --dumpsk [n]                 skirmish screen
//   --anim [n]                   the button reveal, frame by frame
//   --sfx <id>                   one sound as a WAV
//   --music [track]              list tracks, or decode one to music.wav
//   --cursor [name]              list cursors, or tile one to cursor.raw
//   --frames <record>            every frame of a sprite strip
//   --obj <archive> <record>     describe a strip
//   --terrain <map> [layer] [zoom] [start] [w] [h]
//   --panel <map> [w] [h]        the full HUD over a map
//   --drive <map>                one unit sent eight ways
//   --path <map> [tries] [level] random paths, checked step by step
//   --bench <map> <zoom> <w> <h> [frames]
//
// Returns the exit code for main, or -1 to carry on and open the window.
#pragma once
#include <map>
#include <algorithm>
#include <cstring>
#include <set>
static bool g_dockShot = false;

#include "steditcheck.h"
#include "stterraincheck.h"
#include "stcavecheck.h"
#include "stfogcheck.h"
#include "stmapcheck.h"
#include "stweaponcheck.h"
#include "stbuildingcheck.h"
#include "stsoundcheck.h"
#include "stunitcheck.h"

inline int RunCli(int argc, char **argv, const std::string &gameDir)
{
    (void)gameDir;
    if (argc > 2 && std::strcmp(argv[2], "--unitcheck") == 0) return RunUnitCheck();
    if (argc > 2 && std::strcmp(argv[2], "--soundcheck") == 0) return RunSoundCheck();
    if (argc > 2 && std::strcmp(argv[2], "--buildingcheck") == 0) return RunBuildingCheck();
    if (argc > 2 && std::strcmp(argv[2], "--weaponcheck") == 0) return RunWeaponCheck();
    if (argc > 2 && std::strcmp(argv[2], "--mapcheck") == 0) return RunMapCheck();
    if (argc > 2 && std::strcmp(argv[2], "--mapsavecheck") == 0) return RunMapSaveCheck();
    // Repack through the editor's real serializer, leaving the source intact.
    if (argc > 4 && std::strcmp(argv[2], "--maprepair") == 0) {
        std::string source = argv[3];
        size_t dot = source.find_last_of('.');
        if (dot == std::string::npos) return 1;
        maps::Entry entry;
        if (!maps::readOne(source, source.substr(0,dot)+".DKD", "repair", entry)) return 1;
        g_menu.skMaps.push_back(entry);
        if (!g_menu.OpenEditor(int(g_menu.skMaps.size())-1)) return 1;
        std::string err = g_menu.EdSaveAs(argv[4]);
        std::printf("maprepair: %s\n", err.empty() ? "OK" : err.c_str());
        if (err.empty()) {
            std::string x,d; g_menu.EdTargets(argv[4],x,d);
            std::printf("%s\n%s\n",x.c_str(),d.c_str());
        }
        return err.empty() ? 0 : 1;
    }
    if (argc > 2 && std::strcmp(argv[2], "--cavecheck") == 0) return RunCaveCheck();
    if (argc > 3 && std::strcmp(argv[2], "--terraincheck") == 0) return RunTerrainCheck(std::atoi(argv[3]));
    if (argc > 3 && std::strcmp(argv[2], "--fogcheck") == 0) return RunFogCheck(std::atoi(argv[3]));
    if (argc > 2 && std::strcmp(argv[2], "--editorcheck") == 0)
        return RunEditorCheck(argc > 3 && std::strcmp(argv[3], "--keep") == 0);
    // --dump composes a frame and writes it out, so the native compositor can
    // be checked without a window
    if (argc > 2 && std::strcmp(argv[2], "--dump") == 0) {
        g_menu.hot = 0;                       // show one button hovered
        // Settle to what the menu actually looks like: buttons open, and the
        // backgrounds wound forward so the shot is not every animation's
        // frame 0.  argv[3] picks how far in.
        int ticks = (argc > 3) ? std::atoi(argv[3]) : 30;
        if (argc > 4) g_menu.mode = std::atoi(argv[4]);
        DWORD t = 0;
        for (int i = 0; i < 400 && g_menu.phase != Menu::PH_IDLE; ++i)
            g_menu.Step(t += g_menu.interval);
        for (int i = 0; i < ticks; ++i) g_menu.StepBackgrounds(t += FLC_INTERVAL);
        g_menu.Compose();
        {   // **Guzik menu ma DRUGI pasek** - `MM_MABUT<nn>`, czterdziesci
            // klatek ciaglej animacji w samym licu. Liczy sie ruch na
            // plotnie, a nie to, ze pasek sie wczytal: policzmy, ile
            // pikseli WEWNATRZ guzikow zmienia sie po kilku klatkach.
            std::vector<uint32_t> przed = g_menu.canvas;
            for (int i = 0; i < 12; ++i)
                g_menu.StepBackgrounds(t += FLC_INTERVAL);
            g_menu.Compose();
            int wGuzikach = 0, poza = 0, zPaskiem = 0;
            for (int b = 0; b < BTN_COUNT; ++b)
                if (g_menu.maCount[b] > 1) ++zPaskiem;
            for (int y = 0; y < SCREEN_H; ++y)
                for (int x = 0; x < SCREEN_W; ++x) {
                    size_t i = size_t(y) * size_t(SCREEN_W) + size_t(x);
                    if (przed[i] == g_menu.canvas[i]) continue;
                    bool w = false;
                    for (int b = 0; b < BTN_COUNT; ++b)
                        if (x >= kButtonPos[b].x && x < kButtonPos[b].x + BTN_W &&
                            y >= kButtonPos[b].y && y < kButtonPos[b].y + BTN_H)
                            w = true;
                    if (w) ++wGuzikach; else ++poza;
                }
            std::printf("lico guzikow: %d z %d ma pasek MM_MABUT (po %d klatek), "
                        "pikseli ruchu w licu %d, poza (tla FLC) %d\n",
                        zPaskiem, BTN_COUNT, g_menu.maCount[0],
                        wGuzikach, poza);
            g_menu.Compose();   // oddaj zrzut do stanu, ktory zapisujemy
        }
        FILE *o = std::fopen("compose.raw", "wb");
        if (o) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
            std::printf("zapisano compose.raw %dx%d BGRA\n", SCREEN_W, SCREEN_H);
        }
        return 0;
    }

    // --dumpload sklada ekran wczytywania: [mapa] [krok].
    if (argc > 2 && std::strcmp(argv[2], "--dumpload") == 0) {
        {   int nr = argc > 3 ? std::atoi(argv[3]) : 0;
            g_menu.SkSetTab(nr >= g_menu.skCustom ? 1 : 0);
            g_menu.skSel = nr;
        }
        g_menu.SkReveal();
        int doKroku = argc > 4 ? std::atoi(argv[4]) : 2;
        g_menu.OpenLoad();
        DWORD t = 0;
        for (int i = 0; i < doKroku; ++i) g_menu.StepLoad(t += 100);
        g_menu.screen = SCR_LOAD;
        g_menu.Compose();
        if (FILE *o = std::fopen("compose.raw", "wb")) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        // **Obraz jest losowany z trzech na rase** - sprawdzamy, ze
        // wszystkie dziewiec rekordow jest w archiwum, bo brak jednego
        // wychodzi dopiero wtedy, gdy los na niego padnie.
        int mam = 0;
        for (int r2 = 0; r2 < 3; ++r2)
            for (int v = 1; v <= LOAD_VARIANTS; ++v) {
                char nm[32];
                std::snprintf(nm, sizeof(nm), "%s%d", kLoadPrefix[r2], v);
                if (!g_menu.inter.read(nm).empty()) ++mam;
            }
        std::printf("ekran wczytywania: obrazow %d z 9, wybrany %s %dx%d, rasa %d, krok %d\n",
                    mam, g_menu.loadPick, g_menu.loadBg.w, g_menu.loadBg.h,
                    g_menu.LoadRace(), g_menu.loadStage);
        std::printf("  konsola:");
        for (const std::string &ln : g_menu.loadLines)
            std::printf(" [%s]", ln.c_str());
        if (!g_menu.loadStat.empty())
            std::printf(" [%s]", g_menu.loadStat.c_str());
        std::printf("\n");
        {   // **Caly przebieg musi dojsc do gry.** Krecimy nim do konca
            // i patrzymy, gdzie wyladuje - dla misji ma to byc odprawa,
            // dla potyczki teren.
            for (int i = 0; i < 40 && g_menu.screen == SCR_LOAD; ++i)
                g_menu.StepLoad(t += 100);
            const char *gdzie = g_menu.screen == SCR_BRIEF ? "odprawa"
                              : g_menu.screen == SCR_TERRAIN ? "teren"
                              : g_menu.screen == SCR_SKIRMISH ? "powrot (blad)"
                                                              : "?";
            std::printf("  po przebiegu: %s, komorek %d, wierszy konsoli %d\n",
                        gdzie, int(g_menu.terr.cells.size()),
                        int(g_menu.loadLines.size()));
        }
        std::fflush(stdout);
        return 0;
    }

    // --dumpset sklada ekran ustawien bitwy: [mapa].
    if (argc > 2 && std::strcmp(argv[2], "--dumpset") == 0) {
        {   // **`SkSetTab` przestawia wybor na poczatek zakladki**, wiec
            // numer mapy trzeba wpisac PO niej, nie przed.
            int nr = argc > 3 ? std::atoi(argv[3]) : 0;
            g_menu.SkSetTab(nr >= g_menu.skCustom ? 1 : 0);
            g_menu.skSel = nr;
        }
        g_menu.SkReveal();
        g_menu.OpenSetup();
        {   DWORD t = 0;
            for (int i = 0; i < 200 && g_menu.MsgStep(t += g_menu.interval); ++i) {}
        }
        g_menu.Compose();
        if (FILE *o = std::fopen("compose.raw", "wb")) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        std::vector<const maps::MapSlot *> sl;
        g_menu.SetupSlots(sl);
        std::printf("ustawienia bitwy: mapa %s, gniazd %d",
                    g_menu.skSel >= 0 && g_menu.skSel < int(g_menu.skMaps.size())
                        ? g_menu.skMaps[size_t(g_menu.skSel)].title.c_str() : "-",
                    int(sl.size()));
        for (size_t k = 0; k < sl.size(); ++k)
            std::printf(" %d:%s%s", int(sl[k]->id) & 7,
                        Menu::CivName(g_menu.SetupCivOf(*sl[k])),
                        sl[k]->human ? "*" : "");
        std::printf(" (* = gniazdo ludzkie wg mapy)\n");
        {   // **Klikniecie ma naprawde zmieniac to, czym gramy.** Lewa
            // polowa wiersza przebiera rase, prawa przejmuje gniazdo;
            // liczy sie skutek po wczytaniu mapy, a nie samo pole.
            int zmian = 0, pikseli = 0;
            if (!sl.empty()) {
                RECT r = g_menu.SetupRow(0);
                int i0 = int(sl[0]->id) & 7;
                int przed = g_menu.SetupCivOf(*sl[0]);
                std::vector<uint32_t> bez = g_menu.canvas;
                g_menu.SkClick((r.left + r.right) / 4 + r.left / 2, (r.top + r.bottom) / 2);
                if (g_menu.SetupCivOf(*sl[0]) != przed) ++zmian;
                g_menu.Compose();
                for (size_t q = 0; q < bez.size(); ++q)
                    if (bez[q] != g_menu.canvas[q]) ++pikseli;
                g_menu.SkClick(r.right - 8, (r.top + r.bottom) / 2);
                bool przejete = g_menu.setupMe == i0;
                int chce = g_menu.setupCiv[i0];
                if (g_menu.OpenTerrain(g_gameDir)) {
                    std::printf("  po starcie: gramy gniazdem %d (chcielismy %d), "
                                "rasa %d (chcielismy %d)\n",
                                g_menu.me, przejete ? i0 : -1,
                                g_menu.players[g_menu.me & 7].civ, chce);
                }
            }
            std::printf("  klikniecie: rasa przebrana %d, pikseli roznicy %d\n",
                        zmian, pikseli);
        }
        {   // **Zasady partii.** Trzy pola z dziewieciu remake wykonuje;
            // liczy sie skutek, nie to, ze pole sie przelacza.
            g_menu.OpenSetup();
            int przelacza = 0, poza = 0, kolizji = 0;
            for (int k = 0; k < SETUP_OPTS; ++k) {
                RECT r = g_menu.SetupOptBox(k);
                if (r.left < 0 || r.top < 0
                    || r.right > MENU_W || r.bottom > MENU_H) ++poza;
                for (int j = 0; j < k; ++j) {
                    RECT o = g_menu.SetupOptBox(j);
                    if (r.left < o.right && o.left < r.right &&
                        r.top < o.bottom && o.top < r.bottom) ++kolizji;
                }
                std::string przed = g_menu.SetupOptVal(k);
                g_menu.SkClick((r.left + r.right) / 2, (r.top + r.bottom) / 2);
                if (g_menu.SetupOptVal(k) != przed) ++przelacza;
            }
            // Poziom surowcow: NISKI kontra WYSOKI po wczytaniu mapy.
            g_menu.OpenSetup();
            g_menu.setupOpt[SOPT_RES] = 0;
            g_menu.OpenTerrain(g_gameDir);
            int niski = g_menu.Me().bank.metal;
            g_menu.setupOpt[SOPT_RES] = 2;
            g_menu.OpenTerrain(g_gameDir);
            int wysoki = g_menu.Me().bank.metal;
            // Limit jednostek: zamow ponad limit i policz kolejke.
            g_menu.setupOpt[SOPT_CAP] = 1;           // 50
            int mam = 0;
            for (const Menu::Unit &u : g_menu.units)
                if ((u.owner & 7) == uint32_t(g_menu.me & 7) && u.hp > 0) ++mam;
            {   // **Obsada gniazda: CZLOWIEK i trzy poziomy komputera.**
                // Napisy sa z gry (8002, 8007..8009), a sam cykl klikania
                // jest moj - exe podaje slownik, nie kolejnosc.
                //
                // Mierzy sie **skutek**, nie etykiete: po przestawieniu
                // poziomu mapa jest wczytywana jeszcze raz i pytamy `aiSt`
                // o to, co naprawde dostal komputer. Samo przestawienie
                // `setupAi` wygladaloby tak samo, gdyby nigdzie nie doszlo.
                std::vector<const maps::MapSlot *> gn2;
                g_menu.SetupSlots(gn2);
                int wiersz = -1;
                for (int k = 0; k < int(gn2.size()); ++k)
                    if ((int(gn2[size_t(k)]->id) & 7) != (g_menu.me & 7)) {
                        wiersz = k; break;
                    }
                if (wiersz < 0) {
                    std::printf("  obsada gniazda: BRAK CUDZEGO GNIAZDA"
                                " - test nie ma na czym stanac\n");
                } else {
                    int slot = int(gn2[size_t(wiersz)]->id) & 7;
                    RECT rw = g_menu.SetupRow(wiersz);
                    int px = (rw.left + rw.right * 3) / 4;   // prawa polowa
                    int py = (rw.top + rw.bottom) / 2;
                    std::string cykl;
                    int roznych = 0, ostatni = -99;
                    for (int k = 0; k < 4; ++k) {
                        bool nasz = g_menu.setupMe == slot;
                        if (!cykl.empty()) cykl += " -> ";
                        cykl += nasz ? "CZLOWIEK"
                                     : Text(g_menu.AiSlotStr(slot));
                        int stan = nasz ? -1 : g_menu.AiSlotLvl(slot);
                        if (stan != ostatni) ++roznych;
                        ostatni = stan;
                        g_menu.SetupClick(px, py);
                    }
                    g_menu.setupAi[slot] = 2;
                    g_menu.OpenTerrain(g_gameDir);
                    int poWysokim = g_menu.aiSt[slot].level;
                    g_menu.setupAi[slot] = 0;
                    g_menu.OpenTerrain(g_gameDir);
                    int poSlabym = g_menu.aiSt[slot].level;
                    std::printf("  obsada gniazda %d: %s\n", slot,
                                cykl.c_str());
                    std::printf("    stanow roznych %d z 4; do komputera "
                                "WYSOKA->%d SLABA->%d%s\n",
                                roznych, poWysokim, poSlabym,
                                (roznych == 4 && poWysokim == 2
                                 && poSlabym == 0)
                                    ? "" : "  <- BEZ SKUTKU");
                    g_menu.setupAi[slot] = -1;
                }
            }
            {   // **Rodzaje rozgrywki z `strategs.DKX`.** Nazwa kazdego jest
                // w samym rekordzie `AIBOSS*` (bajt 6, 63 znaki), a cel
                // w blizniaczym `OBJECTIVES*`. Nic tu nie jest wpisane
                // z reki - gdyby archiwum sie nie otworzylo, lista bylaby
                // pusta i wiersz to powie.
                std::printf("  rodzaje rozgrywki: %d z archiwum\n",
                            int(g_menu.gmName.size()));
                for (size_t i = 0; i < g_menu.gmName.size(); ++i)
                    std::printf("    %d  %-28s %s\n", int(i),
                                g_menu.gmName[i].c_str(),
                                i < g_menu.gmGoal.size()
                                    ? g_menu.gmGoal[i].c_str() : "");
                std::printf("    barw z nazwy %d, werdykty \"%s\" / \"%s\"%s\n",
                            int(g_menu.gmColour.size()),
                            g_menu.gmWin.c_str(), g_menu.gmLose.c_str(),
                            (g_menu.gmName.size() == 3
                             && g_menu.gmColour.size() == 8
                             && !g_menu.gmWin.empty())
                                ? "" : "  <- NIE WCZYTALO SIE");
            }
            {   // **The Flagship Hunt mierzy sie PARA.** Samo „gracz wypadl
                // po stracie okretu" nie dowodzi niczego - wyszloby tak samo,
                // gdyby wypadal po stracie czegokolwiek. Obok stoi wiec ta
                // sama mapa w trybie Kill All, gdzie po stracie okretu ma
                // **zostac** w grze, bo ma jeszcze lodzie i budynki.
                // Scena jest w obu przebiegach **ta sama** - mapa zawsze
                // wczytuje sie w trybie polowania, zeby okrety w ogole byly.
                // Rozni je dopiero regula konca, przestawiana tuz przed
                // `CheckWin`. Inaczej porownywaloby sie partie, ktore nawet
                // nie maja tych samych jednostek.
                auto probuj = [&](int tryb) {
                    g_menu.setupOpt[SOPT_MODE] = GM_FLAGSHIP;
                    g_menu.setupOpt[SOPT_PLACE] = 1;      // rozstaw sily
                    g_menu.OpenTerrain(g_gameDir);
                    g_menu.setupOpt[SOPT_MODE] = tryb;
                    int ofiara = -1, okretow = 0, innych = 0;
                    for (const Menu::Unit &u : g_menu.units)
                        if (Menu::IsFlagship(int(u.type))) {
                            ++okretow;
                            if (int(u.owner & 7) != (g_menu.me & 7)
                                && ofiara < 0)
                                ofiara = int(u.owner & 7);
                        }
                    if (ofiara < 0) return std::string("BRAK OFIARY");
                    for (Menu::Unit &u : g_menu.units)
                        if (int(u.owner & 7) == ofiara
                            && Menu::IsFlagship(int(u.type))) u.hp = 0;
                        else if (int(u.owner & 7) == ofiara && u.hp > 0)
                            ++innych;
                    g_menu.CheckWin();
                    char c[96];
                    std::snprintf(c, sizeof(c),
                                  "okretow %d, ofiara %d ma jeszcze %d lodzi,"
                                  " po stracie %s", okretow, ofiara, innych,
                                  g_menu.players[ofiara].active
                                      ? "GRA DALEJ" : "wypadl");
                    return std::string(c);
                };
                std::string a = probuj(GM_FLAGSHIP);
                std::string b = probuj(GM_KILLALL);
                bool ok = a.find("wypadl") != std::string::npos
                       && b.find("GRA DALEJ") != std::string::npos;
                std::printf("  polowanie na okrety: %s\n", a.c_str());
                std::printf("  to samo w Kill All:  %s%s\n", b.c_str(),
                            ok ? "" : "  <- REGULA BEZ SKUTKU");
                g_menu.setupOpt[SOPT_MODE] = GM_KILLALL;
                g_menu.setupOpt[SOPT_PLACE] = 0;
                g_menu.OpenTerrain(g_gameDir);
            }
            std::printf("  zasady: pol %d, przelacza %d, poza ekranem %d, kolizji %d\n", SETUP_OPTS, przelacza, poza, kolizji);
            std::printf("  poziom surowcow: NISKI metal %d, WYSOKI %d%s; limit 50 przy %d lodziach\n",
                        niski, wysoki, wysoki > niski ? "" : "  <- BEZ SKUTKU",
                        mam);

            // **WIDZIALNI SOJUSZNICY.** Mierzone tabelka 2x2: pole wlaczone
            // albo nie, razy sojusz zawarty albo nie. Sam pomiar "z polem
            // widac wiecej" by nie wystarczyl - wyszedlby tak samo, gdyby
            // pole odslanialo cala mape niezaleznie od sojuszu. Dopiero
            // cztery liczby pokazuja, ze liczy sie **para** (pole, sojusz).
            // **Test musi miec CO odslonic.** Bez tego `--dumpset` wywolane
            // bez numeru mapy dawalo same zera i wiersz wypisywal
            // `BEZ SKUTKU` - a powodem bylo tylko to, ze na tamtej mapie
            // gracz nie ma zadnego obiektu. Stawiamy wiec obie lodzie sami,
            // swoja i sojusznika, daleko od siebie.
            const uint32_t jaSlot = uint32_t(g_menu.me & 7);
            uint32_t sojSlot = (jaSlot + 1) & 7;
            size_t bylyLodzie = g_menu.units.size();
            {   Menu::Unit a;
                a.owner = jaSlot; a.type = 1;
                a.x = 10; a.y = 10;
                g_menu.units.push_back(a);
                Menu::Unit b2;
                b2.owner = sojSlot; b2.type = 1;
                b2.x = float(g_menu.terr.bw - 6) * 2.0f;
                b2.y = float(g_menu.terr.bh - 6) * 2.0f;
                g_menu.units.push_back(b2);
            }
            auto odsloniete = [&](bool pole, bool sojusz) {
                g_menu.setupOpt[SOPT_ALLYSEE] = pole ? 1 : 0;
                for (int i = 0; i < Menu::MAX_PLAYERS; ++i) g_menu.ally[i] = 0;
                if (sojusz) g_menu.ally[sojSlot] = 1;
                g_menu.fogOn = true;
                g_menu.ResetFog();
                g_menu.fogAcc = 1.0f;
                g_menu.StepFog(1.0f);
                int n = 0;
                for (uint8_t f : g_menu.fog)
                    if (f != Menu::FOG_NONE) ++n;
                return n;
            };
            int bezBez = odsloniete(false, false);
            int bezSoj = odsloniete(false, true);
            int zBez   = odsloniete(true,  false);
            int zSoj   = odsloniete(true,  true);
            for (int i = 0; i < Menu::MAX_PLAYERS; ++i) g_menu.ally[i] = 0;
            g_menu.setupOpt[SOPT_ALLYSEE] = 0;
            while (g_menu.units.size() > bylyLodzie) g_menu.units.pop_back();
            g_menu.Reap();
            // **GRA DRUZYNOWA.** Mierzone parami i **skutkiem w `Foe`**, a nie
            // samym polem `ally`: wpisanie jedynek do tablicy wygladaloby tak
            // samo, gdyby nic z nich nie wynikalo. Przy wylaczonym polu
            // wszyscy maja byc wrogami, przy wlaczonym czesc ma przestac nimi
            // byc - i to musi widziec ta sama funkcja, ktora rozstrzyga ogien.
            auto zDruzyna = [&](int wl) {
                g_menu.setupOpt[SOPT_TEAM] = wl;
                g_menu.OpenTerrain(g_gameDir);
                int soj = 0, wrog = 0;
                for (int k2 = 0; k2 < Menu::MAX_PLAYERS; ++k2) {
                    if (!g_menu.players[k2].active) continue;
                    if (k2 == (g_menu.me & 7)) continue;
                    if (g_menu.Foe(uint32_t(g_menu.me), uint32_t(k2))) ++wrog;
                    else ++soj;
                }
                return std::make_pair(soj, wrog);
            };
            auto bezDr = zDruzyna(0);
            auto zDr = zDruzyna(1);
            g_menu.setupOpt[SOPT_TEAM] = 0;
            std::printf("  gra druzynowa: NIE sojusznikow %d wrogow %d, "
                        "TAK %d / %d -> %s\n",
                        bezDr.first, bezDr.second, zDr.first, zDr.second,
                        (bezDr.first == 0 && zDr.first > 0
                         && zDr.second < bezDr.second)
                            ? "dzieli na druzyny"
                            : (bezDr.second + bezDr.first < 2
                                ? "ZA MALO GRACZY NA TEJ MAPIE"
                                : "BEZ SKUTKU"));

            // **POZIOM TECHNOLOGII i MAX POZIOM** - dwa pola zasad, mierzone
            // parami. Sam pomiar "przy WYSOKIM cos jest zbadane" nie
            // dowodzilby niczego: wyszedlby tak samo, gdyby remake badal
            // wszystko przy kazdym ustawieniu. Dlatego obok stoi NISKI,
            // ktory ma dac **zero**, i licznik odblokowanych budynkow,
            // ktory ma z tego wyniknac.
            auto zTech = [&](int start, int cap) {
                g_menu.setupOpt[SOPT_TECH] = start;
                g_menu.setupOpt[SOPT_TECHMAX] = cap;
                g_menu.OpenTerrain(g_gameDir);
                int zbad = 0, wolnych = 0, ponad = 0;
                int nn = 0;
                const tech::Tech *tt = tech::list(nn);
                Menu::Player &p = g_menu.Me();
                for (int k = 0; k < nn; ++k) {
                    if (tt[k].side != p.civ) continue;
                    if (k < int(p.techDone.size()) && p.techDone[size_t(k)]) ++zbad;
                    if (tt[k].level > g_menu.SetupTechCap()) ++ponad;
                }
                int ile = 0;
                const int *lst = cost::bldList(g_menu.PlayerSide(), ile);
                for (int k = 0; k < ile; ++k)
                    if (g_menu.BuildUnlocked(lst[k], false)) ++wolnych;
                return std::make_tuple(zbad, wolnych, ponad);
            };
            auto tNiski = zTech(0, 0);
            auto tWysoki = zTech(2, 2);
            g_menu.setupOpt[SOPT_TECH] = g_menu.setupOpt[SOPT_TECHMAX] = 0;
            std::printf("  poziom technologii: NISKI zbadanych %d (budynkow "
                        "wolnych %d), WYSOKI %d (%d) -> %s\n",
                        std::get<0>(tNiski), std::get<1>(tNiski),
                        std::get<0>(tWysoki), std::get<1>(tWysoki),
                        (std::get<0>(tNiski) == 0
                         && std::get<0>(tWysoki) > 0
                         && std::get<1>(tWysoki) > std::get<1>(tNiski))
                            ? "daje i odblokowuje" : "BEZ SKUTKU");
            std::printf("  max poziom technologii: NISKI odcina %d wezlow, "
                        "WYSOKI %d -> %s\n",
                        std::get<2>(tNiski), std::get<2>(tWysoki),
                        std::get<2>(tNiski) > std::get<2>(tWysoki)
                            ? "limit tnie" : "BEZ SKUTKU");

            const char *werdykt =
                bezBez == 0 ? "NIC DO ODSLONIECIA - test nie mial na czym stanac"
                : (bezBez == bezSoj && zBez == bezBez && zSoj > zBez)
                    ? "liczy sie para (pole, sojusz)"
                    : "BEZ SKUTKU albo dziala bez sojuszu";
            std::printf("  widzialni sojusznicy: NIE/bez %d, NIE/sojusz %d, "
                        "TAK/bez %d, TAK/sojusz %d -> %s\n",
                        bezBez, bezSoj, zBez, zSoj, werdykt);
        }
        std::fflush(stdout);
        return 0;
    }

    // --dumpcamp sklada ekran wyboru kampanii: [rasa 1-3] [samouczek 0/1].
    if (argc > 2 && std::strcmp(argv[2], "--dumpcamp") == 0) {
        g_menu.screen = SCR_CAMPAIGN;
        g_menu.OpenCampaign(argc > 4 && std::atoi(argv[4]) != 0);
        if (argc > 3) {
            int r = std::atoi(argv[3]);
            if (r >= 1 && r <= 3) g_menu.campRace = r;
        }
        {   DWORD t = 0;
            for (int i = 0; i < 200 && g_menu.MsgStep(t += g_menu.interval); ++i) {}
        }
        g_menu.Compose();
        FILE *o = std::fopen("compose.raw", "wb");
        if (o) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        // **Jeden ekran, dwa tytuly** - `PaintCampaign` bierze 0x26B1
        // ROZPOCZNIJ KAMPANIE albo 0x2359 SAMOUCZEK. Sprawdzamy, ze obie
        // drogi maja komplet trzech ras i ze kazda prowadzi na wlasna
        // pierwsza mape, a nie zawsze na te sama.
        std::printf("kampania: tlo %s, obrazki ras %d z 3, tytul \"%s\"\n",
                    g_menu.campBg.ok() ? "jest" : "BRAK",
                    (g_menu.campPanel[0].ok() ? 1 : 0)
                    + (g_menu.campPanel[1].ok() ? 1 : 0)
                    + (g_menu.campPanel[2].ok() ? 1 : 0),
                    Text(g_menu.campTutor ? STR_TUTOR_TITLE
                                          : STR_CAMP_TITLE).c_str());
        for (int tut = 0; tut < 2; ++tut) {
            std::printf("  %s:", tut ? "samouczki" : "kampanie ");
            int rozne = 0, byly[4] = { -1, -1, -1, -1 };
            for (int r = 1; r <= 3; ++r) {
                int k = g_menu.CampFirst(r, tut != 0);
                byly[r] = k;
                std::printf(" %d:%s(%d map)", r,
                            k >= 0 ? g_menu.skMaps[size_t(k)].file.c_str() : "-",
                            g_menu.CampCount(r, tut != 0));
            }
            for (int r = 1; r <= 3; ++r)
                if (byly[r] >= 0 && byly[r] != byly[r == 1 ? 2 : 1]) ++rozne;
            std::printf("  roznych wejsc %d z 3\n", rozne);
        }
        {   // Przyciski rasy nie moga na siebie zachodzic ani wypadac
            // poza ekran, a START musi prowadzic na liste misji.
            int poza = 0, kolizji = 0;
            for (int i = 0; i < 3; ++i) {
                const CampBtn &c = kCampBtn[i];
                if (c.x < 0 || c.y < 0 || c.x + BTN_W > MENU_W
                    || c.y + BTN_H > MENU_H) ++poza;
                for (int j = 0; j < i; ++j) {
                    const CampBtn &d = kCampBtn[j];
                    if (c.x < d.x + BTN_W && d.x < c.x + BTN_W &&
                        c.y < d.y + BTN_H && d.y < c.y + BTN_H) ++kolizji;
                }
            }
            int trafien = 0;
            for (int i = 0; i < 3; ++i) {
                const CampBtn &c = kCampBtn[i];
                if (g_menu.CampHitRace(c.x + BTN_W / 2, c.y + BTN_H / 2) == i)
                    ++trafien;
            }
            g_menu.campRace = 2;
            g_menu.campTutor = false;
            g_menu.CampStart();
            std::printf("  przyciski ras: poza ekranem %d, kolizji %d, "
                        "trafien %d z 3; START rasy 2 -> ekran %d, wpis %d (%s)\n",
                        poza, kolizji, trafien, int(g_menu.screen), g_menu.skSel,
                        g_menu.skSel >= 0 && g_menu.skSel < int(g_menu.skMaps.size())
                            ? g_menu.skMaps[size_t(g_menu.skSel)].title.c_str() : "-");
        }
        {   // **POSTEP KAMPANII.** Mierzone parami i **skutkiem na START**,
            // a nie samym plikiem: dopisanie nazwy do listy wygladaloby tak
            // samo, gdyby START dalej szedl na pierwsza misje.
            //
            // **Audyt nie moze zalezec od prawdziwego postepu gracza** ani go
            // zepsuc. Pierwsza wersja odhaczala "pierwsza misje rasy" i przy
            // pliku, w ktorym ta misja juz byla, mierzyla zupelnie co innego -
            // przebieg po wszystkich trybach pokazal `1 -> 2, z wpisu 25 na 27
            // -> BEZ SKUTKU`. Teraz test odklada liste na bok, pracuje na
            // pustej i na koniec ja oddaje.
            g_menu.CampLoad();
            std::vector<std::string> kopia = g_menu.campDone;
            g_menu.campDone.clear();
            g_menu.CampSaveAll();

            g_menu.campTutor = false;
            g_menu.campRace = 1;
            int pierwsza = g_menu.CampFirst(1, false);
            std::string plik = pierwsza >= 0
                ? g_menu.skMaps[size_t(pierwsza)].file : std::string();
            g_menu.CampStart();
            int przedSel = g_menu.skSel;
            int zrobione0 = g_menu.CampDoneCount(1, false);
            g_menu.CampMarkDone(plik);
            g_menu.screen = SCR_CAMPAIGN;
            g_menu.CampStart();
            int poSel = g_menu.skSel;
            int zrobione1 = g_menu.CampDoneCount(1, false);
            std::printf("  postep kampanii: ukonczonych %d -> %d, "
                        "START z wpisu %d na %d -> %s\n",
                        zrobione0, zrobione1, przedSel, poSel,
                        (zrobione0 == 0 && zrobione1 == 1
                         && poSel == przedSel + 1)
                            ? "przeskakuje na nastepna" : "BEZ SKUTKU");

            // Ekran statystyki: ma sie zlozyc i pokazac wlasna tresc.
            g_menu.screen = SCR_STATS;
            g_menu.Compose();
            if (FILE *o2 = std::fopen("stats.raw", "wb")) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o2);
                std::fclose(o2);
            }
            size_t ink = 0;
            for (uint32_t p : g_menu.canvas) if (p) ++ink;
            std::vector<uint32_t> zeStat = g_menu.canvas;
            g_menu.screen = SCR_SKIRMISH;
            g_menu.Compose();
            size_t inne = 0;
            for (size_t q = 0; q < zeStat.size() && q < g_menu.canvas.size(); ++q)
                if (zeStat[q] != g_menu.canvas[q]) ++inne;
            std::printf("  ekran statystyki: tusz %zu px, rozni sie od listy "
                        "map o %zu px -> %s\n", ink, inne,
                        (ink > 0 && inne > 5000) ? "wlasna tresc"
                                                 : "TO SAMO CO LISTA MAP");
            // Oddaj postep gracza nietkniety.
            g_menu.campDone = kopia;
            g_menu.CampSaveAll();
            g_menu.screen = SCR_CAMPAIGN;
        }

        {   // **Cztery narozne animacje.** Liczy sie nie to, ze rekord sie
            // wczytal, tylko ze widac go na plotnie i ze sie RUSZA - to ta
            // sama lekcja, co przy wrakach ("licznik narysowanych potrafi
            // klamac"). Mierzymy wiec dwie roznice klatek: z animacjami
            // kontra bez, i klatka po kroku kontra przed.
            int maja = 0, pozaEkranem = 0;
            for (int i = 0; i < CAMP_FON; ++i) {
                if (!g_menu.campFon[i].ok()) continue;
                ++maja;
                const POINT &a = kCampFonAt[i];
                if (a.x < 0 || a.y < 0 ||
                    a.x + g_menu.campFon[i].width()  > MENU_W ||
                    a.y + g_menu.campFon[i].height() > MENU_H) ++pozaEkranem;
            }
            g_menu.screen = SCR_CAMPAIGN;
            g_menu.campRace = 1;
            g_menu.Compose();
            std::vector<uint32_t> zNimi = g_menu.canvas;
            flc::Anim schowek[CAMP_FON];
            for (int i = 0; i < CAMP_FON; ++i)
                schowek[i] = std::move(g_menu.campFon[i]);
            g_menu.Compose();
            size_t widac = 0;
            for (size_t q = 0; q < zNimi.size() && q < g_menu.canvas.size(); ++q)
                if (zNimi[q] != g_menu.canvas[q]) ++widac;
            for (int i = 0; i < CAMP_FON; ++i)
                g_menu.campFon[i] = std::move(schowek[i]);
            g_menu.Compose();
            std::vector<uint32_t> przed = g_menu.canvas;
            DWORD t2 = g_menu.lastCampFon;
            for (int i = 0; i < 12; ++i) g_menu.StepCampFon(t2 += FLC_INTERVAL);
            g_menu.Compose();
            size_t rusza = 0;
            for (size_t q = 0; q < przed.size() && q < g_menu.canvas.size(); ++q)
                if (przed[q] != g_menu.canvas[q]) ++rusza;
            std::printf("  narozniki: %d z 4 animacji, poza ekranem %d, "
                        "pikseli na plotnie %zu, po 12 klatkach zmienilo sie %zu\n",
                        maja, pozaEkranem, widac, rusza);
            {   // **Piec srodkowych pasow.** Sprawdzian nie jest z liczby
                // wczytanych rekordow, tylko z **ukladu**: rozmiar kazdego
                // ma sie zgadzac z jego miejscem tak, zeby razem
                // z naroznikami pokryly cala szerokosc **bez szpary
                // i bez zachodzenia**. Gdyby ktoras wspolrzedna albo
                // przypisanie rozmiaru do miejsca bylo zle, ta suma by sie
                // rozjechala - to ten sam sprawdzian, ktory potwierdzil
                // narozniki.
                for (int r = 0; r < 3; ++r) {
                    int ma = 0;
                    struct Pas { int a, b; } pasy[CAMP_MID + CAMP_FON];
                    int np = 0;
                    // Podzial jest w **x**, a pasy zaczynaja sie na roznych
                    // wysokosciach (7, 37, 55) - to uklad schodkowy, wiec
                    // nie ma jednego wiersza, ktory je wszystkie tnie.
                    // Narozniki **01 i 02 powielaja zakresy x** pasow 0 i 1,
                    // tylko nizej (y 181), wiec do podzialu wchodza tylko
                    // **00 i 03**, czyli te siegajace od krawedzi.
                    auto dodaj = [&](int x, int w) {
                        pasy[np].a = x; pasy[np].b = x + w; ++np;
                    };
                    for (int q = 0; q < CAMP_MID; ++q) {
                        if (!g_menu.campMid[r][q].ok()) continue;
                        ++ma;
                        dodaj(kCampMidAt[q].x, g_menu.campMid[r][q].width());
                    }
                    for (int i : { 0, 3 })
                        if (g_menu.campFon[i].ok())
                            dodaj(kCampFonAt[i].x, g_menu.campFon[i].width());
                    for (int i = 1; i < np; ++i)          // po x rosnaco
                        for (int j = i; j > 0
                             && pasy[j].a < pasy[j - 1].a; --j) {
                            Pas t = pasy[j]; pasy[j] = pasy[j - 1];
                            pasy[j - 1] = t;
                        }
                    int szpar = 0, kraniec = np ? pasy[np - 1].b : 0;
                    for (int i = 1; i < np; ++i)
                        if (pasy[i].a != pasy[i - 1].b) ++szpar;
                    std::printf("    rasa %d: srodkowych %d z %d, pasow %d, "
                                "szpar %d, konczy na %d%s\n",
                                r + 1, ma, CAMP_MID, np, szpar, kraniec,
                                (ma == CAMP_MID && szpar == 0
                                 && kraniec == MENU_W)
                                    ? "" : "  <- UKLAD SIE NIE SKLADA");
                }
            }
        }
        std::fflush(stdout);
        return 0;
    }

    // --gra <mapa> [sekundy]: CALA DROGA GRACZA, od menu glownego po ekran
    // wyniku, etap po etapie.
    //
    // Kazdy poprzedni audyt sprawdzal jeden kawalek: `--dumpsk` ekran mapy,
    // `--dumpset` ustawienia, `--dumpload` wczytywanie, `--brief` odprawe,
    // `--sim` potyczke, `--report` wynik. Zaden nie sprawdzal, czy te kawalki
    // **sie ze soba lacza** - a to jest jedyna rzecz, ktora gracz robi za
    // kazdym razem. Ten tryb przechodzi ja **klikajac**: trafienie myszy,
    // obsluga przycisku, przewiniecie animacji przejscia. Nic nie jest
    // wolane obok interfejsu, wiec martwy przycisk albo przejscie, ktore
    // nie dochodzi, konczy etap na "NIE DOSZLO".
    if (argc > 2 && std::strcmp(argv[2], "--gra") == 0) {
        // **`--gra wszystko`** przechodzi te sama droge po **kazdej mapie
        // gry** - 25 potyczek i 33 misje - i daje po jednej linijce na mape.
        // Pojedyncza mapa nie mowi, czy droga gracza dziala **wszedzie**:
        // start blisko krawedzi, brak zloza w zasiegu, misja bez budowniczego
        // - kazde z tych wychodzi dopiero na przebiegu po calosci. Przy okazji
        // jest to sprawdzian, ze stan **czysci sie miedzy mapami**: 58 partii
        // w jednym procesie, jedna po drugiej.
        const bool wszystko = argc > 3 && std::strcmp(argv[3], "wszystko") == 0;
        const int jedna = (argc > 3 && !wszystko) ? std::atoi(argv[3]) : 26;
        const int sekund = argc > 4 ? std::atoi(argv[4]) : (wszystko ? 40 : 300);
        // Rasa gracza 0 WS / 1 BO / 2 SI; -1 zostawia te z mapy. Wchodzi ta
        // sama droga, co klikniecie na ekranie ustawien - przez `setupCiv`.
        const int rasa = argc > 5 ? std::atoi(argv[5]) : -1;
        DWORD zegar = 1000;
        int etapow = 0, doszlo = 0;
        int mapa = jedna;
        std::string pierwszyBlad;
        int mapDobrych = 0, mapWszystkich = 0;
        const int ileMap = int(g_menu.skMaps.size());
        for (int nrMapy = 0; nrMapy < (wszystko ? ileMap : 1); ++nrMapy) {
        if (wszystko) { mapa = nrMapy; etapow = doszlo = 0; pierwszyBlad.clear(); }

        // Przewin animacje przejscia, az ekran naprawde sie zmieni.
        auto przewin = [&](int docelowy, int limit) {
            for (int i = 0; i < limit; ++i) {
                zegar += 40;
                g_menu.Step(zegar);
                g_menu.StepBackgrounds(zegar);
                if (int(g_menu.screen) == docelowy) return true;
            }
            return int(g_menu.screen) == docelowy;
        };
        auto etap = [&](const char *co, bool ok, const char *czym) {
            ++etapow;
            if (ok) ++doszlo;
            else if (pierwszyBlad.empty()) {
                pierwszyBlad = co;
                if (*czym) { pierwszyBlad += ": "; pierwszyBlad += czym; }
            }
            if (!wszystko)
                std::printf("  %-22s %s%s%s\n", co, ok ? "doszlo" : "NIE DOSZLO",
                            *czym ? "  " : "", czym);
        };

        if (!wszystko)
            std::printf("cala droga gracza: mapa %d, %d s rozgrywki\n",
                        mapa, sekund);

        // --- 1. menu glowne -> menu gry dla jednej osoby --------------------
        g_menu.screen = SCR_MENU;
        g_menu.mode = 0;
        g_menu.phase = Menu::PH_IDLE;
        {   POINT p = kButtonPos[0];
            int t = g_menu.HitTest(p.x + BTN_W / 2, p.y + BTN_H / 2);
            if (t == 0) OnPress(0);
            for (int i = 0; i < 400 && g_menu.mode != 1; ++i) {
                zegar += 40;
                g_menu.Step(zegar);
            }
            etap("menu glowne", t == 0 && g_menu.mode == 1,
                 t == 0 ? "" : "przycisk nie lapie kliknieca");
        }

        // --- 2. menu 1 osoby -> ekran wyboru mapy ---------------------------
        {   POINT p = kButtonPos[1];
            int t = g_menu.HitTest(p.x + BTN_W / 2, p.y + BTN_H / 2);
            if (t == 1) OnPress(1);
            bool ok = przewin(SCR_SKIRMISH, 400);
            etap("BITWA", ok, ok ? "" : "nie weszlo na liste map");
        }

        // --- 3. wybor mapy -> ustawienia bitwy ------------------------------
        {   // **Numer mapy PO zakladce** - `SkSetTab` przestawia wybor na
            // poczatek zakresu, wiec odwrotna kolejnosc sklada inna mape.
            g_menu.SkSetTab(mapa >= g_menu.skCustom ? 1 : 0);
            g_menu.skSel = mapa;
            g_menu.SkReveal();
            const RECT &r = kSkButtons[SKB_SETTINGS].r;
            int b = g_menu.SkHitButton(int(r.left + r.right) / 2,
                                       int(r.top + r.bottom) / 2);
            if (b == SKB_SETTINGS) OnSkirmishPress(b);
            bool ok = g_menu.screen == SCR_SETUP;
            char c[96] = "";
            std::vector<const maps::MapSlot *> gn;
            g_menu.SetupSlots(gn);
            if (ok && g_menu.skSel >= 0 && g_menu.skSel < int(g_menu.skMaps.size()))
                std::snprintf(c, sizeof(c), "mapa \"%s\", gniazd %d",
                              g_menu.skMaps[size_t(g_menu.skSel)].title.c_str(),
                              int(gn.size()));
            etap("USTAWIENIA", ok, c);
        }

        // --- 4. ustawienia bitwy -> ekran wczytywania -----------------------
        {   // **Ekran ustawien dzieli deske z ekranem kampanii**, wiec jego
            // przyciski to `kCampBar`, a nie `kSkButtons`: trzy gniazda
            // zamiast pieciu i inna numeracja. Trafienie liczone prostokatem
            // z listy map nie moglo wypasc na START i klikniecie nie szlo
            // nigdzie - audyt pokazywal to jako martwy przycisk gry.
            if (rasa >= 0 && rasa <= 2)
                for (int i = 0; i < Menu::MAX_PLAYERS; ++i)
                    g_menu.setupCiv[i] = rasa;
            const RECT &r = g_menu.msgRect[kCampBar[CAMPB_START].slot];
            int b = g_menu.SkHitButton(int(r.left + r.right) / 2,
                                       int(r.top + r.bottom) / 2);
            if (b == CAMPB_START) OnSkirmishPress(b);
            bool ok = g_menu.screen == SCR_LOAD;
            etap("START", ok, ok ? "" : "nie weszlo na ekran wczytywania");
        }

        // --- 5. wczytywanie -> teren albo odprawa ---------------------------
        {   int krokow = 0;
            for (; krokow < 200 && g_menu.screen == SCR_LOAD; ++krokow) {
                zegar += 40;
                g_menu.StepBackgrounds(zegar);
            }
            bool ok = g_menu.screen == SCR_TERRAIN || g_menu.screen == SCR_BRIEF;
            char c[96];
            std::snprintf(c, sizeof(c), "%d krokow konsoli, komorek %d, na %s",
                          krokow, int(g_menu.terr.cells.size()),
                          g_menu.screen == SCR_BRIEF ? "odprawe" : "teren");
            etap("wczytywanie", ok, c);
        }

        // --- 6. odprawa -> teren (tylko misje) ------------------------------
        if (g_menu.screen == SCR_BRIEF) {
            for (int i = 0; i < 40 && g_menu.screen == SCR_BRIEF; ++i) {
                zegar += 40;
                g_menu.StepBackgrounds(zegar);
                g_menu.FullScreenClick();
            }
            etap("odprawa", g_menu.screen == SCR_TERRAIN, "");
        }

        // --- 7. rozgrywka ---------------------------------------------------
        //
        // Tu zaczyna sie wlasciwa gra, i tu tez idziemy **droga gracza**:
        // zaznacz, otworz palete, kliknij gniazdo, wskaz miejsce, poczekaj.
        // Kazdy podetap ma wlasny wiersz, wiec widac, ktory z nich nie
        // dochodzi - a nie jedno "rozgrywka doszlo", ktore przechodzi takze
        // wtedy, gdy przez piec minut nie dzieje sie nic.
        {
            const uint32_t jaSlot = uint32_t(g_menu.me & 7);
            // **Czym gra odmowila.** `PlaceBuilding` podaje powod przez
            // `Say`, wiec zamiast samego "ROZKAZ NIE POSZEDL" przebieg poda
            // jej wlasny komunikat - inaczej trzeba zgadywac, czy chodzi
            // o miejsce, o kase, czy o budowniczego.
            std::string odmowa;
            auto graj = [&](int sek) {
                for (int k = 0; k < sek * 10; ++k) g_menu.StepUnits(zegar += 100);
            };
            auto moich = [&](void) {
                int n = 0;
                for (const Menu::Unit &u : g_menu.units)
                    if ((u.owner & 7) == jaSlot && u.hp > 0) ++n;
                return n;
            };
            // **Budynek liczy sie dopiero GOTOWY.** `hp > 0` ma juz budowa
            // w toku i budynek w koncowej animacji, wiec liczenie po tym
            // konczylo etap "budowa" zanim cokolwiek stanelo - a nastepne
            // etapy szukaly stoczni i laboratorium, ktorych jeszcze nie bylo.
            auto mojeBld = [&](void) {
                int n = 0;
                for (const Menu::Bld &b : g_menu.blds)
                    if ((b.owner & 7) == jaSlot && b.hp > 0 && b.buildLeft <= 0)
                        ++n;
                return n;
            };
            // Postaw budynek typu `tobj` budowniczym i poczekaj, az **stanie**.
            // Czas bierzemy z cennika razy cztery: bez ekstraktora tlenu
            // `OxygenFactor` schodzi do 1/4 i kazda budowa trwa czterokrotnie
            // dluzej. Sztywne 90 s nie starczalo glownemu budynkowi i etap
            // konczyl sie na "nie stanal w czasie" przy sprawnej budowie.
            auto postaw = [&](int tobj, int zapas) {
                int sek = cost::bldPrice(tobj, g_menu.PlayerSide()).secs * 4
                        + 60 + zapas;
                if(Menu::IsChubModule(tobj)) {
                    int hub=-1;
                    for(size_t q=0;q<g_menu.blds.size();++q) {
                        const auto &b=g_menu.blds[q];
                        if((b.owner&7)==jaSlot && b.tobj==83 && b.hp>0 && !g_menu.BldBusy(b))
                            {hub=int(q);break;}
                    }
                    if(hub<0) return -4;
                    g_menu.sel.clear();g_menu.selBld={hub};
                    int x=-1,y=-1;
                    for(int cy=0;cy<g_menu.terr.bh*2 && x<0;++cy)
                        for(int cx=0;cx<g_menu.terr.bw*2;++cx)
                            if(g_menu.ModuleSite(jaSlot,tobj,cx,cy)) {x=cx;y=cy;break;}
                    if(x<0) return -2;
                    if(!g_menu.PlaceModule(tobj,x,y)) return -3;
                    for(int t=0;t<sek;++t) {
                        graj(1);
                        for(const auto &b:g_menu.blds)
                            if((b.owner&7)==jaSlot && int(b.tobj)==tobj && b.x==x && b.y==y
                                && b.hp>0 && !g_menu.BldBusy(b)) return 1;
                    }
                    return 0;
                }
                int bud = -1;
                for (size_t q = 0; q < g_menu.units.size(); ++q) {
                    const Menu::Unit &u = g_menu.units[q];
                    if ((u.owner & 7) != jaSlot || u.hp <= 0) continue;
                    if (cost::sideOfBuilder(int(u.type)) >= 0) { bud = int(q); break; }
                }
                if (bud < 0) return -1;
                // **Kandydatow jest wielu, nie jeden.** `CanBuildAt` mowi
                // tylko, ze na tej komorce budynek sie miesci - a rozkaz
                // wydaje sie **klikajac**, wiec punkt musi jeszcze wypasc
                // w oknie mapy. Przy komorce blisko krawedzi `ClampCamera`
                // nie dosunie widoku i klik laduje pod dolnym paskiem, gdzie
                // `PlaceBuilding` slusznie go odrzuca. Pierwsza wersja brala
                // **najblizsza** komorke i konczyla na "rozkaz nie poszedl"
                // przy zupelnie sprawnej budowie.
                const Menu::Unit &bu = g_menu.units[size_t(bud)];
                std::vector<std::pair<float, POINT>> kand;
                for (int by = 2; by < g_menu.terr.bh - 2; ++by)
                    for (int bx = 2; bx < g_menu.terr.bw - 2; ++bx) {
                        if (!g_menu.CanBuildAt(bx * 2, by * 2, tobj)) continue;
                        float dx = float(bx * 2) - bu.x, dy = float(by * 2) - bu.y;
                        kand.push_back({ dx * dx + dy * dy,
                                         POINT{ bx * 2, by * 2 } });
                    }
                if (kand.empty()) return -2;
                std::sort(kand.begin(), kand.end(),
                          [](const std::pair<float, POINT> &a,
                             const std::pair<float, POINT> &b) {
                              return a.first < b.first;
                          });
                g_menu.sel.assign(1, bud);
                g_menu.selBld.clear();
                bool zlecone = false;
                size_t prob = kand.size() < 40 ? kand.size() : 40;
                for (size_t q = 0; q < prob && !zlecone; ++q) {
                    int px = kand[q].second.x, py = kand[q].second.y;
                    g_menu.buildPick = tobj;    // gniazdo palety tego typu
                    int sx3, sy3;
                    g_menu.CellToScreen(float(px), float(py), sx3, sy3);
                    RECT vp3 = g_menu.Viewport();
                    g_menu.camX += sx3 - int(vp3.left + vp3.right) / 2;
                    g_menu.camY += sy3 - int(vp3.top + vp3.bottom) / 2;
                    g_menu.ClampCamera();
                    g_menu.CellToScreen(float(px), float(py), sx3, sy3);
                    vp3 = g_menu.Viewport();
                    if (sx3 < vp3.left || sx3 >= vp3.right
                        || sy3 < vp3.top || sy3 >= vp3.bottom) continue;
                    g_menu.PlaceBuilding(sx3, sy3);
                    zlecone = g_menu.units[size_t(bud)].bldTobj >= 0;
                }
                g_menu.buildPick = -1;
                if (!zlecone) { odmowa = g_menu.buildMsg; return -3; }
                int przed = mojeBld();
                // Dociagamy do konca: budowa plus zwijanie rusztowania.
                for (int k = 0; k < sek && mojeBld() <= przed; ++k) graj(1);
                return mojeBld() > przed ? 1 : 0;
            };

            // Powod, dla ktorego budynek nie stanal - ta sama numeracja,
            // ktora zwraca `postaw`.
            // **„Nie ma budowniczego" znaczy dwie rozne rzeczy**: mapa go nie
            // dala, albo mielismy go i zginal po drodze. Druga to zwykla
            // rozgrywka - komputer dochodzi do nas w cztery minuty i zabija
            // Konstruktora - a nie usterka lancucha budowy, wiec wiersz musi
            // je rozroznic.
            bool mielismyBud = false;
            auto powod = [&](int r) {
                return r == 1 ? "stoi"
                     : r == -4 ? "BRAK GOTOWEGO HUBA"
                     : r == -1 ? (mielismyBud ? "BUDOWNICZY ZGINAL"
                                              : "BRAK BUDOWNICZEGO")
                     : r == -2 ? "NIE MA GDZIE"
                     : r == -3 ? "ROZKAZ NIE POSZEDL"
                               : "NIE ZDAZYL";
            };

            // 7a. Build the actual submarine producer, not the Silicon Command Hub.
            int glowny = Menu::BoatProducer(g_menu.PlayerSide(),cost::builderFor(g_menu.PlayerSide()));
            int rb = postaw(glowny, 0);
            char c7[200];
            auto budowniczych = [&](void) {
                int n = 0;
                for (const Menu::Unit &u : g_menu.units)
                    if ((u.owner & 7) == jaSlot && u.hp > 0
                        && cost::sideOfBuilder(int(u.type)) >= 0) ++n;
                return n;
            };
            // **Skad wziely sie sily startowe i czy sa kompletne.**
            // Mapa potyczki nie ma ich w pliku, wiec wstawia je
            // `PlaceStartForces`: budowniczy, DWA transportowce i DWIE
            // lodzie bojowe. Brak ktoregokolwiek wychodzil dotad dopiero
            // trzy etapy dalej jako "wydobycie NIE DOSZLO,
            // transportowcow 0" - czyli wygladal jak brak zloza.
            //
            // **Liczby sa z chwili WSTAWIENIA** (`openHaul`/`openFight`
            // spisane tuz po `BuildPlayers`), a nie z tego miejsca: tutaj
            // czesc lodzi zdazyla juz zginac i wiersz zapalalby sie na
            // mapach, ktore uruchomione osobno maja komplet.
            std::printf("    sily startowe: %s, budowniczych %d,"
                        " transportowcow %d, bojowych %d%s\n",
                        g_menu.openFromFile ? "z pliku mapy" : "wstawione",
                        g_menu.openBuild, g_menu.openHaul, g_menu.openFight,
                        (!g_menu.openFromFile
                         && (g_menu.openHaul < 2 || g_menu.openFight < 2))
                            ? "  <- POTYCZKA BEZ KOMPLETU" : "");
            if (rb == 1)
                // **Ilu budowniczych zostalo** - bez tego "BRAK BUDOWNICZEGO"
                // w nastepnych etapach nie mowi, czy zginal w walce, czy
                // zostal zuzyty. Zuzywa sie tylko kapsula Silikonow (typ 25);
                // Konstruktor i Asembler zostaja i buduja dalej.
            {   std::snprintf(c7, sizeof(c7),
                              "TOBJ %d, budynkow %d, budowniczych %d",
                              glowny, mojeBld(), budowniczych());
                mielismyBud = budowniczych() > 0;
            }
            else if (rb == -1) {
                // **Co gracz w ogole ma.** Bez tego "nie ma budowniczego"
                // nie odroznia usterki wczytywania od mapy, ktora go nie
                // daje - a `Pierwsza krew` to misja bojowa i nie daje.
                std::string t;
                for (const Menu::Unit &u : g_menu.units) {
                    if ((u.owner & 7) != jaSlot || u.hp <= 0) continue;
                    char n[16];
                    std::snprintf(n, sizeof(n), "%d ", int(u.type));
                    if (t.find(n) == std::string::npos) t += n;
                }
                std::snprintf(c7, sizeof(c7),
                              "mapa nie daje budowniczego; typy gracza: %s",
                              t.empty() ? "(brak lodzi)" : t.c_str());
            } else
                std::snprintf(c7, sizeof(c7), "%s%s%s",
                              rb == -2 ? "nie ma gdzie postawic"
                            : rb == -3 ? "rozkaz nie poszedl"
                                       : "nie stanal w czasie",
                              odmowa.empty() ? "" : " - gra mowi: ",
                              odmowa.c_str());
            etap("budowa", rb == 1, c7);

            // 7b. PRODUKCJA: zamow lodz z glownego budynku.
            {   int yard = -1;
                for (size_t q = 0; q < g_menu.blds.size(); ++q) {
                    const Menu::Bld &b = g_menu.blds[q];
                    if ((b.owner & 7) == jaSlot && b.hp > 0
                        && int(b.tobj) == glowny && b.buildLeft <= 0) {
                        yard = int(q); break;
                    }
                }
                bool ok = false;
                char c[120] = "nie ma stoczni";
                if (yard >= 0) {
                    g_menu.sel.clear();
                    g_menu.selBld.assign(1, yard);
                    g_menu.palOpen = true;
                    g_menu.palTab = 0;
                    g_menu.palPage = 0;
                    g_menu.Compose();
                    int n = 0; bool isU = false;
                    const int *lst = g_menu.Palette(n, isU);
                    int gniazdo = -1;
                    for (int k = 0; k < n && k < Menu::BLD_SLOTS; ++k)
                        if (lst && g_menu.BuildUnlocked(lst[k], isU)
                            && g_menu.Afford(g_menu.PriceOf(lst[k], isU)))
                        { gniazdo = k; break; }
                    int przed = moich();
                    // **Straznik kolejnosci klikniecia.** Okno palety lezy
                    // NA pasku, wiec klik w jego gniazdo ma dojsc do palety,
                    // a nie do przycisku komend pod spodem. Gdy `BuildWinAt`
                    // nie ma gdzie powiesic okna, siada ono na prawym panelu
                    // komend - i wlasnie tam Silikony (panel 210 px zamiast
                    // 187) traciły kazde zamowienie. Mierzymy to osobno od
                    // samego "lodz wyszla", bo klik moze nie dojsc, a lodz
                    // i tak sie pojawic z innego powodu.
                    bool dochodzi = false;
                    int lapieKomenda = -1;
                    if (gniazdo >= 0) {
                        RECT r2;
                        if (g_menu.BuildSlot(gniazdo, r2)) {
                            int mx = int(r2.left + r2.right) / 2;
                            int my = int(r2.top + r2.bottom) / 2;
                            bool prawy = false;
                            lapieKomenda = g_menu.CmdHit(mx, my, prawy);
                            size_t byloProd = g_menu.prod.size();
                            g_menu.PanelClick(mx, my);
                            dochodzi = g_menu.prod.size() > byloProd;
                        }
                    }
                    if (!wszystko)
                        std::printf("    klik w gniazdo palety: %s%s\n",
                                dochodzi ? "dochodzi" : "NIE DOCHODZI",
                                lapieKomenda >= 0
                                    ? "  (gniazdo lezy na przycisku komend)"
                                    : "");
                    int wKolejce = 0;
                    for (const Menu::Job &jb : g_menu.prod)
                        if (jb.bld == yard) ++wKolejce;
                    g_menu.palOpen = false;
                    graj(240);
                    ok = moich() > przed;
                    std::snprintf(c, sizeof(c),
                                  "paleta %s %d poz., gniazdo %d (id %d), "
                                  "w kolejce %d, lodzi %d -> %d",
                                  isU ? "lodzi" : "BUDYNKOW", n, gniazdo,
                                  (gniazdo >= 0 && lst) ? lst[gniazdo] : -1,
                                  wKolejce, przed, moich());
                }
                etap("produkcja lodzi", ok, c);
            }

            // 7c. WYDOBYCIE: wydobywak na zlozu + magazyn + transportowiec.
            {   // **Kopiemy to, co mapa oferuje.** Nie kazda mapa ma zloze
                // korium w zasiegu - `Clash` nie ma go wcale tam, gdzie
                // siega budowniczy, i etap konczyl sie na "NIE MA GDZIE"
                // przy zupelnie sprawnym lancuchu wydobycia. Metal jest
                // rownie dobrym dowodem, ze lancuch dziala.
                const bool si = g_menu.PlayerSide() == 2;
                const int mag = si ? 96 : 59;                  // magazyn
                const int kKop[2] = { si ? 94 : 57, si ? 100 : 79 };  // kor, met
                int kor0 = g_menu.Me().bank.corium;
                int met0 = g_menu.Me().bank.metal;
                int r1 = -2, ktory = 0;
                for (int q = 0; q < 2 && r1 != 1; ++q) {
                    ktory = q;
                    r1 = postaw(kKop[q], 0);
                }
                int r2 = postaw(mag, 0);
                // Kazda wlasna lodz transportowa dostaje rozkaz wahadla.
                int woz = 0;
                for (size_t q = 0; q < g_menu.units.size(); ++q) {
                    Menu::Unit &u = g_menu.units[q];
                    if ((u.owner & 7) != jaSlot || u.hp <= 0) continue;
                    if (!g_menu.IsHauler(int(u.type))) continue;
                    u.hauling = true;
                    ++woz;
                }
                graj(180);
                int kor1 = g_menu.Me().bank.corium;
                int met1 = g_menu.Me().bank.metal;
                // Metal schodzi tez na budowe, wiec przy kopalni metalu
                // patrzymy na oba liczniki - wystarczy, ze ktorykolwiek rosnie.
                bool plynie = ktory == 0 ? kor1 > kor0 : (met1 > met0 || kor1 > kor0);
                char c[170];
                std::snprintf(c, sizeof(c),
                              "%s %s%s%s, magazyn %s, transportowcow %d (po %d), "
                              "kurs %.1f kratki, sklad %d, korium %d -> %d, metal %d -> %d",
                              ktory == 0 ? "kopalnia korium" : "kopalnia metalu",
                              powod(r1),
                              (r1 == -3 && !odmowa.empty()) ? " - gra mowi: " : "",
                              (r1 == -3 && !odmowa.empty()) ? odmowa.c_str() : "",
                              powod(r2), woz,
                              [&]{   // ilu zostalo PO oknie pomiaru - bez tego
                                     // "transportowcow 2" przy zerowym urobku
                                     // wyglada jak zerwany lancuch, a znaczy
                                     // tylko, ze komputer je w miedzyczasie zabil
                                  int n2 = 0;
                                  for (const auto &u : g_menu.units)
                                      if ((u.owner & 7) == jaSlot && u.hp > 0
                                          && Menu::IsHauler(int(u.type))) ++n2;
                                  return n2;
                              }(),
                              [&]{   // najkrotszy dystans kopalnia-magazyn
                                  float best = -1;
                                  for (const auto &m : g_menu.blds) {
                                      if ((m.owner & 7) != jaSlot) continue;
                                      if (!Menu::IsExtractor(int(m.tobj))) continue;
                                      for (const auto &d2 : g_menu.blds) {
                                          if ((d2.owner & 7) != jaSlot) continue;
                                          if (!Menu::IsDepot(int(d2.tobj))) continue;
                                          float dx = float(m.x) - float(d2.x);
                                          float dy = float(m.y) - float(d2.y);
                                          float d3 = std::sqrt(dx * dx + dy * dy);
                                          if (best < 0 || d3 < best) best = d3;
                                      }
                                  }
                                  return best;
                              }(),
                              [&]{   // ile zostalo w zlozu kopalni
                                  for (const auto &m : g_menu.blds)
                                      if ((m.owner & 7) == jaSlot
                                          && Menu::IsExtractor(int(m.tobj)))
                                          return int(g_menu.MineRemaining(m));
                                  return -1;
                              }(), kor0, kor1, met0, met1);
                etap("wydobycie", r1 == 1 && r2 == 1 && woz > 0 && plynie, c);
            }

            // 7d. BADANIA: laboratorium i pierwsza wolna technologia.
            {   int lab = g_menu.PlayerSide() == 2 ? 84 : 53;
                int rl = 1;
                if (g_menu.PlayerSide() == 2) {
                    bool hub = false;
                    for (const auto &b : g_menu.blds)
                        if ((b.owner & 7) == jaSlot && b.tobj == 83
                            && b.hp > 0 && !g_menu.BldBusy(b)) hub = true;
                    if (!hub) rl = postaw(83, 0);
                }
                if (rl == 1) rl = postaw(lab, 0);
                int zbad0 = g_menu.Me().stTech;
                bool ruszylo = false;
                if (rl == 1) {
                    g_menu.researchBld = -1;
                    for (size_t q = 0; q < g_menu.blds.size(); ++q) {
                        const auto &b = g_menu.blds[q];
                        if ((b.owner & 7) == jaSlot && int(b.tobj) == lab
                            && b.hp > 0 && !g_menu.BldBusy(b)) {
                            g_menu.researchBld = int(q);
                            break;
                        }
                    }
                    g_menu.Me().bank.gold += 5000;   // zeby bylo za co
                    if (g_menu.TechOpenCount() > 0)
                        ruszylo = g_menu.StartResearch(g_menu.TechOpen(0));
                }
                graj(180);
                char c[140];
                std::snprintf(c, sizeof(c),
                              "laboratorium %s, zlecone %s, zbadanych %d -> %d",
                              powod(rl), ruszylo ? "tak" : "NIE",
                              zbad0, g_menu.Me().stTech);
                etap("badania", rl == 1 && ruszylo
                                && g_menu.Me().stTech > zbad0, c);
            }

            // 7e. WALKA: rozkaz ataku na najblizszego obcego.
            {   int strz0 = g_menu.Me().stShots;
                int cel = -1;
                for (size_t q = 0; q < g_menu.units.size(); ++q)
                    if ((g_menu.units[q].owner & 7) != jaSlot
                        && g_menu.units[q].hp > 0) { cel = int(q); break; }
                int ilu = 0;
                if (cel >= 0) {
                    float tx = g_menu.units[size_t(cel)].x;
                    float ty = g_menu.units[size_t(cel)].y;
                    for (size_t q = 0; q < g_menu.units.size(); ++q) {
                        Menu::Unit &u = g_menu.units[q];
                        if ((u.owner & 7) != jaSlot || u.hp <= 0) continue;
                        if (!wep::unitGun(int(u.type)).armed()) continue;
                        u.tgt = cel;
                        u.tgtBld = false;
                        u.tgtOrder = true;
                        u.moving = true;
                        u.tx = tx; u.ty = ty;
                        ++ilu;
                    }
                }
                int obcych0 = 0;
                for (const Menu::Unit &u : g_menu.units)
                    if ((u.owner & 7) != jaSlot && u.hp > 0) ++obcych0;
                graj(180);
                int obcych1 = 0;
                for (const Menu::Unit &u : g_menu.units)
                    if ((u.owner & 7) != jaSlot && u.hp > 0) ++obcych1;
                char c[140];
                std::snprintf(c, sizeof(c),
                              "uzbrojonych %d, strzalow %d, obcych %d -> %d",
                              ilu, g_menu.Me().stShots - strz0, obcych0, obcych1);
                etap("walka", ilu > 0 && g_menu.Me().stShots > strz0, c);
            }

            Menu::Player &ja = g_menu.Me();
            if (!wszystko)
                std::printf("    bilans: lodzie zbudowane %d, stracone %d; "
                        "budynki %d / %d; badania %d; strzaly %d\n",
                        ja.stBuiltU, ja.stLostU, ja.stBuiltB, ja.stLostB,
                        ja.stTech, ja.stShots);
        }

        // --- 8. czy partia konczy sie SAMA -----------------------------------
        {   // Najpierw sprawdzamy, czy gra w ogole umie sie skonczyc bez
            // naszej pomocy: komputer ma dobic przeciwnikow albo nas. To
            // jedyny etap, ktory moze wyjsc "nie zdazyla" bez usterki -
            // dlatego obok stoi czas, a nie samo tak/nie.
            // W trybie zbiorczym krocej - inaczej 58 map po pol godziny
            // symulacji kazda trwaloby dobe.
            const int limitSek = wszystko ? 120 : 1800;
            int ile = 0;
            for (; ile < limitSek * 10 && g_menu.winner < 0; ++ile)
                g_menu.StepUnits(zegar += 100);
            char c[96];
            std::snprintf(c, sizeof(c), "%s po %.0f s",
                          g_menu.winner >= 0 ? "rozstrzygnieta" : "nie zdazyla",
                          double(ile) / 10.0);
            if (!wszystko) std::printf("  %-22s %s\n", "partia sama", c);
        }

        // --- 9. koniec partii -> ekran wyniku -------------------------------
        {   // **Obie drogi maja konczyc sie tym samym ekranem.** Wczesniej
            // etap zakladal zwyciestwo i dobijal obcych - a gdy komputer
            // zdazyl nas wczesniej wykonczyc, po dobiciu nie zostawal nikt,
            // `CheckWin` nie mial kogo oglosic i wiersz konczyl sie na
            // "NIE DOSZLO" bez zadnej usterki w grze.
            // **Partia mogla skonczyc sie wczesniej.** Gdy `CheckWin`
            // rozstrzygnal ja w trakcie ktoregos z poprzednich etapow, ekran
            // wyniku jest juz otwarty i `rptWon` trzyma **tamten** werdykt -
            // a etap 9, ktory dobija obcych, mierzy wtedy stan sprzed swojej
            // wlasnej ingerencji. Bez tego wiersz krzyczal "PLANSZA NIE
            // ZGADZA SIE Z WYNIKIEM" tam, gdzie plansza byla poprawna,
            // tylko z wczesniejszego konca partii.
            const bool bylJuzWynik = g_menu.screen == SCR_REPORT;
            int mam = 0, obcy = 0;
            for (const Menu::Unit &u : g_menu.units) {
                if (u.hp <= 0) continue;
                if ((u.owner & 7) == uint32_t(g_menu.me & 7)) ++mam; else ++obcy;
            }
            for (const Menu::Bld &b : g_menu.blds) {
                if (b.hp <= 0) continue;
                if ((b.owner & 7) == uint32_t(g_menu.me & 7)) ++mam; else ++obcy;
            }
            const bool zyjemy = mam > 0;
            if (zyjemy) {                       // dobijamy obcych: zwyciestwo
                for (Menu::Unit &u : g_menu.units)
                    if ((u.owner & 7) != uint32_t(g_menu.me & 7)) u.hp = 0;
                for (Menu::Bld &b : g_menu.blds)
                    if ((b.owner & 7) != uint32_t(g_menu.me & 7)) b.hp = 0;
            } else {                            // juz przegralismy
                // zostaw jednego obcego, zeby bylo komu wygrac
                bool jeden = false;
                for (Menu::Unit &u : g_menu.units) {
                    if ((u.owner & 7) == uint32_t(g_menu.me & 7)) continue;
                    if (u.hp <= 0) continue;
                    if (jeden) u.hp = 0; else jeden = true;
                }
                for (Menu::Bld &b : g_menu.blds)
                    if ((b.owner & 7) != uint32_t(g_menu.me & 7) && jeden) b.hp = 0;
            }
            g_menu.Reap();
            g_menu.CheckWin();
            // **Ekran wyniku ma wlasna zwloke** (`REPORT_DELAY` 2,5 s), zeby
            // bylo widac ostatni wybuch - wiec trzeba ja przewinac.
            for (int i2 = 0; i2 < 200 && g_menu.screen != SCR_REPORT; ++i2)
                g_menu.StepReport(0.1f);
            bool ok = g_menu.screen == SCR_REPORT;
            // Gdy wynik zapadl wczesniej, plansza ma sie zgadzac z **tamtym**
            // rozstrzygnieciem, czyli z tym, czy zwycieca to my.
            bool oczekiwana = bylJuzWynik
                ? (g_menu.winner == (g_menu.me & 7)) : zyjemy;
            bool zgodny = ok && (g_menu.rptWon == oczekiwana);
            char c[190];
            std::snprintf(c, sizeof(c),
                          "%s%s, plansza %s, zwycieca gniazdo %d (gramy %d)%s",
                          bylJuzWynik ? "partia skonczyla sie wczesniej, " : "",
                          zyjemy ? "zyjemy" : "juz przegralismy",
                          g_menu.rptWon ? "ZWYCIESTWA" : "porazki",
                          g_menu.winner, g_menu.me & 7,
                          zgodny ? "" : "  <- PLANSZA NIE ZGADZA SIE Z WYNIKIEM");
            etap("koniec partii", ok && zgodny, c);
        }

        if (wszystko) {
            ++mapWszystkich;
            if (doszlo == etapow) ++mapDobrych;
            std::printf("%3d %-26.26s %2d/%2d  %s\n", mapa,
                        (mapa >= 0 && mapa < ileMap)
                            ? g_menu.skMaps[size_t(mapa)].title.c_str() : "-",
                        doszlo, etapow,
                        doszlo == etapow ? "" : pierwszyBlad.c_str());
        } else {
            std::printf("razem: %d z %d etapow\n", doszlo, etapow);
        }
        std::fflush(stdout);
        }   // koniec petli po mapach
        if (wszystko)
            std::printf("RAZEM: %d z %d map przechodzi cala droge\n",
                        mapDobrych, mapWszystkich);
        std::fflush(stdout);
        return 0;
    }

    // --dumpsk composes the map setup screen instead of the menu.
    if (argc > 2 && std::strcmp(argv[2], "--dumpsk") == 0) {
        g_menu.screen = SCR_SKIRMISH;
        if (argc > 4) g_menu.SkSetTab(std::atoi(argv[4]) ? 1 : 0);
        g_menu.skSel = g_menu.SkFirst() + ((argc > 3) ? std::atoi(argv[3]) : 0);
        g_menu.SkReveal();
        // Deska `MMsgTy` wysuwa sie z rogow jak przyciski menu glownego,
        // wiec zrzut trzeba doprowadzic do stanu spoczynku - inaczej pokaze
        // klatke zerowa, czyli goly wysiegnik.
        {   DWORD t = 0;
            for (int i = 0; i < 200 && g_menu.MsgStep(t += g_menu.interval); ++i) {}
        }
        g_menu.Compose();
        FILE *o = std::fopen("compose.raw", "wb");
        if (o) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        std::printf("map: %d (potyczki %d, misje %d), zakladka %s, "
                    "wybrana %d, compose.raw %dx%d\n",
                    int(g_menu.skMaps.size()), g_menu.skCustom,
                    int(g_menu.skMaps.size()) - g_menu.skCustom,
                    g_menu.skTab ? "MISJE" : "POTYCZKA",
                    g_menu.skSel, SCREEN_W, SCREEN_H);
        {   // Deska menu: gniazdo ma **pole trafien z exe**, a tusz grafiki
            // swoj wlasny obrys - jedno musi lezec w drugim, inaczej albo
            // klika sie pustke, albo tablica nie reaguje tam, gdzie widac.
            int z = 0, bezTuszu = 0, kolizji = 0, napisPoza = 0, zwis = 0;
            for (int q = 0; q < MSG_SLOTS; ++q) {
                if (g_menu.msgCount[q] <= 0) continue;
                ++z;
                const RECT &h = g_menu.msgRect[q], &k = g_menu.msgInk[q];
                // Pole trafien ma lezec NA tablicy - inaczej klika sie
                // goly wysiegnik albo tlo obok niej.
                if (!(h.left < k.right && k.left < h.right &&
                      h.top < k.bottom && k.top < h.bottom)) ++bezTuszu;
                int d = 0;
                if (k.left   - h.left   > d) d = k.left   - h.left;
                if (k.top    - h.top    > d) d = k.top    - h.top;
                if (h.right  - k.right  > d) d = h.right  - k.right;
                if (h.bottom - k.bottom > d) d = h.bottom - k.bottom;
                if (d > zwis) zwis = d;
                for (int p = 0; p < q; ++p) {
                    const RECT &o = g_menu.msgRect[p];
                    if (g_menu.msgCount[p] > 0 &&
                        h.left < o.right && o.left < h.right &&
                        h.top < o.bottom && o.top < h.bottom) ++kolizji;
                }
            }
            // **Napis musi wypasc na tablicy**, nie obok niej - to jedyna
            // rzecz, ktora widac natychmiast, gdy gniazdo jest zle dobrane.
            for (int i = 0; i < SK_BTN_COUNT; ++i) {
                RECT L = g_menu.SkLabelBox(i);
                const RECT &k = g_menu.msgInk[kSkButtons[i].slot];
                int cx = (L.left + L.right) / 2, cy = (L.top + L.bottom) / 2;
                if (cx < k.left || cx >= k.right || cy < k.top || cy >= k.bottom)
                    ++napisPoza;
            }
            std::printf("deska menu: %d z %d gniazd ma grafike, deska %d klatek, "
                        "pole bez tuszu %d, zwis %d px, kolizji %d, "
                        "napis poza tablica %d\n",
                        z, MSG_SLOTS, g_menu.tabloCount, bezTuszu, zwis,
                        kolizji, napisPoza);
            std::printf("  uzyte gniazda:");
            for (int i = 0; i < SK_BTN_COUNT; ++i)
                std::printf(" %d=%s", kSkButtons[i].slot,
                            kMsgSlot[kSkButtons[i].slot].rec);
            std::printf("\n");
        }
        {   // Przyciski trybu 1: KAMPANIA, BITWA, WCZYTAJ, DODATKOWE,
            // SAMOUCZEK. Sprawdzamy, ktore prowadza na WLASNE miejsce
            // listy - przycisk, ktory tylko wraca do menu, jest martwy.
            // KAMPANIA i SAMOUCZEK nie skacza juz wprost na liste - oba
            // prowadza na `SCR_CAMPAIGN`, czyli wybor rasy, bo w grze
            // robi to jedna klasa `CampaignTy` z dwoma tytulami.
            int ileTut = 0;
            for (int k = g_menu.skCustom; k < int(g_menu.skMaps.size()); ++k)
                if (g_menu.skMaps[size_t(k)].file.compare(0, 5, "tutor") == 0)
                    ++ileTut;
            std::printf("menu 1 osoby: KAMPANIA i SAMOUCZEK -> wybor rasy; "
                        "kampanii %d+%d+%d map, samouczkow %d\n",
                        g_menu.CampCount(1, false), g_menu.CampCount(2, false),
                        g_menu.CampCount(3, false), ileTut);

            // **Kazdy z pieciu przyciskow podmenu, nacisniety naprawde.**
            // Sama lista etykiet niczego nie dowodzi - to lustro obslugi,
            // a nie sprawdzian; przycisk bez reakcji przez nia przelatuje.
            // Dlatego audyt idzie droga gracza: trafienie myszy, `OnPress`,
            // przewiniecie animacji zwijania - i dopiero potem pyta, na
            // ktorym ekranie wyladowal.
            //
            // Docelowe ekrany sa **odczytane z exe**, nie przyjete:
            // `SetMode` wpisuje kazdemu przyciskowi wlasny komunikat pod
            // `+0x10` jego rekordu (krok 0x1fb) i parametr pod `+0x14`,
            // a `NoneMainMenu` wysyla go, gdy guzik sie schowa. Wychodzi:
            // KAMPANIA 0x6122/0, BITWA 0x611f/2, WCZYTAJ 0x611f/9,
            // DODATKOWE 0x611f/3, SAMOUCZEK 0x6122/1. `StartSystemTy`
            // zamienia 0x6122 na klase 0x307 (`CampaignTy`), a 0x611f na
            // 0x305 (`ChooseMapTy`), ktory parametrem wybiera katalog:
            // 2 -> CUSTOM, 3 -> MISSIONS, 9 -> SAVEGAME.
            {
                // Nazwy i cele **obu** menu. Dla menu glownego numery
                // komunikatow tez sa z exe: `SetMode` w trybie 0 wpisuje
                // 0x6944 (przejscie w podmenu), 0x6105, 0x6103 i 0x7102,
                // a przyciskowi **3 zostawia zero** - on jako jedyny idzie
                // od razu przez `vtbl+0x10` z klasa 0x309 (`SIDTy`).
                static const char *kNaz[2][5] = {
                    { "1 OSOBA", "SIEC", "STATYSTYKA", "WYBIERZ GRACZA",
                      "ZAKONCZ" },
                    { "KAMPANIA", "BITWA", "WCZYTAJ", "DODATKOWE",
                      "SAMOUCZEK" }
                };
                // -1 ma zostac w menu, -2 nie sprawdzamy (ZAKONCZ otwiera
                // okno systemowe, a GRA SIECIOWA jest swiadomie pominieta).
                static const int kEkr[2][5] = {
                    { -1, -2, SCR_STATS, SCR_PLAYER, -2 },
                    { SCR_CAMPAIGN, SCR_SKIRMISH, -1, SCR_SKIRMISH,
                      SCR_CAMPAIGN }
                };
                static const int kZak[2][5] = {
                    { -1, -1, -1, -1, -1 }, { -1, 0, -1, 1, -1 }
                };
                // Po nacisnieciu 0 w menu glownym ma sie zmienic **tryb**,
                // a nie ekran - to tez jest wynik i tez trzeba go sprawdzic.
                static const int kTryb[2][5] = {
                    { 1, -1, -1, -1, -1 }, { -1, -1, -1, -1, -1 }
                };
                std::string zap = g_menu.sidName;
                for (int tryb = 0; tryb < 2; ++tryb) {
                    int zgodnych = 0, badanych = 0;
                    std::string opis;
                    for (int b = 0; b < 5; ++b) {
                        if (kEkr[tryb][b] == -2) continue;
                        ++badanych;
                        g_menu.screen = SCR_MENU;
                        g_menu.mode   = tryb;
                        g_menu.phase  = Menu::PH_IDLE;
                        g_menu.skTab  = 0;
                        POINT p = kButtonPos[b];
                        int t = g_menu.HitTest(p.x + BTN_W / 2,
                                               p.y + BTN_H / 2);
                        if (t == b) OnPress(b);
                        DWORD zeg = 1000;
                        for (int k = 0; k < 400; ++k) {
                            zeg += 40;
                            g_menu.Step(zeg);
                            if (g_menu.screen != SCR_MENU) break;
                            if (g_menu.mode != tryb) break;
                        }
                        int ekr = int(g_menu.screen);
                        bool ok = kEkr[tryb][b] < 0
                                ? (ekr == SCR_MENU)
                                : (ekr == kEkr[tryb][b]
                                   && (kZak[tryb][b] < 0
                                       || g_menu.skTab == kZak[tryb][b]));
                        if (ok && kTryb[tryb][b] >= 0)
                            ok = g_menu.mode == kTryb[tryb][b];
                        if (ok) ++zgodnych;
                        const char *gdzie =
                              ekr == SCR_CAMPAIGN
                                  ? (g_menu.campTutor ? "samouczek"
                                                      : "kampania")
                            : ekr == SCR_SKIRMISH
                                  ? (g_menu.skTab ? "misje" : "potyczki")
                            : ekr == SCR_STATS  ? "statystyka"
                            : ekr == SCR_PLAYER ? "gracze"
                            : ekr == SCR_MENU
                                  ? (g_menu.mode == tryb ? "menu"
                                     : g_menu.mode ? "podmenu" : "menu gl.")
                            : "?";
                        char c[96];
                        std::snprintf(c, sizeof(c), "%s%s->%s",
                                      opis.empty() ? "" : " ",
                                      kNaz[tryb][b], gdzie);
                        opis += c;
                    }
                    std::printf("  %s: %s\n",
                                tryb ? "podmenu" : "menu glowne",
                                opis.c_str());
                    std::printf("    trafia tam, gdzie mowi exe: %d z %d%s\n",
                                zgodnych, badanych,
                                zgodnych == badanych
                                    ? "" : "  <- ktorys przycisk MARTWE");
                }
                // **Sam numer ekranu nie dowodzi, ze cokolwiek widac.**
                // Mierzymy roznice klatki miedzy menu a ekranem gracza -
                // ta sama lekcja, co przy wrakach i przy zegarze.
                {
                    g_menu.screen = SCR_MENU;
                    g_menu.mode = 0;
                    g_menu.Compose();
                    std::vector<uint32_t> bezNiego = g_menu.canvas;
                    g_menu.OpenPlayers();
                    for (int k = 0; k < 40; ++k) g_menu.MsgStep(1000u + 40u * DWORD(k));
                    g_menu.Compose();
                    size_t inne = 0;
                    for (size_t q = 0; q < bezNiego.size()
                                    && q < g_menu.canvas.size(); ++q)
                        if (bezNiego[q] != g_menu.canvas[q]) ++inne;
                    std::printf("    ekran gracza: profili %d, panel %d klatek,"
                                " pikseli roznicy %d%s\n",
                                int(g_menu.sidList.size()), g_menu.sidCount,
                                int(inne),
                                inne == 0 ? "  <- NIC NIE WIDAC" : "");
                }
                // Profil ma przestawiac plik postepu - to jest ten pojemnik,
                // o ktorym mowila notatka przy STATYSTYCE KAMPANII. Test
                // zaklada wlasny katalog i po sobie sprzata.
                {
                    std::string bylo = g_menu.CampFile();
                    bool zal = g_menu.SidCreate("AUDYT_TEST");
                    g_menu.SidUse("AUDYT_TEST");
                    std::string teraz = g_menu.CampFile();
                    g_menu.SidUse("");
                    bool skas = g_menu.SidDelete("AUDYT_TEST");
                    std::printf("    profil: zalozony %s, plik postepu "
                                "\"%s\" -> \"%s\"%s, sprzatniety %s\n",
                                zal ? "tak" : "NIE", bylo.c_str(),
                                teraz.c_str(),
                                bylo == teraz ? "  <- BEZ ZMIANY" : "",
                                skas ? "tak" : "NIE");
                }
                g_menu.screen = SCR_MENU;
                g_menu.mode = 0;
                g_menu.sidName = zap;
                g_menu.campLoaded = false;
                g_menu.campDone.clear();
                g_menu.CampLoad();
            }
        }
        {   // Lista pokazuje TYTUL z mapy, nie nazwe pliku - i to trzeba
            // zmierzyc, bo obie sa napisami i pomylka nie wywala niczego.
            int zTytulem = 0, zCelami = 0, rozne = 0;
            for (const maps::Entry &m : g_menu.skMaps) {
                if (!m.title.empty()) ++zTytulem;
                if (!m.goals.empty()) ++zCelami;
                if (!m.title.empty() && m.title != m.file) ++rozne;
            }
            std::printf("  tytuly: %d z %d map ma TITLE_MISSION, "
                        "%d rozni sie od nazwy pliku; z celami %d\n",
                        zTytulem, int(g_menu.skMaps.size()), rozne, zCelami);
            int a = g_menu.SkFirst(), b = g_menu.SkLast();
            for (int k = a; k < a + 3 && k < b; ++k)
                std::printf("    %d: %-28s (plik %s, celow %d)\n", k,
                            g_menu.skMaps[size_t(k)].title.c_str(),
                            g_menu.skMaps[size_t(k)].file.c_str(),
                            int(g_menu.skMaps[size_t(k)].goals.size()));
        }
        return 0;
    }

    // --terrain N [warstwa] composes the terrain view for one map.
    if (argc > 3 && std::strcmp(argv[2], "--terrain") == 0) {
        // Skirmish maps place no units of their own, so the preview also
        // lists the campaign missions, which do place them explicitly.
        std::vector<maps::Entry> miss = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), miss.begin(), miss.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) {
            std::printf("nie udalo sie wczytac terenu\n");
            return 1;
        }
        if (argc > 4) g_menu.terrLevel = std::atoi(argv[4]);
        if (argc > 8) {                       // rozmiar plotna, zanim ustawimy kamere
            g_clientW = std::atoi(argv[7]);
            g_clientH = std::atoi(argv[8]);
        }
        if (argc > 5) { g_menu.ApplyZoom(std::atoi(argv[5])); g_menu.CentreCamera(); }
        if (argc > 6) g_menu.CameraOnStart();
        g_menu.Compose();
        FILE *o = std::fopen("compose.raw", "wb");
        if (o) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        std::printf("plotno %dx%d\n", SCREEN_W, SCREEN_H);
        std::printf("%s: %d komorek, %dx%d blokow, warstwy", g_menu.terrName.c_str(),
                    int(g_menu.terr.cells.size()), g_menu.terr.bw, g_menu.terr.bh);
        for (int i = 0; i < maps::LEVELS; ++i) std::printf(" %d:%d", i, g_menu.terr.perLevel[i]);
        std::printf("\n  jednostki: %s, crui=%s, obiektow=%d",
                    g_menu.unitSet.ok() ? "TAK" : "NIE",
                    g_menu.unitSet.boat("crui") ? "TAK" : "NIE",
                    int(g_menu.terr.objects.size()));
        g_menu.Compose();
        std::printf("  budynki z grafika: %d, bez: %d", g_menu.bldArt, g_menu.bldMiss);
        std::printf("\n  BOAT=%d rekordow, OBJECT=%d, crui ma %d bajtow",
                    int(g_menu.unitSet.boatCount()), int(g_menu.unitSet.objCount()),
                    int(g_menu.unitSet.boatBytes("crui")));
        // What the map puts down: buildings (type 1000, +20 is the TOBJ id)
        // and, on a mission, units (type 20, +24 is 1..40 into the name table).
        // A '+' marks a unit this remake spawned rather than read.
        for (uint32_t pl = 0; pl < 8; ++pl) {
            bool any = false;
            for (const maps::Object &o : g_menu.terr.objects) {
                if (o.owner != pl) continue;
                if (o.type == maps::OBJ_BUILDING) {
                    if (!any) { std::printf("\n  gracz %u:", pl); any = true; }
                    const char *b = units::buildingName(int(o.subtype), 0);
                    if (!b) b = units::buildingName(int(o.subtype), 1);
                    if (!b) b = units::buildingName(int(o.subtype), 2);
                    std::printf("  (%d,%d) %s%s%s", o.x, o.y, b ? b : "?",
                                o.name.empty() ? "" : ":", o.name.c_str());
                } else if (o.type == maps::OBJ_UNIT) {
                    if (!any) { std::printf("\n  gracz %u:", pl); any = true; }
                    int ut = int(o.subtype);
                    const char *n = units::unitName(ut);
                    std::string lbl = Text(units::unitNameId(ut));
                    for (char &ch : lbl) if (ch == 10 || ch == 13) ch = 32;
                    std::printf("  (%d,%d) %s%s/%s", o.x, o.y, o.spawned ? "+" : "",
                                n ? n : "?", lbl.c_str());
                }
            }
        }
        std::printf("\n");
        return 0;
    }

    // --obj <archiwum> <rekord> opisuje pasek sprite'ow.
    if (argc > 4 && std::strcmp(argv[2], "--obj") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        g_menu.skSel = 0;
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        bool boat = std::strcmp(argv[3], "boat") == 0;
        const spr::Strip *st = boat ? g_menu.unitSet.boat(argv[4])
                                    : g_menu.unitSet.object(argv[4]);
        if (!st) { std::printf("%s: brak rekordu albo pusty\n", argv[4]); return 1; }
        std::printf("%s: %d klatek\n", argv[4], int(st->count()));
        for (int i = 0; i < int(st->count()) && i < 32; ++i) {
            const spr::Frame *f = st->at(i);
            if (!f) { std::printf("  [%d] nullptr\n", i); continue; }
            std::printf("  [%d] %dx%d na plotnie %dx%d w (%d,%d) ok=%d\n",
                        i, f->w, f->h, f->canvasW, f->canvasH, f->x, f->y, int(f->ok()));
        }
        return 0;
    }

    // --frames <rekord> uklada wszystkie klatki paska w siatce, do kalibracji
    // kierunkow: trzeba wiedziec, ktora klatka patrzy w ktora strone.
    if (argc > 3 && std::strcmp(argv[2], "--frames") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        g_menu.skSel = argc > 4 ? std::atoi(argv[4]) : 0;
        if (!g_menu.OpenTerrain(g_gameDir)) return 1;
        const spr::Strip *st = g_menu.unitSet.boat(argv[3]);
        if (!st) st = g_menu.unitSet.object(argv[3]);
        if (!st) st = g_menu.unitSet.misc(argv[3]);
        if (!st && g_menu.natSet.ok()) st = g_menu.natSet.strip(argv[3]);
        if (!st) st = g_menu.GetTvStrip(argv[3]);       // paski z CONTROLG
        if (!st && g_menu.landSet.ok()) st = g_menu.landSet.strip(argv[3]);  // dekoracje
        if (!st) { std::printf("brak %s (BOAT=%d OBJECT=%d OTHER=%d)\n", argv[3],
                               int(g_menu.unitSet.boatCount()), int(g_menu.unitSet.objCount()),
                               int(g_menu.unitSet.miscCount())); std::printf("  rekord ma %d bajtow\n", int(g_menu.unitSet.miscBytes(argv[3]))); return 1; }
        int n = int(st->count());
        {   int wmin = 1 << 20, wmax = 0, hmin = 1 << 20, hmax = 0;
            for (int q2 = 0; q2 < n; ++q2) {
                const spr::Frame *fq = st->at(q2);
                if (!fq || !fq->ok()) continue;
                if (fq->w < wmin) wmin = fq->w;
                if (fq->w > wmax) wmax = fq->w;
                if (fq->h < hmin) hmin = fq->h;
                if (fq->h > hmax) hmax = fq->h;
            }
            // Pokrycie kolejnych klatek w kanwie: animacja to ta sama rzecz
            // w ruchu, wiec piksele w duzej czesci sie pokrywaja. Warianty to
            // rozne obiekty i pokrycie jest male.
            double cov = 0;
            int pairs = 0;
            for (int q2 = 0; q2 + 1 < n; ++q2) {
                const spr::Frame *fa = st->at(q2), *fb = st->at(q2 + 1);
                if (!fa || !fb || !fa->ok() || !fb->ok()) continue;
                int both = 0, any = 0;
                for (int yy = 0; yy < fa->canvasH; ++yy)
                    for (int xx = 0; xx < fa->canvasW; ++xx) {
                        int ax2 = xx - fa->x, ay2 = yy - fa->y;
                        int bx2 = xx - fb->x, by2 = yy - fb->y;
                        bool pa = ax2 >= 0 && ay2 >= 0 && ax2 < fa->w && ay2 < fa->h &&
                                  (fa->px[size_t(ay2) * size_t(fa->w) + size_t(ax2)] & 0xFF000000);
                        bool pb = bx2 >= 0 && by2 >= 0 && bx2 < fb->w && by2 < fb->h &&
                                  (fb->px[size_t(by2) * size_t(fb->w) + size_t(bx2)] & 0xFF000000);
                        if (pa || pb) ++any;
                        if (pa && pb) ++both;
                    }
                if (any) { cov += double(both) / double(any); ++pairs; }
            }
            std::printf("%s: klatek %d, szer %d..%d, pokrycie %.2f\n",
                        argv[3], n, wmin, wmax, pairs ? cov / pairs : 0.0);
        }
        if (const spr::Frame *f0 = st->at(0))
            std::printf("%s: %d klatek, klatka 0 x=%d y=%d w=%d h=%d kanwa %dx%d\n",
                        argv[3], n, f0->x, f0->y, f0->w, f0->h, f0->canvasW, f0->canvasH);
        int cols = 8, rows = (n + cols - 1) / cols;
        int cw = 150, ch = 130;
        g_clientW = cols * cw;
        g_clientH = rows * ch;
        g_menu.screen = SCR_TERRAIN;
        g_menu.FitCanvas();
        std::fill(g_menu.canvas.begin(), g_menu.canvas.end(), 0x101820u);
        for (int i = 0; i < n; ++i) {
            const spr::Frame *f = st->at(i);
            if (!f || !f->ok()) continue;
            int cx = (i % cols) * cw + cw / 2;
            int cy = (i / cols) * ch + ch / 2;
            g_menu.BlitFrame(*f, cx, cy, 128, f->canvasW > 0 ? f->canvasW : 180);
            char lbl[8];
            std::snprintf(lbl, sizeof(lbl), "%d", i);
            g_menu.TextAt(g_menu.fontYellow, cx - cw / 2 + 4, cy - ch / 2 + 2, lbl);
        }
        FILE *o = std::fopen("frames.raw", "wb");
        if (o) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        std::printf("frames.raw %dx%d, %d klatek\n", g_clientW, g_clientH, n);
        return 0;
    }

    // --drive <mapa> zaznacza pierwsza jednostke, wysyla ja w osmiu kierunkach
    // i zapisuje po klatce z kazdego - sprawdzenie kierunkow i samego ruchu.
    if (argc > 3 && std::strcmp(argv[2], "--drive") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 320;
        g_clientH = 260;
        g_menu.ApplyZoom(6);
        if (g_menu.units.empty()) { std::printf("brak jednostek\n"); return 1; }
        g_menu.sel.assign(1, 0);
        static const int kDx[8] = {  1,  1,  0, -1, -1, -1,  0,  1 };
        static const int kDy[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
        FILE *o = std::fopen("drive.raw", "wb");
        if (!o) return 1;
        float ox = g_menu.units[0].x, oy = g_menu.units[0].y;
        DWORD t = 0;
        for (int d = 0; d < 8; ++d) {
            Menu::Unit &u = g_menu.units[0];
            u.x = ox; u.y = oy;
            u.tx = ox + float(kDx[d]) * 8.0f;
            u.ty = oy + float(kDy[d]) * 8.0f;
            u.moving = true;
            for (int k = 0; k < 12 && u.moving; ++k) { t += 50; g_menu.StepUnits(t); }
            g_menu.camX = 0; g_menu.camY = 0;
            int sx, sy;
            g_menu.CellToScreen(u.x, u.y, sx, sy);
            g_menu.camX = sx - g_clientW / 2;
            g_menu.camY = sy - g_clientH / 2;
            g_menu.Compose();
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::printf("  dx=%2d dy=%2d -> klatka %2d, poz (%.1f,%.1f)\n",
                        kDx[d], kDy[d], u.dir, u.x, u.y);
        }
        std::fclose(o);
        std::printf("drive.raw: 8 klatek %dx%d\n", g_clientW, g_clientH);
        return 0;
    }

    // --path <mapa> [prob] szuka drogi miedzy losowymi przejezdnymi blokami
    // i sprawdza, czy kazdy krok jest przejezdny i sasiaduje z poprzednim.
    if (argc > 3 && std::strcmp(argv[2], "--path") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        int tries = argc > 4 ? std::atoi(argv[4]) : 200;
        if (argc > 5) g_menu.passLevel = std::atoi(argv[5]);
        int W = g_menu.terr.bw, H = g_menu.terr.bh;
        int open = 0;
        for (int y = 0; y < H; ++y)
            for (int x = 0; x < W; ++x)
                if (g_menu.Passable(x, y)) ++open;
        std::printf("%s: %dx%d blokow, przejezdnych %d (%d%%)\n",
                    g_menu.terrName.c_str(), W, H, open, open * 100 / (W * H));
        unsigned seed = 12345;
        int found = 0, none = 0, bad = 0; long total = 0; int longest = 0; long climb = 0;
        for (int t = 0; t < tries; ++t) {
            int ax, ay, bx, by, guard = 0;
            do { seed = seed * 1103515245u + 12345u; ax = int((seed >> 16) % unsigned(W));
                 seed = seed * 1103515245u + 12345u; ay = int((seed >> 16) % unsigned(H));
            } while (!g_menu.Passable(ax, ay) && ++guard < 500);
            guard = 0;
            do { seed = seed * 1103515245u + 12345u; bx = int((seed >> 16) % unsigned(W));
                 seed = seed * 1103515245u + 12345u; by = int((seed >> 16) % unsigned(H));
            } while (!g_menu.Passable(bx, by) && ++guard < 500);
            std::vector<POINT> pa = g_menu.FindPath(ax, ay, bx, by, g_menu.passLevel);
            if (pa.empty()) { ++none; continue; }
            ++found;
            total += long(pa.size());
            if (int(pa.size()) > longest) longest = int(pa.size());
            int px = ax, py = ay;
            for (const POINT &q : pa) {
                int dz = g_menu.TopAt(int(q.x), int(q.y)) - g_menu.TopAt(px, py);
                climb += (dz > 0 ? dz : -dz);
                int dx = q.x - px, dy = q.y - py;
                if (dx < -1 || dx > 1 || dy < -1 || dy > 1) { ++bad; break; }
                if (!g_menu.Passable(int(q.x), int(q.y))) { ++bad; break; }
                px = int(q.x); py = int(q.y);
            }
            if (px != bx || py != by) { /* dojscie do najblizszego wolnego */ }
        }
        std::printf("  prob %d: znaleziono %d, bez drogi %d, BLEDNYCH %d\n",
                    tries, found, none, bad);
        if (found) std::printf("  srednia dlugosc %ld blokow, najdluzsza %d, suma wznoszenia %ld\n",
                               total / found, longest, climb);
        return 0;
    }

    // --panel <mapa> stawia kilka jednostek, zaznacza je, wydaje rozkaz grupowy
    // i zapisuje klatke z otwartym panelem - podglad calego sterowania.
    // Zrzut jednej klatki z kamera na wskazanej komorce.
    struct DumpHelper {
        static void Shot(const char *file, float cx, float cy)
        {
            int bx, by;
            g_menu.CellToScreen(cx, cy, bx, by);
            g_menu.camX += bx - SCREEN_W / 2;
            g_menu.camY += by - SCREEN_H / 2;
            g_menu.ClampCamera();
            g_menu.Compose();
            FILE *f = std::fopen(file, "wb");
            if (!f) return;
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), f);
            std::fclose(f);
            std::printf("  zrzut %s %dx%d\n", file, SCREEN_W, SCREEN_H);
        }
    };
    #define Dump DumpHelper::Shot

    // --build <mapa>: przejscie calej drogi budowania bez okna - zaznacz
    // budowniczego, wybierz z palety, postaw, przeczekaj, zamow lodz.
    // --sim <mapa> [sekundy]: cala potyczka bez okna. Jedziemy zegarem po
    // kroku 0,1 s przez zadany czas i patrzymy, co z tego wyszlo - kto komu
    // co zniszczyl, ile kto ma, czy ktos wygral. To jest test tego, ze gra
    // w ogole *chodzi*, a nie pojedynczego mechanizmu.
    if (argc > 3 && std::strcmp(argv[2], "--sim") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1400;
        g_clientH = 900;
        g_menu.FitCanvas();          // ustaw SCREEN_W/H przed liczeniem kamery
        g_menu.ApplyZoom(4);
        g_menu.fogOn = false;               // zeby AI obu stron widzialo tak samo
        int secs = argc > 4 ? std::atoi(argv[4]) : 180;
        if (argc > 5) g_menu.aiLevel = std::atoi(argv[5]);

        int u0 = int(g_menu.units.size()), b0 = int(g_menu.blds.size());
        std::printf("%s: %d lodzi, %d budynkow, gracze:",
                    g_menu.skMaps[size_t(g_menu.skSel)].title.c_str(), u0, b0);
        for (int i = 0; i < Menu::MAX_PLAYERS; ++i)
            if (g_menu.players[i].active)
                std::printf(" %d%c", i, "WBS"[g_menu.players[i].civ % 3]);
        std::printf("\n");

        int shotsFired = 0, maxShots = 0, kills = 0;
        bool shot = false;
        int prevU = u0, prevB = b0;
        // StepUnits robi wszystko naraz - ruch, walke, gospodarke, AI - wiec
        // symulacja jedzie dokladnie tym, co chodzi w grze, a nie wycinkiem.
        for (int t = 0; t < secs * 10; ++t) {
            g_menu.StepUnits(DWORD(1000 + t * 100));
            int nu = int(g_menu.units.size()), nb = int(g_menu.blds.size());
            if (nu < prevU) kills += prevU - nu;
            if (nb < prevB) kills += prevB - nb;
            prevU = nu; prevB = nb;
            if (int(g_menu.shots.size()) > maxShots) maxShots = int(g_menu.shots.size());
            shotsFired += int(g_menu.shots.size());
            // Zrzut w chwili, gdy w powietrzu jest najwiecej pociskow -
            // wtedy widac walke, a nie przerwe miedzy salwami.
            if (int(g_menu.shots.size()) >= 3 && !shot) {
                shot = true;
                g_menu.ApplyZoom(6);            // z bliska widac pociski
                float sx = g_menu.shots[0].x, sy = g_menu.shots[0].y;
                // Kamera liczona od zera, a nie przyrostowo - po zmianie zoomu
                // stare camX jest w innej skali i przyrost by nie trafil.
                int wx = 0, wy = 0;
                g_menu.camX = 0;
                g_menu.camY = 0;
                g_menu.CellToScreen(sx, sy, wx, wy);
                g_menu.camX = wx - SCREEN_W / 2;
                g_menu.camY = wy - SCREEN_H / 2;
                g_menu.ClampCamera();
                g_menu.Compose();
                FILE *fo = std::fopen("sim.raw", "wb");
                if (fo) {
                    std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), fo);
                    std::fclose(fo);
                    std::printf("  zrzut sim.raw po %.1f s, %d pociskow w locie\n",
                                t / 10.0, int(g_menu.shots.size()));
                }
            }
            if (g_menu.winner >= 0) {
                std::printf("koniec po %.0f s: wygrywa gracz %d\n", t / 10.0, g_menu.winner);
                break;
            }
        }
        std::printf("po %d s: lodzi %d -> %d, budynkow %d -> %d, zniszczonych %d\n",
                    secs, u0, int(g_menu.units.size()), b0, int(g_menu.blds.size()), kills);
        std::printf("  pociskow naraz do %d, efektow %d\n",
                    maxShots, int(g_menu.fx.size()));
        {   // Komputer ma wlasny stan badan, wiec musi schodzic w dol drzewa:
            // czesc zbadanych powinna miec wymagania.
            int n = 0;
            const tech::Tech *t = tech::list(n);
            int zbad = 0, zWym = 0, graczy = 0;
            for (int k = 0; k < Menu::MAX_PLAYERS; ++k) {
                if (k == (g_menu.me & 7) || !g_menu.players[k].active) continue;
                int mam = 0;
                for (int i = 0; i < n && i < int(g_menu.players[k].techDone.size()); ++i)
                    if (g_menu.players[k].techDone[size_t(i)]) {
                        ++mam;
                        if (t[i].preId[0]) ++zWym;
                    }
                if (mam) ++graczy;
                zbad += mam;
            }
            std::printf("  badania AI: %d zbadanych u %d graczy, z wymaganiami %d\n",
                        zbad, graczy, zWym);
        }
        std::printf("  AI: fal %d, do doku %d, odwrotow %d, badan %d, poziom %d\n",
                    g_menu.aiWaves, g_menu.aiRepairs, g_menu.aiRetreats,
                    g_menu.aiResearch, g_menu.aiLevel);
        for (int i = 0; i < Menu::MAX_PLAYERS; ++i) {
            if (!g_menu.players[i].active || i == (g_menu.me & 7)) continue;
            const Menu::AiState &st = g_menu.aiSt[i];
            std::printf("    gracz %d: faza %s, fal %d, zwiad %s\n",
                        i, st.phase ? "uderza" : "zbiera", st.waveNo,
                        st.scouted ? "wyslany" : "nie");
        }
        {   // Budynki nie moga na siebie nachodzic ani wychodzic poza mape.
            int nachodzi = 0, pozaMapa = 0;
            int W = g_menu.terr.bw * 2, H = g_menu.terr.bh * 2;
            for (size_t a = 0; a < g_menu.blds.size(); ++a) {
                const Menu::Bld &A = g_menu.blds[a];
                int as = g_menu.BldSpan(A);
                if (A.x < 0 || A.y < 0 || A.x + as > W || A.y + as > H) ++pozaMapa;
                for (size_t b = a + 1; b < g_menu.blds.size(); ++b) {
                    const Menu::Bld &B = g_menu.blds[b];
                    int bs = g_menu.BldSpan(B);
                    if (A.x < B.x + bs && B.x < A.x + as &&
                        A.y < B.y + bs && B.y < A.y + as) ++nachodzi;
                }
            }
            int duzych = 0;
            for (const Menu::Bld &B : g_menu.blds)
                if (g_menu.BldSpan(B) > 1) ++duzych;
            std::printf("  budynki: %d par nachodzi, %d poza mapa, odrzuconych %d, na dwoch komorkach %d z %d\n",
                        nachodzi, pozaMapa, g_menu.buildRefused,
                        duzych, int(g_menu.blds.size()));

            {   // **Kazdy budynek musi byc z listy SWOJEJ rasy.** AI mialo
                // zaszyte TOBJ 50 i 51 (glowny budynek i dok LUDZI) dla
                // wszystkich, wiec komputerowy Silikon stawial budynki obcej
                // cywilizacji.
                int obce = 0;
                for (const Menu::Bld &B : g_menu.blds) {
                    int side = g_menu.SideOfBld(B);
                    int ile7 = 0;
                    const int *lista7 = cost::bldList(side, ile7);
                    bool jest = false;
                    for (int i = 0; i < ile7; ++i)
                        if (lista7[i] == int(B.tobj)) { jest = true; break; }
                    if (!jest) {
                        ++obce;
                        if (obce <= 3)
                            std::printf("      gracz %u (civ %d) ma TOBJ %u\n",
                                        B.owner & 7, side, B.tobj);
                    }
                }
                std::printf("  budynki obcej rasy: %d z %d\n",
                            obce, int(g_menu.blds.size()));
            }
        }
        {   // **Wrak zostaje na dnie.** Do tej pory zatopiona lodz i zburzony
            // budynek znikaly bez sladu. Liczymy NARYSOWANE, nie dodane -
            // pasek z bledna nazwa siedzialby w liscie i nie byloby go widac.
            int kadlub = 0, ruina = 0, narys = 0;
            for (const Menu::Wreck &w : g_menu.wrecks) {
                if (w.rec.compare(0, 5, "rubb_") == 0) ++kadlub;
                else if (w.rec.compare(0, 5, "ruin_") == 0) ++ruina;
            }
            g_menu.Compose();
            for (size_t k = 0; k < g_menu.wrecks.size(); ++k)
                narys += g_menu.DrawWrecks(int(k));
            std::printf("  wraki: kadlubow %d, ruin %d, na plotnie %d "
                        "(w kadrze, bo reszta poza ekranem)\n",
                        kadlub, ruina, narys);
            if (!g_menu.wrecks.empty()) {
                // Kamera na pierwszy wrak i zrzut - zeby dalo sie OBEJRZEC,
                // czy szczatki leza tam, gdzie zatonela lodz.
                int wx, wy;
                g_menu.CellToScreen(g_menu.wrecks[0].x, g_menu.wrecks[0].y, wx, wy);
                g_menu.camX += wx - SCREEN_W / 2;
                g_menu.camY += wy - SCREEN_H / 2;
                g_menu.ClampCamera();
                g_menu.Compose();
                FILE *fw = std::fopen("sim_wreck.raw", "wb");
                if (fw) {
                    std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), fw);
                    std::fclose(fw);
                    std::printf("  zrzut sim_wreck.raw %dx%d (kadr na %.0f,%.0f)\n",
                                SCREEN_W, SCREEN_H,
                                double(g_menu.wrecks[0].x),
                                double(g_menu.wrecks[0].y));
                }
            }
        }
        std::printf("  kolizje: lodz w budynku %d razy, lodzie na sobie %d razy\n",
                    g_menu.thruBld, g_menu.overlaps);
        // **Zakleszczenie AI: chce budowac, stac go, nie ma czym.**
        // W tym stanie `AiEconomy` wracalo w kolko galezia `want >= 0`
        // i nigdy nie docieralo do produkcji, wiec gniazdo zamieralo na
        // stale. U ludzi niewidoczne - Konstruktor i Asembler zostaja po
        // budowie; u Silikonow zabojcze, bo kapsula jest jednorazowa.
        {   int zakleszczonych = 0, aiGniazd = 0;
            for (int pl = 0; pl < 8; ++pl) {
                if (!g_menu.players[pl].active || pl == g_menu.me) continue;
                ++aiGniazd;
                int chce = g_menu.AiWanted(uint32_t(pl));
                if (chce < 0) continue;
                cost::Price pr = cost::bldPrice(chce, g_menu.SideOf(uint32_t(pl)));
                if (!g_menu.AffordFor(uint32_t(pl), pr)) continue;
                int bud = 0;
                for (const auto &u : g_menu.units)
                    if ((u.owner & 7) == uint32_t(pl) && u.hp > 0
                        && cost::sideOfBuilder(int(u.type)) >= 0) ++bud;
                if (!bud) ++zakleszczonych;
            }
            std::printf("  AI bez budowniczego przy nieskonczonej liscie:"
                        " %d z %d gniazd%s\n",
                        zakleszczonych, aiGniazd,
                        zakleszczonych ? "  <- ZAKLESZCZONE" : "");
        }
        // **Tlen komputera ludzkiej rasy.** Bez ekstraktora (TOBJ 80)
        // `OxygenFactor` schodzi do 0.25 i gniazdo buduje oraz produkuje
        // czterokrotnie wolniej przez cala partie. Silikony tlenu nie
        // potrzebuja, wiec licza sie tylko gniazda ludzkie.
        {   int ludzkich = 0, bezRigu = 0;
            float najgorsze = 1.0f;
            for (int pl = 0; pl < 8; ++pl) {
                if (!g_menu.players[pl].active || pl == g_menu.me) continue;
                if (g_menu.SideOf(uint32_t(pl)) == 2) continue;
                ++ludzkich;
                float f = g_menu.OxygenFactor(uint32_t(pl));
                if (f < najgorsze) najgorsze = f;
                int rig = 0;
                for (const auto &b : g_menu.blds)
                    if ((b.owner & 7) == uint32_t(pl) && b.tobj == 80) ++rig;
                if (!rig && g_menu.players[pl].bank.oxyNeed > 0) ++bezRigu;
            }
            if (ludzkich)
                std::printf("  AI tlen: gniazd ludzkich %d, bez ekstraktora %d,"
                            " najgorsze tempo %.2f%s\n",
                            ludzkich, bezRigu, najgorsze,
                            bezRigu ? "  <- DUSI SIE" : "");
        }
        std::printf("  gramy graczem %d, AI siegnelo po nasze %d razy\n",
                    g_menu.me, g_menu.aiTrespass);
        for (int i = 0; i < Menu::MAX_PLAYERS; ++i) {
            if (!g_menu.players[i].active) continue;
            int mu = 0, mb = 0;
            for (const Menu::Unit &u : g_menu.units) if ((u.owner & 7) == uint32_t(i)) ++mu;
            for (const Menu::Bld &b : g_menu.blds)   if ((b.owner & 7) == uint32_t(i)) ++mb;
            std::printf("  gracz %d (%c): %d lodzi, %d budynkow\n",
                        i, "WBS"[g_menu.players[i].civ % 3], mu, mb);
            // Rozklad budynkow: ile roznych typow i jak szeroko rozstawione.
            // Komputer, ktory buduje w linii, wychodzi tu jako jeden typ
            // i zerowa rozpietosc w jednej osi.
            int lo = 1 << 30, hi = -(1 << 30), lo2 = 1 << 30, hi2 = -(1 << 30);
            std::vector<int> kinds;
            for (const Menu::Bld &b : g_menu.blds) {
                if ((b.owner & 7) != uint32_t(i)) continue;
                if (b.x < lo) lo = b.x;
                if (b.x > hi) hi = b.x;
                if (b.y < lo2) lo2 = b.y;
                if (b.y > hi2) hi2 = b.y;
                bool seen = false;
                for (int k : kinds) if (k == int(b.tobj)) { seen = true; break; }
                if (!seen) kinds.push_back(int(b.tobj));
            }
            if (mb > 0) {
                std::printf("    kasa: kor %d met %d\n",
                        g_menu.players[i].bank.corium, g_menu.players[i].bank.metal);
            std::printf("    typow %d (", int(kinds.size()));
                for (size_t k = 0; k < kinds.size(); ++k)
                    std::printf("%s%d", k ? "," : "", kinds[k]);
                std::printf("), rozpietosc %dx%d komorek\n",
                            hi - lo + 1, hi2 - lo2 + 1);
            }
        }
        return 0;
    }

    // --gui <mapa>: wszystkie okna rozgrywki. Gra ma na kazde wlasne tlo
    // w DATA\\CONTROLG (BKG_PAUSE, BKG_OPTIONS, BKG_BEHAVIOURW,
    // BKG_FORMATIONW, BKG_RESEARCHW, BKG_UPDATESW, BKG_TRADECENTERW,
    // BKG_INFOCENTERW, BKG_DIPLOMACYW, BKG_HELPW), po jednym na rase.
    if (argc > 3 && std::strcmp(argv[2], "--gui") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = argc > 4 ? std::atoi(argv[4]) : 1600;
        g_clientH = argc > 5 ? std::atoi(argv[5]) : 900;
        g_menu.FitCanvas();
        g_menu.ApplyZoom(4);
        g_menu.fogOn = false;
        g_menu.aiOn = false;
        g_menu.CentreCamera();
        g_menu.sel.clear();
        for (size_t k = 0; k < g_menu.units.size() && g_menu.sel.size() < 3; ++k)
            if ((g_menu.units[k].owner & 7) == uint32_t(g_menu.me & 7))
                g_menu.sel.push_back(int(k));
        if (!g_menu.blds.empty()) g_menu.selBld.assign(1, 0);

        static const char *kName[Menu::WIN_COUNT] = {
            "-", "pauza", "ustawienia", "zachowanie", "szyk", "badania",
            "ulepszenia", "rynek", "infocentrum", "dyplomacja", "pomoc",
            "drzewo", "zapis", "obiekty", "czat", "ustawgry"
        };
        int mam = 0, brak = 0;
        for (int w = Menu::WIN_PAUSE; w < Menu::WIN_COUNT; ++w) {
            g_menu.winOpen = w;
            g_menu.paused = (w == Menu::WIN_PAUSE);
            RECT r;
            bool ok = g_menu.WinRect(w, r);
            g_menu.Compose();
            const panel::Image *bk = g_menu.WinBkg(w);
            const spr::Frame *bs = g_menu.WinBkgStrip(w);
            int bw = bk ? bk->w : (bs ? bs->w : 0);
            int bh = bk ? bk->h : (bs ? bs->h : 0);
            std::printf("%-12s tlo %-6s %3dx%-3d na %d,%d, wierszy %d\n",
                        kName[w], bk ? "DIB" : (bs ? "pasek" : "BRAK"), bw, bh,
                        ok ? int(r.left) : -1, ok ? int(r.top) : -1,
                        g_menu.winRows);
            if (bk || bs) ++mam; else ++brak;
            if (w == Menu::WIN_RESEARCH) {
                FILE *o = std::fopen("gui.raw", "wb");
                if (o) { std::fwrite(g_menu.canvas.data(), 4,
                                     g_menu.canvas.size(), o); std::fclose(o); }
            }
            if (w == Menu::WIN_SETANY) {
                FILE *o = std::fopen("gui_sam.raw", "wb");
                if (o) { std::fwrite(g_menu.canvas.data(), 4,
                                     g_menu.canvas.size(), o); std::fclose(o); }
            }
        }
        g_menu.winOpen = Menu::WIN_NONE;
        g_menu.paused = false;

        // ---- KURSORY: kazdy tryb ma swoj ---------------------------------
        {   static const char *kAll[] = {
                "CUR_ARROW", "CUR_MENU", "CUR_CMD", "CUR_FIRE", "CUR_PATROL",
                "CUR_CAPTURE", "CUR_VIEW", "CUR_LOADOBJ", "CUR_DCBOMBER",
                "CUR_OWNBOAT", "CUR_OWNOBJ", "CUR_NOBUILD", "CUR_CONFIRM",
                "CUR_SLU", "CUR_SUP", "CUR_SRU", "CUR_SLT", "CUR_SRT",
                "CUR_SLD", "CUR_SDN", "CUR_SRD", "CUR_SNO", "CUR_CLOCK",
                "CUR_REPAIR", "CUR_TELEPORT", "CUR_DISMANTLING", "CUR_RC",
                "CUR_UNLOADRC", "CUR_REPLENISH", "CUR_DEFENCE", "CUR_FORMATION",
            };
            int jest = 0, nie = 0;
            std::string brakuje;
            for (const char *n : kAll) {
                if (g_menu.cursors.frames(n) > 0) ++jest;
                else { ++nie; brakuje += n; brakuje += ' '; }
            }
            std::printf("kursory: %d z %d wczytanych%s%s\n", jest,
                        jest + nie, nie ? "  brak: " : "", brakuje.c_str());
        {   // Czym sa IND_PNT: dziewiec znacznikow 3x8 na rase.
            for (int k = 0; k < 9; ++k) {
                char rec[32];
                std::snprintf(rec, sizeof(rec), "IND_PNT_WS_%d", k);
                const panel::Image *im = g_menu.hud.image(rec);
                if (!im || !im->ok()) { std::printf("  %s brak\n", rec); continue; }
                long r = 0, g = 0, b = 0, n = 0;
                for (int y = 0; y < im->h; ++y)
                    for (int x = 0; x < im->w; ++x) {
                        uint8_t ix = im->idx[size_t(y) * size_t(im->w) + size_t(x)];
                        if (!ix) continue;
                        uint32_t c = g_menu.hud.palette()[ix];
                        r += (c >> 16) & 0xFF; g += (c >> 8) & 0xFF; b += c & 0xFF; ++n;
                    }
                if (n) std::printf("  %s %dx%d  sredni %02lX%02lX%02lX\n",
                                   rec, im->w, im->h, r / n, g / n, b / n);
            }
            const panel::Image *bg = g_menu.hud.image("IND_BKG_00");
            std::printf("  IND_BKG_00 %s\n",
                        bg && bg->ok() ? "jest" : "brak");
        }
        {   // Kursor: zaznaczenie samo w sobie NIE zmienia wskaznika.
            // Celownik `CUR_CMD` jest potwierdzeniem rozkazu i pokazuje sie
            // dopiero po kliknieciu celu, a nad kazdym elementem GUI ma byc
            // zwykla strzalka.
            g_menu.sel.clear();
            for (size_t q = 0; q < g_menu.units.size() && g_menu.sel.empty(); ++q)
                if ((g_menu.units[q].owner & 7) == uint32_t(g_menu.me & 7))
                    g_menu.sel.push_back(int(q));
            bool hudBylo = g_menu.hudOn;
            g_menu.hudOn = true;                // bez ramki caly ekran to mapa
            RECT vp = g_menu.Viewport();
            auto kursor = [&](int x, int y) {
                g_menu.mouse.x = x;
                g_menu.mouse.y = y;
                g_menu.oneShot = 0;
                g_menu.PickCursor();
                return g_menu.curName;
            };
            // srodek mapy, z dala od obiektow
            std::string nadMapa = kursor((vp.left + vp.right) / 2,
                                         (vp.top + vp.bottom) / 2);
            std::string nadPaskiem = kursor((vp.left + vp.right) / 2,
                                            g_clientH - 40);
            std::string nadRamka = kursor(4, 4);
            // po rozkazie ruchu kursor ma potwierdzic
            g_menu.OrderMove((vp.left + vp.right) / 2, (vp.top + vp.bottom) / 2);
            std::string poRozkazie = g_menu.curName;
            int trwa = g_menu.oneShot;
            g_menu.oneShot = 0;
            g_menu.hudOn = hudBylo;

            // Zaznaczenie ma przezyc klikniecie w pusta wode. Ramka, ktora
            // nikogo nie zlapala, zostawia poprzedni oddzial.
            size_t przed = g_menu.sel.size();
            g_menu.bandFrom = POINT{ 2, 2 };
            g_menu.bandTo = POINT{ 12, 12 };
            g_menu.SelectInBand();
            size_t poPustej = g_menu.sel.size();
            std::printf("zaznaczenie: przed %d, po ramce w pustce %d -> %s\n",
                        int(przed), int(poPustej),
                        (przed > 0 && poPustej == przed) ? "trzyma" : "ZGUBIONE");

            // Klikniecie w panel nie moze siegac mapy pod spodem.
            {
                RECT vp2 = g_menu.Viewport();
                Menu::BarLay L2;
                bool polkniete = false, rozkaz = false;
                if (g_menu.BarLayout(&L2)) {
                    int x = (vp2.left + vp2.right) / 2, y = L2.y + 60;
                    polkniete = g_menu.BarClick(x, y);
                    rozkaz = g_menu.OverBar(x, y);
                }
                std::printf("panel: klikniecie polkniete %s, rozkaz zablokowany %s\n",
                            polkniete ? "tak" : "NIE", rozkaz ? "tak" : "NIE");
            }
            std::printf("kursor: nad mapa %s, nad paskiem %s, nad ramka %s, "
                        "po rozkazie %s na %d tykow\n",
                        nadMapa.c_str(), nadPaskiem.c_str(), nadRamka.c_str(),
                        poRozkazie.c_str(), trwa);
        }
        {   // System badan: cennik z przewodnika, wymagania, przerwanie.
            int n = 0;
            const tech::Tech *t = tech::list(n);
            int perRasa[3] = { 0, 0, 0 }, bezKosztu = 0, zWymagan = 0, kradliwych = 0;
            int minZ = 999999, maxZ = 0, minS = 999999, maxS = 0;
            for (int i = 0; i < n; ++i) {
                if (t[i].side >= 0 && t[i].side < 3) ++perRasa[t[i].side];
                if (!t[i].gold || !t[i].secs) ++bezKosztu;
                if (t[i].preId[0]) ++zWymagan;
                if (t[i].steal) ++kradliwych;
                if (t[i].gold < minZ) minZ = t[i].gold;
                if (t[i].gold > maxZ) maxZ = t[i].gold;
                if (t[i].secs < minS) minS = t[i].secs;
                if (t[i].secs > maxS) maxS = t[i].secs;
            }
            int tech1 = 0, poziomow = 0, zIkona = 0, zNazwa = 0;
            for (int i = 0; i < n; ++i) {
                if (t[i].level == 1) ++tech1;
                if (t[i].level > poziomow) poziomow = t[i].level;
                if (t[i].icon >= 0 && t[i].icon < 171) ++zIkona;
                if (t[i].name[0]) ++zNazwa;
            }
            std::printf("badania: %d wezlow (WS %d, BO %d, SI %d), technologii %d,"
                        " poziomow do %d\n",
                        n, perRasa[0], perRasa[1], perRasa[2], tech1, poziomow);
            std::printf("  z gry: nazw %d, ikon UPG %d, bez ceny %d\n",
                        zNazwa, zIkona, bezKosztu);
            std::printf("  cennik: %d..%d, czas %d..%d s; z wymaganiami %d, kradliwych %d\n",
                        minZ, maxZ, minS, maxS, zWymagan, kradliwych);

            // Droga gracza: bez wymagan nie ruszy, z wymaganiami tak,
            // a przerwanie oddaje zloto co do sztuki.
            g_menu.Me().techDone.assign(size_t(n), 0);
            g_menu.Me().research.clear();
            int civE = g_menu.Me().civ;
            g_menu.Me().civ = 0;
            g_menu.SyncRace();
            int zablokowana = -1, wolna = -1;
            for (int i = 0; i < n; ++i) {
                if (t[i].side != 0) continue;
                if (t[i].preId[0] && zablokowana < 0) zablokowana = i;
                if (!t[i].preId[0] && wolna < 0) wolna = i;
            }
            g_menu.Me().bank.gold = 100000;
            // **Bez laboratorium `StartResearch` odmawia ZAWSZE**, wiec bez
            // niego ten pomiar mowil tylko tyle, ze laboratorium nie ma -
            // i obie proby wypadaly na „NIE". Widac to bylo dopiero, gdy
            // gramy gniazdem, ktore na tej mapie nie ma zadnego budynku.
            size_t bldPrzed = g_menu.blds.size();
            {
                Menu::Bld lb;
                lb.owner = uint32_t(g_menu.me);
                lb.tobj = 53;                   // TechCenter / Research Lab
                lb.x = 4; lb.y = 4;
                lb.hp = lb.hpMax = 1000;
                g_menu.blds.push_back(lb);
            }
            bool odmowa = zablokowana >= 0 && !g_menu.StartResearch(zablokowana);
            int przed = g_menu.Me().bank.gold;
            bool ruszyla = wolna >= 0 && g_menu.StartResearch(wolna);
            int poStarcie = g_menu.Me().bank.gold;
            g_menu.AbortResearch();
            int poZwrocie = g_menu.Me().bank.gold;
            std::printf("  gracz: bez wymagan %s, wolna %s (koszt %d), zwrot %s\n",
                        odmowa ? "odmowa" : "WPUSZCZONA",
                        ruszyla ? "ruszyla" : "NIE",
                        przed - poStarcie,
                        poZwrocie == przed ? "pelny" : "NIEPELNY");
            while (g_menu.blds.size() > bldPrzed) g_menu.blds.pop_back();
            // Blokada palety: ile pozycji wymaga badania i czy odblokowuje
            // sie po zbadaniu.
            {
                int zBadaniem = 0, dostepnych = 0;
                for (int r2 = 0; r2 < 3; ++r2) {
                    int all = 0;
                    const int *lst = cost::bldList(r2, all);
                    for (int i = 0; i < all; ++i)
                        if (*cost::bldTech(lst[i], r2)) ++zBadaniem;
                }
                for (int i = 1; i <= 40; ++i)
                    if (*cost::unitTech(i)) ++dostepnych;
                std::printf("  blokada: %d budynkow i %d lodzi wymaga badania\n",
                            zBadaniem, dostepnych);

                // Konkretny przypadek: HF Cannon (62) wymaga Hydro-Fusion.
                g_menu.Me().civ = 0;
                g_menu.SyncRace();
                g_menu.Me().techDone.assign(g_menu.Me().techDone.size(), 0);
                bool przed = g_menu.BuildUnlocked(62, false);
                // Nazwy technologii sa teraz polskie, prosto z gry - wymagania
                // budowy dalej chodza po nazwie z przewodnika, wiec szukamy
                // po polu `guide`.
                int hf = -1;
                for (int i = 0; i < n; ++i)
                    if (std::strcmp(t[i].guide, "Hydro-Fusion Technology") == 0) hf = i;
                if (hf >= 0) g_menu.Me().techDone[size_t(hf)] = 1;
                bool po = g_menu.BuildUnlocked(62, false);
                std::printf("  HF Cannon: przed badaniem %s, po %s\n",
                            przed ? "WOLNY" : "zablokowany",
                            po ? "wolny" : "DALEJ ZABLOKOWANY");

                // Skutek z tablicy exe: opoznienie strzalu na polowe.
                // Typ 12 ulepsza **badanie 11**, nie byle jakie.
                g_menu.Me().techDone.assign(g_menu.Me().techDone.size(), 0);
                float bez = g_menu.FireDelayFactor(uint32_t(g_menu.me), 12);
                int u11 = tech::indexOf(11, 1);
                if (u11 >= 0) g_menu.Me().techDone[size_t(u11)] = 1;
                float po2 = g_menu.FireDelayFactor(uint32_t(g_menu.me), 12);
                std::printf("  skutek: opoznienie typu 12 %.2f -> %.2f\n",
                            bez, po2);
                g_menu.Me().techDone.assign(g_menu.Me().techDone.size(), 0);

                // ---- premia x1,5 do predkosci pociskow ----
                //
                // `GetSpeed` (0x00430750) robi `v = (v >> 1) + v` dla rodziny
                // torped, gdy White Sharks maja badanie **153**, a Black
                // Octopi **150**. Gra nazywa oba te wezly **ULEPSZONA
                // SZYBKOSC TORPEDY** (`Torpedo Speed Upgrade`) - czyli odczyt
                // z `GetSpeed`, numery pociskow z tablicy lodzi i wlasna
                // nazwa gry mowia to samo.
                //
                // Mierzone jest to, co **naprawde wylatuje z lufy**: predkosc
                // pocisku w `shots` po przejsciu przez `StepCombat`, a nie
                // wynik `ShotSpeed` wolanego obok gry. Tylko tak widac, czy
                // miejsce wywolania podaje numer pocisku - bez tego premia
                // jest martwa, a kazdy pomiar liczony obok niej wychodzilby
                // tak samo dobrze.
                //
                // Obok torpedy stoi **kontrola**: lodz strzelajaca czyms innym
                // (typ 1, pocisk 0x9f), ktorej to samo badanie nie ma prawa
                // przyspieszyc. Bez niej „po badaniu szybciej" wyszloby tak
                // samo, gdyby premia dotyczyla wszystkiego.
                {
                    const size_t byloU = g_menu.units.size();
                    const uint32_t ja = uint32_t(g_menu.me & 7);
                    const uint32_t obcy = (ja + 1) & 7;
                    for (int i = 0; i < Menu::MAX_PLAYERS; ++i) g_menu.ally[i] = 0;
                    g_menu.Me().civ = 0;            // White Sharks: badanie 153
                    g_menu.SyncRace();
                    const int wTorp = tech::indexOf(153, 1);

                    // **Cel musi byc BEZBRONNY.** Uzbrojony odpowiada ogniem
                    // w tym samym przebiegu petli i to JEGO pocisk zostaje na
                    // koncu `shots` - mierzylibysmy nie te lodz.
                    auto predkosc = [&](int typ, bool zBadaniem) {
                        g_menu.units.resize(byloU);
                        g_menu.shots.clear();
                        g_menu.Me().techDone.assign(
                            g_menu.Me().techDone.size(), 0);
                        if (zBadaniem && wTorp >= 0
                            && wTorp < int(g_menu.Me().techDone.size()))
                            g_menu.Me().techDone[size_t(wTorp)] = 1;
                        Menu::Unit a;
                        a.owner = ja; a.type = uint32_t(typ);
                        a.x = 20; a.y = 20; a.hp = a.hpMax = 1000;
                        g_menu.units.push_back(a);
                        Menu::Unit c;
                        c.owner = obcy; c.type = 12;   // Konstruktor - bez broni
                        c.x = 21; c.y = 20; c.hp = c.hpMax = 100000;
                        g_menu.units.push_back(c);
                        g_menu.units[byloU].tgt = int(byloU + 1);
                        g_menu.units[byloU].tgtOrder = true;
                        g_menu.units[byloU].cool = 0;
                        g_menu.StepCombat(0.05f);
                        return g_menu.shots.empty() ? 0.0f
                                                    : g_menu.shots.back().speed;
                    };
                    float tBez = predkosc(2, false), tPo = predkosc(2, true);
                    float kBez = predkosc(1, false), kPo = predkosc(1, true);
                    bool tOk = tBez > 0 && tPo > tBez * 1.4f;
                    bool kOk = kBez > 0 && kPo == kBez;
                    std::printf("  premia predkosci (ULEPSZONA SZYBKOSC TORPEDY,"
                                " wezel %d): torpeda 0x%02x %.2f -> %.2f %s,"
                                " kontrola 0x%02x %.2f -> %.2f %s\n",
                                wTorp, proj::ofUnit(2), tBez, tPo,
                                tBez <= 0 ? "NIE STRZELILA"
                                          : (tOk ? "szybciej" : "BEZ ZMIANY"),
                                proj::ofUnit(1), kBez, kPo,
                                kBez <= 0 ? "NIE STRZELILA"
                                          : (kOk ? "bez zmian" : "ZMIENILA SIE"));

                    // **Drugi pomiar: sama predkosc, nie premia.** Powyzsze
                    // dwa typy maja etykiete przewodnika zgodna z tablica exe
                    // (48 i 96), wiec gdyby numer pocisku nie dochodzil,
                    // predkosci i tak by sie zgadzaly - obalona bylaby tylko
                    // premia. Exe ma jednak **piec** pasm (48, 60, 72, 96,
                    // 201), a przewodnik cztery kubelki: pasmo **72** nie ma
                    // w nim odpowiednika i „medium" zbiera je razem z 96.
                    //
                    // Typ 10 strzela pociskiem 0xa4, czyli z pasma 72. Etykieta
                    // przewodnika dalaby mu 96. Rozna liczba w tych dwoch
                    // kolumnach jest wiec dowodem, ze predkosc idzie z exe,
                    // a nie z etykiety - i tylko ten jeden pomiar to pokazuje.
                    {
                        wep::Gun g10 = wep::unitGun(10);
                        float zTabl = predkosc(10, false);
                        float zEtyk = Menu::ShotSpeed(g10.pspeed, 0);
                        std::printf("  pasmo z exe: typ 10 pocisk 0x%02x -> %.2f,"
                                    " etykieta przewodnika -> %.2f %s\n",
                                    proj::ofUnit(10), zTabl, zEtyk,
                                    zTabl <= 0 ? "NIE STRZELILA"
                                    : (zTabl == zEtyk
                                       ? "TO SAMO - etykieta wystarczyla"
                                       : "rozne -> liczy sie tablica"));
                    }
                    g_menu.units.resize(byloU);
                    g_menu.shots.clear();
                    g_menu.Me().techDone.assign(g_menu.Me().techDone.size(), 0);
                }
            }
            // Grafika drzewa: tlo na rase i ikona przy kazdym wezle.
            for (int r2 = 0; r2 < 3; ++r2) {
                g_menu.Me().civ = r2;
                g_menu.SyncRace();
                const spr::Frame *bf = g_menu.WinBkgStrip(Menu::WIN_TTREE);
                RECT tr = { 0, 0, 0, 0 };
                g_menu.WinRect(Menu::WIN_TTREE, tr);
                int male = 0, duze = 0, poza = 0, ile = 0;
                for (int i = 0; i < n; ++i) {
                    if (t[i].side != r2) continue;
                    ++ile;
                    if (g_menu.TechIcon(i, false, true)) ++male;
                    if (g_menu.TechIcon(i, true, true)) ++duze;
                    if (t[i].x + 26 > tr.right - tr.left
                        || t[i].y + 14 > tr.bottom - tr.top) ++poza;
                }
                std::printf("  drzewo %s: tlo %dx%d, okno %dx%d, wezlow %d,"
                            " HLP_UPG %d, UPG %d, poza oknem %d\n",
                            r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                            bf ? bf->w : 0, bf ? bf->h : 0,
                            int(tr.right - tr.left), int(tr.bottom - tr.top),
                            ile, male, duze, poza);
            }
            {   // Podzial technologii Silikonow na moduly Command Huba:
                // kazdy numer musi trafic do dokladnie jednego modulu.
                int suma = 0, bez = 0, podwojnie = 0;
                std::printf("  moduly SI:");
                for (int tobj = 84; tobj <= 90; ++tobj) {
                    int c = 0;
                    for (int i = 0; i < n; ++i)
                        if (t[i].side == 2 && Menu::BldResearches(tobj, 2, t[i].id))
                            ++c;
                    suma += c;
                    std::printf(" %d:%d", tobj, c);
                }
                for (int i = 0; i < n; ++i) {
                    if (t[i].side != 2) continue;
                    int gdzie = 0;
                    for (int tobj = 84; tobj <= 90; ++tobj)
                        if (Menu::BldResearches(tobj, 2, t[i].id)) ++gdzie;
                    if (gdzie == 0) ++bez;
                    else if (gdzie > 1) ++podwojnie;
                }
                std::printf("  razem %d z %d, bez modulu %d, w dwoch %d\n",
                            suma, tech::countFor(2), bez, podwojnie);
            }

            // Cala droga gracza: laboratorium -> przycisk BADANIA -> okno
            // -> gniazdo -> badanie rusza -> przerwanie oddaje zloto.
            for (int r2 = 0; r2 < 3; ++r2) {
                g_menu.Me().civ = r2;
                g_menu.SyncRace();
                g_menu.Me().techDone.assign(size_t(n), 0);
                g_menu.Me().research.clear();
                g_menu.Me().bank.gold = 100000;
                g_menu.winOpen = Menu::WIN_NONE;
                g_menu.researchBld = -1;
                g_menu.selBld.clear();
                size_t ile0 = g_menu.blds.size();
                // Bez laboratorium nie wolno badac.
                int wolna = -1;
                for (int i = 0; i < n; ++i)
                    if (t[i].side == r2 && !t[i].preId[0]) { wolna = i; break; }
                bool bezLab = wolna >= 0 && !g_menu.StartResearch(wolna);

                // Stawiamy laboratorium tej rasy i zaznaczamy je.
                int lab = r2 == 2 ? 85 : 53;
                Menu::Bld b;
                b.owner = uint32_t(g_menu.me);
                b.tobj = uint32_t(lab);
                b.x = 4; b.y = 4;
                b.hp = b.hpMax = 1000;
                g_menu.blds.push_back(b);
                g_menu.selBld.push_back(int(g_menu.blds.size()) - 1);

                // Panel komend musi teraz pokazac BADANIA.
                int nc = 0;
                const Menu::Cmd *cs = g_menu.CmdsFor(true, nc);
                int przycisk = -1;
                for (int k = 0; k < nc; ++k)
                    if (std::strstr(cs[k].rec, "RESEARCH")) przycisk = k;
                if (przycisk >= 0) g_menu.CmdAction(true, przycisk);
                bool okno = g_menu.winOpen == Menu::WIN_RESEARCH;
                int wyborow = g_menu.TechOpenCount();

                // Klikniecie w pierwsze gniazdo.
                RECT rw, rs = { 0, 0, 0, 0 };
                bool ruszylo = false, zwrot = false;
                int zl0 = g_menu.Me().bank.gold, koszt = 0;
                if (okno && g_menu.WinRect(Menu::WIN_RESEARCH, rw)
                    && g_menu.ResearchSlot(rw, 0, rs)) {
                    g_menu.WinClick(int(rs.left) + 4, int(rs.top) + 4);
                    ruszylo = !g_menu.Me().research.empty();
                    koszt = zl0 - g_menu.Me().bank.gold;
                    if (r2 == 0 && ruszylo) {   // zrzut okna w trakcie badania
                        g_menu.Me().research.front().left *= 0.4f;
                        g_menu.Compose();
                        if (FILE *o = std::fopen("research.raw", "wb")) {
                            std::fwrite(g_menu.canvas.data(), 4,
                                        g_menu.canvas.size(), o);
                            std::fclose(o);
                            std::printf("  research.raw %dx%d\n",
                                        g_clientW, g_clientH);
                        }
                    }
                    // Pole postepu przerywa i oddaje zloto.
                    g_menu.WinClick(int(rw.left) + Menu::RES_BOX1_X + 4,
                                    int(rw.top) + Menu::RES_BOX_Y + 4);
                    zwrot = g_menu.Me().research.empty()
                            && g_menu.Me().bank.gold == zl0;
                }
                std::printf("  badania %s: bez lab %s, przycisk %s, okno %s,"
                            " wyborow %d, start %s (koszt %d), przerwanie %s\n",
                            r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                            bezLab ? "odmowa" : "WPUSCILO",
                            przycisk >= 0 ? "jest" : "BRAK",
                            okno ? "otwarte" : "ZAMKNIETE", wyborow,
                            ruszylo ? "tak" : "NIE", koszt,
                            zwrot ? "pelny zwrot" : "BEZ ZWROTU");
                g_menu.winOpen = Menu::WIN_NONE;
                g_menu.researchBld = -1;
                g_menu.selBld.clear();
                while (g_menu.blds.size() > ile0) g_menu.blds.pop_back();
                g_menu.Me().techDone.assign(size_t(n), 0);
                g_menu.Me().research.clear();
            }

            // Zrzut samego drzewa, zeby dalo sie je porownac z gra.
            g_menu.Me().civ = 0;
            g_menu.SyncRace();
            g_menu.winOpen = Menu::WIN_TTREE;
            g_menu.Compose();
            if (FILE *o = std::fopen("ttree.raw", "wb")) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
                std::fclose(o);
                std::printf("  ttree.raw %dx%d\n", g_clientW, g_clientH);
            }
            g_menu.winOpen = Menu::WIN_NONE;
            g_menu.Me().civ = civE;
            g_menu.SyncRace();
            g_menu.Me().research.clear();
        }

        {   // Dolny pasek ma reagowac, nie tylko sie rysowac.
            g_menu.sel.clear();
            for (size_t q = 0; q < g_menu.units.size() && g_menu.sel.size() < 3; ++q)
                if ((g_menu.units[q].owner & 7) == uint32_t(g_menu.me & 7))
                    g_menu.sel.push_back(int(q));
            g_menu.Compose();
            Menu::BarLay L;
            int trafienLista = 0, trafienGlab = 0, trafienBld = 0;
            if (g_menu.BarLayout(&L)) {
                // **Lewa lista jest dla lodzi, prawa dla budynkow** -
                // caly pasek dzieli sie tak samo. Prawa dostawala wczesniej
                // **nadmiar lodzi**, wiec przy zaznaczonym budynku stala
                // pusta zawsze i nikt tego nie liczyl. Sprawdzamy obie:
                // najpierw z lodziami, potem z budynkami.
                {   std::vector<int> selWas = g_menu.sel;
                    std::vector<int> bldWas = g_menu.selBld;
                    g_menu.selBld.clear();
                    for (int i2 = 0; i2 < int(g_menu.blds.size())
                                     && int(g_menu.selBld.size()) < 4; ++i2)
                        g_menu.selBld.push_back(i2);
                    for (int side = 0; side < 2; ++side) {
                        int ox = side == 0 ? L.rosterL : L.rosterR;
                        if (ox < 0) continue;
                        for (int k = 0; k < 8; ++k) {
                            int x = ox + 6 + (k % 2) * 50 + 20;
                            int y = L.y + 8 + (k / 2) * 35 + 15;
                            bool bld = false;
                            if (g_menu.RosterHit(x, y, &bld) < 0) continue;
                            if (bld) ++trafienBld; else ++trafienLista;
                        }
                    }
                    g_menu.sel = selWas;
                    g_menu.selBld = bldWas;
                }
                if (L.deep >= 0)
                    for (int k = 0; k < 5; ++k)
                        if (g_menu.DeepHit(L.deep + 8 + k * 14 + 6, L.y + 30) == k)
                            ++trafienGlab;
            }
            // Stany przyciskow: bez zaznaczenia wszystkie maja byc niedostepne.
            int zywych = 0, martwych = 0;
            for (int k = 0; k < 8; ++k) if (g_menu.CmdEnabled(false, k)) ++zywych;
            g_menu.sel.clear();
            for (int k = 0; k < 8; ++k) if (!g_menu.CmdEnabled(false, k)) ++martwych;
            std::printf("pasek: gniazd listy lodzi %d, budynkow %d, segmentow glebokosci %d z 5, "
                        "przyciskow zywych %d, bez zaznaczenia martwych %d\n",
                        trafienLista, trafienBld, trafienGlab, zywych, martwych);

            // ---- ikona uzbrojenia `INF_WEAP_<NN>` ----
            //
            // **Wycofane: `INF_WEAP_<NN> = numer - 0x96`.** To byla
            // arytmetyczna zbieznosc (45 ikon, 45 numerow 0x96..0xc2).
            // Gniazdo daje switch `FUN_005259b0(numer, poziom, flaga)`,
            // a drugi argument to POZIOM ULEPSZENIA 0..4: trzy rodziny
            // torped maja po piec gniazd, reszta po jednym.
            //
            // Pierwsza liczba jest **decydujaca i nie polega na
            // dopasowywaniu**: gniazda ulozone przez ten switch maja
            // pokryc dokladnie 0..44, bez jednej dziury. Przy odczycie
            // bez poziomu wyszloby 33 gniazda i dwanascie dziur, wiec
            // ten sam pomiar obala bledna lekture.
            {
                bool zajete[64] = { false };
                int pozaZakresem = 0, ikonWArchiwum = 0;
                for (int nr = 0x90; nr <= 0xff; ++nr)
                    for (int lv = 0; lv < 5; ++lv) {
                        int g = weapicon::slotFor(nr, lv);
                        if (g < 0) continue;
                        if (g < 64) zajete[g] = true; else ++pozaZakresem;
                    }
                int dziur = 0;
                for (int g = 0; g < weapicon::count(); ++g)
                    if (!zajete[g]) ++dziur;
                for (int g = 0; g < weapicon::count(); ++g) {
                    char rec[32];
                    std::snprintf(rec, sizeof(rec), "INF_WEAP_%02d", g);
                    const spr::Strip *st = g_menu.hud.strip(rec);
                    if (st && st->count() && st->at(0) && st->at(0)->ok())
                        ++ikonWArchiwum;
                }
                // Ile ROZNYCH ikon dostaje czterdziesci typow lodzi -
                // tablica, ktora wszystkim daje to samo gniazdo, wygladalaby
                // w kazdym innym wierszu tak samo jak dzialajaca.
                bool widziane[64] = { false };
                int uzbrojonych = 0, roznych = 0;
                for (int t = 1; t <= 40; ++t) {
                    int g = weapicon::slotFor(proj::ofUnit(t), 0);
                    if (g < 0) continue;
                    ++uzbrojonych;
                    if (g < 64 && !widziane[g]) { widziane[g] = true; ++roznych; }
                }
                std::printf("  ikona uzbrojenia: gniazd %d z %d, dziur %d,"
                            " poza zakresem %d, ikon w archiwum %d;"
                            " lodzi uzbrojonych %d -> roznych ikon %d\n",
                            weapicon::count() - dziur, weapicon::count(), dziur,
                            pozaZakresem, ikonWArchiwum, uzbrojonych, roznych);

                // **Czy ikona dociera na plotno.** Mierzone roznica klatki
                // w pudelku samej ikony: lodz uzbrojona kontra bezbronna
                // (Konstruktor, typ 12). Sam licznik „gniazdo znalezione"
                // nie dowodzilby niczego - rekord moze sie wczytac i nigdzie
                // nie trafic, ta sama lekcja co przy wrakach.
                Menu::BarLay BL;
                if (g_menu.BarLayout(&BL) && BL.infoL >= 0) {
                    int bx = BL.infoL + 5, by = BL.y + g_menu.InfoUpperH() + 47;
                    int bw = 70, bh = 30;
                    size_t byloU2 = g_menu.units.size();
                    auto pudelko = [&](int typ, std::vector<uint32_t> &poza) {
                        g_menu.units.resize(byloU2);
                        Menu::Unit a;
                        a.owner = uint32_t(g_menu.me & 7);
                        a.type = uint32_t(typ);
                        a.x = 20; a.y = 20; a.hp = a.hpMax = 1000;
                        g_menu.units.push_back(a);
                        g_menu.sel.assign(1, int(byloU2));
                        g_menu.selBld.clear();
                        g_menu.Compose();
                        poza.clear();
                        for (int yy = by; yy < by + bh; ++yy)
                            for (int xx = bx; xx < bx + bw; ++xx) {
                                size_t at = size_t(yy) * size_t(SCREEN_W)
                                          + size_t(xx);
                                if (at < g_menu.canvas.size())
                                    poza.push_back(g_menu.canvas[at]);
                            }
                    };
                    std::vector<uint32_t> aTorp, aLaser, aBez;
                    pudelko(2, aTorp);        // 0x96 - torpeda
                    pudelko(18, aLaser);      // 0x9c - laser, inne gniazdo
                    pudelko(12, aBez);        // Konstruktor - bez broni
                    auto rozne = [](const std::vector<uint32_t> &p,
                                    const std::vector<uint32_t> &q) {
                        int n = 0;
                        for (size_t i = 0; i < p.size() && i < q.size(); ++i)
                            if (p[i] != q[i]) ++n;
                        return n;
                    };
                    int jest = rozne(aTorp, aBez), inna = rozne(aTorp, aLaser);
                    std::printf("    na plotnie: uzbrojona kontra bezbronna"
                                " %d px %s, torpeda kontra laser %d px %s\n",
                                jest, jest > 0 ? "rysuje sie" : "NIE RYSUJE SIE",
                                inna, inna > 0 ? "inna ikona" : "TA SAMA IKONA");
                    g_menu.units.resize(byloU2);
                    g_menu.sel.clear();
                }

                // ---- lodz naprawcza pokazuje WIEZIONA lodz ----
                //
                // `PaintInfoBoat` rozgalezia sie po typie: 7, 19, 27 dostaja
                // ikone wiezionego obiektu i pionowy pasek jego stanu, cala
                // reszta - ikone broni. Te trzy numery to dokladnie
                // `IsRepSub`, wiec osobny odczyt potwierdza podzial, ktory
                // remake juz mial.
                //
                // Pierwszy wiersz sprawdza, ze **uklad sie sklada**: ramka
                // musi objac ikone I pasek. To ten sam rodzaj sprawdzianu,
                // co przy filarach ekranu kampanii - gdyby ktorakolwiek
                // wspolrzedna byla przepisana zle, obejmowanie by przepadlo.
                {
                    char fr[32];
                    std::snprintf(fr, sizeof(fr), "FRAMES_%s_1", g_menu.hudRace);
                    const spr::Strip *fs = g_menu.hud.strip(fr);
                    const spr::Frame *ff = fs && fs->count() ? fs->at(0) : nullptr;
                    const panel::Image *ic = g_menu.hud.image("BOATS_R_00");
                    const panel::Image *lb = g_menu.hud.image("INF_LIFELEVU");
                    int fw = ff && ff->ok() ? ff->canvasW : 0;
                    int fh = ff && ff->ok() ? ff->canvasH : 0;
                    // Miejsca z exe: ramka (10,48), ikona (11,49), pasek (59,49).
                    bool obejmuje = fw > 0
                        && 10 <= 11 && 10 + fw >= 59 + 7
                        && 48 <= 49 && 48 + fh >= 49 + 33;
                    bool kolumny = lb && lb->ok() && lb->w == 15 && lb->h == 33;
                    std::printf("  wieziona lodz: ramka %s %dx%d, ikona %s,"
                                " pasek %s %s -> %s\n",
                                fr, fw, fh,
                                ic && ic->ok() ? "BOATS_R 48x33" : "BRAK",
                                lb && lb->ok() ? "INF_LIFELEVU" : "BRAK",
                                kolumny ? "15x33 = trzy kolumny po 5"
                                        : "INNY ROZMIAR",
                                obejmuje ? "ramka obejmuje ikone i pasek"
                                         : "UKLAD SIE NIE SKLADA");

                    Menu::BarLay BL2;
                    if (g_menu.BarLayout(&BL2) && BL2.infoL >= 0) {
                        int bx = BL2.infoL + 10;
                        int by = BL2.y + g_menu.InfoUpperH() + 48;
                        int bw = 60, bh = 36;
                        size_t bylo3 = g_menu.units.size();
                        auto scena = [&](int hpWiez, std::vector<uint32_t> &poza) {
                            g_menu.units.resize(bylo3);
                            Menu::Unit rs;               // 7 = RepSub (WS)
                            rs.owner = uint32_t(g_menu.me & 7); rs.type = 7;
                            rs.x = 20; rs.y = 20; rs.hp = rs.hpMax = 1000;
                            g_menu.units.push_back(rs);
                            if (hpWiez > 0) {
                                Menu::Unit w;            // wieziona lodz bojowa
                                w.owner = rs.owner; w.type = 1;
                                w.x = 20; w.y = 20;
                                w.hpMax = 1000; w.hp = hpWiez;
                                w.carried = true;
                                g_menu.units.push_back(w);
                                g_menu.units[bylo3].carry = int(bylo3 + 1);
                            }
                            g_menu.sel.assign(1, int(bylo3));
                            g_menu.selBld.clear();
                            g_menu.Compose();
                            poza.clear();
                            for (int yy = by; yy < by + bh; ++yy)
                                for (int xx = bx; xx < bx + bw; ++xx) {
                                    size_t at = size_t(yy) * size_t(SCREEN_W)
                                              + size_t(xx);
                                    if (at < g_menu.canvas.size())
                                        poza.push_back(g_menu.canvas[at]);
                                }
                        };
                        std::vector<uint32_t> pusta, pelna, ranna;
                        scena(0, pusta);        // nic nie wiezie
                        scena(1000, pelna);     // wiezie zdrowa
                        scena(100, ranna);      // wiezie ledwo zywa
                        auto rozne = [](const std::vector<uint32_t> &p,
                                        const std::vector<uint32_t> &q) {
                            int n = 0;
                            for (size_t i = 0; i < p.size() && i < q.size(); ++i)
                                if (p[i] != q[i]) ++n;
                            return n;
                        };
                        int jest = rozne(pusta, pelna);
                        int pasek = rozne(pelna, ranna);
                        std::printf("    na plotnie: z ladunkiem kontra pusta"
                                    " %d px %s, pelna kontra ranna %d px %s\n",
                                    jest, jest > 0 ? "rysuje sie" : "NIE RYSUJE SIE",
                                    pasek, pasek > 0 ? "pasek reaguje"
                                                     : "PASEK NIE REAGUJE");
                        g_menu.units.resize(bylo3);
                        g_menu.sel.clear();
                    }
                }
            }
        }
            // Tryb uzbrojony ma dawac inny kursor niz zwykly.
            g_menu.sel.assign(1, 0);
            g_menu.armed = Menu::ARM_NONE;
            RECT vp = g_menu.Viewport();
            g_menu.mouse = POINT{ (vp.left + vp.right) / 2, (vp.top + vp.bottom) / 2 };
            g_menu.PickCursor();
            std::string zwykly = g_menu.curName;
            g_menu.armed = Menu::ARM_ATTACK;
            g_menu.PickCursor();
            std::string atak = g_menu.curName;
            g_menu.armed = Menu::ARM_PATROL;
            g_menu.PickCursor();
            std::string patrol = g_menu.curName;
            g_menu.armed = Menu::ARM_NONE;
            g_menu.mouse = POINT{ vp.left + 2, (vp.top + vp.bottom) / 2 };
            g_menu.PickCursor();
            std::string krawedz = g_menu.curName;
            std::printf("  zwykly %s, atak %s, patrol %s, krawedz %s\n",
                        zwykly.c_str(), atak.c_str(), patrol.c_str(),
                        krawedz.c_str());
        }

        // ---- SLADY: rozkaz zostawia znacznik ------------------------------
        {   g_menu.fx.clear();
            g_menu.sel.clear();
            for (size_t k = 0; k < g_menu.units.size() && g_menu.sel.size() < 3; ++k)
                if ((g_menu.units[k].owner & 7) == uint32_t(g_menu.me & 7))
                    g_menu.sel.push_back(int(k));
            int px2, py2;
            RECT vp2 = g_menu.Viewport();
            px2 = (vp2.left + vp2.right) / 2;
            py2 = (vp2.top + vp2.bottom) / 2;
            g_menu.OrderMove(px2, py2);
            int poRuchu = int(g_menu.fx.size());
            for (DWORD t = 1000; t < 1400; t += 100) g_menu.StepUnits(t);
            g_menu.Compose();
            const spr::Strip *t7 = g_menu.unitSet.misc("TRAKS07");
            const spr::Strip *t4 = g_menu.unitSet.misc("TRAKS04");
            const spr::Strip *t2 = g_menu.unitSet.misc("TRAKS02");
            std::printf("slady: TRAKS07 %d klatek, TRAKS04 %d, TRAKS02 %d; "
                        "po rozkazie ruchu efektow %d\n",
                        t7 ? int(t7->count()) : 0, t4 ? int(t4->count()) : 0,
                        t2 ? int(t2->count()) : 0, poRuchu);
            int przed = int(g_menu.fx.size());
            for (DWORD t = 1000; t < 4000; t += 100) g_menu.StepUnits(t);
            {   FILE *o = std::fopen("trail.raw", "wb");
                if (o) { std::fwrite(g_menu.canvas.data(), 4,
                                     g_menu.canvas.size(), o); std::fclose(o); }
            }
            std::printf("  znacznik gasnie: efektow %d -> %d\n",
                        przed, int(g_menu.fx.size()));
        }

        g_menu.paused = false;
        std::printf("okna: %d z tlem, %d bez\n", mam, brak);
        {   // **Cele misji sa W MAPIE.** Trzydziesci map misji niesie
            // `OBJECTIVES` i `DESCRIPTION` jako tablice napisow typu 23,
            // a remake ich nie otwieral wcale. Liczymy, ile map je ma,
            // i sprawdzamy ZMIANA PLOTNA, ze naprawde wchodza do okna -
            // sam odczyt nie dowodzi, ze widac je na ekranie.
            int zCel = 0, zOdpr = 0, celRazem = 0;
            for (const maps::Entry &m : g_menu.skMaps) {
                if (!m.goals.empty()) { ++zCel; celRazem += int(m.goals.size()); }
                if (!m.brief.empty()) ++zOdpr;
            }
            std::printf("cele misji: %d z %d map ma cele (%d celow razem), "
                        "%d ma odprawe\n", zCel, int(g_menu.skMaps.size()),
                        celRazem, zOdpr);
            const maps::Entry *cm = g_menu.CurMap();
            if (cm) {
                std::printf("  ta mapa: %s - celow %d, wierszy odprawy %d\n",
                            cm->title.c_str(), int(cm->goals.size()),
                            int(cm->brief.size()));
                for (size_t k = 0; k < cm->goals.size() && k < 4; ++k)
                    std::printf("    cel %d: %s\n", int(k) + 1,
                                cm->goals[k].c_str());
            }
            // Blok odprawy nie ma stalego gniazda - audyt po wszystkich mapach.
            {
                int zBlokiem = 0, akapRazem = 0, zNaglowkiem = 0, wpadkaCel = 0;
                int zOdprawa2 = 0;
                for (const maps::Entry &m : g_menu.skMaps) {
                    if (m.brief.empty()) continue;
                    ++zOdprawa2;
                    int ba, bb;
                    Menu::BriefRange(m.brief, ba, bb);
                    if (ba > bb) continue;
                    ++zBlokiem;
                    akapRazem += bb - ba + 1;
                    if (ba > 0 && m.brief[ba - 1].find('@') != std::string::npos)
                        ++zNaglowkiem;
                    for (int k = ba; k <= bb; ++k)
                        for (const std::string &c : m.goals)
                            if (!c.empty() && c == m.brief[k]) ++wpadkaCel;
                }
                std::printf("  blok odprawy: %d z %d map, %d akapitow razem, "
                            "naglowek z '@' %d, akapit bedacy celem %d\n",
                            zBlokiem, zOdprawa2, akapRazem, zNaglowkiem,
                            wpadkaCel);
                if (cm) {
                    int ba, bb;
                    Menu::BriefRange(cm->brief, ba, bb);
                    std::printf("    ta mapa: gniazda %d..%d z %d\n", ba, bb,
                                int(cm->brief.size()));
                }
            }
            g_menu.winOpen = Menu::WIN_HELP;
            g_menu.helpTab = 0;
            g_menu.Compose();
            std::vector<uint32_t> pelne = g_menu.canvas;
            std::vector<std::string> zapasG, zapasB;
            // **Osobno cele i osobno odprawa** - wspolny pomiar nie pokazuje,
            // ze blok odprawy w ogole wszedl na plotno.
            //
            // **Cele trzeba mierzyc przy odprawie ZDJETEJ w obu stanach.**
            // Usuniecie celow podciaga odprawe o kilka wierszy w gore, wiec
            // roznica policzona wprost liczyla jeszcze raz caly przesuniety
            // akapit - stad zawyzone 22108 zamiast wlasciwej liczby.
            int zmian = 0, zmianB = 0;
            if (cm) {
                maps::Entry &mm = g_menu.skMaps[size_t(g_menu.skSel)];
                zapasG = mm.goals; zapasB = mm.brief;
                mm.brief.clear();                      // same cele
                g_menu.Compose();
                std::vector<uint32_t> bezOdprawy = g_menu.canvas;
                for (size_t k = 0; k < pelne.size() && k < bezOdprawy.size(); ++k)
                    if (pelne[k] != bezOdprawy[k]) ++zmianB;
                mm.goals.clear();                      // ani celow, ani odprawy
                g_menu.Compose();
                for (size_t k = 0; k < bezOdprawy.size()
                                   && k < g_menu.canvas.size(); ++k)
                    if (bezOdprawy[k] != g_menu.canvas[k]) ++zmian;
                mm.goals = zapasG; mm.brief = zapasB;
            }
            std::printf("  zakladka CELE MISJI: pikseli od celow %d, "
                        "od odprawy %d\n", zmian, zmianB);
            g_menu.Compose();
            if (FILE *o = std::fopen("gui_goals.raw", "wb")) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
                std::fclose(o);
            }
            g_menu.winOpen = Menu::WIN_NONE;
        }
        {   // **Odprawa misji jest w mapie.** Notatka mowila, ze glos
            // siedzi wylacznie w DATA\TASKS - to prawda tylko dla potyczki.
            // Liczymy, ile map niesie `TaskSpeach`, i sprawdzamy SKUTKIEM,
            // ze mikser naprawde go podjal.
            int zGlosem = 0;
            for (const maps::Entry &m : g_menu.skMaps) {
                snd::Bank probe;
                if (probe.openMap(m.dkx, m.dkd) && probe.hasMapSpeech())
                    ++zGlosem;
            }
            g_menu.sfx.openMap(g_menu.skMaps[size_t(g_menu.skSel)].dkx,
                               g_menu.skMaps[size_t(g_menu.skSel)].dkd);
            bool ma = g_menu.sfx.hasMapSpeech();
            g_menu.briefBip = true;                 // sygnal juz byl
            g_menu.briefLeft = 0.01f;
            g_menu.StepBrief(0.02f);
            std::printf("odprawa z mapy: %d z %d map ma TaskSpeach; ta mapa "
                        "%s, zagrala %s\n", zGlosem, int(g_menu.skMaps.size()),
                        ma ? "ma" : "NIE MA",
                        g_menu.briefFromMap ? "z mapy" : "zastepczo z TASKS");
        }
        {   // **Tablica gniazd graczy w DESCRIPTOR** - uklad z
            // `SettMapMTy::PrepPlList`: krok 0x51, `+1 == 0xFF` znaczy puste.
            // Para (+3, +7) miesci sie w rozmiarze kazdej mapy, ale CO
            // dokladnie znaczy, nie jest rozstrzygniete - mierzymy wiec, jak
            // blisko jest srodka ciezkosci jednostek ktoregos gracza,
            // i podajemy liczbe zamiast wniosku.
            int zGniazdami = 0, gniazdRazem = 0, wZakresie = 0;
            for (const maps::Entry &m : g_menu.skMaps) {
                int uzytych = 0;
                for (const maps::MapSlot &q : m.slots) {
                    if (!q.used) continue;
                    ++uzytych;
                    if (int(q.x) < m.w && int(q.y) < m.h) ++wZakresie;
                }
                if (uzytych) { ++zGniazdami; gniazdRazem += uzytych; }
            }
            {   // OBIEKTY NA MAPIE: przewijana lista. Strzalka bez
                // ruchu wyglada tak samo jak strzalka wpieta - liczy sie,
                // czy zmienil sie OBRAZ, a nie samo pole `mobjTop`.
                g_menu.winOpen = Menu::WIN_MOBJ;
                g_menu.mobjTop = 0;
                std::vector<std::string> lst;
                g_menu.MobjRows(lst);
                g_menu.Compose();
                std::vector<uint32_t> gora = g_menu.canvas;
                RECT wr5;
                int zmian = 0;
                bool ruszyl = false;
                if (g_menu.WinRect(Menu::WIN_MOBJ, wr5)) {
                    int x = (int(wr5.left) + Menu::MOBJ_AX0
                             + int(wr5.left) + Menu::MOBJ_AX1) / 2;
                    int y = int(wr5.top) + Menu::MOBJ_DN_Y + 5;
                    g_menu.WinClick(x, y);
                    ruszyl = g_menu.mobjTop > 0;
                    g_menu.Compose();
                    for (size_t k = 0; k < gora.size()
                                       && k < g_menu.canvas.size(); ++k)
                        if (gora[k] != g_menu.canvas[k]) ++zmian;
                }
                std::printf("obiekty na mapie: wierszy %d, strzalka w dol %s,"
                            " pikseli roznicy %d\n", int(lst.size()),
                            ruszyl ? "tak" : "NIE", zmian);
                g_menu.mobjTop = 0;
                g_menu.winOpen = Menu::WIN_NONE;
            }
            {   // **Belka tytulu ma czesto wrysowany wlasny napis.**
                // Zmierzona belka to najwiekszy prostokat jednego
                // indeksu, ktory ten napis z definicji **omija** - wiec
                // bywa od niego wezsza i zamalowanie jej zostawia
                // koncowki po bokach. Liczymy, w ilu oknach tusz w pasie
                // belki w ogole jest i w ilu wystaje poza nia.
                int zBelka = 0, zNapisem = 0, wystaje = 0, pokryte = 0;
                for (int wi = 0; wi < Menu::WIN_COUNT; ++wi) {
                    RECT wr5;
                    if (!g_menu.WinRect(wi, wr5)) continue;
                    const Menu::WinFields *f = g_menu.Fields(wi);
                    if (!f) continue;
                    ++zBelka;
                    if (f->ink.right <= f->ink.left) continue;
                    ++zNapisem;
                    if (f->ink.left < f->title.left
                        || f->ink.right > f->title.right) ++wystaje;
                    RECT band = g_menu.WinTitleBand(wi, wr5);
                    RECT ik{ wr5.left + f->ink.left, wr5.top + f->ink.top,
                             wr5.left + f->ink.right, wr5.top + f->ink.bottom };
                    if (band.left <= ik.left && band.right >= ik.right
                        && band.top <= ik.top && band.bottom >= ik.bottom)
                        ++pokryte;
                }
                std::printf("belki tytulu: zmierzonych %d, z wrysowanym napisem %d, napis szerszy niz belka %d, pas zakrywa %d z %d\n",
                            zBelka, zNapisem, wystaje, pokryte, zNapisem);
                {   // **ZAMKNIJ tez jest w exe.** Szukanie go w grafice
                    // (ciag nasyconych pikseli u dolu) to heurystyka;
                    // `SpecPanelTy::InitPanel` podaje rog wprost. Tam,
                    // gdzie obie drogi istnieja, musza sie zgadzac -
                    // a gdzie heurystyka nic nie znajdowala, exe daje
                    // miejsce zamiast dorysowanego przycisku w rogu.
                    int nC = 0, znaleziony = 0, zgodnych = 0, najdalej = 0;
                    const Menu::CloseDef *cd = Menu::CloseDefs(nC);
                    for (int i2 = 0; i2 < nC; ++i2) {
                        RECT wr6, zExe, zGraf;
                        if (!g_menu.WinRect(cd[i2].win, wr6)) continue;
                        if (!g_menu.CloseFromExe(cd[i2].win, wr6, zExe)) continue;
                        if (!g_menu.WinCloseRect(cd[i2].win, zGraf)) continue;
                        ++znaleziony;
                        int dx = int(std::abs(long(zGraf.left - zExe.left)));
                        int dy = int(std::abs(long(zGraf.top - zExe.top)));
                        int d = dx > dy ? dx : dy;
                        if (d > najdalej) najdalej = d;
                        if (d <= 4) ++zgodnych;
                    }
                    std::printf("zamykanie: pozycji z exe %d, znalezionych w grafice %d, zgodnych %d, roznica do %d px\n",
                                nC, znaleziony, zgodnych, najdalej);
                }
                {   // **Pole tytulu z exe kontra pomiar w grafice.**
                    // Szesc z siedmiu tych okien **nie ma zmierzonej
                    // belki w ogole** - pomiar szuka najwiekszej plamy
                    // jednego indeksu i u nich sie nie udaje, wiec tytul
                    // szedl dotad w **zgadniety** prostokat zastepczy.
                    // Teraz idzie w pole z `SpecPanelTy::InitPanel`.
                    // Tam, gdzie obie drogi istnieja, musza sie zgadzac.
                    int nT = 0, zNapisemGry = 0, porownanych = 0;
                    int zgodnych = 0, najdalej = 0;
                    const Menu::TitleDef *td = Menu::TitleDefs(nT);
                    for (int i2 = 0; i2 < nT; ++i2) {
                        if (!Text(td[i2].str).empty()) ++zNapisemGry;
                        const Menu::WinFields *f = g_menu.Fields(td[i2].win);
                        if (!f) continue;
                        ++porownanych;
                        int d = int(std::abs(long(f->title.top) - td[i2].y));
                        int d2 = int(std::abs(long(f->title.bottom - f->title.top)
                                              - td[i2].h));
                        if (d2 > d) d = d2;
                        if (d > najdalej) najdalej = d;
                        if (d <= 2) ++zgodnych;
                    }
                    std::printf("  pole z exe: %d okien, z napisem gry %d; bez pomiaru w grafice %d (szly w prostokat zastepczy), porownanych %d, zgodnych %d, roznica do %d px\n",
                                nT, zNapisemGry, nT - porownanych,
                                porownanych, zgodnych, najdalej);
                }
            }
            {   // **Dyplomacja ma trzy obrazki, nie jeden.** Do duzego
                // tla dochodzi deska listy `BKG_DIPLOMACYB` (356x133 na
                // 29,19) i przyciski stanu `GAMEB_ALLY__<rasa><0..3>`
                // (22x14). Liczymy, ile z nich archiwum naprawde ma,
                // i ile pikseli dokladaja na plotnie.
                int desek = 0, ikon = 0;
                static const char *kR[3] = { "WS", "BO", "SI" };
                for (int r2 = 0; r2 < 3; ++r2) {
                    char nm[48];
                    std::snprintf(nm, sizeof(nm), "BKG_DIPLOMACYB_%s", kR[r2]);
                    const panel::Image *im2 = g_menu.hud.image(nm);
                    if (im2 && im2->ok()) ++desek;
                    for (int st = 0; st < 4; ++st) {
                        std::snprintf(nm, sizeof(nm), "GAMEB_ALLY__%s%d",
                                      kR[r2], st);
                        const spr::Strip *ss = g_menu.GetTvStrip(nm);
                        if (ss && ss->count()) ++ikon;
                    }
                }
                g_menu.winOpen = Menu::WIN_DIPLO;
                g_menu.Compose();
                std::vector<uint32_t> zOknem = g_menu.canvas;
                g_menu.winOpen = Menu::WIN_NONE;
                g_menu.Compose();
                int rozne = 0;
                for (size_t q = 0; q < zOknem.size(); ++q)
                    if (zOknem[q] != g_menu.canvas[q]) ++rozne;
                std::printf("dyplomacja: desek listy %d z 3, ikon stanu %d z 12, pikseli okna %d\n", desek, ikon, rozne);
                g_menu.winOpen = Menu::WIN_NONE;
            }
            {   // **USTAWIENIA MINY AKUSTYCZNEJ**, nie ustawienia gry:
                // `BKG_SETANYW` nalezy do `SAMPanelTy` i szesc wierszy to
                // klasy celow (napisy 12500..12505), a nie przelaczniki,
                // ktore sam tu kiedys wstawilem.
                g_menu.winOpen = Menu::WIN_SETANY;
                RECT wr4;
                int dziala = 0, zNapisem = 0;
                unsigned przed = g_menu.samMask;
                if (g_menu.WinRect(Menu::WIN_SETANY, wr4)) {
                    for (int k = 0; k < Menu::SET_ROWS; ++k) {
                        if (!g_menu.SetAnyName(k).empty()
                            && g_menu.SetAnyName(k) != "?") ++zNapisem;
                        bool byl = g_menu.SetAnyOn(k);
                        int x = int(wr4.left) + Menu::SET_BOX_X + 4;
                        int y = int(wr4.top) + Menu::SET_Y0
                              + k * Menu::SET_STEP + 5;
                        g_menu.WinClick(x, y);
                        if (g_menu.SetAnyOn(k) != byl) ++dziala;
                        g_menu.WinClick(x, y);      // z powrotem
                    }
                }
                // **Klasy musza byc rozne** - szesc wierszy, z ktorych
                // kazdy lapie te same typy, wygladaloby tak samo.
                int ile[Menu::SET_ROWS] = {};
                for (int t = 1; t <= 40; ++t) {
                    int c = Menu::SamClass(t);
                    if (c >= 0 && c < Menu::SET_ROWS) ++ile[c];
                }
                int pustych = 0;
                for (int k = 0; k < Menu::SET_ROWS; ++k)
                    if (!ile[k]) ++pustych;
                std::printf("mina akustyczna: wierszy %d, z napisem gry %d, przelacza %d, klasy"
                            , Menu::SET_ROWS, zNapisem, dziala);
                for (int k = 0; k < Menu::SET_ROWS; ++k)
                    std::printf(" %d:%d", k, ile[k]);
                std::printf(", klasa bez lodzi %d (STRUKTURY RUCHOME to budynki), maska %s\n", pustych,
                            g_menu.samMask == przed ? "oddana" : "ZMIENIONA");
                g_menu.samMask = przed;
                g_menu.winOpen = Menu::WIN_NONE;
            }
            {   // Infocentrum: raport na siatce i przelaczanie gracza
                // kolumna gniazd z lewej.
                g_menu.winOpen = Menu::WIN_INFOC;
                g_menu.infoPlayer = -1;
                g_menu.Compose();
                std::vector<uint32_t> nas = g_menu.canvas;
                int inny = -1;
                for (int k = 0; k < Menu::MAX_PLAYERS; ++k)
                    if (g_menu.players[k].active && k != (g_menu.me & 7))
                        { inny = k; break; }
                bool przelaczyl = false;
                int zmian = 0;
                if (inny >= 0) {
                    RECT wr3;
                    if (g_menu.WinRect(Menu::WIN_INFOC, wr3)) {
                        int x = (int(wr3.left) + Menu::INF_PL_X0
                                 + int(wr3.left) + Menu::INF_PL_X1) / 2;
                        int y = int(wr3.top) + Menu::INF_PL_Y0
                              + inny * Menu::INF_PL_STEP + 5;
                        g_menu.WinClick(x, y);
                        przelaczyl = g_menu.infoPlayer == inny;
                    }
                    g_menu.Compose();
                    for (size_t k = 0; k < nas.size() && k < g_menu.canvas.size(); ++k)
                        if (nas[k] != g_menu.canvas[k]) ++zmian;
                }
                if (FILE *o = std::fopen("infoc.raw", "wb")) {
                    std::fwrite(g_menu.canvas.data(), 4,
                                g_menu.canvas.size(), o);
                    std::fclose(o);
                }
                std::printf("infocentrum: gniazd graczy %d, przelaczyl na %d %s,"
                            " pikseli roznicy %d\n", Menu::MAX_PLAYERS, inny,
                            przelaczyl ? "tak" : "NIE", zmian);
                g_menu.infoPlayer = -1;
                g_menu.winOpen = Menu::WIN_NONE;
            }
            {   // Rynek: cala droga gracza po WIDGETACH z grafiki -
                // zakladka surowca, strzalki ilosci, zatwierdzenie.
                g_menu.winOpen = Menu::WIN_TRADE;
                g_menu.Me().civ = 0;
                g_menu.SyncRace();
                RECT wr2;
                bool maOkno = g_menu.WinRect(Menu::WIN_TRADE, wr2);
                auto klikT = [&](int k) {
                    if (!maOkno) return;
                    int L = int(wr2.left), T = int(wr2.top);
                    int x = 0, y = 0;
                    if (k == 0 || k == 1) {
                        x = L + Menu::TRD_TAB_X[k] + Menu::TRD_TAB_W / 2;
                        y = T + (Menu::TRD_TAB_Y0 + Menu::TRD_TAB_Y1) / 2;
                    } else if (k == 2 || k == 3) {
                        x = L + Menu::TRD_ARR_X + Menu::TRD_ARR_W / 2;
                        y = T + (k == 2 ? Menu::TRD_UP_Y : Menu::TRD_DN_Y)
                          + Menu::TRD_ARR_H / 2;
                    } else {
                        RECT action{};
                        if (!g_menu.TradeButtonRect(k, action)) return;
                        x = (action.left + action.right) / 2;
                        y = (action.top + action.bottom) / 2;
                    }
                    g_menu.WinClick(x, y);
                };
                g_menu.tradeRes = 1;
                klikT(0);
                bool zakl = g_menu.tradeRes == 0;
                int amt0 = g_menu.tradeAmt;
                klikT(2);
                bool wGore = g_menu.tradeAmt > amt0;
                klikT(3);
                bool wDol = g_menu.tradeAmt == amt0;
                g_menu.Me().bank.corium = 5000;
                g_menu.Me().bank.gold = 0;
                g_menu.tradeSell = true;
                g_menu.tradeAmt = 100;
                int kor0 = g_menu.Me().bank.corium;
                klikT(5);
                bool sprzedal = g_menu.Me().bank.corium < kor0
                             && g_menu.Me().bank.gold > 0;
                std::printf("rynek: zakladka %s, strzalki %s/%s, "
                            "sprzedaz %s (korium %d -> %d, zloto %d)\n",
                            zakl ? "tak" : "NIE", wGore ? "+" : "NIE",
                            wDol ? "-" : "NIE", sprzedal ? "tak" : "NIE",
                            kor0, g_menu.Me().bank.corium,
                            g_menu.Me().bank.gold);
                g_menu.winOpen = Menu::WIN_NONE;
            }
            {   // Okno ULEPSZEN: siatka 12 gniazd ma sie wypelnic ikonami
                // `UPG_<NN>` zbadanych technologii. Mierzymy roznica plotna
                // przed zbadaniem i po - puste gniazda wygladaja tak samo
                // jak brak kodu.
                int nT = 0;
                tech::list(nT);
                g_menu.Me().techDone.assign(size_t(nT), 0);
                g_menu.winOpen = Menu::WIN_UPGRADE;
                g_menu.winPage = 0;
                g_menu.Compose();
                std::vector<uint32_t> bezT = g_menu.canvas;
                int ile = 0;
                for (int k = 0; k < nT && ile < 12; ++k)
                    if (tech::list(nT)[k].side == g_menu.PlayerSide()) {
                        g_menu.Me().techDone[size_t(k)] = 1;
                        ++ile;
                    }
                g_menu.Compose();
                int zmian = 0;
                for (size_t k = 0; k < bezT.size() && k < g_menu.canvas.size(); ++k)
                    if (bezT[k] != g_menu.canvas[k]) ++zmian;
                if (FILE *o = std::fopen("upgrades.raw", "wb")) {
                    std::fwrite(g_menu.canvas.data(), 4,
                                g_menu.canvas.size(), o);
                    std::fclose(o);
                }
                g_menu.Me().techDone.assign(size_t(nT), 0);
                g_menu.winOpen = Menu::WIN_NONE;
                std::printf("okno ulepszen: gniazd 12, zbadanych %d, "
                            "pikseli od ikon %d\n", ile, zmian);
            }
            {   // Podglady TV: nazwa z exe to jedno, rekord w archiwum
                // drugie. Liczy sie ten drugi - nazwa bez grafiki wyglada
                // tak samo jak brak tablicy.
                int uMa = 0, uBrak = 0, bMa = 0, bBrak = 0;
                for (int t = 1; t <= 40; ++t) {
                    const char *n = tv::forUnit(t, g_menu.PlayerSide());
                    if (n && g_menu.GetTvStrip(n)) ++uMa; else ++uBrak;
                }
                for (int side = 0; side < 3; ++side) {
                    int bn = 0;
                    const int *bl = cost::bldList(side, bn);
                    for (int i = 0; i < bn; ++i) {
                        const char *n = tv::forBld(bl[i], side);
                        if (n && g_menu.GetTvStrip(n)) ++bMa; else ++bBrak;
                    }
                }
                std::printf("podglady TV: lodzi %d z %d, budynkow %d z %d "
                            "(razem trzy rasy)\n",
                            uMa, uMa + uBrak, bMa, bMa + bBrak);
            }
            {   // **Komunikat ma byc widoczny BEZ palety.** Wczesniej
                // rysowal sie w jej wnetrzu, wiec przy zamknietej palecie
                // nie bylo go wcale - a wlasnie wtedy jest potrzebny.
                g_menu.winOpen = Menu::WIN_NONE;
                g_menu.palOpen = false;
                g_menu.buildMsg.clear();
                g_menu.buildMsgLeft = 0;
                g_menu.Compose();
                std::vector<uint32_t> bez = g_menu.canvas;
                g_menu.Say("miejsce zajete", 5.0f);
                g_menu.Compose();
                int zmian = 0;
                for (size_t k = 0; k < bez.size() && k < g_menu.canvas.size(); ++k)
                    if (bez[k] != g_menu.canvas[k]) ++zmian;
                g_menu.buildMsgLeft = 0;
                {   // **Linia komunikatow w grze to `PopUpTy`** - te dwa
                    // zdarzenia rozgrywki, ktore przez nia ida, remake tez
                    // ma, wiec biora teraz **napisy gry**, a nie moje.
                    // Sprawdzamy jedno i drugie: ze napis sie wczytal
                    // i ze zdarzenie naprawde go wypisuje.
                    std::string lim = Text(STR_MSG_UNITCAP);
                    std::string soj = Text(STR_MSG_ALLY);
                    g_menu.buildMsg.clear();
                    int kto = -1;
                    for (int i = 0; i < Menu::MAX_PLAYERS; ++i)
                        if (i != (g_menu.me & 7) && g_menu.players[i].active) {
                            kto = i; break;
                        }
                    bool odezwal = false;
                    RECT dw;
                    g_menu.winOpen = Menu::WIN_DIPLO;
                    if (kto >= 0 && g_menu.WinRect(Menu::WIN_DIPLO, dw)) {
                        RECT rr, sw, bt;
                        if (g_menu.DiploRow(dw, kto, rr, sw, bt)) {
                            g_menu.WinClick(int(sw.left + bt.right) / 2,
                                            int(rr.top + rr.bottom) / 2);
                            odezwal = !g_menu.buildMsg.empty();
                        }
                    }
                    g_menu.winOpen = Menu::WIN_NONE;
                    // **Audyt sprzata po sobie.** Przelaczenie sojuszu
                    // zostawalo wlaczone i psulo pomiar dyplomacji kilka
                    // wierszy nizej (`gracz 3 wrogiem NIE`) - ta sama
                    // pulapka, co przy audycie kampanii, ktory na stale
                    // odhaczal misje.
                    std::string powiedzial = g_menu.buildMsg;
                    if (kto >= 0) g_menu.ally[kto] = 0;
                    g_menu.buildMsg.clear();
                    std::printf("komunikaty gry: limit \"%s\", sojusz \"%s\"; "
                                "po zmianie sojuszu %s%s\n",
                                lim.c_str(), soj.c_str(),
                                odezwal ? powiedzial.c_str() : "CISZA",
                                (!lim.empty() && !soj.empty() && odezwal)
                                    ? "" : "  <- BEZ SKUTKU");
                }
                std::printf("komunikat bez palety: pikseli %d (paleta %s)\n",
                            zmian, g_menu.palOpen ? "otwarta" : "zamknieta");
            }
            {   // Zegar partii: ma byc WIDOCZNY, wiec liczy sie roznica
                // plotna z nim i bez niego.
                g_menu.winOpen = Menu::WIN_NONE;
                g_menu.playSecs = 754.0f;          // 12:34
                g_menu.clockOn = true;
                g_menu.Compose();
                std::vector<uint32_t> zZeg = g_menu.canvas;
                g_menu.clockOn = false;
                g_menu.Compose();
                int zmian = 0;
                for (size_t k = 0; k < zZeg.size() && k < g_menu.canvas.size(); ++k)
                    if (zZeg[k] != g_menu.canvas[k]) ++zmian;
                g_menu.clockOn = true;
                char nmz[24];
                std::snprintf(nmz, sizeof(nmz), "BKG_TIMER_%s", g_menu.hudRace);
                const panel::Image *zi = g_menu.hud.image(nmz);
                std::printf("zegar: %s %dx%d, font wys. %d, pikseli %d\n",
                            nmz, zi ? zi->w : 0, zi ? zi->h : 0,
                            g_menu.fontTimer.height(), zmian);
            }
            std::printf("gniazda z DESCRIPTOR: %d z %d map, %d gniazd, "
                        "%d w rozmiarze mapy\n", zGniazdami,
                        int(g_menu.skMaps.size()), gniazdRazem, wZakresie);
            {   // Rasa gracza: z mapy czy zgadnieta z typow lodzi?
                int aktywnych = 0;
                for (int p = 0; p < Menu::MAX_PLAYERS; ++p)
                    if (g_menu.players[p].active) ++aktywnych;
                std::printf("  rasa gracza: %d z mapy, %d zgadnietych z lodzi "
                            "(aktywnych graczy %d)\n", g_menu.civZMapy,
                            g_menu.civZLodzi, aktywnych);
                // Ktorym gniazdem gramy - i czy mapa faktycznie tak mowi.
                int ludzkich = 0, ludzkichWsz = 0, mapZeZgoda = 0, mapZLudzkim = 0;
                for (const maps::Entry &m : g_menu.skMaps) {
                    int lu = 0, pierwszy = -1;
                    for (const maps::MapSlot &q : m.slots)
                        if (q.used && q.human) {
                            ++lu;
                            if (pierwszy < 0) pierwszy = int(q.id);
                        }
                    ludzkichWsz += lu;
                    if (lu) ++mapZLudzkim;
                    if (lu == 1) ++mapZeZgoda;
                }
                const maps::Entry *cmh = g_menu.CurMap();
                if (cmh)
                    for (const maps::MapSlot &q : cmh->slots)
                        if (q.used && q.human) ++ludzkich;
                std::printf("  gramy gniazdem %d (rasa %d); mapa ma gniazd dla "
                            "czlowieka %d; po wszystkich mapach: z ludzkim %d "
                            "z %d, z dokladnie jednym %d\n",
                            g_menu.me, g_menu.players[g_menu.me & 7].civ + 1,
                            ludzkich, mapZLudzkim,
                            int(g_menu.skMaps.size()), mapZeZgoda);
                (void)ludzkichWsz;
                // Kamera startowa: ile kratek od srodka ekranu do najblizszego
                // WLASNEGO obiektu. Sam fakt, ze funkcja cos ustawila, niczego
                // nie dowodzi - liczy sie, gdzie wyladowala.
                g_menu.CentreCamera();
                g_menu.CameraOnStart();
                {
                    float cx, cy;
                    g_menu.ScreenToCell(SCREEN_W / 2, SCREEN_H / 2, cx, cy);
                    double best = -1;
                    for (const maps::Object &o : g_menu.terr.objects) {
                        if (o.type != maps::OBJ_UNIT
                            && o.type != maps::OBJ_BUILDING) continue;
                        if (int(o.owner) != (g_menu.me & 7)) continue;
                        double dx = double(o.x) / 2.0 - double(cx) / 2.0;
                        double dy = double(o.y) / 2.0 - double(cy) / 2.0;
                        double d = std::sqrt(dx * dx + dy * dy);
                        if (best < 0 || d < best) best = d;
                    }
                    std::printf("  kamera startowa: %s, do najblizszego "
                                "wlasnego obiektu %.1f kratki\n",
                                g_menu.camOnSlot ? "punkt z mapy"
                                                 : "zapas (wlasny budynek)",
                                best);
                }
            }
            {   // Koniec partii tekstem z mapy. Numer gniazda nie jest staly,
                // wiec reguła czyta tresc - a audyt sprawdza, czy nie wybrala
                // celu misji ani linijki przeciwnego wyniku.
                int maW = 0, maL = 0, z18 = 0, z19 = 0, wpadka = 0;
                for (const maps::Entry &m : g_menu.skMaps) {
                    if (m.brief.empty()) continue;
                    for (int t = 0; t < 2; ++t) {
                        int want = t == 0 ? 1 : 2;
                        int pref = t == 0 ? 18 : 19;
                        int wzial = -1;
                        if (pref < int(m.brief.size())
                            && Menu::VerdictKind(m.brief[pref]) == want)
                            wzial = pref;
                        else
                            for (int k = 0; k < int(m.brief.size()); ++k)
                                if (Menu::VerdictKind(m.brief[k]) == want) {
                                    wzial = k; break;
                                }
                        if (wzial < 0) continue;
                        if (t == 0) { ++maW; if (wzial == 18) ++z18; }
                        else        { ++maL; if (wzial == 19) ++z19; }
                        // celu misji pokazac nie wolno
                        for (const std::string &c : m.goals)
                            if (!c.empty() && c == m.brief[wzial]) ++wpadka;
                    }
                }
                int zOdprawa = 0;
                for (const maps::Entry &m : g_menu.skMaps)
                    if (!m.brief.empty()) ++zOdprawa;
                std::printf("koniec partii z mapy: zwyciestwo %d z %d map "
                            "(gniazdo 18 na %d), porazka %d z %d "
                            "(gniazdo 19 na %d), wybrany cel misji %d\n",
                            maW, zOdprawa, z18, maL, zOdprawa, z19, wpadka);
                const maps::Entry *cmv = g_menu.CurMap();
                if (cmv && !cmv->brief.empty())
                    std::printf("  ta mapa: wygrana %.60s | przegrana %.60s\n",
                                g_menu.VerdictText(true),
                                g_menu.VerdictText(false));
            }
            const maps::Entry *cm2 = g_menu.CurMap();
            if (cm2) {
                for (const maps::MapSlot &q : cm2->slots) {
                    if (!q.used) continue;
                    // Najblizszy srodek ciezkosci jednostek gracza.
                    double best = 1e9;
                    int kto = -1;
                    for (int p = 0; p < 8; ++p) {
                        double sx = 0, sy = 0;
                        int n = 0;
                        for (const maps::Object &o : g_menu.terr.objects) {
                            if (o.type != maps::OBJ_UNIT) continue;
                            if (int(o.owner) != p) continue;
                            sx += o.x; sy += o.y; ++n;
                        }
                        if (!n) continue;
                        double dx = sx / n - double(q.x), dy = sy / n - double(q.y);
                        double d = std::sqrt(dx * dx + dy * dy);
                        if (d < best) { best = d; kto = p; }
                    }
                    std::printf("  gniazdo id %d: rasa %d barwa %d (%u,%u) "
                                "poziom %u -> najblizszy gracz %d o %.1f kratki\n",
                                q.id, q.race, q.colour, q.x, q.y, q.z,
                                kto, kto < 0 ? -1.0 : best);
                }
            }
        }
        {   // Zrzut okna ustawien - napisy maja siedziec w czarnych polach.
            g_menu.winOpen = Menu::WIN_OPTIONS;
            g_menu.optTab = 1;
            g_menu.Compose();
            if (FILE *o = std::fopen("options.raw", "wb")) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
                std::fclose(o);
                std::printf("  options.raw %dx%d\n", g_clientW, g_clientH);
            }
            g_menu.winOpen = Menu::WIN_NONE;
        }
        {   // Zrzut dyplomacji i pomocy - okna, w ktorych napisy mijaly pola.
            for (int wi : { int(Menu::WIN_DIPLO), int(Menu::WIN_HELP) }) {
                g_menu.winOpen = wi;
                g_menu.Compose();
                const char *nm = wi == Menu::WIN_DIPLO ? "diplo.raw" : "help.raw";
                if (FILE *o = std::fopen(nm, "wb")) {
                    std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
                    std::fclose(o);
                    std::printf("  %s %dx%d\n", nm, g_clientW, g_clientH);
                }
            }
            g_menu.winOpen = Menu::WIN_NONE;
        }

        {   // **Silikony placa za budowe STOPNIOWO** - przewodnik mowi
            // o kapsule wprost. Liczy sie, ile zostalo w skarbcu zaraz
            // po postawieniu i ile po skonczeniu: u ludzi cala kwota
            // schodzi od razu, u Silikonow rozklada sie na czas budowy.
            //
            // I osobno, tym samym przebiegiem: w koncowej animacji budynek
            // jest **nietykalny**. Mierzone **para** - ta sama zapytanie
            // o cel w trakcie budowy ma trafiac, a w czasie zwijania
            // rusztowania nie. Sam brak trafienia niczego by nie dowodzil:
            // wyszedlby tak samo, gdyby budowy nie dalo sie celowac nigdy.
            struct Wynik {
                int  cenaC, cenaM;          // cennik
                int  stawC, stawM;          // skarbiec po postawieniu
                int  konC,  konM;           // skarbiec po skonczeniu
                bool celBud, celAnim;       // czy da sie wziac na cel
            };
            auto proba = [&](int civ) {
                int byl = g_menu.players[g_menu.me & 7].civ;
                g_menu.players[g_menu.me & 7].civ = civ;
                g_menu.Me().bank.corium = g_menu.Me().bank.metal = 100000;
                // **Magazyn ma u kazdej rasy swoj numer**: 59 u ludzi,
                // 96 u Silikonow. Bez tego test Silikonow bral budynek,
                // ktorego oni nie maja - cena wychodzila zerem i wiersz
                // nie mierzyl niczego.
                int tobj = civ == 2 ? 96 : 59;
                Menu::Bld nb;
                nb.owner = uint32_t(g_menu.me);
                nb.tobj = uint32_t(tobj);
                nb.x = 10; nb.y = 10;
                nb.hpMax = 1000;
                nb.hp = nb.hpBuilt = 100;
                nb.buildTotal = nb.buildLeft = 8.0f;
                cost::Price pr = cost::bldPrice(tobj, civ);
                if (civ == 2) { nb.payCor = pr.corium; nb.payMet = pr.metal; }
                else { g_menu.Me().bank.corium -= pr.corium;
                       g_menu.Me().bank.metal  -= pr.metal; }
                g_menu.blds.push_back(nb);
                size_t k = g_menu.blds.size() - 1;
                Wynik r{};
                r.cenaC = pr.corium; r.cenaM = pr.metal;
                r.stawC = g_menu.Me().bank.corium;
                r.stawM = g_menu.Me().bank.metal;
                uint32_t wrog = uint32_t((g_menu.me & 7) == 1 ? 2 : 1);
                for (int i2 = 0; i2 < 4000; ++i2) {
                    g_menu.StepBuilding(0.05f);
                    bool isB = false;
                    int cel = g_menu.FindTarget(wrog, 10.0f, 10.0f, 4, isB);
                    bool trafia = isB && cel == int(k);
                    if (g_menu.blds[k].closeT > 0) r.celAnim |= trafia;
                    else if (g_menu.blds[k].buildLeft > 0) r.celBud |= trafia;
                    if (g_menu.blds[k].buildLeft <= 0) break;
                }
                r.konC = g_menu.Me().bank.corium;
                r.konM = g_menu.Me().bank.metal;
                g_menu.blds.pop_back();
                g_menu.players[g_menu.me & 7].civ = byl;
                return r;
            };
            Wynik lud = proba(0), sil = proba(2);
            std::printf("platnosc za budowe: ludzie z gory %d+%d "
                        "(skarbiec po postawieniu %d+%d, po budowie %d+%d)\n",
                        lud.cenaC, lud.cenaM, lud.stawC, lud.stawM,
                        lud.konC, lud.konM);
            std::printf("  Silikony w trakcie: przy stawianiu zeszlo %d+%d, "
                        "do konca %d+%d z %d+%d\n",
                        100000 - sil.stawC, 100000 - sil.stawM,
                        sil.stawC - sil.konC + (100000 - sil.stawC),
                        sil.stawM - sil.konM + (100000 - sil.stawM),
                        sil.cenaC, sil.cenaM);
            std::printf("  koncowa animacja: w budowie celowalny %s, "
                        "przy zwijaniu %s\n",
                        (lud.celBud && sil.celBud) ? "tak" : "NIE",
                        (lud.celAnim || sil.celAnim) ? "TAK (zle)" : "nie");
        }

        {   // **Przerwanie budowy traci to, co juz poszlo** - przewodnik mowi
            // to przy kapsule. Rozbiorka **gotowego** budynku dalej oddaje
            // polowe ceny, wiec te dwa przypadki mierzy sie **para**: sam
            // zerowy zwrot niczego by nie dowodzil, bo wyszedlby tak samo,
            // gdyby rozbiorka nie oddawala nigdy nic.
            //
            // Przed poprawka byl z tego **zarobek**: Silikon placi w trakcie,
            // wiec tuz po postawieniu ma zaplacony ulamek ceny, a rozbiorka
            // oddawala mu polowe PELNEJ - stawianie i kasowanie w kolko
            // przynosilo surowce.
            struct Roz { int cena, przed, po; bool klik; size_t zostalo; };
            auto rozbierz = [&](int civ, bool wBudowie) {
                int byl = g_menu.players[g_menu.me & 7].civ;
                g_menu.players[g_menu.me & 7].civ = civ;
                g_menu.blds.clear();
                g_menu.selBld.clear();
                g_menu.Me().bank.corium = g_menu.Me().bank.metal = 100000;
                int tobj = civ == 2 ? 96 : 59;
                cost::Price pr = cost::bldPrice(tobj, civ);
                Menu::Bld nb;
                nb.owner = uint32_t(g_menu.me);
                nb.tobj = uint32_t(tobj);
                nb.x = 10; nb.y = 10;
                nb.hpMax = 1000;
                nb.hp = nb.hpBuilt = wBudowie ? 300 : 1000;
                nb.buildTotal = 8.0f;
                nb.buildLeft = wBudowie ? 4.0f : 0.0f;
                g_menu.blds.push_back(nb);
                g_menu.selBld.push_back(0);
                Roz r{};
                r.cena = pr.metal;
                r.przed = g_menu.Me().bank.metal;
                int nc = 0;
                const Menu::Cmd *cs = g_menu.CmdsFor(true, nc);
                int k = -1;
                for (int i = 0; i < nc && cs; ++i)
                    if (std::strstr(cs[i].rec, "DISMANTL") ||
                        std::strstr(cs[i].rec, "DISSASSEMBLE")) k = i;
                r.klik = k >= 0;
                if (r.klik) g_menu.CmdAction(true, k);
                r.po = g_menu.Me().bank.metal;
                r.zostalo = g_menu.blds.size();
                g_menu.players[g_menu.me & 7].civ = byl;
                return r;
            };
            Roz got = rozbierz(0, false), bud = rozbierz(0, true);
            std::printf("rozbiorka: gotowy oddaje %d z %d, "
                        "przerwana budowa %d z %d; przycisk %s, zniklo %s\n",
                        got.po - got.przed, got.cena,
                        bud.po - bud.przed, bud.cena,
                        (got.klik && bud.klik) ? "jest" : "BRAK",
                        (got.zostalo == 0 && bud.zostalo == 0) ? "tak" : "NIE");
        }
        {   // **Budynek w budowie da sie zniszczyc**, a paleta wyszarza to,
            // czego nie da sie postawic.
            size_t ile0 = g_menu.blds.size();
            Menu::Bld b;
            b.owner = uint32_t(g_menu.me == 1 ? 2 : 1);   // obcy, zeby dalo sie strzelac
            b.tobj = 59;
            b.x = 6; b.y = 6;
            b.hpMax = 1000;
            b.hp = b.hpBuilt = 100;
            b.buildTotal = b.buildLeft = 20.0f;
            g_menu.blds.push_back(b);
            size_t bi = g_menu.blds.size() - 1;
            int hp0 = g_menu.blds[bi].hp;
            g_menu.blds[bi].hp -= 60;                     // trafienie
            g_menu.StepBuilding(0.5f);                    // i tyk budowy
            int hp1 = g_menu.blds[bi].hp;
            bool foe = false;
            int cel = g_menu.FindTarget(uint32_t(g_menu.me), 6.0f, 6.0f, 20, foe);
            std::printf("budowa: hp %d -> %d po trafieniu 60 (%s), celowalna %s\n",
                        hp0, hp1, hp1 < hp0 ? "obrazenia zostaja"
                                            : "SKASOWANE PRZEZ BUDOWE",
                        (cel >= 0 && foe) ? "tak" : "NIE");
            {   // **Budowa konczy sie na 100% wytrzymalosci**, wiec
                // trafienie musi ja WYDLUZYC. Sam fakt, ze obrazenia
                // zostaja, tego nie dowodzi: przy budowie liczonej
                // zegarem hp tez spadalo, a koniec przychodzil o tej
                // samej sekundzie. Stawiamy wiec dwie takie same budowy
                // i mierzymy, ktora konczy sie pozniej.
                auto postaw = [&]() {
                    Menu::Bld nb;
                    nb.owner = uint32_t(g_menu.me);
                    nb.tobj = 59;
                    nb.x = 8; nb.y = 8;
                    nb.hpMax = 1000;
                    nb.hp = nb.hpBuilt = 100;
                    nb.buildTotal = nb.buildLeft = 10.0f;
                    g_menu.blds.push_back(nb);
                    return g_menu.blds.size() - 1;
                };
                auto zmierz = [&](bool trafic) {
                    size_t k = postaw();
                    float t = 0;
                    bool bito = false;
                    for (int i2 = 0; i2 < 4000; ++i2) {
                        if (trafic && !bito && t >= 2.0f) {
                            g_menu.blds[k].hp -= 300;
                            bito = true;
                        }
                        g_menu.StepBuilding(0.05f);
                        t += 0.05f;
                        if (g_menu.blds[k].buildLeft <= 0) break;
                    }
                    g_menu.blds.pop_back();
                    return t;
                };
                float bez = zmierz(false);
                float po = zmierz(true);
                std::printf("  ostrzelana budowa: bez trafienia %.1f s, po trafieniu 300 hp %.1f s -> %s\n",
                            bez, po, po > bez + 0.2f ? "dluzej"
                                                     : "TAK SAMO");
            }
            while (g_menu.blds.size() > ile0) g_menu.blds.pop_back();
        }
        {   // Dzwieki zdarzen: kazdy numer z exe musi miec nagranie
            // w archiwum, a zestawy z wieloma nagraniami musza dawac
            // wiecej niz jedno - inaczej jednostka powtarza te sama kwestie.
            int n = 0;
            const sndev::Ev *ev = sndev::table(n);
            int maja = 0;
            for (int i = 0; i < n; ++i)
                if (!g_menu.sfx.resolveAll(ev[i].id).empty()) ++maja;
            int wiele = 0, razem = 0, lodzi = 0;
            for (int t = 1; t <= 40; ++t) {
                int id = units::orderSoundId(t);
                size_t c = id ? g_menu.sfx.resolveAll(id).size() : 0;
                if (c) ++lodzi;
                if (c > 1) ++wiele;
                razem += int(c);
            }
            std::printf("dzwieki: zdarzen %d z %d ma nagranie, lodzi %d z 40,"
                        " nagran %d, zestawow z wyborem %d\n",
                        maja, n, lodzi, razem, wiele);
            for (int i = 0; i < n && i < 6; ++i) {
                std::vector<std::string> v = g_menu.sfx.resolveAll(ev[i].id);
                std::printf("  %-16s %4d -> %s\n", ev[i].name, ev[i].id,
                            v.empty() ? "BRAK" : v[0].c_str());
            }
            // Alarm to trzeci numer bloku - u wszystkich to samo `bsen_001`.
            int alarmow = 0;
            for (int t = 1; t <= 40; ++t) {
                std::vector<std::string> v =
                    g_menu.sfx.resolveAll(Menu::sfxAlarm(t));
                if (!v.empty() && v[0] == "bsen_001") ++alarmow;
            }
            std::printf("  alarm (id+1) = bsen_001 dla %d z 40 typow\n", alarmow);
            // **Dzwieki musza sie nakladac, nie przerywac.** Mikser ma
            // dwanascie glosow; puszczamy piec naraz i liczymy, ile gra.
            g_menu.sfx.stop();
            int zaczete = 0;
            for (int q = 0; q < 5; ++q)
                if (g_menu.sfx.play(ev[q % n].id)) ++zaczete;
            std::printf("  mikser: %s, zaczetych %d, gra naraz %d\n",
                        g_menu.sfx.mix.ok() ? "otwarty" : "BRAK", zaczete,
                        g_menu.sfx.mix.voices());
            g_menu.sfx.stop();
            // Kwestia po badaniu: baza 89/92/95 plus numer rasy.
            int civE2 = g_menu.Me().civ;
            for (int r3 = 0; r3 < 3; ++r3) {
                g_menu.Me().civ = r3;
                g_menu.SyncRace();
                // Ktory numer co znaczy, wiadomo **z nagrania**, nie z
                // kolejnosci: 92/93/94 to "nowa klasa lodzi podwodnych",
                // 95/96/97 "nowa struktura". Kiedys bylo tu odwrotnie.
                int u = -1, b2 = -1, p2 = -1;
                for (int i = 0; i < 178; ++i) {
                    int q = g_menu.ResearchSfx(i);
                    if (q >= 95) b2 = q; else if (q >= 92) u = q; else p2 = q;
                }
                std::printf("  badanie %s: ulepszenie %d, lodz %d, struktura %d\n",
                            r3 == 0 ? "WS" : (r3 == 1 ? "BO" : "SI"), p2, u, b2);
            }
            g_menu.Me().civ = civE2;
            g_menu.SyncRace();
            std::printf("  narrator z DATA/TASKS: BIP %s, WS %s, BO %s, SI %s\n",
                        g_menu.sfx.hasTask("DEFAULT_BIP") ? "jest" : "brak",
                        g_menu.sfx.hasTask("DEFAULT_WS") ? "jest" : "brak",
                        g_menu.sfx.hasTask("DEFAULT_BO") ? "jest" : "brak",
                        g_menu.sfx.hasTask("DEFAULT_SI") ? "jest" : "brak");

            // **Kwestie lektora.** Blok 50..169 to `baza + rasa`, po trzy
            // numery na zdarzenie. Sprawdzamy komplet: kazda baza ma miec
            // nagranie dla wszystkich trzech ras. Jedyny brak to "za malo
            // silikonu" - tego licznika maja tylko Silikony.
            {
                static const int kSay[] = {
                    sndev::SAY_WIN, sndev::SAY_LOSE, sndev::SAY_ATTACKED,
                    sndev::SAY_NO_CORIUM, sndev::SAY_NO_GOLD,
                    sndev::SAY_NO_METAL, sndev::SAY_NO_ENERGY,
                    sndev::SAY_UNIT_LOST, sndev::SAY_RESEARCH,
                    sndev::SAY_NEW_BOAT, sndev::SAY_NEW_BLD,
                    sndev::SAY_BUILT, sndev::SAY_BOAT_DONE,
                    sndev::SAY_DISMANTLED, sndev::SAY_CAPTURED,
                };
                const int sn = int(sizeof(kSay) / sizeof(kSay[0]));
                int pelne = 0;
                for (int i = 0; i < sn; ++i) {
                    int ok3 = 0;
                    for (int r = 0; r < 3; ++r)
                        if (!g_menu.sfx.resolveAll(sndev::say(kSay[i], r)).empty())
                            ++ok3;
                    if (ok3 == 3) ++pelne;
                }
                std::printf("  lektor: %d zdarzen wpietych, komplet trzech ras"
                            " dla %d\n", sn, pelne);
                std::printf("    atak %d, strata %d, budowa %d, lodz %d,"
                            " przejecie %d\n",
                            sndev::say(sndev::SAY_ATTACKED, 0),
                            sndev::say(sndev::SAY_UNIT_LOST, 0),
                            sndev::say(sndev::SAY_BUILT, 0),
                            sndev::say(sndev::SAY_BOAT_DONE, 0),
                            sndev::say(sndev::SAY_CAPTURED, 0));
            }

            // **Tablice po grupie budynku**, obie z exe. Sprawdzian, ze
            // indeksem jest grupa (TOBJ-50), a nie typ lodzi: niezerowe
            // wpisy dzwieku postoju musza wypasc na samych wiezyczkach.
            int idleN = 0, idleTur = 0, actWS = 0, actBO = 0, actSI = 0;
            for (int tobj = 50; tobj < 50 + sndev::GROUPS; ++tobj) {
                if (sndev::bldIdle(tobj)) {
                    ++idleN;
                    if (wep::bldGun(tobj, 0).armed() || wep::bldGun(tobj, 1).armed())
                        ++idleTur;
                }
                if (!g_menu.sfx.resolveAll(sndev::bldActive(tobj, 0)).empty()) ++actWS;
                if (!g_menu.sfx.resolveAll(sndev::bldActive(tobj, 1)).empty()) ++actBO;
                if (!g_menu.sfx.resolveAll(sndev::bldActive(tobj, 2)).empty()) ++actSI;
            }
            std::printf("  budynki: postoj %d wpisow, z tego wiezyczek %d;"
                        " zaznaczenie WS %d BO %d SI %d\n",
                        idleN, idleTur, actWS, actBO, actSI);

            // Petla otoczenia: 1207 `surn_001`, rodzaj 1 z `StartGame`.
            // Ma isc **obok** efektow, nie zamiast nich - wiec po jej
            // wlaczeniu dwanascie glosow ma dalej byc wolnych.
            g_menu.sfx.stop();
            bool amb = g_menu.sfx.ambient(sndev::SFX_AMBIENT, 0.3f);
            int freeAfter = g_menu.sfx.mix.voices();
            g_menu.sfx.play(sndev::SFX_CLICK);
            std::printf("  tlo: %s, glosow zajetych po wlaczeniu %d,"
                        " efekt obok %s\n",
                        amb ? "gra" : "BRAK", freeAfter,
                        g_menu.sfx.mix.voices() > freeAfter ? "tak" : "NIE");
            g_menu.sfx.stopAmbient();
            g_menu.sfx.stop();
        }
        {   // Gorny pasek: cztery przyciski z exe. Grafika, miejsce w pasie,
            // brak nachodzenia i to, ze klikniecie otwiera wlasciwe okno.
            int civE = g_menu.Me().civ;
            for (int r2 = 0; r2 < 3; ++r2) {
                g_menu.Me().civ = r2;
                g_menu.SyncRace();
                int nb = 0;
                const Menu::TopBtn *bb = Menu::TopBtns(nb);
                int zGrafika = 0, pozaPasem = 0, nachodzi = 0, dziala = 0;
                RECT prev = { 0, 0, 0, 0 };
                for (int k = 0; k < nb; ++k) {
                    RECT r;
                    if (!g_menu.TopBtnRect(k, r)) break;
                    char rec[48];
                    std::snprintf(rec, sizeof(rec), "%s_%s0", bb[k].rec,
                                  g_menu.hudRace);
                    const panel::Image *im = g_menu.hud.image(rec);
                    if (im && im->ok()) ++zGrafika;
                    if (r.top < 0 || r.bottom > Menu::HUD_T) ++pozaPasem;
                    if (k > 0 && r.left < prev.right) ++nachodzi;
                    prev = r;
                    // Klikniecie w srodek przycisku ma otworzyc jego okno.
                    g_menu.winOpen = Menu::WIN_NONE;
                    g_menu.PanelClick(int(r.left + r.right) / 2,
                                      int(r.top + r.bottom) / 2);
                    if (g_menu.winOpen == bb[k].win) ++dziala;
                }
                g_menu.winOpen = Menu::WIN_NONE;
                RECT oh;
                bool ohelp = g_menu.ObjHelpRect(oh);
                if (ohelp) {
                    g_menu.PanelClick(int(oh.left + oh.right) / 2,
                                      int(oh.top + oh.bottom) / 2);
                    ohelp = g_menu.winOpen == Menu::WIN_HELP;
                    g_menu.winOpen = Menu::WIN_NONE;
                }
                std::printf("  gorny pasek %s: %d z 4 grafik, poza pasem %d,"
                            " nachodzi %d, otwiera %d z 4, obiekt %s\n",
                            r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                            zGrafika, pozaPasem, nachodzi, dziala,
                            ohelp ? "tak" : "NIE");

                // **Responsywnosc**: ramka jest rozciagana, a przyciski nie,
                // wiec sprawdzamy, ze kazdy siedzi w SWOJEJ wnece - tej
                // samej, ktora wypada z `TopMapX` na jej krawedziach.
                {
                    const panel::Image *fr = g_menu.hud.frame(
                        g_menu.hudRace,
                        panel::Set::variantFor(g_clientW, g_clientH));
                    int pozaWneka = 0;
                    bool si = r2 == 2;
                    for (int k = 0; k < nb && fr && fr->ok(); ++k) {
                        RECT r;
                        if (!g_menu.TopBtnRect(k, r)) break;
                        int x = si ? bb[k].xSi : bb[k].xHum;
                        int wl = g_menu.TopMapX(x, fr->w);
                        int wr2 = g_menu.TopMapX(x + Menu::TOP_BTN_W, fr->w);
                        if (r.left < wl - 1 || r.right > wr2 + 1) ++pozaWneka;
                    }
                    std::printf("  pasek %s %dx%d: ramka %dx%d, przyciskow poza"
                                " wneka %d\n",
                                r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                                g_clientW, g_clientH,
                                fr ? fr->w : 0, fr ? fr->h : 0, pozaWneka);
                }

                {   // Wskaznik tlenu/energii: znaczniki musza siedziec
                    // w swoim gniezdzie i zmieniac barwe ze stanem.
                    const panel::Image *p0 = g_menu.hud.image(
                        (std::string("IND_PNT_") + g_menu.hudRace + "_0").c_str());
                    const panel::Image *fr2 = g_menu.hud.frame(
                        g_menu.hudRace,
                        panel::Set::variantFor(g_clientW, g_clientH));
                    int S2 = fr2 ? fr2->w - 239 : 0;
                    int lewo = fr2 ? g_menu.TopMapX(S2 + Menu::IND_X0, fr2->w) : 0;
                    int prawo = fr2 ? g_menu.TopMapX(
                        S2 + Menu::IND_X0 + Menu::IND_PIPS * Menu::IND_STEP,
                        fr2->w) : 0;
                    int mam = 0;
                    for (int q = 0; q < 5; ++q) {
                        char rec[32];
                        std::snprintf(rec, sizeof(rec), "IND_PNT_%s_%d",
                                      g_menu.hudRace, q);
                        if (g_menu.hud.image(rec)) ++mam;
                    }
                    std::printf("  wskaznik %s: gniazdo %d..%d (%d px na %d"
                                " znacznikow po %dx%d), barw %d z 5\n",
                                r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                                lewo, prawo, prawo - lewo, Menu::IND_PIPS,
                                p0 ? p0->w : 0, p0 ? p0->h : 0, mam);
                }

                // Podpowiedz: czy miesci sie na ekranie i czy nie wchodzi
                // na sasiedni przycisk ani na liczniki zasobow.
                int zle = 0, przyPrzycisku = 0;
                for (int k = 0; k < nb; ++k) {
                    RECT r;
                    if (!g_menu.TopBtnRect(k, r)) break;
                    int tw = 0;
                    for (const char *c = bb[k].tip; *c; ++c)
                        tw += g_menu.font.charWidth(uint8_t(*c));
                    RECT t;
                    t.left = r.left; t.top = r.bottom + 1;
                    t.right = t.left + tw + 8;
                    t.bottom = t.top + g_menu.font.height() + 4;
                    if (t.right > g_clientW || t.bottom > g_clientH
                        || t.left < 0 || t.top < 0) ++zle;
                    if (t.top - r.bottom <= 2) ++przyPrzycisku;
                    for (int j = 0; j < nb; ++j) {  // nie na innym przycisku
                        RECT o2;
                        if (j == k || !g_menu.TopBtnRect(j, o2)) continue;
                        if (t.left < o2.right && t.right > o2.left
                            && t.top < o2.bottom && t.bottom > o2.top) ++zle;
                    }
                }
                std::printf("  podpowiedzi %s: poza ekranem albo na przycisku %d,"
                            " przy przycisku %d z %d, barwa %06X\n",
                            r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                            zle, przyPrzycisku, nb,
                            unsigned(g_menu.RaceAccent()));

                // Zamykanie: kazde okno z tlem musi dac sie zamknac
                // przyciskiem ZAMKNIJ.
                int zamyka = 0, ileOkien = 0, wrysowany = 0;
                for (int wi = Menu::WIN_PAUSE; wi < Menu::WIN_COUNT; ++wi) {
                    RECT cb, baked;
                    g_menu.winOpen = wi;
                    if (!g_menu.CloseButton(wi, cb)) continue;
                    ++ileOkien;
                    if (g_menu.WinCloseRect(wi, baked)) ++wrysowany;
                    g_menu.WinClick(int(cb.left + cb.right) / 2,
                                    int(cb.top + cb.bottom) / 2);
                    if (g_menu.winOpen == Menu::WIN_NONE) ++zamyka;
                }
                g_menu.winOpen = Menu::WIN_NONE;
                std::printf("  zamykanie %s: %d z %d okien, wrysowany przycisk %d\n",
                            r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                            zamyka, ileOkien, wrysowany);

                // **Napisy nie moga lezec na grafice.** Kazde okno ma
                // w tle czarne pole tresci; sprawdzamy, ze jest zmierzone
                // i ze wszystkie wiersze, ktore okno wypisalo, mieszcza sie
                // w nim co do piksela.
                int zmierzone = 0, mieszcza = 0, sprawdzone = 0, kolizje = 0;
                for (int wi = Menu::WIN_PAUSE; wi < Menu::WIN_COUNT; ++wi) {
                    RECT wr;
                    if (!g_menu.WinBkg(wi)) continue;
                    g_menu.winOpen = wi;
                    if (!g_menu.WinRect(wi, wr)) continue;
                    ++sprawdzone;
                    if (g_menu.Fields(wi)) ++zmierzone;
                    g_menu.Compose();
                    RECT b = g_menu.WinBody(wi, wr);
                    RECT tb = g_menu.WinTitleBar(wi, wr);
                    int lh = g_menu.font.height() + 3;
                    int last = b.top + 2 + (g_menu.winRows - 1) * lh
                             + g_menu.font.height();
                    bool inWin = b.left >= wr.left && b.right <= wr.right
                              && b.top >= wr.top && b.bottom <= wr.bottom;
                    if (inWin && (g_menu.winRows == 0 || last <= b.bottom))
                        ++mieszcza;
                    else std::printf("    GUI window %d: rows=%d last=%d body=[%ld,%ld,%ld,%ld] window=[%ld,%ld,%ld,%ld]\n",
                                     wi,g_menu.winRows,last,b.left,b.top,b.right,b.bottom,
                                     wr.left,wr.top,wr.right,wr.bottom);
                    // Tlo tekstu nie moze wejsc na belke tytulu ani na
                    // przycisk ZAMKNIJ.
                    if (b.top < tb.bottom) { ++kolizje; std::printf("    GUI window %d: body overlaps title\n",wi); }
                    RECT cb;
                    if (g_menu.CloseButton(wi, cb) && b.bottom > cb.top) {
                        ++kolizje; std::printf("    GUI window %d: body overlaps close button\n",wi);
                    }
                }
                g_menu.winOpen = Menu::WIN_NONE;
                std::printf("  napisy %s: belka zmierzona %d z %d, tlo w oknie"
                            " i wiersze w tle %d, kolizje %d\n",
                            r2 == 0 ? "WS" : (r2 == 1 ? "BO" : "SI"),
                            zmierzone, sprawdzone, mieszcza, kolizje);
            }
            g_menu.Me().civ = civE;
            g_menu.SyncRace();
        }

        // Badania od poczatku do konca: laboratorium, gniazdo, odliczanie.
        // Bez budynku badawczego nic nie ruszy, wiec trzeba go postawic -
        // dokladnie to sprawdza osobno sekcja "badania WS/BO/SI" wyzej.
        {   size_t ile0 = g_menu.blds.size();
            Menu::Bld lab;
            lab.owner = uint32_t(g_menu.me);
            lab.tobj = uint32_t(g_menu.PlayerSide() == 2 ? 85 : 53);
            lab.x = 4; lab.y = 4;
            lab.hp = lab.hpMax = 1000;
            g_menu.blds.push_back(lab);
            g_menu.selBld.clear();
            g_menu.selBld.push_back(int(g_menu.blds.size()) - 1);
            g_menu.researchBld = int(g_menu.blds.size()) - 1;
            g_menu.winOpen = Menu::WIN_RESEARCH;
            g_menu.winPage = 0;
            int zl0 = g_menu.Me().bank.gold;
            RECT r, sl;
            int busy = -1;
            if (g_menu.WinRect(Menu::WIN_RESEARCH, r)
                && g_menu.ResearchSlot(r, 0, sl)) {
                g_menu.WinClick(int(sl.left) + 4, int(sl.top) + 4);
                busy = g_menu.Me().research.empty() ? -1 : g_menu.Me().research.front().tech;
            }
            // Czasy sa teraz z gry - do 500 s - wiec petla musi im starczyc.
            for (int t = 0; t < 60000 && !g_menu.Me().research.empty(); ++t)
                g_menu.StepWindows(0.1f);
            int zbadanych = 0;
            for (uint8_t v : g_menu.Me().techDone) if (v) ++zbadanych;
            std::printf("badania: do wyboru %d, zlecone %d, zloto %d -> %d, "
                        "zbadanych %d\n",
                        g_menu.TechOpenCount(), busy, zl0,
                        g_menu.Me().bank.gold, zbadanych);
            g_menu.winOpen = Menu::WIN_NONE;
            g_menu.researchBld = -1;
            g_menu.selBld.clear();
            while (g_menu.blds.size() > ile0) g_menu.blds.pop_back();
        }

        // Rynek: kurs dryfuje, kupno zabiera zloto.
        {   g_menu.winOpen = Menu::WIN_TRADE;
            float k0 = g_menu.rateCor;
            for (int t = 0; t < 400; ++t) g_menu.StepWindows(0.1f);
            int zl0 = g_menu.Me().bank.gold, kor0 = g_menu.Me().bank.corium;
            RECT r;
            g_menu.WinRect(Menu::WIN_TRADE, r);
            g_menu.WinClick(r.left + 20, r.top + 30 + 2 * (g_menu.font.height() + 3));
            std::printf("rynek: kurs korium %.1f -> %.1f, zloto %d -> %d, "
                        "korium %d -> %d\n",
                        double(k0), double(g_menu.rateCor), zl0,
                        g_menu.Me().bank.gold, kor0, g_menu.Me().bank.corium);
        }

        // Dyplomacja: sojusz zdejmuje wrogosc.
        {   int inny = -1;
            for (int i = 0; i < Menu::MAX_PLAYERS; ++i)
                if (g_menu.players[i].active && i != (g_menu.me & 7)) { inny = i; break; }
            if (inny >= 0) {
                bool przed = g_menu.Foe(uint32_t(g_menu.me), uint32_t(inny));
                g_menu.ally[inny] = 1;
                bool po = g_menu.Foe(uint32_t(g_menu.me), uint32_t(inny));
                g_menu.ally[inny] = 0;
                std::printf("dyplomacja: gracz %d wrogiem %s, po sojuszu %s\n",
                            inny, przed ? "tak" : "NIE", po ? "TAK" : "nie");
            }
        }

        // Pauza zatrzymuje swiat.
        {   g_menu.units.resize(1);
            // Lodz w drodze - inaczej "nie ruszyla sie" nic nie dowodzi.
            {   int px2, py2;
                g_menu.sel.assign(1, 0);
                g_menu.CellToScreen(g_menu.units[0].x + 16.0f,
                                    g_menu.units[0].y, px2, py2);
                g_menu.camX += px2 - SCREEN_W / 2;
                g_menu.camY += py2 - SCREEN_H / 2;
                g_menu.ClampCamera();
                g_menu.CellToScreen(g_menu.units[0].x + 16.0f,
                                    g_menu.units[0].y, px2, py2);
                g_menu.OrderMove(px2, py2);
            }
            float x0 = g_menu.units[0].x;
            g_menu.paused = true;
            for (DWORD t = 1000; t < 6000; t += 100) g_menu.StepUnits(t);
            float xp = g_menu.units[0].x;
            g_menu.paused = false;
            for (DWORD t = 6000; t < 11000; t += 100) g_menu.StepUnits(t);
            std::printf("pauza: przesuniecie w pauzie %.2f, po wznowieniu %.2f\n",
                        double(xp - x0), double(g_menu.units[0].x - xp));
        }
        return 0;
    }

    // --orders <mapa>: rozkazy trwale. W grze kazdy z nich to osobne BOATCMD
    // (STBoatC::CmdToObj) i osobna funkcja Grp* - razem czternascie rozkazow
    // grupowych. Tu sprawdzamy te, ktore remake wlasnie dostal: patrol,
    // oslone i rozbiorke ze zwrotem polowy ceny.
    // --keys <mapa>: grupy pod klawiszami.
    if (argc > 3 && std::strcmp(argv[2], "--keys") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mk = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mk.begin(), mk.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }

        // **Do grupy ida lodzie o WYZSZYCH indeksach**, a topimy jedna
        // o nizszym - inaczej `Reap()` niczego nie przesuwa i sprawdzian
        // przechodzi tak samo z przenumerowaniem, jak i bez niego.
        std::vector<int> nasze;
        for (int i = 0; i < int(g_menu.units.size()); ++i)
            if ((g_menu.units[size_t(i)].owner & 7) == uint32_t(g_menu.me & 7))
                nasze.push_back(i);
        g_menu.sel.clear();
        for (size_t q = nasze.size() >= 5 ? nasze.size() - 4 : 1;
             q < nasze.size(); ++q)
            g_menu.sel.push_back(nasze[q]);
        int ile = int(g_menu.sel.size());
        g_menu.AssignGroup(1);
        int wGrupie = int(g_menu.group[1].size());
        // Kto jest ostatni w grupie - po zatopieniu kogos PRZED nim indeksy
        // sie przesuwaja i bez przenumerowania grupa wskaze kogo innego.
        int ostatni = wGrupie ? g_menu.group[1][size_t(wGrupie - 1)] : -1;
        float xPrzed = ostatni >= 0 ? g_menu.units[size_t(ostatni)].x : 0;
        float yPrzed = ostatni >= 0 ? g_menu.units[size_t(ostatni)].y : 0;

        // Topimy lodz spoza grupy o NAJNIZSZYM indeksie.
        int ofiara = -1;
        for (int i = 0; i < int(g_menu.units.size()) && ofiara < 0; ++i) {
            bool wGr = false;
            for (int q : g_menu.group[1]) if (q == i) wGr = true;
            if (!wGr) ofiara = i;
        }
        bool niziej = ofiara >= 0 && ofiara < ostatni;
        if (ofiara >= 0) g_menu.units[size_t(ofiara)].hp = 0;
        g_menu.Reap();

        int po = int(g_menu.group[1].size());
        int nowyOst = po ? g_menu.group[1][size_t(po - 1)] : -1;
        bool tenSam = nowyOst >= 0 && nowyOst < int(g_menu.units.size())
                   && g_menu.units[size_t(nowyOst)].x == xPrzed
                   && g_menu.units[size_t(nowyOst)].y == yPrzed;

        g_menu.sel.clear();
        g_menu.RecallGroup(1, 1000);
        int przywolane = int(g_menu.sel.size());

        {   // **Lancuch klawiszy to jeden `else if`**, wiec galaz
            // wczesniejsza przykrywa pozniejsza bez sladu. Nie da sie
            // tego zobaczyc w kodzie na oko - trzeba **nacisnac kazdy
            // klawisz i sprawdzic, czy cos sie stalo**.
            struct KT { int vk; int win; const char *co; };
            static const KT kt[] = {
                { 'P',     Menu::WIN_PAUSE,    "pauza" },
                { 'O',     Menu::WIN_OPTIONS,  "ustawienia" },
                { 'B',     Menu::WIN_BEHAV,    "zachowanie" },
                { 'Y',     Menu::WIN_FORM,     "szyk" },
                { 'U',     Menu::WIN_UPGRADE,  "ulepszenia" },
                { 'G',     Menu::WIN_TRADE,    "rynek" },
                { 'I',     Menu::WIN_INFOC,    "infocentrum" },
                { 'J',     Menu::WIN_DIPLO,    "dyplomacja" },
                { VK_F1,   Menu::WIN_HELP,     "pomoc" },
                { VK_F2,   Menu::WIN_TTREE,    "drzewo" },
                { VK_F4,   Menu::WIN_MOBJ,     "obiekty" },
                { VK_F5,   Menu::WIN_SETANY,   "mina akustyczna" },
            };
            g_menu.devInfo = false;
            g_menu.palOpen = false;
            int otwiera = 0, martwych = 0;
            std::string zle;
            for (const KT &k : kt) {
                g_menu.winOpen = Menu::WIN_NONE;
                g_menu.OnKey(k.vk);
                if (g_menu.winOpen == k.win) ++otwiera;
                else { ++martwych; zle += " "; zle += k.co; }
            }
            g_menu.winOpen = Menu::WIN_NONE;
            // Glebokosc: PageUp/PageDown to rozkaz, a nie strona palety.
            int zZ = 0;
            for (int i2 = 0; i2 < int(g_menu.units.size()); ++i2)
                if ((g_menu.units[size_t(i2)].owner & 7)
                        == uint32_t(g_menu.me & 7)) {
                    g_menu.sel.assign(1, i2);
                    break;
                }
            if (!g_menu.sel.empty()) {
                float przed = g_menu.units[size_t(g_menu.sel[0])].wantZ;
                g_menu.OnKey(VK_PRIOR);
                if (g_menu.units[size_t(g_menu.sel[0])].wantZ != przed) ++zZ;
                przed = g_menu.units[size_t(g_menu.sel[0])].wantZ;
                g_menu.OnKey(VK_NEXT);
                if (g_menu.units[size_t(g_menu.sel[0])].wantZ != przed) ++zZ;
            }
            // **I odwrotnie**: przy otwartej palecie PageUp ma stronicowac
            // palete, a nie zmieniac glebokosc - inaczej guard dziala tylko
            // w jedna strone i nic tego nie pokaze.
            int zPaleta = 0;
            if (!g_menu.sel.empty()) {
                g_menu.palOpen = true;
                float przed2 = g_menu.units[size_t(g_menu.sel[0])].wantZ;
                g_menu.OnKey(VK_PRIOR);
                if (g_menu.units[size_t(g_menu.sel[0])].wantZ == przed2) ++zPaleta;
                g_menu.palOpen = false;
            }
            // Panel deweloperski ma chodzic **tylko** z nakladka spod `~`.
            bool bezNakladki = g_menu.adminOpen;
            g_menu.devInfo = true;
            g_menu.OnKey('P');
            bool zNakladka = g_menu.adminOpen;
            g_menu.adminOpen = false;
            g_menu.devInfo = false;
            std::printf("klawisze gry: otwiera %d z %d%s%s, glebokosc %d z 2, przy palecie glebokosc stoi %d z 1, panel dev bez nakladki %s, z nakladka %s\n",
                        otwiera, int(sizeof(kt) / sizeof(kt[0])),
                        martwych ? ", MARTWE:" : "", zle.c_str(), zZ, zPaleta,
                        bezNakladki ? "TAK" : "nie",
                        zNakladka ? "tak" : "NIE");
        }

        // ================= PANEL ADMINA =================
        //
        // To narzedzie, nie funkcja gry - ale kazda jego pozycja ma dzialac,
        // a lista mozliwosci **nie jest sprawdzianem**: dokladnie tak psuly
        // sie kiedyś przyciski panelu komend. Mierzymy wiec skutek kazdej
        // z czterech rzeczy osobno, kazda z kontrola.
        {
            g_menu.screen = SCR_TERRAIN;
            g_menu.devInfo = false;
            g_menu.adminOpen = true;
            g_menu.adminTab = 0;
            g_menu.adminPick = -1;
            g_menu.adminBld = -1;
            g_menu.adminOwner = uint32_t(g_menu.me & 7);

            // --- 1. ikony, nie nazwy ---
            //
            // Liczymy gniazda, ktore dostaly **prawdziwa ikone** z CONTROLG.
            // Sam licznik pozycji nie dowodzilby niczego: siatka pustych
            // ramek wygladalaby w nim tak samo.
            auto zIkona = [&](int tab) {
                g_menu.adminTab = tab;
                int n = g_menu.AdminItems(), ma = 0;
                for (int i = 0; i < n; ++i) {
                    int typ = g_menu.AdminItemAt(i);
                    if (typ > 0 && g_menu.AdminIcon(typ, tab == 0)) ++ma;
                }
                return std::make_pair(n, ma);
            };
            auto lodzie = zIkona(0), budynki = zIkona(1);

            // Czy panel w ogole dociera na plotno, i czy zakladki roznia sie
            // trescia - dwie zakladki pokazujace to samo wygladalyby
            // w liczbach powyzej identycznie.
            g_menu.adminTab = 0;
            g_menu.Compose();
            std::vector<uint32_t> zLodziami = g_menu.canvas;
            g_menu.adminTab = 1;
            g_menu.Compose();
            std::vector<uint32_t> zBudynkami = g_menu.canvas;
            g_menu.adminOpen = false;
            g_menu.Compose();
            std::vector<uint32_t> bez = g_menu.canvas;
            g_menu.adminOpen = true;
            auto rozne = [](const std::vector<uint32_t> &p,
                            const std::vector<uint32_t> &q) {
                size_t n = 0;
                for (size_t i = 0; i < p.size() && i < q.size(); ++i)
                    if (p[i] != q[i]) ++n;
                return n;
            };
            std::printf("panel admina: lodzi %d (ikon %d), budynkow %d (ikon %d);"
                        " na plotnie %zu px, zakladki roznia sie %zu px %s\n",
                        lodzie.first, lodzie.second,
                        budynki.first, budynki.second,
                        rozne(bez, zLodziami), rozne(zLodziami, zBudynkami),
                        (rozne(bez, zLodziami) > 0
                         && rozne(zLodziami, zBudynkami) > 0)
                            ? "-> rysuje sie" : "-> NIE RYSUJE SIE");

            // --- 2. stawianie: lodz i budynek ---
            //
            // Idziemy **droga gracza**: klik w gniazdo siatki uzbraja, klik
            // na mapie stawia. Wolanie `SpawnAt` obok panelu nie mowiloby,
            // czy klik w ogole dochodzi - to ta sama lekcja, co przy
            // gniezdzie palety pod przyciskiem komend.
            Menu::AdmLay AL = g_menu.AdminLayout();
            int gx = AL.box.left + 6 + Menu::ADM_CW / 2;
            int gy = AL.yGrid + Menu::ADM_CH / 2;
            RECT vp2 = g_menu.Viewport();
            int mx = (vp2.left + vp2.right) / 2, my = (vp2.top + vp2.bottom) / 2;

            g_menu.adminTab = 0;
            g_menu.adminPick = -1;
            size_t luPrzed = g_menu.units.size();
            bool klikL = g_menu.AdminClick(gx, gy);
            int uzbrL = g_menu.adminPick;
            g_menu.AdminClick(mx, my);
            size_t luPo = g_menu.units.size();

            g_menu.adminTab = 1;
            g_menu.adminBld = -1;
            size_t lbPrzed = g_menu.blds.size();
            bool klikB = g_menu.AdminClick(gx, gy);
            int uzbrB = g_menu.adminBld;
            g_menu.AdminClick(mx, my);
            size_t lbPo = g_menu.blds.size();
            bool gotowy = lbPo > lbPrzed
                       && g_menu.blds.back().buildLeft <= 0
                       && g_menu.blds.back().hp == g_menu.blds.back().hpMax;
            std::printf("  stawianie: klik w siatke %s/%s, uzbroil lodz %d"
                        " budynek %d, lodzi %zu -> %zu, budynkow %zu -> %zu,"
                        " budynek gotowy %s\n",
                        klikL ? "tak" : "NIE", klikB ? "tak" : "NIE",
                        uzbrL, uzbrB, luPrzed, luPo, lbPrzed, lbPo,
                        gotowy ? "tak" : "NIE");

            // --- 3. turbo ---
            //
            // Mierzone **zegarem partii**: `playSecs` rosnie o tyle, ile
            // swiat naprawde przeliczyl. Samo przestawienie licznika
            // wygladaloby tak samo, gdyby nic z niego nie wynikalo.
            auto ileCzasu = [&](int turbo) {
                g_menu.admTurbo = turbo;
                g_menu.lastUnitStep = 0;
                g_menu.paused = false;
                DWORD t = 100000;
                g_menu.StepUnits(t);              // pierwszy tylko ustawia baze
                float przed = g_menu.playSecs;
                for (int k = 0; k < 5; ++k) { t += 100; g_menu.StepUnits(t); }
                return g_menu.playSecs - przed;
            };
            float t1 = ileCzasu(1), t4 = ileCzasu(4);
            g_menu.admTurbo = 1;
            // Cykl przycisku: 1 -> 2 -> 4 -> 8 -> 1.
            int cykl[5] = { 0, 0, 0, 0, 0 };
            for (int k = 0; k < 5; ++k) {
                g_menu.AdminClick(AL.box.left + 10, AL.yTog + 8);
                cykl[k] = g_menu.admTurbo;
            }
            g_menu.admTurbo = 1;
            std::printf("  turbo: x1 przeliczyl %.2f s, x4 %.2f s %s;"
                        " cykl %d %d %d %d %d\n",
                        double(t1), double(t4),
                        (t1 > 0 && t4 > t1 * 3.5f) ? "-> szybciej"
                                                   : "-> BEZ SKUTKU",
                        cykl[0], cykl[1], cykl[2], cykl[3], cykl[4]);

            // --- 4. szybkie badania i szybka budowa ---
            //
            // Obie pary: z wlaczonym przelacznikiem ma zejsc **wielokrotnie**
            // wiecej niz bez niego. Sam spadek nic by nie mowil - schodzi
            // tak czy tak.
            auto badanie = [&](bool szybko) {
                Menu::Player &p = g_menu.Me();
                Menu::Bld lab;
                lab.owner=uint32_t(g_menu.me & 7); lab.tobj=53; lab.hp=1000;
                int count=0;const auto *defs=tech::list(count);
                float duration=float(std::max(1,defs[0].ticks/100)*100)/25.0f;
                int savedGold=p.bank.gold;p.bank.gold=100000;
                p.research={{0,int(g_menu.blds.size()),duration,0}}; g_menu.blds.push_back(lab);
                g_menu.admFastRes = szybko;
                float przed = p.research.front().left;
                g_menu.StepResearch(1.0f * (szybko ? Menu::ADM_FAST : 1.0f));
                float ub = przed - (p.research.empty()?0:p.research.front().left);
                p.bank.gold=savedGold;
                p.research.clear();
                g_menu.blds.pop_back();
                g_menu.admFastRes = false;
                return ub;
            };
            float bWol = badanie(false), bSzyb = badanie(true);

            auto budowa = [&](bool szybko) {
                size_t byloB = g_menu.blds.size();
                Menu::Bld nb;
                nb.owner = uint32_t(g_menu.me & 7);
                nb.tobj = 59;
                nb.x = 20; nb.y = 20;
                nb.hpMax = 1000; nb.hp = nb.hpBuilt = 100;
                nb.buildTotal = nb.buildLeft = 100.0f;
                nb.span = 1;
                g_menu.blds.push_back(nb);
                g_menu.admFastBld = szybko;
                float przed = g_menu.blds.back().buildLeft;
                g_menu.StepBuilding(1.0f * (szybko ? Menu::ADM_FAST : 1.0f));
                float ub = przed - g_menu.blds.back().buildLeft;
                g_menu.admFastBld = false;
                g_menu.blds.resize(byloB);
                return ub;
            };
            float uWol = budowa(false), uSzyb = budowa(true);
            std::printf("  szybkie: badania %.2f -> %.2f s/tik %s,"
                        " budowa %.2f -> %.2f s/tik %s\n",
                        double(bWol), double(bSzyb),
                        bSzyb > bWol * 5.0f ? "dziala" : "BEZ SKUTKU",
                        double(uWol), double(uSzyb),
                        uSzyb > uWol * 5.0f ? "dziala" : "BEZ SKUTKU");

            // Oddaj stan nietkniety - inaczej nastepne audyty mierza co innego.
            g_menu.units.resize(luPrzed);
            g_menu.blds.resize(lbPrzed);
            g_menu.adminOpen = false;
            g_menu.adminTab = 0;
            g_menu.adminPick = -1;
            g_menu.adminBld = -1;
            g_menu.admTurbo = 1;
            g_menu.admFastRes = g_menu.admFastBld = false;
            g_menu.sel.clear();
            g_menu.selBld.clear();
        }
        std::printf("grupy: przypisano %d z %d, po zatopieniu %d, "
                    "ofiara nizej %s, ta sama lodz %s, przywolano %d\n",
                    wGrupie, ile, po, niziej ? "tak" : "nie",
                    tenSam ? "tak" : "NIE", przywolane);

        // Drugie przywolanie w pol sekundy przesuwa kamere.
        int cx0 = g_menu.camX, cy0 = g_menu.camY;
        g_menu.RecallGroup(1, 1200);
        std::printf("  dwuklik grupy: kamera %d,%d -> %d,%d (%s)\n",
                    cx0, cy0, g_menu.camX, g_menu.camY,
                    (g_menu.camX != cx0 || g_menu.camY != cy0) ? "przesunela"
                                                               : "BEZ RUCHU");
        return 0;
    }

    // --form <mapa>: osiem szykow z okna, kazdy ma dawac inny uklad.
    if (argc > 3 && std::strcmp(argv[2], "--form") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mf = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mf.begin(), mf.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }

        // Osiem lodzi jednego gracza, zeby bylo co ustawiac.
        g_menu.sel.clear();
        for (int i = 0; i < int(g_menu.units.size()) && int(g_menu.sel.size()) < 8; ++i)
            if ((g_menu.units[size_t(i)].owner & 7) == uint32_t(g_menu.me & 7))
                g_menu.sel.push_back(i);
        int ile = int(g_menu.sel.size());
        std::printf("szyk: lodzi w grupie %d, gniazd w oknie %d\n",
                    ile, Menu::FORM_COUNT);
        if (ile < 2) { std::printf("  za malo lodzi\n"); return 0; }

        int cx = int(g_menu.units[size_t(g_menu.sel[0])].x) / 2 + 6;
        int cy = int(g_menu.units[size_t(g_menu.sel[0])].y) / 2 + 6;
        static const char *nm[Menu::FORM_COUNT] = {
            "linia", "klin", "luzny", "blokada",
            "fala", "ukos", "kwadrat", "brak" };
        std::vector<std::vector<POINT>> uklady;
        for (int f = 0; f < Menu::FORM_COUNT; ++f) {
            g_menu.formation = f;
            g_menu.formGap = 1;
            g_menu.formTurn = 0;
            std::vector<POINT> p = g_menu.DistributeTargets(cx, cy, ile, 2);
            uklady.push_back(p);
            // rozpietosc: najdalsze dwa punkty
            int minx = 9999, maxx = -9999, miny = 9999, maxy = -9999;
            for (const POINT &q : p) {
                if (q.x < minx) minx = q.x;
                if (q.x > maxx) maxx = q.x;
                if (q.y < miny) miny = q.y;
                if (q.y > maxy) maxy = q.y;
            }
            std::printf("  %-8s rozpietosc %dx%d\n", nm[f],
                        maxx - minx + 1, maxy - miny + 1);
        }
        // Ile par szykow daje IDENTYCZNE rozstawienie - to jest ten pomiar,
        // ktory lapie "osiem nazw, jeden ksztalt".
        int takie_same = 0;
        for (size_t a = 0; a < uklady.size(); ++a)
            for (size_t b2 = a + 1; b2 < uklady.size(); ++b2) {
                bool rowne = uklady[a].size() == uklady[b2].size();
                for (size_t k = 0; rowne && k < uklady[a].size(); ++k)
                    if (uklady[a][k].x != uklady[b2][k].x
                        || uklady[a][k].y != uklady[b2][k].y) rowne = false;
                if (rowne) ++takie_same;
            }
        std::printf("  par o identycznym ukladzie: %d z %d\n", takie_same,
                    Menu::FORM_COUNT * (Menu::FORM_COUNT - 1) / 2);
        // Rozstaw ma rozciagac.
        g_menu.formation = Menu::FORM_LINE;
        g_menu.formGap = 1;
        std::vector<POINT> w1 = g_menu.DistributeTargets(cx, cy, ile, 2);
        g_menu.formGap = 3;
        std::vector<POINT> w3 = g_menu.DistributeTargets(cx, cy, ile, 2);
        auto rozp = [](const std::vector<POINT> &p) {
            int a = 9999, b3 = -9999;
            for (const POINT &q : p) { if (q.x < a) a = q.x; if (q.x > b3) b3 = q.x; }
            return b3 - a + 1;
        };
        std::printf("  rozstaw 1 -> %d kratek, rozstaw 3 -> %d kratek\n",
                    rozp(w1), rozp(w3));
        // Zrzut okna: podswietlenie ma trafic w gniazda wrysowane
        // w grafike, wiec to trzeba OBEJRZEC, nie tylko policzyc.
        g_clientW = 1024; g_clientH = 768;
        g_menu.formation = Menu::FORM_WAVE;
        g_menu.winOpen = Menu::WIN_FORM;
        g_menu.Compose();
        if (FILE *o = std::fopen("form.raw", "wb")) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        std::printf("  form.raw %dx%d\n", SCREEN_W, SCREEN_H);
        return 0;
    }

    if (argc > 3 && std::strcmp(argv[2], "--orders") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1280;
        g_clientH = 860;
        g_menu.FitCanvas();
        g_menu.ApplyZoom(4);
        g_menu.fogOn = false;
        g_menu.aiOn = false;
        g_menu.units.clear();
        g_menu.blds.clear();
        g_menu.sel.clear();
        g_menu.selBld.clear();

        int sx = -1, sy = -1;
        for (int by = 3; by < g_menu.terr.bh - 8 && sx < 0; ++by)
            for (int bx = 3; bx < g_menu.terr.bw - 8; ++bx)
                if (g_menu.Passable(bx, by)) { sx = bx; sy = by; break; }

        auto put = [&](int cx, int cy) {
            Menu::Unit u;
            u.owner = uint32_t(g_menu.me);
            u.type = 1;
            u.x = float(cx); u.y = float(cy);
            u.tx = u.x; u.ty = u.y;
            u.dir = Menu::DIR_REST;
            u.spawned = true;
            g_menu.SetUnitStats(u);
            g_menu.units.push_back(u);
            return int(g_menu.units.size()) - 1;
        };

        // ---- PATROL: lodz ma wracac miedzy dwoma punktami ----------------
        int a = put(sx * 2, sy * 2);
        g_menu.sel.assign(1, a);
        int px, py;
        // Kamera na lodz, inaczej drugi koniec trasy wypada poza oknem mapy
        // i ArmedClick slusznie go odrzuca.
        g_menu.CellToScreen(float(sx * 2 + 10), float(sy * 2), px, py);
        g_menu.camX += px - SCREEN_W / 2;
        g_menu.camY += py - SCREEN_H / 2;
        g_menu.ClampCamera();
        g_menu.CellToScreen(float(sx * 2 + 20), float(sy * 2), px, py);
        g_menu.armed = Menu::ARM_PATROL;
        g_menu.ArmedClick(px, py);
        std::printf("patrol: wlaczony %s, koniec A %.0f,%.0f, koniec B %.0f,%.0f\n",
                    g_menu.units[size_t(a)].patrol ? "tak" : "NIE",
                    g_menu.units[size_t(a)].patAx, g_menu.units[size_t(a)].patAy,
                    g_menu.units[size_t(a)].patBx, g_menu.units[size_t(a)].patBy);
        int zawroty = 0;
        bool doB = g_menu.units[size_t(a)].patToB;
        float lo = 1e9f, hi = -1e9f;
        for (DWORD t = 1000; t < 121000; t += 100) {
            g_menu.StepUnits(t);
            const Menu::Unit &u = g_menu.units[size_t(a)];
            if (u.patToB != doB) { ++zawroty; doB = u.patToB; }
            if (u.x < lo) lo = u.x;
            if (u.x > hi) hi = u.x;
        }
        std::printf("  po 120 s: zawrotow %d, przejechal od %.0f do %.0f\n",
                    zawroty, lo, hi);

        // ---- OSLONA: lodz ma wracac na swoje miejsce ---------------------
        g_menu.units.clear();
        int b = put(sx * 2, sy * 2);
        g_menu.sel.assign(1, b);
        {   // **Oslony nie ma w panelu lodzi i nie ma jej tam w grze.**
            // `BUT_GUARD` to komenda **5**, a tablica gniazd z exe
            // (`stbldui.h`) nie daje jej **zadnemu** z czterdziestu typow:
            // lodzie maja 1, 2, 3, 4, 6, 47 i garsc specjalnych. Rozkaz
            // `GrpGuard` istnieje, tylko wydaje sie go panelem akcji
            // specjalnych (`SpecPanelTy`, napis 20053 STRZEZ STREFY),
            // ktorego remake nie ma. Liczymy to, zamiast szukac guzika,
            // ktorego nigdzie nie bylo.
            int zGuard = 0, ile = 0;
            {   int ns = 0;
                const bldui::Slot *sl = bldui::unitSlots(ns);
                for (int i = 0; i < ns; ++i) {
                    if (sl[i].slot == 0) ++ile;
                    if (sl[i].cmd == 5) ++zGuard;
                }
            }
            g_menu.OrderGuard();
            std::printf("oslona: typow lodzi %d, z komenda 5 w panelu %d "
                        "(w grze tez zero - to panel akcji specjalnych), "
                        "wlaczona %s, miejsce %.0f,%.0f\n",
                        ile, zGuard,
                        g_menu.units[size_t(b)].guard ? "tak" : "nie",
                        g_menu.units[size_t(b)].guardX, g_menu.units[size_t(b)].guardY);

        {   // **FANTOM.** Przewodnik: generator „makes it invisible to the
            // enemy units and sonar", trzyma **48 s** i laduje sie **145 s**,
            // a Liberator „detects stealth in **radius of 8**". Typy z tym
            // przyciskiem to 22 PHANTOM i 37 STEALTH_SCOUT, a strefe
            // antystealth maja 11, 23 i 35 - obie listy sa z exe.
            //
            // Mierzone tabelka: da sie wziac na cel / widac na plotnie, przy
            // generatorze wylaczonym, wlaczonym i wlaczonym przy Liberatorze
            // w zasiegu. Samo „po wlaczeniu nie widac" niczego by nie
            // dowodzilo - wyszloby tak samo, gdyby lodz nie rysowala sie
            // nigdy; dlatego obok stoi pomiar z generatorem wylaczonym.
            g_menu.units.clear();
            g_menu.blds.clear();
            g_menu.Reap();
            // Mgla zaslonilaby obca lodz niezaleznie od generatora - wtedy
            // wszystkie trzy pomiary wyszlyby zerem i wiersz nie mierzylby
            // niczego.
            const bool bylaMgla = g_menu.fogOn;
            g_menu.fogOn = false;
            // **`Compose()` rysuje swiat dopiero na ekranie terenu.** Bez tego
            // skladalo sie menu i wszystkie trzy pomiary wychodzily zerem -
            // czyli dokladnie tak samo, jak przy dzialajacym ukryciu.
            const int bylEkran = int(g_menu.screen);
            g_menu.screen = SCR_TERRAIN;
            const uint32_t ja = uint32_t(g_menu.me & 7);
            const uint32_t on = (ja + 1) & 7;
            const float cx = float(g_menu.terr.bw), cy = float(g_menu.terr.bh);
            {   int sx0, sy0;
                g_menu.CellToScreen(cx, cy, sx0, sy0);
                g_menu.camX += sx0 - SCREEN_W / 2;
                g_menu.camY += sy0 - SCREEN_H / 2;
            }
            // **Liczymy piksele w pudelku wokol samego fantoma**, a nie tusz
            // calego plotna - tamten to w 99% teren i nie drgnalby nawet
            // wtedy, gdyby lodz zniknela.
            int px0, py0;
            g_menu.CellToScreen(cx, cy, px0, py0);
            const int BOX = 40;
            g_menu.Compose();
            std::vector<uint32_t> tlo = g_menu.canvas;   // scena bez lodzi
            auto stan = [&](bool schowany, bool zLiberatorem) {
                g_menu.units.clear();
                Menu::Unit f;                          // fantom wroga
                f.owner = on; f.type = 22;
                f.hp = f.hpMax = units::unitHp(22);
                f.x = cx; f.y = cy;
                g_menu.units.push_back(f);
                if (zLiberatorem) {
                    Menu::Unit l;                      // nasz Liberator w zasiegu
                    l.owner = ja; l.type = 11;
                    l.hp = l.hpMax = units::unitHp(11);
                    l.x = cx + 12.0f; l.y = cy;        // 6 blokow: mniej niz 8
                    g_menu.units.push_back(l);
                }
                g_menu.units[0].hidden = schowany;
                g_menu.units[0].cloak = Menu::STEALTH_SECS;
                bool isB = false;
                int cel = g_menu.FindTarget(ja, cx, cy, 30, isB);
                bool celowalny = !isB && cel == 0;
                g_menu.Compose();
                size_t widac = 0;
                for (int y = py0 - BOX; y <= py0 + BOX; ++y) {
                    if (y < 0 || y >= SCREEN_H) continue;
                    for (int x = px0 - BOX; x <= px0 + BOX; ++x) {
                        if (x < 0 || x >= SCREEN_W) continue;
                        size_t q = size_t(y) * SCREEN_W + size_t(x);
                        if (q < tlo.size() && tlo[q] != g_menu.canvas[q]) ++widac;
                    }
                }
                return std::make_pair(celowalny, widac);
            };
            auto jawny = stan(false, false);
            auto skryty = stan(true, false);
            auto wykryty = stan(true, true);
            std::printf("fantom: cel jawny %s, schowany %s, "
                        "przy Liberatorze %s\n",
                        jawny.first ? "tak" : "NIE",
                        skryty.first ? "TAK (zle)" : "nie",
                        wykryty.first ? "tak" : "NIE");
            std::printf("  na plotnie: jawny %zu px, schowany %zu, "
                        "przy Liberatorze %zu -> %s\n",
                        jawny.second, skryty.second, wykryty.second,
                        (jawny.second > 0 && skryty.second == 0 &&
                         wykryty.second > 0) ? "znika i wraca" : "BEZ ZMIANY");
            // Licznik generatora: 48 s starcza, 50 juz nie.
            g_menu.units.resize(1);
            g_menu.units[0].hidden = true;
            g_menu.units[0].cloak = Menu::STEALTH_SECS;
            for (int t = 0; t < 47; ++t) g_menu.StepCloak(1.0f);
            bool po47 = g_menu.units[0].hidden;
            for (int t = 0; t < 3; ++t) g_menu.StepCloak(1.0f);
            bool po50 = g_menu.units[0].hidden;
            for (int t = 0; t < 145; ++t) g_menu.StepCloak(1.0f);
            float odzysk = g_menu.units[0].cloak;
            std::printf("  generator: po 47 s %s, po 50 s %s, "
                        "po 145 s ladowania %.0f z %.0f s\n",
                        po47 ? "chowa" : "JUZ NIE", po50 ? "WCIAZ CHOWA" : "zgasl",
                        double(odzysk), double(Menu::STEALTH_SECS));
            g_menu.fogOn = bylaMgla;
            g_menu.screen = Screen(bylEkran);
        }
        }
        // Odciagamy ja recznie i patrzymy, czy wroci.
        g_menu.units[size_t(b)].x += 8.0f;
        for (DWORD t = 1000; t < 61000; t += 100) g_menu.StepUnits(t);
        {   const Menu::Unit &u = g_menu.units[size_t(b)];
            float dx = u.x - u.guardX, dy = u.y - u.guardY;
            std::printf("  po odciagnieciu o 8 komorek wrocil na %.1f komorki\n",
                        std::sqrt(dx * dx + dy * dy));
        }

        // ---- OKNO GLEBOKOSCI: cel za wysoko jest nie do dosiegniecia -----
        {   g_menu.units.clear();
            int me = put(sx * 2, sy * 2);
            g_menu.units[size_t(me)].z = 0.0f;
            g_menu.units[size_t(me)].owner = uint32_t(g_menu.me);
            int foe = put(sx * 2 + 2, sy * 2);
            g_menu.units[size_t(foe)].owner = uint32_t(g_menu.me) + 1;
            bool isB = false;
            int blisko = -1, daleko = -1;
            g_menu.units[size_t(foe)].z = 0.0f;
            blisko = g_menu.FindTarget(g_menu.units[size_t(me)].owner,
                                       g_menu.units[size_t(me)].x,
                                       g_menu.units[size_t(me)].y, 10, isB, 0.0f);
            g_menu.units[size_t(foe)].z = 4.0f;
            daleko = g_menu.FindTarget(g_menu.units[size_t(me)].owner,
                                       g_menu.units[size_t(me)].x,
                                       g_menu.units[size_t(me)].y, 10, isB, 0.0f);
            std::printf("glebokosc: cel na tym samym poziomie %s, "
                        "cztery poziomy wyzej %s (okno %.0f)\n",
                        blisko >= 0 ? "widziany" : "NIE",
                        daleko >= 0 ? "WIDZIANY" : "nie", double(Menu::FIRE_DEPTH));
        }

        // ---- DOK NAPRAWCZY: leczy wlasne lodzie --------------------------
        {   g_menu.units.clear();
            g_menu.blds.clear();
            Menu::Bld d;
            d.owner = uint32_t(g_menu.me);
            d.tobj = 51;                    // dok naprawczy
            d.x = sx * 2;
            d.y = sy * 2;
            d.hpMax = d.hp = 1000;
            d.span = 2;
            g_menu.blds.push_back(d);
            int r = put(sx * 2 + 1, sy * 2);
            g_menu.units[size_t(r)].hp = g_menu.units[size_t(r)].hpMax / 4;
            int hp0 = g_menu.units[size_t(r)].hp;
            g_menu.repaired = 0;
            for (int t = 0; t < 100; ++t) g_menu.StepEconomy(0.1f);
            std::printf("naprawa: hp %d -> %d z %d, oddanych punktow %d\n",
                        hp0, g_menu.units[size_t(r)].hp,
                        g_menu.units[size_t(r)].hpMax, g_menu.repaired);
            // Ta sama lodz na najwyzszym poziomie - dok nie ma jak jej dosiegnac.
            g_menu.units[size_t(r)].hp = hp0;
            g_menu.units[size_t(r)].z = float(maps::LEVELS - 1);
            g_menu.repaired = 0;
            for (int t = 0; t < 100; ++t) g_menu.StepEconomy(0.1f);
            {   // Ruchome czesci: dok naprawczy ma wlasny pasek _repd_ani_.
                const spr::Strip *an = g_menu.BldAniStrip(g_menu.blds[0]);
                int sx2, sy2;
                g_menu.CellToScreen(float(g_menu.blds[0].x),
                                    float(g_menu.blds[0].y), sx2, sy2);
                int drew = g_menu.DrawBldAni(g_menu.blds[0], sx2, sy2);
                std::printf("  ruchome czesci doku: pasek %s (%d klatek), "
                            "narysowane %d\n",
                            an ? "jest" : "BRAK", an ? int(an->count()) : 0, drew);
            }
            std::printf("  na najwyzszym poziomie oddanych punktow %d "
                        "(ma byc 0 - budynek potrzebuje miejsca nad soba)\n",
                        g_menu.repaired);
            g_menu.blds.clear();
        }

        // ---- PRZEJECIE: tylko cudza cywilizacja --------------------------
        {   g_menu.units.clear();
            g_menu.blds.clear();
            int c = put(sx * 2, sy * 2);
            g_menu.units[size_t(c)].type = 12;      // Konstruktor (WS)
            g_menu.units[size_t(c)].z = 0.0f;
            g_menu.sel.assign(1, c);
            Menu::Bld e;
            e.owner = uint32_t(g_menu.me) + 1;      // cudzy
            e.tobj = 51;
            e.x = sx * 2 + 2;
            e.y = sy * 2;
            e.hpMax = e.hp = 800;
            e.span = 2;
            g_menu.blds.push_back(e);
            g_menu.players[(uint32_t(g_menu.me) + 1) & 7].active = true;
            g_menu.players[(uint32_t(g_menu.me) + 1) & 7].civ = 1;   // Black Octopi
            g_menu.players[uint32_t(g_menu.me) & 7].civ = 0;         // White Sharks
            int bx2, by2;
            g_menu.CellToScreen(float(e.x), float(e.y), bx2, by2);
            g_menu.camX += bx2 - SCREEN_W / 2;
            g_menu.camY += by2 - SCREEN_H / 2;
            g_menu.ClampCamera();
            g_menu.CellToScreen(float(e.x), float(e.y), bx2, by2);
            uint32_t przed = g_menu.blds[0].owner;
            g_menu.armed = Menu::ARM_CAPTURE;
            g_menu.ArmedClick(bx2, by2);
            std::printf("przejecie: wlasciciel %u -> %u (%s), komunikat: %s\n",
                        przed, g_menu.blds.empty() ? 0u : g_menu.blds[0].owner,
                        (!g_menu.blds.empty() && g_menu.blds[0].owner
                            == uint32_t(g_menu.me)) ? "przejete" : "NIE",
                        g_menu.buildMsg.c_str());
            // Ta sama proba na budynku tej samej cywilizacji ma sie nie udac.
            if (!g_menu.blds.empty()) {
                g_menu.blds[0].owner = uint32_t(g_menu.me) + 2;
                g_menu.players[(uint32_t(g_menu.me) + 2) & 7].active = true;
                g_menu.players[(uint32_t(g_menu.me) + 2) & 7].civ = 0;  // tez WS
                g_menu.armed = Menu::ARM_CAPTURE;
                g_menu.ArmedClick(bx2, by2);
                std::printf("  ta sama cywilizacja: %s (%s)\n",
                            g_menu.blds[0].owner == uint32_t(g_menu.me)
                                ? "PRZEJETE - zle" : "odmowa - dobrze",
                            g_menu.buildMsg.c_str());
            }
            g_menu.blds.clear();
        }

        // ---- PRZEWOZENIE: lodz naprawcza bierze inna na poklad ------------
        {   g_menu.units.clear();
            int r = put(sx * 2, sy * 2);
            g_menu.units[size_t(r)].type = 7;       // RepSub
            int c = put(sx * 2 + 2, sy * 2);
            g_menu.units[size_t(c)].hp = g_menu.units[size_t(c)].hpMax / 2;
            g_menu.sel.assign(1, r);
            int cx2, cy2;
            g_menu.CellToScreen(g_menu.units[size_t(c)].x,
                                g_menu.units[size_t(c)].y, cx2, cy2);
            g_menu.camX += cx2 - SCREEN_W / 2;
            g_menu.camY += cy2 - SCREEN_H / 2;
            g_menu.ClampCamera();
            g_menu.UnitToScreen(g_menu.units[size_t(c)], cx2, cy2);
            g_menu.armed = Menu::ARM_TAKE;
            g_menu.ArmedClick(cx2, cy2);
            std::printf("przewozenie: zabrana %s (%s)\n",
                        g_menu.units[size_t(r)].carry == c ? "tak" : "NIE",
                        g_menu.buildMsg.c_str());
            // Wieziona ma jechac razem z wiozaca i nic nie robic.
            g_menu.units[size_t(r)].x += 5.0f;
            for (int t = 0; t < 20; ++t) g_menu.StepEconomy(0.1f);
            float dx = g_menu.units[size_t(c)].x - g_menu.units[size_t(r)].x;
            float dy = g_menu.units[size_t(c)].y - g_menu.units[size_t(r)].y;
            std::printf("  po ruchu wiozacej odstep %.2f komorki, "
                        "wieziona wylaczona %s\n",
                        std::sqrt(dx * dx + dy * dy),
                        g_menu.units[size_t(c)].carried ? "tak" : "NIE");
            // Transport alone does not enable repair; nearby boats must not heal.
            g_menu.units[size_t(r)].carry = -1;
            g_menu.units[size_t(c)].carried = false;
            int hp0 = g_menu.units[size_t(c)].hp;
            g_menu.repaired = 0;
            for (int t = 0; t < 100; ++t) g_menu.StepEconomy(0.1f);
            std::printf("  naprawa w polu: hp %d -> %d, oddanych %d\n",
                        hp0, g_menu.units[size_t(c)].hp, g_menu.repaired);
        }

        // ---- GLEBOKOSC: rozkaz trzyma poziom ------------------------------
        {   g_menu.units.clear();
            int d = put(sx * 2, sy * 2);
            g_menu.sel.assign(1, d);
            float z0 = g_menu.units[size_t(d)].z;
            g_menu.OrderDepth(+2);
            for (int t = 0; t < 60; ++t) g_menu.StepUnits(DWORD(200000 + t * 100));
            std::printf("glebokosc: z %.1f -> %.1f (zadane %.1f)\n",
                        z0, g_menu.units[size_t(d)].z, g_menu.units[size_t(d)].wantZ);
        }

        // ---- REGENERACJA SILIKONOW ----------------------------------------
        {   std::printf("regeneracja SI: 100%% co %.1f s, 75%% co %.1f, "
                        "50%% co %.1f, 25%% co %.1f, 10%% co %.1f\n",
                        (regeneration::quote(32,100,0,false).interval+1)/25.0,
                        (regeneration::quote(32,75,0,false).interval+1)/25.0,
                        (regeneration::quote(32,50,0,false).interval+1)/25.0,
                        (regeneration::quote(32,25,0,false).interval+1)/25.0,
                        (regeneration::quote(32,10,0,false).interval+1)/25.0);
        }

        // ---- ENERGIA SILIKONOW: samoleczenie i Replenisher ---------------
        {   g_menu.units.clear();
            g_menu.blds.clear();
            g_menu.players[uint32_t(g_menu.me) & 7].civ = 2;    // Silikony
            int e = put(sx * 2, sy * 2);
            g_menu.units[size_t(e)].type = 32;                  // Escort
            g_menu.SetUnitStats(g_menu.units[size_t(e)]);
            g_menu.units[size_t(e)].hp = g_menu.units[size_t(e)].hpMax / 2;
            int hp0 = g_menu.units[size_t(e)].hp;
            int en0 = g_menu.units[size_t(e)].energy;
            g_menu.repaired = 0;
            for (int t = 0; t < 200; ++t) g_menu.StepEconomy(0.1f);
            std::printf("energia SI: Escort zapas %d, hp %d -> %d po 20 s, "
                        "energii zostalo %d, wyleczonych %d\n",
                        en0, hp0, g_menu.units[size_t(e)].hp,
                        g_menu.units[size_t(e)].energy, g_menu.repaired);
            // Replenisher dolewa energii ze skarbca.
            g_menu.units[size_t(e)].energy = 0;
            int r = put(sx * 2 + 1, sy * 2);
            g_menu.units[size_t(r)].type = 29;                  // Replenisher
            g_menu.SetUnitStats(g_menu.units[size_t(r)]);
            int kasa0 = g_menu.Me().bank.gold;
            g_menu.recharged = 0;
            for (int t = 0; t < 50; ++t) g_menu.StepEconomy(0.1f);
            std::printf("  Replenisher: energia 0 -> %d, ze skarbca ubylo %d\n",
                        g_menu.units[size_t(e)].energy,
                        kasa0 - g_menu.Me().bank.gold);
            g_menu.players[uint32_t(g_menu.me) & 7].civ = 0;
            g_menu.units.clear();
        }

        // ---- TELEPORT: z bramy do bramy ----------------------------------
        {   g_menu.units.clear();
            g_menu.blds.clear();
            for (int g = 0; g < 2; ++g) {
                Menu::Bld b;
                b.owner = uint32_t(g_menu.me);
                b.tobj = 55;                    // brama
                b.x = sx * 2 + g * 20;
                b.y = sy * 2;
                b.hpMax = b.hp = 600;
                b.span = 2;
                g_menu.blds.push_back(b);
            }
            int t = put(sx * 2, sy * 2);
            g_menu.sel.assign(1, t);
            float x0 = g_menu.units[size_t(t)].x;
            g_menu.OrderTeleport();
            std::printf("teleport: x %.0f -> %.0f (druga brama %d), komunikat: %s\n",
                        x0, g_menu.units[size_t(t)].x, g_menu.blds[1].x,
                        g_menu.buildMsg.c_str());
            // Bez drugiej bramy ma odmowic.
            g_menu.blds.pop_back();
            g_menu.units[size_t(t)].x = float(g_menu.blds[0].x);
            g_menu.units[size_t(t)].y = float(g_menu.blds[0].y);
            g_menu.OrderTeleport();
            std::printf("  z jedna brama: %s\n", g_menu.buildMsg.c_str());
            g_menu.blds.clear();
        }

        // ---- ZACHOWANIE: obrona / postoj / atak --------------------------
        {   g_menu.units.clear();
            int u = put(sx * 2, sy * 2);
            g_menu.sel.assign(1, u);
            int nn = 0;
            const Menu::Cmd *cc = g_menu.CmdsFor(false, nn);
            int slot = -1;
            for (int i = 0; i < nn; ++i)
                if (std::strstr(cc[i].rec, "BEHAVIOUR")) { slot = i; break; }
            int b0 = g_menu.units[size_t(u)].beh;
            if (slot >= 0) g_menu.CmdAction(false, slot);
            int b1 = g_menu.units[size_t(u)].beh;
            if (slot >= 0) g_menu.CmdAction(false, slot);
            int b2 = g_menu.units[size_t(u)].beh;
            if (slot >= 0) g_menu.CmdAction(false, slot);
            std::printf("zachowanie: przycisk %s, cykl %d -> %d -> %d -> %d\n",
                        slot >= 0 ? "jest" : "BRAK", b0, b1, b2,
                        g_menu.units[size_t(u)].beh);
        }

        // ---- PARY KOPALNIA-MAGAZYN i przydzial transportowcow ------------
        {   g_menu.units.clear();
            g_menu.blds.clear();
            auto bld = [&](uint32_t tobj, int cx, int cy) {
                Menu::Bld b;
                b.owner = uint32_t(g_menu.me);
                b.tobj = tobj;
                b.x = cx; b.y = cy;
                b.hpMax = b.hp = 800;
                b.span = 2;
                g_menu.blds.push_back(b);
            };
            bld(79, sx * 2, sy * 2);            // kopalnia metalu
            bld(57, sx * 2 + 14, sy * 2);       // kopalnia korium
            bld(59, sx * 2 + 2, sy * 2 + 2);    // magazyn
            for (int i = 0; i < 5; ++i) {
                int h = put(sx * 2 + i, sy * 2 + 6);
                g_menu.units[size_t(h)].type = 8;   // TranSub
                g_menu.SetUnitStats(g_menu.units[size_t(h)]);
            }
            int przydz = g_menu.DistributeMD(uint32_t(g_menu.me));
            std::printf("pary MD: %d par, przydzielono %d z 5 transportowcow\n",
                        int(g_menu.mdPairs.size()), przydz);
            for (size_t k = 0; k < g_menu.mdPairs.size(); ++k) {
                const Menu::MDPair &pr = g_menu.mdPairs[k];
                std::printf("  para %d: kopalnia %d, magazyn %d, kurs %d ms, "
                            "chce %d lodzi\n",
                            int(k), g_menu.blds[size_t(pr.mine)].tobj,
                            g_menu.blds[size_t(pr.depot)].tobj, pr.tripMs, pr.want);
            }
        }

        // ---- POWROT DO DOKU ----------------------------------------------
        //
        // **Obie rasy, bo numer doku od rasy zalezy**: 51 u ludzi, 110
        // (`mrest`) u Silikonow. Test stawial na sztywno 51, wiec to, ze
        // silikonowy dok byl calkiem martwy - `NearestDock` go nie widzial,
        // `StepRepair` przy nim nie leczyl - przechodzilo bez sladu.
        for (int dk = 0; dk < 2; ++dk) {
            const int tobjDok = dk == 0 ? 51 : 110;
            g_menu.units.clear();
            g_menu.blds.clear();
            Menu::Bld d;
            d.owner = uint32_t(g_menu.me);
            d.tobj = uint32_t(tobjDok);
            d.x = sx * 2 + 10;
            d.y = sy * 2;
            d.hpMax = d.hp = 900;
            d.span = 2;
            g_menu.blds.push_back(d);
            int u = put(sx * 2, sy * 2);
            g_menu.units[size_t(u)].hp = g_menu.units[size_t(u)].hpMax / 3;
            g_menu.sel.assign(1, u);
            int ruszylo = g_menu.OrderReturnRepair();
            int hp0 = g_menu.units[size_t(u)].hp;
            for (DWORD t = 1000; t < 61000; t += 100) g_menu.StepUnits(t);
            for (int t = 0; t < 200; ++t) g_menu.StepEconomy(0.1f);
            float dx = g_menu.units[size_t(u)].x - float(d.x);
            float dy = g_menu.units[size_t(u)].y - float(d.y);
            int hp1 = g_menu.units[size_t(u)].hp;
            std::printf("powrot do doku (TOBJ %d, %s): ruszylo %d,"
                        " doplynelo na %.1f komorki, hp %d -> %d%s\n",
                        tobjDok, dk == 0 ? "ludzie" : "Silikony",
                        ruszylo, std::sqrt(dx * dx + dy * dy), hp0, hp1,
                        (ruszylo && hp1 > hp0) ? "" : "  <- DOK NIE LECZY");
            g_menu.blds.clear();
        }

        // ---- NALOT DC BOMBERA --------------------------------------------
        {   g_menu.units.clear();
            // Bombowiec od razu nad celem - inaczej trzy Sentinele zestrzeliwuja
            // go po drodze i test mierzy walke, a nie nalot.
            int b = put(sx * 2 + 7, sy * 2);
            g_menu.units[size_t(b)].type = 4;       // DC Bomber
            g_menu.SetUnitStats(g_menu.units[size_t(b)]);
            g_menu.sel.assign(1, b);
            int ofiar = 0;
            for (int i = 0; i < 3; ++i) {
                int f = put(sx * 2 + 6 + i, sy * 2);
                g_menu.units[size_t(f)].owner = uint32_t(g_menu.me) + 1;
                g_menu.SetUnitStats(g_menu.units[size_t(f)]);
                ++ofiar;
            }
            g_menu.players[(uint32_t(g_menu.me) + 1) & 7].active = true;
            int przed = int(g_menu.units.size());
            wep::Gun g = wep::bombGun(4);
            int px2, py2;
            g_menu.CellToScreen(float(sx * 2 + 7), float(sy * 2), px2, py2);
            g_menu.camX += px2 - SCREEN_W / 2;
            g_menu.camY += py2 - SCREEN_H / 2;
            g_menu.ClampCamera();
            g_menu.CellToScreen(float(sx * 2 + 7), float(sy * 2), px2, py2);
            g_menu.armed = Menu::ARM_BOMB;
            g_menu.ArmedClick(px2, py2);
            std::printf("nalot: bomb %d po %d obrazen, celow %d, zlecony %s\n",
                        g.shots, g.dmgUnit, ofiar,
                        g_menu.units[size_t(b)].bombing ? "tak" : "NIE");
            for (DWORD t = 1000; t < 61000; t += 100) g_menu.StepUnits(t);
            {   int zywych = 0;
                for (const Menu::Unit &q : g_menu.units)
                    if ((q.owner & 7) != uint32_t(g_menu.me) && q.hp > 0) ++zywych;
                std::printf("  wrogow zywych %d, bombowiec nadal leci %s\n",
                            zywych,
                            g_menu.units.empty() ? "?" :
                            (g_menu.units[0].bombing ? "tak" : "nie"));
            }
            std::printf("  po nalocie lodzi %d -> %d\n", przed, int(g_menu.units.size()));
        }

        // ---- ZWIAD: szerszy wzrok ----------------------------------------
        {   g_menu.units.clear();
            int a = put(sx * 2, sy * 2);
            g_menu.units[size_t(a)].type = 1;       // Sentinel
            int b = put(sx * 2, sy * 2);
            g_menu.units[size_t(b)].type = 37;      // Stealth Scout
            std::printf("zwiad: zwykla widzi %d blokow, zwiadowca %d\n",
                        g_menu.SightOf(g_menu.units[size_t(a)]),
                        g_menu.SightOf(g_menu.units[size_t(b)]));
        }

        // ---- KRADZIEZ TECHNOLOGII ----------------------------------------
        {   g_menu.techLeft = 0;
            g_menu.GrantTech(52, 1);            // Armcenter - daje
            float t1 = g_menu.techLeft;
            g_menu.techLeft = 0;
            g_menu.GrantTech(51, 1);            // dok naprawczy - nie daje
            float t2 = g_menu.techLeft;
            g_menu.techLeft = 0;
            g_menu.GrantTech(59, 1);            // magazyn - nie daje
            std::printf("technologie: Armcenter %.0f s, dok naprawczy %.0f s, "
                        "magazyn %.0f s (dwa ostatnie maja byc 0)\n",
                        double(t1), double(t2), double(g_menu.techLeft));
        }

        // ---- LINIA STRZALU: skala zaslania ------------------------------
        {   // Szukamy pary: strzelec i cel po dwoch stronach czegos wyzszego.
            int ax = -1, ay = -1, bx2 = -1, by2 = -1, hx = -1, hy = -1;
            for (int y = 2; y < g_menu.terr.bh - 2 && ax < 0; ++y)
                for (int x = 2; x < g_menu.terr.bw - 4; ++x) {
                    int l0 = g_menu.terr.topAt(x, y);
                    int l1 = g_menu.terr.topAt(x + 1, y);
                    int l2 = g_menu.terr.topAt(x + 2, y);
                    if (l1 < l0 + 2 || l1 < l2 + 2) continue;
                    ax = x; ay = y; bx2 = x + 2; by2 = y; hx = x + 1; hy = y;
                    break;
                }
            if (ax < 0) {
                std::printf("linia strzalu: nie znalazlem skaly miedzy dwoma polkami\n");
            } else {
                bool zaslonieta = g_menu.RayBlocked(float(ax * 2), float(ay * 2),
                                                    float(g_menu.terr.topAt(ax, ay)),
                                                    float(bx2 * 2), float(by2 * 2),
                                                    float(g_menu.terr.topAt(bx2, by2)));
                bool zGory = g_menu.RayBlocked(float(ax * 2), float(ay * 2),
                                               float(g_menu.terr.topAt(hx, hy)) + 1.0f,
                                               float(bx2 * 2), float(by2 * 2),
                                               float(g_menu.terr.topAt(hx, hy)) + 1.0f);
                std::printf("linia strzalu: skala %d,%d poziom %d miedzy polkami "
                            "%d i %d\n",
                            hx, hy, g_menu.terr.topAt(hx, hy),
                            g_menu.terr.topAt(ax, ay), g_menu.terr.topAt(bx2, by2));
                std::printf("  przez skale %s, ponad nia %s\n",
                            zaslonieta ? "zaslonieta - dobrze" : "PRZECHODZI",
                            zGory ? "ZASLONIETA" : "czysto - dobrze");

                // **Ktore bronie omijaja test, jest odczytane z `CheckRay`**
                // (`0x0041f9b0`): switch po numerze pocisku rzuca wyjatek dla
                // **dziewietnastu** numerow, zanim w ogole pusci promien.
                // Remake bral dotad etykiete `Guided: Yes` z przewodnika,
                // a ta ma cztery wpisy i jest **podzbiorem** tamtej listy.
                //
                // Pomiar jest trojka przez **te sama skale**, i kazda noga
                // obala co innego: pocisk z listy ma przejsc, pocisk spoza
                // niej ma zostac zatrzymany, a bez numeru ma wyjsc to samo,
                // co przed zmiana. Sam „przechodzi" nic by nie dowodzil -
                // wyszedlby tak samo, gdyby `ClearShot` przestal cokolwiek
                // sprawdzac.
                {
                    const size_t byloU = g_menu.units.size();
                    Menu::Unit c;
                    c.owner = uint32_t((g_menu.me + 1) & 7); c.type = 12;
                    c.x = float(bx2 * 2); c.y = float(by2 * 2);
                    c.z = float(g_menu.terr.topAt(bx2, by2));
                    c.hp = c.hpMax = 1000;
                    g_menu.units.push_back(c);
                    wep::Gun g0 = wep::unitGun(2);      // torpeda: Guided = No
                    float sx = float(ax * 2), sy = float(ay * 2);
                    float sz = float(g_menu.terr.topAt(ax, ay));
                    int cel = int(byloU);
                    bool bezNr = g_menu.ClearShot(sx, sy, sz, cel, false, g0, 0);
                    bool zList = g_menu.ClearShot(sx, sy, sz, cel, false, g0,
                                                  proj::ofUnit(31));
                    bool spoza = g_menu.ClearShot(sx, sy, sz, cel, false, g0,
                                                  proj::ofUnit(2));
                    int lodzi = 0, budynkow = 0;
                    for (int i = 1; i <= 40; ++i)
                        if (proj::ignoresRay(proj::ofUnit(i))) ++lodzi;
                    for (int t2 = 50; t2 <= 115; ++t2)
                        for (int sl = 0; sl < 2; ++sl)
                            if (proj::ignoresRay(proj::ofBld(t2, sl))) ++budynkow;
                    std::printf("  bez linii strzalu (CheckRay, 19 numerow):"
                                " z listy 0x%02x %s, spoza 0x%02x %s,"
                                " bez numeru %s; lodzi %d, gniazd budynkow %d\n",
                                proj::ofUnit(31),
                                zList ? "przechodzi - dobrze" : "ZATRZYMANY",
                                proj::ofUnit(2),
                                spoza ? "PRZECHODZI" : "zatrzymany - dobrze",
                                bezNr ? "PRZECHODZI" : "zatrzymany - dobrze",
                                lodzi, budynkow);
                    g_menu.units.resize(byloU);
                }
            }
        }

        // ---- ROZBIORKA: polowa ceny wraca --------------------------------
        {   Menu::Bld e;
            e.owner = uint32_t(g_menu.me);
            e.tobj = 50;
            e.x = sx * 2;
            e.y = sy * 2 + 6;
            e.hpMax = e.hp = 1000;
            e.span = 2;
            g_menu.blds.push_back(e);
            g_menu.selBld.assign(1, 0);
            cost::Price pr = cost::bldPrice(50, g_menu.SideOf(e.owner));
            int m0 = g_menu.Me().bank.metal, k0 = g_menu.Me().bank.corium;
            int nn = 0;
            const Menu::Cmd *cc = g_menu.CmdsFor(true, nn);
            int slot = -1;
            for (int i = 0; i < nn; ++i)
                if (std::strstr(cc[i].rec, "DISMANTL")
                    || std::strstr(cc[i].rec, "DISSASSEMBLE")) { slot = i; break; }
            if (slot >= 0) g_menu.CmdAction(true, slot);
            std::printf("rozbiorka: cena met %d kor %d, zwrot met %d kor %d "
                        "(polowa: %d / %d), budynkow %d\n",
                        pr.metal, pr.corium,
                        g_menu.Me().bank.metal - m0, g_menu.Me().bank.corium - k0,
                        pr.metal / 2, pr.corium / 2, int(g_menu.blds.size()));
        }
        return 0;
    }

    // --group <mapa> [ilu]: ruch pojedynczy i grupowy. W grze rozkaz niesie
    // GRUPA (STAllPlayersC::RegisterObject: obiekt bez grupy nie ma czym
    // wykonac ruchu), a Way3DGrpDistribTgt rozdaje kazdemu czlonkowi wlasna
    // komorke docelowa - grupa jednoosobowa idzie prosto w klikniety punkt.
    // Tu sprawdzamy dokladnie to: czy kazdy dostal swoje miejsce, czy nikt
    // nie stoi na kimś i czy wszyscy dojechali.
    if (argc > 3 && std::strcmp(argv[2], "--group") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        int howMany = argc > 4 ? std::atoi(argv[4]) : 9;
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1280;
        g_clientH = 860;
        g_menu.FitCanvas();
        g_menu.ApplyZoom(4);
        g_menu.fogOn = false;
        g_menu.aiOn = false;
        g_menu.units.clear();
        g_menu.sel.clear();

        g_menu.blds.clear();        // bez wiezyczek - test mierzy sam ruch
        g_menu.selBld.clear();

        // Start: pierwszy plaski kawalek mapy.
        int sx = -1, sy = -1;
        for (int by = 2; by < g_menu.terr.bh - 6 && sx < 0; ++by)
            for (int bx = 2; bx < g_menu.terr.bw - 6; ++bx)
                if (g_menu.Passable(bx, by)) { sx = bx; sy = by; break; }
        if (sx < 0) { std::printf("mapa bez wody\n"); return 1; }

        for (int i = 0; i < howMany; ++i) {
            Menu::Unit u;
            u.owner = uint32_t(g_menu.me);
            u.type = 1;
            u.x = float(sx * 2 + (i % 4) * 2);
            u.y = float(sy * 2 + (i / 4) * 2);
            u.tx = u.x; u.ty = u.y;
            u.dir = Menu::DIR_REST;
            u.spawned = true;
            g_menu.SetUnitStats(u);
            g_menu.units.push_back(u);
            g_menu.sel.push_back(i);
        }

        // Cel po drugiej stronie mapy, na przejezdnym.
        int gx = -1, gy = -1;
        for (int by = g_menu.terr.bh - 3; by > 2 && gx < 0; --by)
            for (int bx = g_menu.terr.bw - 3; bx > 2; --bx)
                if (g_menu.Passable(bx, by)) { gx = bx; gy = by; break; }
        std::printf("mapa %dx%d blokow, start %d,%d, cel %d,%d, lodzi %d\n",
                    g_menu.terr.bw, g_menu.terr.bh, sx, sy, gx, gy, howMany);

        int px, py;
        g_menu.CellToScreen(float(gx * 2), float(gy * 2), px, py);
        g_menu.camX += px - SCREEN_W / 2;
        g_menu.camY += py - SCREEN_H / 2;
        g_menu.ClampCamera();
        g_menu.CellToScreen(float(gx * 2), float(gy * 2), px, py);
        g_menu.OrderMove(px, py);

        {   // Czy kazdy dostal INNE miejsce docelowe.
            int para = 0;
            for (size_t a = 0; a < g_menu.units.size(); ++a)
                for (size_t b = a + 1; b < g_menu.units.size(); ++b)
                    if (int(g_menu.units[a].tx) == int(g_menu.units[b].tx)
                        && int(g_menu.units[a].ty) == int(g_menu.units[b].ty)) ++para;
            int bezDrogi = 0, sumaKrokow = 0;
            for (const Menu::Unit &u : g_menu.units) {
                if (u.path.empty()) ++bezDrogi;
                sumaKrokow += int(u.path.size());
            }
            std::printf("  rozdzial: wspolnych celow %d, bez drogi %d, "
                        "srednio %d krokow\n",
                        para, bezDrogi, sumaKrokow / (howMany ? howMany : 1));
        }

        // Przejazd.
        float minPara = 1e9f;
        int ruch = 0;
        for (DWORD t = 1000; t < 181000; t += 100) {
            g_menu.StepUnits(t);
            int m = 0;
            for (const Menu::Unit &u : g_menu.units) if (u.moving) ++m;
            if (m > ruch) ruch = m;
            for (size_t a = 0; a < g_menu.units.size(); ++a)
                for (size_t b = a + 1; b < g_menu.units.size(); ++b) {
                    float dx = g_menu.units[a].x - g_menu.units[b].x;
                    float dy = g_menu.units[a].y - g_menu.units[b].y;
                    float d = std::sqrt(dx * dx + dy * dy);
                    if (d < minPara) minPara = d;
                }
            if (m == 0 && t > 2000) break;
        }
        int doszlo = 0;
        std::vector<long> gniazda;
        for (const Menu::Unit &u : g_menu.units) {
            float dx = u.x - float(gx * 2), dy = u.y - float(gy * 2);
            if (dx * dx + dy * dy < 400.0f) ++doszlo;   // 20 komorek
            gniazda.push_back(long(int(u.x)) * 10000 + int(u.y));
        }
        std::sort(gniazda.begin(), gniazda.end());
        int osobnych = int(std::unique(gniazda.begin(), gniazda.end()) - gniazda.begin());
        std::printf("  przejazd: ruszylo %d, doszlo w poblize celu %d, "
                    "osobnych komorek %d z %d\n",
                    ruch, doszlo, osobnych, howMany);
        {   int nieDoszlo = 0, wRuchu = 0;
            float najgorzej = 0;
            for (const Menu::Unit &u : g_menu.units) {
                float dx = u.x - u.tx, dy = u.y - u.ty;
                float d = std::sqrt(dx * dx + dy * dy);
                if (d > 1.5f) ++nieDoszlo;
                if (d > najgorzej) najgorzej = d;
                if (u.moving) ++wRuchu;
            }
            std::printf("  do swojego gniazda nie doszlo %d lodzi "
                        "(najdalej %.1f komorki), w ruchu %d\n",
                        nieDoszlo, najgorzej, wRuchu);
        }
        std::printf("  najblizsze dwie lodzie przez caly czas: %.2f komorki\n", minPara);

        // I to samo dla jednej lodzi - grupa jednoosobowa idzie prosto w punkt.
        g_menu.units.resize(1);
        g_menu.sel.assign(1, 0);
        g_menu.units[0].x = float(sx * 2);
        g_menu.units[0].y = float(sy * 2);
        g_menu.units[0].moving = false;
        g_menu.units[0].path.clear();
        g_menu.OrderMove(px, py);
        std::printf("  pojedyncza: cel %d,%d (klikniete %d,%d), krokow %d\n",
                    int(g_menu.units[0].tx), int(g_menu.units[0].ty), gx * 2, gy * 2,
                    int(g_menu.units[0].path.size()));
        return 0;
    }

    // --depth <mapa>: kto kogo zaslania. Teren stempluje pasmo i poziom;
    // zaslonieta czesc obiektu jest wycinana, bez kolorowej sylwetki.
    if (argc > 3 && std::strcmp(argv[2], "--depth") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (g_menu.skSel < 0 || g_menu.skSel >= int(g_menu.skMaps.size()))
            return std::printf("nie ma mapy %d\n", g_menu.skSel), 1;
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1280;
        g_clientH = 860;
        g_menu.FitCanvas();
        g_menu.ApplyZoom(4);
        g_menu.fogOn = false;
        g_menu.aiOn = false;
        g_menu.CentreCamera();

        // Szukamy pary: wysokiej skaly i nizszej doliny KILKA pasm za nia.
        // W tej projekcji blok o poziom wyzej jest rysowany o LevelStep wyzej,
        // a pasmo dalej o tileH/2 nizej - zaslania wiec nie to, co lezy tuz za
        // nim, tylko to, co jest o (poziom * LevelStep / (tileH/2)) pasm dalej.
        int wx = -1, wy = -1, wl = 0, ux = -1, uy = -1, ul = 0;
        for (int by = 6; by < g_menu.terr.bh - 2 && wx < 0; ++by)
            for (int bx = 6; bx < g_menu.terr.bw - 2 && wx < 0; ++bx) {
                int l = g_menu.terr.topAt(bx, by);
                if (l < 3) continue;
                // Krok poziomu rowna sie krokowi pasma, wiec blok wyzszy
                // o L i blizszy o D pasm rysuje sie o (D - L) * krok nizej.
                // Zeby zaslonic cos, co jeszcze unosi sie nad dnem, musi byc
                // wyzszy o WIECEJ pasm, niz jest blizszy.
                for (int k = 1; k <= 3 && wx < 0; ++k) {
                    int px = bx - k, py = by - k;
                    if (px < 1 || py < 1) break;
                    int pl = g_menu.terr.topAt(px, py);
                    if (l - pl < 2 * k + 1 || !g_menu.Passable(px, py)) continue;
                    wx = bx; wy = by; wl = l;
                    ux = px; uy = py; ul = pl;
                }
            }
        if (wx < 0) { std::printf("nie znalazlem sciany z dolina za nia\n"); return 1; }
        std::printf("skala: blok %d,%d poziom %d; dolina %d,%d poziom %d "
                    "(%d pasm dalej)\n",
                    wx, wy, wl, ux, uy, ul, (wx - ux) + (wy - uy));
        std::printf("  krok poziomu %d px, krok pasma %d px\n",
                    g_menu.LevelStep(), g_menu.tileH / 2);

        auto put = [&](int cx, int cy, float z) {
            Menu::Unit u;
            u.owner = uint32_t(g_menu.me);
            u.type = 1;
            u.x = float(cx); u.y = float(cy);
            u.tx = u.x; u.ty = u.y;
            u.dir = Menu::DIR_REST;
            u.spawned = true;
            g_menu.SetUnitStats(u);
            u.z = z;
            g_menu.units.push_back(u);
        };
        auto shot = [&](const char *why, int cx, int cy, float z, const char *file) {
            g_menu.units.clear();
            g_menu.sel.clear();
            put(cx, cy, z);
            int sx2, sy2;
            g_menu.CellToScreen(float(cx), float(cy), sx2, sy2);
            int c0 = g_menu.camX, c1 = g_menu.camY;
            g_menu.camX += sx2 - g_clientW / 2;
            g_menu.camY += sy2 - g_clientH / 2;
            g_menu.ClampCamera();
            g_menu.Compose();
            std::printf("%s: przycietych obiektow %d, pikseli %d, cien %d px (odrzucony %d)\n",
                        why, g_menu.silObjs, g_menu.silUnitPixels,
                        g_menu.shadowPx, g_menu.shadowCut);
            if (file) {
                FILE *o = std::fopen(file, "wb");
                if (o) {
                    std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
                    std::fclose(o);
                }
            }
            g_menu.camX = c0;
            g_menu.camY = c1;
            return g_menu.silUnitPixels;
        };

        int za = shot("za skala", ux * 2, uy * 2, float(ul), "depth.raw");

        // Kontrola: ta sama lodz, ale wyniesiona na poziom skaly - nic jej
        // juz nie zaslania.
        int nad = shot("wyniesiona na poziom skaly", ux * 2, uy * 2, float(wl), 0);

        // I kontrola druga: plaski teren, nic wyzszego w poblizu.
        int fx = -1, fy = -1;
        for (int by = 2; by < g_menu.terr.bh - 8 && fx < 0; ++by)
            for (int bx = 2; bx < g_menu.terr.bw - 8; ++bx) {
                if (!g_menu.Passable(bx, by)) continue;
                int l = g_menu.terr.topAt(bx, by);
                bool flat = true;
                for (int dy = 0; dy <= 6 && flat; ++dy)
                    for (int dx = 0; dx <= 6; ++dx)
                        if (g_menu.terr.topAt(bx + dx, by + dy) > l) { flat = false; break; }
                if (!flat) continue;
                fx = bx; fy = by;
                break;
            }
        int otw = fx < 0 ? -1
                : shot("na otwartym", fx * 2, fy * 2, float(g_menu.terr.topAt(fx, fy)), 0);
        // Kolejnosc obiektow miedzy soba. Sama liczba zmienionych pikseli nic
        // nie mowi - lodz ZA budynkiem lezy wyzej na ekranie i legalnie wystaje
        // ponad dach. Liczy sie tylko CZESC WSPOLNA: tam, gdzie obie bryly
        // zachodza na siebie, wygrac ma ta blizsza.
        int wspol = 0, wygral = 0, wspolT = 0, wygralT = 0;
        if (fx >= 0) {
            g_menu.units.clear();
            g_menu.sel.clear();
            g_menu.blds.clear();
            int sxB, syB;
            g_menu.CellToScreen(float(fx * 2), float(fy * 2), sxB, syB);
            int c0 = g_menu.camX, c1 = g_menu.camY;
            g_menu.camX += sxB - g_clientW / 2;
            g_menu.camY += syB - g_clientH / 2;
            g_menu.ClampCamera();

            g_menu.Compose();
            std::vector<uint32_t> pusto = g_menu.canvas;

            auto stawBld = [&]() {
                Menu::Bld b;
                b.owner = uint32_t(g_menu.me);
                // Magazyn, nie glowny budynek: TOBJ 50 ma dach doku, ktory
                // zaslania lodz stojaca w hali - a mierzymy tu sama regule
                // "lodz nad bryla".
                b.tobj = 59;
                b.x = fx * 2; b.y = fy * 2;
                b.hpMax = b.hp = 1000;
                b.span = g_menu.BldFootprint(59);
                g_menu.blds.push_back(b);
            };
            auto stawUnit = [&](int dcx, int dcy) {
                Menu::Unit u;
                u.owner = uint32_t(g_menu.me);
                u.type = 1;
                u.x = float(fx * 2 + dcx); u.y = float(fy * 2 + dcy);
                u.tx = u.x; u.ty = u.y;
                u.dir = Menu::DIR_REST;
                u.spawned = true;
                g_menu.SetUnitStats(u);
                u.z = float(g_menu.terr.topAt(fx, fy));
                g_menu.units.push_back(u);
            };

            g_menu.blds.clear(); g_menu.units.clear();
            stawBld();
            g_menu.Compose();
            std::vector<uint32_t> samBld = g_menu.canvas;

            auto probuj = [&](int dcx, int dcy, int &wsp, int &wyg) {
                g_menu.blds.clear(); g_menu.units.clear();
                stawUnit(dcx, dcy);
                g_menu.Compose();
                std::vector<uint32_t> samUnit = g_menu.canvas;
                g_menu.blds.clear(); g_menu.units.clear();
                stawBld();
                stawUnit(dcx, dcy);
                g_menu.Compose();
                wsp = wyg = 0;
                for (size_t k = 0; k < pusto.size(); ++k) {
                    if (samUnit[k] == pusto[k]) continue;      // lodz tu nie rysuje
                    if (samBld[k] == pusto[k]) continue;       // budynek tu nie rysuje
                    ++wsp;
                    // Wygrana to **zmiana piksela budynku**, nie rownosc
                    // z lodzia rysowana solo: przy nalozeniu cien lodzi pada
                    // na bryle zamiast na teren, obwodka miesza sie z tlem
                    // i piksel nigdy nie wychodzi identyczny.
                    if (g_menu.canvas[k] != samBld[k]) ++wyg;
                }
            };
            probuj(2, 2, wspol, wygral);        // lodz o pasmo BLIZEJ
            probuj(-2, -2, wspolT, wygralT);    // lodz o pasmo DALEJ
            g_menu.camX = c0;
            g_menu.camY = c1;
            g_menu.units.clear();
            g_menu.blds.clear();
        }
        std::printf("czesc wspolna lodz/budynek: przed %d px, lodz wygrala %d; "
                    "za %d px, lodz wygrala %d\n",
                    wspol, wygral, wspolT, wygralT);
        // Rownosci piksel w piksel nie da sie wymagac: cien lodzi pada w tej
        // scenie na bryle zamiast na teren, a obwodka i barwa gracza mieszaja
        // sie z tlem. Liczy sie wyrazna wiekszosc.
        // **Lodz ma wygrac w OBU przypadkach.** Plywa nad dnem, a budynek na
        // nim lezy, wiec pasmo nie moze jej przykryc - to przez to lodz
        // z dalszego pasma znikala za budynkiem z blizszego.
        if (wspol > 0 && wspolT > 0)
            std::printf("  lodz nad bryla: przed %s, za %s\n",
                        wygral * 20 >= wspol * 17 ? "dobrze" : "ZLE",
                        wygralT * 20 >= wspolT * 17 ? "dobrze" : "ZLE");
        std::printf("wynik: za skala %d, wyniesiona %d, na otwartym %d -> %s\n",
                    za, nad, otw,
                    (za > 0 && nad == 0 && otw == 0) ? "dobrze" : "ZLE");
        return 0;
    }

    // --mine <mapa>: caly lancuch wydobycia bez okna. Kopalnia na zlozu,
    // transportowiec, magazyn - i sprawdzenie, ze surowiec dociera do skarbca
    // dopiero po przewiezieniu, a nie od razu.
    // --nature <mapa> [w] [h]
    //
    // Obiekty swiata, ktore nie sa ani lodzia, ani budynkiem, ani zlozem.
    // Rekord `OBJ_*` niesie KLASE obiektu, nie TOBJ, i to ona mowi, co to
    // jest: 140 STSharkC, 230 STVolcanoC, 430 STMineSetC (spis w stmap.h).
    // Remake dlugo widzial z tego tylko piec rodzin ryb, wiec rekiny,
    // wulkany i miny z mapy nie istnialy.
    //
    // Tryb kadruje kamere na pierwszym wulkanie (a gdy go nie ma - na
    // pierwszym rekinie) i zrzuca `nature.raw`, zeby dalo sie to OBEJRZEC,
    // a nie tylko policzyc.
    if (argc > 3 && std::strcmp(argv[2], "--nature") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = argc > 4 ? std::atoi(argv[4]) : 1280;
        g_clientH = argc > 5 ? std::atoi(argv[5]) : 860;
        g_menu.FitCanvas();
        g_menu.ApplyZoom(4);
        g_menu.fogOn = false;
        g_menu.aiOn = false;

        int rek = 0, wul = 0, min_ = 0, ryb = 0;
        for (const maps::Object &o : g_menu.terr.objects) {
            if (o.type == maps::OBJ_SHARK)   ++rek;
            else if (o.type == maps::OBJ_VOLCANO) ++wul;
            else if (o.type == maps::OBJ_MINE)    ++min_;
            else if (maps::isCreature(o.type))    ++ryb;
        }
        std::printf("mapa %d: %s\n", g_menu.skSel,
                    g_menu.skMaps[size_t(g_menu.skSel)].title.c_str());
        std::printf("rekordy: rekiny %d, wulkany %d, miny %d, pozostale stworzenia %d\n",
                    rek, wul, min_, ryb);

        // Paski musza sie wczytac - inaczej obiekt jest w liscie, a na
        // ekranie go nie ma, i licznik tego nie widzi.
        const spr::Strip *sh = g_menu.natSet.forKind(maps::KIND_SHARK);
        const char *vol[3] = { "expl_vol", "expl_vob", "expl_vop" };
        std::printf("paski: shark1 %d klatek", sh ? sh->count() : 0);
        for (int k = 0; k < 3; ++k) {
            const spr::Strip *st = g_menu.landSet.strip(vol[k]);
            std::printf(", %s %d", vol[k], st ? st->count() : 0);
        }
        std::printf("\n");

        // Poziom plywania: rekord podaje go pod +28 i jest BEZWZGLEDNY.
        int zR = 0, zF = 0;
        for (const maps::Object &o : g_menu.terr.objects)
            if (maps::isCreature(o.type) && o.z > 0) ++zR;
        for (const Menu::Fish &q : g_menu.fish)
            if (int(q.z) > 0) ++zF;
        std::printf("poziom: stworzen z z>0 w rekordach %d, w scenie %d "
                    "(stworzen razem %d)\n", zR, zF, int(g_menu.fish.size()));
        {   int rodz[4] = { 0, 0, 0, 0 };
            for (const Menu::Mine &q : g_menu.mines) rodz[q.kind & 3] += 1;
            std::printf("miny w scenie: %d (mine %d, lassn %d, akmine %d,"
                        " beacon %d)\n", int(g_menu.mines.size()),
                        rodz[0], rodz[1], rodz[2], rodz[3]);
            std::printf("paski min:");
            for (int k = 0; k < 4; ++k) {
                const spr::Strip *b = g_menu.unitSet.misc(Menu::MineBody(k));
                char rc[32];
                std::snprintf(rc, sizeof(rc), "%s0", Menu::MineTint(k));
                const spr::Strip *t = g_menu.unitSet.misc(rc);
                std::printf(" %s %d/%d", Menu::MineBody(k),
                            b ? b->count() : 0, t ? t->count() : 0);
            }
            std::printf("  (bryla/barwa)\n");
        }

        // Kamera na wulkan, a jak go nie ma - na rekina.
        float cx = -1, cy = -1;
        if (!g_menu.volcs.empty()) { cx = g_menu.volcs[0].x; cy = g_menu.volcs[0].y; }
        else if (!g_menu.mines.empty()) { cx = g_menu.mines[0].x; cy = g_menu.mines[0].y; }
        else for (const Menu::Fish &q : g_menu.fish)
            if (q.kind == maps::KIND_SHARK) { cx = q.x; cy = q.y; break; }
        if (cx < 0) { std::printf("nic do pokazania\n"); return 0; }

        int sx, sy;
        g_menu.CellToScreen(cx, cy, sx, sy);
        g_menu.camX += sx - SCREEN_W / 2;
        g_menu.camY += sy - SCREEN_H / 2;
        g_menu.ClampCamera();

        // Wulkan w spoczynku i w wybuchu - dwa zrzuty, zeby bylo widac, ze
        // wyrzut naprawde dochodzi.
        for (int k = 0; k < 20; ++k) g_menu.StepVolcanoes(0.05f);
        g_menu.Compose();
        int spok = g_menu.DrawVolcanoes(-1);
        {   // Ile rekinow naprawde weszlo na plotno. Sam wpis w liscie nie
            // wystarczy: bez paska albo z bledna kotwica obiekt jest w
            // scenie, a na ekranie go nie ma.
            int wKadrze = 0, narys = 0, pierwX = -1, pierwY = -1;
            for (size_t k = 0; k < g_menu.fish.size(); ++k) {
                if (g_menu.fish[k].kind != maps::KIND_SHARK) continue;
                int qx, qy;
                g_menu.CellToScreen(g_menu.fish[k].x, g_menu.fish[k].y, qx, qy);
                if (qx < 0 || qx >= SCREEN_W || qy < 0 || qy >= SCREEN_H) continue;
                ++wKadrze;
                int d = g_menu.DrawFish(-1, int(k));
                if (d > 0 && pierwX < 0) { pierwX = qx; pierwY = qy; }
                narys += d;
            }
            std::printf("rekiny: w kadrze %d, narysowanych %d, pierwszy na "
                        "ekranie %d,%d\n", wKadrze, narys, pierwX, pierwY);
        }
        {   // Mina byla dotad NIEWIDZIALNA - liczymy NARYSOWANE, nie wczytane.
            // Cudzej miny nie widac, dopoki jej nie wykryjesz, wiec na
            // zrzucie diagnostycznym odslaniamy je RECZNIE - ale najpierw
            // mowimy, ile widac bez tego.
            int wykryte = 0;
            for (const Menu::Mine &q : g_menu.mines) if (q.seen) ++wykryte;
            std::printf("miny wykryte bez odslaniania: %d z %d\n",
                        wykryte, int(g_menu.mines.size()));
            for (Menu::Mine &q : g_menu.mines) q.seen = true;
            int wKadrze = 0, narys = 0, pierwX = -1, pierwY = -1;
            for (size_t k = 0; k < g_menu.mines.size(); ++k) {
                int qx, qy;
                g_menu.CellToScreen(g_menu.mines[k].x, g_menu.mines[k].y, qx, qy);
                if (qx < 0 || qx >= SCREEN_W || qy < 0 || qy >= SCREEN_H) continue;
                ++wKadrze;
                int d = g_menu.DrawMines(int(k));
                if (d > 0 && pierwX < 0) { pierwX = qx; pierwY = qy; }
                narys += d;
            }
            std::printf("miny: w kadrze %d, narysowanych %d, pierwsza na "
                        "ekranie %d,%d\n", wKadrze, narys, pierwX, pierwY);
        }
        {
            FILE *f = std::fopen("nature.raw", "wb");
            if (f) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), f);
                std::fclose(f);
                std::printf("zrzut nature.raw %dx%d (kadr na %.0f,%.0f)\n",
                            SCREEN_W, SCREEN_H, double(cx), double(cy));
            }
        }
        {   // **Wrak i ruina na czystym dnie.** Na mapie pelnej wodorostow
            // nie da sie ich odroznic okiem, wiec scena jest ustawiona:
            // jedna lodz i jeden budynek w srodku kadru, obu zdejmujemy hp
            // i patrzymy, co zostalo.
            float wx = cx, wy = cy;
            g_menu.units.clear();
            g_menu.blds.clear();
            g_menu.wrecks.clear();
            g_menu.fx.clear();
            Menu::Unit lu;
            lu.owner = uint32_t(g_menu.me);
            lu.type = 1;
            lu.x = wx; lu.y = wy;
            g_menu.SetUnitStats(lu);
            lu.hp = 0;
            g_menu.units.push_back(lu);
            Menu::Bld bu;
            bu.owner = uint32_t(g_menu.me);
            bu.tobj = 59;                       // magazyn, jedna komorka
            bu.x = int(wx) + 4; bu.y = int(wy);
            bu.span = 1;
            bu.hpMax = 1000;
            bu.hp = 0;
            g_menu.blds.push_back(bu);
            g_menu.Reap();
            int kad = 0, rui = 0;
            for (const Menu::Wreck &w : g_menu.wrecks) {
                if (w.rec.compare(0, 5, "rubb_") == 0) ++kad;
                else if (w.rec.compare(0, 5, "ruin_") == 0) ++rui;
            }
            int narys = 0;
            // Scena bez wrakow, potem z wrakami - roznica pikseli mowi, czy
            // cokolwiek naprawde weszlo na plotno.
            std::vector<Menu::Wreck> zapas = g_menu.wrecks;
            g_menu.wrecks.clear();
            g_menu.Compose();
            std::vector<uint32_t> bez = g_menu.canvas;
            {   FILE *fb = std::fopen("nature_nowreck.raw", "wb");
                if (fb) { std::fwrite(bez.data(), 4, bez.size(), fb); std::fclose(fb); }
            }
            g_menu.wrecks = zapas;
            g_menu.Compose();
            int zmian = 0;
            for (size_t k = 0; k < bez.size() && k < g_menu.canvas.size(); ++k)
                if (bez[k] != g_menu.canvas[k]) ++zmian;
            for (size_t k = 0; k < g_menu.wrecks.size(); ++k)
                narys += g_menu.DrawWrecks(int(k));
            std::printf("wrak: po lodzi kadlubow %d, po budynku ruin %d, "
                        "narysowanych %d z %d, pikseli zmienionych %d",
                        kad, rui, narys, int(g_menu.wrecks.size()), zmian);
            if (!g_menu.wrecks.empty())
                std::printf(" (pierwszy pasek %s)",
                            g_menu.wrecks[0].rec.c_str());
            std::printf("\n");
            FILE *fw = std::fopen("nature_wreck.raw", "wb");
            if (fw) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), fw);
                std::fclose(fw);
                std::printf("zrzut nature_wreck.raw %dx%d\n", SCREEN_W, SCREEN_H);
            }
        }
        if (!g_menu.volcs.empty()) {
            int przed = g_menu.volcBlew;
            for (int k = 0; k < 1300 && g_menu.volcBlew == przed; ++k)
                g_menu.StepVolcanoes(1.0f);
            for (int k = 0; k < 20; ++k) g_menu.StepVolcanoes(0.05f);
            g_menu.Compose();
            int wyb = g_menu.DrawVolcanoes(-1);
            FILE *f = std::fopen("nature_blow.raw", "wb");
            if (f) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), f);
                std::fclose(f);
            }
            std::printf("wulkan: czesci w spoczynku %d, przy wybuchu %d "
                        "(ma byc 2 i 3), zrzut nature_blow.raw\n", spok, wyb);
        }
        return 0;
    }

    if (argc > 3 && std::strcmp(argv[2], "--mine") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1400;
        g_clientH = 900;
        g_menu.FitCanvas();
        g_menu.ApplyZoom(4);
        g_menu.units.clear();
        g_menu.blds.clear();
        g_menu.fogOn = false;
        g_menu.aiOn = false;                    // sam lancuch, bez wtracen AI

        // Znajdz zloze i postaw na nim kopalnie.
        maps::Object *dep = nullptr;
        for (maps::Object &o : g_menu.terr.objects)
            if (o.type == maps::OBJ_RESOURCE && o.amount > 0) { dep = &o; break; }
        if (!dep) { std::printf("mapa bez zloz\n"); return 1; }
        std::printf("zloze: surowiec %u, ilosc %u, komorka %d,%d\n",
                    dep->subtype, dep->amount, dep->x, dep->y);

        int mineTobj = dep->subtype == 221 ? 57 : 79;
        {   // punkt z dala od kazdego zloza - mapa ma ich czterdziesci
            int fx = -1, fy = -1;
            for (int yy = 4; yy < g_menu.terr.bh * 2 - 4 && fx < 0; yy += 3)
                for (int xx = 4; xx < g_menu.terr.bw * 2 - 4; xx += 3)
                    if (g_menu.Passable(xx / 2, yy / 2) && !g_menu.DepositUnder(xx, yy)) {
                        fx = xx; fy = yy;
                        break;
                    }
            std::printf("wydobywak %d na zlozu: %s, na pustym (%d,%d): %s\n", mineTobj,
                        g_menu.CanBuildAt(dep->x, dep->y, mineTobj) ? "wolno" : "NIE WOLNO",
                        fx, fy,
                        fx < 0 ? "?" : (g_menu.CanBuildAt(fx, fy, mineTobj) ? "WOLNO" : "nie wolno"));
        }

        auto put = [&](int tobj, int cx, int cy) {
            Menu::Bld b;
            b.owner = 1;
            b.tobj = uint32_t(tobj);
            b.x = cx; b.y = cy;
            b.hpMax = b.hp = 1000;
            g_menu.blds.push_back(b);
            return int(g_menu.blds.size()) - 1;
        };
        int mine = put(mineTobj, dep->x, dep->y);
        g_menu.blds[size_t(mine)].z=dep->z;
        int depot = put(59, dep->x + 10, dep->y + 10);       // magazyn piec blokow dalej

        Menu::Unit u;
        u.owner = 1;
        u.type = 8;                             // TranSub
        u.x = float(dep->x); u.y = float(dep->y);
        u.tx = u.x; u.ty = u.y;
        u.dir = Menu::DIR_REST;
        u.spawned = true;
        u.hauling = true;                       // w grze wydaje to przycisk
        g_menu.SetUnitStats(u);
        g_menu.units.push_back(u);
        // Przewodnik radzi trzymac dwa transportowce, zeby jeden ladowal, gdy
        // drugi rozladowuje - i to wlasnie tu sie sprawdza, bo dwa na jednym
        // wlazie staly wczesniej dokladnie na sobie.
        u.x += 3.0f;
        g_menu.units.push_back(u);
        g_menu.BuildPlayers();
        g_menu.Me().bank = Menu::Bank();
        int kor0 = g_menu.Me().bank.corium, met0 = g_menu.Me().bank.metal;

        std::printf("start: skarbiec kor %d met %d, zloze %u\n",
                    kor0, met0, g_menu.MineRemaining(g_menu.blds[size_t(mine)]));

        int firstStore = -1, firstCargo = -1, firstDrop = -1;
        const uint32_t initialDeposit=dep->amount;
        int ani[4] = { 0, 0, 0, 0 };
        for (int t = 0; t < 1800; ++t) {
            g_menu.StepUnits(DWORD(1000 + t * 100));
            const Menu::Bld &m = g_menu.blds[size_t(mine)];
            const Menu::Unit &h = g_menu.units[0];
            if (firstStore < 0 && dep->amount < initialDeposit) firstStore = t;
            if (firstCargo < 0 && h.cargo.total() > 0) firstCargo = t;
            if (firstDrop < 0 && g_menu.Me().bank.corium > kor0) firstDrop = t;
            if (firstDrop < 0 && g_menu.Me().bank.metal > met0) firstDrop = t;
            if (m.aniState >= 0 && m.aniState < 4) ++ani[m.aniState];
        }
        const Menu::Bld &m = g_menu.blds[size_t(mine)];
        std::printf("po 180 s: zloze %u, ladunek lodzi %d, faza %d\n",
                    g_menu.MineRemaining(m), g_menu.units[0].cargo.total(), g_menu.units[0].haulPhase);
        std::printf("  pierwszy urobek po %.1f s, zaladunek po %.1f s, wsyp po %.1f s\n",
                    firstStore < 0 ? -1.0 : firstStore / 10.0,
                    firstCargo < 0 ? -1.0 : firstCargo / 10.0,
                    firstDrop < 0 ? -1.0 : firstDrop / 10.0);
        std::printf("  skarbiec kor %d -> %d, met %d -> %d, zloze zostalo %u\n",
                    kor0, g_menu.Me().bank.corium, met0, g_menu.Me().bank.metal,
                    dep->amount);
        {   float lo = 1e9f;
            int both = 0;
            for (size_t a = 0; a < g_menu.units.size(); ++a)
                for (size_t b = a + 1; b < g_menu.units.size(); ++b) {
                    float dx = g_menu.units[a].x - g_menu.units[b].x;
                    float dy = g_menu.units[a].y - g_menu.units[b].y;
                    float d = std::sqrt(dx * dx + dy * dy);
                    if (d < lo) lo = d;
                    if (g_menu.units[a].haulPhase == 1
                        && g_menu.units[b].haulPhase == 1) ++both;
                }
            std::printf("  dwa transportowce: najblizej %.2f komorki, "
                        "obie przy wlazie naraz %d razy\n", lo, both);
        }
        std::printf("  stany luku (nic/gora/surowiec/dol): %d %d %d %d\n",
                    ani[0], ani[1], ani[2], ani[3]);
        {   Menu::Bld &mm = g_menu.blds[size_t(mine)];
            int had = mm.aniState;
            mm.aniState = 2;
            const spr::Strip *ws = g_menu.WorkStrip(mm);
            std::printf("  animacja surowca: %s (%d klatek)\n",
                        ws ? "jest" : "BRAK", ws ? int(ws->count()) : 0);
            mm.aniState = 1;
            ws = g_menu.WorkStrip(mm);
            std::printf("  animacja luku: %s (%d klatek)\n",
                        ws ? "jest" : "BRAK", ws ? int(ws->count()) : 0);
            mm.aniState = had;
        }
        (void)depot;
        return 0;
    }

    // --fight <mapa>: dwie wrogie grupy naprzeciw siebie, bez okna. Sprawdza
    // caly lancuch walki - dobor celu, strzal, obrazenia, smierc i sprzatanie.
    if (argc > 3 && std::strcmp(argv[2], "--fight") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1400;
        g_clientH = 900;
        g_menu.ApplyZoom(4);
        g_menu.units.clear();
        g_menu.blds.clear();
        g_menu.sel.clear();
        g_menu.selBld.clear();
        g_menu.fogOn = false;

        // Wolna woda posrodku mapy.
        int bx = g_menu.terr.bw / 2, by = g_menu.terr.bh / 2;
        while (by < g_menu.terr.bh - 2 && !g_menu.Passable(bx, by)) ++by;

        // Po trzy Sentinele z kazdej strony, cztery bloki od siebie.
        auto put = [&](uint32_t owner, int type, float ox, float oy) {
            Menu::Unit u;
            u.owner = owner;
            u.type = uint32_t(type);
            u.x = float(bx * 2) + ox;
            u.y = float(by * 2) + oy;
            u.tx = u.x; u.ty = u.y;
            u.dir = Menu::DIR_REST;
            u.spawned = true;
            g_menu.SetUnitStats(u);
            g_menu.units.push_back(u);
        };
        for (int i = 0; i < 3; ++i) {
            put(1, 1, float(i) * 2.0f - 2.0f, -4.0f);   // WS Sentinel
            put(2, 1, float(i) * 2.0f - 2.0f,  4.0f);
        }
        g_menu.BuildPlayers();

        wep::Gun g = wep::unitGun(1);
        std::printf("bron Sentinela: %d na lodz, %d na budynek, co %d ms, zasieg %d\n",
                    g.dmgUnit, g.dmgBld, g.delayMs, g.range);
        std::printf("start: %d lodzi, hp %d\n", int(g_menu.units.size()),
                    g_menu.units.empty() ? 0 : g_menu.units[0].hp);

        // Zegar na sztywno, 0,1 s na krok.
        int firstHit = -1, firstKill = -1, maxFx = 0, maxShots = 0;
        for (int t = 0; t < 900; ++t) {
            int before = int(g_menu.units.size());
            g_menu.StepCombat(0.1f);
            g_menu.StepShots(0.1f);
            if (int(g_menu.shots.size()) > maxShots) maxShots = int(g_menu.shots.size());
            g_menu.StepFx(0.1f);
            if (firstHit < 0)
                for (const Menu::Unit &u : g_menu.units)
                    if (u.hp < u.hpMax) { firstHit = t; break; }
            if (firstKill < 0 && int(g_menu.units.size()) < before) firstKill = t;
            if (int(g_menu.fx.size()) > maxFx) maxFx = int(g_menu.fx.size());
            if (g_menu.units.size() <= 3) break;
        }
        std::printf("pierwsze trafienie po %.1f s, pierwsza smierc po %.1f s\n",
                    firstHit < 0 ? -1.0 : firstHit / 10.0,
                    firstKill < 0 ? -1.0 : firstKill / 10.0);
        int a = 0, b = 0;
        for (const Menu::Unit &u : g_menu.units) ((u.owner & 7) == 1 ? a : b)++;
        std::printf("koniec: zostalo %d lodzi (gracz1 %d, gracz2 %d), efektow %d, pociskow %d\n",
                    int(g_menu.units.size()), a, b, maxFx, maxShots);
        {   // czym rozni sie uzbrojenie poszczegolnych lodzi
            static const int kShow[] = { 1, 3, 4, 10, 15, 22, 31 };
            std::printf("bron wg jednostki:\n");
            for (int ty : kShow) {
                wep::Gun w = wep::unitGun(ty);
                if (!w.armed()) continue;
                static const char *kSp[4] = { "wolny", "sredni", "szybki", "nieruchomy" };
                std::printf("  %-18s %2d x %4d obr, co %4d ms, zasieg %d, %s, %s\n",
                            w.shot, w.shots, w.dmgUnit, w.delayMs, w.range,
                            kSp[w.pspeed & 3], w.guided ? "naprowadzany" : "prosty");
            }
        }
        {   // ile pociskow leci z jednej salwy
            g_menu.units.clear();
            g_menu.shots.clear();
            Menu::Unit a2;
            a2.owner = 1; a2.type = 4;          // DC Bomber: ordinary attack uses one torpedo
            a2.x = float(bx * 2); a2.y = float(by * 2);
            a2.tx = a2.x; a2.ty = a2.y; a2.spawned = true;
            g_menu.SetUnitStats(a2);
            g_menu.units.push_back(a2);
            Menu::Unit b2 = a2;
            b2.owner = 2; b2.type = 1;
            b2.x += 1.0f;
            g_menu.units.push_back(b2);
            g_menu.BuildPlayers();
            g_menu.StepCombat(0.1f);
            int bomberShots=0;
            for(const auto &shot:g_menu.shots)
                if(shot.owner==1 && shot.shooterType==4) ++bomberShots;
            std::printf("zwykly atak DC Bombera: %d pociskow naraz (oczekiwane 1)\n", bomberShots);
            if(bomberShots!=1) return 1;
            g_menu.units.clear();
            g_menu.shots.clear();
        }
        {   wep::Gun sg = wep::unitGun(3);
            bool dd = false;
            std::printf("pociski: Sentinel %s -> %s, Cruiser %s -> %s\n",
                        g.shot, Menu::ShotSprite(g.shot, dd),
                        sg.shot, Menu::ShotSprite(sg.shot, dd));
        }

        // Wiezyczka kontra lodz: budynek tez musi strzelac.
        g_menu.units.clear();
        g_menu.blds.clear();
        Menu::Bld tw;
        tw.owner = 1;
        tw.tobj = 62;                       // dzialo HF
        tw.x = bx * 2; tw.y = by * 2;
        tw.hpMax = tw.hp = 2000;
        g_menu.blds.push_back(tw);
        put(2, 1, 6.0f, 0.0f);
        wep::Gun tg = wep::bldGun(62, 0);
        std::printf("wiezyczka: %d na lodz, co %d ms, zasieg %d\n",
                    tg.dmgUnit, tg.delayMs, tg.range);
        int hp0 = g_menu.units[0].hp;
        for (int t = 0; t < 200 && !g_menu.units.empty(); ++t) {
            g_menu.StepCombat(0.1f);
            g_menu.StepShots(0.1f);
        }
        std::printf("po ostrzale: lodzi %d (hp startowe %d)\n",
                    int(g_menu.units.size()), hp0);
        return 0;
    }

    if (argc > 3 && std::strcmp(argv[2], "--build") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1400;
        g_clientH = 900;
        g_menu.ApplyZoom(argc > 6 ? std::atoi(argv[6]) : 4);
        if (argc > 7) g_menu.terrMeshed = std::atoi(argv[7]) != 0;
        if (argc > 9) g_menu.decorLag = std::atoi(argv[9]);
        g_menu.CentreCamera();

        // budowniczy wlasnej rasy tam, gdzie jest woda
        int bx = g_menu.terr.bw / 2, by = g_menu.terr.bh / 2;
        while (by < g_menu.terr.bh - 2 && !g_menu.Passable(bx, by)) ++by;
        Menu::Unit u;
        u.owner = uint32_t(g_menu.me);      // nasz, inaczej cala mapa go ostrzeliwuje
        u.type = uint32_t(cost::builderFor(g_menu.PlayerSide()));
        u.x = float(bx * 2);
        u.y = float(by * 2);
        u.tx = u.x; u.ty = u.y;
        u.dir = Menu::DIR_REST;
        u.spawned = true;
        g_menu.SetUnitStats(u);
        u.hpMax = u.hp = 1000000;           // test mierzy budowanie, nie przezywalnosc
        g_menu.units.push_back(u);
        g_menu.sel.assign(1, int(g_menu.units.size()) - 1);

        int n = 0;
        bool isU = false;
        {   // przyciski rozkazow, kazda rasa ma swoj komplet
            static const char *kRace[3] = { "WS", "BO", "SI" };
            int saveCiv = g_menu.Me().civ;
            for (int r = 0; r < 3; ++r) {
                g_menu.Me().civ = r;        // rasa interfejsu idzie za cywilizacja
                g_menu.SyncRace();
                int nu = 0, nb = 0;
                const Menu::Cmd *cu = Menu::UnitCmds(r, cost::builderFor(r), nu);
                const Menu::Cmd *cb = Menu::BldCmds(r, 50, false, nb);
                int okU = 0, okB = 0;
                for (int k = 0; k < nu && cu; ++k) if (g_menu.CmdArt(cu[k].rec)) ++okU;
                for (int k = 0; k < nb && cb; ++k) if (g_menu.CmdArt(cb[k].rec)) ++okB;
                std::printf("przyciski %s: lodz %d/%d, stocznia %d/%d\n",
                            kRace[r], okU, nu, okB, nb);
            }
            g_menu.Me().civ = saveCiv;
            g_menu.SyncRace();
        }
        {   int st2 = 0;
            const uint8_t *tb = g_menu.landSet.remap("PLT_GLOW", 9, st2);
            const std::unordered_map<uint32_t,uint32_t> *gm = g_menu.GlowMap(9);
            std::printf("PLT_GLOW: %s, stopni %d, mapa %d kolorow\n",
                        tb ? "jest" : "BRAK", st2, int(gm->size()));
        }
        {   // paski efektow budowania z DATA\\OTHER
            static const char *kFx[] = { "rippleM", "star_em", "basis_10" };
            std::printf("efekty:");
            for (const char *n : kFx) {
                const spr::Strip *st = g_menu.unitSet.misc(n);
                std::printf(" %s=%d", n, st ? int(st->count()) : 0);
            }
            std::printf("\n");
        }
        g_menu.palOpen = true;
        const int *list = g_menu.Palette(n, isU);
        std::printf("budowniczy %d, paleta %s, %d pozycji\n",
                    int(u.type), isU ? "lodzi" : "budynkow", n);
        if (!list || n == 0) return 1;

        g_menu.Compose();                   // FitCanvas ustawia SCREEN_W/H
        {   // trafienia w okno: gniazda, zamkniecie, strzalki
            RECT r;
            int hitSlot = -99, hitClose = -99;
            if (g_menu.BuildSlot(2, r))
                hitSlot = g_menu.PaletteHit((r.left + r.right) / 2, (r.top + r.bottom) / 2);
            int wx = 0, wy = 0;
            if (g_menu.BuildWinAt(wx, wy)) {
                Menu::WinGeom gg = g_menu.CurGeom();
                hitClose = g_menu.PaletteHit(wx + gg.closeX + 5, wy + gg.botY + 5);
            }
            std::printf("okno: gniazdo 2 -> %d (ma byc 2), zamknij -> %d (ma byc %d)\n",
                        hitSlot, hitClose, Menu::HIT_CLOSE);
        }
        {   int act = 0;
            char civs[32] = { 0 };
            for (int i = 0; i < Menu::MAX_PLAYERS; ++i)
                if (g_menu.players[i].active) {
                    ++act;
                    size_t at = std::strlen(civs);
                    if (at + 4 < sizeof(civs))
                        std::snprintf(civs + at, sizeof(civs) - at, "%d%c ", i,
                                      "WBS"[g_menu.players[i].civ % 3]);
                }
            std::printf("gracze: %d aktywnych (%s), gramy %d, cywilizacja %s\n",
                        act, civs, g_menu.me, g_menu.hudRace);
        }
        {   // mgla: ile blokow widac po odsloniecie przez wlasne jednostki
            g_menu.fogOn = true;
            g_menu.ResetFog();
            g_menu.StepFog(1.0f);
            int live = 0, seen = 0, none = 0;
            for (uint8_t f : g_menu.fog)
                (f == Menu::FOG_LIVE ? live : (f == Menu::FOG_SEEN ? seen : none))++;
            std::printf("jasnosc zapamietanego: %.3f (HALF stopien 20 w PLT_FOG)\n",
                        double(Menu::FOG_SEEN_LIGHT));
            {   // druga tura bez wlasnych: widoczne schodzi do zapamietanego
                int saveMe = g_menu.me;
                g_menu.me = 7;
                g_menu.fogAcc = 999.0f;
                g_menu.StepFog(1.0f);
                g_menu.me = saveMe;
                int seen2 = 0;
                for (uint8_t f2 : g_menu.fog) if (f2 == Menu::FOG_SEEN) ++seen2;
                std::printf("po odejsciu jednostek zapamietanych %d, maska na %d blokow\n",
                            seen2, g_menu.DrawFogOverlay());
                g_menu.fogAcc = 999.0f;
                g_menu.StepFog(1.0f);
            }
            std::printf("mgla: widoczne %d, zapamietane %d, nieznane %d z %d\n",
                        live, seen, none, int(g_menu.fog.size()));
            g_menu.fogOn = false;
        }
        {   // zakladki: ile budynkow w kazdej
            g_menu.palOpen = true;
            std::printf("zakladki:");
            for (int t = 0; t < Menu::PAL_TABS; ++t) {
                g_menu.palTab = t;
                int cn = 0;
                bool cu = false;
                g_menu.Palette(cn, cu);
                std::printf(" %d=%d", t, cn);
            }
            g_menu.palTab = 0;
            std::printf("\n");
        }
        {   // minimapa: trafienie w srodek rombu i przestawienie kamery
            int mcx = 0, mcy = 0;
            bool has = g_menu.MiniMapAt(mcx, mcy);
            bool hit = has && g_menu.MiniHit(mcx, mcy);
            int cx0 = g_menu.camX, cy0 = g_menu.camY;
            if (has) g_menu.MiniGoto(mcx - 30, mcy - 20);
            std::printf("minimapa: romb %s, trafienie %s, kamera %s\n",
                        has ? "jest" : "BRAK", hit ? "tak" : "NIE",
                        (g_menu.camX != cx0 || g_menu.camY != cy0) ? "ruszyla" : "STOI");
            g_menu.camX = cx0; g_menu.camY = cy0;
        }
        {   // panel komend: ile gniazd i czy trafiam w pierwsze
            Menu::BarLay BL;
            int nu = 0, nb = 0;
            g_menu.BarLayout(&BL);
            g_menu.CmdsFor(false, nu);
            g_menu.CmdsFor(true, nb);
            RECT cr;
            bool rr = false;
            int got = -1;
            if (g_menu.CmdSlot(BL, false, 0, cr))
                got = g_menu.CmdHit((cr.left + cr.right) / 2, (cr.top + cr.bottom) / 2, rr);
            std::printf("komendy: lodz %d, budynek %d, gniazdo 0 -> %d (strona %s)\n",
                        nu, nb, got, rr ? "P" : "L");
        }
        int before = int(g_menu.blds.size());
        int metal0 = g_menu.Me().bank.metal, kor0 = g_menu.Me().bank.corium;
        // Bez wtracen komputera: Reap() przesuwa indeksy, gdy cos zginie,
        // i test gubi swojego budowniczego.
        g_menu.aiOn = false;
        g_menu.palOpen = true;                      // jak po nacisnieciu BUDUJ
        g_menu.palTab = 0;                          // zakladka uzytkowych
        g_menu.PaletteClick(0);                     // glowny budynek
        std::printf("po kliknieciu: wybrany tobj %d\n", g_menu.buildPick);

        // Wskazanie miejsca to teraz zlecenie: lodz ma tam doplynac.
        // **Miejsca trzeba SZUKAC, nie zakladac.** Audyt bral sztywno osiem
        // kratek w dol-prawo od budowniczego. Tam na **27 z 58 map** jest
        // skarpa albo cudza bryla, `CanBuildAt` slusznie odmawial i caly
        // `--build` konczyl sie na `NIC NIE STANELO` — czyli wszystkie
        // pozniejsze pomiary tej mapy przepadaly. To ta sama pulapka, co przy
        // teleporcie budynku w `--bldui` i przy `--gra`: **kandydatow jest
        // wielu, nie jeden**, a klik musi jeszcze wypasc w oknie mapy.
        //
        // Drugi warunek: **z dala od obcych**. Swieza budowa ma niska
        // wytrzymalosc, a `aiOn = false` nie wycisza wiezyczek — na mapie 47
        // budynek stawal po 4,2 s i padal po 4,9 s, majac dwa obce budynki
        // w promieniu 12 kratek. Audyt mierzy budowanie, nie przezywalnosc,
        // tak samo jak przy `hp` budowniczego wyzej.
        int sx = 0, sy = 0, celX = -1, celY = -1, odrzuconych = 0, pozaOknem = 0, niewraca = 0;
        int najObcy = -1;
        for (int rr = 1; rr <= 14; ++rr)
            for (int dy = -rr; dy <= rr; ++dy)
                for (int dx = -rr; dx <= rr; ++dx) {
                    int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
                    if ((ax > ay ? ax : ay) != rr) continue;
                    int cx2 = int(u.x) + dx * 2, cy2 = int(u.y) + dy * 2;
                    if (!g_menu.CanBuildAt(cx2, cy2, g_menu.buildPick)) {
                        ++odrzuconych; continue;
                    }
                    int px = 0, py = 0;
                    g_menu.CellToScreen(float(cx2), float(cy2), px, py);
                    RECT vp = g_menu.Viewport();
                    if (px < vp.left || px >= vp.right
                        || py < vp.top || py >= vp.bottom) { ++pozaOknem; continue; }
                    // **Rozkaz idzie KLIKNIECIEM**, a `PlaceBuilding` wyprowadza
                    // komorke z piksela jeszcze raz. Liczy sie wiec ta komorka,
                    // ktora wroci z `ScreenToCell`, a nie ta, o ktora pytamy.
                    float rcx = 0, rcy = 0;
                    g_menu.ScreenToCell(px, py, rcx, rcy);
                    if (int(rcx) != cx2 || int(rcy) != cy2) { ++niewraca; continue; }
                    int blisko = 9999;                 // do najblizszego obcego
                    for (const Menu::Bld &bb : g_menu.blds)
                        if (g_menu.Foe(bb.owner, uint32_t(g_menu.me))) {
                            int d2 = (bb.x > cx2 ? bb.x - cx2 : cx2 - bb.x)
                                   + (bb.y > cy2 ? bb.y - cy2 : cy2 - bb.y);
                            if (d2 < blisko) blisko = d2;
                        }
                    for (const Menu::Unit &uu : g_menu.units)
                        if (uu.hp > 0 && g_menu.Foe(uu.owner, uint32_t(g_menu.me))) {
                            int d2 = int((uu.x > float(cx2) ? uu.x - float(cx2)
                                                            : float(cx2) - uu.x)
                                       + (uu.y > float(cy2) ? uu.y - float(cy2)
                                                            : float(cy2) - uu.y));
                            if (d2 < blisko) blisko = d2;
                        }
                    if (blisko > najObcy) {
                        najObcy = blisko; celX = cx2; celY = cy2; sx = px; sy = py;
                    }
                }
        std::printf("miejsce: %s (%d,%d), odrzuconych %d, poza oknem mapy %d,"
                    " nie wraca w swoja %d, do najblizszego obcego %d kratek\n",
                    celX < 0 ? "NIE MA GDZIE" : "znalezione", celX, celY,
                    odrzuconych, pozaOknem, niewraca, najObcy);
        if (celX < 0) {
            std::printf("NIE MA GDZIE POSTAWIC - test nie ma na czym stanac\n");
            return 1;
        }
        g_menu.PlaceBuilding(sx, sy);
        size_t me = g_menu.units.size() - 1;
        uint32_t meType = g_menu.units[me].type;
        {   const Menu::Unit &bu = g_menu.units[me];
            cost::Price pr = cost::bldPrice(50, g_menu.SideOf(bu.owner));
            std::printf("budowniczy: typ %d wlasciciel %d (gramy %d), "
                        "cena kor %d met %d, kasa wlasciciela kor %d met %d\n",
                        int(bu.type), int(bu.owner), g_menu.me, pr.corium, pr.metal,
                        g_menu.BankOf(bu.owner).corium, g_menu.BankOf(bu.owner).metal);
        }
        std::printf("zlecenie: tobj %d na %d,%d, faza %d, budynkow %d, kasa nietknieta %s\n",
                    g_menu.units[me].bldTobj, g_menu.units[me].bldX, g_menu.units[me].bldY,
                    g_menu.units[me].bldPhase, int(g_menu.blds.size()),
                    (g_menu.Me().bank.metal == metal0 && g_menu.Me().bank.corium == kor0)
                        ? "tak" : "NIE");

        // Zegar na sztywno, 0,1 s na krok - inaczej test czekalby naprawde.
        bool shotA = false, shotB = false;
        int seenPhase[4] = { 0, 0, 0, 0 };
        int maxFx = 0, stoodAt = -1;
        for (DWORD t = 1000; t < 61000; t += 100) {
            g_menu.StepUnits(t);
            // Gdy cos zginie, Reap() sciaga tablice i indeks przestaje wskazywac
            // naszego budowniczego - trzeba go odnalezc po typie i zleceniu.
            if (me >= g_menu.units.size() || g_menu.units[me].type != meType) {
                size_t f = g_menu.units.size();
                for (size_t k = 0; k < g_menu.units.size(); ++k)
                    if (g_menu.units[k].type == meType) { f = k; break; }
                if (f == g_menu.units.size()) { std::printf("    budowniczy zginal\n"); break; }
                me = f;
            }
            int ph = g_menu.units[me].bldTobj >= 0 ? g_menu.units[me].bldPhase : -1;
            if (ph >= 0 && ph < 4) ++seenPhase[ph];
            if (int(g_menu.fx.size()) > maxFx) maxFx = int(g_menu.fx.size());
            if (stoodAt < 0 && int(g_menu.blds.size()) > before)
                stoodAt = int((t - 1000) / 100);
            // Zrzut w chwili rozblysku i drugi juz przy iskrach budowy.
            if (ph == 2 && g_menu.units[me].bldTimer > 0.45f && !g_menu.fx.empty() && !shotA) {
                shotA = true;
                Dump("raise.raw", float(g_menu.units[me].bldX), float(g_menu.units[me].bldY));
            }
            if (stoodAt >= 0 && !g_menu.fx.empty() && !shotB
                && int((t - 1000) / 100) > stoodAt + 30) {
                shotB = true;
                Dump("sparks.raw", float(g_menu.blds.back().x), float(g_menu.blds.back().y));
            }
        }
        std::printf("  fazy (plynie/opada/stawia/wraca): %d %d %d %d, efektow naraz %d\n",
                    seenPhase[0], seenPhase[1], seenPhase[2], seenPhase[3], maxFx);
        std::printf("  zanurzenie po wszystkim %.2f, zlecenie %s\n",
                    g_menu.units[me].dip,
                    g_menu.units[me].bldTobj < 0 ? "zamkniete" : "WISI");
        std::printf("  stanelo po %.1f s: budynkow %d -> %d, kor %d -> %d, met %d -> %d\n",
                    stoodAt < 0 ? -1.0 : stoodAt / 10.0,
                    before, int(g_menu.blds.size()), kor0, g_menu.Me().bank.corium,
                    metal0, g_menu.Me().bank.metal);
        if (int(g_menu.blds.size()) == before) { std::printf("NIC NIE STANELO\n"); return 1; }

        const Menu::Bld &nb = g_menu.blds.back();
        std::printf("  hp %d/%d, zostalo %.0f s\n", nb.hp, nb.hpMax, nb.buildLeft);
        for (int t = 0; t < 200; ++t) g_menu.StepBuilding(1.0f);
        std::printf("  po 200 s: zostalo %.0f s, hp %d/%d\n",
                    g_menu.blds.back().buildLeft, g_menu.blds.back().hp,
                    g_menu.blds.back().hpMax);

        // teraz z pochylni: zaznacz glowny budynek i zamow pierwsza lodz
        g_menu.sel.clear();
        g_menu.selBld.assign(1, int(g_menu.blds.size()) - 1);
        list = g_menu.Palette(n, isU);
        {   // Reguly stawiania: sasiedztwo wolno, nachodzenie nie, wydobywak
            // tylko na srodku zloza, i nigdy na skarpie.
            const Menu::Bld &b0 = g_menu.blds.back();
            int span = g_menu.BldSpan(b0);
            int okObok = 0, okNa = 0;
            for (int d = span; d <= span + 2; ++d)
                if (g_menu.CanBuildAt(b0.x + d, b0.y, 50)) { okObok = d; break; }
            for (int d = 0; d < span; ++d)
                if (g_menu.CanBuildAt(b0.x + d, b0.y, 50)) ++okNa;
            std::printf("  reguly: obok wolno od %d komorek, na sobie wolno %d razy\n",
                        okObok, okNa);

            // Sama regula to za malo - liczy sie sciezka ROZKAZU. `PlaceBuilding`
            // nie wolalo `CanBuildAt` w ogole: duch swiecil na czerwono, a
            // rozkaz i tak szedl, i budynek stawal na skarpie albo na cudzym.
            {
                int budowniczy = -1;
                for (size_t i = 0; i < g_menu.units.size(); ++i)
                    if (cost::sideOfBuilder(int(g_menu.units[i].type)) >= 0 &&
                        (g_menu.units[i].owner & 7) == uint32_t(g_menu.me & 7))
                        { budowniczy = int(i); break; }
                int odmowy = 0, prob = 0;
                if (budowniczy >= 0) {
                    g_menu.sel.clear();
                    g_menu.sel.push_back(budowniczy);
                    // Trzy miejsca, w ktorych stawiac nie wolno: na istniejacym
                    // budynku, na skarpie i poza mapa.
                    int px[3], py[3];
                    px[0] = b0.x; py[0] = b0.y;                  // na budynku
                    px[1] = -1;   py[1] = -1;                    // skarpa
                    for (int by = 1; by < g_menu.terr.bh - 1 && px[1] < 0; ++by)
                        for (int bx = 1; bx < g_menu.terr.bw - 1; ++bx)
                            if (g_menu.Passable(bx, by) && !g_menu.FlatAt(bx, by, 1))
                                { px[1] = bx * 2; py[1] = by * 2; break; }
                    px[2] = g_menu.terr.bw * 2 + 4;              // poza mapa
                    py[2] = g_menu.terr.bh * 2 + 4;
                    for (int k = 0; k < 3; ++k) {
                        if (px[k] < 0) continue;
                        ++prob;
                        size_t ile = g_menu.blds.size();
                        g_menu.buildPick = 50;
                        g_menu.units[size_t(budowniczy)].bldTobj = -1;
                        int sxk, syk;
                        g_menu.CellToScreen(float(px[k]), float(py[k]), sxk, syk);
                        g_menu.PlaceBuilding(sxk, syk);
                        // Rozkaz odmowiony = budowniczy nie dostal zlecenia.
                        if (g_menu.units[size_t(budowniczy)].bldTobj < 0) ++odmowy;
                        g_menu.units[size_t(budowniczy)].bldTobj = -1;
                        g_menu.buildPick = -1;
                        while (g_menu.blds.size() > ile) g_menu.blds.pop_back();
                    }
                    g_menu.sel.clear();
                }
                std::printf("  rozkaz budowy: odmowil %d z %d nielegalnych miejsc\n",
                            odmowy, prob);
            }

            {   // **Cala droga gracza, od palety do rozkazu.** Testowalem
                // dotad wylacznie odmowy, wiec kiedy `BarClick` zaczal
                // polykac klikniecia w palete, audyt tego nie zauwazyl.
                int budowniczy = -1;
                for (size_t q = 0; q < g_menu.units.size(); ++q)
                    if ((g_menu.units[q].owner & 7) == uint32_t(g_menu.me & 7) &&
                        cost::sideOfBuilder(int(g_menu.units[q].type)) >= 0)
                        { budowniczy = int(q); break; }
                bool wybrano = false, zlecono = false;
                if (budowniczy >= 0) {
                    g_menu.sel.clear();
                    g_menu.sel.push_back(budowniczy);
                    g_menu.selBld.clear();
                    g_menu.units[size_t(budowniczy)].bldTobj = -1;
                    g_menu.palOpen = true;
                    g_menu.palTab = 0;
                    g_menu.palPage = 0;
                    g_menu.buildPick = -1;
                    g_menu.Compose();
                    // klikniecie w pierwsze gniazdo palety - ta sama droga,
                    // co w oknie gry
                    RECT r;
                    if (g_menu.BuildSlot(0, r))
                        g_menu.PanelClick((r.left + r.right) / 2,
                                          (r.top + r.bottom) / 2);
                    wybrano = g_menu.buildPick >= 0;
                    if (wybrano) {
                        // legalne miejsce dla tego typu
                        int px = -1, py = -1;
                        for (int by = 2; by < g_menu.terr.bh - 2 && px < 0; ++by)
                            for (int bx = 2; bx < g_menu.terr.bw - 2; ++bx)
                                if (g_menu.CanBuildAt(bx * 2, by * 2,
                                                      g_menu.buildPick)) {
                                    px = bx * 2; py = by * 2; break;
                                }
                        if (px >= 0) {
                            int sx3, sy3;
                            g_menu.CellToScreen(float(px), float(py), sx3, sy3);
                            RECT vp3 = g_menu.Viewport();
                            g_menu.camX += sx3 - (vp3.left + vp3.right) / 2;
                            g_menu.camY += sy3 - (vp3.top + vp3.bottom) / 2;
                            g_menu.ClampCamera();
                            g_menu.CellToScreen(float(px), float(py), sx3, sy3);
                            if (!g_menu.PanelClick(sx3, sy3))
                                g_menu.PlaceBuilding(sx3, sy3);
                            zlecono = g_menu.units[size_t(budowniczy)].bldTobj >= 0;
                        }
                    }
                    g_menu.palOpen = false;
                    g_menu.buildPick = -1;
                    g_menu.units[size_t(budowniczy)].bldTobj = -1;
                    g_menu.sel.clear();
                }
                {   // **Wyszarzanie**: paleta ma pokazac przygaszona ikone
                    // tego, czego nie da sie postawic. Gra ma na to gotowe
                    // `OBJSD_<NN>` i `BOATS_D_<NN>`.
                    int all = 0;
                    const int *lst = cost::bldList(g_menu.PlayerSide(), all);
                    int blok = 0, maDim = 0;
                    for (int i = 0; i < all; ++i) {
                        cost::Price pw = cost::bldPrice(lst[i], g_menu.PlayerSide());
                        if (!g_menu.Afford(pw)
                            || !g_menu.BuildUnlocked(lst[i], false)) ++blok;
                        if (g_menu.PanelIcon(lst[i], false, uint32_t(g_menu.me), true)
                            != g_menu.PanelIcon(lst[i], false, uint32_t(g_menu.me), false))
                            ++maDim;
                    }
                    std::printf("  wyszarzenie: %d z %d pozycji zablokowanych,"
                                " przygaszona ikona dla %d\n", blok, all, maDim);
                }
                {   // **Okno ZACHOWANIE ma zawartosc z gry**: 20106 ZABLOKUJ
                    // POZYCJE, 20104 ODBLOKUJ, 20105 RUCH AGRESYWNY i cztery
                    // progi `BUT_BEHREPAIR0/20/50/80`.
                    int ki = -1;
                    for (size_t k = 0; k < g_menu.units.size(); ++k)
                        if ((g_menu.units[k].owner & 7) == uint32_t(g_menu.me & 7))
                            { ki = int(k); break; }
                    int progi = 0;
                    bool blok = false, agr = false;
                    if (ki >= 0) {
                        g_menu.sel.assign(1, ki);
                        g_menu.winOpen = Menu::WIN_BEHAV;
                        RECT wr;
                        g_menu.WinRect(Menu::WIN_BEHAV, wr);
                        RECT body = g_menu.WinBody(Menu::WIN_BEHAV, wr);
                        int lh = g_menu.font.height() + 3;
                        [[maybe_unused]] auto klik = [&](int row) {
                            g_menu.WinClick(int(body.left) + 6,
                                            int(body.top) + 2 + row * lh);
                        };
                        // **Okno zachowania ma gniazda, nie wiersze.**
                        // Napisy ladowaly wprost na obrazkach, wiec stan idzie
                        // teraz podswietleniem gniazda - i klikac trzeba
                        // w gniazdo, po zmierzonej geometrii.
                        auto klikSlot = [&](int k) {
                            RECT sr;
                            if (!g_menu.BehSlotRect(wr, k, sr)) return;
                            g_menu.WinClick(int(sr.left + sr.right) / 2,
                                            int(sr.top + sr.bottom) / 2);
                        };
                        klikSlot(Menu::BEH_HOLD);
                        blok = g_menu.units[size_t(ki)].holdPos;
                        klikSlot(Menu::BEH_AGGRO);
                        agr = g_menu.units[size_t(ki)].aggro;
                        for (int r4 = 0; r4 < Menu::BEH_REPAIRS; ++r4) {
                            RECT rr;
                            if (!g_menu.BehRepairRect(wr, r4, rr)) break;
                            g_menu.WinClick(int(rr.left + rr.right) / 2,
                                            int(rr.top + rr.bottom) / 2);
                            if (g_menu.units[size_t(ki)].repairAt
                                == Menu::BehRepairPct(r4)) ++progi;
                        }
                        klikSlot(Menu::BEH_HOLD);   // z powrotem
                        g_menu.winOpen = Menu::WIN_NONE;
                        g_menu.sel.clear();
                    }
                    // Grafiki progow sa w archiwum, po jednej na rase.
                    int graf = 0;
                    for (int r4 = 0; r4 < 4; ++r4) {
                        static const int kR[4] = { 0, 20, 50, 80 };
                        char rec[48];
                        std::snprintf(rec, sizeof(rec), "BUT_BEHREPAIR%d_%s0",
                                      kR[r4], g_menu.hudRace);
                        if (g_menu.hud.image(rec) || g_menu.GetTvStrip(rec)) ++graf;
                    }
                    std::printf("  zachowanie: blokada %s, agresja %s,"
                                " progi %d z 4, grafik %d z 4\n",
                                blok ? "tak" : "NIE", agr ? "tak" : "NIE",
                                progi, graf);
                    if (ki >= 0) {
                        // **Mobile Sonar (technologia 10) podwaja zasieg.**
                        int idx10 = tech::indexOf(10, 1);
                        g_menu.Me().techDone.assign(
                            g_menu.Me().techDone.size(), 0);
                        int przed = g_menu.SightOf(g_menu.units[size_t(ki)]);
                        if (idx10 >= 0) g_menu.Me().techDone[size_t(idx10)] = 1;
                        int po = g_menu.SightOf(g_menu.units[size_t(ki)]);
                        g_menu.Me().techDone.assign(
                            g_menu.Me().techDone.size(), 0);
                        std::printf("  Mobile Sonar: zasieg %d -> %d (%s)\n",
                                    przed, po,
                                    po == przed * 2 ? "podwojony" : "BEZ ZMIANY");
                    }
                }
                {   // **Okno zachowania BUDYNKU**: priorytet tlenu 1-6,
                    // ogien dowolny i - u Silikonow - przywracanie.
                    size_t il0 = g_menu.blds.size();
                    Menu::Bld bb;
                    bb.owner = uint32_t(g_menu.me);
                    bb.tobj = 80;                       // ekstraktor tlenu
                    bb.x = 8; bb.y = 8;
                    bb.hp = bb.hpMax = 800;
                    g_menu.blds.push_back(bb);
                    g_menu.sel.clear();
                    g_menu.selBld.assign(1, int(g_menu.blds.size()) - 1);
                    g_menu.winOpen = Menu::WIN_BEHAV;
                    RECT wr2;
                    g_menu.WinRect(Menu::WIN_BEHAV, wr2);
                    int ust = 0, graf = 0;
                    for (int k = 0; k < Menu::BLD_PRIOS; ++k) {
                        RECT pr;
                        if (!g_menu.BldPrioRect(wr2, k, pr)) break;
                        g_menu.WinClick(int(pr.left + pr.right) / 2,
                                        int(pr.top + pr.bottom) / 2);
                        if (g_menu.blds.back().prio == k + 1) ++ust;
                        char rec[48];
                        std::snprintf(rec, sizeof(rec), "BUT_PRIORITY_%s_%02d",
                                      g_menu.hudRace, k + 1);
                        if (g_menu.hud.image(rec)) ++graf;
                    }
                    // Priorytet ma zmieniac tempo przy niedoborze tlenu.
                    g_menu.Me().bank.oxyNeed = 100;
                    g_menu.Me().bank.oxy = 40;
                    g_menu.blds.back().prio = 1;
                    float f1 = g_menu.OxygenFactor(g_menu.blds.back());
                    g_menu.blds.back().prio = 6;
                    float f6 = g_menu.OxygenFactor(g_menu.blds.back());
                    std::printf("  zachowanie budynku: priorytetow %d z %d,"
                                " grafik %d, tempo prio1 %.2f prio6 %.2f\n",
                                ust, Menu::BLD_PRIOS, graf, f1, f6);
                    g_menu.winOpen = Menu::WIN_NONE;
                    g_menu.selBld.clear();
                    while (g_menu.blds.size() > il0) g_menu.blds.pop_back();
                }
                {   // **Rozkaz ruchu ma przerwac atak.**
                    int ki = -1;
                    for (size_t k = 0; k < g_menu.units.size(); ++k)
                        if ((g_menu.units[k].owner & 7) == uint32_t(g_menu.me & 7))
                            { ki = int(k); break; }
                    bool zdjete = false, stop = false;
                    if (ki >= 0) {
                        g_menu.units[size_t(ki)].tgt = 0;
                        g_menu.units[size_t(ki)].tgtBld = true;
                        g_menu.units[size_t(ki)].tgtOrder = true;
                        g_menu.sel.assign(1, ki);
                        g_menu.OrderMove(g_clientW / 2, g_clientH / 2);
                        zdjete = g_menu.units[size_t(ki)].tgt < 0;
                        g_menu.units[size_t(ki)].tgt = 0;
                        g_menu.units[size_t(ki)].tgtOrder = true;
                        int nc2 = 0;
                        const Menu::Cmd *cs2 = g_menu.CmdsFor(false, nc2);
                        // **Czystego `BUT_STOP` w grze nie ma.** Komenda 1
                        // to u Silikonow `BUT_SISTOP`, a w wariancie
                        // domyslnym - czyli u ludzi - `BUT_DEFENCE`.
                        // Audyt szukajacy slowa "STOP" nie klikal wiec
                        // niczego i przez to przechodzil obok usterki.
                        for (int q = 0; q < nc2; ++q)
                            if (std::strstr(cs2[q].rec, "STOP")
                                || std::strstr(cs2[q].rec, "DEFENCE")
                                || std::strstr(cs2[q].rec, "HOLD")) {
                                g_menu.CmdAction(false, q);
                                break;
                            }
                        stop = g_menu.units[size_t(ki)].tgt < 0;
                        g_menu.sel.clear();
                    }
                    std::printf("  przerwanie ataku: rozkaz ruchu %s, przycisk STOP %s\n",
                                zdjete ? "zdejmuje cel" : "NIE",
                                stop ? "zdejmuje cel" : "NIE");
                }
                std::printf("  droga gracza: paleta %s, rozkaz %s\n",
                            wybrano ? "wybrala" : "NIC NIE WYBRALA",
                            zlecono ? "wydany" : "NIE WYDANY");
            }

            {   // ILE miejsc w ogole przyjmuje budynek. Testowalem dotad
                // wylacznie odmowy, wiec regula, ktora odrzuca wszystko,
                // przechodzila audyt bez sladu.
                int wolne = 0, prob = 0;
                for (int by = 2; by < g_menu.terr.bh - 2; ++by)
                    for (int bx = 2; bx < g_menu.terr.bw - 2; ++bx) {
                        ++prob;
                        if (g_menu.CanBuildAt(bx * 2, by * 2, 52)) ++wolne;
                    }
                std::printf("  miejsc pod stocznie: %d z %d blokow\n",
                            wolne, prob);
            }

            {   // Duch pod kursorem: kolor ma niesc BRYLA, nie plyta pod
                // nia. Renderujemy raz bez ducha i raz z nim, i sprawdzamy,
                // czy zmienione piksele leza w ksztalcie budynku, a nie
                // w rombie plyty na ziemi.
                // **Trzeba DWOCH miejsc, nie jednego.** Duch w srodku okna
                // mapy trafia tam, gdzie postawic sie nie da, wiec wiersz
                // pokazywal sam czerwony i para zielony/czerwony przestawala
                // cokolwiek rozrozniac. Szukamy wiec miejsca, ktore
                // `CanBuildAt` PRZYJMUJE, i osobno takiego, ktore odrzuca.
                RECT vp = g_menu.Viewport();
                g_menu.screen = SCR_TERRAIN;   // Compose rysuje swiat tylko tutaj
                // **Duch potrzebuje zaznaczonego budowniczego.** Podtest
                // kapsuly zuzywa swojego (jest jednorazowa), wiec na mapach
                // jednoosobowych nie zostawal zaden i duch nie rysowal sie
                // wcale - obie polowy pary wychodzily zerem, co czytalo sie
                // jak zepsuty duch. Ta sama regula, co przy ikonach palety:
                // test musi miec na czym stanac.
                if (g_menu.SelectedBuilder() < 0) {
                    int bbx = g_menu.terr.bw / 2, bby = g_menu.terr.bh / 2;
                    while (bby < g_menu.terr.bh - 2 && !g_menu.Passable(bbx, bby)) ++bby;
                    Menu::Unit bu2;
                    bu2.owner = uint32_t(g_menu.me);
                    bu2.type = uint32_t(cost::builderFor(g_menu.PlayerSide()));
                    bu2.x = float(bbx * 2);
                    bu2.y = float(bby * 2);
                    bu2.tx = bu2.x; bu2.ty = bu2.y;
                    bu2.dir = Menu::DIR_REST;
                    bu2.spawned = true;
                    g_menu.SetUnitStats(bu2);
                    bu2.hpMax = bu2.hp = 1000000;
                    g_menu.units.push_back(bu2);
                    g_menu.sel.assign(1, int(g_menu.units.size()) - 1);
                }
                int okX = -1, okY = -1, nieX = -1, nieY = -1;
                for (int py = vp.top + 40; py < vp.bottom - 40 &&
                     (okX < 0 || nieX < 0); py += 12)
                    for (int px = vp.left + 40; px < vp.right - 40 &&
                         (okX < 0 || nieX < 0); px += 12) {
                        float cx, cy;
                        g_menu.ScreenToCell(px, py, cx, cy);
                        // Punkt musi WROCIC na ekran: przy kliknieciu poza
                        // romb mapy `ScreenToCell` oddaje komorke brzegowa,
                        // `CanBuildAt` ja przyjmuje, a duch siada gdzie
                        // indziej albo poza kadrem - i pomiar wychodzi zerem,
                        // czyli wyglada jak zepsuty duch.
                        int wx2 = 0, wy2 = 0;
                        g_menu.CellToScreen(cx, cy, wx2, wy2);
                        if (wx2 < vp.left + 20 || wx2 >= vp.right - 20
                            || wy2 < vp.top + 20 || wy2 >= vp.bottom - 20) continue;
                        bool mozna = g_menu.CanBuildAt(int(cx), int(cy), 50);
                        if (mozna && okX < 0)  { okX = px;  okY = py; }
                        if (!mozna && nieX < 0) { nieX = px; nieY = py; }
                    }
                auto duch = [&](int px, int py, int &zmian,
                                int &zielonych, int &czerwonych) {
                    zmian = zielonych = czerwonych = 0;
                    if (px < 0) return;
                    g_menu.buildPick = -1;
                    g_menu.mouse.x = px;
                    g_menu.mouse.y = py;
                    g_menu.Compose();
                    std::vector<uint32_t> bez = g_menu.canvas;
                    g_menu.buildPick = 50;
                    g_menu.Compose();
                    for (size_t q = 0; q < bez.size(); ++q) {
                        if (bez[q] == g_menu.canvas[q]) continue;
                        ++zmian;
                        uint32_t c = g_menu.canvas[q];
                        int r = (c >> 16) & 0xFF, g2 = (c >> 8) & 0xFF,
                            b2 = c & 0xFF;
                        if (g2 > r + 20 && g2 > b2 + 20) ++zielonych;
                        else if (r > g2 + 20 && r > b2 + 20) ++czerwonych;
                    }
                    g_menu.buildPick = -1;
                };
                int zA = 0, gA = 0, cA = 0, zB = 0, gB = 0, cB = 0;
                duch(okX, okY, zA, gA, cA);
                duch(nieX, nieY, zB, gB, cB);
                const char *werdykt =
                    (okX < 0 || nieX < 0 || zA == 0 || zB == 0)
                        ? "  <- NIE MA PARY MIEJSC"
                    : (gA > cA && cB > gB) ? "" : "  <- DUCH NIE ROZROZNIA";
                std::printf("  duch gdzie WOLNO (%d,%d): %d pikseli, zielonych %d,"
                            " czerwonych %d\n", okX, okY, zA, gA, cA);
                std::printf("  duch gdzie NIE WOLNO (%d,%d): %d pikseli, zielonych %d,"
                            " czerwonych %d%s\n", nieX, nieY, zB, gB, cB, werdykt);
            }

            std::printf("  gramy: gracz %d, civ %d, hudRace %s\n",
                        g_menu.me, g_menu.PlayerSide(), g_menu.hudRace);
            {   // Ikony OBJS_ to zblizenia fragmentow budynkow, po jednym
                // na budynek; gniazdo 46 to zaslepka "COMPLETE BUILDING".
                // Dwa budynki jednej rasy na jednym gniezdzie znacza, ze
                // ktorys pokazuje CUDZY obrazek.
                static const char *kR[3] = { "WS", "BO", "SI" };
                for (int side = 0; side < 3; ++side) {
                    int all = 0;
                    const int *src = cost::bldList(side, all);
                    std::map<int,int> uzyte;
                    int kolizji = 0, zaslepek = 0, brak = 0, a = -1, b2 = -1;
                    for (int i = 0; i < all; ++i) {
                        int sl = units::iconSlot(src[i], side);
                        if (sl < 0) { ++brak; continue; }
                        if (sl == 46) ++zaslepek;
                        auto it = uzyte.find(sl);
                        if (it != uzyte.end()) {
                            ++kolizji;
                            if (a < 0) { a = it->second; b2 = src[i]; }
                        } else uzyte[sl] = src[i];
                    }
                    std::printf("  ikony %s: %d numerow -> %d gniazd, kolizji %d, "
                                "zaslepka %d, bez gniazda %d",
                                kR[side], all, int(uzyte.size()), kolizji,
                                zaslepek, brak);
                    if (a >= 0) std::printf(" (TOBJ %d i %d razem)", a, b2);
                    std::printf("\n");
                }
            }

            {   // Panel komend osobno dla kazdej cywilizacji: czy bierze
                // WLASNE tlo, czy podklada rekina White Sharks.
                static const char *kR[3] = { "WS", "BO", "SI" };
                const char *bylo = g_menu.hudRace;
                for (int r = 0; r < 3; ++r) {
                    g_menu.hudRace = kR[r];
                    const panel::Image *c0 = g_menu.CmdPanelArt(0);
                    const panel::Image *c1 = g_menu.CmdPanelArt(1);
                    const panel::Image *c2 = g_menu.CmdPanelArt(2);
                    const spr::Frame *up = g_menu.CmdPanelTop();
                    const panel::Image *rekin = g_menu.hud.image("CP2_CLEAR1");
                    std::printf("  panel %s: cyfra %d, szerokosc %d, "
                                "clear/1row/2row %s/%s/%s, gora %s, rekin %s\n",
                                kR[r], g_menu.RaceDigit(), g_menu.CmdPanelW(),
                                c0 ? "jest" : "brak", c1 ? "jest" : "brak",
                                c2 ? "jest" : "brak",
                                up ? "jest" : "brak",
                                (c0 && c0 == rekin && r != 0) ? "PODLOZONY" : "nie");
                }
                g_menu.hudRace = bylo;
            }

            {   // Bryly SI, cztery warianty przemiany kapsuly i komplet
                // nakladek barwy gracza do kazdego z nich.
                int civD = g_menu.Me().civ;
                g_menu.Me().civ = 2;
                g_menu.SyncRace();
                int all9 = 0;
                const int *src9 = cost::bldList(2, all9);
                int obcych = 0, brak = 0;
                for (int i = 0; i < all9; ++i) {
                    const spr::Strip *got = g_menu.BldStrip(uint32_t(src9[i]), 2);
                    const spr::Strip *wl = g_menu.BldStripFor(uint32_t(src9[i]), 2);
                    if (!got || !got->count()) { ++brak; continue; }
                    if (got != wl) ++obcych;
                }
                int warianty = 0, zBarwa = 0;
                for (int v = 1; v <= 4; ++v) {
                    char rq[32];
                    std::snprintf(rq, sizeof(rq), "_si_emb%d_", v);
                    const spr::Strip *st = g_menu.unitSet.object(rq);
                    if (!st || !st->count()) continue;
                    ++warianty;
                    int kolorow = 0;
                    for (int c = 0; c < 8; ++c) {
                        std::snprintf(rq, sizeof(rq), "_si_emb%d_id%d_", v, c);
                        const spr::Strip *ts = g_menu.unitSet.object(rq);
                        if (ts && ts->count()) ++kolorow;
                    }
                    if (kolorow == 8) ++zBarwa;
                }
                std::printf("  bryly SI: %d numerow, obcych %d, bez grafiki %d; "
                            "przemiana: %d z 4 wariantow, %d z barwa gracza\n",
                            all9, obcych, brak, warianty, zBarwa);

                // Kapsula Silikonow: najpierw OBROT z poziomu w pion, potem
                // ROZKLADANIE w parasol - i parasol zostaje na ostatniej
                // klatce do konca budowy.
                {   // **Kolejnosc w czasie**: kapsula gra przemiane, DOPIERO
                    // potem znika i zaczyna sie budowa. Symulujemy faze 1.
                    g_menu.fx.clear();
                    g_menu.blds.clear();
                    Menu::Unit kp;
                    kp.owner = uint32_t(g_menu.me);
                    kp.type = 25;
                    kp.x = float(b0.x); kp.y = float(b0.y);
                    kp.tx = kp.x; kp.ty = kp.y;
                    kp.spawned = true;
                    g_menu.SetUnitStats(kp);
                    kp.bldTobj = 91;                // dowolny budynek SI
                    kp.bldX = b0.x; kp.bldY = b0.y;
                    kp.bldPhase = 1;
                    kp.bldTimer = 0;
                    g_menu.units.push_back(kp);
                    size_t ki = g_menu.units.size() - 1;
                    float trwa = g_menu.EmbryoSecs(1);
                    int bldPrzed = -1, bldPo = -1;
                    float t = 0;
                    for (int q = 0; q < 400; ++q) {
                        g_menu.StepBuildOrder(g_menu.units[ki], 0.02f);
                        g_menu.StepFx(0.02f);
                        t += 0.02f;
                        if (bldPrzed < 0 && t >= trwa * 0.5f)
                            bldPrzed = int(g_menu.blds.size());
                        if (bldPo < 0 && t >= trwa + 1.2f)
                            bldPo = int(g_menu.blds.size());
                    }
                    bool zyla = bldPrzed == 0;
                    std::printf("  sekwencja: przemiana %.1f s; w polowie "
                                "budynkow %d (%s), po niej %d; kapsula %s\n",
                                trwa, bldPrzed, zyla ? "jeszcze nie ma" : "ZA WCZESNIE",
                                bldPo,
                                g_menu.units[ki].hp <= 0 ? "zuzyta" : "ZOSTALA");
                    g_menu.units.pop_back();
                    g_menu.blds.clear();
                    g_menu.fx.clear();
                }

                // Pasek obrotu trzyma CZTERY animacje po dziewiec klatek -
                // kazdy kat ma dostac swoj wycinek.
                int zakresy = 0;
                for (int d = 0; d < Menu::DIR_COUNT; d += 6) {
                    g_menu.fx.clear();
                    g_menu.EmbryoFx(float(b0.x), float(b0.y), 1, d,
                                    uint32_t(g_menu.me), 999);
                    if (g_menu.fx.size() == 2 &&
                        g_menu.fx[0].to - g_menu.fx[0].from == 8 &&
                        g_menu.fx[0].from == (d * 4 / Menu::DIR_COUNT) * 9)
                        ++zakresy;
                }
                std::printf("  katy kapsuly: %d z 4 dostaje wlasny wycinek (9 klatek)\n", zakresy);
                g_menu.fx.clear();
                g_menu.EmbryoFx(float(b0.x), float(b0.y), 1, 0, uint32_t(g_menu.me), 999);
                int etapow = int(g_menu.fx.size());
                std::string pierwszy = etapow ? g_menu.fx[0].name : "";
                for (int t = 0; t < 40; ++t) g_menu.StepFx(0.05f);
                int poObrocie = int(g_menu.fx.size());
                for (int t = 0; t < 300; ++t) g_menu.StepFx(0.05f);
                int naKoncu = int(g_menu.fx.size());
                std::string zostal = naKoncu ? g_menu.fx[0].name : "";
                float klatka = naKoncu ? g_menu.fx[0].step : -1;
                g_menu.DropFx(999);
                std::printf("  kapsula: %d etapy (%s -> ...), w trakcie %d, "
                            "na koncu %d (%s, klatka %.0f)\n",
                            etapow, pierwszy.c_str(), poObrocie, naKoncu,
                            zostal.c_str(), klatka);

                // Rusztowanie ludzi - `_tlo_emb*`, ktorego SI nie dostaja.
                for (int r = 0; r < 3; ++r) {
                    g_menu.Me().civ = r;
                    g_menu.SyncRace();
                    g_menu.fx.clear();
                    int bylo = g_menu.scaffoldPlayed;
                    g_menu.ScaffoldFx(float(b0.x), float(b0.y), 1,
                                      uint32_t(g_menu.me), 998);
                    int ile = int(g_menu.fx.size());
                    int zBarwa = 0;
                    for (const Menu::Fx &e : g_menu.fx)
                        if (!e.tint.empty()) ++zBarwa;
                    for (int t = 0; t < 200; ++t) g_menu.StepFx(0.05f);
                    int zostalo = int(g_menu.fx.size());
                    g_menu.DropFx(998);
                    static const char *kN[3] = { "WS", "BO", "SI" };
                    std::printf("  rusztowanie %s: %d efektow (%d z barwa), "
                                "po 10 s %d\n",
                                kN[r], ile, zBarwa, zostalo);
                    (void)bylo;
                }
                g_menu.Me().civ = civD;
                g_menu.SyncRace();

                // **Rusztowanie zwija sie przed budynkiem.** Sprawdzian jest
                // taki: w chwili, gdy postep dobija konca, budynek ma byc
                // dalej niegotowy (`buildLeft > 0`), efekt ma isc wstecz,
                // a dopiero po jego dlugosci budynek staje.
                g_menu.fx.clear();
                g_menu.blds.clear();
                g_menu.Me().civ = 0;                    // rusztowanie maja ludzie
                g_menu.SyncRace();
                {
                    Menu::Bld nb;
                    nb.owner = uint32_t(g_menu.me);
                    nb.tobj = 59;                       // magazyn, jedna komorka
                    nb.x = b0.x;
                    nb.y = b0.y;
                    nb.hpMax = 1000;
                    nb.hp = 100;
                    nb.hpBuilt = 100;
                    nb.span = 1;
                    nb.buildLeft = 0.2f;
                    nb.buildTotal = 10.0f;
                    g_menu.blds.push_back(nb);
                }
                Menu::Bld &bz = g_menu.blds[0];
                // **Postep budowy to jej wytrzymalosc**, wiec do konca
                // dochodzi sie punktami, a nie zegarem. Jeden krok 2 s
                // dawal tu `zwijanie: 0.00 s` i `po zwinieciu NIEGOTOWY`,
                // bo budowa byla dopiero w 28% - audyt mierzyl pustke.
                for (int t = 0; t < 400 && bz.closeT <= 0; ++t)
                    g_menu.StepBuilding(0.05f);
                const float trwa = bz.closeT;
                int wstecz = 0;
                for (const Menu::Fx &e : g_menu.fx) if (e.back) ++wstecz;
                const bool wtrakcie = bz.buildLeft > 0;
                for (int t = 0; t < 200 && bz.closeT > 0; ++t)
                    g_menu.StepBuilding(0.05f);
                std::printf("  zwijanie: %.2f s, efektow wstecz %d, "
                            "budynek w trakcie %s, po zwinieciu %s\n",
                            double(trwa), wstecz, wtrakcie ? "niegotowy" : "GOTOWY",
                            bz.buildLeft <= 0 ? "gotowy" : "NIEGOTOWY");
            }

            {   // Cala droga Silikonow: kapsula (typ 25) wybiera z palety,
                // wydaje rozkaz i **znika przy postawieniu**.
                int civB = g_menu.Me().civ;
                g_menu.Me().civ = 2;
                g_menu.SyncRace();
                Menu::Unit kap;
                kap.owner = uint32_t(g_menu.me);
                kap.type = 25;                      // Capsule Prototype
                kap.x = float(b0.x + 6); kap.y = float(b0.y + 6);
                kap.tx = kap.x; kap.ty = kap.y;
                kap.spawned = true;
                g_menu.SetUnitStats(kap);
                g_menu.units.push_back(kap);
                int ki = int(g_menu.units.size()) - 1;
                g_menu.sel.clear();
                g_menu.sel.push_back(ki);
                g_menu.selBld.clear();
                g_menu.palOpen = true;
                g_menu.palTab = 0;
                g_menu.palPage = 0;
                g_menu.buildPick = -1;
                g_menu.Compose();

                int nPal = 0;
                bool isU8 = false;
                const int *lst = g_menu.Palette(nPal, isU8);
                int chub = 0;
                for (int i = 0; i < nPal; ++i)
                    if (Menu::IsChubModule(lst[i])) ++chub;

                RECT rr;
                bool wyb = false, zlec = false, znikla = false;
                if (g_menu.BuildSlot(0, rr))
                    g_menu.PanelClick((rr.left + rr.right) / 2,
                                      (rr.top + rr.bottom) / 2);
                wyb = g_menu.buildPick >= 0;
                if (wyb) {
                    int px8 = -1, py8 = -1;
                    for (int by = 2; by < g_menu.terr.bh - 2 && px8 < 0; ++by)
                        for (int bx = 2; bx < g_menu.terr.bw - 2; ++bx)
                            if (g_menu.CanBuildAt(bx * 2, by * 2, g_menu.buildPick)) {
                                px8 = bx * 2; py8 = by * 2; break;
                            }
                    if (px8 >= 0) {
                        g_menu.units[size_t(ki)].bldTobj = g_menu.buildPick;
                        g_menu.units[size_t(ki)].bldX = px8;
                        g_menu.units[size_t(ki)].bldY = py8;
                        zlec = true;
                        size_t ile8 = g_menu.blds.size();
                        int uzyte = g_menu.capsulesUsed;
                        g_menu.RaiseBuilding(g_menu.units[size_t(ki)]);
                        znikla = g_menu.capsulesUsed > uzyte &&
                                 g_menu.units[size_t(ki)].hp <= 0;
                        while (g_menu.blds.size() > ile8) g_menu.blds.pop_back();
                    }
                }
                // Co konkretnie wyszlo z palety i co wstalo.
                std::printf("  Silikony: paleta %d pozycji (modulow CHub %d), "
                            "wybor %s, rozkaz %s, kapsula %s, embrion %s\n",
                            nPal, chub, wyb ? "tak" : "NIE", zlec ? "tak" : "NIE",
                            znikla ? "zuzyta" : "ZOSTALA",
                            g_menu.embryoPlayed ? "zagral" : "NIE");
                for (int i = 0; i < nPal && i < 8; ++i) {
                    int t9 = lst[i];
                    const char *nm9 = units::buildingName(t9, 2);
                    std::printf("      pozycja %d: TOBJ %d %s (%s)\n",
                                i, t9, nm9 ? nm9 : "-",
                                (t9 >= 83) ? "SI" : "NIE-SI");
                }
                g_menu.units.pop_back();
                g_menu.sel.clear();
                g_menu.palOpen = false;
                g_menu.buildPick = -1;
                g_menu.Me().civ = civB;
                g_menu.SyncRace();
            }

            {   // Czy kazda rasa ma czym placic - paleta odrzuca budynek
                // z cena {0,0,0} komunikatem "brak ceny". Silikony mialy tak
                // WSZYSTKIE 32, bo przewodnik pisze im "Silicon:" zamiast
                // "Metal:" i parser kosztow ich nie widzial.
                static const char *kRp[3] = { "WS", "BO", "SI" };
                for (int r = 0; r < 3; ++r) {
                    int all6 = 0;
                    const int *src6 = cost::bldList(r, all6);
                    int zCena = 0;
                    for (int i = 0; i < all6; ++i) {
                        cost::Price pr6 = cost::bldPrice(src6[i], r);
                        if (pr6.corium || pr6.metal || pr6.secs) ++zCena;
                    }
                    std::printf("  ceny %s: %d z %d budynkow ma cene\n",
                                kRp[r], zCena, all6);
                }
            }

            {   // Cztery stany przycisku maja istniec dla kazdej rasy.
                // `CPanelTy::PaintBut` sklada nazwe jako `<BUT_NAZWA><stan>`
                // (MakeRecordName z cyfra podana przez wolajacego), a stany to
                // 0 zwykly, 1 pod kursorem, 2 wcisniety, 3 niedostepny.
                static const char *kR[3] = { "WS", "BO", "SI" };
                const char *bylo3 = g_menu.hudRace;
                int civBylo = g_menu.Me().civ;
                for (int r = 0; r < 3; ++r) {
                    // Rase trzeba przestawic NAPRAWDE: `UnitCmds` bierze
                    // `PlayerSide()` (to ono wybiera nazwy `BUT_SI*`),
                    // a `CmdArt` sklada sufiks z `hudRace`.
                    g_menu.Me().civ = r;
                    g_menu.SyncRace();
                    // **Lodz musi byc tej rasy, ktorej pytamy o grafike.**
                    // Wczesniej brana byla pierwsza wlasna lodz z mapy - na
                    // `Pierwsza krew` zawsze WS - wiec wiersz SI pytal
                    // o `BUT_RETREPAIR_SI3`, czyli o nazwe, ktorej gra nigdy
                    // nie sklada (komendy 47 nie ma zaden typ 25..36).
                    // Wychodzilo `0 z 4 stanow` przy rozkazie, ktorego ta rasa
                    // w ogole nie ma, a prawdziwy zestaw SI nie byl sprawdzany
                    // ani razu. Teraz idziemy po **wszystkich typach tej rasy**.
                    std::vector<const char *> rekordy;
                    int rozkazow = 0;
                    for (int t = 1; t <= 40; ++t) {
                        if (cost::unitSide(t) != r) continue;
                        int nt = 0;
                        const Menu::Cmd *ct = Menu::UnitCmds(r, t, nt);
                        for (int i = 0; i < nt && ct; ++i) {
                            bool byl = false;
                            for (const char *e : rekordy)
                                if (std::strcmp(e, ct[i].rec) == 0) { byl = true; break; }
                            if (!byl) rekordy.push_back(ct[i].rec);
                        }
                        rozkazow += nt;
                    }
                    int pelnych = 0, brakow = 0;
                    for (const char *rec : rekordy) {
                        int mam = 0;
                        for (int st = 0; st < 4; ++st) {
                            const panel::Image *im = nullptr;
                            const spr::Frame *fr = nullptr;
                            if (g_menu.CmdArt(rec, st, im, fr)) ++mam;
                        }
                        if (mam == 4) ++pelnych;
                        else { ++brakow; std::printf("      %s ma %d z 4 stanow\n",
                                                     rec, mam); }
                    }
                    std::printf("  stany %s: %d roznych przyciskow na %d gniazd, "
                                "pelne cztery stany %d, niepelne %d\n",
                                kR[r], int(rekordy.size()), rozkazow,
                                pelnych, brakow);
                }
                g_menu.Me().civ = civBylo;
                g_menu.SyncRace();
                g_menu.hudRace = bylo3;
            }

            {   // Gniazda: trzy w rzedzie, maja siedziec w panelu i nie
                // nachodzic na siebie.
                Menu::BarLay L3;
                if (g_menu.BarLayout(&L3) && L3.cpL >= 0) {
                    int poza = 0, nachodzi = 0;
                    RECT prev{ 0, 0, 0, 0 };
                    for (int k = 0; k < 6; ++k) {
                        RECT r4;
                        if (!g_menu.CmdSlot(L3, false, k, r4)) break;
                        if (r4.left < L3.cpL || r4.right > L3.cpL + g_menu.CmdPanelW())
                            ++poza;
                        if (k % 3 && r4.left < prev.right) ++nachodzi;
                        prev = r4;
                    }
                    std::printf("  gniazda: %d poza panelem, %d nachodzi\n",
                                poza, nachodzi);
                }
            }


            {   // Ile budynkow rasa ma i ile z nich trafia do zakladek.
                // Lista numerow idzie z tablicy exe (tools/gen_bldlist.py),
                // a kategorie z sekcji przewodnika - numer bez kategorii
                // wypadlby ze wszystkich zakladek i nie dalo by sie go
                // postawic. Kazdy musi miec grafike i zakladke.
                static const char *kRasy[3] = { "WS", "BO", "SI" };
                for (int side = 0; side < 3; ++side) {
                    int all = 0;
                    const int *src = cost::bldList(side, all);
                    int wZakladkach = 0, zGrafika = 0;
                    int naZakladke[4] = { 0, 0, 0, 0 };
                    for (int i = 0; i < all; ++i) {
                        int c = Menu::BldTab(src[i], side);
                        if (c >= 0 && c < 4) { ++wZakladkach; ++naZakladke[c]; }
                        if (g_menu.BldStripFor(uint32_t(src[i]), side)) ++zGrafika;
                    }
                    std::printf("  paleta %s: %d numerow, w zakladkach %d "
                                "(%d/%d/%d/%d), z grafika %d\n",
                                kRasy[side], all, wZakladkach, naZakladke[0],
                                naZakladke[1], naZakladke[2], naZakladke[3],
                                zGrafika);
                }
            }

            {   // Grafika w palecie ma isc za RASA gracza. `BldStrip` bez
                // strony przeszukuje tablice po kolei i zwraca pierwsze
                // trafienie, czyli zawsze White Sharks - a 11 z 33 numerow
                // TOBJ ma inna bryle dla WS i BO.
                int rozne = 0, boWlasne = 0, sprawdzonych = 0;
                for (int tobj = 50; tobj <= 115; ++tobj) {
                    const spr::Strip *ws = g_menu.BldStripFor(uint32_t(tobj), 0);
                    const spr::Strip *bo = g_menu.BldStripFor(uint32_t(tobj), 1);
                    if (!ws || !bo) continue;
                    ++sprawdzonych;
                    if (ws != bo) ++rozne;
                    // to, co dostanie paleta grajac Black Octopi
                    const spr::Strip *got = g_menu.BldStrip(uint32_t(tobj), 1);
                    if (got == bo) ++boWlasne;
                }
                std::printf("  paleta BO: %d numerow ma osobna bryle, "
                            "wlasna dostaje %d z %d\n",
                            rozne, boWlasne, sprawdzonych);
            }

            {   // Zloze musi blokowac WSZYSTKO poza swoim wydobywakiem.
                const maps::Object *dep = nullptr;
                for (const maps::Object &o : g_menu.terr.objects)
                    if (o.type == maps::OBJ_RESOURCE && o.amount &&
                        o.subtype == 221) { dep = &o; break; }
                int zablokowane = 0, wpuszczony = 0;
                if (dep) {
                    // magazyn (59), stocznia (52) i wieza (60) - zadne nie moze
                    static const int kInne[3] = { 59, 52, 60 };
                    for (int q = 0; q < 3; ++q)
                        if (!g_menu.CanBuildAt(dep->x, dep->y, kInne[q])) ++zablokowane;
                    // a wlasny wydobywak korium (57) ma wejsc
                    if (g_menu.CanBuildAt(dep->x, dep->y, 57)) wpuszczony = 1;
                }
                std::printf("  zloze korium: obcych odrzuconych %d z 3, "
                            "wlasny wydobywak %s\n",
                            zablokowane, wpuszczony ? "wchodzi" : "NIE WCHODZI");
            }

            {   // Zloto nie ma zloza, ma odstep. Przewodnik: 13 komorek.
                int wolne = -1, wolne2 = -1;
                for (int by = 3; by < g_menu.terr.bh - 3 && wolne < 0; ++by)
                    for (int bx = 3; bx < g_menu.terr.bw - 3; ++bx)
                        if (g_menu.CanBuildAt(bx * 2, by * 2, 56)) {
                            wolne = bx * 2; wolne2 = by * 2; break;
                        }
                bool bezZloza = wolne >= 0 &&
                                g_menu.DepositAt(wolne, wolne2) == nullptr;
                int blisko = 0, daleko = 0;
                if (wolne >= 0) {
                    Menu::Bld g;
                    g.owner = uint32_t(g_menu.me);
                    g.tobj = 56;
                    g.x = wolne; g.y = wolne2;
                    g.hpMax = g.hp = 500;
                    g.span = 1;
                    size_t ile = g_menu.blds.size();
                    g_menu.blds.push_back(g);
                    blisko = g_menu.CanBuildAt(wolne + 4, wolne2, 56) ? 1 : 0;
                    daleko = g_menu.CanBuildAt(wolne + 30, wolne2, 56) ? 1 : 0;
                    while (g_menu.blds.size() > ile) g_menu.blds.pop_back();
                }
                std::printf("  zloto: stoi bez zloza %s, drugi o 2 komorki %s, "
                            "o 15 komorek %s\n",
                            bezZloza ? "tak" : "NIE",
                            blisko ? "WPUSZCZONY" : "odrzucony",
                            daleko ? "wpuszczony" : "ODRZUCONY");
            }

            {   // Tlen: bez ekstraktora tempo spada, z ekstraktorem wraca.
                uint32_t ja = uint32_t(g_menu.me);
                float bez = g_menu.OxygenFactor(ja);
                size_t ile = g_menu.blds.size();
                Menu::Bld a;
                a.owner = ja;
                a.tobj = 80;                    // Air Extractor / O2 Sublimator
                a.x = b0.x; a.y = b0.y + 8;
                a.hpMax = a.hp = 500;
                a.span = 1;
                g_menu.blds.push_back(a);
                g_menu.StepSupply(1.0f);
                float z = g_menu.OxygenFactor(ja);
                while (g_menu.blds.size() > ile) g_menu.blds.pop_back();
                g_menu.StepSupply(1.0f);
                std::printf("  tlen: potrzeba %d, bez ekstraktora tempo %.2f, "
                            "z ekstraktorem %.2f\n",
                            g_menu.Me().bank.oxyNeed, bez, z);
            }

            // Wydobywak: srodek zloza tak, obrzeze nie, poza zlozem nie.
            const maps::Object *dep = nullptr;
            for (const maps::Object &o : g_menu.terr.objects)
                if (o.type == maps::OBJ_RESOURCE && o.amount > 0) { dep = &o; break; }
            if (dep) {
                int mt = dep->subtype == 221 ? 57 : 79;
                float fx = float(dep->x) + 1, fy = float(dep->y) + 1;
                g_menu.SnapToDeposit(mt, fx, fy);
                std::printf("  wydobywak: srodek %s, brzeg (%+d%+d) dosuniety %s, "
                            "poza zlozem %s\n",
                            g_menu.CanBuildAt(dep->x, dep->y, mt) ? "tak" : "NIE",
                            1, 1,
                            (int(fx) == dep->x && int(fy) == dep->y) ? "tak" : "NIE",
                            g_menu.CanBuildAt(dep->x + 9, dep->y + 9, mt) ? "WOLNO" : "nie");
            }

            // Offset efektow wzgledem poziomu. Kolo i iskry maja siadac
            // dokladnie tam, gdzie budynek - niezaleznie od tego, jak wysoko
            // stoi blok. Roznica musi byc taka sama na kazdym poziomie.
            {   int lo = 1 << 30, hi = -(1 << 30), sample = 0;
                for (int yy = 2; yy < g_menu.terr.bh - 2 && sample < 4096; ++yy)
                    for (int xx = 2; xx < g_menu.terr.bw - 2; ++xx) {
                        if (!g_menu.Passable(xx, yy)) continue;
                        ++sample;
                        int bxs, bys, exs, eys;
                        g_menu.CellToScreen(float(xx * 2), float(yy * 2), bxs, bys);
                        g_menu.FxScreen(float(xx * 2), float(yy * 2), 0.0f, exs, eys);
                        int d = eys - bys;
                        if (d < lo) lo = d;
                        if (d > hi) hi = d;
                    }
                std::printf("  efekty: odchylka od bryly %d..%d px na %d blokach%s\n",
                            lo, hi, sample, lo == hi ? "" : "  <- ZALEZY OD POZIOMU");
            }

            {   // **Powrot przez ekran.** Kazde klikniecie gracza idzie przez
                // `ScreenToCell`, a kod, ktory wskazuje miejsce (duch pod
                // kursorem, AI, `--gra`), liczy punkt przez `CellToScreen`.
                // Jesli te dwie nie sa swoimi odwrotnosciami, klikniecie
                // w srodek komorki laduje w sasiedniej - i widac to dopiero
                // jako "tu nie postawisz" w miejscu, ktore `CanBuildAt` przed
                // chwila przepuscilo.
                //
                // **Sama nierownosc nie jest jednak usterka.** Przy urwisku
                // kotwica dalekiej komorki jest przykryta blizszym blokiem,
                // wiec ten piksel nalezy do kogos innego i `ScreenToCell` ma
                // prawo - wrecz ma obowiazek - zwrocic ta blizsza. Bufor pasm
                // mowi, ktory to przypadek: trzyma pasmo powierzchni
                // narysowanej w tym pikselu.
                g_menu.Compose();                   // bufor pasm musi byc swiezy
                int wraca = 0, zasl = 0, mija = 0, najdalej = 0, bezTerenu = 0;
                RECT vp5 = g_menu.Viewport();
                for (int by = 2; by < g_menu.terr.bh - 2; by += 3)
                    for (int bx = 2; bx < g_menu.terr.bw - 2; bx += 3) {
                        if (!g_menu.Passable(bx, by)) continue;
                        int cx = bx * 2, cy = by * 2;
                        int sx = 0, sy = 0;
                        g_menu.CellToScreen(float(cx), float(cy), sx, sy);
                        if (sx < vp5.left || sx >= vp5.right
                            || sy < vp5.top || sy >= vp5.bottom) continue;
                        ++wraca;
                        float rx = 0, ry = 0;
                        g_menu.ScreenToCell(sx, sy, rx, ry);
                        int dx = int(rx) - cx, dy = int(ry) - cy;
                        int d = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                        if (d == 0) continue;
                        uint16_t v = g_menu.depthBuf.empty() ? 0
                            : g_menu.depthBuf[size_t(sy) * size_t(SCREEN_W)
                                              + size_t(sx)];
                        if (!v) { ++bezTerenu; continue; }
                        // Pasmo narysowane kontra wlasne pasmo komorki.
                        if (int(v >> 3) - 1 != bx + by) { ++zasl; continue; }
                        ++mija;
                        if (d > najdalej) najdalej = d;
                    }
                std::printf("  powrot przez ekran: %d komorek, wraca w swoje "
                            "%d, zaslonietych %d, MIJA %d (najdalej o %d)%s\n",
                            wraca, wraca - zasl - mija - bezTerenu, zasl, mija,
                            najdalej,
                            mija == 0 ? "" : "  <- klikniecie mija komorke");
            }

            // Skarpa: policz, ile komorek mapy odpada przez sam warunek polki.
            int plaskie = 0, skarpy = 0;
            for (int yy = 2; yy < g_menu.terr.bh - 2; ++yy)
                for (int xx = 2; xx < g_menu.terr.bw - 2; ++xx) {
                    if (!g_menu.Passable(xx, yy)) continue;
                    if (g_menu.FlatAt(xx, yy, 1)) ++plaskie; else ++skarpy;
                }
            std::printf("  teren: %d blokow plaskich, %d skarp (%.0f%% odpada)\n",
                        plaskie, skarpy,
                        100.0 * skarpy / (plaskie + skarpy ? plaskie + skarpy : 1));
        }
        {   // **Paleta musi byc otwarta**, inaczej nie ma czego liczyc.
            // Wiersz wypisywal `z CONTROLG 0, zastepczych 0` i wygladal
            // niewinnie - a zero bralo sie stad, ze klatka skladala sie
            // z zamknieta paleta i zaden slot sie nie rysowal. Ta sama
            // pulapka, co przy audycie stoczni: licznik, ktory moze wyjsc
            // zerem dlatego, ze test nie mial na czym dzialac.
            bool bylaPal = g_menu.palOpen;
            int bylTab = g_menu.palTab;
            std::vector<int> bylSel = g_menu.sel;
            // **`DrawPalette` wychodzi od razu, gdy `Palette()` nic nie zwraca**,
            // a nic nie zwraca bez zaznaczonego budowniczego. Samo otwarcie
            // palety nie wystarczy - trzeba dac jej co pokazac, wiec stawiamy
            // wlasnego budowniczego swojej rasy (12 WS, 24 BO, 25 SI).
            static const int kBud[3] = { 12, 24, 25 };
            Menu::Unit bu;
            bu.owner = uint32_t(g_menu.me);
            bu.type = uint32_t(kBud[g_menu.PlayerSide() & 3]);
            bu.x = 10; bu.y = 10;
            g_menu.units.push_back(bu);
            g_menu.sel.assign(1, int(g_menu.units.size()) - 1);
            g_menu.palOpen = true;                  // jak po nacisnieciu BUDUJ
            g_menu.palTab = 0;
            int nlist = 0; bool lu = false;
            const int *plist = g_menu.Palette(nlist, lu);
            g_menu.iconsFromHud = g_menu.iconsFallback = 0;
            g_menu.Compose();
            g_menu.units.pop_back();
            g_menu.sel = bylSel;
            g_menu.palOpen = bylaPal;
            g_menu.palTab = bylTab;
            g_menu.Reap();
            std::printf("  paleta ma %d pozycji, na stronie %d\n",
                        plist ? nlist : 0,
                        g_menu.iconsFromHud + g_menu.iconsFallback);
            std::printf("ikony palety: z CONTROLG %d, zastepczych %d\n",
                        g_menu.iconsFromHud, g_menu.iconsFallback);
        }
        std::printf("stocznia: paleta %s, %d pozycji\n",
                    list ? (isU ? "lodzi" : "budynkow") : "brak", n);
        if (list && isU && n > 0) {
            // **Stocznia musi byc WLASNA i musi istniec.** Wczesniejszy
            // podtest czysci `blds`, a poprzednia wersja tego audytu
            // szukala potem TOBJ 50 i nie znajdowala nic: kolejka
            // wychodzila pusta, wrota nie ruszaly sie ani razu, a wydruk
            // "w kolejce 0" przechodzil jako zwykla liczba. Stawiamy
            // glowny budynek wlasnej rasy sami.
            g_menu.BuildBlds();          // podtesty wyzej czyszcza `blds`
            const int glowny = Menu::BoatProducer(g_menu.PlayerSide(),cost::builderFor(g_menu.PlayerSide()));
            int yard = -1;
            for (size_t k = 0; k < g_menu.blds.size(); ++k)
                if (int(g_menu.blds[k].tobj) == glowny) { yard = int(k); break; }
            if (yard >= 0) g_menu.blds[size_t(yard)].owner = uint32_t(g_menu.me);
            if (yard < 0) {
                // **Mapa nie musi dawac nam glownego budynku** - misje
                // czesto zaczynaja sie bez niego. Stawiamy wiec swoj,
                // z tym samym stanem, jaki nadaje `BuildBlds`, bo bez
                // paska i rozmiaru nie ma czym otworzyc wrot.
                Menu::Bld yb;
                yb.owner = uint32_t(g_menu.me);
                yb.tobj = uint32_t(glowny);
                yb.x = 6; yb.y = 6;
                yb.hpMax = units::buildingHp(glowny);
                if (yb.hpMax <= 0) yb.hpMax = 1000;
                yb.hp = yb.hpMax;
                yb.rnd = 4242u;
                const spr::Strip *ys = g_menu.BldStrip(yb.tobj,
                                                       g_menu.PlayerSide());
                yb.turret = ys && ys->count() > 0 && ys->count() % Menu::DIR_COUNT == 0;
                yb.span = g_menu.BldFootprint(glowny);
                g_menu.blds.push_back(yb);
                yard = int(g_menu.blds.size()) - 1;
            }
            g_menu.selBld.assign(1, yard);
            g_menu.sel.clear();
            g_menu.palOpen = true;       // palete otwiera przycisk BUDUJ
            g_menu.palTab = 0;
            list = g_menu.Palette(n, isU);
            // **Gniazdo 0 nie musi byc do kupienia.** Dwadziescia szesc
            // lodzi siedzi za badaniem, a na czesc moze nie starczyc
            // surowcow - klikanie zawsze zerowki mierzylo wiec odmowe.
            int wolne = -1, zaBadaniem = 0, zaDrogie = 0;
            for (int q = 0; q < n && list; ++q) {
                cost::Price pq = g_menu.PriceOf(list[q], true);
                if (pq.corium == 0 && pq.metal == 0 && pq.secs == 0) continue;
                if (!g_menu.BuildUnlocked(list[q], true)) { ++zaBadaniem; continue; }
                if (!g_menu.Afford(pq)) { ++zaDrogie; continue; }
                if (wolne < 0) wolne = q;
            }
            size_t u0 = g_menu.units.size();
            int met1 = g_menu.Me().bank.metal;
            if (wolne >= 0) g_menu.PaletteClick(wolne);
            std::printf("  zamowione: gniazdo %d z %d (za badaniem %d, "
                        "za drogich %d), w kolejce %d, met %d -> %d\n",
                        wolne, n, zaBadaniem, zaDrogie,
                        int(g_menu.prod.size()), met1, g_menu.Me().bank.metal);
            if (wolne >= 0) { g_menu.PaletteClick(wolne); g_menu.PaletteClick(wolne); }
            std::printf("  kolejka po trzech zamowieniach: %d\n",
                        int(g_menu.prod.size()));
            // Wrota: budynek ma otworzyc dach, wypuscic lodz i zamknac go
            // z powrotem. Liczymy, czy kazda faza w ogole wystapila.
            int seen[4] = { 0, 0, 0, 0 };
            float maxT = 0;
            for (int t = 0; t < 1200; ++t) {
                g_menu.StepBuilding(0.1f);
                if (yard >= 0 && yard < int(g_menu.blds.size())) {
                    const Menu::Bld &yb = g_menu.blds[size_t(yard)];
                    if (yb.dockPhase >= 0 && yb.dockPhase < 4) ++seen[yb.dockPhase];
                    if (yb.dockT > maxT) maxT = yb.dockT;
                    if (yb.dockT > 0.85f && !g_dockShot) {
                        g_dockShot = true;
                        int dx2, dy2;
                        g_menu.CellToScreen(float(yb.x), float(yb.y), dx2, dy2);
                        g_menu.camX += dx2 - SCREEN_W / 2;
                        g_menu.camY += dy2 - SCREEN_H / 2;
                        g_menu.ClampCamera();
                        g_menu.selBld.assign(1, yard);
                        g_menu.Compose();
                        FILE *o = std::fopen("dock.raw", "wb");
                        if (o) { std::fwrite(g_menu.canvas.data(), 4,
                                             g_menu.canvas.size(), o); std::fclose(o); }
                    }
                }
            }
            std::printf("  po 120 s: lodzi %d -> %d, kolejka %d\n",
                        int(u0), int(g_menu.units.size()), int(g_menu.prod.size()));
            std::printf("  wrota (zamkniete/otwiera/wypuszcza/zamyka): %d %d %d %d,"
                        " otwarcie do %.2f\n",
                        seen[0], seen[1], seen[2], seen[3], maxT);
            {   const spr::Strip *cv = yard >= 0
                        ? g_menu.DockCover(g_menu.blds[size_t(yard)]) : nullptr;
                std::printf("  dach doku: %s (%d klatek), hala %d klatek\n",
                            cv ? "jest" : "BRAK", cv ? int(cv->count()) : 0,
                            yard >= 0 && g_menu.BldStrip(g_menu.blds[size_t(yard)].tobj)
                                ? int(g_menu.BldStrip(g_menu.blds[size_t(yard)].tobj)->count())
                                : 0);
            }
        }

        {   // zrzut okna lodzi - stocznia jest zaznaczona, wiec paleta to lodzie
            g_menu.CentreCamera();
            g_menu.Compose();
            FILE *f = std::fopen("build.raw", "wb");
            if (f) {
                std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), f);
                std::fclose(f);
                std::printf("zrzut build.raw %dx%d\n", SCREEN_W, SCREEN_H);
            }
        }

        // wydobycie: zloze i wydobywak obok niego
        int dep = -1;
        for (size_t k = 0; k < g_menu.terr.objects.size(); ++k)
            if (g_menu.terr.objects[k].type == maps::OBJ_RESOURCE) { dep = int(k); break; }
        if (dep >= 0) {
            const maps::Object &o = g_menu.terr.objects[size_t(dep)];
            std::printf("zloze: surowiec %u, ilosc %u\n", o.subtype, o.amount);
            Menu::Bld e;
            e.owner = 1;
            e.tobj = o.subtype == 221 ? 57u : 79u;   // wydobywak korium / metalu
            e.x = o.x;
            e.y = o.y;
            e.hpMax = e.hp = 500;
            g_menu.blds.push_back(e);
            int k0 = g_menu.Me().bank.corium, m0 = g_menu.Me().bank.metal;
            for (int t = 0; t < 10; ++t) g_menu.StepEconomy(1.0f);
            std::printf("  po 10 s wydobycia: kor %d -> %d, met %d -> %d, zostalo %u\n",
                        k0, g_menu.Me().bank.corium, m0, g_menu.Me().bank.metal,
                        g_menu.terr.objects[size_t(dep)].amount);
        }
        return 0;
    }

    // --pedia <mapa> [kategoria] [pozycja] - Titanopedia w oknie pomocy.
    if (argc > 3 && std::strcmp(argv[2], "--pedia") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mp2 = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mp2.begin(), mp2.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1024; g_clientH = 768;
        static const char *kC[7] = { "OBIEKTY NATURY", "KONSTRUKCJE",
            "LODZIE PODWODNE", "TECHNOLOGIE", "BRON", "SUROWCE",
            "OBIEKTY SPECJALNE" };
        std::printf("Titanopedia, rasa %s:\n", Menu::RaceTag(g_menu.PlayerSide()));
        int puste = 0, razem = 0;
        for (int c = 0; c < 7; ++c) {
            std::vector<Menu::PediaRow> l;
            g_menu.PediaList(c, l);
            razem += int(l.size());
            if (l.empty()) ++puste;
            std::printf("  %-18s %3d  %s%s\n", kC[c], int(l.size()),
                        l.empty() ? "" : l[0].name.c_str(),
                        l.size() > 1 ? " ..." : "");
        }
        std::printf("  razem pozycji %d, kategorii pustych %d\n", razem, puste);

        g_menu.winOpen = Menu::WIN_HELP;
        g_menu.helpTab = 1;
        g_menu.helpItem = argc > 4 ? std::atoi(argv[4]) : -1;
        g_menu.helpEntry = argc > 5 ? std::atoi(argv[5]) : -1;
        g_menu.Compose();
        if (FILE *o = std::fopen("pedia.raw", "wb")) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        // **Karta ma byc WIDOCZNA.** Roznica plotna z trescia i bez niej.
        std::vector<uint32_t> peln = g_menu.canvas;
        int zapI = g_menu.helpItem, zapE = g_menu.helpEntry;
        g_menu.helpItem = -1; g_menu.helpEntry = -1;
        g_menu.Compose();
        int zmian = 0;
        for (size_t k = 0; k < peln.size() && k < g_menu.canvas.size(); ++k)
            if (peln[k] != g_menu.canvas[k]) ++zmian;
        g_menu.helpItem = zapI; g_menu.helpEntry = zapE;
        std::printf("  kategoria %d, pozycja %d: pikseli innych niz spis %d, "
                    "pedia.raw %dx%d\n", zapI, zapE, zmian, SCREEN_W, SCREEN_H);
        return 0;
    }

    // --brief <mapa> [stan] - ekran odprawy z mowiaca glowa.
    if (argc > 3 && std::strcmp(argv[2], "--brief") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mb = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mb.begin(), mb.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 800; g_clientH = 600;
        g_menu.OpenBrief();
        int stan = argc > 4 ? std::atoi(argv[4]) : 1;
        g_menu.brfState = stan;
        g_menu.brfT = 0.3f;
        g_menu.Compose();
        if (FILE *o = std::fopen("brief.raw", "wb")) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        const maps::Entry *bm = g_menu.CurMap();
        static const char *sn[4] = { "START", "SPEAK", "FINISH", "SILENT" };
        std::printf("odprawa: %s, rasa %s, stan %s, brief.raw %dx%d\n",
                    bm ? bm->title.c_str() : "?",
                    Menu::RaceTag(g_menu.PlayerSide()), sn[stan & 3],
                    SCREEN_W, SCREEN_H);
        std::printf("  glowa: START %d, SPEAK %d, FINISH %d, SILENT %d klatek; "
                    "tlo %dx%d\n", g_menu.brfHead[0].count(),
                    g_menu.brfHead[1].count(), g_menu.brfHead[2].count(),
                    g_menu.brfHead[3].count(), g_menu.brfBg.w, g_menu.brfBg.h);
        std::printf("  glos z mapy: %s, %.1f s\n",
                    g_menu.sfx.hasMapSpeech() ? "jest" : "brak",
                    double(g_menu.sfx.mapSpeechSecs()));
        // **Glowa ma byc WIDOCZNA.** Roznica klatki z nia i bez niej - sam
        // licznik klatek paska mowi tylko, ze rekord sie otworzyl.
        std::vector<uint32_t> peln = g_menu.canvas;
        spr::Strip zap;
        std::swap(zap, g_menu.brfHead[stan & 3]);
        g_menu.Compose();
        int zmian = 0;
        for (size_t k = 0; k < peln.size() && k < g_menu.canvas.size(); ++k)
            if (peln[k] != g_menu.canvas[k]) ++zmian;
        std::swap(zap, g_menu.brfHead[stan & 3]);
        std::printf("  pikseli od glowy %d\n", zmian);
        return 0;
    }

    // --report <mapa> [wygrana] [zakladka]: ekran po partii.
    if (argc > 3 && std::strcmp(argv[2], "--report") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mr = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mr.begin(), mr.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = 1024; g_clientH = 768;
        bool won = argc > 4 ? std::atoi(argv[4]) != 0 : true;
        int tab = argc > 5 ? std::atoi(argv[5]) : 0;

        // Troche partii, zeby liczniki nie byly zerami. `StepUnits` robi
        // wszystko naraz - ruch, walke, gospodarke i AI - wiec to ta sama
        // maszyneria, co w grze.
        for (int t = 0; t < 1800; ++t) g_menu.StepUnits(DWORD(1000 + t * 100));

        g_menu.winner = won ? (g_menu.me & 7) : ((g_menu.me & 7) + 1) % 8;
        g_menu.OpenReport(won);
        g_menu.rptTab = tab;
        g_menu.Compose();
        if (FILE *o = std::fopen("report.raw", "wb")) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        const Menu::Player &p = g_menu.players[g_menu.me & 7];
        std::printf("ekran wyniku: %s, rasa %s, zakladka %d, report.raw %dx%d\n",
                    won ? "ZWYCIESTWO (REPORT_*_A)" : "PORAZKA (REPORT_*_F)",
                    Menu::RaceTag(g_menu.PlayerSide()), tab,
                    SCREEN_W, SCREEN_H);
        std::printf("  grafika: tlo %dx%d, tabela klatek %d, kreska %d, "
                    "font wys. %d\n", g_menu.rptBg.w, g_menu.rptBg.h,
                    g_menu.rptTable.count(), g_menu.rptInd.count(),
                    g_menu.rptFont.height());
        (void)p;
        for (int q = 0; q < Menu::MAX_PLAYERS; ++q) {
            const Menu::Player &pp = g_menu.players[q];
            if (!pp.active && !pp.stBuiltU && !pp.stLostU) continue;
            std::printf("  gracz %d%s: lodzie %d/%d, budynki %d/%d, "
                        "zloto %d, surowce %d, badania %d, wynik %d\n",
                        q, q == (g_menu.me & 7) ? " (my)" : "",
                        pp.stBuiltU, pp.stLostU, pp.stBuiltB, pp.stLostB,
                        pp.stGold, pp.stRes, pp.stTech,
                        g_menu.ReportScore(pp));
        }
        // **Ekran ma byc WIDOCZNY, nie tylko wczytany.** Liczy sie roznica
        // plotna z tabela i bez niej - licznik klatek paska mowi tylko tyle,
        // ze rekord sie otworzyl.
        std::vector<uint32_t> peln = g_menu.canvas;
        spr::Strip zapas;
        std::swap(zapas, g_menu.rptTable);
        int zapasTab = g_menu.rptLoadedTab;
        g_menu.rptLoadedTab = tab;          // nie doczytuj z powrotem
        g_menu.Compose();
        int zmian = 0;
        for (size_t k = 0; k < peln.size() && k < g_menu.canvas.size(); ++k)
            if (peln[k] != g_menu.canvas[k]) ++zmian;
        std::swap(zapas, g_menu.rptTable);
        g_menu.rptLoadedTab = zapasTab;
        std::printf("  pikseli od tabeli %d\n", zmian);
        return 0;
    }

    // ======================================================== --mesh ======
    //
    // **Kazdy mesh z kazdej mapy, przez prawdziwy loader.** Pojedyncza mapa
    // dotyka tylko czesci zestawu: `--panel 26` widzi 825 uzyc i 209 numerow,
    // a caly zestaw ma ich 220 - wiec „bez grafiki 0" na jednej mapie nie
    // mowi nic o pozostalych jedenastu.
    //
    // Liczy sie przy tym nie samo wczytanie, tylko **co po drodze wypada**:
    // rekord bez wpisu, naglowek obiecujacy wiecej niz rekord ma, trojkat
    // o indeksie poza tablica i wierzcholek z NaN. Loader odrzucal trzy
    // ostatnie **bez sladu** - `mesh()` zwracal wtedy `nullptr` albo bryle
    // z dziura, i nie dalo sie tego odroznic od braku rekordu.
    // Rzad budynkow z obu klas kanwy — czy stoja w linii.
    if (argc > 2 && std::strcmp(argv[2], "--linia") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi3 = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi3.begin(), mi3.end());
        g_menu.skSel = argc > 3 ? std::atoi(argv[3]) : 26;
        g_clientW = 1400;
        g_clientH = 800;
        if (!g_menu.OpenTerrain(g_gameDir)) {
            std::printf("nie udalo sie wczytac terenu\n");
            return 1;
        }
        g_menu.ApplyZoom(192);
        g_menu.BuildBlds();
        // po jednym numerze na klase kanwy
        int maly = -1, duzy = -1, n = 0;
        const int *lst = cost::bldList(0, n);
        for (int i = 0; i < n && (maly < 0 || duzy < 0); ++i) {
            const spr::Strip *st = g_menu.BldStripFor(uint32_t(lst[i]), 0);
            if (!st || st->count() <= 0) continue;
            const spr::Frame *f = st->at(0);
            if (!f || !f->ok()) continue;
            if (f->canvasW == 240 && duzy < 0) duzy = lst[i];
            if (f->canvasW == 180 && maly < 0) maly = lst[i];
        }
        if (maly < 0 || duzy < 0) {
            std::printf("brak pary kanw (maly %d, duzy %d)\n", maly, duzy);
            return 1;
        }
        // plaski pas komorek: szukamy wiersza, gdzie 12 komorek ma ten sam poziom
        int cy = -1, cx0 = -1;
        for (int y = 4; y < g_menu.terr.bh * 2 - 4 && cy < 0; y += 2)
            for (int x = 4; x + 24 < g_menu.terr.bw * 2 - 4; x += 2) {
                int lv = g_menu.terr.topAt(x / 2, y / 2);
                if (lv < 0) continue;
                bool ok = true;
                for (int k = 0; k < 24 && ok; k += 2)
                    if (g_menu.terr.topAt((x + k) / 2, y / 2) != lv) ok = false;
                if (ok) { cy = y; cx0 = x; break; }
            }
        if (cy < 0) { std::printf("nie ma plaskiego pasa\n"); return 1; }
        g_menu.blds.clear();
        for (int k = 0; k < 6; ++k) {
            int tobj = (k & 1) ? duzy : maly;
            g_menu.adminBld = tobj;
            g_menu.adminOwner = 0;
            int px, py;
            g_menu.CellToScreen(float(cx0 + k * 4), float(cy), px, py);
            g_menu.AdminPlaceBld(px, py);
        }
        g_menu.SyncOcc();
        int sxA, syA;
        g_menu.CellToScreen(float(cx0 + 10), float(cy), sxA, syA);
        g_menu.camX += sxA - SCREEN_W / 2;
        g_menu.camY += syA - SCREEN_H / 2;
        g_menu.ClampCamera();
        g_menu.Compose();
        // Znacznik na KAZDEJ zajmowanej komorce: zolty krzyz w jej srodku.
        // Podstawa bryly ma je nakrywac - to jedyne twarde odniesienie dla
        // kotwicy, jakie mam po tej stronie.
        for (const auto &b : g_menu.blds) {
            int sp = g_menu.BldSpan(b);
            for (int dy = 0; dy < sp; ++dy)
                for (int dx = 0; dx < sp; ++dx) {
                    int px, py;
                    g_menu.CellToScreen(float(b.x + dx), float(b.y + dy), px, py);
                    for (int k = -6; k <= 6; ++k) {
                        if (px + k >= 0 && px + k < SCREEN_W &&
                            py >= 0 && py < SCREEN_H)
                            g_menu.canvas[size_t(py) * SCREEN_W + size_t(px + k)]
                                = 0xFFFF00;
                        if (py + k >= 0 && py + k < SCREEN_H &&
                            px >= 0 && px < SCREEN_W)
                            g_menu.canvas[size_t(py + k) * SCREEN_W + size_t(px)]
                                = 0xFFFF00;
                    }
                }
        }
        if (FILE *o = std::fopen("linia.raw", "wb")) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        std::printf("linia: mapa %d, plaski pas y=%d od x=%d, poziom %d;"
                    " maly TOBJ %d, duzy TOBJ %d; budynkow %d;"
                    " linia.raw %dx%d\n",
                    g_menu.skSel, cy, cx0, g_menu.terr.topAt(cx0 / 2, cy / 2),
                    maly, duzy, int(g_menu.blds.size()), SCREEN_W, SCREEN_H);
        return 0;
    }

    if (argc > 2 && std::strcmp(argv[2], "--mesh") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        const int ileMap = int(g_menu.skMaps.size());
        g_clientW = 1024;
        g_clientH = 768;

        // ---- czy druga mapa dostaje SWOJ teren ----
        //
        // Liczniki nizej mowia, ze kazdy blok dostal siatke - nie mowia, czy
        // dostal **swoja**. Tablice podreczne `land::Set` i spis rekordow
        // `ark::Archive` byly kluczowane sama nazwa, bez pamieci o zestawie,
        // wiec druga mapa dostawala kafle i siatki z poprzedniej.
        //
        // **Skazenie ma kierunek**: przy wspolnym numerze wygrywa zestaw
        // wczytany PIERWSZY i nigdy nie zostaje wymieniony. Mapa pierwsza
        // jest wiec zawsze poprawna, a ofiara jest kazda nastepna - i zaden
        // odcisk liczony w tym samym procesie tego nie zlapie, bo nie ma
        // z czym porownac. Dlatego otwieramy **drugi, prywatny** `land::Set`,
        // ktory z definicji jest czysty.
        //
        // (Pierwsza wersja porownywala zlozone KLATKI i dala 527736 roznych
        // pikseli - ale kontrola „ta sama mapa dwa razy" dala dokladnie tyle
        // samo, czyli to byla animacja sceny, nie skazenie.)
        {
            int a = -1, b = -1;
            std::string zA;
            for (int i = 0; i < ileMap && b < 0; ++i) {
                std::string z = g_menu.skMaps[size_t(i)].texture;
                for (char &c : z) c = char(std::tolower((unsigned char)c));
                if (a < 0) { a = i; zA = z; }
                else if (z != zA) b = i;
            }
            if (a < 0 || b < 0) {
                std::printf("zestawy: NIE MA DWOCH MAP O ROZNYM TERENIE\n");
            } else {
                g_menu.skSel = a; g_menu.OpenTerrain(g_gameDir);   // najpierw A
                g_menu.skSel = b; g_menu.OpenTerrain(g_gameDir);   // potem B
                land::Set czysty;
                if (!czysty.open(g_gameDir, g_menu.skMaps[size_t(b)].texture)) {
                    std::printf("zestawy: nie otwieram %s\n",
                                g_menu.skMaps[size_t(b)].texture.c_str());
                } else {
                    std::set<int> teks, siat;
                    for (int yy = 0; yy < g_menu.terr.bh; ++yy)
                        for (int xx = 0; xx < g_menu.terr.bw; ++xx)
                            for (int lv = 0; lv < maps::LEVELS; ++lv) {
                                if (uint16_t t = g_menu.terr.texAt(xx, yy, lv))
                                    teks.insert(int(t));
                                if (uint16_t m = g_menu.terr.meshAt(xx, yy, lv))
                                    siat.insert(int(m));
                            }
                    int zleTeks = 0, zleSiat = 0;
                    for (int t : teks) {
                        const uint8_t *p = g_menu.landSet.tileBlend(uint16_t(t), 0);
                        const uint8_t *q = czysty.tileBlend(uint16_t(t), 0);
                        if (!p || !q) { ++zleTeks; continue; }
                        if (std::memcmp(p, q, 4096) != 0) ++zleTeks;
                    }
                    for (int m : siat) {
                        const land::Mesh *p = g_menu.landSet.mesh(uint16_t(m));
                        const land::Mesh *q = czysty.mesh(uint16_t(m));
                        if (!p || !q) { ++zleSiat; continue; }
                        if (p->verts.size() != q->verts.size()
                            || p->tris.size() != q->tris.size()) { ++zleSiat; continue; }
                        bool inny = false;
                        for (size_t k = 0; k < p->verts.size() && !inny; ++k)
                            if (p->verts[k].x != q->verts[k].x
                                || p->verts[k].y != q->verts[k].y
                                || p->verts[k].z != q->verts[k].z) inny = true;
                        if (inny) ++zleSiat;
                    }
                    std::printf("zestawy: mapa %d (%s) wczytana PO mapie %d (%s):"
                                " kafli %d (zlych %d), siatek %d (zlych %d) %s\n",
                                b, g_menu.skMaps[size_t(b)].texture.c_str(),
                                a, g_menu.skMaps[size_t(a)].texture.c_str(),
                                int(teks.size()), zleTeks,
                                int(siat.size()), zleSiat,
                                (zleTeks == 0 && zleSiat == 0)
                                    ? "-> swoj teren"
                                    : "-> TEREN Z POPRZEDNIEJ MAPY");
                }
            }
        }

        // zestaw -> numer -> ile komorek go uzywa
        std::map<std::string, std::map<int, long>> uzyte;
        int bezTerenu = 0;
        // **Mesh 0 znaczy „brak siatki"** i taki blok spada na plaski romb.
        // Pierwsza wersja tego spisu liczyla tylko `if (mn)`, wiec zer nie
        // widziala w ogole - a to one sa jedynym miejscem, gdzie teren
        // naprawde rysuje sie inaczej niz siatka.
        long zerZTeks = 0, zerBezTeks = 0, zSiatka = 0;
        // **Wczytanie mesha to nie to samo, co narysowanie go.** Blok
        // z tekstura, ktoremu `landSet.mesh()` odmowi, spada na plaski romb
        // i teren robi sie schodkowy - a licznik wczytanych tego nie widzi.
        // Dlatego kazda mapa sklada tez KLATKE i liczymy jedno i drugie.
        long rysSiatka = 0, rysRomb = 0, rysPlaski = 0;
        std::vector<int> mapyZRombem;
        std::map<int, long> czemuRomb;
        for (int i = 0; i < ileMap; ++i) {
            g_menu.skSel = i;
            if (!g_menu.OpenTerrain(g_gameDir)) { ++bezTerenu; continue; }
            g_menu.screen = SCR_TERRAIN;
            g_menu.ApplyZoom(4);
            g_menu.CentreCamera();
            g_menu.meshBlocks = g_menu.diamondBlocks = g_menu.flatBlocks = 0;
            g_menu.Compose();
            rysSiatka += g_menu.meshBlocks;
            rysRomb += g_menu.diamondBlocks;
            rysPlaski += g_menu.flatBlocks;
            if (g_menu.diamondBlocks > 0) mapyZRombem.push_back(i);
            for (uint16_t q : g_menu.diamondWhy) ++czemuRomb[int(q)];
            g_menu.diamondWhy.clear();
            // **Mapy pisza nazwe zestawu roznie** - `land00`, `Land00`
            // i `LAND00` to ten sam plik (Windows nie rozroznia wielkosci
            // liter), wiec bez sprowadzenia do jednej postaci wychodzi
            // dziewiec zestawow zamiast czterech.
            std::string zestaw = g_menu.skMaps[size_t(i)].texture;
            for (char &c : zestaw) c = char(std::tolower((unsigned char)c));
            for (int yy = 0; yy < g_menu.terr.bh; ++yy)
                for (int xx = 0; xx < g_menu.terr.bw; ++xx)
                    for (int lv = 0; lv < maps::LEVELS; ++lv) {
                        uint16_t mn = g_menu.terr.meshAt(xx, yy, lv);
                        bool teks = g_menu.terr.texAt(xx, yy, lv) != 0;
                        if (mn) { ++uzyte[zestaw][int(mn)]; if (teks) ++zSiatka; }
                        else if (teks) ++zerZTeks;
                        else ++zerBezTeks;
                    }
        }
        std::printf("--mesh: map %d (bez terenu %d), zestawow %d\n",
                    ileMap, bezTerenu, int(uzyte.size()));

        std::printf("  komorki: z siatka %ld, mesh 0 z tekstura %ld\n",
                    zSiatka, zerZTeks);
        std::printf("  w klatkach 58 map: siatka %ld blokow, PLASKI ROMB %ld,"
                    " bez tekstury %ld\n",
                    rysSiatka, rysRomb, rysPlaski);
        if (!mapyZRombem.empty()) {
            std::printf("    mapy z rombem:");
            for (size_t k = 0; k < mapyZRombem.size() && k < 12; ++k)
                std::printf(" %d", mapyZRombem[k]);
            std::printf("%s\n", mapyZRombem.size() > 12 ? " ..." : "");
        }
        if (!czemuRomb.empty()) {
            std::printf("    numery, ktorym odmowiono siatki (%d roznych):",
                        int(czemuRomb.size()));
            int k = 0;
            for (const auto &e : czemuRomb) {
                if (k++ >= 8) { std::printf(" ..."); break; }
                std::printf(" %d(x%ld)", e.first, e.second);
            }
            std::printf("\n");
        }
        long sumUzyc = 0;
        int sumNum = 0, sumBrak = 0, sumZly = 0, sumPusty = 0;
        long sumTroj = 0, sumOdrzuc = 0, sumNan = 0;
        int sumPoza = 0;
        for (const auto &z : uzyte) {
            // Kazdy zestaw trzeba otworzyc osobno - numer mesha znaczy co
            // innego w kazdym z czterech archiwow.
            land::Set ls;
            if (!ls.open(g_gameDir, z.first)) {
                std::printf("  %-8s BRAK ARCHIWUM\n", z.first.c_str());
                continue;
            }
            int brak = 0, zly = 0, pusty = 0, ok = 0, poza = 0;
            long troj = 0, odrzuc = 0, nan = 0, uzyc = 0;
            int minN = 1 << 30, maxN = 0;
            float zLo = 0, zHi = 0, xyLo = 0, xyHi = 0;
            std::vector<int> zleNumery, pozaNumery;
            for (const auto &e : z.second) {
                uzyc += e.second;
                if (e.first < minN) minN = e.first;
                if (e.first > maxN) maxN = e.first;
                const land::Mesh *m = ls.meshRaw(uint16_t(e.first));
                if (!m || m->missing) { ++brak; zleNumery.push_back(e.first); continue; }
                if (m->badSize)      { ++zly;  zleNumery.push_back(e.first); continue; }
                if (m->nonFinite)    nan += m->nonFinite;
                odrzuc += m->dropped;
                troj += long(m->tris.size());
                if (m->tris.empty()) { ++pusty; zleNumery.push_back(e.first); continue; }
                ++ok;
                // **Pudelko bloku**: naglowek stland.h mowi, ze x i y biegna
                // 0..20, a z schodzi do -40. Wierzcholek poza tym oznacza,
                // ze rzut z gory - na ktorym stoi cale rysowanie terenu -
                // jest zalozeniem, a nie odczytem.
                if (m->minX < -0.5f || m->maxX > land::MESH_SPAN + 0.5f
                    || m->minY < -0.5f || m->maxY > land::MESH_SPAN + 0.5f
                    || m->minZ < -40.5f || m->maxZ > 0.5f) {
                    ++poza;
                    pozaNumery.push_back(e.first);
                }
                if (m->minZ < zLo) zLo = m->minZ;
                if (m->maxZ > zHi) zHi = m->maxZ;
                if (m->minX < xyLo) xyLo = m->minX;
                if (m->maxX > xyHi) xyHi = m->maxX;
                if (m->minY < xyLo) xyLo = m->minY;
                if (m->maxY > xyHi) xyHi = m->maxY;
            }
            std::printf("  %-8s numerow %3d (%ld uzyc, %d..%d): wczytanych %d,"
                        " BEZ REKORDU %d, ZLY ROZMIAR %d, BEZ TROJKATOW %d\n",
                        z.first.c_str(), int(z.second.size()), uzyc, minN, maxN,
                        ok, brak, zly, pusty);
            std::printf("           trojkatow %ld, ODRZUCONYCH %ld,"
                        " wierzcholkow z NaN %ld; x,y %.1f..%.1f  z %.1f..%.1f,"
                        " poza pudelkiem %d\n",
                        troj, odrzuc, nan, double(xyLo), double(xyHi),
                        double(zLo), double(zHi), poza);
            if (!zleNumery.empty()) {
                std::printf("           niepoprawne numery:");
                for (size_t k = 0; k < zleNumery.size() && k < 12; ++k)
                    std::printf(" %d", zleNumery[k]);
                std::printf("%s\n", zleNumery.size() > 12 ? " ..." : "");
            }
            if (!pozaNumery.empty()) {
                std::printf("           POZA PUDELKIEM:");
                for (size_t k = 0; k < pozaNumery.size() && k < 12; ++k)
                    std::printf(" %d (%ld uzyc)", pozaNumery[k],
                                z.second.at(pozaNumery[k]));
                std::printf("\n");
            }
            sumUzyc += uzyc; sumNum += int(z.second.size());
            sumBrak += brak; sumZly += zly; sumPusty += pusty;
            sumTroj += troj; sumOdrzuc += odrzuc; sumNan += nan; sumPoza += poza;
        }
        std::printf("RAZEM: numerow %d, uzyc %ld, trojkatow %ld;"
                    " bez rekordu %d, zly rozmiar %d, bez trojkatow %d,"
                    " odrzuconych trojkatow %ld, NaN %ld, poza pudelkiem %d\n",
                    sumNum, sumUzyc, sumTroj, sumBrak, sumZly, sumPusty,
                    sumOdrzuc, sumNan, sumPoza);

        return 0;
    }

    // --edytor <mapa> [widok 0..2] [w] [h] - ekran edytora map.
    //
    // Sprawdza trzy rzeczy, ktorych sam zrzut nie pokazuje: ze uklad sie
    // SKLADA (panele nie nachodza i nie wychodza poza okno), ze kazdy widok
    // naprawde COS rysuje (roznica plotna wobec pustego tla), i ze klik
    // trafia tam, gdzie widac (kazda zakladka i kazde narzedzie da sie
    // wybrac mysza).
    // --kafle <mapa> [grupa] - kontaktowka jednej grupy kafli terenu.
    //
    // **Numer kafla to `(grupa << 8) | wariant`** - wysoki bajt wybiera
    // rodzine tekstury, niski jej wariant, a slownik wariantow jest ten sam
    // w kazdej grupie (sprawdzone: grupy 33, 97 i 113 maja identyczne 31
    // numerow). Ten tryb rysuje grupe kafel przy kaflu, zeby dalo sie
    // ZOBACZYC, ktory wariant jest srodkiem, a ktory krawedzia - bo po samym
    // numerze tego nie widac.
    if (argc > 3 && std::strcmp(argv[2], "--kafle") == 0) {
        int mapa = std::atoi(argv[3]);
        int grupa = argc > 4 ? std::atoi(argv[4]) : -1;
        g_clientW = 1400;
        g_clientH = 900;
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        {
            std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
            g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        }
        if (!g_menu.OpenEditor(mapa)) { std::printf("nie otwiera mapy %d\n", mapa); return 1; }

        // Spis grup i wariantow, ktore ten zestaw terenu naprawde ma.
        std::map<int, std::vector<int>> grupy;
        for (int n = 1; n < 0x10000; ++n)
            if (g_menu.landSet.tile(uint16_t(n))) grupy[n >> 8].push_back(n & 255);
        std::printf("--kafle: mapa %d, zestaw %s; grup %d\n", mapa,
                    g_menu.skMaps[size_t(mapa)].texture.c_str(), int(grupy.size()));
        for (const auto &kv : grupy) {
            // Obok tego, co MA zestaw, stoi to, co edytor z tego bierze:
            // ile komorek mapy nalezy do grupy i ktory wariant jest jej
            // podstawa. Sama liczba wariantow nie mowi, ktory z nich
            // paleta pokaze.
            const ed::TexGroupInfo *ug = g_menu.EdTexGroup(kv.first);
            std::printf("  grupa %3d (0x%02x): wariantow %2d",
                        kv.first, kv.first, int(kv.second.size()));
            if (ug)
                std::printf("; na mapie %5d komorek, bazowy %3d (%d uzyc)",
                            ug->uses, ug->base, ug->baseUses);
            std::printf("\n");
        }
        // Czy slownik wariantow jest ten sam w kazdej duzej grupie.
        std::vector<int> wzor;
        int zgodnych = 0, duzych = 0;
        for (const auto &kv : grupy)
            if (kv.second.size() >= 20) {
                ++duzych;
                if (wzor.empty()) wzor = kv.second;
                if (kv.second == wzor) ++zgodnych;
            }
        std::printf("  duzych grup %d, o IDENTYCZNYM slowniku wariantow %d%s\n",
                    duzych, zgodnych,
                    // To NIE jest flaga bledu: 17 z 22 to zmierzony stan
                    // zestawu, a nie usterka odczytu.
                    (duzych && zgodnych == duzych) ? "" : "  (reszta ma wlasny)");

        if (grupa >= 0 && grupy.count(grupa)) {
            const std::vector<int> &ws = grupy[grupa];
            const int S = 64, KOL = 8, PAD2 = 4;
            int wier = (int(ws.size()) + KOL - 1) / KOL;
            int W = KOL * (S + PAD2), H = wier * (S + PAD2 + 12);
            std::vector<uint32_t> obraz(size_t(W) * H, 0xFF141820u);
            for (size_t i = 0; i < ws.size(); ++i) {
                const uint32_t *px = g_menu.landSet.tile(uint16_t((grupa << 8) | ws[i]));
                if (!px) continue;
                int ox = int(i % KOL) * (S + PAD2) + PAD2 / 2;
                int oy = int(i / KOL) * (S + PAD2 + 12) + PAD2 / 2;
                for (int y = 0; y < S; ++y)
                    for (int x = 0; x < S; ++x)
                        obraz[size_t(oy + y) * W + size_t(ox + x)] =
                            px[size_t(y) * land::TILE + size_t(x)];
            }
            FILE *o = std::fopen("kafle.raw", "wb");
            if (o) { std::fwrite(obraz.data(), 4, obraz.size(), o); std::fclose(o); }
            std::printf("  kafle.raw %dx%d, wariantow %d: ", W, H, int(ws.size()));
            for (int v : ws) std::printf("%d ", v);
            std::printf("\n");
        }
        return 0;
    }

    if (argc > 3 && std::strcmp(argv[2], "--edytor") == 0) {
        int mapa = std::atoi(argv[3]);
        int widok = argc > 4 ? std::atoi(argv[4]) : 1;
        g_clientW = argc > 5 ? std::atoi(argv[5]) : 1400;
        g_clientH = argc > 6 ? std::atoi(argv[6]) : 900;
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        if (!g_menu.OpenEditor(mapa)) { std::printf("edytor: nie otwiera mapy %d\n", mapa); return 1; }
        {   // **Zapis: mapa bez zmian ma wyjsc BAJT W BAJT taka sama.**
            // To jest ten sprawdzian, ktory rozstrzyga, czy format jest znany
            // na tyle, zeby zapisywac - a nie deklaracja, ze jest.
            const maps::Entry &m = g_menu.skMaps[size_t(mapa)];
            ark::Archive a;
            std::vector<uint8_t> orig, znow;
            int rozne = -1;
            if (a.open(m.dkx, m.dkd)) orig = a.read("3D_MAP");
            znow = maps::serializeCells(g_menu.terr);
            if (orig.size() == znow.size()) {
                rozne = 0;
                for (size_t i = 0; i < orig.size(); ++i)
                    if (orig[i] != znow[i]) ++rozne;
            }
            std::printf("  teren tam i z powrotem: %d B -> %d B, roznych bajtow %d%s\n",
                        int(orig.size()), int(znow.size()), rozne,
                        (rozne == 0 && !orig.empty()) ? ""
                            : "  <- ZAPIS NIE ODTWARZA REKORDU");

            // Zapis do katalogu tymczasowego - nie do gry. Potem odczyt
            // z powrotem, bo sam brak bledu nie mowi, ze plik da sie otworzyc.
            std::string tdkx = "edytor_test.dkx", tdkd = "edytor_test.DKD";
            std::remove(tdkx.c_str());
            std::remove(tdkd.c_str());
            std::string err = ark::patchCopy(m.dkx, m.dkd, tdkx, tdkd,
                                             "3D_MAP", znow);
            int wroc = -1;
            if (err.empty()) {
                ark::Archive b;
                if (b.open(tdkx, tdkd)) {
                    std::vector<uint8_t> t2 = b.read("3D_MAP");
                    wroc = (t2.size() == znow.size()) ? 0 : -2;
                    if (wroc == 0)
                        for (size_t i = 0; i < t2.size(); ++i)
                            if (t2[i] != znow[i]) ++wroc;
                }
            }
            std::printf("  zapis kopii: %s, odczyt z powrotem %s%s\n",
                        err.empty() ? "ok" : err.c_str(),
                        wroc == 0 ? "zgodny" : "NIE ZGADZA SIE",
                        (err.empty() && wroc == 0) ? "" : "  <- ZAPIS NIE DZIALA");

            // **Nadpisania ma ODMOWIC.** Bez tej pary poprzedni wiersz
            // wygladalby tak samo, gdyby zapis pisal wszedzie i po wszystkim.
            std::string err2 = ark::patchCopy(m.dkx, m.dkd, tdkx, tdkd,
                                              "3D_MAP", znow);
            std::string err3 = ark::patchCopy(m.dkx, m.dkd, m.dkx, m.dkd,
                                              "3D_MAP", znow);
            std::printf("  ochrona: nadpisanie kopii %s, nadpisanie MAPY GRY %s%s\n",
                        err2.empty() ? "PRZESZLO" : "odmowione",
                        err3.empty() ? "PRZESZLO" : "odmowione",
                        (!err2.empty() && !err3.empty()) ? ""
                            : "  <- ZAPIS NADPISUJE CUDZE PLIKI");
            std::remove(tdkx.c_str());
            std::remove(tdkd.c_str());
        }

        {   // **Nowa mapa.** Zapisujemy do katalogu roboczego, nie do gry,
            // i zaraz czytamy z powrotem - bo sam brak bledu nie mowi, ze
            // powstala mapa, ktora da sie otworzyc.
            std::remove("NowaMapa.dkx");
            std::remove("NowaMapa.DKD");
            std::vector<uint8_t> teren;
            const int cw = 32, ch = 32;
            for (int y = 0; y < ch; y += 2)
                for (int x = 0; x < cw; x += 2) {
                    uint8_t r[9] = {};
                    r[1] = uint8_t(x);
                    r[2] = uint8_t(y);
                    ark::wr16(r + 3, 1);
                    ark::wr16(r + 7, 4352);
                    teren.insert(teren.end(), r, r + 9);
                }
            std::vector<uint8_t> desc(6553, 0);
            ark::wr16(&desc[12], cw);
            ark::wr16(&desc[14], ch);
            ark::wr16(&desc[16], 2);
            std::vector<uint8_t> tyt = { 'N','o','w','a',' ','M','a','p','a', 0 };
            std::vector<ark::OutRec> recs;
            recs.push_back(ark::OutRec{ "DESCRIPTOR", 12, 0, 0, desc });
            recs.push_back(ark::OutRec{ "3D_MAP", 12, cw, ch, teren });
            recs.push_back(ark::OutRec{ "TITLE_MISSION", 12, 0, 0, tyt });
            std::string err = ark::writeArchive("NowaMapa.dkx", "NowaMapa.DKD", recs);

            // Odczyt idzie ta sama droga, co przy mapach gry: `maps::scan`
            // po katalogu, potem `loadTerrain`. Gdyby indeks byl zly, mapa
            // po prostu nie pojawilaby sie na liscie.
            std::vector<maps::Entry> lista = maps::scan(".");
            const maps::Entry *nowa = nullptr;
            for (const maps::Entry &e : lista)
                if (e.file == "NowaMapa") nowa = &e;
            maps::Terrain t2;
            bool wczytany = nowa && maps::loadTerrain(*nowa, t2);
            std::printf("  nowa mapa: zapis %s, na liscie %s, teren %s,"
                        " %dx%d komorek, blokow %d%s\n",
                        err.empty() ? "ok" : err.c_str(),
                        nowa ? "jest" : "NIE MA",
                        wczytany ? "wczytany" : "NIE WCZYTANY",
                        nowa ? nowa->w : 0, nowa ? nowa->h : 0,
                        int(t2.cells.size()),
                        (err.empty() && nowa && wczytany
                         && t2.cells.size() == size_t(cw / 2) * (ch / 2))
                            ? "" : "  <- NOWA MAPA NIE POWSTAJE");
            std::string err2 = ark::writeArchive("NowaMapa.dkx", "NowaMapa.DKD", recs);
            std::printf("  ochrona nowej: drugi zapis pod ta sama nazwa %s%s\n",
                        err2.empty() ? "PRZESZEDL" : "odmowiony",
                        err2.empty() ? "  <- NOWA MAPA NADPISUJE" : "");
            std::remove("NowaMapa.dkx");
            std::remove("NowaMapa.DKD");
        }

        {   // **Edycja musi dojsc do PLIKU, nie tylko do sceny.** Podniesienie
            // bloku zmienia `cells`, `cells` sa rekordem, a rekord idzie do
            // archiwum - wiec mierzymy cala te droge, a nie samo `topAt`.
            // **Blok musi dac sie podniesc.** Srodek mapy bywa juz na
            // najwyzszym poziomie i wtedy `EdRaise` slusznie odmawia -
            // a test czytal to jako usterke i ciagnal za soba kaskade
            // kolejnych. Ta sama regula, co przy audycie stoczni: licznik,
            // ktory moze wyjsc zerem, bo test nie mial na czym stanac,
            // nie jest sprawdzianem.
            int bx = -1, by = -1;
            for (int y = 0; y < g_menu.terr.bh && bx < 0; ++y)
                for (int x = 0; x < g_menu.terr.bw && bx < 0; ++x) {
                    int t = g_menu.terr.topAt(x, y);
                    if (t >= 0 && t + 1 < maps::LEVELS) { bx = x; by = y; }
                }
            if (bx < 0) {
                std::printf("  podniesienie: CALA MAPA NA NAJWYZSZYM POZIOMIE"
                            " - test nie ma na czym stanac\n");
                bx = g_menu.terr.bw / 2;
                by = g_menu.terr.bh / 2;
            }
            std::remove("edytor_edit.dkx");
            std::remove("edytor_edit.DKD");
            int przed = g_menu.terr.topAt(bx, by);
            size_t komPrzed = g_menu.terr.cells.size();
            bool podniesiono = g_menu.EdRaise(bx, by);
            int poPodn = g_menu.terr.topAt(bx, by);
            size_t komPo = g_menu.terr.cells.size();
            std::printf("  podniesienie: blok %d,%d poziom %d -> %d,"
                        " komorek %d -> %d%s\n", bx, by, przed, poPodn,
                        int(komPrzed), int(komPo),
                        (podniesiono && poPodn == przed + 1
                         && komPo == komPrzed + 1) ? "" : "  <- NIE PODNOSI");

            std::string tdkx = "edytor_edit.dkx", tdkd = "edytor_edit.DKD";
            std::remove(tdkx.c_str());
            std::remove(tdkd.c_str());
            const maps::Entry &m2 = g_menu.skMaps[size_t(mapa)];
            std::vector<uint8_t> po = maps::serializeCells(g_menu.terr);
            std::string e1 = ark::patchCopy(m2.dkx, m2.dkd, tdkx, tdkd,
                                            "3D_MAP", po);
            // Rozmiar rekordu urosl o dziewiec bajtow, wiec `patchCopy`
            // MUSI odmowic - indeks nie moze zostac ten sam. To nie usterka,
            // to granica tej drogi zapisu, i lepiej, zeby byla widoczna.
            std::printf("  zapis po zmianie rozmiaru: %s%s\n",
                        e1.empty() ? "PRZESZEDL" : e1.c_str(),
                        e1.empty() ? "  <- ZAPIS NIE PILNUJE ROZMIARU" : "");

            // Droga, ktora dziala przy KAZDEJ zmianie: cale archiwum od nowa.
            std::vector<ark::OutRec> recs2;
            {
                ark::Archive src;
                if (src.open(m2.dkx, m2.dkd)) {
                    recs2.push_back(ark::OutRec{ "DESCRIPTOR", 12, 0, 0,
                                                 src.read("DESCRIPTOR") });
                    recs2.push_back(ark::OutRec{ "3D_MAP", 12,
                                                 uint16_t(g_menu.terr.bw * 2),
                                                 uint16_t(g_menu.terr.bh * 2), po });
                    recs2.push_back(ark::OutRec{ "TEXTURE", 12, 0, 0,
                                                 src.read("TEXTURE") });
                    recs2.push_back(ark::OutRec{ "TITLE_MISSION", 12, 0, 0,
                                                 src.read("TITLE_MISSION") });
                }
            }
            std::string e2 = ark::writeArchive(tdkx, tdkd, recs2);
            maps::Terrain t3;
            bool wroc2 = false;
            if (e2.empty()) {
                std::vector<maps::Entry> l2 = maps::scan(".");
                for (const maps::Entry &e : l2)
                    if (e.file == "edytor_edit")
                        wroc2 = maps::loadTerrain(e, t3);
            }
            std::printf("  zapis calego archiwum: %s, odczyt %s,"
                        " komorek %d (bylo %d)%s\n",
                        e2.empty() ? "ok" : e2.c_str(),
                        wroc2 ? "wczytany" : "NIE WCZYTANY",
                        int(t3.cells.size()), int(komPo),
                        (e2.empty() && wroc2 && t3.cells.size() == komPo)
                            ? "" : "  <- ZMIANA NIE DOCHODZI DO PLIKU");
            std::remove(tdkx.c_str());
            std::remove(tdkd.c_str());
            g_menu.EdLower(bx, by);       // scena wraca do stanu wyjsciowego
        }

        {   // **Cala droga zapisu, ta sama, ktora ma gracz pod klawiszem S.**
            // Podnosimy blok, zapisujemy pod wolna nazwa, czytamy z powrotem
            // i sprzatamy po sobie. Liczy sie, czy zapisana mapa ma te sama
            // liczbe komorek CO PO ZMIANIE, a nie czy zapis nie zwrocil bledu.
            int bx = -1, by = -1;
            for (int y = 0; y < g_menu.terr.bh && bx < 0; ++y)
                for (int x = 0; x < g_menu.terr.bw && bx < 0; ++x) {
                    int t = g_menu.terr.topAt(x, y);
                    if (t >= 0 && t + 1 < maps::LEVELS) { bx = x; by = y; }
                }
            if (bx >= 0) g_menu.EdRaise(bx, by);
            size_t oczek = g_menu.terr.cells.size();
            std::string nazwa = g_menu.EdFreeName("AUDYT_EDYTOR");
            std::string err = nazwa.empty() ? "brak wolnej nazwy"
                                            : g_menu.EdSaveAs(nazwa);
            std::string dkx, dkd;
            g_menu.EdTargets(nazwa, dkx, dkd);
            size_t komorek = 0;
            bool wroc = false;
            if (err.empty()) {
                std::vector<maps::Entry> l = maps::scan(g_menu.EdMapDir());
                for (const maps::Entry &e : l)
                    if (e.file == nazwa) {
                        maps::Terrain t;
                        wroc = maps::loadTerrain(e, t);
                        komorek = t.cells.size();
                    }
            }
            std::printf("  zapis pod S: nazwa \"%s\", %s, odczyt %s,"
                        " komorek %d z %d%s\n", nazwa.c_str(),
                        err.empty() ? "ok" : err.c_str(),
                        wroc ? "wczytany" : "nie",
                        int(komorek), int(oczek),
                        (err.empty() && wroc && komorek == oczek)
                            ? "" : "  <- ZAPIS NIE ODDAJE ZMIANY");
            // Sprzatamy TYLKO to, co ten test sam stworzyl.
            if (err.empty()) {
                std::remove(dkx.c_str());
                std::remove(dkd.c_str());
            }
            if (bx >= 0) g_menu.EdLower(bx, by);
        }

        {   // **Kontury maja robic sie SAME.**
            //
            // Miara jest jedna i nie da sie jej obejsc: najwiekszy skok
            // poziomu miedzy sasiadujacymi blokami. Pedzel, ktory tylko
            // podnosi swoja plame, zostawia sciane o kilka poziomow; pedzel
            // z konturem schodzi do otoczenia schodkami, wiec skok nigdy nie
            // przekracza jednego.
            //
            // Sprawdzian jest PARA - bez wylaczonego konturu obok, "skok 1"
            // wygladalby tak samo, gdyby pedzel w ogole nic nie podnosil.
            auto najwiekszySkok = [&]() {
                int m = 0;
                for (int y = 0; y < g_menu.terr.bh; ++y)
                    for (int x = 0; x < g_menu.terr.bw; ++x) {
                        int a = g_menu.terr.topAt(x, y);
                        if (a < 0) continue;
                        int b1 = g_menu.terr.topAt(x + 1, y);
                        int b2 = g_menu.terr.topAt(x, y + 1);
                        if (b1 >= 0 && (a - b1 > m || b1 - a > m))
                            m = a > b1 ? a - b1 : b1 - a;
                        if (b2 >= 0 && (a - b2 > m || b2 - a > m))
                            m = a > b2 ? a - b2 : b2 - a;
                    }
                return m;
            };
            // Plaski kawalek mapy, zeby pomiar mowil o pedzlu, a nie o tym,
            // co juz na mapie stalo.
            std::vector<maps::Cell> kopia = g_menu.terr.cells;
            for (maps::Cell &c : g_menu.terr.cells) c.level = 0;
            {   // zostaw po jednej komorce na blok
                std::vector<maps::Cell> jedna;
                std::vector<bool> byl(size_t(g_menu.terr.bw) * g_menu.terr.bh, false);
                for (const maps::Cell &c : g_menu.terr.cells) {
                    size_t k = size_t(c.y / 2) * g_menu.terr.bw + c.x / 2;
                    if (k < byl.size() && !byl[k]) { byl[k] = true; jedna.push_back(c); }
                }
                g_menu.terr.cells = jedna;
            }
            maps::sortCells(g_menu.terr);
            maps::rebuildDerived(g_menu.terr);
            int plasko = najwiekszySkok();

            int bx2 = g_menu.terr.bw / 2, by2 = g_menu.terr.bh / 2;
            g_menu.edit.brush = 2;
            for (int k = 0; k < 4; ++k) g_menu.EdStroke(bx2, by2, ed::T_RAISE);
            int bezKonturu = najwiekszySkok();

            // ten sam teren jeszcze raz, tym razem z konturem
            for (maps::Cell &c : g_menu.terr.cells) c.level = 0;
            {
                std::vector<maps::Cell> jedna;
                std::vector<bool> byl(size_t(g_menu.terr.bw) * g_menu.terr.bh, false);
                for (const maps::Cell &c : g_menu.terr.cells) {
                    size_t k = size_t(c.y / 2) * g_menu.terr.bw + c.x / 2;
                    if (k < byl.size() && !byl[k]) { byl[k] = true; jedna.push_back(c); }
                }
                g_menu.terr.cells = jedna;
            }
            maps::sortCells(g_menu.terr);
            maps::rebuildDerived(g_menu.terr);
            for (int k = 0; k < 4; ++k) g_menu.EdStroke(bx2, by2, ed::T_RAISE);
            int zKonturem = najwiekszySkok();
            int szczyt = g_menu.terr.topAt(bx2, by2);

            std::printf("  kontury: plasko skok %d; pierwszy przebieg skok %d,"
                        " drugi %d, szczyt %d%s\n",
                        plasko, bezKonturu, zKonturem, szczyt,
                        (bezKonturu <= 1 && zKonturem <= 1 && szczyt >= 4)
                            ? "" : "  <- KONTURY NIE ROBIA SIE SAME");

            // Pedzel ma zmieniac WIECEJ niz jedna kratke, i tym wiecej, im
            // wiekszy. Bez tego "skok 1" wychodzilby tez wtedy, gdyby pedzel
            // malowal po jednej komorce.
            g_menu.edit.brush = 0;
            int m1 = g_menu.EdStroke(2, 2, ed::T_RAISE);
            g_menu.edit.brush = 3;
            int m3 = g_menu.EdStroke(10, 10, ed::T_RAISE);
            std::printf("  pedzel: promien 0 zmienil %d blokow, promien 3 zmienil %d%s\n",
                        m1, m3, (m1 >= 1 && m3 > m1 * 4) ? ""
                                                         : "  <- PEDZEL NIE ROSNIE");
            g_menu.terr.cells = kopia;
            maps::rebuildDerived(g_menu.terr);
            g_menu.edit.brush = 1;
        }

        g_menu.edit.view = widok;
        g_menu.FitCanvas();
        ed::Layout L = g_menu.EdLayout();
        std::printf("edytor: mapa %d \"%s\", okno %dx%d, widok %s\n",
                    mapa, g_menu.terrName.c_str(), g_clientW, g_clientH,
                    ed::viewName(widok));

        {   // Uklad: panele maja pokryc okno bez dziur i bez nachodzenia.
            // Gdyby ktorys zachodzil na widok, mapa chowalaby sie pod panelem
            // i nikt by tego nie zauwazyl - klik i tak trafialby w panel.
            const RECT *r[5] = { &L.top, &L.rail, &L.view, &L.insp, &L.status };
            const char *nm[5] = { "gorny", "listwa", "widok", "panel", "stan" };
            long pole = 0;
            int nachodzi = 0, poza = 0;
            for (int i = 0; i < 5; ++i) {
                long w = r[i]->right - r[i]->left, h = r[i]->bottom - r[i]->top;
                if (w <= 0 || h <= 0) continue;
                pole += w * h;
                if (r[i]->left < 0 || r[i]->top < 0
                    || r[i]->right > g_clientW || r[i]->bottom > g_clientH) ++poza;
                for (int j = i + 1; j < 5; ++j) {
                    long w2 = r[j]->right - r[j]->left, h2 = r[j]->bottom - r[j]->top;
                    if (w2 <= 0 || h2 <= 0) continue;
                    if (r[i]->left < r[j]->right && r[j]->left < r[i]->right
                        && r[i]->top < r[j]->bottom && r[j]->top < r[i]->bottom)
                        ++nachodzi;
                }
            }
            long okno = long(g_clientW) * g_clientH;
            std::printf("  uklad: pola %ld z %ld (%s), nachodzi %d, poza oknem %d%s\n",
                        pole, okno, pole == okno ? "pokrywa okno" : "ZOSTAJE DZIURA",
                        nachodzi, poza,
                        (pole == okno && !nachodzi && !poza) ? "" : "  <- UKLAD SIE NIE SKLADA");
            for (int i = 0; i < 5; ++i)
                std::printf("    %-7s %4ld,%-4ld %4ld x %-4ld\n", nm[i],
                            long(r[i]->left), long(r[i]->top),
                            long(r[i]->right - r[i]->left),
                            long(r[i]->bottom - r[i]->top));
        }

        {   // Kazdy widok ma cos rysowac W SWOIM oknie. Sam licznik obiektow
            // by nie wystarczyl - to ta sama lekcja, co przy wrakach: licznik
            // mowi, ze pasek sie wczytal, a nie ze cos widac.
            int px[ed::V_COUNT] = { 0, 0, 0 };
            for (int v = 0; v < ed::V_COUNT; ++v) {
                g_menu.edit.view = v;
                g_menu.Compose();
                std::vector<uint32_t> a = g_menu.canvas;
                int zmian = 0;
                for (int y = L.view.top; y < L.view.bottom; ++y)
                    for (int x = L.view.left; x < L.view.right; ++x)
                        if (a[size_t(y) * SCREEN_W + x] != ed::VIEW_BG
                            && a[size_t(y) * SCREEN_W + x] != ed::BG) ++zmian;
                px[v] = zmian;
            }
            std::printf("  widoki: 3D %d px, 2D %d px, WARSTWY %d px%s\n",
                        px[0], px[1], px[2],
                        (px[0] && px[1] && px[2]) ? "" : "  <- WIDOK NIC NIE RYSUJE");
            // Dwa widoki, ktore pokazuja to samo, wygladalyby w tej linijce
            // tak samo jak dwa dzialajace - wiec liczy sie jeszcze roznica.
            g_menu.edit.view = ed::V_FLAT;
            g_menu.Compose();
            std::vector<uint32_t> f = g_menu.canvas;
            g_menu.edit.view = ed::V_LAYERS;
            g_menu.Compose();
            int inne = 0;
            for (size_t i = 0; i < f.size(); ++i)
                if (f[i] != g_menu.canvas[i]) ++inne;
            std::printf("  2D kontra WARSTWY: %d roznych pikseli%s\n", inne,
                        inne > 1000 ? "" : "  <- WIDOKI POKAZUJA TO SAMO");
        }

        {   // Droga gracza: kazda zakladka i kazde narzedzie ma dac sie wybrac
            // KLIKNIECIEM w swoj prostokat, a nie przestawieniem pola.
            int tabOK = 0, toolOK = 0;
            for (int i = 0; i < ed::V_COUNT; ++i) {
                RECT r = ed::tabRect(L, i);
                g_menu.edit.view = -1;
                g_menu.EditorClick((r.left + r.right) / 2, (r.top + r.bottom) / 2, false);
                if (g_menu.edit.view == i) ++tabOK;
            }
            for (int i = 0; i < ed::T_COUNT; ++i) {
                RECT r = ed::railRect(L, i);
                g_menu.edit.tool = -1;
                g_menu.EditorClick((r.left + r.right) / 2, (r.top + r.bottom) / 2, false);
                if (g_menu.edit.tool == i) ++toolOK;
            }
            std::printf("  klikanie: zakladek %d z %d, narzedzi %d z %d%s\n",
                        tabOK, ed::V_COUNT, toolOK, ed::T_COUNT,
                        (tabOK == ed::V_COUNT && toolOK == ed::T_COUNT)
                            ? "" : "  <- MARTWY WIDGET");
        }

        {   // Komorka pod kursorem: srodek widoku 2D ma wskazac blok mapy,
            // a klik w panel - nie wskazac nic. Sam pierwszy pomiar
            // przechodzilby tez wtedy, gdyby kursor wskazywal zawsze.
            g_menu.edit.view = ed::V_FLAT;
            g_menu.EditorMove((L.view.left + L.view.right) / 2,
                              (L.view.top + L.view.bottom) / 2);
            int bx = g_menu.edit.curBX, by = g_menu.edit.curBY;
            g_menu.EditorMove(L.rail.left + 4, L.rail.top + 4);
            bool pustoNadPanelem = g_menu.edit.curBX < 0;
            std::printf("  kursor: nad mapa blok %d,%d, nad panelem %s%s\n",
                        bx, by, pustoNadPanelem ? "nic" : "BLOK",
                        (bx >= 0 && pustoNadPanelem) ? "" : "  <- KURSOR NIE ROZROZNIA");
        }

        {   // Przycisk menu glownego musi lezec w oknie menu i nie nachodzic
            // na piatke guzikow gry ani na stopke.
            int kolizji = 0;
            RECT e{ EDBTN_X, EDBTN_Y, EDBTN_X + EDBTN_W, EDBTN_Y + EDBTN_H };
            for (int b = 0; b < BTN_COUNT; ++b) {
                RECT g{ kButtonPos[b].x, kButtonPos[b].y,
                        kButtonPos[b].x + BTN_W, kButtonPos[b].y + BTN_H };
                if (e.left < g.right && g.left < e.right
                    && e.top < g.bottom && g.top < e.bottom) ++kolizji;
            }
            bool wOknie = e.left >= 0 && e.top >= 0
                       && e.right <= MENU_W && e.bottom <= kFooterBox.top;
            g_menu.mode = 0;
            int traf = g_menu.HitTest((e.left + e.right) / 2, (e.top + e.bottom) / 2);
            std::printf("  przycisk EDYTOR MAP: %ld,%ld %ldx%ld, w oknie %s,"
                        " kolizji z guzikami gry %d, trafienie %d%s\n",
                        long(e.left), long(e.top), long(e.right - e.left),
                        long(e.bottom - e.top), wOknie ? "tak" : "NIE", kolizji, traf,
                        (wOknie && !kolizji && traf == BTN_EDITOR)
                            ? "" : "  <- PRZYCISK NIE DZIALA");
        }

        {   // **Palety pokazuja GRAFIKE, i musza dac sie klikac.** Sam fakt,
            // ze lista ma pozycje, niczego nie dowodzi - to ta sama pulapka,
            // co przy „ikony palety: z CONTROLG 0".
            g_menu.edit.tool = ed::T_TEXTURE;
            g_menu.Compose();
            // **Paleta liczy GRUPY, nie kafle.** Kafli jest dwiescie
            // z okladem i sa prawie identyczne; grup kilkanascie i to one
            // odpowiadaja „rodzajom gruntu". Liczba „z kaflem" jest tu
            // sprawdzianem: wariant bazowy musi rozwiazac sie do prawdziwej
            // grafiki, inaczej paleta pokazuje puste ramki.
            int kafli = int(g_menu.edit.texGroups.size());
            int zKafla = 0;
            for (const ed::TexGroupInfo &tg : g_menu.edit.texGroups)
                if (g_menu.landSet.tile(uint16_t((tg.group << 8) | tg.base)))
                    ++zKafla;
            int przedT = g_menu.edit.texGroup;
            int pw1, ph1, pk1;
            g_menu.EdPalCell(pw1, ph1, pk1);
            RECT s1 = g_menu.EdPalSlot(L, 3, pw1, ph1, pk1);
            g_menu.EditorClick((s1.left + s1.right) / 2, (s1.top + s1.bottom) / 2, false);
            bool klikT = kafli > 3
                      && g_menu.edit.texGroup == g_menu.edit.texGroups[3].group
                      && g_menu.edit.texGroup != przedT;

            g_menu.edit.tool = ed::T_OBJECT;
            g_menu.Compose();
            int obiektow = int(g_menu.edit.objList.size());
            int zIkona = 0;
            for (int t : g_menu.edit.objList) {
                int sl = units::iconSlot(t, g_menu.PlayerSide());
                char rec[24];
                std::snprintf(rec, sizeof(rec), "OBJS_%02d", sl);
                const panel::Image *ic = sl >= 0 ? g_menu.hud.image(rec) : nullptr;
                if (ic && ic->ok()) ++zIkona;
            }
            RECT s2 = g_menu.EdPalSlot(L, 2, 48, 33, 5);
            g_menu.EditorClick((s2.left + s2.right) / 2, (s2.top + s2.bottom) / 2, false);
            bool klikO = obiektow > 2 && g_menu.edit.placeTobj == g_menu.edit.objList[2];
            // Dekoracje: paleta nazw paskow tej mapy, i kazda musi miec
            // grafike - lista nazw bez paska wygladalaby tak samo pelna.
            g_menu.edit.tool = ed::T_DECOR;
            g_menu.Compose();
            int dekPoz = int(g_menu.edit.decChoices.size());
            int zPaskiem = 0;
            for (const auto &choice : g_menu.edit.decChoices)
                if (g_menu.landSet.strip(choice.name)) ++zPaskiem;
            bool klikD = false;
            if (dekPoz > 1) {
                RECT s3 = g_menu.EdPalSlot(L, 1, 40, 40, 6);
                g_menu.EditorClick((s3.left + s3.right) / 2,
                                   (s3.top + s3.bottom) / 2, false);
                klikD = g_menu.edit.placeDecor == g_menu.edit.decChoices[1].name
                    && g_menu.edit.placeDecorFrame == g_menu.edit.decChoices[1].frame;
            }

            // Gniazda startu: wiersze na calej szerokosci panelu.
            g_menu.edit.tool = ed::T_START;
            g_menu.Compose();
            bool klikG = false;
            if (g_menu.edit.slots.size() > 2) {
                g_menu.edit.startSlot = 0;
                int fh3 = g_menu.font.height();
                g_menu.EditorClick(L.insp.left + ed::PAD + 4,
                                   g_menu.edit.palTop + 2 * (fh3 + 5) + 2, false);
                klikG = g_menu.edit.startSlot == 2;
            }
            std::printf("  palety: gruntow %d (grafika %d), budynkow %d (ikona %d),"
                        " dekoracji %d (pasek %d), gniazd %d; kliki %s %s %s %s%s\n",
                        kafli, zKafla, obiektow, zIkona, dekPoz, zPaskiem,
                        int(g_menu.edit.slots.size()),
                        klikT ? "kafel" : "KAFEL", klikO ? "budynek" : "BUDYNEK",
                        klikD ? "dekoracja" : (dekPoz > 1 ? "DEKORACJA" : "-"),
                        klikG ? "gniazdo" : "GNIAZDO",
                        (kafli && zKafla == kafli && obiektow && zIkona == obiektow
                         && klikT && klikO && zPaskiem == dekPoz
                         && (dekPoz <= 1 || klikD) && klikG)
                            ? "" : "  <- PALETA NIE DZIALA");
        }

        {   // **Duch pod kursorem.** Para: przy narzedziu obiektu na plotnie
            // ma cos przybyc, a przy wskazniku - nie. Sam pierwszy pomiar
            // wyszedlby tak samo, gdyby duch rysowal sie zawsze.
            g_menu.edit.view = ed::V_ISO;
            g_menu.EditorMove((L.view.left + L.view.right) / 2,
                              (L.view.top + L.view.bottom) / 2);
            g_menu.edit.tool = ed::T_SELECT;
            g_menu.Compose();
            std::vector<uint32_t> bez = g_menu.canvas;
            g_menu.edit.tool = ed::T_OBJECT;
            g_menu.Compose();
            int duch = 0;
            for (size_t i = 0; i < bez.size(); ++i)
                if (bez[i] != g_menu.canvas[i]) ++duch;
            // Kontur pedzla: przy narzedziu terenu ma przybyc kresek, przy
            // wskazniku ma ich nie byc.
            g_menu.edit.tool = ed::T_SELECT;
            g_menu.edit.brush = 3;
            g_menu.Compose();
            std::vector<uint32_t> bezP = g_menu.canvas;
            g_menu.edit.tool = ed::T_RAISE;
            g_menu.Compose();
            int obrys = 0;
            for (size_t i = 0; i < bezP.size(); ++i)
                if (bezP[i] != g_menu.canvas[i]) ++obrys;
            // ------------------------------------------------- grunty
            // Trzy rzeczy naraz, i kazda obala co innego: ze grupa zbiera
            // warianty, ze dwa tryby pedzla daja INNY teren (bez tego
            // „WARIANTY" to nazwa bez skutku), i ze tryb bazowy ZDEJMUJE
            // nakladke - bo o to chodzi w „zostaw tylko ta bazowa srodkowa".
            {
                g_menu.edit.tool = ed::T_TEXTURE;
                int grup = int(g_menu.edit.texGroups.size());
                int wariantow = 0, zGrafika = 0;
                for (const ed::TexGroupInfo &tg : g_menu.edit.texGroups) {
                    wariantow += int(tg.variants.size());
                    for (const auto &wv : tg.variants)
                        if (g_menu.landSet.tile(uint16_t((tg.group << 8) | wv.first)))
                            ++zGrafika;
                }
                // Grupa o najwiekszej liczbie wariantow - tylko na takiej
                // widac roznice miedzy trybami.
                int naj = -1, ileW = 0;
                for (const ed::TexGroupInfo &tg : g_menu.edit.texGroups)
                    if (int(tg.variants.size()) > ileW) {
                        ileW = int(tg.variants.size());
                        naj = tg.group;
                    }
                std::vector<maps::Cell> odloz = g_menu.terr.cells;
                // Szukamy bloku, ktory da sie pomalowac, i dajemy mu nakladke,
                // zeby bylo co zdjac.
                int mbx = -1, mby = -1;
                for (int by = 1; by < g_menu.terr.bh - 1 && mbx < 0; ++by)
                    for (int bx = 1; bx < g_menu.terr.bw - 1; ++bx) {
                        int lv = g_menu.terr.topAt(bx, by);
                        if (lv < 0) continue;
                        mbx = bx; mby = by; break;
                    }
                int bazowych = 0, wariantowych = 0, roznych = 0, nakladek = 0;
                if (mbx >= 0 && naj >= 0 && ileW > 1) {
                    g_menu.edit.texGroup = naj;
                    const int PROM = 4;
                    // Nakladka na calym pedzlu - w trybie bazowym ma zniknac.
                    for (int dy = -PROM; dy <= PROM; ++dy)
                        for (int dx = -PROM; dx <= PROM; ++dx) {
                            int lv = g_menu.terr.topAt(mbx + dx, mby + dy);
                            if (lv < 0) continue;
                            int at = g_menu.EdCellIndex(mbx + dx, mby + dy, lv);
                            if (at >= 0) g_menu.terr.cells[size_t(at)].texB = 8473;
                        }
                    int stary = g_menu.edit.brush;
                    bool staryK = g_menu.edit.brushRound;
                    // **Pedzel musi byc KWADRATOWY.** Pomiar chodzi po
                    // kwadracie (2*PROM+1)^2, a okragly pedzel rogow nie
                    // maluje - wychodzilo z tego „4 kafle" i „nakladek
                    // zostalo 3" przy zupelnie sprawnym pedzlu.
                    g_menu.edit.brushRound = false;
                    g_menu.edit.brush = PROM;
                    g_menu.edit.texMode = ed::TEX_BAZOWY;
                    g_menu.EdStroke(mbx, mby, ed::T_TEXTURE);
                    std::vector<uint16_t> poBaz;
                    for (int dy = -PROM; dy <= PROM; ++dy)
                        for (int dx = -PROM; dx <= PROM; ++dx) {
                            int lv = g_menu.terr.topAt(mbx + dx, mby + dy);
                            if (lv < 0) continue;
                            int at = g_menu.EdCellIndex(mbx + dx, mby + dy, lv);
                            if (at < 0) continue;
                            poBaz.push_back(g_menu.terr.cells[size_t(at)].texA);
                            if (g_menu.terr.cells[size_t(at)].texB) ++nakladek;
                        }
                    std::vector<uint16_t> u = poBaz;
                    std::sort(u.begin(), u.end());
                    u.erase(std::unique(u.begin(), u.end()), u.end());
                    bazowych = int(u.size());

                    g_menu.terr.cells = odloz;
                    g_menu.edit.texMode = ed::TEX_WARIANTY;
                    g_menu.EdStroke(mbx, mby, ed::T_TEXTURE);
                    std::vector<uint16_t> poWar;
                    for (int dy = -PROM; dy <= PROM; ++dy)
                        for (int dx = -PROM; dx <= PROM; ++dx) {
                            int lv = g_menu.terr.topAt(mbx + dx, mby + dy);
                            if (lv < 0) continue;
                            int at = g_menu.EdCellIndex(mbx + dx, mby + dy, lv);
                            if (at >= 0) poWar.push_back(g_menu.terr.cells[size_t(at)].texA);
                        }
                    std::vector<uint16_t> u2 = poWar;
                    std::sort(u2.begin(), u2.end());
                    u2.erase(std::unique(u2.begin(), u2.end()), u2.end());
                    wariantowych = int(u2.size());
                    for (size_t i = 0; i < poBaz.size() && i < poWar.size(); ++i)
                        if (poBaz[i] != poWar[i]) ++roznych;
                    // Powtarzalnosc: to samo pociagniecie ma dac to samo.
                    g_menu.terr.cells = odloz;
                    g_menu.EdStroke(mbx, mby, ed::T_TEXTURE);
                    size_t k = 0;
                    bool powt = true;
                    for (int dy = -PROM; dy <= PROM; ++dy)
                        for (int dx = -PROM; dx <= PROM; ++dx) {
                            int lv = g_menu.terr.topAt(mbx + dx, mby + dy);
                            if (lv < 0) continue;
                            int at = g_menu.EdCellIndex(mbx + dx, mby + dy, lv);
                            if (at < 0 || k >= poWar.size()) continue;
                            if (g_menu.terr.cells[size_t(at)].texA != poWar[k]) powt = false;
                            ++k;
                        }
                    g_menu.edit.brush = stary;
                    g_menu.edit.brushRound = staryK;
                    g_menu.terr.cells = odloz;
                    g_menu.edit.texMode = ed::TEX_BAZOWY;
                    std::printf("  grunty: grup %d, wariantow %d (grafika %d);"
                                " najwieksza grupa %d ma %d\n",
                                grup, wariantow, zGrafika, naj, ileW);
                    std::printf("    pedzel BAZOWY %d kafel, nakladek zostalo %d;"
                                " WARIANTY %d kafli, komorek innych %d, powtarzalne %s%s\n",
                                bazowych, nakladek, wariantowych, roznych,
                                powt ? "tak" : "NIE",
                                (bazowych == 1 && nakladek == 0 && wariantowych > 1
                                 && roznych > 0 && powt)
                                    ? "" : "  <- TRYBY PEDZLA NIE ROZROZNIAJA");
                } else {
                    std::printf("  grunty: grup %d, wariantow %d (grafika %d)"
                                "  <- NIE MA GRUPY Z WARIANTAMI, nie ma czego mierzyc\n",
                                grup, wariantow, zGrafika);
                }

                // Krawedzie: para. Przy teksturze maja sie rysowac, przy
                // wskazniku ani jedna - inaczej pomiar wyszedlby tak samo,
                // gdyby rysowaly sie zawsze.
                g_menu.edit.tool = ed::T_TEXTURE;
                g_menu.Compose();
                int krzTex = g_menu.edit.texEdges;
                g_menu.edit.tool = ed::T_SELECT;
                g_menu.Compose();
                int krzWsk = g_menu.edit.texEdges;
                std::printf("    styk gruntow: przy TEKSTURZE %d kresek,"
                            " przy WSKAZNIKU %d%s\n", krzTex, krzWsk,
                            (krzTex > 0 && krzWsk == 0)
                                ? "" : "  <- STYK NIE ROZROZNIA NARZEDZIA");
                g_menu.edit.tool = ed::T_TEXTURE;
            }

            std::printf("  pod kursorem: duch obiektu %d px, obrys pedzla %d px%s\n",
                        duch, obrys,
                        (duch > 200 && obrys > 100) ? ""
                            : "  <- POD KURSOREM NIC NIE WIDAC");
            g_menu.edit.brush = 1;
        }

        {   // **Cofanie.** Trzy stany, nie dwa: po zmianie, po cofnieciu
            // i po ponowieniu. Samo „cofniecie wraca do poprzedniego" wyszloby
            // tak samo, gdyby cofanie nie robilo nic, a zmiana nie dochodzila.
            size_t a = g_menu.terr.cells.size();
            g_menu.EdPushUndo();
            g_menu.edit.brush = 2;
            g_menu.EdStroke(g_menu.terr.bw / 3, g_menu.terr.bh / 3, ed::T_RAISE);
            size_t b = g_menu.terr.cells.size();
            bool cof = g_menu.EdUndo();
            size_t c = g_menu.terr.cells.size();
            bool pon = g_menu.EdRedo();
            size_t d = g_menu.terr.cells.size();
            g_menu.EdUndo();
            std::printf("  cofanie: %d -> %d -> cofniete %d -> ponowione %d%s\n",
                        int(a), int(b), int(c), int(d),
                        (b > a && cof && c == a && pon && d == b)
                            ? "" : "  <- COFANIE NIE DZIALA");
            g_menu.edit.brush = 1;
        }

        {   // **Malowanie przeciagnieciem.** Ruch przy trzymanym przycisku ma
            // podnosic kolejne bloki; ruch bez trzymania - nie.
            g_menu.edit.tool = ed::T_RAISE;
            g_menu.edit.brush = 0;
            g_menu.edit.view = ed::V_FLAT;
            g_menu.edit.painting = false;
            // **Miara to ODCISK terenu, nie liczba komorek.** Przy wlaczonym
            // konturze pociagniecie potrafi komorke DODAC i inna ZDJAC, wiec
            // licznik bywa taki sam albo mniejszy - a teren i tak sie zmienil.
            // Liczenie sztuk pokazywalo to jako usterke na trzech mapach.
            g_menu.Compose();                 // dopasuj zoom przed pomiarem
            auto odcisk = [&]() {
                uint32_t h = 2166136261u;
                for (const maps::Cell &c : g_menu.terr.cells) {
                    uint32_t v = uint32_t(c.level) | (uint32_t(c.x) << 8)
                               | (uint32_t(c.y) << 16);
                    h = (h ^ v) * 16777619u;
                }
                return h;
            };
            // Trasa musi isc po blokach, ktore DA SIE podniesc - blok na
            // najwyzszym poziomie slusznie nie drgnie, a test czytalby to
            // jako usterke. Ta sama regula, co przy audycie stoczni.
            int sx3 = -1, sy3 = -1;
            for (int y = 1; y < g_menu.terr.bh - 8 && sx3 < 0; ++y)
                for (int x = 1; x < g_menu.terr.bw - 8 && sx3 < 0; ++x) {
                    int ile = 0;
                    for (int k = 0; k < 6; ++k) {
                        int t = g_menu.terr.topAt(x + k, y);
                        if (t >= 0 && t + 1 < maps::LEVELS) ++ile;
                    }
                    if (ile == 6) { sx3 = x; sy3 = y; }
                }
            int ox3 = 0, oy3 = 0;
            g_menu.EdFlatOrigin(L.view, ox3, oy3);
            int krok = g_menu.edit.flatZoom > 0 ? g_menu.edit.flatZoom : 8;
            uint32_t przed = odcisk();
            if (sx3 >= 0)
                for (int k = 0; k < 6; ++k)
                    g_menu.EditorMove(ox3 + (sx3 + k) * krok + krok / 2,
                                      oy3 + sy3 * krok + krok / 2);
            uint32_t bezTrzym = odcisk();
            g_menu.edit.painting = true;
            g_menu.edit.lastBX = g_menu.edit.lastBY = -1;
            if (sx3 >= 0)
                for (int k = 0; k < 6; ++k)
                    g_menu.EditorMove(ox3 + (sx3 + k) * krok + krok / 2,
                                      oy3 + sy3 * krok + krok / 2);
            uint32_t zTrzym = odcisk();
            g_menu.EditorRelease();
            std::printf("  przeciaganie: trasa %d,%d; bez trzymania teren %s,"
                        " z trzymaniem %s%s\n", sx3, sy3,
                        bezTrzym == przed ? "bez zmian" : "ZMIENIONY",
                        zTrzym != bezTrzym ? "zmieniony" : "BEZ ZMIAN",
                        (sx3 >= 0 && bezTrzym == przed && zTrzym != bezTrzym)
                            ? "" : "  <- PRZECIAGANIE NIE MALUJE");
            g_menu.edit.brush = 1;
        }

        {   // **Przewijanie dziala w obu widokach.** Mierzy sie SKUTKIEM na
            // plotnie, a nie zmiana pola kamery: pole moze sie ruszyc, a obraz
            // nie, gdy cos po drodze je przytnie.
            auto obraz = [&]() {
                g_menu.Compose();
                uint32_t h = 2166136261u;
                for (size_t i = 0; i < g_menu.canvas.size(); i += 97)
                    h = (h ^ g_menu.canvas[i]) * 16777619u;
                return h;
            };
            // **Widok, w ktorym cala mapa sie MIESCI, nie ma czego przewijac**
            // - kamera jest wtedy przyszpilona i obraz sie nie zmienia. To nie
            // usterka, tylko brak czego mierzyc; liczy sie wiec tylko te
            // widoki, w ktorych mapa wystaje za okno. Ta sama regula, co przy
            // audycie stoczni - ten wiersz zapalal sie przez nia na mapie 26
            // przy oknie 1200x800, a przy 1400x900 i 900x600 nie.
            // **Mierzymy OSOBNO po osiach.** Widok potrafi byc przyszpilony
            // w jednej osi (mapa wezsza niz okno) i wolny w drugiej, wiec
            // wspolny pomiar „przesun o (120,90)" mylil jedno z drugim:
            // wiersz zapalal sie na mapie 26 przy 1200x800 i 1920x1080,
            // a przy 1400x900 i 900x600 nie. Os, w ktorej cala mapa sie
            // miesci, nie ma czego przewijac - to nie usterka, tylko brak
            // czego mierzyc. Ta sama regula, co przy audycie stoczni.
            int ile = 0, dziala = 0, ciasnych = 0;
            for (int v = 0; v < 2; ++v) {
                g_menu.edit.view = v == 0 ? ed::V_ISO : ed::V_FLAT;
                g_menu.EdCentre();
                (void)obraz();                 // sklada raz, zeby widok 2D dopasowal zoom
                // **Porownujemy z tym prostokatem, ktorym przycina KAMERA.**
                // Widok 3D rysuje swiat na CALYM oknie (panele wchodza na
                // niego z gory), a `ClampCamera` liczy wzgledem `SCREEN_W/H`;
                // porownanie z mniejszym prostokatem widoku mowilo „mapa
                // wystaje", gdy kamera byla juz przyszpilona - i tak wlasnie
                // ten wiersz zapalal sie przy 1920x1080.
                int vw, vh, mw, mh;
                if (v == 0) {
                    vw = SCREEN_W; vh = SCREEN_H;
                    mw = g_menu.WorldW(); mh = g_menu.WorldH();
                } else {
                    vw = L.view.right - L.view.left;
                    vh = L.view.bottom - L.view.top;
                    mw = g_menu.terr.bw * g_menu.edit.flatZoom;
                    mh = g_menu.terr.bh * g_menu.edit.flatZoom;
                }
                for (int os = 0; os < 2; ++os) {
                    if (os == 0 ? mw <= vw : mh <= vh) { ++ciasnych; continue; }
                    // **Ruch mierzymy obrazem, powrot LICZBAMI kamery.**
                    // Widok 3D jest animowany (kepki, nakladki budynkow), wiec
                    // dwa zlozenia tej samej kamery roznia sie pikselami -
                    // i „powrot" liczony obrazem wychodzil NIE przy sprawnym
                    // HOME. Widok 2D jest statyczny i tego nie pokazywal.
                    g_menu.EdCentre();
                    int sx0 = v == 0 ? g_menu.camX : g_menu.edit.flatX;
                    int sy0 = v == 0 ? g_menu.camY : g_menu.edit.flatY;
                    uint32_t a2 = obraz();
                    g_menu.EdPan(os == 0 ? 120 : 0, os == 0 ? 0 : 90);
                    uint32_t b2 = obraz();
                    g_menu.EdCentre();
                    int sx1 = v == 0 ? g_menu.camX : g_menu.edit.flatX;
                    int sy1 = v == 0 ? g_menu.camY : g_menu.edit.flatY;
                    ++ile;
                    if (a2 != b2 && sx0 == sx1 && sy0 == sy1) ++dziala;
                    else std::printf("  [sonda] widok %s os %s: mapa %dx%d"
                                     " okno %dx%d, ruch %s, kamera %d,%d -> %d,%d\n",
                                     v == 0 ? "3D" : "2D", os == 0 ? "x" : "y",
                                     mw, mh, vw, vh, a2 != b2 ? "tak" : "NIE",
                                     sx0, sy0, sx1, sy1);
                }
            }
            // Ciagniecie srodkowym: ta sama droga, co u gracza.
            g_menu.edit.view = ed::V_FLAT;
            g_menu.EdCentre();
            uint32_t d0 = obraz();
            g_menu.EditorPanStart(L.view.left + 200, L.view.top + 200);
            g_menu.EditorMove(L.view.left + 260, L.view.top + 250);
            g_menu.EditorRelease();
            uint32_t d1 = obraz();
            std::printf("  przewijanie: osi %d z %d (mapa w oknie %d),"
                        " ciagniecie %s, HOME wraca %s%s\n", dziala, ile,
                        ciasnych, d0 != d1 ? "tak" : "NIE",
                        dziala == ile ? "tak" : "NIE",
                        (dziala == ile && (ile == 0 || d0 != d1)) ? ""
                            : "  <- MAPA SIE NIE PRZEWIJA");
            g_menu.EdCentre();
        }

        {   // **Obiekty i dekoracje tam i z powrotem, BAJT W BAJT.**
            //
            // To jest ten sam sprawdzian, co przy terenie, i tak samo
            // rozstrzyga: jesli serializacja odtwarza oryginalne bajty
            // rekordu, to zapis nie ma czego zepsuc - a jesli nie, to
            // widac to tu, a nie w zepsutej mapie u gracza.
            int obRazem = 0, obZgodnych = 0, obBezBajtow = 0;
            for (const maps::Object &o : g_menu.terr.objects) {
                if (o.spawned || o.raw.empty()) { ++obBezBajtow; continue; }
                ++obRazem;
                std::vector<uint8_t> z = maps::serializeObject(o);
                if (z == o.raw) ++obZgodnych;
            }
            std::printf("  obiekty tam i z powrotem: %d z %d zgodnych"
                        " (bez bajtow %d)%s\n", obZgodnych, obRazem, obBezBajtow,
                        (obRazem && obZgodnych == obRazem)
                            ? "" : "  <- OBIEKT NIE ODTWARZA SIE");

            int dekZgodnych = -1, dekIle = int(g_menu.terr.decor.size());
            {
                ark::Archive a;
                const maps::Entry &me = g_menu.skMaps[size_t(mapa)];
                if (a.open(me.dkx, me.dkd) && !g_menu.terr.decorRec.empty()) {
                    std::vector<uint8_t> orig = a.read(g_menu.terr.decorRec);
                    std::vector<uint8_t> z =
                        maps::serializeDecor(g_menu.terr.decor, orig);
                    if (z.size() == orig.size()) {
                        dekZgodnych = 0;
                        for (size_t i = 0; i < z.size(); ++i)
                            if (z[i] != orig[i]) ++dekZgodnych;
                    }
                }
            }
            std::printf("  dekoracje tam i z powrotem: %d wpisow,"
                        " roznych bajtow %d%s\n", dekIle, dekZgodnych,
                        (dekIle == 0 || dekZgodnych == 0)
                            ? "" : "  <- DEKORACJE NIE ODTWARZAJA SIE");
        }

        {   // **Zapis ma ODDAC obiekty i dekoracje**, a nie tylko teren.
            // Mierzone po odczycie zapisanej mapy, bo sam brak bledu nie
            // mowi, czy rekordy wyszly.
            int bx4 = -1, by4 = -1;
            for (int y = 0; y < g_menu.terr.bh && bx4 < 0; ++y)
                for (int x = 0; x < g_menu.terr.bw && bx4 < 0; ++x) {
                    int t = g_menu.terr.topAt(x, y);
                    if (t >= 0 && t + 1 < maps::LEVELS) { bx4 = x; by4 = y; }
                }
            if (bx4 >= 0) g_menu.EdRaise(bx4, by4);      // wymusza cale archiwum
            size_t obPrzed = g_menu.terr.objects.size();
            size_t dekPrzed = g_menu.terr.decor.size();
            size_t komPrzed = g_menu.terr.cells.size();
            std::string nz = g_menu.EdFreeName("AUDYT_OBIEKTY");
            std::string er = nz.empty() ? "brak wolnej nazwy" : g_menu.EdSaveAs(nz);
            std::string dkx4, dkd4;
            g_menu.EdTargets(nz, dkx4, dkd4);
            size_t obPo = 0, dekPo = 0, komPo = 0;
            bool wroc4 = false;
            if (er.empty()) {
                std::vector<maps::Entry> l = maps::scan(g_menu.EdMapDir());
                for (const maps::Entry &e : l)
                    if (e.file == nz) {
                        maps::Terrain t;
                        wroc4 = maps::loadTerrain(e, t);
                        obPo = t.objects.size();
                        dekPo = t.decor.size();
                        komPo = t.cells.size();
                    }
            }
            std::printf("  zapis calosci: %s, odczyt %s; obiektow %d -> %d,"
                        " dekoracji %d -> %d, komorek %d -> %d%s\n",
                        er.empty() ? "ok" : er.c_str(), wroc4 ? "tak" : "nie",
                        int(obPrzed), int(obPo), int(dekPrzed), int(dekPo),
                        int(komPrzed), int(komPo),
                        (er.empty() && wroc4 && obPo == obPrzed
                         && dekPo == dekPrzed && komPo == komPrzed)
                            ? "" : "  <- ZAPIS GUBI ZAWARTOSC");
            if (er.empty()) {
                std::remove(dkx4.c_str());
                std::remove(dkd4.c_str());
            }
            if (bx4 >= 0) g_menu.EdLower(bx4, by4);
        }

        {   // Sonda: KTORE offsety serializacja psuje. Bez tego „5 z 130 nie
            // zgadza sie" nie mowi, co poprawic.
            std::map<int, int> psute;
            std::map<uint32_t, int> klasy;
            for (const maps::Object &o : g_menu.terr.objects) {
                if (o.spawned || o.raw.empty()) continue;
                std::vector<uint8_t> z = maps::serializeObject(o);
                if (z == o.raw) continue;
                ++klasy[o.type];
                for (size_t i = 0; i < z.size() && i < o.raw.size(); ++i)
                    if (z[i] != o.raw[i]) ++psute[int(i)];
            }
            std::printf("  [sonda] obiekty: klasy");
            for (const auto &kv : klasy) std::printf(" %u x%d", kv.first, kv.second);
            std::printf("; offsety");
            for (const auto &kv : psute) std::printf(" +%d x%d", kv.first, kv.second);
            std::printf("\n");
            {
                ark::Archive a;
                const maps::Entry &me2 = g_menu.skMaps[size_t(mapa)];
                if (a.open(me2.dkx, me2.dkd) && !g_menu.terr.decorRec.empty()) {
                    std::vector<uint8_t> orig = a.read(g_menu.terr.decorRec);
                    std::vector<uint8_t> z =
                        maps::serializeDecor(g_menu.terr.decor, orig);
                    std::printf("  [sonda] dekoracje: offsety");
                    for (size_t i = 0; i < z.size() && i < orig.size(); ++i)
                        if (z[i] != orig[i])
                            std::printf(" +%d (wpis %d, w wpisie +%d: %02x->%02x)",
                                        int(i),
                                        int(i) < maps::DECOR_BASE ? -1
                                          : (int(i) - maps::DECOR_BASE) / maps::DECOR_STRIDE,
                                        int(i) < maps::DECOR_BASE ? int(i)
                                          : (int(i) - maps::DECOR_BASE) % maps::DECOR_STRIDE,
                                        orig[i], z[i]);
                    std::printf("\n");
                }
            }
        }

        {   // **Postawiony obiekt ma DOJSC DO PLIKU**, a zdjety z niego zniknac.
            // Scena i plik to dwie rozne rzeczy - dokladnie ta roznica dzielila
            // przez caly czas „USUN dziala" od „USUN idzie do pliku".
            //
            // Liczy sie tylko to, co naprawde stanelo. Mapa bez ani jednego
            // obiektu danej klasy nie ma czego sklonowac i edytor **slusznie
            // odmawia** - a test, ktory wymagal wtedy usuniecia, pokazywal to
            // jako usterke na 30 z 58 map. Ta sama regula, co przy audycie
            // stoczni: brak czego mierzyc to nie to samo, co zepsute.
            int wx[3] = { -1, -1, -1 }, wy[3] = { -1, -1, -1 };
            {
                int ile = 0;
                for (int y = 1; y < g_menu.terr.bh - 1 && ile < 3; ++y)
                    for (int x = 1; x < g_menu.terr.bw - 1 && ile < 3; ++x)
                        if (g_menu.terr.topAt(x, y) >= 0 && !g_menu.EdObjectAt(x, y)) {
                            wx[ile] = x; wy[ile] = y; ++ile;
                        }
            }
            size_t obPrzed = g_menu.terr.objects.size();
            size_t dekPrzed = g_menu.terr.decor.size();
            std::string eB = "brak wolnej komorki", eZ = eB, eD = eB;
            if (wx[0] >= 0) eB = g_menu.EdPlaceObject(wx[0], wy[0], maps::OBJ_BUILDING, 50);
            if (wx[1] >= 0) eZ = g_menu.EdPlaceObject(wx[1], wy[1], maps::OBJ_RESOURCE, 221);
            if (wx[2] >= 0) eD = g_menu.EdPlaceDecor(wx[2], wy[2], std::string());
            int stanelo = int(eB.empty()) + int(eZ.empty());
            size_t obPo = g_menu.terr.objects.size();
            size_t dekPo = g_menu.terr.decor.size();

            std::string nz2 = g_menu.EdFreeName("AUDYT_STAWIANIE");
            std::string er2 = nz2.empty() ? "brak wolnej nazwy" : g_menu.EdSaveAs(nz2);
            std::string dkx5, dkd5;
            g_menu.EdTargets(nz2, dkx5, dkd5);
            size_t obPlik = 0, dekPlik = 0;
            if (er2.empty()) {
                for (const maps::Entry &e : maps::scan(g_menu.EdMapDir()))
                    if (e.file == nz2) {
                        maps::Terrain t;
                        if (maps::loadTerrain(e, t)) {
                            obPlik = t.objects.size();
                            dekPlik = t.decor.size();
                        }
                    }
                std::remove(dkx5.c_str());
                std::remove(dkd5.c_str());
            }
            bool okStaw = er2.empty()
                       && int(obPo - obPrzed) == stanelo
                       && obPlik == obPo
                       && dekPo - dekPrzed == size_t(eD.empty() ? 1 : 0)
                       && dekPlik == dekPo;
            std::printf("  stawianie: budynek %s, zloze %s, dekoracja %s;"
                        " obiektow %d -> %d (w pliku %d), dekoracji %d -> %d"
                        " (w pliku %d)%s\n",
                        eB.empty() ? "ok" : eB.c_str(),
                        eZ.empty() ? "ok" : eZ.c_str(),
                        eD.empty() ? "ok" : eD.c_str(),
                        int(obPrzed), int(obPo), int(obPlik),
                        int(dekPrzed), int(dekPo), int(dekPlik),
                        okStaw ? "" : "  <- STAWIANIE NIE DOCHODZI DO PLIKU");

            // Para: zdejmujemy DOKLADNIE to, co stanelo, i sprawdzamy, ze
            // plik to oddaje. Bez tej polowy „stawianie dziala" nie mowi,
            // czy da sie cokolwiek poprawic.
            int zdjeto = 0;
            if (eB.empty() && g_menu.EdEraseObject(wx[0], wy[0])) ++zdjeto;
            if (eZ.empty() && g_menu.EdEraseObject(wx[1], wy[1])) ++zdjeto;
            bool zdjD = eD.empty() && g_menu.EdEraseDecor(wx[2], wy[2]);
            size_t obKon = g_menu.terr.objects.size();
            std::string nz3 = g_menu.EdFreeName("AUDYT_USUWANIE");
            std::string er3 = nz3.empty() ? "brak wolnej nazwy" : g_menu.EdSaveAs(nz3);
            std::string dkx6, dkd6;
            g_menu.EdTargets(nz3, dkx6, dkd6);
            size_t obPlik2 = 0;
            if (er3.empty()) {
                for (const maps::Entry &e : maps::scan(g_menu.EdMapDir()))
                    if (e.file == nz3) {
                        maps::Terrain t;
                        if (maps::loadTerrain(e, t)) obPlik2 = t.objects.size();
                    }
                std::remove(dkx6.c_str());
                std::remove(dkd6.c_str());
            }
            bool okUsun = er3.empty() && zdjeto == stanelo
                       && zdjD == eD.empty()
                       && obKon == obPrzed && obPlik2 == obKon;
            std::printf("  usuwanie: zdjetych %d z %d postawionych,"
                        " dekoracja %s; obiektow %d -> %d (w pliku %d)%s\n",
                        zdjeto, stanelo, zdjD ? "zdjeta" : (eD.empty() ? "NIE" : "nie stawiano"),
                        int(obPo), int(obKon), int(obPlik2),
                        okUsun ? "" : "  <- USUWANIE NIE DOCHODZI DO PLIKU");
        }

        {   // **Punkt startu ma dojsc do `DESCRIPTOR`.** Mierzone po odczycie
            // zapisanej mapy tablica gniazd, a nie polem w edytorze - bo
            // latka w miejscu podmienia wylacznie `3D_MAP` i przy samej
            // zmianie startu zapis wyszedlby bez bledu, gubiac ja po cichu.
            int gn = -1;
            for (size_t i = 0; i < g_menu.edit.slots.size(); ++i)
                if (g_menu.edit.slots[i].used) { gn = int(i); break; }
            int cx7 = g_menu.terr.bw / 2, cy7 = g_menu.terr.bh / 3;
            uint32_t bylX = 0, bylY = 0;
            if (gn >= 0) {
                bylX = g_menu.edit.slots[size_t(gn)].x;
                bylY = g_menu.edit.slots[size_t(gn)].y;
                g_menu.edit.startSlot = gn;
                g_menu.edit.tool = ed::T_START;
                g_menu.edit.curBX = cx7;
                g_menu.edit.curBY = cy7;
                g_menu.EditorClick(L.view.left + 1, L.view.top + 1, false);
            }
            uint32_t terazX = gn >= 0 ? g_menu.edit.slots[size_t(gn)].x : 0;
            uint32_t terazY = gn >= 0 ? g_menu.edit.slots[size_t(gn)].y : 0;
            std::string nz7 = g_menu.EdFreeName("AUDYT_START");
            std::string er7 = (gn < 0) ? "mapa nie ma uzytego gniazda"
                            : (nz7.empty() ? "brak wolnej nazwy" : g_menu.EdSaveAs(nz7));
            std::string dkx7, dkd7;
            g_menu.EdTargets(nz7, dkx7, dkd7);
            uint32_t plikX = 0, plikY = 0;
            if (er7.empty()) {
                for (const maps::Entry &e : maps::scan(g_menu.EdMapDir()))
                    if (e.file == nz7 && gn < int(e.slots.size())) {
                        plikX = e.slots[size_t(gn)].x;
                        plikY = e.slots[size_t(gn)].y;
                    }
                std::remove(dkx7.c_str());
                std::remove(dkd7.c_str());
            }
            std::printf("  punkt startu: gniazdo %d, %u,%u -> %u,%u,"
                        " w pliku %u,%u%s\n", gn, bylX, bylY, terazX, terazY,
                        plikX, plikY,
                        (gn >= 0 && er7.empty()
                         && terazX == uint32_t(cx7 * 2) && terazY == uint32_t(cy7 * 2)
                         && plikX == terazX && plikY == terazY)
                            ? "" : "  <- PUNKT STARTU NIE DOCHODZI DO PLIKU");
        }

        // Zrzut robi sie w stanie, ktory cos POKAZUJE: narzedzie terenu,
        // pedzel 2 i kursor na srodku mapy. Inaczej ostatni podtest zostawia
        // wybrane USUN i na obrazku nie widac ani pedzla, ani konturu.
        g_menu.edit.view = widok;
        g_menu.edit.tool = argc > 7 ? std::atoi(argv[7]) : ed::T_RAISE;
        g_menu.edit.brush = 2;
        g_menu.EditorMove((L.view.left + L.view.right) / 2,
                          (L.view.top + L.view.bottom) / 2);
        g_menu.Compose();
        {   FILE *o = std::fopen("edytor.raw", "wb");
            if (o) { std::fwrite(g_menu.canvas.data(), 4,
                                 g_menu.canvas.size(), o); std::fclose(o); }
        }
        std::printf("  zrzut: edytor.raw %dx%d\n", SCREEN_W, SCREEN_H);
        return 0;
    }

    if (argc > 3 && std::strcmp(argv[2], "--panel") == 0) {
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        // A stable file stem avoids moving the render baseline when the
        // user adds another custom map. Numeric indices still work.
        if (argv[3][0] < '0' || argv[3][0] > '9') {
            g_menu.skSel = -1;
            for (size_t k=0;k<g_menu.skMaps.size();++k)
                if (lstrcmpiA(g_menu.skMaps[k].file.c_str(),argv[3])==0) g_menu.skSel=int(k);
            if (g_menu.skSel < 0) { std::printf("brak mapy %s\n",argv[3]); return 1; }
        }
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = argc > 5 ? std::atoi(argv[4]) : 1200;
        g_clientH = argc > 5 ? std::atoi(argv[5]) : 800;
        g_menu.ApplyZoom(4);
        g_menu.CentreCamera();
        g_menu.adminOpen = true;
        g_menu.adminPick = 3;                       // crui
        g_menu.adminOwner = 1;

        // Put a squad down near the middle of the open water.
        int bx = g_menu.terr.bw / 2, by = g_menu.terr.bh / 2;
        while (by < g_menu.terr.bh - 2 && !g_menu.Passable(bx, by)) ++by;
        static const int kTypes[6] = { 3, 3, 2, 1, 12, 14 };
        for (int i = 0; i < 6; ++i) {
            Menu::Unit u;
            u.owner = 1;
            u.type = uint32_t(kTypes[i]);
            u.x = float(bx * 2) + float(i % 3) * 2.0f;
            u.y = float(by * 2) + float(i / 3) * 2.0f;
            u.tx = u.x; u.ty = u.y;
            u.dir = 3;
            u.spawned = true;
            g_menu.units.push_back(u);
        }
        g_menu.sel.clear();
        for (int i = 0; i < 6; ++i)
            g_menu.sel.push_back(int(g_menu.units.size()) - 6 + i);

        // Order them somewhere across the map and let them get going.
        int tx = 0, ty = 0, r = 0;
        for (r = 6; r < g_menu.terr.bw; ++r) {
            if (g_menu.Passable(bx + r, by + r)) { tx = bx + r; ty = by + r; break; }
        }
        if (tx == 0) { tx = bx; ty = by; }
        int sx, sy;
        g_menu.CellToScreen(float(tx * 2), float(ty * 2), sx, sy);
        g_menu.OrderMove(sx, sy);
        DWORD t = 0;
        for (int k = 0; k < 30; ++k) { t += 50; g_menu.StepUnits(t); }
        int cx, cy;
        g_menu.CellToScreen(g_menu.units[g_menu.units.size() - 6].x,
                            g_menu.units[g_menu.units.size() - 6].y, cx, cy);
        g_menu.camX += cx - g_clientW / 2;
        g_menu.camY += cy - g_clientH / 2;
        g_menu.ClampCamera();
        // kursor nad wlasna lodzia, zeby bylo go widac na zrzucie
        for (const maps::Object &ro : g_menu.terr.objects)
            if (ro.type == maps::OBJ_RESOURCE) {    // kamera na pierwsze zloze
                int rx2, ry2;
                g_menu.CellToScreen(float(ro.x), float(ro.y), rx2, ry2);
                g_menu.camX += rx2 - g_clientW / 2;
                g_menu.camY += ry2 - g_clientH / 2;
                g_menu.ClampCamera();
                break;
            }
        if (false) {                        // kamera na pierwszy budynek
            int bx2, by2;
            g_menu.CellToScreen(float(g_menu.blds[0].x), float(g_menu.blds[0].y), bx2, by2);
            g_menu.camX += bx2 - g_clientW / 2;
            g_menu.camY += by2 - g_clientH / 2;
            g_menu.ClampCamera();
        }
        g_menu.mouse = POINT{ g_clientW / 2 + 240, g_clientH / 2 + 90 };
        if (!g_menu.blds.empty()) g_menu.selBld.assign(1, 0);   // pokaz zaznaczony budynek
        // Budowniczy zaznaczony i wybrany budynek - w zrzucie widac panel
        // komend, palete i ducha pod kursorem.
        for (size_t k = 0; k < g_menu.units.size(); ++k)
            if (g_menu.units[k].type == 12) { g_menu.sel.assign(1, int(k)); break; }
        g_menu.palOpen = true;                      // jak po nacisnieciu BUDUJ
        g_menu.palTab = 1;                          // zakladka wiezyczek
        g_menu.fogOn = argc > 8 ? std::atoi(argv[8]) != 0 : false;
        g_menu.ResetFog();
        g_menu.StepFog(1.0f);                       // odkryj wokol wlasnych
        int bp = 0, bn = 0;
        {   const int *bl = cost::bldList(g_menu.PlayerSide(), bn);
            for (int q = 0; q < bn; ++q)
                if (cost::bldCategory(bl[q], g_menu.PlayerSide()) == 1) { bp = bl[q]; break; }
        }
        g_menu.buildPick = bp;                      // pierwsza wiezyczka rasy
        {   float gx, gy;
            g_menu.ScreenToCell(g_menu.mouse.x, g_menu.mouse.y, gx, gy);
            int gcx = int(gx), gcy = int(gy);
            int near2 = 0;
            for (const Menu::Bld &bb : g_menu.blds) {
                int ddx = bb.x - gcx, ddy = bb.y - gcy;
                if (ddx * ddx + ddy * ddy < 16) ++near2;
            }
            std::printf("duch: komorka %d,%d przejezdna %s, blisko budynkow %d, mozna %s\n",
                        gcx, gcy,
                        g_menu.Passable(gcx / 2, gcy / 2) ? "tak" : "NIE", near2,
                        g_menu.CanBuildAt(gcx, gcy) ? "tak" : "NIE");
        }
        std::printf("mapa %d: %s (%s)\n", g_menu.skSel,
                    g_menu.skMaps[size_t(g_menu.skSel)].title.c_str(),
                    g_menu.landName.c_str());
        {   std::map<std::string,int> seen;
            std::map<std::string,int> mx;
            for (const maps::Decor &dd : g_menu.terr.decor)
                if (int(dd.frame) > mx[dd.name]) mx[dd.name] = int(dd.frame);
            for (const maps::Decor &dd : g_menu.terr.decor) {
                if (seen.count(dd.name)) continue;
                const spr::Strip *ds = g_menu.landSet.strip(dd.name);
                const spr::Frame *df = ds && ds->count() ? ds->at(0) : nullptr;
                seen[dd.name] = 1;
                if (df) std::printf("dekoracja %-10s klatek %2d, frame do %3d, %s\n",
                                    dd.name.c_str(), int(ds->count()), mx[dd.name],
                                    mx[dd.name] == 255 ? "ANIMACJA" : "warianty");
            }
        }
        {   int two = 0, one = 0;
            for (int yy = 0; yy < g_menu.terr.bh; ++yy)
                for (int xx = 0; xx < g_menu.terr.bw; ++xx)
                    for (int lv = 0; lv < maps::LEVELS; ++lv) {
                        if (!g_menu.terr.texAt(xx, yy, lv)) continue;
                        if (g_menu.terr.tex2At(xx, yy, lv)) ++two; else ++one;
                    }
            std::printf("tekstury: %d blokow jednowarstwowych, %d z nakladka\n", one, two);
        }
        {   int mn = 1 << 30, mx = 0, brak = 0, tot = 0;
            std::map<int,int> hist;
            for (int yy = 0; yy < g_menu.terr.bh; ++yy)
                for (int xx = 0; xx < g_menu.terr.bw; ++xx)
                    for (int lv = 0; lv < maps::LEVELS; ++lv) {
                        uint16_t mnum = g_menu.terr.meshAt(xx, yy, lv);
                        if (!mnum) continue;
                        ++tot;
                        if (mnum < mn) mn = mnum;
                        if (mnum > mx) mx = mnum;
                        if (!g_menu.landSet.mesh(mnum)) { ++brak; hist[mnum]++; }
                    }
            std::printf("siatki: %d uzyc, numery %d..%d, bez grafiki %d\n",
                        tot, mn == (1 << 30) ? 0 : mn, mx, brak);
            int shown = 0;
            for (const auto &e : hist) {
                if (shown++ >= 6) break;
                std::printf("   brak siatki %d (%d razy)\n", e.first, e.second);
            }
        }
        {   int nres = 0, okres = 0;
            for (const maps::Object &o : g_menu.terr.objects) {
                if (o.type != maps::OBJ_RESOURCE) continue;
                ++nres;
                if (g_menu.ResPiece(o.subtype, 0)) ++okres;
            }
            std::printf("zloza: %d na mapie, %d z grafika\n", nres, okres);
        }
        {   int solid = 0, lit = 0, mid = 0;
            for (int yy = 0; yy < g_menu.terr.bh; ++yy)
                for (int xx = 0; xx < g_menu.terr.bw; ++xx) {
                    float cc[4];
                    g_menu.FogCorners(xx, yy, cc);
                    float mx = cc[0];
                    for (int q2 = 1; q2 < 4; ++q2) if (cc[q2] > mx) mx = cc[q2];
                    if (mx > 0.995f) ++solid; else if (mx <= 0.004f) ++lit; else ++mid;
                }
            std::printf("pole mgly: czarnych %d, jasnych %d, przejsciowych %d, naroza %d\n",
                        solid, lit, mid, int(g_menu.fogC.size()));
        }
        g_menu.PickCursor();
        g_menu.ringStep = 6;
        g_menu.texBlocks = g_menu.flatBlocks = 0;
        g_menu.meshBlocks = g_menu.diamondBlocks = 0;
        g_menu.Compose();
        {   // Kazdy budynek ma dostac bryle i barwe SWOJEJ rasy. Przeszukiwanie
            // tablicy po kolei zwracalo zawsze White Sharks, wiec budynki
            // Black Octopi wygladaly jak cudze.
            int swoja = 0, cudza = 0, bezBarwy = 0;
            for (const Menu::Bld &b : g_menu.blds) {
                int side = g_menu.SideOf(b.owner);
                if (g_menu.BldStripFor(b.tobj, side)) ++swoja;
                else if (g_menu.BldStrip(b.tobj, side)) ++cudza;
                const char *n = units::buildingName(int(b.tobj), side);
                char rec[64];
                bool col = false;
                if (n) {
                    std::snprintf(rec, sizeof(rec), "_%s_id_%u_", n, b.owner & 7);
                    col = g_menu.unitSet.object(rec) != nullptr;
                }
                if (!col) ++bezBarwy;
            }
            // Ile z nich rysowaloby sie wczesniej grafika obcej rasy: tyle,
            // ile ma inny rekord dla White Sharks niz dla wlasciciela.
            int mylone = 0;
            for (const Menu::Bld &b : g_menu.blds) {
                int side = g_menu.SideOf(b.owner);
                if (side == 0) continue;
                const char *mine = units::buildingName(int(b.tobj), side);
                const char *ws = units::buildingName(int(b.tobj), 0);
                if (mine && ws && std::strcmp(mine, ws) != 0) ++mylone;
            }
        {   const spr::Strip *a = g_menu.LifeStrip(false);
            const spr::Strip *b = g_menu.LifeStrip(true);
            const spr::Strip *r = g_menu.unitSet.misc("actfr1s0o");
            std::printf("pasek zycia: maly %s (%d klatek), duzy %s (%d), pierscien %s (%d)\n",
                        a ? "jest" : "BRAK", a ? int(a->count()) : 0,
                        b ? "jest" : "BRAK", b ? int(b->count()) : 0,
                        r ? "jest" : "BRAK", r ? int(r->count()) : 0);
        }
        {   // ile typow budynkow ma wlasny pasek ruchomych czesci
            int zAni = 0, wszystkich = 0;
            for (int t = 50; t <= 115; ++t)
                for (int sd = 0; sd < 3; ++sd) {
                    const char *nm = units::buildingName(t, sd);
                    if (!nm) continue;
                    ++wszystkich;
                    char rec[64];
                    std::snprintf(rec, sizeof(rec), "_%s_ani_", nm);
                    if (g_menu.unitSet.object(rec)) ++zAni;
                }
            int ok2 = 0, zle = 0;
            std::string braki;
            for (int t = 50; t <= 115; ++t)
                for (int side = 0; side < 3; ++side) {
                    const char *n = units::buildingName(t, side);
                    if (!n) continue;
                    Menu::Bld probe;
                    probe.tobj = uint32_t(t);
                    probe.owner = 1;
                    const spr::Frame *tf = nullptr;
                    if (g_menu.BldTintFrame(probe, n, tf)) ++ok2;
                    else { ++zle; braki += n; braki += ' '; }
                }
        {   // Ktore budynki maja animacje i ktore chodza CIAGLE.
            int maja = 0, ciagle = 0, zdarzeniem = 0, brakPaska = 0;
            for (int t = 50; t <= 89; ++t) {
                bool any = false;
                for (int side = 0; side < 3; ++side) {
                    const char *a = units::buildingAni(t, side);
                    if (!a) continue;
                    any = true;
                    Menu::Bld probe;
                    probe.tobj = uint32_t(t);
                    probe.owner = uint32_t(side + 1);
                    if (!g_menu.BldAniStrip(probe)) ++brakPaska;
                    break;
                }
                if (!any) continue;
                ++maja;
                if (units::buildingAniLoops(t)) ++ciagle; else ++zdarzeniem;
            }
        std::printf("ikony palety: z CONTROLG %d, zastepczych %d\n",
                    g_menu.iconsFromHud, g_menu.iconsFallback);
            std::printf("animacje budynkow: %d typow ma nakladke, %d chodzi w petli, %d statyczna (klatka 0), bez paska %d\n",
                        maja, ciagle, zdarzeniem, brakPaska);
        }
            std::printf("nakladki barwy: %d wpisow ma, %d nie ma%s%s\n",
                        ok2, zle, zle ? "  -> " : "", braki.c_str());
        }
        std::printf("barwa budynkow w klatce: nalozona %d, bez nakladki %d\n",
                    g_menu.bldTinted, g_menu.bldNoTint);
            std::printf("grafika budynkow: wlasna rasa %d, zastepcza %d, "
                        "bez nakladki barwy %d, wczesniej mylonych %d\n", swoja, cudza, bezBarwy, mylone);
        }
        std::printf("cien: %d px narysowanych, %d odrzuconych (siatka %d, romb %d)\n",
                    g_menu.shadowPx, g_menu.shadowCut,
                    g_menu.meshBlocks, g_menu.diamondBlocks);

        // ---- ktora tablica i czy dochodzi na plotno ----
        //
        // **Wycofane: PLT_SHAD30.** Renderer swiata dostaje w `StartGame`
        // tablice **PLT_SHAD40** razem z liczba stopni (`+0x278 = 0x10`,
        // `+0x27c = DAT_008032c0`), a `DAT_008032c0` to wprost
        // `mfPltPtrTy(..., "PLT_SHAD40", ...)` z `LoadGamePlt`. Szesnastka
        // zgadza sie z archiwum co do liczby: SHAD40 ma 16 stopni, a SHAD30
        // i SHAD60 po jednym. Cien remake'u byl o dziesiec punktow za jasny.
        //
        // Sam wspolczynnik **nie jest wpisany z reki** - liczy sie z tablicy,
        // wiec ten wiersz mierzy odczyt, a nie stala.
        {
            int stopni30 = 0, stopni40 = 0, stopni60 = 0, stopniFog = 0;
            g_menu.landSet.remap("PLT_SHAD30", 0, stopni30);
            g_menu.landSet.remap("PLT_SHAD40", 0, stopni40);
            g_menu.landSet.remap("PLT_SHAD60", 0, stopni60);
            g_menu.landSet.remap("PLT_FOG", 0, stopniFog);
            std::printf("  tablice: SHAD30 %d st., SHAD40 %d st., SHAD60 %d st.,"
                        " FOG %d st.; cien bierze SHAD40 -> %d %%\n",
                        stopni30, stopni40, stopni60, stopniFog,
                        g_menu.ShadowPct());

            // **Skutek na plotnie**, nie sama stala: ten sam piksel terenu
            // z cieniem i bez. Sam odczyt wspolczynnika nie dowodzilby, ze
            // dochodzi do rysowania - to ta sama lekcja, co przy wrakach.
            g_menu.screen = SCR_TERRAIN;
            size_t byloU = g_menu.units.size();
            // **Para**: ta sama scena z cieniem i bez - to jedyny sposob,
            // zeby pokazac, ze przelacznik CIENIE cokolwiek robi. Sam
            // licznik pikseli nie wystarczy, bo rosnie tak czy tak.
            bool bylyCienie = g_menu.shadowsOn;
            g_menu.shadowsOn = false;
            g_menu.shadowPx = 0;
            g_menu.Compose();
            int bezPx = g_menu.shadowPx;
            g_menu.shadowsOn = true;
            g_menu.shadowPx = 0;
            g_menu.shadowRatio = 0;
            g_menu.shadowRatioN = 0;
            g_menu.Compose();
            int zPx = g_menu.shadowPx;
            double sr = g_menu.shadowRatioN
                      ? g_menu.shadowRatio / double(g_menu.shadowRatioN) : 0.0;
            std::printf("  przelacznik CIENIE: bez %d px, z %d px %s\n",
                        bezPx, zPx,
                        (bezPx == 0 && zPx > 0) ? "-> dziala"
                                                : "-> MARTWY ALBO NIC NIE RYSUJE");
            std::printf("  przyciemnienie w chwili zapisu: %.3f na %ld pikseli %s\n",
                        sr, g_menu.shadowRatioN,
                        (g_menu.shadowRatioN > 0 && sr > 0.55 && sr < 0.65)
                            ? "-> zgodne z PLT_SHAD40" : "-> NIE ZGADZA SIE");
            g_menu.shadowsOn = bylyCienie;
            g_menu.units.resize(byloU);
        }
        {   // Mapa jest rombem, a widok prostokatem - w naroznikach zawsze
            // bedzie widac troche pustki. Mierzymy ILE, po dojechaniu do
            // czterech skrajnych polozen kamery. Przed ograniczeniem do
            // rombu potrafilo to byc pol ekranu.
            int c0 = g_menu.camX, c1 = g_menu.camY, gorsze = 0;
            RECT vp = g_menu.Viewport();
            static const int kx[4] = { -999999, 999999, -999999, 999999 };
            static const int ky[4] = { -999999, -999999, 999999, 999999 };
            for (int k = 0; k < 4; ++k) {
                g_menu.camX = kx[k];
                g_menu.camY = ky[k];
                g_menu.ClampCamera();
                g_menu.Compose();
                int pusto = 0;
                for (int y = vp.top; y < vp.bottom; ++y)
                    for (int x = vp.left; x < vp.right; ++x)
                        if (!(g_menu.canvas[size_t(y) * size_t(g_clientW)
                                            + size_t(x)] & 0xFFFFFF)) ++pusto;
                if (pusto > gorsze) gorsze = pusto;
            }
            // Ograniczenie nie moze odciac dostepu do mapy - kazdy skrajny
            // blok musi dac sie pokazac w widoku.
            int nieosiagalne = 0;
            const int cw = g_menu.terr.bw - 1, chh = g_menu.terr.bh - 1;
            const int rx[4] = { 0, cw, 0, cw }, ry[4] = { 0, 0, chh, chh };
            for (int k = 0; k < 4; ++k) {
                int sx3, sy3;
                g_menu.CellToScreen(float(rx[k] * 2), float(ry[k] * 2), sx3, sy3);
                g_menu.camX += sx3 - (vp.left + vp.right) / 2;
                g_menu.camY += sy3 - (vp.top + vp.bottom) / 2;
                g_menu.ClampCamera();
                g_menu.CellToScreen(float(rx[k] * 2), float(ry[k] * 2), sx3, sy3);
                if (sx3 < vp.left || sx3 >= vp.right ||
                    sy3 < vp.top || sy3 >= vp.bottom) ++nieosiagalne;
            }
            int pole = (vp.right - vp.left) * (vp.bottom - vp.top);
            std::printf("poza mapa: najgorszy naroznik %d%% widoku, rogow mapy nieosiagalnych %d z 4\n",
                        pole > 0 ? gorsze * 100 / pole : 0, nieosiagalne);
            g_menu.camX = c0;
            g_menu.camY = c1;
            g_menu.Compose();
        }
        {   // Ile obiektow szloby w zlej kolejnosci przy dawnym porzadku
            // tablicowym: najpierw wszystkie budynki, potem lodzie.
            int zle = 0;
            std::vector<std::tuple<int,int,int>> stare;
            for (const Menu::Bld &b : g_menu.blds) {
                int lv = g_menu.terr.topAt(b.x / 2, b.y / 2);
                stare.push_back({ Menu::CellBand(b.x, b.y), lv > 0 ? lv : 0,
                                  b.y / 2 });
            }
            for (const Menu::Unit &u : g_menu.units) {
                int lv = int(u.z);
                stare.push_back({ Menu::CellBand(u.x, u.y), lv > 0 ? lv : 0,
                                  int(u.y) / 2 });
            }
            for (size_t a = 1; a < stare.size(); ++a)
                if (stare[a] < stare[a - 1]) ++zle;
            std::printf("kolejnosc rysowania: %d obiektow, w tablicy zle ustawionych %d, po sortowaniu 0\n",
                        int(stare.size()), zle);

            {   // Zloza i dekoracje sortuja sie razem z lodziami. Wczesniej
                // szly PRZED wszystkim, w kolejnosci tablicy mapy, wiec lodz
                // byla nad zlozem takze wtedy, gdy stala za nim. Sprawdzamy,
                // ze przeplot faktycznie zachodzi: ile obiektow dna wypada po
                // jakiejs lodzi i odwrotnie.
                int gruntPoLodzi = 0, lodzPoGruncie = 0;
                bool bylaLodz = false, bylGrunt = false;
                for (const Menu::DrawItem &d : g_menu.drawList) {
                    if (d.kind == Menu::DR_UNIT) {
                        bylaLodz = true;
                        if (bylGrunt) ++lodzPoGruncie;
                    } else if (d.kind == Menu::DR_GROUND) {
                        bylGrunt = true;
                        if (bylaLodz) ++gruntPoLodzi;
                    }
                }
                std::printf("  przeplot dna z lodziami: %d obiektow dna po lodzi, "
                            "%d lodzi po dnie\n", gruntPoLodzi, lodzPoGruncie);
            }

            // Stabilnosc w CZASIE. Gra trzyma na slocie numer z poprzedniej
            // klatki (`slot+0xc`) i uzywa go jako remisu - `BuildDrawList`
            // wpisuje go PO sortowaniu. Dzieki temu obiekty o identycznym
            // kluczu nie zamieniaja sie wierzchem miedzy klatkami, niezaleznie
            // od tego, co dzieje sie z tablica.
            //
            // Test: trzy lodzie w JEDNEJ komorce - wtedy pasmo, poziom, wiersz
            // i rodzaj sa rowne i rozstrzyga wylacznie remis. Potem odwracamy
            // tablice i patrzymy, czy kolejnosc rysowania sie utrzymala.
            // (`Reap()` sam kolejnosci nie psuje, bo zageszcza zachowujac ja -
            // ten mechanizm broni przed KAZDYM przenumerowaniem, nie tylko nim.)
            if (!g_menu.units.empty()) {
                Menu::Unit wz = g_menu.units[0];
                for (int q = 0; q < 3; ++q) {
                    Menu::Unit u = wz;
                    u.x = wz.x; u.y = wz.y;
                    u.drawSeq = 0;
                    u.tx = float(1000 + q);     // znacznik tozsamosci klonu
                    g_menu.units.push_back(u);
                }
                g_menu.sel.clear();
                g_menu.Compose();
                auto klony = [&]() {
                    std::vector<int> v;
                    for (const Menu::DrawItem &d : g_menu.drawList)
                        if (d.kind == 1 && g_menu.units[size_t(d.idx)].tx >= 1000.0f)
                            v.push_back(int(g_menu.units[size_t(d.idx)].tx) - 1000);
                    return v;
                };
                std::vector<int> przed = klony();
                std::reverse(g_menu.units.begin(), g_menu.units.end());
                g_menu.Compose();
                std::vector<int> po = klony();
                int zamian = 0;
                for (size_t q = 0; q < po.size() && q < przed.size(); ++q)
                    if (po[q] != przed[q]) ++zamian;
                std::printf("  stabilnosc po przenumerowaniu: %d z %d klonow "
                            "zmienilo wierzch\n", zamian, int(po.size()));
                // Klony won ze sceny - inaczej brudza zrzut, po ktorym liczy
                // sie md5 porownywane miedzy backendami.
                g_menu.units.erase(
                    std::remove_if(g_menu.units.begin(), g_menu.units.end(),
                                   [](const Menu::Unit &u) { return u.tx >= 1000.0f; }),
                    g_menu.units.end());
                std::reverse(g_menu.units.begin(), g_menu.units.end());
                g_menu.sel.clear();
                g_menu.Compose();
            }
        }
        std::printf("przyciete przez teren: %d obiektow, %d pikseli\n",
                    g_menu.silObjs, g_menu.silPixels);
        std::printf("bloki w klatce: z tekstura %d, plaskim kolorem %d\n",
                    g_menu.texBlocks, g_menu.flatBlocks);
        std::printf("  z tego siatka 3D %d, plaskim rombem %d\n",
                    g_menu.meshBlocks, g_menu.diamondBlocks);

        // ---- MASKA TROJKATOW: ktory z czterech wariantow ----
        //
        // `FUN_006dd050` ustawia `+0xac = 1 << orientacja` (0x01..0x08),
        // a rasteryzer rysuje trojkat tylko gdy `maska & flagi`. Kazdy mesh
        // niesie wiec cztery zestawy trojkatow, po jednym na obrot widoku.
        // Ktory pasuje do kamery remake'u, rozstrzyga pomiar: dobra maska
        // ma pokryc blok **bez dziur** i z najmniejszym nadmiarem.
        {
            uint8_t byla = g_menu.terrTriMask;
            static const uint8_t kM[6] = { 0, 0x01, 0x02, 0x04, 0x08, 0x10 };
            for (int k = 0; k < 6; ++k) {
                g_menu.terrTriMask = kM[k];
                g_menu.overBuf.assign(g_menu.depthBuf.size(), 0);
                g_menu.overWho.assign(g_menu.depthBuf.size(), 0);
                g_menu.overOn = true;
                g_menu.overTwice = g_menu.overTotal = 0;
                g_menu.Compose();
                g_menu.overOn = false;
                long dziur = 0, wnetrze = 0;
                RECT vp4 = g_menu.Viewport();
                for (int y = vp4.top; y < vp4.bottom && y < SCREEN_H; ++y) {
                    size_t row = size_t(y) * size_t(SCREEN_W);
                    int lo = -1, hi = -1;
                    for (int x = vp4.left; x < vp4.right && x < SCREEN_W; ++x)
                        if (g_menu.depthBuf[row + size_t(x)]) { if (lo < 0) lo = x; hi = x; }
                    if (lo < 0) continue;
                    for (int x = lo; x <= hi; ++x) {
                        ++wnetrze;
                        if (!g_menu.depthBuf[row + size_t(x)]) ++dziur;
                    }
                }
                std::printf("  maska 0x%02x: zapisow %ld, dwa razy %ld (%.1f %%),"
                            " dziur %ld\n",
                            kM[k], g_menu.overTotal, g_menu.overTwice,
                            g_menu.overTotal
                                ? 100.0 * double(g_menu.overTwice) / double(g_menu.overTotal)
                                : 0.0,
                            dziur);
            }
            g_menu.terrTriMask = byla;
        }

        // ---- SZWY: dziury na stykach blokow ----
        //
        // **Uwaga: ten licznik liczy tez NIEBO.** Bierze dla kazdego wiersza
        // zakres od pierwszego do ostatniego stempla terenu i pyta o dziury
        // miedzy nimi - a przy grani wiersz zaczyna sie na jednym szczycie
        // i konczy na drugim, wiec siodlo miedzy nimi wypada jako „dziura".
        // Na mapie 0 wychodzi z tego 3628 pikseli w 41 wierszach, najdluzszy
        // ciag 176 - i wszystkie leza w czarnym siodle nad grzbietem, nie
        // w terenie. Wiersz zaczyna od tego podawac **polozenie** trzech
        // pierwszych, zeby nie trzeba bylo tego szukac drugi raz.
        //
        // `depthBuf` dostaje stempel przy KAZDYM zapisanym pikselu terenu,
        // wiec zero w srodku obszaru, ktory teren wypelnia, znaczy: zaden
        // trojkat tego piksela nie pokryl. To sa te ciemne kreski na styku
        // blokow - tlo przebijajace przez szew.
        //
        // Liczone tylko **wewnatrz** obszaru zamalowanego terenem: dla kazdego
        // wiersza bierzemy zakres od pierwszego do ostatniego stempla i pytamy
        // o dziury miedzy nimi. Inaczej policzylibysmy cala wode dookola mapy.
        // ---- NADMIAROWE ZAPISY: piksel malowany dwa razy ----
        //
        // Dziury to nie jest ten artefakt (ponizej wychodzi 0,01 %). Widoczne
        // sa **kontury blokow**, a te biora sie z drugiej strony: piksel
        // dokladnie na wspolnej krawedzi dwoch trojkatow spelnia `w >= 0`
        // w obu i jest malowany dwa razy. Gdy trojkaty naleza do roznych
        // blokow, wygrywa rysowany pozniej.
        {
            g_menu.overBuf.assign(g_menu.depthBuf.size(), 0);
            g_menu.overWho.assign(g_menu.depthBuf.size(), 0);
            g_menu.overOn = true;
            g_menu.overTwice = g_menu.overTotal = g_menu.overSameBand = 0;
            g_menu.overCross = 0;
            g_menu.Compose();
            g_menu.overOn = false;
            std::printf("  zapisy terenu: %ld pikseli, z tego %ld pomalowanych"
                        " po raz drugi (%.2f %%); to samo pasmo %ld, MIEDZY BLOKAMI %ld\n",
                        g_menu.overTotal, g_menu.overTwice,
                        g_menu.overTotal
                            ? 100.0 * double(g_menu.overTwice) / double(g_menu.overTotal)
                            : 0.0,
                        g_menu.overSameBand, g_menu.overCross);
        }
        std::printf("  dekoracje: w mapie %d, mgla %d, poza ekranem %d,"
                    " bez paska %d, bez klatki %d; zakrytych %d, widocznych %d,"
                    " przycietych pikseli %ld\n",
                    int(g_menu.terr.decor.size()), g_menu.decMgla,
                    g_menu.decPozaEkr, g_menu.decBezPaska, g_menu.decBezKlatki,
                    g_menu.decZaslon, g_menu.decWidoczne, g_menu.decCut);
        std::printf("  na plaskim (7x7 blokow na tym samym poziomie): kepek %ld,"
                    " przycietych %ld, pikseli %ld%s\n",
                    g_menu.decPlaskich, g_menu.decPlaskoCiete,
                    g_menu.decPlaskoPix,
                    g_menu.decPlaskoCiete ? "  <- NIE MA CO ZASLANIAC" : "");
        // ---- SZEW: czy widac kontury blokow ----
        //
        // Para sasiednich pikseli terenu albo nalezy do jednego bloku, albo
        // do dwoch. Jesli kontury widac, to srednia |roznica jasnosci| przez
        // granice jest wyraznie wieksza niz wewnatrz bloku. Sama liczba nic
        // nie znaczy - znaczy jej **stosunek** i to, jak sie zmienia.
        {
            g_menu.overBuf.assign(g_menu.depthBuf.size(), 0);
            g_menu.overWho.assign(g_menu.depthBuf.size(), 0);
            g_menu.overZ.assign(g_menu.depthBuf.size(), -1e30f);
            g_menu.overBackOverFront = g_menu.overBackOverFrontAll = 0;
            g_menu.overOn = true;
            g_menu.Compose();
            g_menu.overOn = false;
            // **Ten licznik mierzy artefakt wprost.** Piksel malowany dwa
            // razy jest w porzadku, dopoki blizszy maluje po dalszym; zle
            // jest odwrotnie - i to sa te kwadratowe fasety na zboczu.
            std::printf("  kolejnosc: dalszy zamalowal blizszego na %ld"
                        " pikselach (w tym samym kaflu %ld)\n",
                        g_menu.overBackOverFrontAll, g_menu.overBackOverFront);

            auto luma = [](uint32_t c) {
                return (int((c >> 16) & 0xff) * 77 + int((c >> 8) & 0xff) * 151
                        + int(c & 0xff) * 28) >> 8;
            };
            double sumW = 0, sumP = 0;
            long nW = 0, nP = 0, mocne = 0;
            double sumX = 0, sumY = 0; long nXY = 0;
            // najgorsza para blokow
            std::map<std::pair<uint32_t, uint32_t>, std::pair<long, double> > wg;
            RECT vs = g_menu.Viewport();
            for (int y = vs.top; y + 1 < vs.bottom && y + 1 < SCREEN_H; ++y)
            for (int x = vs.left; x + 1 < vs.right && x + 1 < SCREEN_W; ++x) {
                size_t a = size_t(y) * size_t(SCREEN_W) + size_t(x);
                const size_t sasiad[2] = { a + 1, a + size_t(SCREEN_W) };
                for (int k = 0; k < 2; ++k) {
                    size_t b = sasiad[k];
                    if (!g_menu.overWho[a] || !g_menu.overWho[b]) continue;
                    double d = std::abs(luma(g_menu.overPix[a])
                                        - luma(g_menu.overPix[b]));
                    if (g_menu.overWho[a] == g_menu.overWho[b]) {
                        sumW += d; ++nW;
                    } else {
                        // **Tylko sasiedzi na tym samym poziomie.** Para
                        // [poziom 0] kontra [poziom 4] to sylwetka skarpy -
                        // tam twarda krawedz jest poprawna i zalewa pomiar.
                        uint32_t u = g_menu.overWho[a] - 1;
                        uint32_t v = g_menu.overWho[b] - 1;
                        int lu = int(u & 7), lv = int(v & 7);
                        u >>= 3; v >>= 3;
                        int ux = int(u % uint32_t(g_menu.terr.bw));
                        int uy = int(u / uint32_t(g_menu.terr.bw));
                        int vx = int(v % uint32_t(g_menu.terr.bw));
                        int vy = int(v / uint32_t(g_menu.terr.bw));
                        if (lu != lv
                            || std::abs(ux - vx) + std::abs(uy - vy) != 1) continue;
                        sumP += d; ++nP;
                        if (d >= 24) ++mocne;
                        uint32_t ka = g_menu.overWho[a], kb = g_menu.overWho[b];
                        if (ka > kb) std::swap(ka, kb);
                        auto &e = wg[std::make_pair(ka, kb)];
                        e.first += 1; e.second += d;
                        if (d >= 24) { sumX += x; sumY += y; ++nXY; }
                    }
                }
            }
            // **Mapa szwow do pliku.** Sam teren, a na nim na czerwono
            // piksele, gdzie granica bloku daje kontrast >= 24. Liczba
            // mowi ile, obrazek mowi GDZIE - i to drugie jest tu wazniejsze.
            {
                std::vector<uint32_t> obraz = g_menu.overPix;
                for (int y = vs.top; y + 1 < vs.bottom && y + 1 < SCREEN_H; ++y)
                for (int x = vs.left; x + 1 < vs.right && x + 1 < SCREEN_W; ++x) {
                    size_t a = size_t(y) * size_t(SCREEN_W) + size_t(x);
                    const size_t ss[2] = { a + 1, a + size_t(SCREEN_W) };
                    for (int k = 0; k < 2; ++k) {
                        size_t b = ss[k];
                        if (!g_menu.overWho[a] || !g_menu.overWho[b]) continue;
                        if (g_menu.overWho[a] == g_menu.overWho[b]) continue;
                        double d = std::abs(luma(g_menu.overPix[a])
                                            - luma(g_menu.overPix[b]));
                        if (d >= 24) { obraz[a] = 0xFF2020; obraz[b] = 0xFF2020; }
                    }
                }
                FILE *sw = std::fopen("seam.raw", "wb");
                if (sw) {
                    std::fwrite(obraz.data(), 4, obraz.size(), sw);
                    std::fclose(sw);
                    std::printf("  mapa szwow: seam.raw %dx%d\n",
                                SCREEN_W, SCREEN_H);
                }
            }
            double mW = nW ? sumW / double(nW) : 0.0;
            double mP = nP ? sumP / double(nP) : 0.0;
            std::printf("  szew blokow: wewnatrz %.2f (%ld par), sasiedzi na tym samym poziomie"
                        " %.2f (%ld par) -> %.2fx, mocnych (>=24) %ld\n",
                        mW, nW, mP, nP, mW > 0 ? mP / mW : 0.0, mocne);

            if (nXY) std::printf("  srodek ciezkosci mocnych szwow: x=%.0f y=%.0f"
                                 " (%ld pikseli)\n",
                                 sumX / double(nXY), sumY / double(nXY), nXY);
            // Trzy najgorsze pary blokow, z danymi mapy - zeby bylo wiadomo,
            // CZYM te bloki sie roznia, a nie tylko ze sie roznia.
            std::vector<std::pair<double, std::pair<uint32_t, uint32_t> > > top;
            for (auto &e : wg)
                if (e.second.first >= 20)
                    top.push_back(std::make_pair(e.second.second, e.first));
            std::sort(top.begin(), top.end());
            int pokaz = 0;
            for (int i = int(top.size()) - 1; i >= 0 && pokaz < 3; --i, ++pokaz) {
                uint32_t id[2] = { top[size_t(i)].second.first,
                                   top[size_t(i)].second.second };
                std::printf("    para %d:", pokaz);
                for (int k = 0; k < 2; ++k) {
                    uint32_t q = id[k] - 1;
                    int lvl = int(q & 7); q >>= 3;
                    int bx = int(q % uint32_t(g_menu.terr.bw));
                    int by = int(q / uint32_t(g_menu.terr.bw));
                    std::printf("  [%d,%d lvl %d top %d tex %u/%u mesh %u]",
                                bx, by, lvl, g_menu.terr.topAt(bx, by),
                                unsigned(g_menu.terr.texAt(bx, by, lvl)),
                                unsigned(g_menu.terr.tex2At(bx, by, lvl)),
                                unsigned(g_menu.terr.meshAt(bx, by, lvl)));
                }
                std::printf("  suma %.0f\n", top[size_t(i)].first);
            }
        }
        {
            long dziur = 0, wnetrze = 0, wierszyZDziura = 0;
            long najdluzsza = 0;
            RECT vp3 = g_menu.Viewport();
            for (int y = vp3.top; y < vp3.bottom && y < SCREEN_H; ++y) {
                size_t row = size_t(y) * size_t(SCREEN_W);
                int lo = -1, hi = -1;
                for (int x = vp3.left; x < vp3.right && x < SCREEN_W; ++x)
                    if (g_menu.depthBuf[row + size_t(x)]) { if (lo < 0) lo = x; hi = x; }
                if (lo < 0) continue;
                long biezaca = 0;
                bool byla = false;
                for (int x = lo; x <= hi; ++x) {
                    ++wnetrze;
                    if (g_menu.depthBuf[row + size_t(x)]) {
                        if (biezaca > najdluzsza) najdluzsza = biezaca;
                        biezaca = 0;
                        continue;
                    }
                    ++dziur;
                    ++biezaca;
                    byla = true;
                }
                if (biezaca > najdluzsza) najdluzsza = biezaca;
                if (byla) {
                    ++wierszyZDziura;
                    if (wierszyZDziura <= 3) {
                        // **Gdzie** ta dziura jest - sama liczba nie mowi,
                        // czy to szum na krawedzi, czy wyrwa w zboczu.
                        int a = -1, b = -1;
                        for (int x = lo; x <= hi; ++x)
                            if (!g_menu.depthBuf[row + size_t(x)]) {
                                if (a < 0) a = x;
                                b = x;
                            }
                        std::printf("    dziura w wierszu %d: x %d..%d"
                                    " (wiersz terenu %d..%d)\n",
                                    y, a, b, lo, hi);
                    }
                }
            }
            std::printf("  szwy terenu: dziur %ld na %ld pikseli wnetrza"
                        " (%.3f %%), wierszy z dziura %ld, najdluzsza %ld\n",
                        dziur, wnetrze,
                        wnetrze ? 100.0 * double(dziur) / double(wnetrze) : 0.0,
                        wierszyZDziura, najdluzsza);
        }
        FILE *o = std::fopen("panel.raw", "wb");
        if (o) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        int moving = 0;
        for (const Menu::Unit &u : g_menu.units) if (u.moving) ++moving;
        {   // dzwieki i marker
            int okS = 0, okO = 0;
            for (int t = 1; t <= units::UNIT_COUNT; ++t) {
                if (!g_menu.sfx.resolve(Menu::sfxSelect(t)).empty()) ++okS;
                if (!g_menu.sfx.resolve(Menu::sfxOrder(t)).empty())  ++okO;
            }
            {   int pn = 0; bool pu = false;
                const int *pl = g_menu.Palette(pn, pu);
                std::printf("  bank: kor %d met %d, paleta %s %d pozycji\n",
                            g_menu.Me().bank.corium, g_menu.Me().bank.metal,
                            pl ? (pu ? "lodzi" : "budynkow") : "brak", pn);
                if (pl && pn > 0) {
                    cost::Price pr = g_menu.PriceOf(pl[0], pu);
                    std::printf("  pierwsza: tobj %d, kor %d met %d, %d s\n",
                                pl[0], pr.corium, pr.metal, pr.secs);
                }
            }
            std::printf("  budynkow: %d (wiezyczek %d), zaznaczonych %d\n",
                        int(g_menu.blds.size()),
                        int(std::count_if(g_menu.blds.begin(), g_menu.blds.end(),
                                          [](const Menu::Bld &q){ return q.turret; })),
                        int(g_menu.selBld.size()));
            std::printf("  dzwieki: zaznaczenie %d/40, rozkaz %d/40", okS, okO);
            int moved = 0;
            std::vector<std::pair<float,float>> was;
            for (const Menu::Fish &q : g_menu.fish) was.push_back({ q.x, q.y });
            for (int k = 0; k < 60; ++k) g_menu.StepFish(0.1f);
            for (size_t k = 0; k < g_menu.fish.size(); ++k) {
                float dx = g_menu.fish[k].x - was[k].first, dy = g_menu.fish[k].y - was[k].second;
                if (dx*dx + dy*dy > 0.25f) ++moved;
            }
            std::printf(", stworzen %d, ruszylo sie %d", int(g_menu.fish.size()), moved);
            const spr::Strip *mk = g_menu.unitSet.actFrame('s', g_menu.RingZoom(), 's');
            std::printf(", pierscien %s (%d klatek), muzyka %s\n",
                        mk ? "jest" : "BRAK", mk ? mk->count() : 0,
                        g_menu.music.playing().empty() ? "cisza" : g_menu.music.playing().c_str());
        }
        {   // kolysanie: y na ekranie stojacej jednostki w kolejnych chwilach
            Menu::Unit &q = g_menu.units[g_menu.units.size() - 1];
            q.moving = false;
            for (int k = 0; k < 40; ++k) { t += 100; g_menu.StepUnits(t); }  // niech z sie ustali
            std::printf("  kolysanie (y ekranu, jednostka stoi):");
            for (int k = 0; k < 11; ++k) {
                int qx, qy;
                g_menu.UnitToScreen(q, qx, qy);
                std::printf(" %d", qy);
                t += 370; g_menu.StepUnits(t);
            }
            std::printf("   z=%.2f maxLevel=%d\n", q.z, q.maxLevel);
        }
        {   // Kilka wlasnych lodzi zaznaczonych, zeby bylo widac liste
            // w bocznych panelach i wskaznik glebokosci.
            g_menu.sel.clear();
            for (size_t k = 0; k < g_menu.units.size() && g_menu.sel.size() < 6; ++k)
                if ((g_menu.units[k].owner & 7) == uint32_t(g_menu.me & 7))
                    g_menu.sel.push_back(int(k));
            g_menu.Compose();
            Menu::BarLay LL;
            g_menu.BarLayout(&LL);
            std::printf("lista zaznaczonych: %d ikon, wybranych %d, "
                        "panele x %d i %d, y %d\n",
                        g_menu.rosterShown, int(g_menu.sel.size()),
                        LL.rosterL, LL.rosterR, LL.y);
            FILE *o = std::fopen("panel.raw", "wb");
            if (o) { std::fwrite(g_menu.canvas.data(), 4,
                                 g_menu.canvas.size(), o); std::fclose(o); }
        }
        std::printf("panel.raw %dx%d, jednostek %d, w ruchu %d, zaznaczonych %d\n",
                    g_clientW, g_clientH, int(g_menu.units.size()), moving,
                    int(g_menu.sel.size()));
        {   // **Obiekty mapy, ktorych remake dlugo nie widzial.** Rekord OBJ_
            // niesie KLASE obiektu, nie TOBJ: 140 to STSharkC, 230 STVolcanoC,
            // 430 STMineSetC. Sprawdzamy to skutkiem - rekin musi dostac swoj
            // pasek, wulkan wszystkie trzy, a mina wejsc do listy min.
            int rek = 0, wul = 0, min_ = 0, zNad = 0;
            for (const maps::Object &o : g_menu.terr.objects) {
                if (o.type == maps::OBJ_SHARK)   ++rek;
                if (o.type == maps::OBJ_VOLCANO) ++wul;
                if (o.type == maps::OBJ_MINE)    ++min_;
                if (maps::isCreature(o.type) && o.z > 0) ++zNad;
            }
            int zRys = 0;
            for (const Menu::Fish &q : g_menu.fish)
                if (g_menu.FishLevel(q) > 0) ++zRys;
            const spr::Strip *sh = g_menu.natSet.forKind(maps::KIND_SHARK);
            const spr::Strip *vb = g_menu.landSet.strip("expl_vol");
            const spr::Strip *vp = g_menu.landSet.strip("expl_vop");
            const spr::Strip *vu = g_menu.landSet.strip("expl_vob");
            std::printf("  obiekty mapy: rekiny %d (pasek shark1 %s, %d klatek)"
                        ", wulkany %d (%d/%d/%d klatek), miny z mapy %d\n",
                        rek, sh ? "jest" : "BRAK", sh ? sh->count() : 0, wul,
                        vb ? vb->count() : 0, vu ? vu->count() : 0,
                        vp ? vp->count() : 0, min_);
            std::printf("  poziom stworzen: w rekordach ponad dnem %d, "
                        "rysowanych ponad dnem %d\n", zNad, zRys);
            std::printf("  wraki po tej scenie: %d, miny %d\n",
                        int(g_menu.wrecks.size()), int(g_menu.mines.size()));
            if (wul > 0) {
                // Wybuch wypada za 20000..30000 tikow, czyli 800..1200 s.
                // Przewijamy sekunde po sekundzie i patrzymy, czy w ogole
                // przyszedl i czy trwa tyle, ile mowi exe: 31 klatek po
                // trzy tiki to 93 tiki, 3.7 s.
                int przed = g_menu.volcBlew;
                float dlugo = 0;
                bool bylo = false;
                for (int k = 0; k < 1300 && !bylo; ++k) {
                    g_menu.StepVolcanoes(1.0f);
                    if (g_menu.volcBlew > przed) bylo = true;
                }
                for (int k = 0; k < 200 && g_menu.volcs[0].erupt; ++k) {
                    g_menu.StepVolcanoes(0.05f);
                    dlugo += 0.05f;
                }
                std::printf("  wulkan: wybuch %s, trwal %.1f s, klatka ciala "
                            "%.0f, w spoczynku wraca do 0\n",
                            bylo ? "przyszedl" : "NIE PRZYSZEDL", dlugo,
                            double(g_menu.volcs[0].body));
            }
        }
        return 0;
    }

    // --music [utwor] wypisuje liste albo rozkodowuje jeden utwor do music.wav,
    // zeby dalo sie sprawdzic, czy dekoder ADPCM nie produkuje szumu.
    if (argc > 2 && std::strcmp(argv[2], "--music") == 0) {
        if (!g_menu.music.ok()) { std::printf("brak MUSIC\n"); return 1; }
        if (argc < 4) {
            std::printf("utworow w archiwum: %d\n", int(g_menu.music.count()));
            return 0;
        }
        mus::Track t;
        if (!mus::decode(g_menu.music.raw(argv[3]), t) || !t.ok()) {
            std::printf("%s: nie rozkodowalem\n", argv[3]);
            return 1;
        }
        double sum = 0; int clip = 0; long zc = 0; int16_t prev = 0;
        for (size_t i = 0; i < t.pcm.size(); ++i) {
            double v = t.pcm[i];
            sum += v * v;
            if (t.pcm[i] >= 32767 || t.pcm[i] <= -32768) ++clip;
            if ((prev < 0) != (t.pcm[i] < 0)) ++zc;
            prev = t.pcm[i];
        }
        double rms = std::sqrt(sum / double(t.pcm.size()));
        double secs = double(t.pcm.size()) / double(t.channels) / double(t.rate);
        std::printf("%s: %d Hz, %d kan., probek %d = %.1f s\n",
                    argv[3], t.rate, t.channels, int(t.pcm.size()), secs);
        std::printf("  RMS %.0f (%.1f%% skali), obcietych %d (%.3f%%), przejsc przez zero %ld (%.0f/s)\n",
                    rms, rms / 32768.0 * 100.0, clip, 100.0 * clip / double(t.pcm.size()),
                    zc, double(zc) / secs);
        FILE *o = std::fopen("music.wav", "wb");
        if (o) {
            uint32_t dlen = uint32_t(t.pcm.size() * 2), rlen = 36 + dlen;
            uint16_t ch = uint16_t(t.channels), bits = 16;
            uint32_t rate = uint32_t(t.rate), avg = rate * ch * 2;
            uint16_t align = uint16_t(ch * 2), one = 1, fmt = 16;
            std::fwrite("RIFF", 1, 4, o); std::fwrite(&rlen, 4, 1, o);
            std::fwrite("WAVEfmt ", 1, 8, o); std::fwrite(&fmt, 4, 1, o);
            std::fwrite(&one, 2, 1, o); std::fwrite(&ch, 2, 1, o);
            std::fwrite(&rate, 4, 1, o); std::fwrite(&avg, 4, 1, o);
            std::fwrite(&align, 2, 1, o); std::fwrite(&bits, 2, 1, o);
            std::fwrite("data", 1, 4, o); std::fwrite(&dlen, 4, 1, o);
            std::fwrite(t.pcm.data(), 2, t.pcm.size(), o);
            std::fclose(o);
            std::printf("  zapisano music.wav\n");
        }
        return 0;
    }

    // --cursor [nazwa] wypisuje liste kursorow albo uklada klatki jednego
    // w siatke, do obejrzenia.
    if (argc > 2 && std::strcmp(argv[2], "--cursor") == 0) {
        if (!g_menu.cursors.ok()) { std::printf("brak kursorow\n"); return 1; }
        {   // Punkt goracy kontra srodek kanwy. Remake zgadywal srodek dla
            // wszystkiego poza CUR_ARROW/CUR_MENU; exe podaje go jawnie przy
            // kazdym wolaniu CursorClassTy::SetImages (FUN_0054bf40).
            int n = 0;
            const curhot::Hot *tab = curhot::table(n);
            int brak = 0, zgodnych = 0, roznych = 0, najw = 0, grubych = 0;
            const char *gorszy = "";
            for (int i = 0; i < n; ++i) {
                const spr::Frame *f = g_menu.cursors.frame(tab[i].name, 0);
                if (!f || !f->ok()) { ++brak; continue; }
                int dx = tab[i].x - f->canvasW / 2;
                int dy = tab[i].y - f->canvasH / 2;
                int d = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
                if (d == 0) ++zgodnych;
                else {
                    ++roznych;
                    if (d > 2) ++grubych;
                    if (d > najw) { najw = d; gorszy = tab[i].name; }
                }
            }
            std::printf("punkt goracy: %d kursorow; srodek kanwy trafia w %d, "
                        "myli sie o 1-2 px w %d, o wiecej w %d "
                        "(najgorzej %s o %d px); brak grafiki %d\n",
                        n, zgodnych, roznych - grubych, grubych, gorszy, najw, brak);
            {
                int puste = 0;
                for (int i = 0; i < 256; ++i)
                    if (g_menu.cursors.colour(i) == 0) ++puste;
                std::printf("paleta kursorow: %d z 256 wpisow zerowych "
                            "(CURSOR_PAL ma 186 - jest zaslepka)\n", puste);
            }
            for (int i = 0; i < n && i < 6; ++i) {
                const spr::Frame *f = g_menu.cursors.frame(tab[i].name, 0);
                if (!f || !f->ok()) continue;
                std::printf("    %-16s kanwa %dx%d, grafika %dx%d w (%d,%d), "
                            "hot (%d,%d)\n",
                            tab[i].name, f->canvasW, f->canvasH, f->w, f->h,
                            f->x, f->y, tab[i].x, tab[i].y);
            }
        }
        static const char *kAll[] = {
            "CUR_ARROW", "CUR_CMD", "CUR_CONFIRM", "CUR_FIRE", "CUR_OWNBOAT",
            "CUR_OWNOBJ", "CUR_VIEW", "CUR_PATROL", "CUR_REPAIR", "CUR_NOBUILD",
            "CUR_CLOCK", "CUR_MENU", "CUR_SUP", "CUR_SDN", "CUR_SNO", "CUR_CAPTURE",
            "CUR_DEFENCE", "CUR_TELEPORT", "CUR_FORMATION", "CUR_LOADOBJ" };
        if (argc < 4) {
            for (size_t i = 0; i < sizeof(kAll) / sizeof(kAll[0]); ++i)
                std::printf("  %-16s %d klatek\n", kAll[i], g_menu.cursors.frames(kAll[i]));
            return 0;
        }
        const char *nm = argv[3];
        int n = g_menu.cursors.frames(nm);
        if (n <= 0) { std::printf("%s: brak\n", nm); return 1; }
        int cols = 8, rows = (n + cols - 1) / cols, cw = 90, ch = 80;
        g_clientW = cols * cw;
        g_clientH = rows * ch;
        g_menu.screen = SCR_TERRAIN;
        g_menu.FitCanvas();
        std::fill(g_menu.canvas.begin(), g_menu.canvas.end(), 0x102030u);
        int drawn = 0;
        for (int i = 0; i < n; ++i) {
            const spr::Frame *f = g_menu.cursors.frame(nm, i);
            if (!f || !f->ok()) continue;
            ++drawn;
            int cx = (i % cols) * cw + 12, cy = (i / cols) * ch + 16;
            for (int y = 0; y < f->h; ++y)
                for (int x = 0; x < f->w; ++x) {
                    uint32_t c = f->px[size_t(y) * size_t(f->w) + size_t(x)];
                    if (!(c & 0xFF000000)) continue;
                    int tx = cx + x, ty = cy + y;
                    if (tx < 0 || ty < 0 || tx >= SCREEN_W || ty >= SCREEN_H) continue;
                    g_menu.canvas[size_t(ty) * size_t(SCREEN_W) + size_t(tx)] = c & 0xFFFFFF;
                }
        }
        FILE *o = std::fopen("cursor.raw", "wb");
        if (o) {
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            std::fclose(o);
        }
        std::printf("cursor.raw %dx%d, %s: %d klatek, narysowanych %d\n",
                    g_clientW, g_clientH, nm, n, drawn);
        return 0;
    }

    // --bench <mapa> <zoom> <szer> <wys> [klatek] mierzy sam Compose().
    if (argc > 6 && std::strcmp(argv[2], "--bench") == 0) {
        g_menu.skMaps.clear();
        g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
        std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
        g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
        g_menu.skSel = std::atoi(argv[3]);
        if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
        g_clientW = std::atoi(argv[5]);
        g_clientH = std::atoi(argv[6]);
        g_menu.ApplyZoom(std::atoi(argv[4]));
        g_menu.CentreCamera();
        int frames = argc > 7 ? std::atoi(argv[7]) : 40;
        g_menu.Compose();                       // warm the caches
        LARGE_INTEGER f, a, b;
        QueryPerformanceFrequency(&f);
        QueryPerformanceCounter(&a);
        for (int i = 0; i < frames; ++i) g_menu.Compose();
        QueryPerformanceCounter(&b);
        double ms = 1000.0 * double(b.QuadPart - a.QuadPart) / double(f.QuadPart) / frames;
        std::printf("%-22s %4dx%-4d kafel %3d  %6.2f ms/klatka  %5.1f fps\n",
                    g_menu.terrName.c_str(), SCREEN_W, SCREEN_H, g_menu.tileW,
                    ms, 1000.0 / ms);
        return 0;
    }

    // --bldui: panel budynku, budynek po budynku. Wszystko odczytane z exe
    // przez `tools/gen_bldui.py`; ten tryb to pokazuje i sprawdza.
    if (argc > 2 && std::strcmp(argv[2], "--bldui") == 0) {
        int bn = 0, sn = 0, in = 0;
        bldui::buttons(bn);
        const bldui::Slot *sl = bldui::slots(sn);
        const bldui::Info *inf = bldui::info(in);
        std::printf("panel budynku: komend %d, gniazd %d, opisow %d\n",
                    bn, sn, in);

        int zPrzyc = 0, zInfo = 0, zAni = 0, ruchomych = 0, bezNazwy = 0;
        for (int tobj = 50; tobj <= 115; ++tobj) {
            const char *lab = nullptr;
            int side = -1;
            for (int r = 0; r < 3 && !lab; ++r) {
                int cnt = 0;
                const int *lst = cost::bldList(r, cnt);
                for (int q = 0; q < cnt; ++q)
                    if (lst[q] == tobj) { lab = cost::bldLabel(tobj, r); side = r; break; }
            }
            if (!lab) continue;             // TOBJ, ktorego zadna rasa nie ma

            char buts[220];
            int at = 0;
            buts[0] = 0;
            for (int i = 0; i < sn; ++i) {
                if (sl[i].tobj != tobj) continue;
                const char *rec = bldui::record(sl[i].cmd, tobj, side == 2);
                if (!rec || !*rec) { ++bezNazwy; continue; }
                at += std::snprintf(buts + at, sizeof(buts) - size_t(at),
                                    "%s%s", at ? " " : "", rec);
                if (at > 190) break;
            }
            const char *shows = "";
            for (int i = 0; i < in; ++i)
                if (inf[i].tobj == tobj) { shows = inf[i].shows; break; }
            const bool mob = bldui::mobile(tobj);
            // Animacje: nakladka (sekwencja 12) - ciagla albo na klatce
            // zerowej - oraz praca (sekwencja 11), odpalana zdarzeniem.
            char ani[80];
            int aa = 0;
            ani[0] = 0;
            if (const char *ov = units::buildingAni(tobj, side))
                aa += std::snprintf(ani + aa, sizeof(ani) - size_t(aa),
                                    "nakladka %s%s", ov,
                                    units::buildingAniLoops(tobj) ? " (ciagla)" : "");
            if (bldwork::works(tobj))
                aa += std::snprintf(ani + aa, sizeof(ani) - size_t(aa),
                                    "%spraca 3 fazy", aa ? ", " : "");
            if (*buts) ++zPrzyc;
            if (*shows) ++zInfo;
            if (mob) ++ruchomych;
            if (*ani) ++zAni;
            std::printf("  %3d %-26s %-46s %-24s %-30s%s\n", tobj, lab,
                        *buts ? buts : "-", *shows ? shows : "-",
                        *ani ? ani : "-", mob ? " [ruchomy]" : "");
        }
        std::printf("razem: z przyciskami %d, z informacja %d, z animacja %d,"
                    " ruchomych %d, bez nazwy rekordu %d\n",
                    zPrzyc, zInfo, zAni, ruchomych, bezNazwy);

        // **Ile przyciskow naprawde cos robi.** Sam odczyt nazwy to polowa
        // roboty - druga polowa to obsluga klikniecia. Lista slow jest ta
        // sama, po ktorej rozpoznaje je `CmdAction`, wiec audyt pilnuje, ze
        // nie przybedzie przycisku bez reakcji.
        static const char *kZnane[] = {
            "SETDESTINATION", "SIDESTINATION", "RISE", "FALL", "MOVECONSTR",
            "STOPCONSTR", "TELEOBJ", "TRADE", "CRACKINFO", "GETINFO",
            "TRGOLD", "CONTAINER", "GIVEENERGY", "SIGIVERC", "RCTOENERGY",
            "ACTPSIHO", "STOPISO", "IFIELD", "VIEWZONE", "CREATEGATE",
            "RESEARCH", "BUILD", "BLD", "ATTACK", "SISFIRE", "VQB",
            "DISSASSEMBLE", "DISMANTLING", "STOP", "MOVE",
            // rozkazy lodzi - te same slowa, po ktorych rozpoznaje je
            // `CmdAction`
            "PATROL", "GUARD", "BEHAVIOUR", "SCOUT", "CAPTURE", "DEFENCE",
            "RCLOAD", "RCUNLOAD", "REPLOAD", "REPUNLOAD", "RETREPAIR",
            "TELEPORT", "TELETO", "DISTRIB", "SETMINE", "REPSUBM", "REPLINISH",
            "SETLIGHT", "SETSNARE", "ZAPADLO", "PHANTOM",
        };
        // **Dopasowanie dokladne, nie po fragmencie.** `BUT_BREAK` to komenda
        // 82, czyli ta sama, co `BUT_SISFIRE` - exe trzyma obie w jednym
        // `case 0x52`, tylko nazwa zalezy od rasy. Ale `BUT_BREAKBUILD` (39)
        // i `BUT_BREAKAWAY` (46) to zupelnie inne rozkazy i obslugi nie maja,
        // wiec po fragmencie "BREAK" wyszlaby z tego fikcyjna pelnia.
        static const char *kDokladne[] = { "BUT_BREAK" };
        auto znany = [&](const char *rec) {
            for (unsigned q = 0; q < sizeof(kZnane) / sizeof(kZnane[0]); ++q)
                if (std::strstr(rec, kZnane[q])) return true;
            for (unsigned q = 0; q < sizeof(kDokladne) / sizeof(kDokladne[0]); ++q)
                if (std::strcmp(rec, kDokladne[q]) == 0) return true;
            return false;
        };
        int wszystkich = 0, obslugiwanych = 0;
        char brak[400];
        int bat = 0;
        brak[0] = 0;
        for (int i = 0; i < sn; ++i) {
            if (sl[i].tobj < 50 || sl[i].tobj > 115) continue;
            const char *rec = bldui::record(sl[i].cmd, sl[i].tobj, false);
            const char *rs = bldui::record(sl[i].cmd, sl[i].tobj, true);
            if (!rec || !*rec) rec = rs;
            if (!rec || !*rec) continue;
            ++wszystkich;
            bool ok = false;
            ok = znany(rec);
            if (ok) ++obslugiwanych;
            else if (bat < 300 && !std::strstr(brak, rec))
                bat += std::snprintf(brak + bat, sizeof(brak) - size_t(bat),
                                     "%s%s", bat ? " " : "", rec);
        }
        std::printf("obsluga: %d z %d przyciskow ma reakcje%s%s\n",
                    obslugiwanych, wszystkich, bat ? "; bez obslugi: " : "",
                    brak);

        // To samo dla lodzi - 40 typow, kazdy ma swoj uklad do szesciu gniazd.
        {
            int un = 0;
            const bldui::Slot *uk = bldui::unitSlots(un);
            int uall = 0, uok = 0, ubrak = 0;
            char ub[400];
            int uat = 0;
            ub[0] = 0;
            for (int i = 0; i < un; ++i) {
                const char *rec = bldui::record(uk[i].cmd, uk[i].tobj, false);
                const char *rs = bldui::record(uk[i].cmd, uk[i].tobj, true);
                if (!rec || !*rec) rec = rs;
                if (!rec || !*rec) { ++ubrak; continue; }
                ++uall;
                bool ok = false;
                ok = znany(rec);
                if (ok) ++uok;
                else if (uat < 300 && !std::strstr(ub, rec))
                    uat += std::snprintf(ub + uat, sizeof(ub) - size_t(uat),
                                         "%s%s", uat ? " " : "", rec);
            }
            std::printf("lodzie: %d typow, %d przyciskow, %d z reakcja%s%s\n",
                        40, uall, uok, uat ? "; bez obslugi: " : "", ub);
        {   // **Podpis przycisku jest w grze.** `FUN_00525390(komenda,
            // TOBJ)` zwraca identyfikator napisu; panel podpisywal sie
            // dotad nazwa rekordu bez `BUT_`, czyli po angielsku.
            // Liczymy, ile par ma napis, i ile z nich ma go **wlasny**,
            // a nie odziedziczony po wariancie domyslnym - bo tablica
            // bez rozbicia po TOBJ wygladalaby tak samo pelna.
            int zNapisem = 0, bez = 0, wlasny = 0, par = 0;
            int ns = 0;
            const bldui::Slot *sl = bldui::unitSlots(ns);
            int nb = 0;
            const bldui::Slot *sb = bldui::slots(nb);
            for (int pass = 0; pass < 2; ++pass) {
                const bldui::Slot *k = pass ? sb : sl;
                int cnt = pass ? nb : ns;
                for (int i = 0; i < cnt; ++i) {
                    ++par;
                    unsigned id = hint::stringId(k[i].cmd, k[i].tobj);
                    unsigned dom = hint::stringId(k[i].cmd, -1);
                    if (id == hint::NONE || Text(id).empty()) ++bez;
                    else ++zNapisem;
                    if (id != dom) ++wlasny;
                }
            }
            char et[64] = { 0 };
            g_menu.CmdLabel(1, 20, et, sizeof(et));      // lodz
            char et2[64] = { 0 };
            g_menu.CmdLabel(1, 62, et2, sizeof(et2));    // wiezyczka
            for (char *p = et; *p; ++p) if (*p == 10) *p = 32;
            for (char *p = et2; *p; ++p) if (*p == 10) *p = 32;
            std::printf("podpisy przyciskow: %d par, z napisem gry %d, bez %d, wlasny (nie domyslny) %d\n", par, zNapisem, bez, wlasny);
            std::printf("  komenda 1: lodz \"%s\", konstrukcja ruchoma \"%s\"\n", et, et2);
        }
        }

        // **Sprawdzian skutku, nie nazwy.** Dopasowanie napisu nic nie mowi
        // o tym, czy rozkaz cokolwiek robi - wiec kazdy z nowych przechodzi
        // tu cala droge i mierzy sie wynik.
        if (argc > 3) {
            g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
            std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
            g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
            g_menu.skSel = std::atoi(argv[3]);
            g_clientW = 1280;
            g_clientH = 860;
            if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
            g_menu.FitCanvas();
            g_menu.CentreCamera();

            {   // **RUCH lodzi - skutkiem, nie nazwa.** Lista slow w audycie
                // powyzej jest lustrem `CmdAction` i sama z siebie niczego
                // nie dowodzi: przez nia 38 z 40 typow lodzi mialo gniazdo
                // ruchu BEZ NAZWY REKORDU i nikt tego nie zauwazyl. Tu
                // klikamy przycisk tak, jak zrobilby to gracz.
                int lodz = -1;
                for (size_t k = 0; k < g_menu.units.size(); ++k)
                    if ((g_menu.units[k].owner & 7) == uint32_t(g_menu.me & 7)) {
                        lodz = int(k);
                        break;
                    }
                const char *stan = "BRAK WLASNEJ LODZI";
                int gniazdo = -1;
                const char *rec = "-";
                if (lodz >= 0) {
                    g_menu.sel.assign(1, lodz);
                    g_menu.selBld.clear();
                    g_menu.armed = Menu::ARM_NONE;
                    int nn = 0;
                    const Menu::Cmd *cc = g_menu.CmdsFor(false, nn);
                    for (int i = 0; i < nn; ++i)
                        if (std::strcmp(cc[i].rec, "BUT_MOVEBOAT") == 0 ||
                            std::strcmp(cc[i].rec, "BUT_SIMOVE") == 0) {
                            gniazdo = i;
                            rec = cc[i].rec;
                            break;
                        }
                    if (gniazdo >= 0) {
                        g_menu.CmdAction(false, gniazdo);
                        stan = g_menu.armed == Menu::ARM_MOVE
                             ? "uzbroilo klikniecie" : "NIE UZBROILO";
                    } else {
                        stan = "BRAK PRZYCISKU RUCHU";
                    }
                    g_menu.armed = Menu::ARM_NONE;
                    g_menu.sel.clear();
                }
                std::printf("ruch lodzi: gniazdo %d (%s), %s\n",
                            gniazdo, rec, stan);
            }

            // **Sam napis w tablicy nie dowodzi, ze widac go na ekranie.**
            // Podpis wychodzi tylko pod kursorem, wiec mierzymy roznice
            // klatki z kursorem na przycisku i obok niego.
            // **Podtesty wyzej czyszcza `units`** - bez odbudowy pomiar
            // mierzylby pusty panel i wyszedlby zerem, czyli tak samo
            // jak brak podpisu. Ta sama pulapka, co przy stoczni.
            g_menu.BuildUnits();
            for (int i = 0; i < int(g_menu.units.size()); ++i)
                if ((g_menu.units[size_t(i)].owner & 7)
                        == uint32_t(g_menu.me & 7)) {
                    g_menu.sel.assign(1, i);
                    break;
                }
            g_menu.selBld.clear();
            Menu::BarLay L{};
            RECT cr{};
            bool okL = g_menu.BarLayout(&L);
            bool okS = okL && g_menu.CmdSlot(L, false, 0, cr);
            if (g_menu.sel.empty() || !okS)
                std::printf("  pod kursorem: lodzi %d, zaznaczonych %d, pasek %s, gniazdo %s\n",
                            int(g_menu.units.size()), int(g_menu.sel.size()),
                            okL ? "jest" : "BRAK", okS ? "jest" : "BRAK");
            if (!g_menu.sel.empty() && okS) {
                g_menu.mouse = POINT{ 0, 0 };
                g_menu.Compose();
                std::vector<uint32_t> bez = g_menu.canvas;
                g_menu.mouse = POINT{ (cr.left + cr.right) / 2,
                                      (cr.top + cr.bottom) / 2 };
                g_menu.Compose();
                int rozne = 0;
                for (size_t q = 0; q < bez.size(); ++q)
                    if (bez[q] != g_menu.canvas[q]) ++rozne;
                bool prawy = false;
                int kt = g_menu.CmdHit(g_menu.mouse.x, g_menu.mouse.y, prawy);
                int nn = 0;
                const Menu::Cmd *cc = g_menu.CmdsFor(false, nn);
                char pod[64] = { 0 };
                if (cc && kt >= 0 && kt < nn)
                    std::snprintf(pod, sizeof(pod), "%s", cc[kt].what);
                for (char *p = pod; *p; ++p) if (*p == 10) *p = 32;
                std::printf("  pod kursorem: gniazdo %d \"%s\", pikseli roznicy %d\n", kt, pod, rozne);
                if (FILE *o = std::fopen("hint.raw", "wb")) {
                    std::fwrite(g_menu.canvas.data(), 4,
                                g_menu.canvas.size(), o);
                    std::fclose(o);
                    std::printf("  zrzut hint.raw %dx%d\n", SCREEN_W, SCREEN_H);
                }
                g_menu.mouse = POINT{ 0, 0 };
            }

            // Konstrukcja ruchoma: podnies, przestaw, doczekaj.
            g_menu.blds.clear();
            Menu::Bld nb;
            nb.owner = uint32_t(g_menu.me);
            nb.tobj = 62;                       // HF Cannon - ruchoma
            nb.x = 40; nb.y = 40;
            nb.hp = nb.hpMax = 1000;
            nb.span = 1;
            g_menu.blds.push_back(nb);
            g_menu.selBld.assign(1, 0);
            g_menu.SyncOcc();
            Menu::Bld &mb = g_menu.blds[0];
            const bool ruch = bldui::mobile(int(mb.tobj));
            mb.raised = true;
            int sx = 0, sy = 0;
            // **Cel musi spelniac dwa warunki naraz**: wolno tam budowac
            // i klik ma wypasc w oknie mapy. Szukamy wiec najpierw legalnej
            // komorki (`CanBuildAt`), a **dopiero potem** ustawiamy na nia
            // kamere - odwrotna kolejnosc nie dawala sie pogodzic, bo
            // w rzucie izometrycznym komorka o wiekszym `bx+by` schodzi pod
            // dolny pasek.
            int dcx = -1, dcy = -1, prob = 0, legal = 0;
            for (int d = 2; d <= 14 && dcx < 0; ++d)
                for (int a = -d; a <= d && dcx < 0; ++a)
                    for (int b2 = 0; b2 < 2 && dcx < 0; ++b2) {
                        int cx = 40 + (b2 ? a : -d), cy = 40 + (b2 ? -d : a);
                        ++prob;
                        if (!g_menu.CanBuildAt(cx, cy, 62)) continue;
                        ++legal;
                        dcx = cx; dcy = cy;
                    }
            if (dcx < 0) { dcx = 36; dcy = 36; }
            // Kamera na srodek miedzy budynkiem a celem.
            g_menu.CellToScreen(float(40 + dcx) * 0.5f, float(40 + dcy) * 0.5f,
                                sx, sy);
            RECT vpb = g_menu.Viewport();
            g_menu.camX += sx - int(vpb.left + vpb.right) / 2;
            g_menu.camY += sy - int(vpb.top + vpb.bottom) / 2;
            g_menu.ClampCamera();
            g_menu.CellToScreen(float(dcx), float(dcy), sx, sy);
            g_menu.armed = Menu::ARM_BLDMOVE;
            Menu::BarLay Lb;
            const bool hasBar = g_menu.BarLayout(&Lb);
            RECT vpc = g_menu.Viewport();
            std::printf("  [dbg] klik %d,%d vp %ld..%ld / %ld..%ld pasek %s %d sel %d\n",
                        sx, sy, vpc.left, vpc.right, vpc.top, vpc.bottom,
                        hasBar ? "jest" : "brak", hasBar ? Lb.y : -1,
                        int(g_menu.selBld.size()));
            const bool przejal = g_menu.ArmedClick(sx, sy);
            const float trwa = mb.moveT;
            for (int t = 0; t < 400 && mb.moveT > 0; ++t) g_menu.StepBldMove(0.05f);
            std::printf("ruchoma: %s, rozkaz %s, cel (%d,%d) z %d prob,"
                        " jazda %.1f s, stoi (%d,%d)\n",
                        ruch ? "tak" : "NIE", przejal ? "przejety" : "ODRZUCONY",
                        dcx, dcy, prob, double(trwa), mb.x, mb.y);

            // Punkt zbiorki: ustaw i sprawdz, ze siedzi w budynku.
            g_menu.CellToScreen(34.0f, 34.0f, sx, sy);
            g_menu.armed = Menu::ARM_RALLY;
            g_menu.ArmedClick(sx, sy);
            std::printf("punkt zbiorki: (%d,%d)\n", mb.rallyX, mb.rallyY);

            // Zrzut skladu: magazyn oddaje korium do skarbca.
            Menu::Bld dep;
            dep.owner = uint32_t(g_menu.me);
            dep.tobj = 59;
            dep.x = 60; dep.y = 60;
            dep.hp = dep.hpMax = 1000;
            dep.store = 500;
            dep.kind = 221;
            g_menu.blds.push_back(dep);
            const uint32_t przed = g_menu.Me().bank.corium;
            g_menu.DumpStore(g_menu.blds.back(), false);
            std::printf("zrzut skladu: korium %u -> %u, w skladzie %d\n",
                        przed, g_menu.Me().bank.corium, g_menu.blds.back().store);

            // Zloze da sie zaznaczyc - w grze ma wlasny panel (przypadki
            // 221, 222, 224 w `PaintCtrlObj`), a remake go nie mial.
            int zl = -1;
            for (size_t q = 0; q < g_menu.terr.objects.size(); ++q)
                if (g_menu.terr.objects[q].type == 90) { zl = int(q); break; }
            if (zl >= 0) {
                const maps::Object &zo = g_menu.terr.objects[size_t(zl)];
                int zx = 0, zy = 0;
                g_menu.CellToScreen(float(zo.x), float(zo.y), zx, zy);
                RECT vz = g_menu.Viewport();
                g_menu.camX += zx - int(vz.left + vz.right) / 2;
                g_menu.camY += zy - int(vz.top + vz.bottom) / 2;
                g_menu.ClampCamera();
                g_menu.CellToScreen(float(zo.x), float(zo.y), zx, zy);
                g_menu.sel.clear();
                g_menu.selBld.clear();
                g_menu.selRes = -1;
                const bool zlap = g_menu.SelectAt(zx, zy, false);
                std::printf("zloze: %s, wybrane %d, surowiec %s, ilosc %u\n",
                            zlap ? "zaznaczone" : "NIE ZAZNACZONE",
                            g_menu.selRes, g_menu.ResName(zo.subtype), zo.amount);
            }

            // Mina: staw, przeplyn obcym, sprawdz ze wybuchla i zabolala.
            g_menu.mines.clear();
            Menu::Mine mn;
            mn.x = 30.0f; mn.y = 30.0f;
            mn.owner = uint32_t(g_menu.me);
            mn.dmg = 300;
            g_menu.mines.push_back(mn);
            Menu::Unit wrog;
            wrog.owner = uint32_t((g_menu.me + 1) & 7);
            wrog.type = 1;
            wrog.x = 40.0f; wrog.y = 40.0f;
            g_menu.SetUnitStats(wrog);
            g_menu.units.push_back(wrog);
            const size_t wi = g_menu.units.size() - 1;
            const int hp0 = g_menu.units[wi].hp;
            g_menu.StepMines(0.1f);
            const int hpDaleko = g_menu.units[wi].hp;
            g_menu.units[wi].x = 30.0f;
            g_menu.units[wi].y = 30.0f;
            g_menu.StepMines(0.1f);
            std::printf("mina: z daleka %d, po najechaniu %d, min zostalo %d\n",
                        hp0 - hpDaleko, hp0 - g_menu.units[wi].hp,
                        int(g_menu.mines.size()));

            {   // **Lancuch.** Przewodnik mowi wprost: „explosion damages all
                // units, even other Depth Mines, causing a chain reaction".
                // Trzy miny glebinowe po 0,8 kratki od siebie - wejscie na
                // pierwsza ma wysadzic wszystkie trzy.
                g_menu.mines.clear();
                g_menu.mineBlew = 0;
                for (int k = 0; k < 3; ++k) {
                    Menu::Mine q;
                    q.kind = 0;                 // mina glebinowa
                    q.x = 30.0f + 0.8f * float(k);
                    q.y = 30.0f;
                    q.owner = uint32_t(g_menu.me);
                    q.dmg = Menu::MineDamage(0);
                    q.seen = true;
                    g_menu.mines.push_back(q);
                }
                Menu::Unit ofiara;
                ofiara.owner = uint32_t((g_menu.me + 1) & 7);
                ofiara.type = 3;
                ofiara.x = 30.0f; ofiara.y = 30.0f;
                g_menu.SetUnitStats(ofiara);
                g_menu.units.push_back(ofiara);
                const size_t oi = g_menu.units.size() - 1;
                const int hpA = g_menu.units[oi].hp;
                g_menu.StepMines(0.1f);
                std::printf("lancuch: min 3 -> %d, wybuchow %d, hp %d -> %d "
                            "(obrazenia miny %d, promien %.0f)\n",
                            int(g_menu.mines.size()), g_menu.mineBlew,
                            hpA, g_menu.units[oi].hp,
                            Menu::MineDamage(0), double(Menu::MineTrigger(0)));

                // **Sidlo laserowe** ma promien 3 i NIE robi lancucha:
                // „Self-destruction doesn't do any damage or set off nearby
                // Laser Snares".
                g_menu.mines.clear();
                g_menu.mineBlew = 0;
                for (int k = 0; k < 2; ++k) {
                    Menu::Mine q;
                    q.kind = 1;                 // sidlo laserowe
                    q.x = 30.0f + 0.8f * float(k);
                    q.y = 34.0f;
                    q.owner = uint32_t(g_menu.me);
                    q.dmg = Menu::MineDamage(1);
                    q.seen = true;
                    g_menu.mines.push_back(q);
                }
                // Obcy w zasiegu PIERWSZEGO sidla (2,5 kratki) i poza
                // zasiegiem drugiego (3,3) - gdyby byl lancuch, poszlyby oba.
                g_menu.units[oi].x = 27.5f;
                g_menu.units[oi].y = 34.0f;
                g_menu.units[oi].hp = g_menu.units[oi].hpMax;
                const int hpB = g_menu.units[oi].hp;
                g_menu.StepMines(0.1f);
                std::printf("sidlo: promien %.0f, min 2 -> %d (bez lancucha), "
                            "hp %d -> %d (obrazenia %d)\n",
                            double(Menu::MineTrigger(1)),
                            int(g_menu.mines.size()), hpB,
                            g_menu.units[oi].hp, Menu::MineDamage(1));
                g_menu.units.pop_back();
            }
            {   // **Bron masowego razenia**: trzy pary budynek-wyrzutnia, po
                // jednej na rase. Sprawdzamy oba konce drogi gracza: przycisk
                // BUDOWY ma ruszyc produkcje bomby, a przycisk WYSTRZALU ma
                // uzbroic klikniecie i naprawde zadac obrazenia w promieniu
                // z przewodnika (nuklearna 3000/6, laserowa zabija/5,
                // prozniowa 1000/5).
                struct Para { int fab, wyrz, rodzaj; const char *nazwa; };
                static const Para kP[4] = {
                    {  68,  69, 0, "nuklearna" },
                    {  78,  78, 1, "laserowa"  },
                    { 114, 114, 2, "prozniowa" },
                    { 112, 112, 3, "satelita"  },
                };
                for (int p = 0; p < 4; ++p) {
                    const Para &q = kP[p];
                    g_menu.blds.clear();
                    g_menu.units.clear();
                    g_menu.wmds.clear();
                    g_menu.fx.clear();
                    g_menu.wmdFired = g_menu.wmdKills = 0;
                    auto postaw = [&](int tobj, int cx, int cy) {
                        Menu::Bld b;
                        b.owner = uint32_t(g_menu.me);
                        b.tobj = uint32_t(tobj);
                        b.x = cx; b.y = cy;
                        b.span = 1;
                        b.hp = b.hpMax = 1000;
                        g_menu.blds.push_back(b);
                        return int(g_menu.blds.size()) - 1;
                    };
                    int fi = postaw(q.fab, 20, 20);
                    int wi = q.wyrz == q.fab ? fi : postaw(q.wyrz, 22, 20);
                    g_menu.SyncOcc();

                    // 1. przycisk BUDOWY bomby
                    g_menu.selBld.assign(1, fi);
                    g_menu.sel.clear();
                    g_menu.Me().bank.corium = 100000;
                    g_menu.Me().bank.metal = 100000;
                    int nn = 0;
                    const Menu::Cmd *cc = g_menu.CmdsFor(true, nn);
                    int slotB = -1, slotS = -1;
                    for (int i = 0; i < nn; ++i) {
                        const char *rc = cc[i].rec;
                        if (std::strstr(rc, "BLDMISSILE") ||
                            std::strstr(rc, "BLDLASBOMB") ||
                            std::strcmp(rc, "BUT_VQB") == 0) slotB = i;
                    }
                    if (slotB >= 0) g_menu.CmdAction(true, slotB);
                    // Wyrzutnia satelity nie ma przycisku produkcji - laduje
                    // sie sama, wiec wystarczy jeden krok, zeby ruszyla.
                    if (q.rodzaj == 3) g_menu.StepAmmo(0.0f);
                    bool ruszyla = g_menu.blds[size_t(fi)].ammoT > 0.0f;
                    for (int k = 0; k < 2000 && g_menu.blds[size_t(fi)].ammoT > 0.0f; ++k)
                        g_menu.StepAmmo(0.25f);
                    int magazyn = g_menu.blds[size_t(fi)].ammo;

                    // 2. przycisk WYSTRZALU
                    g_menu.selBld.assign(1, wi);
                    g_menu.armed = Menu::ARM_NONE;
                    cc = g_menu.CmdsFor(true, nn);
                    for (int i = 0; i < nn; ++i) {
                        const char *rc = cc[i].rec;
                        if (std::strstr(rc, "ATTACKTLS") ||
                            std::strstr(rc, "ATTACKLBOMB") ||
                            (std::strstr(rc, "ATTACKTRG") &&
                             (q.wyrz == 114 || q.wyrz == 112)))
                            slotS = i;
                    }
                    if (slotS >= 0) g_menu.CmdAction(true, slotS);
                    bool uzbroil = g_menu.armed == Menu::ARM_WMD;

                    // 3. skutek: obcy w promieniu i tuz poza nim
                    float r = Menu::WmdRadius(q.rodzaj);
                    auto wrog = [&](float dx) {
                        Menu::Unit u;
                        u.owner = uint32_t((g_menu.me + 1) & 7);
                        u.type = 3;
                        u.x = 30.0f + dx; u.y = 30.0f;
                        g_menu.SetUnitStats(u);
                        // Wytrzymalosc x20, zeby bylo widac PELNE obrazenia,
                        // a nie tylko to, ze lodz zginela od pierwszego.
                        u.hpMax *= 20;
                        u.hp = u.hpMax;
                        g_menu.units.push_back(u);
                        return g_menu.units.size() - 1;
                    };
                    size_t blisko = wrog(0.0f);
                    size_t daleko = wrog(r + 3.0f);
                    int hp0 = g_menu.units[blisko].hp, hp1 = g_menu.units[daleko].hp;
                    Menu::Wmd w;
                    w.kind = q.rodzaj;
                    w.x = 30.0f; w.y = 30.0f;
                    w.owner = uint32_t(g_menu.me);
                    w.z = q.rodzaj == 1 ? float(maps::LEVELS - 1) : 0.0f;
                    g_menu.wmds.push_back(w);
                    if (q.rodzaj == 0) {
                        // Grzyb `expl_nb0` ma 64 klatki; w polowie jest
                        // najokazalszy - kadrujemy i zrzucamy, zeby nie
                        // wierzyc licznikowi na slowo.
                        for (int k = 0; k < 30; ++k) g_menu.StepWmd(1.0f / 18.0f);
                        int qx, qy;
                        g_menu.CellToScreen(30.0f, 30.0f, qx, qy);
                        g_menu.camX += qx - SCREEN_W / 2;
                        g_menu.camY += qy - SCREEN_H / 2;
                        g_menu.ClampCamera();
                        g_menu.Compose();
                        std::vector<uint32_t> bez;
                        {   std::vector<Menu::Wmd> zap = g_menu.wmds;
                            g_menu.wmds.clear();
                            g_menu.Compose();
                            bez = g_menu.canvas;
                            g_menu.wmds = zap;
                            g_menu.Compose();
                        }
                        int zmian = 0;
                        for (size_t k = 0; k < bez.size() && k < g_menu.canvas.size(); ++k)
                            if (bez[k] != g_menu.canvas[k]) ++zmian;
                        FILE *fn = std::fopen("bldui_nuke.raw", "wb");
                        if (fn) {
                            std::fwrite(g_menu.canvas.data(), 4,
                                        g_menu.canvas.size(), fn);
                            std::fclose(fn);
                        }
                        std::printf("grzyb: pikseli zmienionych %d, zrzut "
                                    "bldui_nuke.raw %dx%d\n", zmian,
                                    SCREEN_W, SCREEN_H);
                    }
                    for (int k = 0; k < 400 && !g_menu.wmds.empty(); ++k)
                        g_menu.StepWmd(0.05f);
                    int hpA = g_menu.units[blisko].hp, hpB = g_menu.units[daleko].hp;

                    std::printf("%-10s produkcja %s, w magazynie %d, przycisk "
                                "%s; w promieniu %d hp %d -> %d, poza %d hp "
                                "%d -> %d\n", q.nazwa,
                                ruszyla ? "ruszyla" : "NIE RUSZYLA", magazyn,
                                uzbroil ? "uzbroil" : "NIE UZBROIL",
                                int(r), hp0, hpA, int(r) + 3, hp1, hpB);
                }
                g_menu.units.clear();
                g_menu.blds.clear();
            }

            // Teleport budynku do bramy.
            Menu::Bld gate;
            gate.owner = uint32_t(g_menu.me);
            gate.tobj = 55;
            gate.x = dcx; gate.y = dcy - 8;
            gate.hp = gate.hpMax = 1000;
            g_menu.blds.push_back(gate);
            g_menu.SyncOcc();
            Menu::Bld &tb = g_menu.blds[0];
            const int ox = tb.x, oy = tb.y;
            g_menu.TeleportBld(tb);
            std::printf("teleport budynku: z (%d,%d) na (%d,%d) [%s], brama SI %s\n",
                        ox, oy, tb.x, tb.y, g_menu.buildMsg.c_str(),
                        Menu::IsGate(108) ? "znana" : "NIEZNANA");
        }

        // Praca budynku: trzy fazy, a faza 3 ma isc WSTECZ - inaczej luk
        // by sie tylko podnosil i nigdy nie opuszczal.
        int faz = 0, wstecz = 0;
        for (int tobj = 50; tobj <= 115; ++tobj) {
            if (!bldwork::works(tobj)) continue;
            for (int ph = 1; ph <= 3; ++ph) {
                if (bldwork::strip(tobj, ph, bldwork::RES_CORIUM)) ++faz;
                if (bldwork::backwards(tobj, ph)) ++wstecz;
            }
        }
        std::printf("praca: faz z paskiem %d, faz wstecz %d\n", faz, wstecz);

        // **Sprawdzian krzyzowy.** Zbior konstrukcji ruchomych ma sie zgadzac
        // z niezerowymi wpisami tablicy dzwieku wiezyczki na postoju - to sa
        // dwa niezalezne odczyty tego samego zbioru budynkow.
        int tylkoRuch = 0, tylkoDzwiek = 0, oba = 0;
        for (int tobj = 50; tobj < 50 + sndev::GROUPS; ++tobj) {
            const bool m = bldui::mobile(tobj), d = sndev::bldIdle(tobj) != 0;
            if (m && d) ++oba;
            else if (m) ++tylkoRuch;
            else if (d) ++tylkoDzwiek;
        }
        std::printf("kontrola: ruchome i z dzwiekiem postoju %d, tylko ruchome %d,"
                    " tylko z dzwiekiem %d\n", oba, tylkoRuch, tylkoDzwiek);
        return 0;
    }

    // --shader <mapa>: filtry obrazu. Najpierw wzorzec, w ktorym wiadomo, co
    // ma wyjsc (plaskie pole ma zostac plaskie, a mieszac wolno tylko xBRZ),
    // potem koszt na prawdziwej klatce - bo to on rozstrzyga, czy filtr
    // nadaje sie do grania.
    if (argc > 2 && std::strcmp(argv[2], "--shader") == 0) {
        const int W = 64, H = 64;
        const uint32_t kBg = 0x203040, kLine = 0xF0E0C0, kBlk = 0x00A000;
        std::vector<uint32_t> src(size_t(W) * size_t(H), kBg);
        for (int i = 0; i < W && i < H; ++i)
            src[size_t(i) * size_t(W) + size_t(i)] = kLine;      // skos
        for (int y = 40; y < 56; ++y)                            // plaskie pole
            for (int x = 40; x < 56; ++x)
                src[size_t(y) * size_t(W) + size_t(x)] = kBlk;
        std::vector<uint32_t> dst(size_t(W * 2) * size_t(H * 2));

        std::printf("filtr obrazu: %d trybow\n", int(shade::COUNT));
        for (int m = 1; m < shade::COUNT; ++m) {
            // **Nie kazdy tryb powieksza.** CRT tylko przemalowuje klatke,
            // wiec wychodzi w rozmiarze wejscia; reszta mnozy przez dwa.
            const int s = shade::scaleOf(m);
            const int ow = W * s, oh = H * s;
            std::fill(dst.begin(), dst.end(), 0u);
            shade::apply(m, src.data(), W, H, dst.data());
            if (m == shade::CRT || m == shade::CRT_XBRZ) {
                // Linie obrazu: co druga linia ma byc ciemniejsza. Mierzone
                // we wnetrzu jednolitego bloku, zeby nie mierzyc krawedzi.
                double lum[2] = { 0, 0 };
                int cnt[2] = { 0, 0 };
                for (int y = 42 * s; y < 54 * s; ++y)
                    for (int x = 42 * s; x < 54 * s; ++x) {
                        uint32_t c = dst[size_t(y) * size_t(ow) + size_t(x)];
                        lum[y & 1] += 0.2126 * double((c >> 16) & 0xFF)
                                    + 0.7152 * double((c >> 8) & 0xFF)
                                    + 0.0722 * double(c & 0xFF);
                        ++cnt[y & 1];
                    }
                std::printf("  %-11s %dx%d (%dx), linia jasna %.1f, ciemna %.1f -> %s\n",
                            shade::name(m), ow, oh, s,
                            lum[0] / cnt[0], lum[1] / cnt[1],
                            lum[1] < lum[0] ? "kontrast jest" : "BRAK KONTRASTU");
                continue;
            }
            int flat = 0, flatN = 0;
            for (int y = 44; y < 52; ++y)
                for (int x = 44; x < 52; ++x)
                    for (int dy = 0; dy < s; ++dy)
                        for (int dx = 0; dx < s; ++dx) {
                            ++flatN;
                            if (dst[size_t(s * y + dy) * size_t(ow)
                                    + size_t(s * x + dx)] == kBlk) ++flat;
                        }
            int mixed = 0;
            for (size_t i = 0; i < size_t(ow) * size_t(oh); ++i) {
                const uint32_t c = dst[i];
                if (c != kBg && c != kLine && c != kBlk) ++mixed;
            }
            std::printf("  %-11s %dx%d (%dx), plaskie pole %d z %d, barw mieszanych %d%s\n",
                        shade::name(m), ow, oh, s, flat, flatN, mixed,
                        m == shade::XBRZ
                            ? (mixed > 0 ? "  (skos wygladzony)" : "  BRAK MIESZANIA")
                            : (mixed == 0 ? "  (tylko kopiuje)" : "  ZMYSLA BARWY"));
        }

        if (argc > 3) {
            g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
            std::vector<maps::Entry> mi = maps::scan(g_gameDir + "\\missions");
            g_menu.skMaps.insert(g_menu.skMaps.end(), mi.begin(), mi.end());
            g_menu.skSel = std::atoi(argv[3]);
            g_clientW = argc > 4 ? std::atoi(argv[4]) : 1920;
            g_clientH = argc > 5 ? std::atoi(argv[5]) : 1080;
            if (!g_menu.OpenTerrain(g_gameDir)) { std::printf("brak terenu\n"); return 1; }
            g_menu.CentreCamera();

            // **Interfejs ma zostac nietkniety** - to jest cala umowa tego
            // filtra, wiec sprawdzamy ja dwoma pomiarami, nie okiem:
            //
            //   * stan kamery po zlozeniu klatki musi byc co do liczby ten
            //     sam, bo przebieg swiata dzieli go przez dwa i oddaje,
            //   * minimapa - kawalek HUD-u, ktory jest w calosci malowany -
            //     musi wyjsc piksel w piksel taka sama.
            g_menu.SetShader(shade::OFF);
            g_menu.Compose();
            const std::vector<uint32_t> ref = g_menu.canvas;
            const int rCamX = g_menu.camX, rCamY = g_menu.camY;
            const int rTileW = g_menu.tileW, rTileH = g_menu.tileH;
            Menu::BarLay L;
            const bool hasBar = g_menu.BarLayout(&L);
            for (int m = 1; m < shade::COUNT; ++m) {
                g_menu.SetShader(m);
                g_menu.Compose();
                int diff = 0, tot = 0;
                if (hasBar)
                    for (int y = L.y; y < SCREEN_H; ++y)
                        for (int x = 0; x < SCREEN_W; ++x) {
                            if (!g_menu.MiniHit(x, y)) continue;
                            ++tot;
                            const size_t i = size_t(y) * size_t(SCREEN_W) + size_t(x);
                            if (ref[i] != g_menu.canvas[i]) ++diff;
                        }
                const bool cam = g_menu.camX == rCamX && g_menu.camY == rCamY
                              && g_menu.tileW == rTileW && g_menu.tileH == rTileH;
                std::printf("  %-11s kamera %s, minimapa %d roznych z %d px%s\n",
                            shade::name(m), cam ? "oddana" : "ZGUBIONA",
                            diff, tot, (cam && !diff) ? "" : "   <- USTERKA");
            }
            g_menu.SetShader(shade::OFF);

            LARGE_INTEGER f, a, b;
            QueryPerformanceFrequency(&f);
            double base = 0;
            for (int m = 0; m < shade::COUNT; ++m) {
                g_menu.SetShader(m);
                g_menu.Compose();                   // rozgrzej
                const int cw = SCREEN_W, chh = SCREEN_H;
                QueryPerformanceCounter(&a);
                for (int i = 0; i < 12; ++i) g_menu.Compose();
                QueryPerformanceCounter(&b);
                double ms = 1000.0 * double(b.QuadPart - a.QuadPart)
                          / double(f.QuadPart) / 12.0;
                if (m == 0) base = ms;
                // **Plotno ma byc to samo w kazdym trybie** - filtr obejmuje
                // sam swiat, wiec ani rozmiar plotna, ani uklad interfejsu
                // nie moga sie ruszyc.
                std::printf("  %-11s plotno %dx%d%s, klatka %5.2f ms (filtr %+.2f)\n",
                            shade::name(m), cw, chh,
                            (cw == g_clientW && chh == g_clientH)
                                ? "" : "  PLOTNO SIE ZMIENILO",
                            ms, ms - base);
                char nm[32];
                std::snprintf(nm, sizeof(nm), "shader%d.raw", m);
                if (FILE *o = std::fopen(nm, "wb")) {
                    std::fwrite(g_menu.canvas.data(), 4,
                                size_t(cw) * size_t(chh), o);
                    std::fclose(o);
                }
            }
            g_menu.SetShader(shade::OFF);
        }

        // Wiersz z filtrem stoi w oknie ustawien jako osmy - sprawdzamy, ze
        // sie tam miesci, bo `WinLine` po cichu obcina to, co wystaje, oraz
        // ze klikniecie w niego faktycznie przestawia filtr i plotno.
        g_menu.winOpen = Menu::WIN_OPTIONS;
        g_menu.optTab = 1;
        RECT ow2;
        if (g_menu.WinRect(Menu::WIN_OPTIONS, ow2)) {
            int rows = g_menu.WinMaxRows(ow2);
            std::printf("okno ustawien: wierszy %d, filtr w 7, podpowiedz w 8 -> %s\n",
                        rows, rows >= 9 ? "mieszcza sie" : "NIE MIESZCZA SIE");
        }
        {
            g_menu.SetShader(shade::OFF);
            const int w0 = SCREEN_W;
            int seen = 0;
            for (int i = 1; i < shade::COUNT; ++i) {
                g_menu.OptRow(7);               // ta sama droga co klikniecie
                if (g_menu.shader == i) ++seen;
            }
            g_menu.OptRow(7);                   // pelne kolo wraca na BRAK
            std::printf("wiersz w oknie: przelaczyl %d z %d, wrocil na %s, "
                        "plotno %d -> %d\n",
                        seen, int(shade::COUNT) - 1, shade::name(g_menu.shader),
                        w0, SCREEN_W);
        }
        g_menu.winOpen = Menu::WIN_NONE;
        return 0;
    }

    // --anim steps the real state machine and writes every composed step, so a
    // preview comes out of the shipping code path instead of a reimplementation
    if (argc > 2 && std::strcmp(argv[2], "--anim") == 0) {
        FILE *o = std::fopen("anim.raw", "wb");
        if (!o) return 1;
        int want = (argc > 3) ? std::atoi(argv[3]) : 0;   // 0 = just the reveal
        DWORD t = 0;
        int n = 0;
        for (; n < 400; ++n) {
            g_menu.Compose();
            std::fwrite(g_menu.canvas.data(), 4, g_menu.canvas.size(), o);
            t += g_menu.interval;
            bool moving = g_menu.Step(t);
            g_menu.StepBackgrounds(t);
            if (!moving && n + 1 >= want) break;
        }
        std::fclose(o);
        std::printf("zapisano anim.raw, %d klatek %dx%d BGRA\n", n + 1, SCREEN_W, SCREEN_H);
        return 0;
    }

    // --sfx N writes the wrapped WAV out so the RIFF header can be checked by
    // something other than the ear.
    if (argc > 3 && std::strcmp(argv[2], "--sfx") == 0) {
        int id = std::atoi(argv[3]);
        std::string name = g_menu.sfx.resolve(id);
        std::vector<uint8_t> wav;
        if (name.empty() || !g_menu.sfx.wrap(name, wav)) {
            std::printf("id %d: nie udalo sie zbudowac WAV\n", id);
            return 1;
        }
        FILE *o = std::fopen("sfx.wav", "wb");
        if (o) { std::fwrite(wav.data(), 1, wav.size(), o); std::fclose(o); }
        std::printf("id %d -> %s, zapisano sfx.wav %d bajtow\n",
                    id, name.c_str(), int(wav.size()));
        return 0;
    }

    if (argc > 2 && std::strcmp(argv[2], "--smoke") == 0) {
        g_menu.Compose();
        {   // kursory uzywane przez PickCursor - kazdy musi sie znalezc,
            // bo w SDL systemowy jest schowany i brak klatki = brak kursora
        std::printf("MONEY_FONT: %s, wysokosc %d, szerokosc cyfry 0 = %d\n",
                    g_menu.fontMoney.ok() ? "wczytany" : "BRAK",
                    g_menu.fontMoney.height(), g_menu.fontMoney.charWidth((unsigned char)(0x30)));
            static const char *kUse[] = { "CUR_MENU", "CUR_ARROW", "CUR_CMD",
                                          "CUR_OWNBOAT", "CUR_OWNOBJ",
                                          "CUR_NOBUILD" };
            std::printf("kursory:");
            for (const char *n : kUse) {
                const spr::Frame *cf = g_menu.cursors.frame(n, 0);
                std::printf(" %s=%s", n, cf && cf->ok() ? "jest" : "BRAK");
            }
            std::printf("\n");
        }
        std::printf("OK tlo %dx%d, klatki przyciskow: %d %d %d %d %d\n",
                    g_menu.bg.w, g_menu.bg.h,
                    g_menu.frameCount[0], g_menu.frameCount[1], g_menu.frameCount[2],
                    g_menu.frameCount[3], g_menu.frameCount[4]);
        for (int b = 0; b < BTN_COUNT; ++b)
            std::printf("  [%d] %s\n", b, Text(kLabels[0][b]).c_str());
        return 0;
    }
    return -1;
}
