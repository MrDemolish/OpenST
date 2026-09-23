// Numery pociskow - wygenerowane przez tools/gen_proj.py z ST.exe.
//
// Numer broni to **TOBJ pocisku**; tablice sa dwie, bo gra trzyma
// lodzie (0x007A8B18, krok 4, indeks = typ) i budynki (0x00792CA0,
// krok 3 dwordy, indeks = gniazdo + grupa*2) osobno.
#pragma once

namespace proj {

// Pocisk lodzi danego typu, 0 gdy nieuzbrojona.
inline int ofUnit(int type)
{
    switch (type) {
    case 1: return 0x9f;
    case 2: return 0x96;
    case 3: return 0x98;
    case 4: return 0x96;
    case 5: return 0x96;
    case 6: return 0x96;
    case 10: return 0xa4;
    case 11: return 0x96;
    case 13: return 0x96;
    case 14: return 0xac;
    case 15: return 0x9d;
    case 16: return 0x99;
    case 17: return 0x96;
    case 18: return 0x9c;
    case 22: return 0x9d;
    case 23: return 0x9c;
    case 28: return 0xad;
    case 30: return 0xb6;
    case 31: return 0xb5;
    case 32: return 0xb7;
    case 33: return 0xb8;
    case 34: return 0xb6;
    case 35: return 0xb6;
    case 38: return 0x98;
    case 39: return 0x9c;
    case 40: return 0xb6;
    default: return 0;
    }
}

// Pocisk budynku, gniazdo 0 albo 1.
inline int ofBld(int tobj, int slot)
{
    switch (tobj * 2 + (slot ? 1 : 0)) {
    case 124: return 0xa0;   // TOBJ 62 gniazdo 0
    case 126: return 0x97;   // TOBJ 63 gniazdo 0
    case 132: return 0xc0;   // TOBJ 66 gniazdo 0
    case 138: return 0xa8;   // TOBJ 69 gniazdo 0
    case 140: return 0x9c;   // TOBJ 70 gniazdo 0
    case 142: return 0x99;   // TOBJ 71 gniazdo 0
    case 143: return 0xab;   // TOBJ 71 gniazdo 1
    case 148: return 0x9e;   // TOBJ 74 gniazdo 0
    case 150: return 0x9a;   // TOBJ 75 gniazdo 0
    case 156: return 0xa3;   // TOBJ 78 gniazdo 0
    case 162: return 0xa4;   // TOBJ 81 gniazdo 0
    case 202: return 0xc1;   // TOBJ 101 gniazdo 0
    case 204: return 0xae;   // TOBJ 102 gniazdo 0
    case 206: return 0xba;   // TOBJ 103 gniazdo 0
    case 208: return 0xb0;   // TOBJ 104 gniazdo 0
    case 210: return 0xb3;   // TOBJ 105 gniazdo 0
    case 212: return 0xbc;   // TOBJ 106 gniazdo 0
    case 214: return 0xbf;   // TOBJ 107 gniazdo 0
    case 224: return 0xb2;   // TOBJ 112 gniazdo 0
    case 226: return 0xb0;   // TOBJ 113 gniazdo 0
    case 228: return 0xbe;   // TOBJ 114 gniazdo 0
    default: return 0;
    }
}

// Predkosc pocisku w jednostkach swiata na tik - `GetSpeed`
// (`0x00430750`). Zero znaczy: nie ma tego numeru w tabeli,
// wiec zostaje etykieta z przewodnika.
inline int speed(int nr)
{
    switch (nr) {
    case 0x96: return 48;
    case 0x97: return 48;
    case 0x98: return 48;
    case 0x99: return 48;
    case 0x9a: return 48;
    case 0x9b: return 201;
    case 0x9c: return 201;
    case 0x9d: return 201;
    case 0x9e: return 201;
    case 0x9f: return 96;
    case 0xa0: return 96;
    case 0xa1: return 48;
    case 0xa3: return 96;
    case 0xa4: return 72;
    case 0xa5: return 201;
    case 0xab: return 6;
    case 0xac: return 48;
    case 0xad: return 72;
    case 0xae: return 96;
    case 0xb4: return 48;
    case 0xb6: return 72;
    case 0xb7: return 48;
    case 0xb8: return 48;
    case 0xb9: return 60;
    case 0xba: return 72;
    case 0xbf: return 96;
    default: return 0;
    }
}

// **Pocisk omijajacy linie strzalu.** `CheckRay` (`0x0041f9b0`)
// dla tych numerow rzuca wyjatek i zwraca 1, zanim w ogole puscil
// promien - przeszkody ich nie obchodza. Lista jest
// **niesasiadujaca**: 0xac, 0xad, 0xae, 0xb6..0xb8, 0xba i 0xbd..0xbf
// w niej nie ma. Cztery bronie, ktorym przewodnik daje `Guided: Yes`
// (0xab, 0xb3, 0xbc, 0xb0), sa tu wszystkie - czyli etykieta
// przewodnika to **podzbior** tej listy, nie to samo.
inline bool ignoresRay(int nr)
{
    switch (nr) {
    case 0x9b:
    case 0xa3:
    case 0xa5:
    case 0xa6:
    case 0xa7:
    case 0xa8:
    case 0xa9:
    case 0xaa:
    case 0xab:
    case 0xaf:
    case 0xb0:
    case 0xb1:
    case 0xb2:
    case 0xb3:
    case 0xb4:
    case 0xb5:
    case 0xb9:
    case 0xbb:
    case 0xbc:
        return true;
    default: return false;
    }
}

// **Premia badawcza x1,5 do predkosci pocisku.** `GetSpeed` robi
// `v = (v >> 1) + v` dla tych numerow, gdy rasa ma swoje badanie.
// Wychodzi z tego **rodzina torped**: Small, Medium, Large,
// Splinter i Electromagnetic Torpedo plus Cassette Shell.
inline bool fasterWithTech(int nr)
{
    switch (nr) {
    case 0x96:
    case 0x97:
    case 0x98:
    case 0x99:
    case 0x9a:
    case 0xac:
    case 0xb4:
        return true;
    default: return false;
    }
}

} // namespace proj
