// The in game HUD - DATA\CONTROLG.
//
// 3242 records: the frame itself, every button, every icon, and the pop-up
// windows the game opens over the map.
//
//   PANEL_BKGND_<race><n>   the whole frame as an 8 bit DIB, one per race and
//                           per resolution: n=0 is 800x600, 1 is 1024x768,
//                           2 is 1280x1024.
//   BOATS_<race>_<nn>       48x33 unit icons
//   OBJS_<nn>, OBJSD_<nn>   building icons, lit and dark
//   BKG_<window>_<race>     the pop-up windows - build, behaviour, artefact,
//                           diplomacy, help
//   BUT_<name>_<race><n>    buttons, one record per state
//
// The frame is 89% palette index 0, and index 0 is black: that hole is the
// viewport, so the frame blits straight over a finished map with index 0 left
// out. Nothing has to be cut out by hand.
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <cstring>
#include <cstdio>

#include "stark.h"
#include "stspr.h"

namespace panel {

struct Image {
    int w = 0, h = 0;
    std::vector<uint8_t> idx;          // palette indices, 0 is transparent
    bool ok() const { return w > 0 && h > 0 && !idx.empty(); }
};

class Set {
public:
    bool open(const std::string &gameDir)
    {
        return ar.open(gameDir + "\\DATA\\CONTROLG.DKX",
                       gameDir + "\\DATA\\CONTROLG.DKD");
    }

    bool ok() const { return ar.count() > 0; }
    // MONEY_FONT i TIMER_FONT to takie same deskryptory typu 3 co SYS_FONT,
    // wiec czyta je ten sam loader - potrzebuje tylko archiwum.
    const ark::Archive &archive() const { return ar; }
    size_t count() const { return ar.count(); }
    const uint32_t *palette() const { return colours; }

    // Which artwork size to use for a canvas. The frame is a border, so the
    // nearest one up is the one that looks least wrong when scaled.
    static int variantFor(int w, int h)
    {
        if (w >= 1152 || h >= 900) return 2;
        if (w >= 912  || h >= 690) return 1;
        return 0;
    }

    // Nie kazde tlo w CONTROLG jest DIB-em typu 1: `BKG_HLPTTREE_0..2`
    // (drzewo technologii) to paski typu 6. Ten sam loader co w units::Set,
    // z wlasnym podrecznym slownikiem.
    const spr::Strip *strip(const std::string &name)
    {
        auto it = strips.find(name);
        if (it != strips.end()) return it->second.count() ? &it->second : nullptr;
        spr::Strip st;
        st.load(ar.read(name), colours);
        auto ins = strips.emplace(name, std::move(st));
        return ins.first->second.count() ? &ins.first->second : nullptr;
    }

    const Image *frame(const char *race, int variant)
    {
        char key[32];
        std::snprintf(key, sizeof(key), "PANEL_BKGND_%s%d", race, variant);
        return image(key);
    }

    // A unit icon: 48x33, one per player colour (1..8) and unit slot (0..43).
    const Image *boatIcon(int player, int type)
    {
        if (player < 1 || player > 8 || type < 1 || type > 44) return nullptr;
        char key[32];
        std::snprintf(key, sizeof(key), "BOATS_%d_%02d", player, type - 1);
        return image(key);
    }

    // Any 8 bit DIB in the archive, kept as indices so index 0 can stay
    // transparent where a caller wants that. The palette is the HUD's own.
    const Image *image(const std::string &key)
    {
        auto it = cache.find(key);
        if (it != cache.end()) return it->second.ok() ? &it->second : nullptr;

        Image im;
        std::vector<uint8_t> b = ar.read(key);
        if (b.size() > 40 + 256 * 4) {
            int w = int(ark::rd32(&b[4])), h = int(ark::rd32(&b[8]));
            bool bottomUp = h > 0;
            if (h < 0) h = -h;
            size_t stride = (size_t(w) + 3) & ~size_t(3);
            size_t data = 40 + 256 * 4;
            if (w > 0 && h > 0 && data + stride * size_t(h) <= b.size()) {
                for (int i = 0; i < 256; ++i) {
                    const uint8_t *e = &b[40 + size_t(i) * 4];
                    colours[i] = (uint32_t(e[2]) << 16) | (uint32_t(e[1]) << 8) | e[0];
                }
                im.w = w;
                im.h = h;
                im.idx.resize(size_t(w) * size_t(h));
                for (int y = 0; y < h; ++y) {
                    const uint8_t *src = &b[data + stride * size_t(bottomUp ? h - 1 - y : y)];
                    std::memcpy(&im.idx[size_t(y) * size_t(w)], src, size_t(w));
                }
            }
        }
        auto ins = cache.emplace(key, std::move(im));
        return ins.first->second.ok() ? &ins.first->second : nullptr;
    }

private:
    ark::Archive ar;
    std::unordered_map<std::string, spr::Strip> strips;
    std::map<std::string, Image> cache;
    uint32_t colours[256] = {};
};

}  // namespace panel
