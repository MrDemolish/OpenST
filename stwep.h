// Bron - wygenerowane przez tools/gen_weapons.py z przewodnika.
//
// W exe nie ma tego przy tablicach nazw, tak samo jak kosztow. Przewodnik
// podaje na kazdy wpis blok `Weapons:` z obrazeniami, opoznieniem strzalu
// i zasiegiem. Obrazenia sa dwie: przeciw lodziom i przeciw budynkom,
// i licza sie NA POCISK - liczba pociskow w salwie jest osobno.
// Zasieg jest w blokach, opoznienie w sekundach - tu w milisekundach.
//
// DC Bomber: torpeda w zwyklym ataku, bomby pod osobnym rozkazem.
// MML: dwa gniazda zgodne z 0x00792CA0: kaseta, potem mina magnetyczna.
// Pozostale wybor/statystyki z przewodnika nadal wymagaja weryfikacji exe.
#pragma once

namespace wep {

struct Gun {
    int dmgUnit;        // na lodz
    int dmgBld;         // na budynek
    int delayMs;        // przerwa miedzy strzalami
    int range;          // w blokach
    const char *shot;   // nazwa broni z przewodnika
    int shots;          // ile pociskow na salwe (DC Bomby: 15)
    int guided;         // czy pocisk sledzi cel
    int pspeed;         // 0 wolny, 1 sredni, 2 szybki, 3 nieruchomy
    bool armed() const { return range > 0 && (dmgUnit > 0 || dmgBld > 0); }
};

inline Gun unitGun(int type)
{
    switch (type) {
    case 1: return { 30, 80, 1200, 5, "Light HF Shells", 1, 0, 1 };
    case 2: return { 50, 70, 2000, 5, "Small Torpedo", 1, 0, 0 };
    case 3: return { 90, 120, 3500, 5, "Large Torpedo", 2, 0, 0 };
    case 4: return { 50, 70, 3000, 5, "Small Torpedo", 1, 0, 0 };
    case 5: return { 50, 70, 3000, 5, "Small Torpedo", 1, 0, 0 };
    case 6: return { 50, 70, 3000, 5, "Small Torpedo", 1, 0, 0 };
    case 10: return { 600, 600, 4000, 5, "High Plasma Charge", 1, 0, 1 };
    case 11: return { 50, 70, 3000, 5, "Small Torpedo", 1, 0, 0 };
    case 13: return { 50, 70, 1500, 5, "Small Torpedo", 1, 0, 0 };
    case 14: return { 60, 150, 2000, 5, "Splinter Torpedo", 2, 0, 0 };
    case 15: return { 120, 120, 2000, 5, "Ruby Laser", 2, 0, 2 };
    case 16: return { 120, 120, 3500, 5, "Cassette Shell", 1, 0, 0 };
    case 17: return { 50, 70, 2500, 5, "Small Torpedo", 1, 0, 0 };
    case 18: return { 60, 60, 2500, 5, "Light Laser", 2, 0, 2 };
    case 22: return { 120, 120, 2000, 5, "Ruby Laser", 2, 0, 2 };
    case 23: return { 60, 60, 2000, 5, "Light Laser", 2, 0, 2 };
    case 28: return { 10, 20, 4000, 5, "Neuro-Shell", 1, 0, 1 };
    case 30: return { 40, 60, 1500, 5, "Energy Shell", 1, 0, 1 };
    case 31: return { 300, 300, 6000, 5, "BHE Shell", 1, 0, 0 };
    case 32: return { 120, 140, 2500, 5, "Ion Cassette Shell", 1, 0, 0 };
    case 33: return { 400, 600, 2500, 5, "Bio-Acid Shell", 1, 0, 0 };
    case 34: return { 40, 60, 2000, 5, "Energy Shell", 1, 0, 1 };
    case 35: return { 40, 60, 1500, 5, "Energy Shell", 1, 0, 1 };
    case 38: return { 90, 120, 3500, 5, "Large Torpedo", 2, 0, 0 };
    case 39: return { 60, 60, 3500, 5, "Light Laser", 1, 0, 2 };
    case 40: return { 40, 60, 1500, 5, "Energy Shell", 1, 0, 1 };
    default: break;
    }
    return { 0, 0, 0, 0, "", 0, 0, 1 };
}

// Bron NIERUCHOMA - mina. Przewodnik opisuje ja przy lodzi, ktora ja
// stawia: Depth Mine 1000 obrazen na promieniu 1, Laser Snare 800 na
// promieniu 3. `unitGun` jej nie zwraca, bo wybiera bron o najwiekszym
// iloczynie obrazen i zasiegu, a to jest zwykla torpeda.
inline Gun mineGun(int type)
{
    switch (type) {
    case 5: return { 1000, 1000, 0, 1, "Depth Mines", 1, 0, 3 };
    case 17: return { 800, 800, 0, 3, "Laser Snare", 1, 0, 3 };
    case 33: return { 950, 950, 0, 1, "Acoustic Mine", 1, 0, 3 };
    default: break;
    }
    return { 0, 0, 0, 0, "", 0, 0, 3 };
}

inline Gun bombGun(int type)
{
    switch (type) {
    case 4: return { 500, 500, 5000, 1, "DC Bombs", 15, 0, 0 };
    default: return {0, 0, 0, 0, "", 0, 0, 1};
    }
}

inline Gun bldGun(int tobj, int side, int slot = 0)
{
    if (slot == 1) {
        switch (tobj * 4 + side) {
        case 285: return { 350, 350, 3000, 5, "Magnetic Mine", 1, 1, 0 };
        default: return {0, 0, 0, 0, "", 0, 0, 1};
        }
    }
    if (slot != 0) return {0, 0, 0, 0, "", 0, 0, 1};
    switch (tobj * 4 + side) {
    case 248: return { 70, 120, 2000, 8, "Heavy HF Shell", 2, 0, 1 };
    case 252: return { 80, 100, 3000, 8, "Medium Torpedo", 2, 0, 0 };
    case 264: return { 25, 25, 3000, 6, "Ultrasonic Field", 1, 0, 3 };
    case 281: return { 60, 60, 1000, 7, "Light Laser", 1, 0, 2 };
    case 285: return { 120, 120, 2000, 8, "Cassette Mother-Shell", 1, 0, 0 };
    case 297: return { 200, 200, 2000, 8, "Gas Laser", 2, 0, 2 };
    case 301: return { 400, 500, 2000, 8, "Electromagnetic Torpedo", 1, 0, 0 };
    case 324: return { 600, 600, 4000, 8, "High Plasma Charge", 1, 0, 1 };
    case 406: return { 30, 30, 1500, 7, "Soliton Field", 1, 0, 3 };
    case 410: return { 150, 150, 2000, 8, "Gas Shell", 2, 0, 1 };
    case 414: return { 100, 120, 2000, 8, "High Temperature Plasma Shell", 2, 0, 1 };
    case 418: return { 20, 20, 1000, 8, "Energy Shell", 1, 0, 1 };
    case 422: return { 200, 200, 3000, 5, "Jump-Mine", 1, 1, 0 };
    case 426: return { 150, 150, 6000, 10, "Bio-Mine", 1, 1, 0 };
    case 430: return { 80, 120, 2000, 8, "Polarized Plasma Shell", 1, 0, 1 };
    case 454: return { 400, 400, 2500, 8, "Self-Guided Energy Shell", 1, 1, 0 };
    default: break;
    }
    return { 0, 0, 0, 0, "", 0, 0, 1 };
}

} // namespace wep
