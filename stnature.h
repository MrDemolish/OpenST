// Sea life - DATA\NATURE.
//
// The small OBJ_ records a map is full of are not scenery in the abstract:
// each is a creature, and its kind is a number the game maps to a sprite in
// FUN_0057a140:
//
//   230 shark1   231 fish1     232 fish_b    233 fish_gr
//   234 scat     235 zmej      242..245 crab2b
//   246..247 langus1           248..249 octopus1     250..251 medusa1
//
// The names line up with the archive exactly - every one of them is a type 6
// strip in NATURE - and with the record types, each of which turns out to be
// one family: type 50 the fish, 270 the crabs, 280 the octopi, 290 the
// jellyfish, 300 the lobsters.
//
// Like BOAT and OBJECT this archive carries no palette of its own and borrows
// the scene's, which the Land set owns.
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

#include "stark.h"
#include "stspr.h"

namespace nature {

// The kind id a record carries, to the sprite that draws it. Straight out of
// the switch in FUN_0057a140; ranges collapse onto one name there too.
inline const char *spriteFor(uint32_t kind)
{
    switch (kind) {
    case 230: return "shark1";
    case 231: return "fish1";
    case 232: return "fish_b";
    case 233: return "fish_gr";
    case 234: return "scat";
    case 235: return "zmej";
    case 242: case 243: case 244: case 245: return "crab2b";
    case 246: case 247: return "langus1";
    case 248: case 249: return "octopus1";
    case 250: case 251: return "medusa1";
    default:  return nullptr;
    }
}

class Set {
public:
    bool open(const std::string &gameDir, const uint32_t pal[256])
    {
        for (int i = 0; i < 256; ++i) colours[i] = pal[i];
        loaded = ar.open(gameDir + "\\DATA\\NATURE.DKX",
                         gameDir + "\\DATA\\NATURE.DKD");
        cache.clear();
        return loaded;
    }

    bool ok() const { return loaded; }
    size_t count() const { return ar.count(); }

    const spr::Strip *strip(const std::string &name)
    {
        auto it = cache.find(name);
        if (it != cache.end()) return it->second.count() ? &it->second : nullptr;
        spr::Strip s;
        s.load(ar.read(name), colours);
        auto ins = cache.emplace(name, std::move(s));
        return ins.first->second.count() ? &ins.first->second : nullptr;
    }

    const spr::Strip *forKind(uint32_t kind)
    {
        const char *n = spriteFor(kind);
        return n ? strip(n) : nullptr;
    }

private:
    ark::Archive ar;
    std::unordered_map<std::string, spr::Strip> cache;
    uint32_t colours[256] = {};
    bool loaded = false;
};

}  // namespace nature
