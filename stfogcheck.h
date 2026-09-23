// Focused fog-of-war checks: screen-space terrain coverage and hidden resources.
inline int RunFogCheck(int map)
{
    g_clientW = 1280; g_clientH = 860;
    g_menu.skMaps = maps::scan(g_gameDir + "\\custom");
    std::vector<maps::Entry> missions = maps::scan(g_gameDir + "\\missions");
    g_menu.skMaps.insert(g_menu.skMaps.end(), missions.begin(), missions.end());
    g_menu.skSel = map;
    if (!g_menu.OpenTerrain(g_gameDir)) return 1;
    g_menu.fogOn = true;
    g_menu.FitCanvas();
    g_menu.ApplyZoom(4);
    g_menu.CentreCamera();

    int failed = 0;
    auto check = [&](bool ok, const char *what) {
        std::printf("fogcheck: %s: %s\n", what, ok ? "OK" : "FAIL");
        if (!ok) ++failed;
    };

    // A fully unknown field must blacken a terrain sample regardless of its
    // height. zBand supplies X+Y, while screen X supplies X-Y.
    g_menu.fog.assign(size_t(g_menu.terr.bw) * size_t(g_menu.terr.bh), Menu::FOG_NONE);
    g_menu.BuildFogField();
    g_menu.DepthReset();
    int ox, oy; g_menu.Origin(ox, oy);
    int px = SCREEN_W / 2, py = SCREEN_H / 2;
    size_t at = size_t(py) * size_t(SCREEN_W) + size_t(px);
    float dif = float(px - ox) / float(g_menu.tileW / 2);
    float wx = float(g_menu.terr.bw) * 0.5f;
    float wy = wx - dif;
    g_menu.zBand[at] = wx + wy;
    g_menu.canvas[at] = 0xFFFFFF;
    g_menu.DrawFogOverlay();
    check(g_menu.canvas[at] == 0, "unknown 3D terrain pixel becomes black");

    // Fully visible uses the same screen-space path and must remain untouched.
    std::fill(g_menu.fog.begin(), g_menu.fog.end(), Menu::FOG_LIVE);
    g_menu.BuildFogField();
    g_menu.canvas[at] = 0xFFFFFF;
    g_menu.DrawFogOverlay();
    check(g_menu.canvas[at] == 0xFFFFFF, "visible terrain pixel stays bright");

    // Unknown deposits used to draw bright sprites because their terrain block
    // was skipped before the final overlay and therefore had no z sample.
    std::fill(g_menu.fog.begin(), g_menu.fog.end(), Menu::FOG_NONE);
    g_menu.BuildFogField();
    std::fill(g_menu.canvas.begin(), g_menu.canvas.end(), 0);
    g_menu.DepthReset();
    g_menu.DrawObjects(ox, oy);
    size_t bright = 0;
    for (uint32_t p : g_menu.canvas) bright += p != 0;
    check(bright == 0, "unknown deposits do not reveal the map");

    // Real frame: no fully unknown terrain sample may survive non-black.
    g_menu.ResetFog();
    g_menu.StepFog(1.0f);
    g_menu.hudOn = false;
    g_menu.ComposeTerrain();
    int solid = 0, leaks = 0;
    int hw = g_menu.tileW / 2;
    g_menu.Origin(ox, oy);
    for (int y = 0; y < SCREEN_H; ++y) for (int x = 0; x < SCREEN_W; ++x) {
        size_t p = size_t(y) * size_t(SCREEN_W) + size_t(x);
        if (g_menu.zBand[p] <= Menu::kZFar / 2) continue;
        float sum = g_menu.zBand[p], dxy = float(x - ox) / float(hw);
        float d = g_menu.FogDarkAt((sum + dxy) * 0.5f, (sum - dxy) * 0.5f);
        if (d < 0.995f) continue;
        ++solid;
        if (g_menu.canvas[p] != 0) ++leaks;
    }
    std::printf("fogcheck: solid samples %d, leaks %d\n", solid, leaks);
    // Some small maps fit wholly inside the starting sight radius and have no
    // solid sample at this camera position; that is valid, while any leak is not.
    check(leaks == 0, "real frame has no holes in solid fog");
    return failed ? 1 : 0;
}
