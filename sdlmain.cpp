// Wejscie SDL: to samo `stapp.h`, inna warstwa okna.
//
// Po co, skoro Win32 dziala: prezentacja idzie przez GPU zamiast przez
// StretchDIBits, jest vsync za darmo, a petla zdarzen przestaje byc zwiazana
// z Windows. Sam rasteryzator zostaje programowy - to on kosztuje te ~17 ms
// przy 1920x1080 i zadna biblioteka tego nie skroci; przeniesienie terenu
// i spritow na GPU to osobna, wieksza robota.
//
// Co jeszcze trzyma nas przy Windows: dzwiek (PlaySound i waveOut) oraz
// tablica napisow (LoadStringA z st_string.dll). Do pelnej przenosnosci trzeba
// bedzie oba zastapic - SDL_mixer albo wlasne miksowanie i wlasny czytnik
// zasobow PE.
#include "stapp.h"

}  // namespace   - stapp.h otwiera anonimowa przestrzen, tu ja domykamy

#include "stcli.h"
#include <SDL2/SDL.h>

namespace {

// SDL nie zna kodow wirtualnych Windows, a obsluga klawiszy w Menu jest na nich
// oparta. Tlumaczymy w jednym miejscu, zeby nie rozjechac sie z wersja Win32.
int VkFromSdl(SDL_Keycode k)
{
    if (k >= SDLK_a && k <= SDLK_z) return 'A' + (k - SDLK_a);
    if (k >= SDLK_0 && k <= SDLK_9) return '0' + (k - SDLK_0);
    switch (k) {
    case SDLK_LEFT:      return VK_LEFT;
    case SDLK_RIGHT:     return VK_RIGHT;
    case SDLK_UP:        return VK_UP;
    case SDLK_DOWN:      return VK_DOWN;
    case SDLK_HOME:      return VK_HOME;
    case SDLK_TAB:       return VK_TAB;
    case SDLK_RETURN:    return VK_RETURN;
    case SDLK_PAGEUP:    return VK_PRIOR;
    case SDLK_PAGEDOWN:  return VK_NEXT;
    case SDLK_DELETE:    return VK_DELETE;
    case SDLK_ESCAPE:    return VK_ESCAPE;
    case SDLK_PLUS:
    case SDLK_EQUALS:
    case SDLK_KP_PLUS:   return VK_OEM_PLUS;
    case SDLK_MINUS:
    case SDLK_KP_MINUS:  return VK_OEM_MINUS;
    case SDLK_LEFTBRACKET:  return VK_OEM_4;
    case SDLK_RIGHTBRACKET: return VK_OEM_6;
    case SDLK_COMMA:     return VK_OEM_COMMA;
    case SDLK_PERIOD:    return VK_OEM_PERIOD;
    default:             return 0;
    }
}

// Gdzie plotno lezy w oknie - ta sama zasada co w wersji Win32: teren dostaje
// caly obszar, ekrany menu zachowuja proporcje i sa wysrodkowane.
SDL_Rect PresentRectSdl(int winW, int winH)
{
    int dw = winW, dh = winH;
    if (SCREEN_W != winW || SCREEN_H != winH) {
        if (winW * SCREEN_H <= winH * SCREEN_W) { dw = winW; dh = winW * SCREEN_H / SCREEN_W; }
        else                                    { dh = winH; dw = winH * SCREEN_W / SCREEN_H; }
    }
    SDL_Rect r{ (winW - dw) / 2, (winH - dh) / 2, dw, dh };
    return r;
}

POINT ToCanvasSdl(int x, int y, int winW, int winH)
{
    SDL_Rect d = PresentRectSdl(winW, winH);
    POINT p{ x, y };
    if (d.w > 0) p.x = (x - d.x) * SCREEN_W / d.w;
    if (d.h > 0) p.y = (y - d.y) * SCREEN_H / d.h;
    return p;
}

}  // namespace

int main(int argc, char **argv)
{
    const std::string gameDir = "C:\\Program Files (x86)\\Submarine Titans";
    g_assetDir = (argc > 1) ? argv[1] : "..\\..\\assets\\inter_raw";
    g_strings = LoadLibraryExA((gameDir + "\\st_string.dll").c_str(),
                               nullptr, LOAD_LIBRARY_AS_DATAFILE);

    if (!g_menu.Load()) {
        std::printf("brak grafik w %s\n", g_assetDir.c_str());
        return 1;
    }
    g_menu.LoadBackgrounds(gameDir);

    int rc = RunCli(argc, argv, gameDir);
    if (rc >= 0) return rc;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::printf("SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("Submarine Titans",
                                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       MENU_W, MENU_H, SDL_WINDOW_RESIZABLE);
    if (!win) { std::printf("okno: %s\n", SDL_GetError()); return 1; }
    SDL_SetWindowMinimumSize(win, 480, 380);

    // Akceleracja plus vsync. Gdy sterownik nie da rady, SDL sam schodzi do
    // programowego renderera, wiec i tak sie uruchomi.
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1,
                        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) ren = SDL_CreateRenderer(win, -1, 0);
    if (!ren) { std::printf("renderer: %s\n", SDL_GetError()); return 1; }
    SDL_RendererInfo info{};
    SDL_GetRendererInfo(ren, &info);
    std::printf("SDL: renderer %s%s\n", info.name,
                (info.flags & SDL_RENDERER_ACCELERATED) ? " (GPU)" : " (programowy)");

    SDL_Texture *tex = nullptr;
    int texW = 0, texH = 0;
    SDL_ShowCursor(SDL_DISABLE);            // gra rysuje wlasny, animowany

    bool run = true;
    Uint32 last = SDL_GetTicks();
    while (run) {
        int winW = 0, winH = 0;
        SDL_GetWindowSize(win, &winW, &winH);
        g_clientW = winW;
        g_clientH = winH;

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT:
                if (g_menu.screen == SCR_EDITOR && g_menu.edit.dirty) g_menu.EdRequest(4);
                else run = false;
                break;
            case SDL_MOUSEMOTION: {
                POINT c = ToCanvasSdl(e.motion.x, e.motion.y, winW, winH);
                g_menu.mouse = c;           // kursor jest na kazdym ekranie
                g_menu.PickCursor();
                if (g_menu.miniDrag) g_menu.MiniGoto(c.x, c.y);
                if (g_menu.screen == SCR_EDITOR) {
                    g_menu.EditorMove(c.x, c.y);
                    break;
                }
                if (g_menu.screen == SCR_TERRAIN) {
                    if (g_menu.banding) g_menu.bandTo = c;
                    else if (g_menu.dragging) {
                        g_menu.camX -= c.x - g_menu.dragFrom.x;
                        g_menu.camY -= c.y - g_menu.dragFrom.y;
                        g_menu.dragFrom = c;
                        g_menu.ClampCamera();
                    }
                } else if (g_menu.SkInput()) {
                    g_menu.SkHover(c.x, c.y);
                } else {
                    g_menu.hot = g_menu.HitTest(c.x, c.y);
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                if (e.button.button == SDL_BUTTON_LEFT && e.button.clicks >= 2
                    && g_menu.screen == SCR_TERRAIN) {
                    POINT c2 = ToCanvasSdl(e.button.x, e.button.y, winW, winH);
                    if (g_menu.SelectSameKind(c2.x, c2.y)) break;
                }
                POINT c = ToCanvasSdl(e.button.x, e.button.y, winW, winH);
                if (g_menu.screen == SCR_EDITOR) {
                    if (e.button.button == SDL_BUTTON_MIDDLE) {
                        g_menu.EditorPanStart(c.x, c.y);
                        SDL_CaptureMouse(SDL_TRUE);
                    } else {
                        g_menu.EditorClick(c.x, c.y,
                                           e.button.button == SDL_BUTTON_RIGHT);
                    }
                    break;
                }
                if (g_menu.screen != SCR_TERRAIN) break;   // menu obsluguje puszczenie
                if (e.button.button == SDL_BUTTON_MIDDLE) {
                    g_menu.dragging = true;
                    g_menu.dragFrom = c;
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    if(g_menu.PaletteCancelAt(c.x,c.y)) break;
                    if (g_menu.buildPick >= 0) { g_menu.buildPick = -1; break; }
                    if (!g_menu.sel.empty()) {
                        if (g_menu.OverBar(c.x, c.y)) break;   // panel, nie mapa
                        if (!g_menu.OrderAttack(c.x, c.y)) g_menu.OrderMove(c.x, c.y);
                        g_menu.curName = "CUR_CONFIRM";
                        g_menu.curStep = 0;
                        g_menu.oneShot = g_menu.cursors.frames("CUR_CONFIRM") * Menu::CUR_DIV;
                    }
                } else if (e.button.button == SDL_BUTTON_LEFT) {
                    // Panel admina - jedno wejscie, wspolne z Win32.
                    if (g_menu.AdminClick(c.x, c.y)) break;
                    if (g_menu.WinClick(c.x, c.y)) break;
                    // Uzbrojony przycisk rozkazu przejmuje nastepne klikniecie.
                    if (g_menu.ArmedClick(c.x, c.y)) break;
                    if (g_menu.MiniHit(c.x, c.y)) {
                        g_menu.MiniGoto(c.x, c.y);
                        g_menu.miniDrag = true;
                        // Bez przechwytu puszczenie przycisku poza oknem nie
                        // dochodzi i kamera zostaje przyklejona do myszy.
                        SDL_CaptureMouse(SDL_TRUE);
                        break;
                    }
                    g_menu.mouseDown = true;
                    if (g_menu.PanelClick(c.x, c.y)) break;
                    if (g_menu.PlaceBuilding(c.x, c.y)) break;
                    // Kolejnosc zaznaczania siedzi w `Menu::SelectAt` -
                    // jedno miejsce na oba backendy.
                    bool add = (SDL_GetModState() & KMOD_SHIFT) != 0;
                    if (g_menu.SelectAt(c.x, c.y, add)) break;
                    g_menu.banding = true;
                    g_menu.bandFrom = g_menu.bandTo = c;
                    // zaznaczenia nie kasujemy - patrz SelectInBand
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                if (g_menu.screen == SCR_EDITOR) {
                    g_menu.EditorRelease();
                    SDL_CaptureMouse(SDL_FALSE);
                    break;
                }
                g_menu.mouseDown = false;
                POINT c = ToCanvasSdl(e.button.x, e.button.y, winW, winH);
                if (g_menu.FullScreenClick()) break;
                if (g_menu.SkInput()) {
                    if (g_menu.SkClick(c.x, c.y)) break;
                    int b = g_menu.SkHitButton(c.x, c.y);
                    if (b >= 0) OnSkirmishPress(b);
                    break;
                }
                if (g_menu.miniDrag && e.button.button == SDL_BUTTON_LEFT) {
                    g_menu.miniDrag = false;    // zawsze, na kazdym ekranie
                    SDL_CaptureMouse(SDL_FALSE);
                }
                if (g_menu.screen == SCR_MENU) {
                    int b = g_menu.HitTest(c.x, c.y);
                    if (b >= 0) OnPress(b);
                    break;
                }
                if (e.button.button == SDL_BUTTON_MIDDLE) g_menu.dragging = false;
                if (e.button.button == SDL_BUTTON_LEFT && g_menu.miniDrag) {
                    g_menu.miniDrag = false;
                    SDL_CaptureMouse(SDL_FALSE);
                }
                if (e.button.button == SDL_BUTTON_LEFT && g_menu.banding) {
                    g_menu.banding = false;
                    g_menu.bandTo = c;
                    long dx = c.x - g_menu.bandFrom.x, dy = c.y - g_menu.bandFrom.y;
                    if (dx * dx + dy * dy > 25) {
                        g_menu.SelectInBand();
                        if (!g_menu.sel.empty())
                            g_menu.sfx.play(Menu::sfxSelect(
                                int(g_menu.units[size_t(g_menu.sel[0])].type)));
                    }
                }
                break;
            }
            case SDL_MOUSEWHEEL:
                if (g_menu.screen == SCR_EDITOR) {
                    g_menu.EditorWheel(e.wheel.y);
                    break;
                }
                if (g_menu.screen == SCR_TERRAIN) {
                    int mx = 0, my = 0;
                    SDL_GetMouseState(&mx, &my);
                    POINT c = ToCanvasSdl(mx, my, winW, winH);
                    g_menu.ZoomAt(e.wheel.y > 0 ? 1 : -1, c.x, c.y);
                } else if (g_menu.screen == SCR_SKIRMISH) {
                    g_menu.SkScroll(-e.wheel.y * 3);
                }
                break;
            case SDL_KEYDOWN: {
                int vk = VkFromSdl(e.key.keysym.sym);
                if (vk == VK_ESCAPE && g_menu.screen == SCR_TERRAIN) {
                    g_menu.music.stop();
                    g_menu.screen = SCR_SKIRMISH;
                    break;
                }
                if (vk) g_menu.OnKey(vk);
                break;
            }
            default:
                break;
            }
        }

        Uint32 now = SDL_GetTicks();
        if (now - last >= 15) {
            last = now;
            g_menu.sfx.pump();       // dokarm mikser raz na klatke
            g_menu.Step(now);
            g_menu.StepBackgrounds(now);
            if (g_menu.screen == SCR_TERRAIN) g_menu.StepUnits(now);
        }

        if (g_menu.quitRequested) break;
        g_menu.Compose();
        if (!tex || texW != SCREEN_W || texH != SCREEN_H) {
            if (tex) SDL_DestroyTexture(tex);
            tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
            texW = SCREEN_W;
            texH = SCREEN_H;
        }
        SDL_UpdateTexture(tex, nullptr, g_menu.canvas.data(), SCREEN_W * 4);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_Rect dst = PresentRectSdl(winW, winH);
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_RenderPresent(ren);
    }

    if (tex) SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    if (g_strings) FreeLibrary(g_strings);
    return 0;
}
