// Numer ikony w panelu dla danego typu obiektu.
//
// Wprost z `IconSlotForType(tobjType, side)` pod 0x00526ba0 - jeden switch
// spinajacy lodzie (TOBJ 1..40) i budynki (50..115) w JEDNA przestrzen numerow.
// Ta przestrzen indeksuje rekordy `BOATS_<gracz>_<NN>` (lodzie, po jednym
// komplecie na kolor gracza) i `OBJS_<NN>` (budynki) w DATA\CONTROLG, po
// 48x33 kazdy - czyli gotowe ikony gniazd palety. Do tej pory bralem zamiast
// nich pierwsza klatke grafiki swiata, co wygladalo jak wcisniety w gniazdo
// sprite.
//
// Kolejnosc ikon NIE jest kolejnoscia TOBJ: 4->4 ale 5->3, 11->0xb a 12->0xa.
// Galezie zalezne od rasy maja w exe ksztalt `(-(side != 1) & MASKA) + BAZA`.
// UWAGA: `side` w tej funkcji NIE jest tym samym numerem, co w tablicy
// budynkow pod 0x007B8330. Sprawdzone wizualnie na TOBJ 50 i 53: galaz
// `side == 1` daje ikone WHITE SHARKS - OBJS_44 to kopula `dyabo`,
// a OBJS_10 to plyty `dyaws`. W tej funkcji **1 to White Sharks**.
// Przepisane doslownie z naszym 0=WS wychodzilo odwrotnie i kazdy
// z jedenastu budynkow o osobnej grafice pokazywal ikone drugiej rasy.
//
// Generuje tools/gen_icons.py - nie poprawiac recznie.
#pragma once

namespace units {

// -1 gdy typ nie ma ikony.
inline int iconSlot(int type, int side)
{
    if (side < 0 || side > 2) side = 0;
    switch (type) {
    case   1: return  0;
    case   2: return  1;
    case   3: return  2;
    case   4: return  4;
    case   5: return  3;
    case   6: return  5;
    case   7: return  6;
    case   8: return  7;
    case   9: return  8;
    case  10: return  9;
    case  11: return 11;
    case  12: return 10;
    case  13: return 13;
    case  14: return 14;
    case  15: return 15;
    case  16: return 17;
    case  17: return 16;
    case  18: return 18;
    case  19: return 19;
    case  20: return 20;
    case  21: return 21;
    case  22: return 24;
    case  23: return 22;
    case  24: return 23;
    case  25: return 26;
    case  26: return 27;
    case  27: return 28;
    case  28: return 29;
    case  29: return 30;
    case  30: return 31;
    case  31: return 32;
    case  32: return 33;
    case  33: return 34;
    case  34: return 35;
    case  35: return 36;
    case  36: return 37;
    case  37: return 25;
    case  38: return 41;
    case  39: return 42;
    case  40: return 43;
    case  43: return 40;
    case  50: return side == 0 ? 10 : 44;
    case  51: return side == 0 ? 21 : 41;
    case  52: return side == 0 ? 11 : 39;
    case  53: return side == 0 ?  0 : 35;
    case  54: return side == 0 ? 15 : 42;
    case  55: return side == 0 ?  2 : 34;
    case  56: return side == 0 ? 12 : 40;
    case  57: return side == 0 ?  4 : 38;
    case  58: return side == 0 ?  1 : 36;
    case  59: return side == 0 ?  5 : 43;
    case  60: return side == 0 ? 13 : 37;
    case  61: return side == 0 ?  3 : 33;
    case  62: return 24;
    case  63: return 25;
    case  64: return 18;
    case  65: return 16;
    case  66: return 20;
    case  67: return 14;
    case  68: return 22;
    case  69: return 23;
    case  70: return 28;
    case  71: return  9;
    case  72: return 27;
    case  73: return  6;
    case  74: return  7;
    case  75: return  8;
    case  76: return 17;
    case  77: return 26;
    case  78: return 19;
    case  79: return side == 0 ? 12 : 40;
    case  80: return side == 0 ? 32 : 29;
    case  81: return 30;
    case  82: return 31;
    case  83: return 47;
    case  84: return 54;
    case  85: return 48;
    case  86: return 52;
    case  87: return 53;
    case  88: return 50;
    case  89: return 55;
    case  90: return 49;
    case  91: return 56;
    case  92: return 57;
    case  93: return 73;
    case  94: return 58;
    case  95: return 80;
    case  96: return 59;
    case  97: return 61;
    case  98: return 62;
    case  99: return 64;
    case 100: return 60;
    case 101: return 69;
    case 102: return 71;
    case 103: return 68;
    case 104: return 70;
    case 105: return 65;
    case 106: return 67;
    case 107: return 66;
    case 108: return 77;
    case 109: return 74;
    case 110: return 76;
    case 111: return 45;
    case 112: return 75;
    case 113: return 72;
    case 114: return 79;
    case 115: return 63;
    case 253: return side == 0 ? 47 : 49;
    case 254: return 45;
    default:  return -1;
    }
}

}   // namespace units
