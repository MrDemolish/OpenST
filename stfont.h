// The game's own bitmap font, so menu text is the right shape and colour
// rather than whatever the system dialog font happens to be.
//
// SYS_FONT (record type 3, 1072 bytes) is a descriptor:
//
//   +0x00  u16   number of colour variants (6)
//   +0x02  u32   one .DKD offset per variant, each a plain 530x10 DIB
//   +0x64  u16   glyph count (96)
//   +0x66        glyph table, stride 10: u16 x, y, w, h, character
//
// Those offsets are addresses inside the .DKD that no record starts at, which
// is why they have to be read through Archive::dkdAt.
//
// ccFntTy::WrCh looks a character up by scanning the table's character field,
// and ccFntTy's own glyph index is clamped with
// "if (variantCount <= index) index = 0" - the same clamp is kept here.
//
// The atlas only covers 0x20..0x7E.  The game rasterises anything else from a
// Windows font (hence CreateFontIndirectA in the binary), so DrawText reports
// which characters it could not place and the caller draws those with GDI.
#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "stark.h"

namespace fnt {

struct Glyph { int x = 0, y = 0, w = 0, h = 0; };

class Font {
public:
    // variant: 0 cyan, 1 dim green-grey, 2 green, 3 yellow, 4 grey, 5 cyan
    bool load(const ark::Archive &ar, const std::string &name, int variant)
    {
        std::vector<uint8_t> d = ar.read(name);
        if (d.size() < 0x66 + 10) return false;
        int variants = ark::rd16(&d[0]);
        if (variants <= 0) return false;
        if (variant >= variants) variant = 0;          // ccFntTy clamps the same way
        count = ark::rd16(&d[0x64]);
        if (count <= 0 || size_t(0x66) + size_t(count) * 10 > d.size()) return false;

        glyphs.assign(256, Glyph{});
        have.assign(256, false);
        for (int i = 0; i < count; ++i) {
            const uint8_t *e = &d[0x66 + size_t(i) * 10];
            int ch = ark::rd16(e + 8);
            if (ch < 0 || ch > 255) continue;
            glyphs[size_t(ch)] = Glyph{ ark::rd16(e), ark::rd16(e + 2),
                                        ark::rd16(e + 4), ark::rd16(e + 6) };
            have[size_t(ch)] = true;
            if (glyphs[size_t(ch)].h > lineH) lineH = glyphs[size_t(ch)].h;
        }
        return unpackDib(ar, ark::rd32(&d[2 + size_t(variant) * 4]));
    }

    bool ok() const { return aw > 0 && !atlas.empty(); }
    int  height() const { return lineH; }

    bool hasGlyph(unsigned char c) const { return have[c] && glyphs[c].w > 0; }

    // First row of the glyph box that carries ink, or -1. Used to line the
    // rasterised fallback characters up with the atlas ones.
    int inkTop(unsigned char c) const
    {
        if (!hasGlyph(c)) return -1;
        const Glyph &g = glyphs[c];
        for (int y = 0; y < g.h; ++y)
            for (int x = 0; x < g.w; ++x)
                if (atlas[size_t(g.y + y) * aw + (g.x + x)] & 0xFF000000) return y;
        return -1;
    }

    int charWidth(unsigned char c) const
    {
        return have[c] ? glyphs[c].w + 1 : 0;          // WrCh advances by w plus one
    }

    // One glyph at a top-left anchor; returns how far the pen moves.
    int drawGlyph(uint32_t *dst, int dstW, int dstH, int px, int py, unsigned char c) const
    {
        if (!hasGlyph(c)) return 0;
        const Glyph &g = glyphs[c];
        for (int y = 0; y < g.h; ++y) {
            int ty = py + y;
            if (ty < 0 || ty >= dstH) continue;
            for (int x = 0; x < g.w; ++x) {
                int tx = px + x;
                if (tx < 0 || tx >= dstW) continue;
                uint32_t s32 = atlas[size_t(g.y + y) * aw + (g.x + x)];
                if (!(s32 & 0xFF000000)) continue;              // index 0 is the ground
                dst[size_t(ty) * dstW + tx] = s32 & 0x00FFFFFF;
            }
        }
        return g.w + 1;
    }

    // Colour of the glyph ink, which fallback characters are drawn in too.
    uint32_t ink() const { return inkColour; }

private:
    std::vector<uint32_t> atlas;      // ARGB, alpha 0 where the DIB index was 0
    std::vector<Glyph> glyphs;
    std::vector<bool> have;
    int aw = 0, ah = 0, lineH = 0, count = 0;
    int fallbackW = 6;
    uint32_t inkColour = 0x00ACD4;

    bool unpackDib(const ark::Archive &ar, uint32_t off)
    {
        const uint8_t *h = ar.dkdAt(off, 40);
        if (!h || ark::rd32(h) != 40) return false;
        int w = int(ark::rd32(h + 4)), hh = int(ark::rd32(h + 8));
        int bpp = ark::rd16(h + 14);
        if (w <= 0 || hh == 0 || bpp != 8) return false;
        bool bottomUp = hh > 0;
        if (hh < 0) hh = -hh;
        uint32_t used = ark::rd32(h + 32);
        if (used == 0) used = 256;
        size_t stride = (size_t(w) * 8 + 31) / 32 * 4;
        const uint8_t *all = ar.dkdAt(off, uint32_t(40 + used * 4 + stride * hh));
        if (!all) return false;
        const uint8_t *pal = all + 40;
        const uint8_t *px  = pal + used * 4;

        aw = w; ah = hh;
        atlas.assign(size_t(w) * hh, 0);
        uint32_t best = 0;
        size_t bestN = 0;
        std::vector<size_t> tally(256, 0);
        for (int y = 0; y < hh; ++y) {
            const uint8_t *row = px + stride * size_t(bottomUp ? (hh - 1 - y) : y);
            for (int x = 0; x < w; ++x) {
                uint8_t i = row[x];
                if (i == 0) continue;                       // ground stays clear
                const uint8_t *c = pal + size_t(i) * 4;      // DIB palette is BGRA
                atlas[size_t(y) * w + x] =
                    0xFF000000u | (uint32_t(c[2]) << 16) | (uint32_t(c[1]) << 8) | c[0];
                if (++tally[i] > bestN) {
                    bestN = tally[i];
                    best = atlas[size_t(y) * w + x] & 0x00FFFFFF;
                }
            }
        }
        if (bestN) inkColour = best;                        // the dominant ink
        return true;
    }
};

}  // namespace fnt
