inline int RunTerrainCheck(int map)
{
    int failures = 0;
    g_clientW = 1440; g_clientH = 900;
    g_menu.skSel = map;
    if (!g_menu.OpenTerrain(g_gameDir)) return 1;
    g_menu.fogOn = false;
    g_menu.FitCanvas();
    g_menu.ApplyZoom(5);
    g_menu.CentreCamera();
    g_menu.overOn = true;
    std::vector<uint32_t> reference;
    for (int mode = 0; mode < 8; ++mode) {
        g_menu.terrainDepthTest = (mode & 1) != 0;
        g_menu.terrainClampUV = (mode & 2) != 0;
        g_menu.terrainSeamFix = (mode & 4) != 0;
        g_menu.terrainRejected = 0;
        g_menu.ComposeTerrain();
        int black = 0, holes = 0;
        for (int y = 560; y < 680; ++y) for (int x = 980; x < 1300; ++x) {
            size_t p = size_t(y)*1440+x;
            if (!g_menu.overPix[p]) { ++black; if (g_menu.zBand[p] < Menu::kZFar/2) ++holes; }
        }
        if (map == 26) {
            std::printf("mountain edge: black %d uncovered %d\n", black, holes);
            if (mode == 7 && holes) ++failures;
        }
        if (mode == 0) reference = g_menu.overPix;
        size_t different = 0;
        for (size_t i = 0; i < reference.size(); ++i) different += reference[i] != g_menu.overPix[i];
        std::printf("terraincheck map %d mode %d changed %zu rejected %ld\n", map, mode, different, g_menu.terrainRejected);
        char name[80]; std::snprintf(name, sizeof(name), "terraincheck_%d_%d.raw", map, mode);
        FILE *out = std::fopen(name, "wb");
        if (out) { std::fwrite(g_menu.overPix.data(), 4, g_menu.overPix.size(), out); std::fclose(out); }
    }
    // Isolated triangles verify depth and gap coverage independently of map art.
    g_menu.overOn = false;
    g_menu.terrainDepthTest = g_menu.terrainClampUV = g_menu.terrainSeamFix = true;
    uint8_t texture[land::TILE * land::TILE] = {};
    uint32_t red[256] = {}, green[256] = {};
    red[0] = 0xFF0000; green[0] = 0x00FF00;
    auto reset = [&]() {
        std::fill(g_menu.canvas.begin(), g_menu.canvas.end(), 0);
        g_menu.DepthReset();
    };
    auto triangle = [&](float edge, float depth, const uint32_t *pal) {
        Menu::TriPt p[3] = {};
        p[0].x = 10; p[0].y = 10;
        p[1].x = edge - 10; p[1].y = 10;
        p[2].x = 10; p[2].y = edge - 10;
        for (auto &v : p) v.z = depth;
        g_menu.TriTex(p, texture, pal);
    };
    auto check = [&](bool ok, const char *name) {
        std::printf("terraincheck: %s: %s\n", name, ok ? "OK" : "FAIL");
        if (!ok) ++failures;
    };
    reset(); triangle(100, 5, green); triangle(100, 1, red);
    check(g_menu.canvas[30 * 1440 + 30] == green[0], "far triangle cannot overwrite near triangle");
    reset(); triangle(100, 1, red); triangle(100, 5, green);
    check(g_menu.canvas[30 * 1440 + 30] == green[0], "depth result independent of submission order");
    reset(); triangle(69.9f, 5, red);
    size_t seam = 39 * 1440 + 30; // pixel centre sum = 70, just outside triangle
    check(g_menu.canvas[seam] == red[0] && g_menu.terrainCoverage[seam] == 1, "subpixel crack gets coverage");
    triangle(100, 1, green);
    check(g_menu.canvas[seam] == green[0] && g_menu.terrainCoverage[seam] == 2, "real surface replaces expanded edge");
    triangle(69.9f, 10, red);
    check(g_menu.canvas[seam] == green[0], "expanded edge cannot overwrite real surface");
    return failures ? 1 : 0;
}
