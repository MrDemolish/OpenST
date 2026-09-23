// Wygenerowane przez tools/gen_cursors.py - nie edytowac.
//
// Punkt goracy kursora, z CursorClassTy::SetImages wolanego
// z FUN_0054bf40 (E:\__titans\Andrey	o_cursor.cpp).
// Kursor kotwiczy sie TYM punktem, nie rogiem i nie srodkiem
// kanwy - dla wiekszosci celownikow to wychodzi na jedno, ale
// nie dla strzalek przewijania ani dla CUR_ARROW.
#pragma once
#include <cstring>

namespace curhot {

struct Hot { const char *name; int x, y, w, h, delay; };

inline const Hot *table(int &n)
{
    static const Hot k[] = {
        { "CUR_MENU", 0, 0, -1, -1, 50 },   // wariant 0
        { "CUR_ARROW", 0, 0, -1, -1, 50 },   // wariant 0
        { "CUR_TASK", 0, 0, -1, -1, 50 },   // wariant 0
        { "CUR_REPORT", 0, 0, -1, -1, 50 },   // wariant 0
        { "CUR_CLOCK", 13, 18, -1, -1, 1000 },   // wariant 0
        { "CUR_CMD", 34, 20, 59, 32, 50 },   // wariant 1,6
        { "CUR_FIRE", 39, 29, 67, 50, 50 },   // wariant 2,7
        { "CUR_OWNBOAT", 34, 20, 53, 31, 50 },   // wariant 3
        { "CUR_OWNOBJ", 34, 20, 53, 31, 50 },   // wariant 4
        { "CUR_DCBOMBER", 37, 38, 55, 57, 50 },   // wariant 8
        { "CUR_CAPTURE", 36, 27, 67, 52, 50 },   // wariant 9
        { "CUR_CAPTUREUSE", 36, 27, 67, 52, 50 },   // wariant 10
        { "CUR_CAPTUREACS", 36, 27, 67, 52, 50 },   // wariant 11
        { "CUR_PARALISE", 36, 27, 67, 52, 50 },   // wariant 12,31
        { "CUR_DEFENCE", 42, 29, 65, 44, 50 },   // wariant 13,14
        { "CUR_PATROL", 34, 20, 59, 32, 50 },   // wariant 15,16
        { "CUR_EQUIPM", 34, 20, 59, 32, 50 },   // wariant 17
        { "CUR_RC", 34, 20, 67, 38, 50 },   // wariant 18
        { "CUR_UNLOADRC", 40, 30, 60, 40, 50 },   // wariant 19
        { "CUR_NOBUILD", 35, 19, -1, -1, 50 },   // wariant 21
        { "CUR_DISMANTLING", 37, 38, 56, 58, 50 },   // wariant 22
        { "CUR_REPAIR", 35, 20, 50, 40, 50 },   // wariant 23,24
        { "CUR_VIEW", 34, 20, 59, 32, 50 },   // wariant 25
        { "CUR_UNLOADCNT", 37, 37, 52, 50, 50 },   // wariant 26,30
        { "CUR_REPLENISH", 33, 19, 58, 33, 50 },   // wariant 27
        { "CUR_FORMATION", 52, 19, 86, 27, 50 },   // wariant 28
        { "CUR_TELEPORT", 40, 35, 65, 46, 50 },   // wariant 29
        { "CUR_SUP", 36, 27, -1, -1, 50 },   // wariant 50
        { "CUR_SDN", 36, 27, -1, -1, 50 },   // wariant 51
        { "CUR_SRT", 36, 27, -1, -1, 50 },   // wariant 52
        { "CUR_SLT", 36, 27, -1, -1, 50 },   // wariant 53
        { "CUR_SLU", 36, 22, -1, -1, 50 },   // wariant 54
        { "CUR_SRU", 36, 22, -1, -1, 50 },   // wariant 55
        { "CUR_SLD", 36, 32, -1, -1, 50 },   // wariant 56
        { "CUR_SRD", 36, 32, -1, -1, 50 },   // wariant 57
        { "CUR_HYPER", 13, 0, 27, 15, 50 },   // wariant 70
        { "CUR_HELPNO", 1, 1, 29, 48, 50 },   // wariant 71
    };
    n = int(sizeof(k) / sizeof(k[0]));
    return k;
}

inline const Hot *find(const char *name)
{
    int n = 0;
    const Hot *k = table(n);
    for (int i = 0; i < n; ++i)
        if (std::strcmp(k[i].name, name) == 0) return &k[i];
    return nullptr;
}

}  // namespace curhot
