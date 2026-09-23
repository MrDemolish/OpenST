// Ikony uzbrojenia w panelu - wygenerowane przez tools/gen_weapicon.py.
//
// Gniazdo `INF_WEAP_<NN>` dla pary (numer pocisku, poziom ulepszenia).
// **Nie jest to `numer - 0x96`** - ta zbieznosc (45 ikon i 45 numerow
// 0x96..0xc2) byla przypadkiem. Switch `FUN_005259b0` daje trzem
// rodzinom torped (0x96, 0x97, 0x98) po PIEC gniazd - po jednym na
// poziom 0..4 - a reszcie po jednym, i razem wypelnia dokladnie
// 0..44.
#pragma once

namespace weapicon {

// Gniazdo ikony, -1 gdy numer nie ma swojej ikony.
inline int slotFor(int nr, int level)
{
    if (level < 0) level = 0;
    if (level > 4) return -1;          // switch chodzi tylko dla < 5
    switch (nr) {
    case 0x96: return level + 0;   // rodzina, piec poziomow
    case 0x97: return level + 5;   // rodzina, piec poziomow
    case 0x98: return level + 10;   // rodzina, piec poziomow
    case 0x99: return 29;
    case 0x9a: return 16;
    case 0x9c: return 19;
    case 0x9d: return 18;
    case 0x9e: return 17;
    case 0x9f: return 20;
    case 0xa0: return 20;
    case 0xa3: return 27;
    case 0xa4: return 21;
    case 0xa6: return 23;
    case 0xa7: return 24;
    case 0xa8: return 26;
    case 0xa9: return 25;
    case 0xab: return 22;
    case 0xac: return 15;
    case 0xad: return 35;
    case 0xae: return 37;
    case 0xaf: return 30;
    case 0xb0: return 43;
    case 0xb2: return 44;
    case 0xb3: return 32;
    case 0xb5: return 33;
    case 0xb6: return 39;
    case 0xb7: return 40;
    case 0xb8: return 36;
    case 0xba: return 41;
    case 0xbc: return 31;
    case 0xbd: return 34;
    case 0xbe: return 38;
    case 0xbf: return 42;
    case 0xff: return 28;
    default: return -1;
    }
}

// Ile ikon ma deskryptor - i tyle samo wypelnia switch.
inline int count() { return 45; }

} // namespace weapicon
