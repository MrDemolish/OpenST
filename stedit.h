// Edytor map - ekran, ktorego gra nie miala.
//
// Oryginalny `StEditor.exe` jest osobnym programem z interfejsem z 2000 roku
// i na dzisiejszych systemach zwykle nie wstaje. Ten edytor siedzi **w grze**,
// pod wlasnym przyciskiem menu glownego, i pokazuje mape w trzech trybach:
// izometrycznym (tak, jak sie w nia gra), plaskim 2D (tak, jak sie ja edytuje)
// i warstwowym (poziom po poziomie).
//
// Ten plik trzyma **sam projekt interfejsu**: barwy, uklad i stan. Nie wie nic
// o `Menu` ani o rysowaniu - dzieki temu uklad da sie policzyc i sprawdzic bez
// skladania klatki, a audyt moze pytac o prostokaty wprost.
#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <array>
#include <string>
#include <vector>

#include "stmap.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace ed {

// ------------------------------------------------------------------ barwy
//
// Ciemny warsztat, a nie zielony kokpit gry: edytor ma byc czytelny przez
// godziny, wiec kontrast idzie na tresc (mape), nie na ramki.
constexpr uint32_t BG        = 0x0E1116;   // tlo okna
constexpr uint32_t PANEL     = 0x161B22;   // paski i panele
constexpr uint32_t PANEL_HI  = 0x1C232C;   // pole pod kursorem
constexpr uint32_t PANEL_SEL = 0x243040;   // pole wybrane
constexpr uint32_t LINE      = 0x2A323D;   // zwykla krawedz
constexpr uint32_t LINE_HI   = 0x3D4855;   // krawedz mocniejsza
constexpr uint32_t ACCENT    = 0x2DD4BF;   // akcent: aktywna zakladka, obrys
constexpr uint32_t ACCENT_DK = 0x145A52;   // ten sam akcent przygaszony
constexpr uint32_t WARN      = 0xE0A030;   // ostrzezenie
constexpr uint32_t VIEW_BG   = 0x090B0F;   // tlo samego widoku

// ------------------------------------------------------------------ uklad
constexpr int TOP_H    = 76;               // actions above view tabs
constexpr int RAIL_W   = 76;               // listwa: ikona 60x35 plus podpis
constexpr int INSP_W   = 300;              // panel wlasciwosci
constexpr int STAT_H   = 26;               // pasek stanu
constexpr int PAD      = 8;

struct Layout {
    RECT top{}, rail{}, view{}, insp{}, status{};
    bool narrow = false;                   // panel wlasciwosci sie nie zmiescil
};

// Uklad liczy sie **raz**, dla rysowania i dla trafien mysza - ta sama
// zasada, co przy `BarLayout` w rozgrywce. Przy waskim oknie pierwszy
// znika panel wlasciwosci, bo widok mapy jest wazniejszy niz liczby obok.
inline Layout layout(int w, int h)
{
    Layout L;
    int insp = (w >= 780) ? INSP_W : 0;
    L.narrow = insp == 0;
    L.top    = RECT{ 0, 0, w, TOP_H };
    L.status = RECT{ 0, h - STAT_H, w, h };
    L.rail   = RECT{ 0, TOP_H, RAIL_W, h - STAT_H };
    L.insp   = RECT{ w - insp, TOP_H, w, h - STAT_H };
    L.view   = RECT{ RAIL_W, TOP_H, w - insp, h - STAT_H };
    return L;
}

// ------------------------------------------------------------- narzedzia
// **Grupa tekstur: wysoki bajt numeru kafla.** Numer to `(grupa << 8) |
// wariant`. Rodzina moze zawierac zarowno grunt, jak i tekstury zboczy.
// Losowy pedzel korzysta tylko z wariantow gruntu, pelna paleta pokazuje wszystkie.
struct TexGroupInfo {
    int group = 0;      // wysoki bajt
    int base = 0;       // wariant, ktory na tej mapie dominuje
    int baseUses = 0;
    int uses = 0;       // ile komorek mapy ma te grupe
    std::vector<std::pair<int, int>> variants;   // (wariant, ile uzyc)
};

struct DecorChoice { std::string name; int32_t frame=0; };

// Czym maluje pedzel tekstury.
enum TexMode {
    TEX_BAZOWY,     // cala plama jednym, bazowym kaflem grupy
    TEX_WARIANTY,   // warianty z rozkladu tej mapy - inaczej widac kratke
    TEX_DOKLADNY,   // konkretny kafel z pelnej palety
    TEX_MODES
};

inline const char *texModeName(int m)
{
    return m == TEX_WARIANTY ? "LOSOWE WARIANTY" : m == TEX_DOKLADNY ? "WYBRANY KAFEL" : "BAZOWY";
}

enum Tool {
    T_SELECT,      // wskaz i obejrzyj
    T_RAISE,       // podnies teren
    T_LOWER,       // obniz teren
    T_PLATFORM,    // niezalezna polka skalna na wybranym poziomie
    T_TEXTURE,     // maluj tekstura dna
    T_OBJECT,      // budynki i lodzie
    T_DECOR,       // roslinnosc i detal dna
    T_DEPOSIT,     // zloza surowcow
    T_START,       // punkty startu graczy
    T_ERASE,       // usun
    T_COUNT
};

// **Ikony sa z gry, nie wymyslone.** Skroty w rodzaju „WSK" i „WYS" nie
// mowily nic nikomu poza tym, kto je napisal. Kazde narzedzie dostaje wiec
// prawdziwy przycisk komend gry (60x35, cztery stany) i **polska nazwe pod
// nim** - razem nie da sie ich pomylic, a listwa wyglada jak czesc gry,
// bo nia jest.
//
// Dobor jest po OBRAZKU, nie po nazwie rekordu - ta sama zasada, co przy
// ikonach budynkow: `BUT_RISE` to kolo ze strzalkami w gore, `BUT_TRGOLD`
// bryly rudy, `BUT_COLLECTOR_RC` roslina, `BUT_GETINFO` panel z napisem INFO.
struct ToolDef { const char *ikona, *nazwa, *opis; };

inline const ToolDef &tool(int i)
{
    static const ToolDef t[T_COUNT] = {
        { "BUT_GETINFO",      "WSKAZNIK",  "klik zaznacza, prawy odznacza" },
        { "BUT_RISE",         "PODNIES",   "lewy podnosi teren, prawy obniza" },
        { "BUT_FALL",         "OBNIZ",     "lewy obniza teren, prawy podnosi" },
        { "BUT_BUILD",        "PLATFORMA", "lewy maluje polke, prawy wycina otwor" },
        { "BUT_VIEWZONE",     "TEKSTURA",  "prawy pobiera wzor, lewy maluje" },
        { "BUT_BUILD",        "OBIEKTY",   "wybierz kategorie, obiekt i miejsce" },
        { "BUT_COLLECTOR_RC", "DEKORACJE", "roslinnosc i detal dna" },
        { "BUT_TRGOLD",       "ZLOZA",     "korium albo metal, z ilosc" },
        { "BUT_SETDESTINATION","START",    "punkt startu gracza" },
        { "BUT_DISMANTLING",  "USUN",      "zdejmij to, co pod kursorem" },
    };
    return t[i < 0 || i >= T_COUNT ? 0 : i];
}

// Ktore narzedzia dzialaja na TEREN, a ktore na obiekty. Pedzel i kontury
// dotycza tylko tych pierwszych.
// Rodzaje zaznaczenia - jeden numer na wszystko, co da sie wskazac.
enum SelKind { SEL_NIC, SEL_OBJ, SEL_DECOR, SEL_START };

inline bool toolPaintsTerrain(int t)
{
    return t == T_RAISE || t == T_LOWER || t == T_TEXTURE || t == T_PLATFORM;
}

// ----------------------------------------------------------------- widoki
enum View { V_ISO, V_FLAT, V_LAYERS, V_COUNT };

inline const char *viewName(int v)
{
    static const char *n[V_COUNT] = { "3D", "2D", "WARSTWY" };
    return n[v < 0 || v >= V_COUNT ? 0 : v];
}

// Zakladki widoku siedza w gornym pasku, wyrownane do prawej od srodka.
constexpr int TAB_W = 92, TAB_H = 26;

// Zakladki zaczynaja sie za nazwa mapy. Przy 220 px nazwa dluzszej mapy
// wchodzila pod pierwsza zakladke - napis byl obciety i nie bylo wiadomo,
// ktora to mapa.
constexpr int TAB_X0 = 360;

inline RECT tabRect(const Layout &L, int i)
{
    int x0 = L.top.left + PAD;
    return RECT{ x0 + i * (TAB_W + 4), L.top.bottom - TAB_H - 6,
                 x0 + i * (TAB_W + 4) + TAB_W, L.top.bottom - 6 };
}

// **Przyciski prawego konca gornego paska.** Zapis i nowa mapa siedzialy
// wylacznie pod klawiszem, a nowej mapy nie dalo sie zaczac z interfejsu
// WCALE - `EdNewMap` istnialo i nikt go nie wolal. Przycisk, ktorego nie ma
// na ekranie, jest funkcja, ktorej nie ma.
enum TopBtn { TB_OTWORZ, TB_NOWA, TB_ZAPISZ, TB_COUNT };
constexpr int TOPB_W = 96, TOPB_H = 26, TOPB_GAP = 6;

inline const char *topBtnName(int i)
{
    static const char *n[TB_COUNT] = { "OTWORZ", "NOWA MAPA", "ZAPISZ" };
    return n[i < 0 || i >= TB_COUNT ? 0 : i];
}

inline RECT topBtnRect(const Layout &L, int i)
{
    int prawy = L.top.right - PAD;
    int x1 = prawy - (TB_COUNT - 1 - i) * (TOPB_W + TOPB_GAP);
    return RECT{ x1 - TOPB_W, L.top.top + 8, x1, L.top.top + 8 + TOPB_H };
}

// Gniazdo narzedzia: przycisk gry 60x35 i pod nim podpis.
constexpr int RAIL_ICO_W = 60, RAIL_ICO_H = 35;
constexpr int RAIL_BTN = RAIL_ICO_H + 13, RAIL_GAP = 5;

inline RECT railRect(const Layout &L, int i)
{
    int x = L.rail.left + (RAIL_W - RAIL_ICO_W) / 2;
    int pitch = std::min(RAIL_BTN + RAIL_GAP, std::max(20, int(L.rail.bottom - L.rail.top - 2 * PAD) / T_COUNT));
    int y = L.rail.top + PAD + i * pitch;
    return RECT{ x, y, x + RAIL_ICO_W, y + pitch - 3 };
}

// Sam obrazek wewnatrz gniazda - podpis siedzi pod nim.
inline RECT railIconRect(const Layout &L, int i)
{
    RECT r = railRect(L, i);
    return RECT{ r.left, r.top, r.left + RAIL_ICO_W, std::min(r.top + RAIL_ICO_H, r.bottom - 13) };
}

// --------------------------------------------------------------- stan
struct Snapshot {
    std::vector<maps::Cell> cells;
    std::vector<maps::Object> objects;
    std::vector<maps::Decor> decor;
    std::vector<maps::MapSlot> slots;
};

struct State {
    bool  on      = false;      // edytor otwarty
    int   view    = V_ISO;
    int   tool    = T_SELECT;
    int   layer   = -1;         // -1 = wszystkie poziomy, inaczej jeden
    bool  grid    = true;       // siatka komorek
    bool  objects = true;       // pokaz obiekty
    bool  decor   = true;       // pokaz dekoracje
    int   mapNo   = -1;         // ktora mapa jest wczytana
    // Plaski widok 2D ma wlasna kamere, w pikselach na komorke.
    // **Zero znaczy "jeszcze nie dopasowany"** - pierwszy rysunek liczy zoom
    // tak, zeby cala mapa weszla w okno. Staly zoom 8 dawal przy mapie 26x26
    // blokow kwadracik 208 px w oknie na 1044 px i wygladalo to, jakby edytor
    // nie wczytal mapy.
    int   flatZoom = 0;
    int   flatX = 0, flatY = 0;
    // Co jest pod kursorem - liczone raz na klatke, uzywane przez pasek stanu.
    int   curBX = -1, curBY = -1, curLvl = -1;
    int   hotTool = -1, hotTab = -1;
    // Zaznaczenie
    // **Co jest zaznaczone.** Jeden model na wszystko, co na mapie stoi -
    // inaczej kazde narzedzie mialoby wlasne pole i panel nie wiedzialby,
    // o czym pisac.
    int   selKind = SEL_NIC;
    int   selIdx = -1;
    // Co zniknie pod kursorem - pasek stanu mowi to slowami, a mapa
    // obrysem; oba biora te sama wartosc, zeby nie moglo sie rozjechac.
    std::string eraseName;
    int   selObj = -1;          // indeks w terr.objects (widok 2D)
    // Czy mapa ma niezapisane zmiany. Pasek stanu mowi o tym wprost,
    // bo edytor nie zapisuje sam z siebie - i nie ma zapisywac.
    bool  dirty = false;
    int   brushTex = 0;         // tekstura pobrana prawym przyciskiem
    // **Pedzel.** Promien w blokach: 0 to jedna kratka, 1 to 3x3, 2 to 5x5.
    // Rysowanie po jednej kratce przy mapie 26x26 blokow to 676 klikniec -
    // pedzel jest tu nie wygoda, tylko warunkiem uzywalnosci.
    int   brush = 1;
    int   platformLevel = 3;    // wysokosc niezalezna od filtra widoku
    bool  platformErase = false;
    bool  brushRound = true;    // okragly, nie kwadratowy
    // Kontury robia sie same: po kazdym pociagnieciu teren wokol pomalowanego
    // miejsca jest wygladzany tak, zeby sasiedzi nie roznili sie wiecej niz
    // o jeden poziom. Dzieki temu maluje sie PLAMY, a nie krawedzie.
    // Co stawia narzedzie OBIEKTY. TOBJ glownego budynku jako start,
    // bo to jedyny numer wspolny dla wszystkich ras w palecie.
    int   placeTobj = 50;
    int   placeRes = 221;       // 221 korium, 222 metal, 223 zloto, 224 silikon
    unsigned placeAmount = 20000;   // ile surowca ma nowe zloze
    int   placeOwner = 0;       // gniazdo, ktore dostanie nowy obiekt
    int   ownTop = 0, resTop = 0;   // gdzie panel narysowal te dwa wybory
    int   resHot = -1;
    std::string placeDecor;     // pusty = klonuj pierwsza dekoracje mapy
    int32_t placeDecorFrame = 0;
    int scatterDensity = 20, scatterSpacing = 1;
    uint32_t scatterSeed = 1;
    bool scatterMixed = true;
    // Punkty startu graczy - kopia tablicy gniazd `DESCRIPTOR`, zeby
    // dalo sie ja edytowac i zapisac bez ruszania reszty rekordu.
    std::vector<maps::MapSlot> slots;
    int   startSlot = 0;        // ktore gniazdo ustawia narzedzie START
    // **Czy zmienilo sie cos POZA terenem.** Latka w miejscu podmienia
    // wylacznie rekord `3D_MAP`, wiec przy zmianie obiektu, dekoracji
    // albo punktu startu musi wejsc zapis calego archiwum - inaczej
    // zapis wyszedlby bez bledu i po cichu zgubilby te zmiane.
    bool  dirtyObj = false;
    // Historia cofania: kopie `cells`, bo one sa calym terenem.
    std::vector<Snapshot> undo, redo;
    std::vector<maps::Object> templates;
    std::vector<maps::Decor> decorTemplates;
    std::set<std::string> decorAnimated;
    // Original mesh/texture pairs, grouped by texture family and high corners.
    std::map<int, std::array<maps::Cell, 16>> contours;
    int contourFamily = -1;
    std::map<uint16_t, uint16_t> textureMeshes;
    std::vector<uint8_t> decorSource;
    int palScroll = 0;
    int objectMode = 0;         // 0 buildings, 1 boats
    int objectSide = 0;
    RECT objectModeRect{}, objectSideRect{};
    int dialog = 0;            // 1 open, 2 new, 3 unsaved changes
    int pending = 0;           // 1 open, 2 new, 3 exit
    int mapTop = 0;
    int newW = 64, newH = 64;
    RECT propertiesRect{};
    RECT controlRect[16]{};
    bool moving = false;
    bool previousAI = true, previousFog = true;
    // Palety narzedzi - liczone raz na mape, nie co klatke.
    std::vector<uint16_t> texList;
    // Grupy tekstur tej mapy - po czestosci uzycia, bo to one ja zbudowaly.
    std::vector<TexGroupInfo> texGroups;
    int   texGroup = -1;            // ktora grupa maluje pedzel
    int   texMode = TEX_WARIANTY;
    bool  texAll = false;
    int   texEdges = 0;             // ile kresek styku narysowal ostatni widok
    std::vector<int>      objList;
    std::vector<std::string> decList;   // nazwy paskow dekoracji tej mapy
    std::vector<DecorChoice> decChoices; // statyczne warianty, animacje jako jedna pozycja
    int   palTop = 0;           // gdzie panel zaczal rysowac palete
    int   palHot = -1;          // gniazdo pod kursorem
    int   topHot = -1;          // przycisk gornego paska pod kursorem
    bool  painting = false;     // lewy trzymany: maluj przeciagnieciem
    bool  panning = false;      // srodkowy trzymany: przewijaj mape
    POINT panFrom{};
    int   lastBX = -1, lastBY = -1;
    std::string lastSave;       // co ostatnio zapisano, do paska stanu
};

}  // namespace ed
