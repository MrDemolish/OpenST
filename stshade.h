// Filtry obrazu - to, co gracz wybiera w ustawieniach jako "FILTR OBRAZU".
//
// **Gdzie to siedzi.** Rdzen sklada klatke w `Menu::canvas` (BGRA), a backend
// - GDI albo tekstura SDL - podaje ja na ekran. Filtr wchodzi dokladnie
// miedzy jedno a drugie: czyta plotno, pisze do `Menu::post`, i to `post`
// idzie na ekran.
//
// **Filtr powiekszajacy potrzebuje czego powiekszac.** W trybie terenu plotno
// ma rozmiar okna, czyli 1:1 - xBRZ nie mialby tam nic do roboty. Wlaczenie
// filtra przestawia wiec plotno na **polowe** rozdzielczosci okna
// (`shade::scaleOf`), a filtr mnozy je z powrotem przez dwa. Skutek uboczny
// jest zamierzony: interfejs gry jest rysowany 1:1 w pikselach, wiec przy
// polowie rozdzielczosci wychodzi na ekranie **dwa razy wiekszy** - tak, jak
// wygladal w oryginale na monitorze 960x540.
//
// **Czego tu nie ma.** ScaleFX (Sp00kyFox) to wielo-przebiegowy shader GPU
// i nie przepisywalem go z pamieci - to byla by zmyslona wersja pod cudza
// nazwa. Zamiast tego sa trzy algorytmy, ktorych ksztalt jest publiczny
// i pewny: xBRZ, Scale2x (AdvMAME2x) i Eagle.
//
// CRT nie ma jednej kanonicznej postaci - kazdy shader kineskopu to model
// artystyczny. Ten sklada sie z trzech rzeczy, ktore maja wszystkie: linii
// obrazu, maski szczelinowej i rozmycia wiazki w poziomie.
#pragma once
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace shade {

enum Mode { OFF = 0, XBRZ, SCALE2X, EAGLE, CRT, CRT_XBRZ, COUNT };

inline const char *name(int m)
{
    static const char *kN[COUNT] = { "BRAK", "XBRZ 2X", "SCALE2X", "EAGLE",
                                     "CRT", "CRT + XBRZ" };
    return kN[m < 0 || m >= COUNT ? 0 : m];
}

// Krotkie "co to jest" - wiersz podpowiedzi w oknie ustawien.
inline const char *note(int m)
{
    switch (m) {
    case XBRZ:     return "wygladza skosy, ostre krawedzie zostaja";
    case SCALE2X:  return "AdvMAME2x - tanio i ostro";
    case EAGLE:    return "zaokragla narozniki";
    case CRT:      return "same linie obrazu, na terenie";
    case CRT_XBRZ: return "pelny kineskop: maska i rozmycie wiazki";
    default:       return "obraz 1:1, bez przetwarzania";
    }
}

// Ile razy filtr powieksza, czyli o ile mniejsze ma byc plotno.
//
// **Tylko powiekszacze zmniejszaja plotno**, i nie da sie tego obejsc: xBRZ,
// Scale2x i Eagle z definicji robia z malego obrazka dwa razy wiekszy, a
// w trybie terenu plotno ma rozmiar okna - nie mialyby czego powiekszac.
// CRT nie powieksza niczego, tylko przemalowuje gotowa klatke, wiec chodzi
// **w pelnej rozdzielczosci** i nie rusza ani wielkosci HUD-u, ani tego, ile
// mapy widac.
inline int scaleOf(int m) { return (m == OFF || m == CRT) ? 1 : 2; }

// --------------------------- pasma na watki -----------------------------
//
// Kazdy filtr przechodzi obraz wierszami i zaden wiersz nie potrzebuje
// wyniku sasiada, wiec da sie je rozdac na rdzenie. Jednowatkowo CRT na
// 1920x1080 zajmowal 35 ms, czyli dwie klatki - nie do grania.
//
// Watki tworzone sa na klatke, bez puli: przy szesciu rdzeniach to okolo
// 0,1 ms, czyli mniej niz procent tego, co robia.
namespace par {

struct Task { void (*fn)(void *, int, int); void *ctx; int y0, y1; };

inline DWORD WINAPI run(LPVOID p)
{
    const Task *t = static_cast<const Task *>(p);
    t->fn(t->ctx, t->y0, t->y1);
    return 0;
}

inline int threads()
{
    static int n = 0;
    if (!n) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        n = int(si.dwNumberOfProcessors);
        if (n < 1) n = 1;
        if (n > 8) n = 8;              // wiecej niz osiem nic juz nie daje
    }
    return n;
}

inline void rows(int h, void (*fn)(void *, int, int), void *ctx)
{
    const int nt = threads();
    if (nt <= 1 || h < 4 * nt) { fn(ctx, 0, h); return; }
    const int step = (h + nt - 1) / nt;
    Task t[8];
    HANDLE k[8];
    int n = 0;
    for (int i = 0; i < nt; ++i) {
        const int y0 = i * step;
        const int y1 = y0 + step < h ? y0 + step : h;
        if (y0 >= y1) break;
        t[n] = Task{ fn, ctx, y0, y1 };
        HANDLE x = CreateThread(nullptr, 0, run, &t[n], 0, nullptr);
        if (x) k[n++] = x;
        else   fn(ctx, y0, y1);
    }
    if (n) {
        WaitForMultipleObjects(DWORD(n), k, TRUE, INFINITE);
        for (int i = 0; i < n; ++i) CloseHandle(k[i]);
    }
}

}  // namespace par

// ------------------------------- kolory --------------------------------
//
// xBRZ mierzy roznice barw w YCbCr, nie w RGB - dwa odcienie tej samej
// jasnosci sa dla niego blizsze niz czern i biel o tej samej odleglosci RGB.
inline float dist(uint32_t p1, uint32_t p2)
{
    if (p1 == p2) return 0.0f;
    const float r = float(int((p1 >> 16) & 0xFF) - int((p2 >> 16) & 0xFF));
    const float g = float(int((p1 >>  8) & 0xFF) - int((p2 >>  8) & 0xFF));
    const float b = float(int( p1        & 0xFF) - int( p2        & 0xFF));
    const float kB = 0.0722f, kR = 0.2126f, kG = 1.0f - kB - kR;
    const float sB = 0.5f / (1.0f - kB), sR = 0.5f / (1.0f - kR);
    const float y  = kR * r + kG * g + kB * b;
    const float cb = sB * (b - y), cr = sR * (r - y);
    return std::sqrt(y * y + cb * cb + cr * cr);
}

// dst = (col * m + dst * (n - m)) / n, po kanale.
inline void blendTo(uint32_t &dst, uint32_t col, int m, int n)
{
    const uint32_t d = dst;
    uint32_t out = 0;
    for (int sh = 0; sh <= 16; sh += 8) {
        const int f = int((col >> sh) & 0xFF), b = int((d >> sh) & 0xFF);
        out |= uint32_t((f * m + b * (n - m)) / n) << sh;
    }
    dst = out;
}

// ------------------------------- xBRZ ----------------------------------
//
// Algorytm Zenju. Dwa kroki na piksel:
//
//   1. `corners` patrzy na jadro 4x4 i rozstrzyga, ktora z dwoch przekatnych
//      przez czworke F G J K jest krawedzia - porownuje sume roznic wzdluz
//      jednej i drugiej. Wynik to "czy i jak mocno" mieszac w kazdym z
//      czterech naroznikow piksela.
//   2. `blendPixel` maluje narozniki, po jednym na obrot o 90 stopni. Ksztalt
//      krawedzi - plaska, stroma, przekatna czy sam narożnik - wychodzi
//      z porownania roznic F-G i H-C.
//
// Progi sa te z oryginalu: 3.6 na kierunek dominujacy, 2.2 na krawedz
// stroma albo plaska, 30 na "ten sam kolor", 4 na wage srodka.
namespace xbrz {

// Jadro 3x3 w czterech obrotach: ktory z indeksow 0..8 czytac jako a..i.
static const int kRot[4][9] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8 },
    { 6, 3, 0, 7, 4, 1, 8, 5, 2 },
    { 8, 7, 6, 5, 4, 3, 2, 1, 0 },
    { 2, 5, 8, 1, 4, 7, 0, 3, 6 },
};

struct Res { unsigned char f, g, j, k; };

// Jadro 4x4, wierszami:  a b c d / e f g h / i j k l / m n o p.
// Rozstrzygany jest narożnik miedzy F(5) G(6) J(9) K(10).
inline Res corners(const uint32_t *K)
{
    Res r = { 0, 0, 0, 0 };
    const uint32_t F = K[5], G = K[6], J = K[9], KK = K[10];
    if ((F == G && J == KK) || (F == J && G == KK)) return r;

    const float w = 4.0f;
    const float jg = dist(K[8], F) + dist(F, K[2]) + dist(K[13], KK)
                   + dist(KK, K[7]) + w * dist(J, G);
    const float fk = dist(K[4], J) + dist(J, K[14]) + dist(K[1], G)
                   + dist(G, K[11]) + w * dist(F, KK);

    if (jg < fk) {
        const bool dom = 3.6f * jg < fk;
        if (F != G && F != J)   r.f = dom ? 2 : 1;
        if (KK != J && KK != G) r.k = dom ? 2 : 1;
    } else if (fk < jg) {
        const bool dom = 3.6f * fk < jg;
        if (J != F && J != KK) r.j = dom ? 2 : 1;
        if (G != F && G != KK) r.g = dom ? 2 : 1;
    }
    return r;
}

// Dwa bity na narożnik: 0 nie mieszaj, 1 zwyczajnie, 2 kierunek dominujacy.
inline unsigned char rotBlend(unsigned char b, int rot)
{
    switch (rot) {
    case 1:  return (unsigned char)((b << 2) | (b >> 6));
    case 2:  return (unsigned char)((b << 4) | (b >> 4));
    case 3:  return (unsigned char)((b << 6) | (b >> 2));
    default: return b;
    }
}

inline void blendPixel(const uint32_t *k9, uint32_t *out, int outW,
                       unsigned char info, int rot)
{
    const unsigned char blend = rotBlend(info, rot);
    const int bottomR = (blend >> 4) & 3;
    if (bottomR == 0) return;

    const int *R = kRot[rot];
    const uint32_t b = k9[R[1]], c = k9[R[2]], d = k9[R[3]], e = k9[R[4]],
                   f = k9[R[5]], g = k9[R[6]], h = k9[R[7]], i = k9[R[8]];

    const int topR = (blend >> 2) & 3, bottomL = (blend >> 6) & 3;
    auto eq = [](uint32_t p, uint32_t q) { return dist(p, q) < 30.0f; };

    bool line;
    if (bottomR >= 2)                       line = true;
    else if (topR    != 0 && !eq(e, g))     line = false;
    else if (bottomL != 0 && !eq(e, c))     line = false;
    // Ksztalt "L" dostaje sam narożnik, nie cala krawedz.
    else if (!eq(e, i) && eq(g, h) && eq(h, i) && eq(i, f) && eq(f, c))
                                            line = false;
    else                                    line = true;

    const uint32_t px = dist(e, f) <= dist(e, h) ? f : h;

    // Kwadrat 2x2 na wyjsciu, obrocony razem z jadrem.
    auto ref = [&](int I, int J) -> uint32_t & {
        int oi, oj;
        switch (rot) {
        case 1:  oi = 1 - J; oj = I;     break;
        case 2:  oi = 1 - I; oj = 1 - J; break;
        case 3:  oi = J;     oj = 1 - I; break;
        default: oi = I;     oj = J;     break;
        }
        return out[size_t(oi) * size_t(outW) + size_t(oj)];
    };

    if (!line) {                       // narożnik zaokraglony: 1 - pi/4
        blendTo(ref(1, 1), px, 21, 100);
        return;
    }
    const float fg = dist(f, g), hc = dist(h, c);
    const bool shallow = 2.2f * fg <= hc && e != g && d != g;
    const bool steep   = 2.2f * hc <= fg && e != c && b != c;
    if (shallow && steep) {
        blendTo(ref(1, 0), px, 1, 4);
        blendTo(ref(0, 1), px, 1, 4);
        blendTo(ref(1, 1), px, 5, 6);
    } else if (shallow) {
        blendTo(ref(1, 0), px, 1, 4);
        blendTo(ref(1, 1), px, 3, 4);
    } else if (steep) {
        blendTo(ref(0, 1), px, 1, 4);
        blendTo(ref(1, 1), px, 3, 4);
    } else {
        blendTo(ref(1, 1), px, 1, 2);
    }
}

// Pasmo wierszy [y0, y1). Pasma sa niezalezne - stad odtworzenie gornych
// naroznikow pierwszego wiersza pasma, ktore inaczej przyszlyby z wiersza
// policzonego przez sasiedni watek.
inline void stripe(const uint32_t *src, int sw, int sh, uint32_t *dst,
                   int y0, int y1)
{
    const int outW = sw * 2;
    std::vector<unsigned char> pre(size_t(sw), 0);

    auto kernel = [&](int y, int x, uint32_t *K) {
        const uint32_t *sm1 = src + size_t(sw) * size_t(y > 0 ? y - 1 : 0);
        const uint32_t *s0  = src + size_t(sw) * size_t(y);
        const uint32_t *sp1 = src + size_t(sw) * size_t(y + 1 < sh ? y + 1 : sh - 1);
        const uint32_t *sp2 = src + size_t(sw) * size_t(y + 2 < sh ? y + 2 : sh - 1);
        const int xm1 = x > 0 ? x - 1 : 0;
        const int xp1 = x + 1 < sw ? x + 1 : sw - 1;
        const int xp2 = x + 2 < sw ? x + 2 : sw - 1;
        K[0]  = sm1[xm1]; K[1]  = sm1[x]; K[2]  = sm1[xp1]; K[3]  = sm1[xp2];
        K[4]  = s0[xm1];  K[5]  = s0[x];  K[6]  = s0[xp1];  K[7]  = s0[xp2];
        K[8]  = sp1[xm1]; K[9]  = sp1[x]; K[10] = sp1[xp1]; K[11] = sp1[xp2];
        K[12] = sp2[xm1]; K[13] = sp2[x]; K[14] = sp2[xp1]; K[15] = sp2[xp2];
    };

    if (y0 > 0) {                       // gorne narozniki pierwszego wiersza
        for (int x = 0; x < sw; ++x) {
            uint32_t K[16];
            kernel(y0 - 1, x, K);
            const Res r = corners(K);
            pre[size_t(x)] = (unsigned char)((pre[size_t(x)] & ~0x0C) | (r.j << 2));
            if (x + 1 < sw)
                pre[size_t(x + 1)] =
                    (unsigned char)((pre[size_t(x + 1)] & ~0x03) | r.k);
        }
    }

    for (int y = y0; y < y1; ++y) {
        uint32_t *out = dst + size_t(2 * y) * size_t(outW);
        unsigned char next = 0;         // lewy gorny narożnik nastepnej kolumny
        for (int x = 0; x < sw; ++x, out += 2) {
            uint32_t K[16];
            kernel(y, x, K);
            const Res r = corners(K);

            unsigned char info = pre[size_t(x)];
            info = (unsigned char)((info & ~0x30) | (r.f << 4));      // prawy dolny
            next = (unsigned char)((next & ~0x0C) | (r.j << 2));      // prawy gorny
            pre[size_t(x)] = next;                                    // na nastepny wiersz
            next = r.k;
            if (x + 1 < sw)
                pre[size_t(x + 1)] =
                    (unsigned char)((pre[size_t(x + 1)] & ~0xC0) | (r.g << 6));

            const uint32_t col = K[5];
            out[0] = col; out[1] = col;
            out[outW] = col; out[outW + 1] = col;
            if (!info) continue;

            const uint32_t k9[9] = { K[0], K[1], K[2], K[4], K[5], K[6],
                                     K[8], K[9], K[10] };
            for (int rot = 0; rot < 4; ++rot)
                blendPixel(k9, out, outW, info, rot);
        }
    }
}

struct Job { const uint32_t *src; int sw, sh; uint32_t *dst; };

inline void worker(void *p, int y0, int y1)
{
    const Job *j = static_cast<const Job *>(p);
    stripe(j->src, j->sw, j->sh, j->dst, y0, y1);
}

// Caly obraz, na tylu watkach ile jest rdzeni.
inline void scale2(const uint32_t *src, int sw, int sh, uint32_t *dst)
{
    Job j{ src, sw, sh, dst };
    par::rows(sh, worker, &j);
}

}  // namespace xbrz

// ----------------------------- Scale2x, Eagle ---------------------------
//
// Oba sa krotkie i **dokladne** - ich reguly to kilka porownan rownosci,
// wiec nie ma tu czego przyblizac.
inline void scale2x(const uint32_t *src, int sw, int sh, uint32_t *dst)
{
    const int outW = sw * 2;
    for (int y = 0; y < sh; ++y) {
        const uint32_t *s0 = src + size_t(sw) * size_t(y);
        const uint32_t *sm = src + size_t(sw) * size_t(y > 0 ? y - 1 : 0);
        const uint32_t *sp = src + size_t(sw) * size_t(y + 1 < sh ? y + 1 : sh - 1);
        uint32_t *out = dst + size_t(2 * y) * size_t(outW);
        for (int x = 0; x < sw; ++x, out += 2) {
            const uint32_t E = s0[x];
            const uint32_t B = sm[x], H = sp[x];
            const uint32_t D = s0[x > 0 ? x - 1 : 0];
            const uint32_t F = s0[x + 1 < sw ? x + 1 : sw - 1];
            out[0]        = (D == B && B != F && D != H) ? D : E;
            out[1]        = (B == F && B != D && F != H) ? F : E;
            out[outW]     = (D == H && D != B && H != F) ? D : E;
            out[outW + 1] = (H == F && D != H && B != F) ? F : E;
        }
    }
}

inline void eagle2x(const uint32_t *src, int sw, int sh, uint32_t *dst)
{
    const int outW = sw * 2;
    for (int y = 0; y < sh; ++y) {
        const uint32_t *s0 = src + size_t(sw) * size_t(y);
        const uint32_t *sm = src + size_t(sw) * size_t(y > 0 ? y - 1 : 0);
        const uint32_t *sp = src + size_t(sw) * size_t(y + 1 < sh ? y + 1 : sh - 1);
        uint32_t *out = dst + size_t(2 * y) * size_t(outW);
        for (int x = 0; x < sw; ++x, out += 2) {
            const int xm = x > 0 ? x - 1 : 0, xp = x + 1 < sw ? x + 1 : sw - 1;
            const uint32_t S = sm[xm], T = sm[x], U = sm[xp];
            const uint32_t V = s0[xm], C = s0[x], W = s0[xp];
            const uint32_t X = sp[xm], Y = sp[x], Z = sp[xp];
            out[0]        = (S == T && S == V) ? S : C;
            out[1]        = (T == U && U == W) ? U : C;
            out[outW]     = (V == X && X == Y) ? X : C;
            out[outW + 1] = (W == Z && Z == Y) ? Z : C;
        }
    }
}

// -------------------------------- CRT -----------------------------------
//
// Trzy rzeczy, ktore ma kazdy shader kineskopu:
//
//   * **linie obrazu** - co druga linia wyjscia jest ciemniejsza, a o ile,
//     zalezy od jasnosci: jasna wiazka rozlewa sie szerzej i prawie zasypuje
//     przerwe, ciemna zostawia ja czarna,
//   * **maska szczelinowa** - co trzecia kolumna gasi inny kanal, tak jak
//     szczeliny przed luminoforem,
//   * **rozmycie wiazki w poziomie** - sasiednia kolumna doklada czesc swojej
//     jasnosci.
//
// Na koncu jasnosc trzeba podniesc, bo maska i linie zabieraja jej mniej
// wiecej jedna trzecia.
//
// Model jest **moj** - w grze nic takiego nie ma i w exe tez nie.
//
// Liczy sie na liczbach calkowitych w formacie 8.8. Na zmiennoprzecinkowych,
// z dzieleniem na piksel, wychodzilo 35 ms na klatke 1920x1080 - filtr byl
// piec razy drozszy niz zlozenie calej sceny.
namespace crtf {

// Maska razy wyrownanie jasnosci (1,30), w formacie 8.8: 256 to jeden.
// Kolumna mod 3 gasi inny kanal - tak jak szczeliny przed luminoforem.
// Gaszenie do 0,72 dawalo obraz w widoczna kropke; 0,80 zostawia strukture,
// ale nie zjada napisow.
static const int kMask[3][3] = {
    { 333, 266, 333 },
    { 333, 333, 266 },
    { 266, 333, 333 },
};

// Bez maski, w pelnej rozdzielczosci: same wyrownanie jasnosci 1,12.
static const int kFlat[3] = { 287, 287, 287 };

struct Job { uint32_t *px; int w; bool full; };

// `full` to model kineskopu w komplecie - maska i rozmycie wiazki. Ma sens
// **tylko przy zrodle o polowe mniejszym niz ekran**, bo wtedy jeden piksel
// gry to dwa piksele obrazu i maska trafia miedzy nie.
//
// W pelnej rozdzielczosci maska i rozmycie sa **tej samej gestosci co tresc**
// i po prostu ja zjadaja: teren wychodzil w kratke, a napisy przestawaly byc
// czytelne. Zostaja wiec same linie obrazu, i to lagodniejsze.
inline void band(void *p, int y0, int y1)
{
    const Job *j = static_cast<const Job *>(p);
    const int w = j->w;
    const bool full = j->full;
    std::vector<uint32_t> row(static_cast<size_t>(w));
    for (int y = y0; y < y1; ++y) {
        uint32_t *q = j->px + size_t(y) * size_t(w);
        std::copy(q, q + w, row.begin());
        const bool dark = (y & 1) != 0;
        for (int x = 0; x < w; ++x) {
            const uint32_t c = row[size_t(x)];
            int cr = int((c >> 16) & 0xFF);
            int cg = int((c >> 8) & 0xFF);
            int cb = int(c & 0xFF);
            if (full) {
                // Rozmycie wiazki w poziomie: 10/16 srodek, po 3/16 na sasiada.
                const uint32_t l = row[size_t(x > 0 ? x - 1 : 0)];
                const uint32_t r = row[size_t(x + 1 < w ? x + 1 : w - 1)];
                cr = (10 * cr + 3 * int((l >> 16) & 0xFF)
                             + 3 * int((r >> 16) & 0xFF)) >> 4;
                cg = (10 * cg + 3 * int((l >> 8) & 0xFF)
                             + 3 * int((r >> 8) & 0xFF)) >> 4;
                cb = (10 * cb + 3 * int(l & 0xFF) + 3 * int(r & 0xFF)) >> 4;
            }
            // Jasna wiazka rozlewa sie szerzej i prawie zasypuje przerwe
            // miedzy liniami; ciemna zostawia ja czarna.
            int scan = 256;
            if (dark) {
                const int luma = (54 * cr + 183 * cg + 19 * cb) >> 8;
                scan = full ? 133 + (97 * luma >> 8)      // 0,52 .. 0,90
                            : 158 + (77 * luma >> 8);     // 0,62 .. 0,92
            }
            const int *m = full ? kMask[size_t(x % 3)] : kFlat;
            const int vr = (cr * scan * m[0]) >> 16;
            const int vg = (cg * scan * m[1]) >> 16;
            const int vb = (cb * scan * m[2]) >> 16;
            q[size_t(x)] = (uint32_t(vr > 255 ? 255 : vr) << 16)
                         | (uint32_t(vg > 255 ? 255 : vg) << 8)
                         |  uint32_t(vb > 255 ? 255 : vb);
        }
    }
}

}  // namespace crtf

inline void crt(uint32_t *px, int w, int h, bool full)
{
    crtf::Job j{ px, w, full };
    par::rows(h, crtf::band, &j);
}

// ------------------------------- wejscie --------------------------------
//
// `dst` ma miec `sw*scaleOf(mode)` na `sh*scaleOf(mode)` pikseli.
//
// Dwa kroki, i tylko pierwszy zmienia rozmiar: powiekszacz (albo przepisanie
// 1:1, gdy trybu nie ma wsrod nich), a potem maska kineskopu na tym, co
// wyszlo. Dzieki temu CRT chodzi w pelnej rozdzielczosci, a CRT + XBRZ na
// polowie - roznica siedzi w jednym miejscu, w `scaleOf`.
inline void apply(int mode, const uint32_t *src, int sw, int sh, uint32_t *dst)
{
    switch (mode) {
    case SCALE2X:  scale2x(src, sw, sh, dst); break;
    case EAGLE:    eagle2x(src, sw, sh, dst); break;
    case XBRZ:
    case CRT_XBRZ: xbrz::scale2(src, sw, sh, dst); break;
    default:       std::copy(src, src + size_t(sw) * size_t(sh), dst); break;
    }
    const int s = scaleOf(mode);
    // Pelny model kineskopu tylko tam, gdzie piksel gry ma na ekranie dwa
    // piksele - inaczej maska bylaby gestsza niz sama tresc.
    if (mode == CRT || mode == CRT_XBRZ) crt(dst, sw * s, sh * s, s > 1);
}

// Filtr wprost na cudze plotno: zrodlo `sw x sh` ma pokryc prostokat
// `dw x dh` w buforze o kroku `stride`.
//
// Wyjscie filtra bywa o piksel wieksze niz miejsce - polowa nieparzystej
// szerokosci jest zaokraglana w gore, zeby po powiekszeniu nie zostawal pasek
// przy krawedzi - wiec nadmiar sie obcina. `tmp` trzyma wolajacy, zeby nie
// alokowac bufora ekranu co klatke.
inline void applyTo(int mode, const uint32_t *src, int sw, int sh,
                    std::vector<uint32_t> &tmp,
                    uint32_t *dst, int dw, int dh, int stride)
{
    const int s = scaleOf(mode);
    const int ow = sw * s, oh = sh * s;
    const size_t n = size_t(ow) * size_t(oh);
    if (tmp.size() != n) tmp.assign(n, 0);
    apply(mode, src, sw, sh, tmp.data());
    const int cw = dw < ow ? dw : ow, ch = dh < oh ? dh : oh;
    for (int y = 0; y < ch; ++y)
        std::copy(tmp.data() + size_t(y) * size_t(ow),
                  tmp.data() + size_t(y) * size_t(ow) + size_t(cw),
                  dst + size_t(y) * size_t(stride));
}

}  // namespace shade
