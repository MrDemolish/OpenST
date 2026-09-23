// The window: how the canvas is presented, and the message loop.
//
// Kept apart from the drawing because it changes for different reasons - input
// handling and window plumbing on one side, what ends up in the canvas on the
// other. Included inside the anonymous namespace, so it sees Menu and the
// globals without any of them having to be exported.
#pragma once

// Where the canvas lands inside the client area. The terrain view matches it
// exactly; the 800x600 artwork screens are scaled up to fit and centred, with
// the aspect ratio kept so the menu never stretches.
void PresentRect(HWND wnd, RECT &dst)
{
    RECT cr{};
    GetClientRect(wnd, &cr);
    int cw = cr.right, ch = cr.bottom;
    if (cw < 1) cw = 1;
    if (ch < 1) ch = 1;
    int dw = cw, dh = ch;
    if (SCREEN_W != cw || SCREEN_H != ch) {
        if (cw * SCREEN_H <= ch * SCREEN_W) { dw = cw; dh = cw * SCREEN_H / SCREEN_W; }
        else                                { dh = ch; dw = ch * SCREEN_W / SCREEN_H; }
    }
    dst.left   = (cw - dw) / 2;
    dst.top    = (ch - dh) / 2;
    dst.right  = dst.left + dw;
    dst.bottom = dst.top + dh;
}

// Client pixel to canvas pixel, so clicks still land on the right button once
// the menu is being scaled.
POINT ToCanvas(HWND wnd, int x, int y)
{
    RECT d{};
    PresentRect(wnd, d);
    int dw = d.right - d.left, dh = d.bottom - d.top;
    POINT p{ x, y };
    if (dw > 0) p.x = (x - d.left) * SCREEN_W / dw;
    if (dh > 0) p.y = (y - d.top)  * SCREEN_H / dh;
    return p;
}

LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CLOSE:
        if (g_menu.screen == SCR_EDITOR && g_menu.edit.dirty && !g_menu.quitRequested) {
            g_menu.EdRequest(4);
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        DestroyWindow(wnd);
        return 0;
    case WM_CREATE:
        // The original steps its animations off timeGetTime inside the idle
        // handler. A timer is the modern equivalent and keeps the loop out of
        // a busy wait; the stepping itself still asks "has interval passed",
        // so the animation runs at its own pace whatever the timer does.
        SetTimer(wnd, TIMER_ANIM, 15, nullptr);
        return 0;
    case WM_TIMER:
        if (wp == TIMER_ANIM) {
            if (g_menu.quitRequested) { DestroyWindow(wnd); return 0; }
            DWORD now = GetTickCount();
            g_menu.sfx.pump();       // dokarm mikser raz na klatke
            bool a = g_menu.Step(now);
            bool b = g_menu.StepBackgrounds(now);   // both, never short circuit
            bool c = g_menu.screen == SCR_TERRAIN && g_menu.StepUnits(now);
            if (a || b || c) InvalidateRect(wnd, nullptr, FALSE);
        }
        return 0;
    case WM_LBUTTONDBLCLK:
        if (g_menu.screen == SCR_TERRAIN) {
            POINT c = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
            if (g_menu.SelectSameKind(c.x, c.y)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
        }
        return 0;
    case WM_LBUTTONDOWN:
        if (g_menu.screen == SCR_EDITOR) {
            POINT c = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
            g_menu.EditorClick(c.x, c.y, false);
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.screen == SCR_TERRAIN) {
            POINT c = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
            // **Panel admina ma jedno wejscie.** Kolejnosc „trafienie
            // w kontrolke -> postawienie na mapie" byla przepisana osobno
            // tutaj i w `sdlmain.cpp` - ta sama pulapka, co przy
            // `PanelClick`, `SelectAt` i `SkClick`.
            if (g_menu.AdminClick(c.x, c.y)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            // Otwarte okno lapie klikniecie przed wszystkim innym.
            if (g_menu.WinClick(c.x, c.y)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            // Uzbrojony przycisk rozkazu przejmuje nastepne klikniecie.
            if (g_menu.ArmedClick(c.x, c.y)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            // Panel przed mapa: minimapa, rozkazy, paleta budowy.
            if (g_menu.MiniHit(c.x, c.y)) {
                g_menu.MiniGoto(c.x, c.y);
                g_menu.miniDrag = true;
                SetCapture(wnd);
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            g_menu.mouseDown = true;
            if (g_menu.PanelClick(c.x, c.y)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            if (g_menu.PlaceBuilding(c.x, c.y)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            // Kolejnosc zaznaczania siedzi w `Menu::SelectAt` - jedno
            // miejsce na oba backendy, tak samo jak przy `PanelClick`.
            bool add = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
            if (g_menu.SelectAt(c.x, c.y, add)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            g_menu.banding = true;                // otherwise drag a box
            g_menu.bandFrom = g_menu.bandTo = c;
            // Zaznaczenia NIE kasujemy - zrobi to SelectInBand, i tylko wtedy,
            // gdy ramka kogos zlapie. Klikniecie w pusta wode ma zostawic
            // oddzial zaznaczony.
            SetCapture(wnd);
            InvalidateRect(wnd, nullptr, FALSE);
        }
        return 0;
    case WM_MBUTTONDOWN:
        if (g_menu.screen == SCR_EDITOR) {        // edytor: srodkowy przewija
            POINT c = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
            g_menu.EditorPanStart(c.x, c.y);
            SetCapture(wnd);
            return 0;
        }
        if (g_menu.screen == SCR_TERRAIN) {       // the camera moved off LPM
            g_menu.dragging = true;
            g_menu.dragFrom = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
            SetCapture(wnd);
        }
        return 0;
    case WM_MBUTTONUP:
        if (g_menu.screen == SCR_EDITOR) {
            g_menu.EditorRelease();
            ReleaseCapture();
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.dragging) { g_menu.dragging = false; ReleaseCapture(); }
        return 0;
    case WM_CAPTURECHANGED:             // przechwyt przepadl - konczymy wszystko
        g_menu.miniDrag = false;
        g_menu.dragging = false;
        g_menu.banding = false;
        return 0;
    case WM_RBUTTONDOWN:
        if(g_menu.screen==SCR_TERRAIN) {
            POINT c=ToCanvas(wnd,short(LOWORD(lp)),short(HIWORD(lp)));
            if(g_menu.PaletteCancelAt(c.x,c.y)) {InvalidateRect(wnd,nullptr,FALSE);return 0;}
        }
        if (g_menu.screen == SCR_EDITOR) {
            POINT c = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
            g_menu.EditorClick(c.x, c.y, true);
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.buildPick >= 0) {          // prawy anuluje stawianie
            g_menu.buildPick = -1;
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.screen == SCR_TERRAIN && !g_menu.sel.empty()) {
            POINT c = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
            // Prawy na obcym to atak, na pustym - ruch.
            if (g_menu.OverBar(c.x, c.y)) return 0;   // panel, nie mapa
            if (!g_menu.OrderAttack(c.x, c.y)) g_menu.OrderMove(c.x, c.y);
            g_menu.curName = "CUR_CONFIRM";   // jednorazowe potwierdzenie
            g_menu.curStep = 0;
            g_menu.oneShot = g_menu.cursors.frames("CUR_CONFIRM") * Menu::CUR_DIV;
            InvalidateRect(wnd, nullptr, FALSE);
        }
        return 0;
    case WM_MOUSEMOVE: {
        POINT cp = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
        int x = cp.x, y = cp.y;
        g_menu.mouse = cp;              // kursor rysuje sie na kazdym ekranie
        if (g_menu.miniDrag) g_menu.MiniGoto(cp.x, cp.y);
        g_menu.PickCursor();
        if (g_menu.screen == SCR_EDITOR) {
            g_menu.EditorMove(x, y);
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.screen == SCR_TERRAIN) {
            // Mysz nie odswieza ekranu sama. Zdarzen ruchu przychodzi wiecej
            // niz klatek, a kazde wymuszalo pelne Compose() - przy jedenastu
            // milisekundach na klatke kolejka rosla szybciej niz malala i
            // ciagniecie ramki zaznaczenia szarpalo. Zegar i tak odswieza co
            // 15 ms, wiec wystarczy zapisac pozycje.
            g_menu.mouse = POINT{ x, y };
            g_menu.PickCursor();
            if (g_menu.banding) {
                g_menu.bandTo = POINT{ x, y };
            } else if (g_menu.dragging) {
                g_menu.camX -= x - g_menu.dragFrom.x;
                g_menu.camY -= y - g_menu.dragFrom.y;
                g_menu.dragFrom = POINT{ x, y };
                g_menu.ClampCamera();
            }
            return 0;
        }
        if (g_menu.SkInput()) {
            int hot = g_menu.skHot, gor = g_menu.campHot;
            g_menu.SkHover(x, y);
            if (hot != g_menu.skHot || gor != g_menu.campHot)
                InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        int hot = g_menu.HitTest(x, y);
        if (hot != g_menu.hot) {
            g_menu.hot = hot;
            InvalidateRect(wnd, nullptr, FALSE);
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        if (g_menu.screen == SCR_EDITOR) {
            g_menu.EditorWheel(GET_WHEEL_DELTA_WPARAM(wp));
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.screen == SCR_TERRAIN) {
            POINT p{ short(LOWORD(lp)), short(HIWORD(lp)) };
            ScreenToClient(wnd, &p);
            p = ToCanvas(wnd, p.x, p.y);
            g_menu.ZoomAt(GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1, p.x, p.y);
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.screen != SCR_SKIRMISH) return 0;
        int notches = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
        g_menu.SkScroll(-notches * 3);
        InvalidateRect(wnd, nullptr, FALSE);
        return 0;
    }
    case WM_KEYDOWN:
        g_menu.OnKey(int(wp));
        InvalidateRect(wnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP:
            if (g_menu.screen == SCR_EDITOR) { g_menu.EditorRelease(); return 0; }
            g_menu.mouseDown = false; {
        POINT cp = ToCanvas(wnd, short(LOWORD(lp)), short(HIWORD(lp)));
        int x = cp.x, y = cp.y;
        // Ciagniecie po minimapie zaczyna LEWY przycisk, wiec konczyc je musi
        // tez lewy. Zwalnianie go dopiero pod srodkowym sprawialo, ze po
        // wyjechaniu poza minimape kamera zostawala przyklejona do myszy.
        if (g_menu.miniDrag) {
            g_menu.miniDrag = false;
            ReleaseCapture();
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.screen == SCR_TERRAIN) {
            if (g_menu.banding) {
                g_menu.banding = false;
                g_menu.bandTo = POINT{ x, y };
                ReleaseCapture();
                // Klikniecie bez ruchu to nie ramka - i nic nie zmienia.
                long w = g_menu.bandTo.x - g_menu.bandFrom.x;
                long h = g_menu.bandTo.y - g_menu.bandFrom.y;
                if (w * w + h * h > 25) {
                    g_menu.SelectInBand();
                    if (!g_menu.sel.empty())
                        g_menu.sfx.play(Menu::sfxSelect(
                            int(g_menu.units[size_t(g_menu.sel[0])].type)));
                }
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            if (g_menu.dragging) { g_menu.dragging = false; ReleaseCapture(); }
            return 0;
        }
        if (g_menu.FullScreenClick()) {
            InvalidateRect(wnd, nullptr, FALSE);
            return 0;
        }
        if (g_menu.SkInput()) {
            if (g_menu.SkClick(x, y)) {
                InvalidateRect(wnd, nullptr, FALSE);
                return 0;
            }
            int b = g_menu.SkHitButton(x, y);
            if (b >= 0) {
                OnSkirmishPress(b); InvalidateRect(wnd, nullptr, FALSE);
                InvalidateRect(wnd, nullptr, FALSE);
            }
            return 0;
        }
        int b = g_menu.HitTest(x, y);
        if (b >= 0) OnPress(b);
        InvalidateRect(wnd, nullptr, FALSE);
        return 0;
    }
    case WM_SIZE:
        g_clientW = LOWORD(lp);
        g_clientH = HIWORD(lp);
        InvalidateRect(wnd, nullptr, FALSE);
        return 0;
    case WM_GETMINMAXINFO:
        reinterpret_cast<MINMAXINFO *>(lp)->ptMinTrackSize = POINT{ 480, 380 };
        return 0;
    case WM_SETCURSOR:
        if (g_menu.screen == SCR_TERRAIN && LOWORD(lp) == HTCLIENT) {
            SetCursor(nullptr);       // the game draws its own, and it animates
            return TRUE;
        }
        break;
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(wnd, &ps);
        g_menu.Compose();
        BITMAPINFO bi{};
        bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth       = SCREEN_W;
        bi.bmiHeader.biHeight      = -SCREEN_H;   // top-down
        bi.bmiHeader.biPlanes      = 1;
        bi.bmiHeader.biBitCount    = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        RECT d{}, cr{};
        PresentRect(wnd, d);
        GetClientRect(wnd, &cr);
        if (d.left > 0 || d.top > 0) {          // letterbox bars
            HBRUSH black = HBRUSH(GetStockObject(BLACK_BRUSH));
            RECT bar;
            bar = RECT{ 0, 0, cr.right, d.top };                    FillRect(dc, &bar, black);
            bar = RECT{ 0, d.bottom, cr.right, cr.bottom };         FillRect(dc, &bar, black);
            bar = RECT{ 0, d.top, d.left, d.bottom };               FillRect(dc, &bar, black);
            bar = RECT{ d.right, d.top, cr.right, d.bottom };       FillRect(dc, &bar, black);
        }
        SetStretchBltMode(dc, COLORONCOLOR);    // keep the pixels crisp
        StretchDIBits(dc, d.left, d.top, d.right - d.left, d.bottom - d.top,
                      0, 0, SCREEN_W, SCREEN_H,
                      g_menu.canvas.data(), &bi, DIB_RGB_COLORS, SRCCOPY);
        EndPaint(wnd, &ps);
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(wnd, msg, wp, lp);
}
