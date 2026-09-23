// The map catalogue behind the skirmish screen.
//
// Every map is its own .DKX/.DKD pair in the game's custom\ folder, and the
// four records the setup screen needs are all in there:
//
//   TITLE_MISSION  the display name, NUL terminated inside 0x104 bytes
//   DESCRIPTOR     +12 u16 width, +14 u16 height, +16 u8 number of players
//                  (the byte above it is a separate flag - reading the field
//                  as a u32 gives 0x103 and friends instead of 3)
//   SMALL_MAP      a 139x139 DIB, already drawn as the isometric diamond with
//                  its corners on palette index 0
//   3D_MAP         the terrain itself, not needed until a match actually runs
//
// The whole custom\ folder is about 2 MB, so everything is read once at start
// and kept; nothing here has to be lazy.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "stark.h"

namespace maps {

inline uint64_t nextTerrainRevision() { static uint64_t generation=0; return ++generation; }

constexpr int PREVIEW = 139;

// **Tablica napisow, rekord typu 23.** Ten sam format ma deskryptor
// dzwieku `SND_<id>`, i nie jest to zbieg okolicznosci: zapis mapy wola dla
// DESCRIPTION i OBJECTIVES `mfSarSave`, czyli zapis tablicy napisow.
//
//   +8   u32  ile wpisow
//   +24  maska obecnosci, ceil(ile/8) bajtow
//   +..  napisy, kazdy zakonczony zerem
//
// Tekst jest w CP1250 i **nie wolno go filtrowac po ASCII** - polskie znaki
// maja kody 0x80..0xFF, a font gry jest indeksowany bajtem, wiec trafiaja
// w swoje glify bez konwersji.
inline std::vector<std::string> sarStrings(const std::vector<uint8_t> &d)
{
    std::vector<std::string> out;
    if (d.size() <= 25) return out;
    uint32_t cnt = ark::rd32(&d[8]);
    if (cnt == 0 || cnt > 256) return out;
    size_t i = 24 + (cnt + 7) / 8;
    while (i < d.size() && out.size() < cnt) {
        std::string t;
        while (i < d.size() && d[i]) t += char(d[i++]);
        ++i;
        // Pusty wpis nie konczy tablicy - maska mowi, ktore gniazda sa
        // zajete, a puste zdarzaja sie w srodku.
        out.push_back(t);
    }
    while (!out.empty() && out.back().empty()) out.pop_back();
    return out;
}

// **Tablica gniazd graczy w DESCRIPTOR.** Uklad nie jest zgadniety - chodzi
// po niej `SettMapMTy::PrepPlList` (`0x005CD430`), czyli ekran ustawien mapy:
//
//     local_c = g_PlayerSetup + 0x24;
//     do {
//         bVar2 = local_c[1];                 // +1: 0xFF znaczy PUSTE
//         if (bVar2 != 0xff) {
//             ... = *pbVar12;                 // +0
//             ... = pbVar12[2];               // +2
//             ... = *(u32 *)(pbVar12 + 3);
//             ... = *(u32 *)(pbVar12 + 7);
//             ... = *(u32 *)(pbVar12 + 0xb);
//             if (pbVar12[-0x21] == 1) strcmp(pbVar12 - 0x20, ...);
//         }
//         local_c = pbVar12 + 0x51;           // KROK 81
//     } while (local_c < 0x808a70);
//
// czyli wpis zaczyna sie 0x21 przed wskaznikiem: bajt flagi, 32 bajty nazwy,
// potem pola. W pliku mapy pierwszy wskaznik wypada na **+293**, co zgadza
// sie z krokiem: kolejne wpisy sa na 374, 455, 536 i dalej.
//
// **Czego NIE wiem**: co dokladnie znaczy para (+3, +7). Na wszystkich 58
// mapach miesci sie w rozmiarze mapy, a na czesci map pokrywa sie ze srodkiem
// ciezkosci jednostek gracza co do kilku kratek - ale na czesci nie. Dopoki
// tego nie rozstrzygnalem, remake tego NIE UZYWA do stawiania baz; czyta to
// i pokazuje w audycie.
struct MapSlot {
    uint8_t  race = 0;                 // +0, 1 WS / 2 BO / 3 SI
    uint8_t  id = 0;                   // +1, rowny numerowi gniazda, 0xFF puste
    uint8_t  colour = 0;               // +2
    uint32_t x = 0, y = 0, z = 0;      // +3, +7, +0xb - punkt startu i poziom
    std::string name;                  // 32 bajty przed wskaznikiem
    // Bajt **-0x21** wpisu (czyli +17 + 81k w rekordzie). Znaczenie jest
    // odczytane, nie przyjete: `FUN_0056F040` przechodzi po osmiu gniazdach
    // i kazdemu, ktorego (id, rasa) NIE jest wybranym graczem, **ustawia ten
    // bajt na 1**. Czyli 0 znaczy „gniazdo dla czlowieka", 1 „komputer".
    // Na 47 z 58 map dokladnie jedno uzyte gniazdo ma zero; reszta to mapy
    // wieloosobowe, gdzie otwartych jest kilka.
    bool human = false;
    bool used = false;
};

// **Wycofane: pierwszy wpis na +293.** Tam wypada gniazdo numer TRZY - remake
// gubil przez to trzy pierwsze na kazdej mapie. Prawdziwy poczatek jest na
// **+50** i wynika z samych danych: bajty, ktore roznia sie miedzy 58 mapami,
// ukladaja sie w grupy odlegle dokladnie o 0x51 (50, 131, 212, 293, 374, ...),
// a 293 to po prostu czwarta z nich.
//
// Potwierdzaja to trzy niezalezne sprawdziany na wszystkich 58 mapach:
// uzytych gniazd wychodzi **218** (bylo 119) i **wszystkie 218** miesci sie
// w rozmiarze swojej mapy (bylo 118 z 119); pole `race` przyjmuje wylacznie
// **1, 2 i 3**, czyli numeracje `g_byGameMode` (WS / BO / SI); a `id` jest
// rowne numerowi gniazda w kazdym z 218 przypadkow.
//
// Trzeci dword to **poziom glebokosci**, nie nieznane pole: przyjmuje 0, 1, 2
// i 3 (180 / 22 / 12 / 4 razy), czyli miesci sie w pieciu poziomach gry.
constexpr int SLOT_PTR = 50;           // gdzie wypada gniazdo 0
constexpr int SLOT_STRIDE = 0x51;      // krok, wprost z exe
constexpr int SLOT_MAX = 8;            // osmiu graczy, tyle samo co w grze

// Gniazda z powrotem do bajtow `DESCRIPTOR`.
//
// Nadpisujemy **tylko** te pola, ktore `readSlots` czyta - rasa, id, barwa,
// punkt startu i poziom, plus bajt „gniazdo dla czlowieka" pod -0x21. Cala
// reszta rekordu, w tym 32 bajty nazwy i wszystko, czego format nie tlumaczy,
// przechodzi nietknieta. Rekord ma 6553 bajty, a osiem gniazd zajmuje z nich
// 50 + 8*0x51 = 698.
inline void writeSlots(std::vector<uint8_t> &d, const std::vector<MapSlot> &sl)
{
    for (size_t k = 0; k < sl.size() && k < size_t(SLOT_MAX); ++k) {
        size_t p = size_t(SLOT_PTR) + k * SLOT_STRIDE;
        if (p + 15 > d.size()) break;
        d[p]     = sl[k].race;
        d[p + 1] = sl[k].id;
        d[p + 2] = sl[k].colour;
        ark::wr32(&d[p + 3],  sl[k].x);
        ark::wr32(&d[p + 7],  sl[k].y);
        ark::wr32(&d[p + 11], sl[k].z);
        if (p >= 0x21) d[p - 0x21] = uint8_t(sl[k].human ? 0 : 1);
    }
}

inline std::vector<MapSlot> readSlots(const std::vector<uint8_t> &d)
{
    std::vector<MapSlot> out;
    for (int k = 0; k < SLOT_MAX; ++k) {
        size_t p = size_t(SLOT_PTR) + size_t(k) * SLOT_STRIDE;
        if (p + 15 > d.size()) break;
        MapSlot s;
        s.race = d[p];
        s.id = d[p + 1];
        s.colour = d[p + 2];
        s.x = ark::rd32(&d[p + 3]);
        s.y = ark::rd32(&d[p + 7]);
        s.z = ark::rd32(&d[p + 11]);
        if (p >= 0x20) {
            const char *nm = reinterpret_cast<const char *>(&d[p - 0x20]);
            s.name.assign(nm, strnlen(nm, 0x20));
        }
        if (p >= 0x21) s.human = d[p - 0x21] == 0;
        s.used = s.id != 0xff && (s.x || s.y || s.race);
        out.push_back(s);
    }
    return out;
}

struct Entry {
    std::string file;                  // stem, which is what the list shows
    std::string title;                 // TITLE_MISSION
    std::vector<std::string> goals;    // OBJECTIVES - cele misji
    std::vector<std::string> brief;    // DESCRIPTION - odprawa
    std::vector<MapSlot> slots;        // tablica gniazd z DESCRIPTOR
    std::string dkx, dkd;              // kept so the terrain can be read later
    std::string texture;               // TEXTURE, e.g. "land00" - which Land archive to use
    int w = 0, h = 0, players = 0;
    int pw = 0, ph = 0;
    std::vector<uint32_t> preview;     // SMALL_MAP as ARGB, corners clear
};

// ------------------------------------------------------------------ 3D_MAP
//
// The record is a flat array of 9 byte descriptors with no header at all: its
// count is the `w` field of the index entry, and size == w * 9 holds exactly
// on all 25 maps the game ships. Each descriptor is three position bytes and
// then the six byte cell the editor writes:
//
//   +0  level, 0..5          +3  u16 texture A
//   +1  x, cell coordinate   +5  u16 texture B
//   +2  y, cell coordinate   +7  u16 mesh
//
// The order of +1 and +2 is not a guess: mfTMapLoad indexes its slot array as
//
//   ((height/2) * p[0] + (p[2] >> 1)) * (width/2) + (p[1] >> 1)
//
// so p[1] is the column and p[2] the row. Reading them the other way round
// transposes the whole map, which on a symmetric map looks almost right and
// only shows up as parts of the terrain appearing flipped.
//
// x and y are always even because the terrain is stored per 2x2 block, which
// is what the map allocator sets up: (w/2)*(h/2) blocks, six slots each.
//
// Only blocks that carry something are stored, so the array is sparse - a
// 80x80 map has 1600 blocks and 9600 possible slots but only ~1900 entries.
//
// Verified by rendering the highest level per block and comparing against the
// map's own SMALL_MAP thumbnail: same shapes, same symmetry.
// ------------------------------------------------------------- dekoracje
//
// STAlgaC::GetMessage (00575cb0) reads count at +20 and starts at +24.
// Shipped editor files have one EXTRA 146-byte allocation at the end; it
// is not part of the header. The runtime save path emits exactly 24+146*N.
// Entry: signed shorts XYZ, primary name[64] at +6, secondary name[64]
// at +70, signed frame at +134 (negative = animated), two fields at +138.
constexpr int DECOR_STRIDE = 146;
constexpr int DECOR_BASE   = 24;
constexpr int DECOR_FRAME  = 134;
constexpr int WORLD_PER_CELL = 100;
// One terrain level is also 100 world units - the mesh spans 20 units across a
// block of two cells and a level is 10 of them, so 10 mesh units = 100 world.
constexpr int WORLD_PER_LEVEL = 100;

struct Decor {
    uint16_t x = 0, y = 0, z = 0;
    int32_t  frame = 0;
    std::string name;
    // Oryginalne 146 bajtow wpisu - ta sama zasada, co przy obiekcie:
    // nadpisujemy pola, ktore rozumiemy, reszta przechodzi bez zmian.
    std::vector<uint8_t> raw;
};

// The small OBJ_ records - everything that is not the decoration list. They
// share a header, and the two kinds that matter here are told apart by it:
//
//   +0   u32 type      what kind of record this is
//   +4   u32 owner     the player number, 255 for neutral
//   +8   u32 kind      1 belongs to a player, 2 is scenery
//
// The rest of the record depends on the type, and the three that matter are
// laid out differently. Reading one layout into all of them is what put objects
// in impossible places:
//
//   20   105 bytes  a placed unit. +20 player slot 0..7, +24 unit type 1..40,
//                   then x and y as a pair of u16 at +28 and +30.
//   90    40 bytes  a resource deposit. +20 x, +24 y, +32 amount, +36 which
//                   resource - 221 and 222 in the shipped maps.
//   1000  87 bytes  a placed BUILDING. +20 is its TOBJ type (50 and up),
//                   +24 x, +28 y, and +44 an optional fifteen byte script
//                   name - the named ones read "TLS12", "CentComp",
//                   "TranCenter", which is what settled it.
//
// Type 10 is the big decoration list; 50, 270, 280, 290 and 300 are sea life.
constexpr uint32_t OBJ_UNIT     = 20;
constexpr uint32_t OBJ_RESOURCE = 90;
constexpr uint32_t OBJ_BUILDING = 1000;

// **Ten numer to nie TOBJ - to KLASA obiektu**, i nie trzeba jej zgadywac.
// `STPlaySystemC::GetMessage` enumeruje rekordy `OBJ_*` i podaje kazdy do
// `FUN_0054cdd0`, ten zamienia pole +0 na identyfikator klasy przez tablice
// par pod `0x007C8238` (46 wpisow), a `AppClassTy::CreateObject` szuka go
// w tablicy {klasa, konstruktor} pod `0x007CA770` i `0x007CA9D8`. Wskazniki
// prowadza do zaslepek skoku, wiec trzeba je rozwinac - po rozwinieciu
// wychodzi pelny spis:
//
//   10 STAlgaC     20 STBoatC      50 STFishC     60 STGroupBoatC
//   70 TraksClassTy 80 VisibleClassTy 90 STResourceC 120 STTorpC
//   140 STSharkC   170 STLBombC    180 STMBombC   190 STRubbishC
//   210 STExplosion 220 STFieldC   230 STVolcanoC 240 STJumpMineC
//   250 STJellyGunC 260 STJellyManC 270/300 STCrabC 280/290 STOctopusC
//   320 STColl3C   360 STLightC    370 STArtiafactC 380 STBHEShellC
//   390 JumpManagC 400 STSatC      420 STContainerC 430 STMineSetC
//   440 STDestC    450 STManBasisC 1000 TLOBaseTy  1001 TLOEmbryoTy
//
// Stad wiadomo, ze **typ 140 to rekin** (`STSharkC::GetTObjType` zwraca 0xe6,
// czyli TOBJ_SHARK 230, a `nature::spriteFor(230)` to `shark1`), typ 230 to
// **wulkan**, a typ 430 to **mina polozona na mapie**.

// The scenery types are not scenery: each is a family of sea creature, and the
// kind number inside picks the sprite (see stnature.h). Type 50 keeps that
// number at +36, the other four at +32 - the record layouts really do differ.
// Rekin nie ma tego pola wcale: jego rodzaj jest **w klasie**, bo
// `STSharkC::GetTObjType` zawsze zwraca 230.
constexpr uint32_t OBJ_FISH     = 50;
constexpr uint32_t OBJ_SHARK    = 140;
constexpr uint32_t OBJ_CRAB     = 270;
constexpr uint32_t OBJ_OCTOPUS  = 280;
constexpr uint32_t OBJ_JELLY    = 290;
constexpr uint32_t OBJ_LOBSTER  = 300;
constexpr uint32_t OBJ_VOLCANO  = 230;
constexpr uint32_t OBJ_MINE     = 430;

constexpr uint32_t KIND_SHARK   = 230;      // to, co zwraca GetTObjType

inline bool isCreature(uint32_t t)
{
    return t == OBJ_FISH || t == OBJ_SHARK || t == OBJ_CRAB ||
           t == OBJ_OCTOPUS || t == OBJ_JELLY || t == OBJ_LOBSTER;
}

struct Object {
    uint32_t type = 0, owner = 0, amount = 0, subtype = 0;
    int x = 0, y = 0;
    // Poziom glebokosci, 0..4. **Bezwzgledny, nie wysokosc nad dnem** -
    // zmierzone na 5423 stworzeniach ze wszystkich 58 map: rozklad (z, szczyt
    // terenu pod spodem) siedzi na przekatnej (z=0/top0 3317, z=1/top1 235,
    // z=2/top2 215, z=3/top3 130), a ponizej szczytu wypada tylko 5%. Gdyby z
    // liczylo sie od dna, nie bylo by zadnego powodu, zeby korelowalo ze
    // szczytem terenu. `STSharkC::CreateShark` sprawdza zreszta `4 < z` jako
    // blad, czyli to te same piec poziomow, co u lodzi.
    int z = 0;
    bool spawned = false;              // not in the file: the opening force
    std::string name;                  // buildings only, and usually empty
    // **Oryginalne bajty rekordu i jego nazwa.** Edytor nadpisuje tylko te
    // pola, ktore rozumie, a reszte przepisuje bez zmian - to samo, co robi
    // zapis terenu, tylko na obiekcie. Kilkanascie bajtow kazdej klasy nie ma
    // jeszcze przypisanego znaczenia (`140 +44/+48`, `430 +60..+77`,
    // `20 +32/+34`) i wlasnie dlatego musza przejsc nietkniete.
    //
    // Pusta nazwa znaczy „obiekt postawiony w edytorze" - taki dostaje nazwe
    // przy zapisie i bajty sklonowane z szablonu swojej klasy.
    std::vector<uint8_t> raw;
    std::string rec;
};

constexpr int LEVELS = 6;

struct Cell {
    uint8_t  level = 0, y = 0, x = 0;
    uint16_t texA = 0, texB = 0, mesh = 0;
};

struct Terrain {
    uint64_t revision = 0;
    int bw = 0, bh = 0;                // in 2x2 blocks
    std::vector<Cell> cells;
    std::vector<int8_t> top;           // highest level per block, -1 when empty
    std::vector<uint8_t> mask;         // one bit per level, so a single layer can be shown
    std::vector<uint16_t> tex;         // texture A per block per level, bw*bh*LEVELS
    // Druga warstwa. mfTMapLoad podaje ja jako drugi argument do
    // _mfTMapSetTxtParam, ktory dla niej dodatkowo wola mfImgGetTransp -
    // czyli nakladka z kolorem kluczujacym. Zmierzone: kafle uzywane jako
    // texB maja 13-14 procent indeksu 255, bazowe zero, wiec kluczem jest 255.
    std::vector<uint16_t> tex2;
    std::vector<Decor> decor;          // sea floor dressing, world coordinates
    // Nazwa rekordu `OBJ_*`, z ktorego przyszla lista dekoracji - zapis musi
    // podmienic TEN rekord, a nie zgadywac, ktory to byl.
    std::string decorRec;
    // Nazwy rekordow, ktore przy wczytaniu daly obiekt. Potrzebne przy
    // zapisie, zeby odroznic „obiekt zdjety w edytorze" (rekord byl, obiektu
    // juz nie ma - rekord ma NIE wyjsc) od „rekord, ktorego nie parsujemy"
    // (ma wyjsc nietkniety). Bez tego usuwanie albo nie dzialaloby, albo
    // kasowalo rekordy, ktorych edytor w ogole nie rozumie.
    std::vector<std::string> objRecs;
    std::vector<uint16_t> meshId;      // mesh per block per level, bw*bh*LEVELS
    std::vector<Object> objects;       // starts and deposits

    uint16_t meshAt(int bx, int by, int level) const
    {
        if (bx < 0 || by < 0 || bx >= bw || by >= bh || level < 0 || level >= LEVELS) return 0;
        return meshId[(size_t(level) * bh + by) * bw + bx];
    }
    int perLevel[LEVELS] = {};

    uint16_t tex2At(int bx, int by, int level) const
    {
        if (bx < 0 || by < 0 || bx >= bw || by >= bh || level < 0 || level >= LEVELS)
            return 0;
        size_t i = (size_t(level) * size_t(bh) + size_t(by)) * size_t(bw) + size_t(bx);
        return i < tex2.size() ? tex2[i] : 0;
    }

    uint16_t texAt(int bx, int by, int level) const
    {
        if (bx < 0 || by < 0 || bx >= bw || by >= bh || level < 0 || level >= LEVELS) return 0;
        return tex[(size_t(level) * bh + by) * bw + bx];
    }

    bool hasLevel(int bx, int by, int level) const
    {
        if (bx < 0 || by < 0 || bx >= bw || by >= bh) return false;
        return (mask[size_t(by) * bw + bx] >> level) & 1;
    }

    int topAt(int bx, int by) const
    {
        if (bx < 0 || by < 0 || bx >= bw || by >= bh) return -1;
        return top[size_t(by) * bw + bx];
    }

    // Which shelf a decoration stands on, from its own z.
    //
    // A block can carry several levels, so the top one is the wrong answer for
    // anything sitting on a lower shelf - which is 13.7% of all decorations
    // across the 25 shipped maps, and is why plants ended up floating over
    // cliffs they were meant to grow at the foot of.
    //
    // z is in world units, 100 to a level. Large values name the shelf
    // outright (200, 400, 498 land on levels 2, 4, 5); small ones are a few
    // units of growth above whatever the block does have. Taking the highest
    // shelf that does not stand above z covers both, and falling back to the
    // lowest present handles a block whose only shelf is already above z.
    int levelForZ(int bx, int by, int z) const
    {
        if (bx < 0 || by < 0 || bx >= bw || by >= bh) return -1;
        int pick = -1, lowest = -1;
        for (int L = 0; L < LEVELS; ++L) {
            if (!hasLevel(bx, by, L)) continue;
            if (lowest < 0) lowest = L;
            if (L * WORLD_PER_LEVEL <= z + WORLD_PER_LEVEL / 2) pick = L;
        }
        return pick >= 0 ? pick : lowest;
    }
};

// The starting force a skirmish map does not carry.
//
// A campaign mission places its units as records of type 20; a skirmish map
// places none at all - only two defensive turrets per player - so the game
// spawns the opening force itself. The code that does it is the run of forty
// __CreateObjPl calls in the AI event module, and it branches first on the
// player's race and then on a three way setting:
//
//   White Sharks   1x cpl1 Constructor          + sent, sent, hunt  + crui, rai1
//   Black Octopi   1x cpl2 Assembler            + ckil, ckil, dest  + hcru, rai2
//   Silicons       3x cll  Capsule Prototype    + 3x shs            + esc, usu
//
// The first column is the default, and it is what a player actually gets.
//
// The race is not written down either, but the turrets give it away: the map
// places the one belonging to the slot's side, and those three ids differ.
constexpr uint32_t TURRET_WS = 62;      // HF Cannon
constexpr uint32_t TURRET_BO = 70;      // Light Laser
constexpr uint32_t TURRET_SI = 107;     // PP Pulsar

constexpr uint32_t UNIT_CONSTRUCTOR = 12;
constexpr uint32_t UNIT_ASSEMBLER   = 24;
constexpr uint32_t UNIT_CAPSULE     = 25;

inline int sideOfTurret(uint32_t tobj)
{
    if (tobj == TURRET_WS) return 0;
    if (tobj == TURRET_BO) return 1;
    if (tobj == TURRET_SI) return 2;
    return -1;
}

// Fill in what the game would spawn, for every player the map gives turrets to.
// Nothing is invented: the units and their counts come from the spawn code, the
// side from the turret, and the place from the player's own buildings.
inline void addDefaultSpawn(std::vector<Object> &objs)
{
    struct Slot { int side = -1, n = 0; long sx = 0, sy = 0; };
    Slot slot[8];
    bool anyUnit = false;
    for (const Object &o : objs) {
        if (o.type == OBJ_UNIT) { anyUnit = true; break; }
    }
    if (anyUnit) return;                        // a mission places its own
    for (const Object &o : objs) {
        if (o.type != OBJ_BUILDING || o.owner >= 8) continue;
        int side = sideOfTurret(o.subtype);
        if (side < 0) continue;
        Slot &s = slot[o.owner];
        s.side = side;
        s.sx += o.x;
        s.sy += o.y;
        ++s.n;
    }
    for (uint32_t pl = 0; pl < 8; ++pl) {
        const Slot &s = slot[pl];
        if (s.side < 0 || s.n == 0) continue;
        uint32_t type = s.side == 0 ? UNIT_CONSTRUCTOR
                      : s.side == 1 ? UNIT_ASSEMBLER : UNIT_CAPSULE;
        int count = s.side == 2 ? 3 : 1;        // Silicons open with three
        for (int i = 0; i < count; ++i) {
            Object u;
            u.type = OBJ_UNIT;
            u.owner = pl;
            u.subtype = type;
            u.x = int(s.sx / s.n) + (i - count / 2);
            u.y = int(s.sy / s.n) + 1;
            u.spawned = true;
            objs.push_back(u);
        }
    }
}

// The decoration list, found by taking the OBJ_ record that is far bigger than
// the rest - the others are a few dozen bytes of start positions and deposits.
inline void loadDecor(ark::Archive &ar, std::vector<Decor> &out,
                      std::vector<Object> &objs, std::string *decorRec = nullptr,
                      std::vector<std::string> *objRecs = nullptr)
{
    out.clear();
    objs.clear();
    if (decorRec) decorRec->clear();
    if (objRecs) objRecs->clear();
    std::vector<uint8_t> best;
    for (const std::string &n : ar.names()) {
        if (n.compare(0,4,"OBJ_") != 0) continue;
        const ark::Record *r = ar.find(n);
        if (!r) continue;
        std::vector<uint8_t> record = ar.read(n);
        // A decoration list can contain just one entry (or no entries).
        // Its class identifies it; a size threshold loses small edited lists.
        if (record.size() >= size_t(DECOR_BASE) && ark::rd32(record.data()) == 10) {
            if (record.size() > best.size()) {
                best = std::move(record);
                if (decorRec) *decorRec = n;
            }
            continue;
        }
        if (r->unpacked < 2000) {                       // a unit, start or deposit
            std::vector<uint8_t> s = std::move(record);
            if (s.size() >= 36) {
                Object o;
                o.type  = ark::rd32(&s[0]);
                o.owner = ark::rd32(&s[4]);
                o.raw   = s;
                o.rec   = n;
                size_t oldCount = objs.size();
                if (o.type == OBJ_UNIT && s.size() >= 32) {
                    o.subtype = ark::rd32(&s[24]);          // 1..40, the unit table
                    o.x = ark::rd16(&s[28]);
                    o.y = ark::rd16(&s[30]);
                    // STBoatC::GetMessage at 0044efdd/0044f241 reads XYZ
                    // as three consecutive shorts, including the saved depth.
                    o.z = ark::rd16(&s[32]);
                    objs.push_back(o);
                } else if (o.type == OBJ_BUILDING && s.size() >= 32) {
                    o.subtype = ark::rd32(&s[20]);          // TOBJ, 50 and up
                    o.x = int(ark::rd32(&s[24]));
                    o.y = int(ark::rd32(&s[28]));
                    // **`+32` to poziom z budynku** (0..4 - dokladnie piec
                    // poziomow gry). Loader go nie czytal, wiec zapis wpisywal
                    // tam zero i piec budynkow na mapie 26 traci swoja
                    // wysokosc. Zlapal to sprawdzian „bajt w bajt".
                    if (s.size() >= 36) o.z = int(ark::rd32(&s[32]));
                    if (s.size() >= 59) {
                        const char *p = reinterpret_cast<const char *>(&s[44]);
                        o.name.assign(p, strnlen(p, 15));   // script handle
                    }
                    objs.push_back(o);
                } else if (isCreature(o.type)) {
                    o.x = int(ark::rd32(&s[20]));
                    o.y = int(ark::rd32(&s[24]));
                    o.z = int(ark::rd32(&s[28]));
                    // Rekin trzyma rodzaj w klasie, nie w rekordzie.
                    o.subtype = o.type == OBJ_SHARK
                              ? KIND_SHARK
                              : ark::rd32(&s[o.type == OBJ_FISH ? 36 : 32]);
                    objs.push_back(o);
                } else if (o.type == OBJ_VOLCANO && s.size() >= 56) {
                    // `STVolcanoC`: +20 x, +24 y, +28 z (na wszystkich 44
                    // rekordach zero - wulkan stoi na dnie), +44 i +48 stale
                    // 2 i 5, +52 wariant 0/1/2.
                    o.x = int(ark::rd32(&s[20]));
                    o.y = int(ark::rd32(&s[24]));
                    o.z = int(ark::rd32(&s[28]));
                    o.subtype = ark::rd32(&s[52]);
                    objs.push_back(o);
                } else if (o.type == OBJ_MINE && s.size() >= 36) {
                    // `STMineSetC`: mina juz polozona przez autora mapy.
                    // +4 wlasciciel, +20 numer broni (166/167/175 - to samo
                    // pasmo 0x96..0xbf, co pociski), +24 x, +28 y, +32 z.
                    o.subtype = ark::rd32(&s[20]);
                    o.x = int(ark::rd32(&s[24]));
                    o.y = int(ark::rd32(&s[28]));
                    o.z = int(ark::rd32(&s[32]));
                    objs.push_back(o);
                } else if (o.type == OBJ_RESOURCE) {
                    o.x = int(ark::rd32(&s[20]));
                    o.y = int(ark::rd32(&s[24]));
                    // **`+28` to poziom z zloza** (0..3). Tak samo jak przy
                    // budynku loader go nie czytal, a zapis wpisywal tam zero
                    // - na 30 z 58 map. Ta sama pomylka, ten sam sprawdzian.
                    o.z = int(ark::rd32(&s[28]));
                    o.amount  = ark::rd32(&s[32]);
                    o.subtype = ark::rd32(&s[36]);          // 221 or 222
                    objs.push_back(o);
                }
                // Only editable classes belong here. Group/unknown records
                // must survive Save As even though they have no scene object.
                if (objRecs && objs.size() != oldCount) objRecs->push_back(n);
            }
            continue;
        }
        if (r->unpacked <= best.size()) continue;
        std::vector<uint8_t> b = ar.read(n);
        if (b.size() > best.size()) {
            best = std::move(b);
            if (decorRec) *decorRec = n;
        }
    }
    if (best.size() < size_t(DECOR_BASE + DECOR_STRIDE)) return;

    size_t count = std::min(size_t(ark::rd32(&best[20])),
                            (best.size() - DECOR_BASE) / DECOR_STRIDE);
    for (size_t i = 0; i < count; ++i) {
        size_t p = DECOR_BASE + i * DECOR_STRIDE;
        Decor d;
        d.x = ark::rd16(&best[p]);
        d.y = ark::rd16(&best[p + 2]);
        d.z = ark::rd16(&best[p + 4]);
        d.frame = int32_t(ark::rd32(&best[p + DECOR_FRAME]));
        d.raw.assign(best.begin() + long(p), best.begin() + long(p + DECOR_STRIDE));
        for (size_t k = p + 6; k < p + 70 && best[k]; ++k) d.name += char(best[k]);
        out.push_back(std::move(d));
    }
}

inline bool loadTerrain(const Entry &e, Terrain &t)
{
    ark::Archive ar;
    if (!ar.open(e.dkx, e.dkd, true)) return false;
    loadDecor(ar, t.decor, t.objects, &t.decorRec, &t.objRecs);
    addDefaultSpawn(t.objects);
    std::vector<uint8_t> b = ar.read("3D_MAP");
    if (b.size() < 9) return false;

    t.bw = e.w / 2;
    t.bh = e.h / 2;
    if (t.bw <= 0 || t.bh <= 0) return false;
    t.top.assign(size_t(t.bw) * t.bh, -1);
    t.mask.assign(size_t(t.bw) * t.bh, 0);
    t.tex.assign(size_t(t.bw) * t.bh * LEVELS, 0);
    t.tex2.assign(size_t(t.bw) * t.bh * LEVELS, 0);
    t.meshId.assign(size_t(t.bw) * t.bh * LEVELS, 0);
    t.cells.clear();
    std::fill(std::begin(t.perLevel), std::end(t.perLevel), 0);
    t.cells.reserve(b.size() / 9);

    for (size_t p = 0; p + 9 <= b.size(); p += 9) {
        Cell c;
        c.level = b[p];
        c.x     = b[p + 1];
        c.y     = b[p + 2];
        c.texA  = ark::rd16(&b[p + 3]);
        c.texB  = ark::rd16(&b[p + 5]);
        c.mesh  = ark::rd16(&b[p + 7]);
        if (c.level >= LEVELS) continue;
        int bx = c.x / 2, by = c.y / 2;
        if (bx >= t.bw || by >= t.bh) continue;
        ++t.perLevel[c.level];
        int8_t &slot = t.top[size_t(by) * t.bw + bx];
        if (int8_t(c.level) > slot) slot = int8_t(c.level);
        t.mask[size_t(by) * t.bw + bx] |= uint8_t(1u << c.level);
        t.tex[(size_t(c.level) * t.bh + by) * t.bw + bx] = c.texA;
        t.tex2[(size_t(c.level) * t.bh + by) * t.bw + bx] = c.texB;
        t.meshId[(size_t(c.level) * t.bh + by) * t.bw + bx] = c.mesh;
        t.cells.push_back(c);
    }
    t.revision=nextTerrainRevision();
    return !t.cells.empty();
}

// Tablice pochodne liczone od nowa z `cells`.
//
// Edytor zmienia **tylko** `cells` - to one sa rekordem. Wszystko inne
// (`top`, `mask`, `tex`, `tex2`, `meshId`, `perLevel`) jest z nich wyliczone,
// wiec po kazdej zmianie musi zostac przeliczone w jednym miejscu. Gdyby
// edytor poprawial je osobno, rozjechalyby sie z rekordem - a zapisuje sie
// rekord, nie tablice.
inline void rebuildDerived(Terrain &t)
{
    t.revision=nextTerrainRevision();
    t.top.assign(size_t(t.bw) * t.bh, -1);
    t.mask.assign(size_t(t.bw) * t.bh, 0);
    t.tex.assign(size_t(t.bw) * t.bh * LEVELS, 0);
    t.tex2.assign(size_t(t.bw) * t.bh * LEVELS, 0);
    t.meshId.assign(size_t(t.bw) * t.bh * LEVELS, 0);
    for (int i = 0; i < LEVELS; ++i) t.perLevel[i] = 0;
    for (const Cell &c : t.cells) {
        if (c.level >= LEVELS) continue;
        int bx = c.x / 2, by = c.y / 2;
        if (bx < 0 || by < 0 || bx >= t.bw || by >= t.bh) continue;
        ++t.perLevel[c.level];
        int8_t &slot = t.top[size_t(by) * t.bw + bx];
        if (int8_t(c.level) > slot) slot = int8_t(c.level);
        t.mask[size_t(by) * t.bw + bx] |= uint8_t(1u << c.level);
        t.tex[(size_t(c.level) * t.bh + by) * t.bw + bx] = c.texA;
        t.tex2[(size_t(c.level) * t.bh + by) * t.bw + bx] = c.texB;
        t.meshId[(size_t(c.level) * t.bh + by) * t.bw + bx] = c.mesh;
    }
}

// Kolejnosc komorek w pliku to (poziom, y, x) - ta sama, ktora `mfTMapLoad`
// indeksuje tablice kafli, i sprawdzona na mapach gry (zero inwersji).
// Po kazdej zmianie trzeba ja przywrocic, inaczej zapisany rekord ma inny
// uklad niz rekordy gry.
inline void sortCells(Terrain &t)
{
    std::sort(t.cells.begin(), t.cells.end(), [](const Cell &a, const Cell &b) {
        if (a.level != b.level) return a.level < b.level;
        if (a.y != b.y) return a.y < b.y;
        return a.x < b.x;
    });
}

// Teren z powrotem na bajty rekordu `3D_MAP`.
//
// **Jest to wierna kopia, i to jest zmierzone**: loader odrzuca komorke
// o poziomie spoza zakresu albo poza mapa, wiec gdyby taka istniala, zapis
// by ja zgubil. Na wszystkich 58 mapach gry odrzuconych jest **zero** ze
// 154 658 komorek, a kazdy rekord `3D_MAP` dzieli sie przez 9 bez reszty -
// czyli `cells` niesie caly rekord, co do bajta, w kolejnosci z pliku.
inline std::vector<uint8_t> serializeCells(const Terrain &t)
{
    std::vector<uint8_t> b(t.cells.size() * 9);
    for (size_t i = 0; i < t.cells.size(); ++i) {
        const Cell &c = t.cells[i];
        uint8_t *p = &b[i * 9];
        p[0] = c.level;
        p[1] = c.x;
        p[2] = c.y;
        ark::wr16(p + 3, c.texA);
        ark::wr16(p + 5, c.texB);
        ark::wr16(p + 7, c.mesh);
    }
    return b;
}

// Obiekt z powrotem na bajty rekordu.
//
// Nadpisywane sa **wylacznie** pola, ktore ten plik ma opisane - reszta bajtow
// zostaje taka, jaka przyszla z pliku albo z szablonu. Uklad kazdej klasy jest
// inny i to jest cala trudnosc: czytanie jednego ukladu we wszystkie kiedys
// stawialo obiekty w niemozliwych miejscach.
inline std::vector<uint8_t> serializeObject(const Object &o)
{
    std::vector<uint8_t> s = o.raw;
    if (s.size() < 36) return s;
    ark::wr32(&s[0], o.type);
    ark::wr32(&s[4], o.owner);
    if (o.type == OBJ_UNIT && s.size() >= 32) {
        // **`+20` NIE jest nadpisywane.** To kopia wlasciciela, ale „w
        // przytlaczajacej wiekszosci rekordow rowna" nie znaczy „zawsze":
        // na mapie 48 jedna jednostka ma tam co innego niz `+4`. Loader tego
        // pola nie czyta, wiec zapis nie ma prawa go ruszac.
        ark::wr32(&s[24], o.subtype);
        ark::wr16(&s[28], uint16_t(o.x));
        ark::wr16(&s[30], uint16_t(o.y));
        ark::wr16(&s[32], uint16_t(o.z));
    } else if (o.type == OBJ_BUILDING && s.size() >= 32) {
        ark::wr32(&s[20], o.subtype);
        ark::wr32(&s[24], uint32_t(o.x));
        ark::wr32(&s[28], uint32_t(o.y));
        if (s.size() >= 36) ark::wr32(&s[32], uint32_t(o.z));
        // `+40` to druga kopia wlasciciela i tak samo nie zawsze rowna `+4`
        // (mapy 45, 46, 51, 53) - zostaje nietknieta.
        if (s.size() >= 59) {
            // Nazwe przepisujemy tylko wtedy, gdy sie ROZNI. Za terminatorem
            // siedzi czasem smiec nalezacy do pliku (mapa 52: bajty +52..+54),
            // a `memset` na pietnastu bajtach go zjadal.
            std::string byla;
            for (int k = 0; k < 15 && s[44 + k]; ++k) byla += char(s[44 + k]);
            if (byla != o.name) {
                std::memset(&s[44], 0, 15);
                for (size_t i = 0; i < o.name.size() && i < 14; ++i)
                    s[44 + i] = uint8_t(o.name[i]);
            }
        }
    } else if (o.type == OBJ_RESOURCE) {
        ark::wr32(&s[20], uint32_t(o.x));
        ark::wr32(&s[24], uint32_t(o.y));
        if (s.size() >= 32) ark::wr32(&s[28], uint32_t(o.z));
        ark::wr32(&s[32], o.amount);
        ark::wr32(&s[36], o.subtype);
    } else if (o.type == OBJ_VOLCANO && s.size() >= 56) {
        ark::wr32(&s[20], uint32_t(o.x));
        ark::wr32(&s[24], uint32_t(o.y));
        ark::wr32(&s[28], uint32_t(o.z));
        ark::wr32(&s[52], o.subtype);
    } else if (o.type == OBJ_MINE && s.size() >= 36) {
        ark::wr32(&s[20], o.subtype);
        ark::wr32(&s[24], uint32_t(o.x));
        ark::wr32(&s[28], uint32_t(o.y));
        ark::wr32(&s[32], uint32_t(o.z));
    } else if (isCreature(o.type)) {
        ark::wr32(&s[20], uint32_t(o.x));
        ark::wr32(&s[24], uint32_t(o.y));
        ark::wr32(&s[28], uint32_t(o.z));
        // Rekin trzyma rodzaj w klasie, nie w rekordzie - nie ma czego pisac.
        if (o.type != OBJ_SHARK) {
            size_t off = o.type == OBJ_FISH ? 36 : 32;
            if (s.size() >= off + 4) ark::wr32(&s[off], o.subtype);
        }
    }
    return s;
}

// Lista dekoracji z powrotem na bajty rekordu klasy 10.
//
// **Naglowek ma 170 bajtow, a `+20` mowi, ile wpisow idzie ZA nim** - i to
// jest zmierzone, nie przyjete: liczba spod `+20` rowna sie `(rozmiar - 170)
// / 146` na **57 z 57 map** z dekoracjami, a liczona od 24 nie zgadza sie
// z zadna. Wczesniejsza lektura „170 = 24 + 146, wiec naglowek niesie
// pierwszy wpis" byla wiec bledna - bajty 24..169 naleza do naglowka.
//
// Wpis ma 146 B: `x`, `y`, `z` jako u16, nazwa paska w 128 bajtach,
// `+134` klatka jako **int32**, potem rodzina i numer rodzaju.
//
// Caly naglowek i wszystkie nieznane pola wpisu przechodza **nietkniete** -
// z oryginalu przy wpisie, ktory z pliku przyszedl, a z pierwszego wpisu
// zrodla przy wpisie dolozonym w edytorze.
inline std::vector<uint8_t> serializeDecor(const std::vector<Decor> &d,
                                           const std::vector<uint8_t> &wzor)
{
    if (wzor.size() < size_t(DECOR_BASE)) return wzor;
    size_t oldEnd = size_t(DECOR_BASE) + size_t(ark::rd32(&wzor[20])) * DECOR_STRIDE;
    size_t tail = oldEnd <= wzor.size() ? wzor.size() - oldEnd : 0;
    std::vector<uint8_t> s(size_t(DECOR_BASE) + d.size() * DECOR_STRIDE + tail, 0);
    if (tail) std::memcpy(s.data() + DECOR_BASE + d.size() * DECOR_STRIDE,
                          wzor.data() + oldEnd, tail);
    std::memcpy(&s[0], &wzor[0], DECOR_BASE);         // caly naglowek bez zmian
    ark::wr32(&s[20], uint32_t(d.size()));            // licznik wpisow
    for (size_t i = 0; i < d.size(); ++i) {
        uint8_t *p = &s[size_t(DECOR_BASE) + i * DECOR_STRIDE];
        // Wpis z pliku niesie swoje bajty; dolozony w edytorze klonuje
        // pierwszy wpis zrodla, zeby rodzina i numer rodzaju byly sensowne.
        if (d[i].raw.size() == size_t(DECOR_STRIDE))
            std::memcpy(p, d[i].raw.data(), DECOR_STRIDE);
        else if (wzor.size() >= size_t(DECOR_BASE + DECOR_STRIDE))
            std::memcpy(p, &wzor[DECOR_BASE], DECOR_STRIDE);
        ark::wr16(p + 0, d[i].x);
        ark::wr16(p + 2, d[i].y);
        ark::wr16(p + 4, d[i].z);
        // **Pola nazwy nie czyscimy w calosci.** Za terminatorem siedzi
        // czasem smiec, ktory nalezy do pliku - bezimienny wpis mapy 26 ma
        // pod `+10..+13` bajty `64 23 77 01`, a `memset` na 128 bajtach je
        // zjadal. Nazwe przepisujemy wiec tylko wtedy, gdy sie ROZNI, i tylko
        // tyle bajtow, ile trzeba.
        {
            std::string byla;
            for (int k = 0; k < 64 && p[6 + k]; ++k) byla += char(p[6 + k]);
            if (byla != d[i].name) {
                std::memset(p + 6, 0, 64);
                for (size_t k = 0; k < d[i].name.size() && k < 63; ++k)
                    p[6 + k] = uint8_t(d[i].name[k]);
            }
        }
        ark::wr32(p + DECOR_FRAME, uint32_t(d[i].frame));
    }
    return s;
}

inline std::string zstr(const std::vector<uint8_t> &b)
{
    std::string s;
    for (uint8_t c : b) {
        if (!c) break;
        s += char(c);
    }
    return s;
}

// One map, or false when the archive is not one.
inline bool readOne(const std::string &dkx, const std::string &dkd,
                    const std::string &stem, Entry &e)
{
    ark::Archive ar;
    if (!ar.open(dkx, dkd, true)) return false;
    std::vector<uint8_t> d = ar.read("DESCRIPTOR");
    if (d.size() < 20) return false;

    e.file    = stem;
    e.dkx     = dkx;
    e.dkd     = dkd;
    e.title   = zstr(ar.read("TITLE_MISSION"));
    e.texture = zstr(ar.read("TEXTURE"));
    e.w       = ark::rd16(&d[12]);
    e.h       = ark::rd16(&d[14]);
    e.players = d[16];                 // low byte only; +17 is a flag
    if (e.title.empty()) e.title = stem;
    if (e.texture.empty()) e.texture = "land00";

    e.slots = readSlots(d);            // tablica gniazd graczy
    // Cele i odprawa - mapa misji niesie je sama, w tablicach napisow.
    e.goals = sarStrings(ar.read("OBJECTIVES"));
    e.brief = sarStrings(ar.read("DESCRIPTION"));

    std::vector<uint8_t> sm = ar.read("SMALL_MAP");
    if (!sm.empty()) ark::decodeDib(sm.data(), sm.size(), e.pw, e.ph, e.preview, true);
    return true;
}

// Everything in custom\, sorted the way the game lists it: by file name, so
// the "_m" maps that ship with the game come first.
inline std::vector<Entry> scan(const std::string &dir)
{
    std::vector<Entry> out;
    WIN32_FIND_DATAA fd{};
    HANDLE h = FindFirstFileA((dir + "\\*.dkx").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return out;
    do {
        std::string name = fd.cFileName;
        size_t dot = name.find_last_of('.');
        if (dot == std::string::npos) continue;
        std::string stem = name.substr(0, dot);
        Entry e;
        if (readOne(dir + "\\" + name, dir + "\\" + stem + ".dkd", stem, e))
            out.push_back(std::move(e));
    } while (FindNextFileA(h, &fd));
    FindClose(h);

    for (size_t i = 1; i < out.size(); ++i)          // small list, plain insertion
        for (size_t j = i; j > 0 && out[j].file < out[j - 1].file; --j)
            std::swap(out[j], out[j - 1]);
    return out;
}

}  // namespace maps
