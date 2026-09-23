// A terrain set - the Land00..Land03 archives in system\.
//
// Each one holds the art the maps refer to by number:
//
//   MAPTXTR<n>   type 28, a flat 64x64 block of palette indices, 4096 bytes
//   MAPMESH<n>   type 27, the 3D geometry: u16 vertex count, u16 triangle
//                count, then float positions - the game draws terrain through
//                Direct3D, which is why d3drm.dll ships with it
//   BRDTXTR<n>   16 border textures
//   PALETTE      a 1x1 DIB whose only point is the 256 entry palette at +40
//
// The numbers in a map's cells are these record numbers directly. They run up
// to 30864 and the names are not fixed width - MAPTXTR001 and MAPTXTR13345 are
// both real - so "%03d" is the right format: it pads short ones and leaves
// long ones alone. Every texture and mesh value in all 25 shipped maps
// resolves to a record that exists.
//
// Only the textures are read here. Drawing the real meshes is a bigger job;
// the height a cell sits at comes from its level instead.
#pragma once
#include <algorithm>
#include <array>
#include <cstdlib>
#include <utility>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "stark.h"
#include "stspr.h"

namespace land {

constexpr int TILE = 64;
// Kanwa sprite'a swiata: 180x140. Wszystkie 363 paski lodzi na niej siedza,
// a dekoracje i efekty tak samo - patrz „Jedna skala swiata: 180".
constexpr int WORLD_CANVAS_W = 180, WORLD_CANVAS_H = 140;

// **Punkt gorący sprite'a świata — z exe, nie ze środka kanwy.**
// `STT3DSprC::Init` podaje hotX/hotY wprost do `TerrainRenderC::AllocSlot`,
// a 24 miejsca wywołania dają dokładnie dwie pary, po jednej na kanwę:
//     180x140 -> (90, 69)     środek kanwy to (90, 70)
//     240x190 -> (120, 86)    środek kanwy to (120, 95)
// W poziomie to środek (30 = 60/2), w pionie NIE — duża kanwa ma nad
// kotwicą 86 px zamiast 95. Kotwiczenie obu środkiem stawiało bryłę na
// kanwie 240 o dziewięć pikseli wyżej niż sąsiada na 180.
inline int worldHotX(int cw, int ch)
{
    if (cw == 240 && ch == 190) return 120;
    if (cw == WORLD_CANVAS_W && ch == WORLD_CANVAS_H) return 90;
    return cw / 2;
}

inline int worldHotY(int cw, int ch)
{
    if (cw == 240 && ch == 190) return 86;
    if (cw == WORLD_CANVAS_W && ch == WORLD_CANVAS_H) return 69;
    return ch / 2;
}

// MAPMESH - the terrain geometry for one 2x2 block.
//
//   +0   u16 vertex count          +2   u16 triangle count
//   +4   four u32 collision masks, one per game cell in this 2x2 block
//   +20  vertices, 12 bytes each: float x, y, z
//        triangles, 28 bytes each: u8 flags, u8 i0, u8 i1, u8 i2, then
//        24 bytes that would be UVs but are zero in every record
//
// size == 20 + nv*12 + nt*28 holds (to within one padding byte) on all 220
// meshes in the set.
//
// x and y run 0..20 across the block and z runs 0..-40 downwards. The most
// used mesh by far is a flat 20x20 quad; the rest carve slopes and hollows.
// Since the UVs are empty the texture has to be projected straight down,
// which is what the flat quad implies anyway.
constexpr float MESH_SPAN = 20.0f;

struct Vert { float x = 0, y = 0, z = 0; };
// Rekord trojkata ma 28 B i **wszystkie** sa zajete: bajt flag, trzy
// indeksy wierzcholkow i **szesc intow UV** - po parze na wierzcholek,
// w 0.16 (65536 = 1.0). Czyta je `FUN_007279b0`, przycinajac kazdy do
// 0xfff6 i mnozac przez szerokosc tekstury.
//
// Sprawdzian strukturalny: po czterech zestawach `Land` wszystkie
// **63 602 trojkaty** maja komplet szesciu skladowych w [0,1] (zakres
// -0.0019 .. 1.0000). Zla para albo zly offset wysypalyby sie natychmiast.
struct Tri  {
    uint8_t flags = 0, i0 = 0, i1 = 0, i2 = 0;
    float u[3] = { 0, 0, 0 }, v[3] = { 0, 0, 0 };
};

struct Mesh {
    std::array<uint32_t,4> collision{}; // +4: masks for the four game cells
    std::vector<Vert> verts;
    std::vector<Tri>  tris;
    float minZ = 0;                    // how far the block digs down
    // **Co loader wyrzuca po cichu.** Bez tych licznikow nie da sie odroznic
    // „mesh bez trojkatow" od „mesh, ktoremu trojkaty odpadly" - a to druga
    // rzecz znaczy dziure w terenie, ktorej nikt nie zobaczy w kodzie.
    int  dropped = 0;                  // trojkaty o indeksie poza tablica
    int  nonFinite = 0;                // wierzcholki z NaN albo nieskonczonoscia
    bool badSize = false;              // naglowek obiecuje wiecej, niz rekord ma
    bool missing = true;               // rekordu w ogole nie ma w archiwum
    float minX = 0, maxX = 0, minY = 0, maxY = 0, maxZ = 0;
    bool ok() const { return !verts.empty() && !tris.empty(); }
};

// One level step is exactly 10 mesh units. Measured, not assumed: across a
// map, the depth of a block's mesh against the level difference to its lowest
// neighbour comes out 0, 9.1, 19.9, 29.9, 39.9 for differences 0 to 4.
constexpr float MESH_PER_LEVEL = 10.0f;

class Set {
public:
    // name comes from the map's own TEXTURE record, e.g. "land00"
    // **Kazda tablica podreczna w tej klasie jest kluczowana SAMYM
    // numerem albo nazwa** - bez zadnej pamieci o tym, z ktorego
    // zestawu pochodzi. Otwarcie drugiego zestawu bez ich
    // wyczyszczenia oddawalo wiec kafle, siatki, paski i tablice
    // remapu z POPRZEDNIEJ mapy, a paleta byla juz nowa. To ta sama
    // rodzina usterek, co `aiSt`, `players[].civ` i efekty
    // przechodzace miedzy mapami: widac ja dopiero wtedy, gdy
    // w jednym procesie zagra sie dwie mapy o roznym terenie.
    bool open(const std::string &gameDir, const std::string &name)
    {
        std::string base = gameDir + "\\system\\" + name;
        // Mapy pisza nazwe roznie (`land00`, `Land00`, `LAND00`),
        // a Windows nie rozroznia wielkosci liter - wiec porownanie
        // idzie w jednej postaci, zeby ten sam zestaw nie
        // przeladowywal sie bez potrzeby.
        std::string klucz = base;
        for (char &c : klucz) c = char(std::tolower((unsigned char)c));
        if (loaded && klucz == openedKey) return true;

        if (!ar.open(base + ".DKX", base + ".DKD")) return false;
        cache.clear();
        raw.clear();
        blends.clear();
        strips.clear();
        meshes.clear();
        remaps.clear();
        openedKey = klucz;

        std::vector<uint8_t> p = ar.read("PALETTE");
        if (p.size() < 40 + 256 * 4) return false;
        for (int i = 0; i < 256; ++i) {
            const uint8_t *c = &p[40 + size_t(i) * 4];      // BGRA
            pal[i] = (uint32_t(c[2]) << 16) | (uint32_t(c[1]) << 8) | c[0];
        }
        loaded = true;
        return true;
    }

    bool ok() const { return loaded; }

    // The editor palette belongs to the land archive, not the current map.
    std::vector<uint16_t> textureIds() const
    {
        std::vector<uint16_t> ids;
        for (const std::string &n : ar.names())
            if (n.compare(0, 7, "MAPTXTR") == 0)
                ids.push_back(uint16_t(std::strtoul(n.c_str() + 7, nullptr, 10)));
        std::sort(ids.begin(), ids.end());
        return ids;
    }

    // Read the four corners from the actual mesh; names do not encode
    // orientation reliably across the different mountain families.
    static std::array<float, 4> corners(const Mesh &m)
    {
        std::array<float, 4> z{};
        const float x[4] = {0,20,20,0}, y[4] = {0,0,20,20};
        for (int k = 0; k < 4; ++k) {
            float best = 1e20f;
            for (const Vert &v : m.verts) {
                float d = (v.x-x[k])*(v.x-x[k]) + (v.y-y[k])*(v.y-y[k]);
                if (d < best) { best = d; z[k] = v.z; }
            }
        }
        return z;
    }

    // Tablice przemapowania palety, rekordy typu 32 w tym samym archiwum:
    // 256 bajtow na stopien, bajt to nowy indeks dla starego. Gra trzyma ich
    // dwanascie w tablicy pod 0x0079AF70 - PLT_PAUSE, PLT_EXPLITE, PLT_SHAD30,
    // PLT_SHAD40, PLT_SHAD60, PLT_FOG, PLT_MMSHAD, PLT_NUCL, PLT_GLOW,
    // PLT_DKD, PLT_FSGSGLASS, PLT_FSGSSHAD - i wybiera je przez
    // STT3DSprC::SetCurShad, ktory zapisuje numer w samym sprite.
    //
    //   PLT_SHAD*   przyciemnienie, stad cienie
    //   PLT_FOG     24 stopnie w dol do czerni
    //   PLT_GLOW    18 stopni: neutralny -> cyan -> magenta
    //   PLT_NUCL     5 stopni: cyan -> biel
    const uint8_t *remap(const char *name, int step, int &steps)
    {
        auto it = remaps.find(name);
        if (it == remaps.end())
            it = remaps.emplace(name, ar.read(name)).first;
        const std::vector<uint8_t> &t = it->second;
        steps = int(t.size() / 256);
        if (steps <= 0) return nullptr;
        if (step < 0) step = 0;
        if (step >= steps) step = steps - 1;
        return &t[size_t(step) * 256];
    }

    // 64x64 pixels for one texture number, or nullptr. Cached: a map reuses
    // the same handful of tiles thousands of times.
    const uint32_t *tile(uint16_t n)
    {
        auto it = cache.find(n);
        if (it != cache.end()) return it->second.empty() ? nullptr : it->second.data();

        std::vector<uint32_t> px;
        char name[24];
        std::snprintf(name, sizeof(name), "MAPTXTR%03u", unsigned(n));
        std::vector<uint8_t> b = ar.read(name);
        if (b.size() >= size_t(TILE) * TILE) {
            px.resize(size_t(TILE) * TILE);
            for (size_t i = 0; i < px.size(); ++i) px[i] = pal[b[i]];
        }
        auto ins = cache.emplace(n, std::move(px));
        return ins.first->second.empty() ? nullptr : ins.first->second.data();
    }

    // The same tile as raw palette indices. The renderer wants these rather
    // than colours: shading a pixel used to cost three integer divisions, and
    // with indices the shade folds into a 256 entry palette built once per
    // triangle, leaving one indexed load in the inner loop.
    // Blok moze miec dwie tekstury: bazowa i nakladke z kluczem 255. Sklejamy
    // je raz na pare i trzymamy - inaczej trzeba by rysowac blok dwa razy.
    // Klucz to 255, zmierzone: kafle uzywane jako nakladka maja go 13-14
    // procent, bazowe zero.
    static const uint8_t TEX_KEY = 255;

    const uint8_t *tileBlend(uint16_t a, uint16_t b)
    {
        if (!b || b == a) return tileRaw(a);
        uint32_t key = (uint32_t(a) << 16) | b;
        auto it = blends.find(key);
        if (it != blends.end())
            return it->second.empty() ? nullptr : it->second.data();
        const uint8_t *base = tileRaw(a);
        const uint8_t *over = tileRaw(b);
        std::vector<uint8_t> v;
        if (base && over) {
            v.assign(base, base + size_t(TILE) * TILE);
            for (size_t i = 0; i < v.size(); ++i)
                if (over[i] != TEX_KEY) v[i] = over[i];
        } else if (over) {
            v.assign(over, over + size_t(TILE) * TILE);
        }
        auto ins = blends.emplace(key, std::move(v));
        return ins.first->second.empty() ? nullptr : ins.first->second.data();
    }

    const uint8_t *tileRaw(uint16_t n)
    {
        auto it = raw.find(n);
        if (it != raw.end()) return it->second.empty() ? nullptr : it->second.data();

        char name[24];
        std::snprintf(name, sizeof(name), "MAPTXTR%03u", unsigned(n));
        std::vector<uint8_t> b = ar.read(name);
        if (b.size() < size_t(TILE) * TILE) b.clear();
        else b.resize(size_t(TILE) * TILE);
        auto ins = raw.emplace(n, std::move(b));
        return ins.first->second.empty() ? nullptr : ins.first->second.data();
    }

    size_t cached() const { return cache.size(); }

    // Units and buildings have no palette of their own; they borrow this one.
    const uint32_t *palette() const { return pal; }

    const Mesh *mesh(uint16_t n)
    {
        auto it = meshes.find(n);
        if (it != meshes.end()) return it->second.ok() ? &it->second : nullptr;

        Mesh m;
        char name[24];
        // **Bit 15 to flaga takze przy siatce, nie tylko przy teksturze.**
        // `mfTMapLoad` sklada obie nazwy tak samo:
        // `MakeRecordName(MAPTXTR, 3, v & 0x7fff)` i
        // `MakeRecordName(MAPMESH, 3, v & 0x7fff)`. Zadna z 58 map go nie
        // ustawia, wiec dzis nic to nie zmienia - ale bez maski pierwsza
        // mapa, ktora go ustawi, dostalaby numer wiekszy o 32768 i blok bez
        // siatki, a `--mesh` pokazalby BEZ REKORDU i nie powiedzial czemu.
        std::snprintf(name, sizeof(name), "MAPMESH%03u", unsigned(n) & 0x7fffu);
        std::vector<uint8_t> b = ar.read(name);
        m.missing = b.empty();
        if (b.size() >= 20) {
            for (int k=0;k<4;++k) m.collision[size_t(k)] = ark::rd32(&b[4+k*4]);
            size_t nv = ark::rd16(&b[0]), nt = ark::rd16(&b[2]);
            if (b.size() < 20 + nv * 12 + nt * 28) m.badSize = true;
            else {
                m.verts.resize(nv);
                for (size_t i = 0; i < nv; ++i) {
                    const uint8_t *p = &b[20 + i * 12];
                    std::memcpy(&m.verts[i].x, p, 4);
                    std::memcpy(&m.verts[i].y, p + 4, 4);
                    std::memcpy(&m.verts[i].z, p + 8, 4);
                    const Vert &v = m.verts[i];
                    // **NaN psuje caly blok, nie jeden wierzcholek**: kazde
                    // porownanie z nim jest falszem, wiec obrys wychodzi
                    // w przypadkowe miejsce i nie widac, skad.
                    if (!std::isfinite(v.x) || !std::isfinite(v.y)
                        || !std::isfinite(v.z)) { ++m.nonFinite; continue; }
                    if (v.z < m.minZ) m.minZ = v.z;
                    if (v.z > m.maxZ) m.maxZ = v.z;
                    if (i == 0 || v.x < m.minX) m.minX = v.x;
                    if (i == 0 || v.x > m.maxX) m.maxX = v.x;
                    if (i == 0 || v.y < m.minY) m.minY = v.y;
                    if (i == 0 || v.y > m.maxY) m.maxY = v.y;
                }
                size_t to = 20 + nv * 12;
                m.tris.reserve(nt);
                for (size_t i = 0; i < nt; ++i) {
                    const uint8_t *p = &b[to + i * 28];
                    if (p[1] >= nv || p[2] >= nv || p[3] >= nv) { ++m.dropped; continue; }
                    Tri t{ p[0], p[1], p[2], p[3], {}, {} };
                    // **UV sa RZUTEM Z GORY - i to jest odczyt, nie
                    // domysl.** Pole trojkata w UV podzielone przez pole
                    // jego rzutu wychodzi 1.00-1.02 w kazdej klasie
                    // stromosci, ze sciana (dz>15) wlacznie; na samych
                    // scianach |u - x/20| + |v - y/20| to 0.0002. Czyli
                    // autorzy siatek zapiekli w UV dokladnie to, co remake
                    // dotad liczyl sam - a pionowe smugi na stromej scianie
                    // sa w grze tak samo i nie sa usterka remake'u.
                    //
                    // Czytamy je mimo to, bo to **dane zamiast wyliczenia**:
                    // jest w nich poltekselowy wsuw od krawedzi (na plaskim
                    // MAPMESH4352 rogi to 33 i 65503, nie 0 i 65536), ktory
                    // trzyma probkowanie z dala od sasiedniego kafla.
                    for (int k = 0; k < 3; ++k) {
                        t.u[k] = float(int32_t(ark::rd32(p + 4 + k * 8))) / 65536.0f;
                        t.v[k] = float(int32_t(ark::rd32(p + 8 + k * 8))) / 65536.0f;
                    }
                    m.tris.push_back(t);
                }
                // **Kolejnosc malarza WEWNATRZ kafla.** `XformTile` wstawia
                // trojkat przed pierwszym, ktory ma wierzcholek blizszy -
                // czyli trzyma liste od najdalszego. Bez tego o wierzchu
                // decyduje kolejnosc w pliku i tylna sciana reliefu potrafi
                // zamalowac przednia; to sa te kwadratowe fasety na zboczu.
                //
                // Kierunek patrzenia jest policzony z rzutu, nie dobrany:
                // jadro macierzy rzutu to (1, 1, 2) w jednostkach
                // (blok, blok, poziom), a blok ma 20 jednostek mesha
                // i poziom 10 - stad (x + y) + 4*z.
                //
                // Sortujemy RAZ, przy wczytaniu, wiec klatka nic nie placi.
                // Stabilnie, zeby remis rozstrzygal plik - tak samo jak
                // w grze, gdzie rowna glebokosc zostawia kolejnosc wstawiania.
                {
                    std::vector<std::pair<float, uint16_t> > kol;
                    kol.reserve(m.tris.size());
                    for (size_t k = 0; k < m.tris.size(); ++k) {
                        const Tri &q = m.tris[k];
                        const uint8_t id[3] = { q.i0, q.i1, q.i2 };
                        float naj = 1e30f;
                        for (int j = 0; j < 3; ++j) {
                            const Vert &v = m.verts[id[j]];
                            float d = (v.x + v.y) + 4.0f * v.z;
                            if (d < naj) naj = d;
                        }
                        kol.push_back(std::make_pair(naj, uint16_t(k)));
                    }
                    std::stable_sort(kol.begin(), kol.end(),
                        [](const std::pair<float, uint16_t> &a,
                           const std::pair<float, uint16_t> &b) {
                            return a.first < b.first;
                        });
                    std::vector<Tri> pos;
                    pos.reserve(m.tris.size());
                    for (size_t k = 0; k < kol.size(); ++k)
                        pos.push_back(m.tris[kol[k].second]);
                    m.tris.swap(pos);
                }
            }
        }
        auto ins = meshes.emplace(n, std::move(m));
        return ins.first->second.ok() ? &ins.first->second : nullptr;
    }

    // To samo, ale **bez odsiewania**: `mesh()` zwraca `nullptr` dla kazdego
    // niepoprawnego wpisu, wiec audyt nie miałby jak odroznic braku rekordu
    // od rekordu, ktoremu odpadly trojkaty.
    const Mesh *meshRaw(uint16_t n)
    {
        mesh(n);
        auto it = meshes.find(n);
        return it == meshes.end() ? nullptr : &it->second;
    }

    // A named sprite strip out of the same set: the sea floor decorations are
    // "coral", "alga_t", "stone_c", "cover", "base", "star", "stars", plus the
    // numbered "alga0".."alga15". Cached, since a map places thousands of them
    // drawn from a handful of strips.
    const spr::Strip *strip(const std::string &name)
    {
        auto it = strips.find(name);
        if (it != strips.end()) return it->second.count() ? &it->second : nullptr;
        spr::Strip s;
        s.load(ar.read(name), pal);
        auto ins = strips.emplace(name, std::move(s));
        return ins.first->second.count() ? &ins.first->second : nullptr;
    }

private:
    ark::Archive ar;
    std::unordered_map<uint16_t, std::vector<uint32_t>> cache;
    std::unordered_map<uint16_t, std::vector<uint8_t>> raw;
    std::unordered_map<uint32_t, std::vector<uint8_t>> blends;
    std::unordered_map<std::string, spr::Strip> strips;
    std::unordered_map<uint16_t, Mesh> meshes;
    uint32_t pal[256] = {};
    std::string openedKey;      // ktory zestaw siedzi w tablicach
    std::unordered_map<std::string, std::vector<uint8_t>> remaps;
    bool loaded = false;
};

}  // namespace land
