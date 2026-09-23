// Units and buildings - DATA\BOAT and DATA\OBJECT.
//
// Neither archive carries a palette: they are drawn in the scene's palette,
// which is the one the Land set supplies. Checking that pays off - a unit body
// spans 103 palette entries between 10 and 207, and under the Land palette
// those come out as the hulls' whites, browns and blues rather than mud.
//
// Naming, which is easy to get wrong:
//
//   rep, crui, hcru, ...     the body strip, tens of kilobytes
//   rep0 .. rep15            small overlays, only indices 10..15 and a few
//                            percent coverage - the team colour mask, not art
//   <name> type 29           strip/shadow names, hot spot and frame timing;
//                            minimum 80 bytes plus 12 bytes per override
//   <name> type 22           3168 bytes of unit table, 40 of them
//
// So the body is the record whose name carries no number.
#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "stark.h"
#include "stspr.h"
#include "stsequence.h"

namespace units {

// The unit name table, read out of ST.exe at 0x007A9348. Forty entries of six
// bytes, and the code addresses it as (0x007A9342 + type * 6) - so it is
// **one based**: type 1 is "sent", type 40 is "fla3". Same count as the forty
// type 22 records in BOAT.
//
// How a unit's sprites are named, from FUN_004D0310 (E:\__titans\Artem\TLO_dock.cpp):
//
//   LoadSequence(0x0E, arch, table[type], 0x1D);         the hull
//   colour = obj[0x379]; if (colour == 0xFF) colour = obj[0x24];
//   wsprintfA(buf, "%s%1i", table[type], colour);
//   LoadSequence(0x0C, arch, buf, 0x1D);                 the team colour overlay
//
// So "crui" is the body and "crui0".."crui7" are the per-player overlays, one
// per slot - which is why the numbered records carry only palette indices
// 10..15 and a few percent coverage.
class Set {
public:
    // pal is borrowed from the terrain set, which owns the scene's colours.
    bool open(const std::string &gameDir, const uint32_t pal[256])
    {
        for (int i = 0; i < 256; ++i) colours[i] = pal[i];
        bool a = boats.open(gameDir + "\\DATA\\BOAT.DKX",   gameDir + "\\DATA\\BOAT.DKD");
        bool b = objs.open(gameDir + "\\DATA\\OBJECT.DKX", gameDir + "\\DATA\\OBJECT.DKD");
        other.open(gameDir + "\\DATA\\OTHER.DKX",  gameDir + "\\DATA\\OTHER.DKD");
        loaded = a || b;
        return loaded;
    }

    bool ok() const { return loaded; }
    size_t boatCount() const { return boats.count(); }
    size_t objCount() const { return objs.count(); }
    size_t miscCount() const { return other.count(); }
    // Surowe bajty rekordu z OTHER - typ 28 (maski mgly) nie jest paskiem
    // i trzeba go czytac samemu.
    std::vector<uint8_t> miscRaw(const std::string &n) { return other.read(n); }
    size_t miscBytes(const std::string &n) const { const ark::Record *r = other.find(n); return r ? r->unpacked : 0; }
    size_t boatBytes(const std::string &n) const { const ark::Record *r = boats.find(n); return r ? r->unpacked : 0; }

    const spr::Strip *boat(const std::string &name)   { return get(boats, cBoats, name); }
    const spr::Strip *object(const std::string &name) { return get(objs,  cObjs,  name); }
    const spr::Strip *misc(const std::string &name)   { return get(other, cOther, name); }

    const sequence::Timing *objectTiming(const std::string &name) {
        auto found=cObjectTimings.find(name);
        if(found!=cObjectTimings.end()) return found->second.ticks.empty()?nullptr:&found->second;
        sequence::Timing timing;
        const ark::Record *record=objs.find(name);
        if(record && record->type==29) {
            auto bytes=objs.read(name);
            if(bytes.size()>=80) {
                size_t end=0; while(end<32 && bytes[end]) ++end;
                if(end>0 && end<32) {
                    std::string sprite(reinterpret_cast<const char*>(bytes.data()),end);
                    if(const spr::Strip *strip=object(sprite)) timing.load(bytes,strip->count());
                }
            }
        }
        auto result=cObjectTimings.emplace(name,std::move(timing));
        return result.first->second.ticks.empty()?nullptr:&result.first->second;
    }

    // The order marker. STSprGameObjC::LoadActFrame builds the name from
    // "actfr", a size letter and the zoom step: 'b' on the 240x190 canvas for
    // buildings, 's' on 180x140 for boats, then 0..2, then which marker.
    const spr::Strip *actFrame(char size, int zoom, char kind)
    {
        if (zoom < 0) zoom = 0;
        if (zoom > 2) zoom = 2;
        char n[16];
        std::snprintf(n, sizeof(n), "actfr1%c%d%c", size, zoom, kind);
        return misc(n);
    }

private:
    ark::Archive boats, objs, other;
    // One cache per archive. Sharing a single map keyed by name meant a miss in
    // BOAT stored an empty strip under that name and every later lookup in
    // OBJECT or OTHER got the empty one back - which is why the order markers
    // looked absent even though the records were right there.
    std::unordered_map<std::string, spr::Strip> cBoats, cObjs, cOther;
    std::unordered_map<std::string, sequence::Timing> cObjectTimings;
    uint32_t colours[256] = {};
    bool loaded = false;

    const spr::Strip *get(ark::Archive &ar,
                          std::unordered_map<std::string, spr::Strip> &cache,
                          const std::string &name)
    {
        auto it = cache.find(name);
        if (it != cache.end()) return it->second.count() ? &it->second : nullptr;
        spr::Strip s;
        s.load(ar.read(name), colours);
        auto ins = cache.emplace(name, std::move(s));
        return ins.first->second.count() ? &ins.first->second : nullptr;
    }
};


// A placed unit (record type 20) names itself through the table at 0x007A9348:
// forty entries of six bytes, addressed by the code as (0x007A9342 + type * 6),
// so it is ONE BASED. The map records confirm the shape independently - their
// unit field runs 1..40 with 26 distinct values across every map and mission.
//
// Buildings had a table too, at 0x007B8330, but nothing indexes it from a map:
// no shipped map or mission places a building. An earlier reading of this file
// claimed object subtypes selected buildings there and "resolved 98%" - it was
// reading the object's y coordinate, and the tables happen to span 1..101, so
// almost any coordinate landed on a name. Both readings of that field are gone.
constexpr int UNIT_COUNT = 40;

inline const char *unitName(int type)          // 1..40
{
    static const char *kU[UNIT_COUNT] = {
        "sent",        "hunt",        "crui",        "dcbo",
        "minl",        "rai1",        "rpl1",        "ltr1",
        "wor",         "term",        "libe",        "cpl1",
        "ckil",        "dest",        "hcru",        "inva",
        "ldef",        "rai2",        "rpl2",        "ltr2",
        "dol",         "phan",        "aven",        "cpl2",
        "cll",         "trn",         "sss",         "ppr",
        "rep",         "shs",         "dre",         "esc",
        "baa",         "usu",         "gmv",         "epr",
        "scou",        "fla1",        "fla2",        "fla3",
    };
    if (type < 1 || type > UNIT_COUNT) return nullptr;
    return kU[type - 1];
}


// The name the game shows for a unit type, as a string id in st_string.dll.
// FUN_00523410 is a switch on the type that returns it, and the order is not
// the id order - only types 1..5 line up - so it is read case by case.
//
// Keeping the id rather than the text means the names arrive in whatever
// language is installed, through the same LoadStringA the menu already uses.
// Type 12 is the Constructor and type 24 the Assembler.
inline int unitNameId(int type)                // 1..40, 0 when unknown
{
    static const int kId[UNIT_COUNT] = {
        11001, 11002, 11003, 11004, 11005, 11073, 11006, 11072,
        11007, 11009, 11017, 11019, 11010, 11011, 11012, 11013,
        11014, 11020, 11015, 11021, 11016, 11008, 11018, 11071,
        11110, 11111, 11112, 11113, 11114, 11115, 11116, 11117,
        11118, 11119, 11120, 11121, 11055, 11133, 11134, 11135,
    };
    if (type < 1 || type > UNIT_COUNT) return 0;
    return kId[type - 1];
}



// Buildings. AiScript.dfn puts them at TOBJ 50 and up, and the same id means a
// different building per side - TOBJ_SUBCENTER (WS) and TOBJ_DOCKYARD (BO) are
// both 50 - which is why the pointer table at 0x007B8330 carries three twelve
// byte entries per id: White Sharks, Black Octopi, Silicons.
//
//     group = tobj - 50
//
// Maps really do place buildings: a record of type 1000 is one, and its +20 is
// exactly this id. On a skirmish map every player gets two defensive turrets
// whose type gives away the slot's race - 62 HF Cannon for White Sharks, 70
// Light Laser for Black Octopi, 107 PP Pulsar for Silicons.
constexpr int BLD_FIRST  = 50;
constexpr int BLD_GROUPS = 80;

// side 0 White Sharks, 1 Black Octopi, 2 Silicons. Only one side usually has
// artwork for a given id, so a caller that does not know the race can try all
// three and keep whichever resolves.
inline const char *buildingName(int tobj, int side)
{
    static const char *kB[BLD_GROUPS][3] = {
        { "dyaws"    , "dyabo"    , nullptr     },
        { "repd"     , "repd"     , nullptr     },
        { "mfacws"   , "mfacbo"   , nullptr     },
        { "rlabws"   , "rlabbo"   , nullptr     },
        { "sonarws"  , "sonarbo"  , nullptr     },
        { "telews"   , "telebo"   , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { "coriws"   , "coribo"   , nullptr     },
        { "sublws"   , "sublbo"   , nullptr     },
        { "depo"     , "depo"     , nullptr     },
        { "info"     , "info"     , nullptr     },
        { "despws"   , "despbo"   , nullptr     },
        { "hfc"      , "hfc"      , nullptr     },
        { "sto"      , "sto"      , nullptr     },
        { "worm"     , "worm"     , nullptr     },
        { "sha"      , "sha"      , nullptr     },
        { "ultr"     , "ultr"     , nullptr     },
        { "psyh"     , "psyh"     , nullptr     },
        { "plasm"    , "plasm"    , nullptr     },
        { "tls"      , "tls"      , nullptr     },
        { "lla"      , "lla"      , nullptr     },
        { "cas"      , "cas"      , nullptr     },
        { "psta"     , "psta"     , nullptr     },
        { "dol"      , "dol"      , nullptr     },
        { "hla"      , "hla"      , nullptr     },
        { "emc"      , "emc"      , nullptr     },
        { "iso"      , "iso"      , nullptr     },
        { "ppr"      , "ppr"      , nullptr     },
        { "htec"     , "htec"     , nullptr     },
        { "mminews"  , "mminebo"  , nullptr     },
        { "airws"    , "airbo"    , nullptr     },
        { "pca"      , "pca"      , nullptr     },
        { "traws"    , "trabo"    , nullptr     },
        { nullptr    , nullptr    , "comh"      },
        { nullptr    , nullptr    , "chmob"     },
        { nullptr    , nullptr    , "chmil"     },
        { nullptr    , nullptr    , "chen"      },
        { nullptr    , nullptr    , "chre"      },
        { nullptr    , nullptr    , "chdef"     },
        { nullptr    , nullptr    , "chpro"     },
        { nullptr    , nullptr    , "chbio"     },
        { nullptr    , nullptr    , "ars"       },
        { nullptr    , nullptr    , "plgen"     },
        { nullptr    , nullptr    , "bson"      },
        { nullptr    , nullptr    , "corisi"    },
        { nullptr    , nullptr    , "econv"     },
        { nullptr    , nullptr    , "silo"      },
        { nullptr    , nullptr    , "eacc"      },
        { nullptr    , nullptr    , "rpr"       },
        { nullptr    , nullptr    , "recy"      },
        { nullptr    , nullptr    , "siext"     },
        { nullptr    , nullptr    , "gosc"      },
        { nullptr    , nullptr    , "gasc"      },
        { nullptr    , nullptr    , "para"      },
        { nullptr    , nullptr    , "ionr"      },
        { nullptr    , nullptr    , "jump"      },
        { nullptr    , nullptr    , "bioa"      },
        { nullptr    , nullptr    , "sipca"     },
        { nullptr    , nullptr    , "gate"      },
        { nullptr    , nullptr    , "ifgen"     },
        { nullptr    , nullptr    , "mrest"     },
        { "atelews"  , nullptr    , nullptr     },
        { nullptr    , nullptr    , "glsat"     },
        { nullptr    , nullptr    , "sipain"    },
        { nullptr    , nullptr    , "vqb"       },
        { nullptr    , nullptr    , "qpara"     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { "sonarws_clop", nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { nullptr    , nullptr    , nullptr     },
        { "hfc_clop" , "hfc_clop" , nullptr     },
        { "sto_clop" , "sto_clop" , nullptr     },
    };
    int g = tobj - BLD_FIRST;
    if (g < 0 || g >= BLD_GROUPS || side < 0 || side > 2) return nullptr;
    return kB[g][side];
}



// Ruch jednostki w jednostkach swiata na tik.
//
// **JEST to w exe** - w `g_anTObjSpeed` pod **0x007dfc6c**, bajt co cztery
// (`GetSpeed` pod 0x00430750 czyta `[typ*4 + 0x7dfc6c]` dla typow 1..40).
// Wczesniejsza notatka mowila, ze w exe tego nie ma; szukalem przy tablicy
// nazw 0x007A9348 i w rekordach typu 22, a lezy to przy predkosciach POCISKOW,
// bo obsluguje je ta sama funkcja.
//
// Odczyt z exe zgadza sie z przewodnikiem **co do wszystkich czterdziestu
// wpisow**, wiec ponizsza tablica zostaje - ale zrodlem prawdy jest teraz exe.
// Komorka to 201 jednostek, wiec 9 na tik to 0,045 komorki na tik.
//
// Skala: najwolniejsze to 6, wiekszosc 9, najszybsze 12.
inline int unitSpeedRaw(int type)              // 1..40, 0 gdy brak
{
    static const int kSpeed[UNIT_COUNT] = {
        12,  9,  9,  6,  9,  9,  6,  6,  6,  6,
         9,  6, 12,  9,  6,  9,  6,  9,  6,  6,
        12,  9,  9,  6,  6,  6,  6, 12,  6, 12,
         6,  9,  6,  9,  9,  9, 12,  9,  9,  9,
    };
    if (type < 1 || type > UNIT_COUNT) return 0;
    return kSpeed[type - 1];
}


// Dzwiek potwierdzenia rozkazu, z kodu gry: STGroupBoatC::StartReceiveOrderSound
// woła FUN_00493d10, a to jest `switch` po typie jednostki oddajacy identyfikator
// zestawu dzwiekow. Bloki maja rozna dlugosc, wiec zadnego wzoru tu nie ma -
// wczesniejsze `50 + (typ-1)*3` bylo zgadywanka z ksztaltu tabeli SND_.
//
// Kwestie sa wspolne dla frakcji, nie dla lodzi: typy 1-12 mowia nagraniami
// `bos_*`, 13-24 `bor_*`, 25-36 `sirs_*`.
inline int orderSoundId(int type)              // 1..40, 0 gdy brak
{
    static const int kSnd[UNIT_COUNT] = {
        202, 208, 214, 220, 226, 233, 240, 247, 255, 260,
        266, 272, 302, 308, 314, 320, 326, 333, 340, 347,
        355, 360, 368, 374, 402, 410, 418, 424, 430, 437,
        443, 449, 455, 462, 469, 475, 380, 278, 386, 481,
    };
    if (type < 1 || type > UNIT_COUNT) return 0;
    return kSnd[type - 1];
}

// Zaznaczenie: identyfikatory ida blokami, a rozkaz siedzi na drugiej pozycji
// bloku, wiec pierwsza jest najpewniej zaznaczeniem. **Nie potwierdzone w
// kodzie** - funkcji grajacej dzwiek zaznaczenia nie znalazlem.
inline int selectSoundId(int type)
{
    int id = orderSoundId(type);
    return id > 0 ? id - 1 : 0;
}


// Punkty wytrzymalosci, z przewodnika gracza (`guide/stats.tsv`). W exe nie ma
// tego przy tablicy nazw - tak samo jak predkosci.
inline int unitHp(int type)                    // 1..40, 0 gdy brak
{
    static const int kHp[UNIT_COUNT] = {
         300,  500, 1300, 2000,  800, 1200,  800,  400, 1000,  800,
         700,  800,  400,  600, 1500,  800,  900, 1300,  800,  400,
        1000,  700,  900,  800,  800,  500,  600,  400,  600,    0,
        1500,  500,  900, 1200,  700,  200,  300, 3000, 3000, 3000,
    };
    if (type < 1 || type > UNIT_COUNT) return 0;
    return kHp[type - 1];
}

// To samo dla budynkow, indeksowane numerem TOBJ (50 i wyzej).
inline int buildingHp(int tobj)
{
    static const struct { int tobj, hp; } kB[] = {
        { 50, 2000}, { 51, 1500}, { 52, 1200}, { 53, 1000}, { 54,  800}, { 55, 1500},
        { 57,  500}, { 58,  500}, { 59,  800}, { 60,  700}, { 61,  800}, { 62, 1600},
        { 64,  600}, { 65,  500}, { 66,  500}, { 67,  800}, { 68,  700}, { 70, 1200},
        { 72,  500}, { 73,  500}, { 74, 1400}, { 77,  800}, { 78,  500}, { 79,  500},
        { 80,  500}, { 81, 1200}, { 82, 1000}, { 83, 2000}, { 91, 1200}, { 92, 1800},
        { 93,  700}, { 94,  500}, { 95, 1200}, { 96,  500}, { 97,  800}, { 98,  700},
        { 99, 1100}, {100,  600}, {101,  700}, {102, 1100}, {104, 1300}, {105, 1000},
        {106, 1200}, {108, 1300}, {109, 1000}, {110,  900}, {111,  700}, {113, 1400},
        {114,  700}, {115,  800},
    };
    for (size_t i = 0; i < sizeof(kB) / sizeof(kB[0]); ++i)
        if (kB[i].tobj == tobj) return kB[i].hp;
    return 0;
}

}  // namespace units
