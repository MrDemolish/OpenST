// Wygenerowane przez tools/gen_bldwork.py - nie edytowac.
//
// Animacja PRACY budynku - sekwencja 11, odpalana zdarzeniem,
// z `GetMineWorkAnim` (0x004E04A0). Stan pracy siedzi pod
// +0x4e8: 1 luk w gore, 2 surowiec plynie, 3 luk w dol;
// stan 0 zatrzymuje sekwencje. Przy magazynie pasek fazy 2
// zalezy jeszcze od rodzaju surowca spod +0x4dc.
#pragma once

namespace bldwork {

// Surowiec numerami z exe: 0xdc zloto, 0xdd korium,
// 0xde metal; 0 znaczy "ten sam pasek na wszystko".
const int RES_GOLD = 0xdc, RES_CORIUM = 0xdd, RES_METAL = 0xde;

struct Step { int tobj; int phase; int res; const char *strip; };

inline const Step *table(int &n)
{
    static const Step k[] = {
        {  56, 1,    0, "mine_ani1_lid" },   // -
        {  56, 2,    0, "mine_ani1_gld" },   // -
        {  56, 3,    0, "mine_ani1_lid" },   // -
        {  57, 1,    0, "mine_ani1_lid" },   // -
        {  57, 2,    0, "mine_ani1_cor" },   // -
        {  57, 3,    0, "mine_ani1_lid" },   // -
        {  59, 1,    0, "depo_ani1_lid" },   // -
        {  59, 2,  220, "depo_ani1_gld" },   // zloto
        {  59, 2,  221, "depo_ani1_cor" },   // korium
        {  59, 2,  222, "depo_ani1_met" },   // metal
        {  59, 3,    0, "depo_ani1_lid" },   // -
        {  79, 1,    0, "mine_ani1_lid" },   // -
        {  79, 2,    0, "mine_ani1_met" },   // -
        {  79, 3,    0, "mine_ani1_lid" },   // -
        {  82, 1,    0, "depo_ani1_lid" },   // -
        {  82, 2,  220, "depo_ani1_gld" },   // zloto
        {  82, 2,  221, "depo_ani1_cor" },   // korium
        {  82, 2,  222, "depo_ani1_met" },   // metal
        {  82, 3,    0, "depo_ani1_lid" },   // -
        {  94, 1,    0, "corisi_lid" },   // -
        {  94, 2,    0, "corisi_ani1_cor" },   // -
        {  94, 3,    0, "corisi_lid" },   // -
        {  96, 1,    0, "silo_ani1_lid" },   // -
        {  96, 2,    0, "silo_ani1_cor" },   // -
        {  96, 3,    0, "silo_ani1_lid" },   // -
    };
    n = int(sizeof(k) / sizeof(k[0]));
    return k;
}

// Czy faza leci WSTECZ. Faza 3 zawsze; faza 2 wstecz u kopalni,
// w przod u magazynu, silosu i rynku - to rozroznienie stoi
// wprost w `GetMineWorkAnim`.
inline bool backwards(int tobj, int phase)
{
    if (phase == 3) return true;
    if (phase != 2) return false;
    static const int fwd[] = { 59, 82, 96 };
    for (unsigned i = 0; i < sizeof(fwd) / sizeof(fwd[0]); ++i)
        if (fwd[i] == tobj) return false;
    return true;
}

// Pasek dla budynku, fazy i surowca. Surowiec liczy sie tylko
// tam, gdzie tablica go rozroznia.
inline const char *strip(int tobj, int phase, int res)
{
    int n = 0;
    const Step *k = table(n);
    const char *any = nullptr;
    for (int i = 0; i < n; ++i) {
        if (k[i].tobj != tobj || k[i].phase != phase) continue;
        if (k[i].res == 0) any = k[i].strip;
        else if (k[i].res == res) return k[i].strip;
    }
    return any;
}

// Czy ten budynek w ogole ma animacje pracy.
inline bool works(int tobj) { return strip(tobj, 1, 0) != nullptr; }

}  // namespace bldwork
