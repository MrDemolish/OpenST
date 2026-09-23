// Sprite strips - record type 6, the format everything animated in the game
// uses, from menu buttons to the seaweed on the sea floor.
//
// A strip is frames back to back, each one led by its own size so they can be
// walked without an index:
//
//   +0   u32 frame size, including this header
//   +4   u32 canvas width        the box the frame is positioned inside
//   +8   u32 canvas height
//   +13  u8  flags - bits 2..4 give ((flags & 0x1C) >> 2) * 2 extra bytes
//             of header before the pixels
//   +14  u16 x, y, w, h          where this frame sits in the canvas
//   +0x16 + extra                the RLE rows
//
// One row of RLE:
//
//   0x00        the whole row is transparent
//   < 0x80      skip that many pixels, leaving them clear
//   0x80..0xBF  literal run of (op & 0x3F) bytes
//   0xC0..0xFF  the next byte repeated (op & 0x3F) times
//
// A row ends once the width is used up.
#pragma once
#include <cstdint>
#include <vector>

#include "stark.h"

namespace spr {

struct Frame {
    int x = 0, y = 0, w = 0, h = 0, canvasW = 0, canvasH = 0;
    std::vector<uint32_t> px;          // ARGB, alpha 0 where the sprite is clear
    bool ok() const { return w > 0 && h > 0 && !px.empty(); }
};

class Strip {
public:
    bool load(const std::vector<uint8_t> &b, const uint32_t pal[256])
    {
        frames.clear();
        size_t pos = 0;
        while (pos + 22 <= b.size()) {
            uint32_t fsize = ark::rd32(&b[pos]);
            if (fsize < 22 || pos + fsize > b.size()) break;
            Frame f;
            if (decode(b, pos, fsize, pal, f)) frames.push_back(std::move(f));
            pos += fsize;
        }
        return !frames.empty();
    }

    int count() const { return int(frames.size()); }

    const Frame *at(int i) const
    {
        if (i < 0 || i >= int(frames.size())) return nullptr;
        return &frames[size_t(i)];
    }

private:
    std::vector<Frame> frames;

    static bool decode(const std::vector<uint8_t> &b, size_t pos, uint32_t fsize,
                       const uint32_t pal[256], Frame &f)
    {
        f.canvasW = int(ark::rd32(&b[pos + 4]));
        f.canvasH = int(ark::rd32(&b[pos + 8]));
        f.x = ark::rd16(&b[pos + 14]);
        f.y = ark::rd16(&b[pos + 16]);
        f.w = ark::rd16(&b[pos + 18]);
        f.h = ark::rd16(&b[pos + 20]);
        if (f.w <= 0 || f.h <= 0) return false;

        size_t src = pos + 0x16 + size_t(((b[pos + 13] & 0x1C) >> 2) * 2);
        size_t end = pos + fsize;
        f.px.assign(size_t(f.w) * f.h, 0);

        for (int row = 0; row < f.h; ++row) {
            if (src >= end) break;
            if (b[src] == 0) { ++src; continue; }       // row entirely clear
            int left = f.w;
            size_t dst = size_t(row) * f.w;
            size_t rowEnd = dst + f.w;
            while (src < end) {
                uint8_t op = b[src++];
                if (op < 0x80) {                        // skip, stays clear
                    dst += op;
                    left -= op;
                } else {
                    int n = op & 0x3F;
                    left -= n;
                    if ((op & 0x40) == 0) {             // literal run
                        for (int i = 0; i < n && src < end && dst < rowEnd; ++i)
                            f.px[dst++] = 0xFF000000u | pal[b[src++]];
                        if (src > end) return true;
                    } else {                            // repeat run
                        if (src >= end) break;
                        uint32_t c = 0xFF000000u | pal[b[src++]];
                        for (int i = 0; i < n && dst < rowEnd; ++i) f.px[dst++] = c;
                    }
                }
                if (left < 1) break;
            }
        }
        return true;
    }
};

}  // namespace spr
