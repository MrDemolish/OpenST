// Wejscie natywne Win32: okno, petla komunikatow, tryby z linii polecen.
#include "stapp.h"

#include "stwnd.h"

} // namespace

#include "stcli.h"

int main(int argc, char **argv)
{
    const std::string gameDir = "C:\\Program Files (x86)\\Submarine Titans";
    g_assetDir = (argc > 1) ? argv[1] : "..\\..\\assets\\inter_raw";
    g_strings = LoadLibraryExA((gameDir + "\\st_string.dll").c_str(),
                               nullptr, LOAD_LIBRARY_AS_DATAFILE);
    if (!g_strings)
        std::printf("uwaga: st_string.dll nie wczytany, etykiety beda numerami\n");

    if (!g_menu.Load()) {
        std::printf("brak grafik w %s\n", g_assetDir.c_str());
        std::printf("uruchom najpierw tools/pull_assets.py\n");
        return 1;
    }
    int flcs = g_menu.LoadBackgrounds(gameDir);
    std::printf("tla: %d z %d animacji z system\\inter (%d rekordow w archiwum)\n",
                flcs, FLC_COUNT, int(g_menu.inter.count()));
    std::printf("dzwieki: %s (id %d -> %s, id %d -> %s)\n",
                g_menu.sfx.ok() ? "SOUND\\SOUNDS" : "brak",
                Menu::SFX_OPEN,  g_menu.sfx.resolve(Menu::SFX_OPEN).c_str(),
                Menu::SFX_CLOSE, g_menu.sfx.resolve(Menu::SFX_CLOSE).c_str());

    int rc = RunCli(argc, argv, gameDir);
    if (rc >= 0) return rc;


    WNDCLASSA wc{};
    // Bez CS_DBLCLKS okno nie dostaje WM_LBUTTONDBLCLK w ogole - dwuklik
    // przychodzi wtedy jako dwa zwykle klikniecia.
    wc.style         = CS_DBLCLKS;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandleA(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "STMenu";
    RegisterClassA(&wc);

    RECT r{ 0, 0, SCREEN_W, SCREEN_H };
    DWORD style = WS_OVERLAPPEDWINDOW;      // resizable, maximisable
    AdjustWindowRect(&r, style, FALSE);
    HWND wnd = CreateWindowA("STMenu", "Submarine Titans", style,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             r.right - r.left, r.bottom - r.top,
                             nullptr, nullptr, wc.hInstance, nullptr);
    ShowWindow(wnd, SW_SHOW);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (g_strings) FreeLibrary(g_strings);
    return 0;
}
