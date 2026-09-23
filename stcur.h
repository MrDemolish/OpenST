// Mouse cursors - system\inter.
//
// The cursors are not static bitmaps: every one of them is an animation, which
// is where the feedback for an order comes from. 43 of them, 807 records.
//
//   CUR_<NAME>        type 7, 49 bytes - the descriptor. The index entry's
//                     width field holds the frame count.
//   CUR_<NAME>_NN     type 6, an ordinary sprite strip of one frame.
//   CURSOR_PAL        type 1, a 1x1 DIB carrying nothing but the palette the
//                     cursor frames are drawn in.
//
// Frame counts range from 6 (CUR_ARROW, CUR_MENU) through 24 (CUR_CMD,
// CUR_FIRE, CUR_OWNBOAT) to 60 for CUR_CLOCK.
//
// Names worth knowing: ARROW is the plain pointer, CMD the move order, FIRE the
// attack, OWNBOAT and OWNOBJ what shows over your own things, CONFIRM the
// acknowledgement, and SLU/SLD/SLT/SRU/SRD/SRT/SUP/SDN/SNO the eight edge
// scroll arrows plus the blocked one.
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "stark.h"
#include "stspr.h"

namespace cur {

class Set {
public:
    // Shares the interface archive with the menu backgrounds.
    bool open(const std::string &gameDir)
    {
        if (!ar.open(gameDir + "\\system\\inter.DKX", gameDir + "\\system\\inter.DKD"))
            return false;
        // `CURSOR_PAL` NIE jest paleta kursorow. To zaslepka: **186 z 256
        // wpisow ma zero**, a reszta to domyslna paleta VGA - indeks 2 wychodzi
        // zielony (008000), 3 oliwkowy (808000). Kursor rysowany przez nia byl
        // czarna sylwetka w zielono-zoltej obwodce.
        //
        // W grze kursor idzie przez `ddPutSpriteRLE` na ekran w palecie
        // AKTUALNIE zainstalowanej (Palette.cpp / mfplt.cpp), czyli w rozgrywce
        // w palecie terenu. Domyslna bierzemy z Land00, a `usePalette` podmienia
        // ja na palete wczytanej mapy.
        loadPal(ar, "CURSOR_PAL");
        ark::Archive land;
        if (land.open(gameDir + "\\system\\Land00.DKX",
                      gameDir + "\\system\\Land00.DKD"))
            loadPal(land, "PALETTE");
        loaded = true;
        return true;
    }

    bool ok() const { return loaded; }

private:
    bool loadPal(ark::Archive &a, const char *rec)
    {
        std::vector<uint8_t> pal = a.read(rec);
        if (pal.size() < 40 + 256 * 4) return false;
        for (int i = 0; i < 256; ++i) {
            const uint8_t *e = &pal[40 + size_t(i) * 4];
            colours[i] = (uint32_t(e[2]) << 16) | (uint32_t(e[1]) << 8) | e[0];
        }
        cache.clear();
        return true;
    }

public:

    // How many frames a cursor has, straight out of the descriptor's index
    // entry - no probing.
    int frames(const std::string &name)
    {
        auto it = counts.find(name);
        if (it != counts.end()) return it->second;
        const ark::Record *r = ar.find(name);
        int n = r ? int(r->w) : 0;
        counts.emplace(name, n);
        return n;
    }

    // One frame, cached. Frames are separate records, so this is not a strip.
    const spr::Frame *frame(const std::string &name, int i)
    {
        int n = frames(name);
        if (n <= 0) return nullptr;
        i = ((i % n) + n) % n;
        char key[48];
        std::snprintf(key, sizeof(key), "%s_%02d", name.c_str(), i);
        auto it = cache.find(key);
        if (it == cache.end()) {
            spr::Strip s;
            s.load(ar.read(key), colours);
            it = cache.emplace(key, std::move(s)).first;
        }
        return it->second.count() ? it->second.at(0) : nullptr;
    }

    size_t count() const { return ar.count(); }

private:
    ark::Archive ar;
    std::unordered_map<std::string, spr::Strip> cache;
    std::unordered_map<std::string, int> counts;
    uint32_t colours[256] = {};
public:
    uint32_t colour(int i) const
    { return (i >= 0 && i < 256) ? colours[i] : 0; }

    // CURSOR_PAL to zaslepka: 186 z 256 wpisow jest zerowych, a reszta to
    // domyslna paleta VGA (indeks 2 zielony, 3 oliwkowy) - stad kursor
    // wychodzil czarny z zielono-zolta obwodka. Kursory rysuja sie w palecie
    // AKTUALNIE zainstalowanej, czyli w grze w palecie terenu.
    void usePalette(const uint32_t *p)
    {
        if (!p) return;
        for (int i = 0; i < 256; ++i) colours[i] = p[i];
        cache.clear();
    }
private:
    bool loaded = false;
};

}  // namespace cur
