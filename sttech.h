// Wygenerowane przez tools/gen_tech.py - nie edytowac.
//
// Drzewo technologii **z gry**. Kazdy wpis to jeden wezel
// drzewa, czyli para (technologia, poziom) - gra numeruje
// technologie 1..154 i trzyma dla kazdej POZIOM, nie flage.
//
//   wezly     0x007C2B58 / 0x007C2DF0 / 0x007C30D8
//   wymagania PTR_DAT_007C0DC8[rasa], rekord 25 bajtow
//   zloto     0x007E482C
//   czas      0x007E5488, w tikach po 25 na sekunde
//   ikona     FUN_005276E0, gniazdo UPG_<NN>
//   napisy    FUN_00528060 / FUN_00528A30, st_string.dll
#pragma once
#include <cstring>

namespace tech {

// 25 tikow na sekunde - z zestawienia czasu badania w tikach
// z czasem w przewodniku (technologia 1: 750 tikow, 0:30).
const int TICKS_PER_SEC = 25;

struct Tech {
    const char *name;    // nazwa z gry (st_string.dll)
    const char *guide;   // nazwa z przewodnika, do wymagan budowy
    int side;            // 0 WS, 1 BO, 2 SI
    int id;              // numer technologii w grze, 1..154
    int level;           // poziom, 1..4
    int gold;            // koszt
    int secs;            // czas badania
    int ticks;           // czas badania w tikach gry
    int icon;            // gniazdo UPG_<NN>
    int nameId, descId, catId;   // napisy w st_string.dll
    int x, y;            // miejsce na tle BKG_HLPTTREE_<rasa>
    int preId[4];        // wymagania: numer technologii, 0 konczy
    int preLvl[4];       // i wymagany poziom
    bool steal;          // czy da sie ukrasc po przejeciu
    const char *unlock[4];
};

inline const Tech *table(int &n)
{
    static const Tech k[] = {
        { "ULEPSZENIE PANCERZA DO POZIOMU  2", "Upgrade to Armor Level 2", 0, 4, 1, 100, 90, 2250, 3, 13003, 23704, 13500, 120, 8, { 7, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE PANCERZA DO POZIOMU  3", "Upgrade to Armor Level 3", 0, 4, 2, 100, 180, 4500, 4, 13004, 23705, 13500, 222, 8, { 4, 153, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE PANCERZA DO POZIOMU  4", "Upgrade to Armor Level 4", 0, 4, 3, 100, 240, 6000, 5, 13005, 23706, 13500, 324, 8, { 27, 4, 0, 0 }, { 1, 2, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA MIN GLEBINOWYCH", "Depth Mine Technology", 0, 9, 1, 150, 160, 4000, 11, 13015, 23716, 13501, 120, 37, { 8, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA BOMB GLEBINOWYCH", "Depth Bomb Technology", 0, 20, 1, 120, 300, 7500, 16, 13027, 23727, 13501, 154, 37, { 9, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA HYDRO-FUZJI", "Hydro-Fusion Technology", 0, 1, 1, 80, 30, 750, 20, 13000, 23700, 13502, 18, 56, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 1, { "HF Cannon", nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA SREDNIEJ WIELKOSCI TORPEDY", "Medium Torpedo Technology", 0, 7, 1, 80, 180, 4500, 22, 13013, 23714, 13502, 52, 56, { 1, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "SToLP", "manufacture of Medium Torpedo", nullptr, nullptr } },
        { "TECHNOLOGIA DUZEJ TORPEDY", "Large Torpedo Technology", 0, 8, 1, 120, 240, 6000, 13, 13014, 23715, 13501, 86, 56, { 7, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU 2", "Upgrade to Torpedo Level 2", 0, 6, 1, 100, 100, 2500, 6, 13009, 23710, 13504, 120, 56, { 8, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU 3", "Upgrade to Torpedo Level 3", 0, 6, 2, 100, 160, 4000, 7, 13010, 23711, 13504, 154, 56, { 6, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA SZYBKOSC TORPEDY", "", 0, 153, 1, 200, 260, 6500, 88, 13160, 23873, 13504, 188, 56, { 6, 0, 0, 0 }, { 2, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU  4", "Upgrade to Torpedo Level 4", 0, 6, 3, 100, 240, 6000, 8, 13011, 23712, 13504, 222, 56, { 153, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU  5", "Upgrade to Torpedo Level 5", 0, 6, 4, 100, 360, 9000, 9, 13012, 23713, 13504, 256, 56, { 6, 0, 0, 0 }, { 3, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "EKRAN ANTYLASEROWY", "Anti-Laser Screen", 0, 27, 1, 80, 180, 4500, 19, 13035, 23735, 13500, 290, 56, { 24, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA SZYKOSC STRZELANIA DZIALA HF", "Upgrade HF Cannon Fire-Rate", 0, 11, 1, 100, 240, 6000, 31, 13017, 23718, 13505, 120, 75, { 8, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA ULTRASONOWA", "Ultrasonic Technology", 0, 15, 1, 80, 240, 6000, 25, 13022, 23722, 13502, 154, 75, { 11, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Ultrasonic Generator", nullptr, nullptr, nullptr } },
        { "ULEPSZONY GENERATOR ULTRASONOWY", "Upgrade Ultrasonic Generator", 0, 28, 1, 60, 200, 5000, 34, 13036, 23736, 13505, 188, 75, { 15, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA PSI", "Psy Technology", 0, 21, 1, 150, 260, 6500, 29, 13028, 23728, 13502, 222, 75, { 28, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Psychotron", nullptr, nullptr, nullptr } },
        { "GENERATOR PLAZMY", "Plasma Generator", 0, 24, 1, 100, 300, 7500, 18, 13031, 23731, 13501, 256, 75, { 21, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA DZIALA PLAZMY", "Plasma Cannon Technology", 0, 31, 1, 100, 260, 6500, 32, 13042, 23741, 13502, 290, 75, { 24, 29, 0, 0 }, { 1, 1, 0, 0 }, 1, { "Plasma Cannon", nullptr, nullptr, nullptr } },
        { "BADANIE CORIUM 296", "Corium 296", 0, 25, 1, 80, 360, 9000, 35, 13032, 23732, 13502, 324, 75, { 31, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Plasmatron", "production of Nuclear Torpedoes", nullptr, nullptr } },
        { "ULEPSZONY POCISK HF", "HF-Shell Upgrade", 0, 151, 1, 120, 200, 5000, 85, 13158, 23871, 13504, 222, 94, { 28, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA TERMONUKLEARNA", "Thermo-Nuclear Technology", 0, 26, 1, 250, 360, 9000, 36, 13033, 23733, 13502, 324, 94, { 31, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "TLS", "loading Nuclear Torpedoes into TLS", nullptr, nullptr } },
        { "TECHNOLOGIA TELEPORTACJI", "Teleportation Technology", 0, 19, 1, 200, 300, 7500, 28, 13026, 23726, 13502, 188, 124, { 17, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Teleporter", nullptr, nullptr, nullptr } },
        { "WYKRYWANIE TELEPORTACJIN", "Detect Teleportation", 0, 64, 1, 200, 300, 7500, 73, 13072, 23770, 13505, 120, 124, { 13, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TELEPOWLOKA", "Teleshield", 0, 59, 1, 200, 360, 9000, 68, 13066, 23764, 13502, 154, 105, { 64, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Teleshield", nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA EKRANU ROZPRASZAJACEGO", "Disperser Screen Technology", 0, 17, 1, 100, 260, 6500, 27, 13024, 23724, 13502, 154, 124, { 64, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Anti-Sonar Shield", nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA ANTYFANTOMOWA", "Anti-Phantom Technology", 0, 18, 1, 150, 360, 9000, 15, 13025, 23725, 13501, 222, 124, { 19, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "SYSTEM OBRONNY PRZECIW BRONI MASOWEGO RAZENIA", "Anti-Mass Weapons Defense System", 0, 66, 1, 300, 360, 9000, 77, 13149, 23772, 13502, 256, 143, { 18, 29, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE SIDEL LASEROWYCH", "Laser Snare Detection", 0, 60, 1, 150, 360, 9000, 71, 13067, 23765, 13500, 154, 143, { 64, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ZWIEKSZONE LADOWANIE TRANCENTRUM", "Increase TranCenter Recharge", 0, 33, 1, 150, 260, 6500, 39, 13041, 23740, 13505, 256, 124, { 18, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE MIN GLEBINOWYCH I AKUSTYCZNYCH ", "", 0, 154, 1, 120, 240, 6000, 72, 13161, 23874, 13500, 154, 162, { 64, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA RUCHOMEGO SONARU", "Mobile Sonar Technology", 0, 10, 1, 80, 120, 3000, 12, 13016, 23717, 13504, 86, 188, { 2, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONE WYPOSAZENIE NAJEZDZCY", "Marauder Equipment Upgrade", 0, 152, 1, 150, 300, 7500, 84, 13159, 23872, 13500, 188, 188, { 61, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "SONAR O DUZYM ZASIEGU", "Long Range Sonar", 0, 2, 1, 50, 180, 4500, 21, 13001, 23701, 13502, 52, 207, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { "Sonar", nullptr, nullptr, nullptr } },
        { "BADANIE KLUCZA SZYFRUJACEGO WROGA", "Research Enemy Cipher Key", 0, 13, 1, 80, 180, 4500, 24, 13019, 23720, 13502, 86, 207, { 2, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "CentComp", nullptr, nullptr, nullptr } },
        { "WYPOSAZENIE NAJEZDZCY", "Marauder Equipment", 0, 14, 1, 70, 260, 6500, 14, 13020, 23721, 13501, 120, 207, { 13, 5, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA UMIEJETNOSC WLAMYWANIA SIE", "Upgrade Hack Ability", 0, 61, 1, 150, 120, 3000, 65, 13068, 23766, 13505, 154, 207, { 14, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ZWIEKSZA BEZPIECZENSTWO INFORMACJI", "Increase Information Security", 0, 63, 1, 150, 160, 4000, 66, 13071, 23769, 13505, 188, 207, { 61, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONY ZASIEG SONARU", "Upgrade Sonar Range", 0, 29, 1, 50, 90, 2250, 33, 13037, 23737, 13505, 222, 207, { 63, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA KONTROLI REKINOW", "Shark Control Technology", 0, 12, 1, 120, 200, 5000, 23, 13018, 23719, 13502, 188, 226, { 61, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Shark Control", nullptr, nullptr, nullptr } },
        { "ULEPSZONY SYSTEM KONTROLI REKINOW", "Upgrade Shark Control", 0, 62, 1, 150, 160, 4000, 63, 13069, 23767, 13505, 222, 226, { 12, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "CYBERTECHNOLOGIA", "Cyber Technology", 0, 22, 1, 100, 320, 8000, 17, 13029, 23729, 13501, 222, 245, { 12, 5, 0, 0 }, { 1, 2, 0, 0 }, 1, { "Cyber Laboratory", "production of Cyberdolphin", nullptr, nullptr } },
        { "ULEPSZENIE SILNIKA DO KLASY  4", "Upgrade to Engine Class 4", 0, 5, 3, 80, 360, 9000, 2, 13008, 23709, 13500, 256, 245, { 22, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE SILNIKA DO KLASY 2", "Upgrade to Engine Class 2", 0, 5, 1, 80, 100, 2500, 0, 13006, 23707, 13500, 86, 264, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "RUCHOMA REPLODZ", "Mobile RepSub", 0, 3, 1, 100, 90, 2250, 10, 13002, 23703, 13501, 120, 264, { 5, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE PORUSZANIA", "Upgrade to Mobility", 0, 16, 1, 50, 240, 6000, 26, 13023, 23723, 13505, 154, 264, { 3, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE SILNIKA DO KLASY  3", "Upgrade to Engine Class 3", 0, 5, 2, 80, 200, 5000, 1, 13007, 23708, 13500, 188, 264, { 16, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA EFEKTYWNIEJSZEJ NAPRAWY", "Improve Repair Technology", 0, 32, 1, 50, 120, 3000, 38, 13040, 23739, 13505, 222, 264, { 5, 0, 0, 0 }, { 2, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONE CENTRUM ZBROJEN", "ArmCenter Upgrade", 0, 23, 1, 50, 160, 4000, 30, 13030, 23730, 13505, 256, 264, { 32, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA PRODUKTYWNOSC MIN", "Upgrade Extractor Productivity", 0, 30, 1, 70, 120, 3000, 37, 13038, 23738, 13505, 290, 264, { 23, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE PANCERZA DO POZIOMU  2", "", 1, 129, 1, 100, 240, 6000, 3, 13003, 23704, 13500, 86, 8, { 34, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE PANCERZA DO POZIOMU  3", "", 1, 129, 2, 100, 200, 5000, 4, 13004, 23705, 13500, 154, 8, { 129, 131, 0, 0 }, { 1, 2, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE PANCERZA DO POZIOMU  4", "", 1, 129, 3, 100, 360, 9000, 5, 13005, 23706, 13500, 222, 8, { 129, 131, 0, 0 }, { 2, 4, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "EKRAN ANTYULTRASONOWY", "Anti-Ultrasonic Screen", 1, 45, 1, 90, 200, 5000, 49, 13054, 23753, 13500, 290, 8, { 57, 129, 0, 0 }, { 1, 3, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "POWLOKA ENERGETYCZNA", "Energy Shield", 1, 50, 1, 250, 360, 9000, 59, 13034, 23734, 13502, 154, 152, { 38, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Power Protector", nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA TORPED ROZSZCZEPIALNYCH", "Splinter Torpedo Technology", 1, 34, 1, 80, 120, 3000, 40, 13043, 23742, 13501, 52, 37, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU 2", "", 1, 131, 1, 100, 180, 4500, 6, 13009, 23710, 13504, 86, 37, { 34, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU 3", "", 1, 131, 2, 100, 240, 6000, 7, 13010, 23711, 13504, 120, 37, { 131, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU  4", "", 1, 131, 3, 100, 300, 7500, 8, 13011, 23712, 13504, 154, 37, { 131, 0, 0, 0 }, { 2, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE TORPEDY DO POZIOMU  5", "", 1, 131, 4, 100, 400, 10000, 9, 13012, 23713, 13504, 188, 37, { 131, 0, 0, 0 }, { 3, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA SZYBKOSC TORPEDY", "Torpedo Speed Upgrade", 1, 150, 1, 200, 300, 7500, 88, 13157, 23870, 13504, 222, 37, { 131, 0, 0, 0 }, { 4, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA TORPEDA ROZSZCZEPIALNA", "Splinter Torpedo Upgrade", 1, 149, 1, 150, 300, 7500, 87, 13156, 23869, 13504, 256, 37, { 150, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA SIDEL LASEROWYCH", "Laser Snare Technology", 1, 37, 1, 80, 120, 3000, 43, 13046, 23745, 13501, 86, 56, { 34, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA MIN MAGNETYCZNYCH", "Magnetic Mine Technology", 1, 41, 1, 150, 240, 6000, 53, 13050, 23749, 13502, 154, 56, { 40, 131, 0, 0 }, { 1, 2, 0, 0 }, 1, { "Magnetic-Mine Launcher", "manufacture of", nullptr, nullptr } },
        { "TECHNOLOGIA POCISKOW KASETOWYCH", "Cassette Shell Technology", 1, 40, 1, 130, 180, 4500, 47, 13049, 23748, 13501, 120, 56, { 37, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TORPEDY ELEKTRO-MAGNETYCZNE", "Electro-Magnetic Torpedoes", 1, 43, 1, 100, 240, 6000, 55, 13052, 23751, 13502, 188, 56, { 41, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Electro-Magnetic Launcher", "manufacture of", nullptr, nullptr } },
        { "TECHNOLOGIA PROMIENI PARALIZUJACYCH", "Paralyzing Rays Technology", 1, 46, 1, 150, 240, 6000, 50, 13055, 23754, 13501, 222, 56, { 43, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA TORPEDA EM", "EM Torpedo Upgrade", 1, 147, 1, 250, 300, 7500, 86, 13154, 23867, 13504, 256, 56, { 46, 58, 0, 0 }, { 1, 1, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA LEKKIEGO LASERA", "Light Laser Technology", 1, 51, 1, 80, 30, 750, 51, 13059, 23758, 13502, 18, 85, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 1, { "Light Laser", nullptr, nullptr, nullptr } },
        { "ULEPSZONY ZASIEG LEKKIEGO LASERA", "Upgrade Light Laser Range", 1, 54, 1, 180, 260, 6500, 69, 13061, 23759, 13505, 52, 85, { 51, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA RUBINOWEGO LASERA", "Ruby Laser Technology", 1, 35, 1, 100, 90, 2250, 41, 13044, 23743, 13501, 86, 85, { 54, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA LASERU GAZOWEGO", "Gas Laser Technology", 1, 38, 1, 120, 280, 7000, 52, 13047, 23746, 13502, 120, 85, { 35, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Heavy Laser", nullptr, nullptr, nullptr } },
        { "ULEPSZONY ZASIEG CIEZKIEGO LASERA", "Upgrade Heavy Laser Range", 1, 55, 1, 200, 300, 7500, 70, 13062, 23760, 13505, 154, 85, { 38, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA SZYBKOSC LEKKIEGO LASERA", "Upgrade Light Laser Rate", 1, 56, 1, 200, 240, 6000, 74, 13063, 23761, 13505, 188, 85, { 55, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA SZYBKOSC LASERA NA LODZI", "Upgrade Heavy Laser Rate", 1, 58, 1, 250, 300, 7500, 67, 13065, 23763, 13500, 222, 85, { 56, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA SZYBKOSC CIEZKIEGO  LASERA", "Upgrade Subs Laser Rate", 1, 57, 1, 250, 300, 7500, 75, 13064, 23762, 13505, 256, 85, { 58, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA BOMB LASEROWYCH", "Laser Bomb Technology", 1, 49, 1, 150, 500, 12500, 58, 13058, 23757, 13502, 324, 85, { 137, 57, 0, 0 }, { 1, 1, 0, 0 }, 1, { "Laser Bomb Launcher", "production of Laser", nullptr, nullptr } },
        { "SYSTEM OBRONNY PRZECIW BRONI MASOWEGO RAZENIA", "", 1, 67, 1, 300, 360, 9000, 76, 13150, 23773, 13502, 358, 190, { 137, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "BADANIE DOSTAW ENERGII", "Research Energy Supply", 1, 42, 1, 170, 180, 4500, 54, 13051, 23750, 13502, 154, 104, { 38, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Power Station", nullptr, nullptr, nullptr } },
        { "ULEPSZENE DOSTAW ENERGII", "Upgrade Energy Supply", 1, 47, 1, 80, 120, 3000, 56, 13056, 23755, 13505, 188, 104, { 42, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA EKRANU OCHRONNEGO", "Protective Screen Technology", 1, 48, 1, 200, 240, 6000, 57, 13057, 23756, 13502, 222, 104, { 47, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { "Protective Shield Generators", nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA TELEPORTACJI", "", 1, 135, 1, 200, 300, 7500, 28, 13026, 23854, 13502, 154, 133, { 38, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE TELEPORTACJIN", "", 1, 143, 1, 200, 200, 5000, 73, 13072, 23863, 13505, 188, 133, { 135, 141, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ZWIEKSZONE LADOWANIE TELEPORTERA", "Increase Teleporter Recharge", 1, 140, 1, 150, 300, 7500, 39, 13166, 23860, 13505, 222, 133, { 143, 47, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE MIN GLEBINOWYCH I AKUSTYCZNYCH", "Depth & Acoustic Mine Detection", 1, 65, 1, 150, 120, 3000, 72, 13073, 23771, 13500, 222, 152, { 143, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE SIDEL LASEROWYCH", "", 1, 145, 1, 150, 200, 5000, 71, 13067, 23865, 13500, 256, 152, { 65, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONE WYPOSAZENIE RAIDERA", "Raider Equipment Upgrade", 1, 148, 1, 180, 240, 6000, 83, 13155, 23868, 13500, 256, 171, { 44, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ASDIC O DUZYM ZASIEGU", "Long Range ASDIC", 1, 127, 1, 50, 60, 1500, 60, 13060, 23702, 13502, 52, 190, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { "ASDIC", nullptr, nullptr, nullptr } },
        { "BADANIE KLUCZA SZYFRUJACEGO WROGA", "", 1, 132, 1, 80, 60, 1500, 24, 13019, 23850, 13502, 86, 190, { 127, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYPOSAZENIE RAIDERA", "Raider Equipment", 1, 52, 1, 100, 90, 2250, 42, 13164, 23851, 13501, 120, 190, { 132, 130, 35, 0 }, { 1, 1, 1, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA UMIEJETNOSC WLAMYWANIA SIE", "", 1, 141, 1, 150, 60, 1500, 65, 13068, 23861, 13505, 154, 190, { 52, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA UKRYTEGO ZWIADOWCY", "", 1, 39, 1, 100, 180, 4500, 45, 13048, 23747, 13501, 188, 190, { 141, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYPOSAZENIE FANTOMA", "Phantom Equipment", 1, 44, 1, 180, 240, 6000, 48, 13053, 23752, 13501, 222, 190, { 39, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "ZWIEKSZA BEZPIECZENSTWO INFORMACJI", "", 1, 142, 1, 150, 100, 2500, 66, 13071, 23862, 13505, 256, 190, { 44, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONY ASDIC", "ASDIC Upgrade", 1, 137, 1, 50, 120, 3000, 60, 13165, 23857, 13505, 290, 190, { 142, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA EKRANU ROZPRASZAJACEGO", "", 1, 134, 1, 100, 120, 3000, 62, 13024, 23853, 13502, 154, 209, { 52, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA ZAGLUSZACZA RADIA", "Radio Clutter Technology", 1, 36, 1, 70, 45, 1125, 44, 13045, 23744, 13500, 188, 209, { 141, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ZWIEKSZONE LADOWANIE FANTOMA", "Phantom Recharge Upgrade", 1, 146, 1, 200, 360, 9000, 82, 13153, 23866, 13500, 256, 209, { 44, 0, 0, 0 }, { 1, 0, 0, 0 }, 1, { nullptr, nullptr, nullptr, nullptr } },
        { "CYBERTECHNOLOGIA", "", 1, 53, 1, 120, 180, 4500, 46, 13029, 23855, 13501, 222, 228, { 130, 39, 0, 0 }, { 2, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE SILNIKA DO KLASY  4", "", 1, 130, 3, 80, 280, 7000, 2, 13008, 23709, 13500, 256, 228, { 53, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE SILNIKA DO KLASY 2", "", 1, 130, 1, 80, 100, 2500, 0, 13006, 23707, 13500, 86, 247, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "RUCHOMA PLATFORMA NAPRAWCZA", "Mobile Repair Platform", 1, 128, 1, 100, 90, 2250, 10, 13163, 23849, 13501, 120, 247, { 130, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE PORUSZANIA", "", 1, 133, 1, 100, 180, 4500, 26, 13023, 23852, 13505, 154, 247, { 128, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE SILNIKA DO KLASY  3", "", 1, 130, 2, 80, 180, 4500, 1, 13007, 23708, 13500, 188, 247, { 133, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA FABRYKA AMUNICJI", "Munitions Factory Upgrade", 1, 136, 1, 50, 120, 3000, 30, 13162, 23856, 13505, 222, 247, { 130, 0, 0, 0 }, { 2, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONA PRODUKTYWNOSC MIN", "Upgrade Mine Productivity", 1, 138, 1, 70, 120, 3000, 37, 13038, 23858, 13505, 256, 247, { 136, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA EFEKTYWNIEJSZEJ NAPRAWY", "", 1, 139, 1, 50, 120, 3000, 38, 13040, 23859, 13505, 290, 247, { 138, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "SYSTEM OBRONNY PRZECIW BRONI MASOWEGO RAZENIA", "", 2, 99, 1, 3000, 460, 11500, 117, 13106, 23808, 13502, 365, 240, { 105, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "LOKALNA TELEPORTACJA", "Teleportation Detection", 2, 95, 1, 3000, 240, 6000, 134, 13101, 23803, 13501, 161, 27, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "BRAMA TELEPORTACJI", "Teleportation Gate", 2, 96, 1, 2800, 300, 7500, 135, 13102, 23804, 13502, 195, 27, { 95, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Gate", nullptr, nullptr, nullptr } },
        { "IDENTYFIKACJA TELEPORTACJI", "ID Teleportation", 2, 98, 1, 1000, 360, 9000, 137, 13105, 23807, 13505, 263, 27, { 100, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "SATELITA LASERU GAZOWEGO", "Gas Laser Satellite", 2, 97, 1, 4000, 360, 9000, 116, 13104, 23806, 13502, 331, 27, { 98, 104, 0, 0 }, { 1, 1, 0, 0 }, 0, { "Gas Laser Satellite Launcher", nullptr, nullptr, nullptr } },
        { "PARALIZATOR KWANTOWY", "Quantum Paralyzer", 2, 100, 1, 3000, 300, 7500, 168, 13176, 23875, 13502, 229, 27, { 96, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Quantum Paralyzer", nullptr, nullptr, nullptr } },
        { "WLAMANIE DO DANYCH", "Data Intrusion", 2, 73, 1, 1000, 120, 3000, 94, 13079, 23779, 13505, 161, 50, { 69, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZENIE BEZPIECZENSTWA DANYCH", "Data Security Upgrade", 2, 71, 1, 1200, 120, 3000, 92, 13077, 23777, 13505, 195, 50, { 73, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "EKSPLORER O DUZYM ZAKRESIE", "High Range Explorer", 2, 119, 1, 1500, 180, 4500, 162, 13129, 23837, 13501, 229, 50, { 96, 72, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "BIOSONAR UKRYTYCH OBIEKTOW", "Anti-Stealth BioSonar", 2, 74, 1, 3200, 300, 7500, 95, 13080, 23780, 13505, 263, 50, { 77, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "BOMBY PROZNIOWE", "Vacuum Bomb", 2, 117, 1, 4000, 460, 11500, 155, 13127, 23835, 13505, 365, 27, { 97, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Vacuum Bomb Launcher", "production of Vacuum", nullptr, nullptr } },
        { "TECHNOLOGIA BIOSONAROWA", "BioSonar Technology", 2, 68, 1, 1000, 120, 3000, 89, 13074, 23774, 13502, 93, 69, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { "BioSonar Station", nullptr, nullptr, nullptr } },
        { "LUDZKI KLUCZ SZYFRUJACY", "Human Cipher Key", 2, 69, 1, 1200, 180, 4500, 90, 13075, 23775, 13505, 127, 69, { 68, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "LOKATOR", "Locator", 2, 70, 1, 2000, 240, 6000, 91, 13076, 23776, 13505, 161, 69, { 69, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ANTYSONAROWY EKRAN ROZPRASZAJACY", "Anti-Sonar Dispersion Screen", 2, 72, 1, 2000, 300, 7500, 93, 13078, 23778, 13505, 195, 69, { 70, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE TELEPORTACJI", "Local Teleportation", 2, 77, 1, 3000, 240, 6000, 98, 13083, 23783, 13505, 229, 69, { 72, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE MIN GLEBINOWYCH I AKUSTYCZNYCH", "", 2, 75, 1, 2800, 240, 6000, 96, 13081, 23781, 13500, 263, 69, { 77, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ZDOBYWANIE WROGA", "Enemy Capture", 2, 114, 1, 1500, 240, 6000, 145, 13124, 23832, 13501, 195, 88, { 109, 70, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONE ZDOBYWANIE WROGA", "Enemy Capture Upgrade", 2, 144, 1, 4000, 180, 4500, 167, 13151, 10001, 13505, 229, 88, { 114, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYKRYWANIE SIDEL LASEROWYCH", "", 2, 76, 1, 2800, 240, 6000, 97, 13082, 23782, 13500, 263, 88, { 77, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "2 POZIOM MOZLIWOSCI PORUSZANIA", "Movement Level 2", 2, 109, 1, 1000, 120, 3000, 99, 13119, 23825, 13500, 161, 107, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYMYKANIE SIE TORPEDOM", "Torpedo Evasion", 2, 110, 1, 1000, 180, 4500, 102, 13120, 23828, 13500, 195, 107, { 109, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "3 POZIOM MOZLIWOSCI PORUSZANIA", "Movement Level 3", 2, 109, 2, 1200, 180, 4500, 100, 13145, 23826, 13500, 229, 107, { 110, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "4 POZIOM MOZLIWOSCI PORUSZANIA", "Movement Level 4", 2, 109, 3, 1400, 240, 6000, 101, 13146, 23827, 13500, 263, 107, { 109, 78, 0, 0 }, { 2, 3, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "OCHRONA ENERGII NA POZIOMIE 2", "Energy Conservation Level 2", 2, 78, 1, 1000, 60, 1500, 129, 13084, 23784, 13503, 93, 126, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TRANSFORMACJA CORIUM NA ENERGIE", "Transform Corium to Energy", 2, 79, 1, 1000, 120, 3000, 132, 13085, 23787, 13502, 127, 126, { 78, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Energy Converter", nullptr, nullptr, nullptr } },
        { "OCHRONA ENERGII NA POZIOMIE 3", "Energy Conservation Level 3", 2, 78, 2, 2000, 240, 6000, 130, 13139, 23785, 13503, 161, 126, { 79, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "OCHRONA ENERGII NA POZIOMIE 4", "Energy Conservation Level 4", 2, 78, 3, 3000, 300, 7500, 131, 13140, 23786, 13503, 229, 126, { 78, 0, 0, 0 }, { 2, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "URZADZENIE NAPRAWY MOLEKULARNEJ", "Molecular Repair Facility", 2, 108, 1, 3000, 240, 6000, 143, 13118, 23824, 13502, 229, 145, { 78, 80, 0, 0 }, { 2, 1, 0, 0 }, 0, { "Molecular Repair Facility", nullptr, nullptr, nullptr } },
        { "TRANSMITER ENERGII", "Energy Transmitter", 2, 81, 1, 2000, 180, 4500, 103, 13087, 23789, 13501, 161, 164, { 79, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONY  AKUMULATOR ENERGII", "Upgrade Energy Accumulator", 2, 80, 1, 1500, 180, 4500, 133, 13086, 23788, 13505, 195, 164, { 81, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONY ARSENAL", "Upgrade Arsenal", 2, 120, 1, 1400, 180, 4500, 146, 13130, 23838, 13505, 263, 164, { 80, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "TECHNOLOGIA UNICESTWIANIA", "Annihilation Technology", 2, 82, 1, 3000, 300, 7500, 104, 13088, 23790, 13502, 161, 183, { 79, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Recyclotron", nullptr, nullptr, nullptr } },
        { "WYZSZA WERSJA EKSTRAKTORA SYLIKONU", "Upgrade Silicon Extractor", 2, 83, 1, 1200, 180, 4500, 105, 13089, 23791, 13505, 195, 183, { 82, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYZSZA WERSJA KOLEKTORA CORIUM", "Upgrade Corium Collector", 2, 84, 1, 1200, 180, 4500, 106, 13090, 23792, 13505, 229, 183, { 83, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONE TEMPO REGENARACJI", "Upgrade Regeneration Speed", 2, 102, 1, 1800, 300, 7500, 113, 13111, 23815, 13503, 263, 202, { 80, 101, 0, 0 }, { 1, 2, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "UZUPELNIAJACA POWLOKA LASEROWA", "Laser Replenish Sheath", 2, 104, 1, 2000, 300, 7500, 114, 13113, 23819, 13500, 297, 202, { 102, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "2 POZIOM PANCERZA SYLIKONU ", "Silicon Armor Level 2", 2, 101, 1, 1000, 240, 6000, 107, 13110, 23812, 13500, 195, 221, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "3 POZIOM PANCERZA SYLIKONU ", "Silicon Armor Level 3", 2, 101, 2, 2000, 300, 7500, 108, 13141, 23813, 13500, 229, 221, { 101, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "4 POZIOM PANCERZA SYLIKONU", "Silicon Armor Level 4", 2, 101, 3, 3000, 360, 9000, 109, 13142, 23814, 13500, 263, 221, { 101, 0, 0, 0 }, { 2, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "EKRAN ANTYULTRASONOWY", "", 2, 106, 1, 2000, 240, 6000, 141, 13116, 23822, 13500, 297, 221, { 101, 0, 0, 0 }, { 3, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "OBRONNA OSLONA JONOWA (20%)", "Ion Defensive Sheath (20%)", 2, 103, 1, 1500, 180, 4500, 110, 13112, 23816, 13500, 229, 240, { 101, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "OBRONNA OSLONA JONOWA(30%)", "Ion Defensive Sheath (30%)", 2, 103, 2, 2500, 240, 6000, 111, 13143, 23817, 13500, 263, 240, { 103, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "OBRONNA OSLONA JONOWA (40%)", "Ion Defensive Sheath (40%)", 2, 103, 3, 3500, 300, 7500, 112, 13144, 23818, 13500, 297, 240, { 103, 101, 0, 0 }, { 2, 3, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "POWLOKA PSI", "Psy-Shield", 2, 105, 1, 3000, 180, 4500, 140, 13115, 23821, 13500, 331, 240, { 103, 0, 0, 0 }, { 3, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYRZUTNIA POCISKOW GAZOWYCH", "Gas Shell Launcher", 2, 86, 1, 1500, 120, 3000, 120, 13092, 23794, 13502, 59, 259, { 85, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "GSL", "manufacture of Gas Shell", nullptr, nullptr } },
        { "ULEPSZONE TEMPO STRZALU GSL", "Upgrade GSL Fire Rate", 2, 93, 1, 2000, 180, 4500, 127, 13099, 23801, 13505, 93, 259, { 86, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "GENERATOR POLA JONOWEGO", "Ion Field Generator", 2, 107, 1, 3200, 240, 6000, 142, 13117, 23823, 13502, 263, 259, { 103, 123, 0, 0 }, { 1, 1, 0, 0 }, 0, { "Ion Field Generator", nullptr, nullptr, nullptr } },
        { "PULSAR SPOLARYZOWANEJ PLAZMY", "Polarized Plasma Pulsar", 2, 85, 1, 1000, 30, 750, 119, 13091, 23793, 13502, 25, 278, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { "PP Pulsar", nullptr, nullptr, nullptr } },
        { "SOLITON OSCYLATOR", "Soliton Oscillator", 2, 87, 1, 5000, 200, 5000, 121, 13093, 23795, 13502, 59, 278, { 85, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Soliton Oscillator", nullptr, nullptr, nullptr } },
        { "REFLEKTOR JONOWY", "Ion Reflector", 2, 92, 1, 3000, 300, 7500, 126, 13098, 23800, 13502, 93, 278, { 87, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Ion Reflector", nullptr, nullptr, nullptr } },
        { "ULEPSZONY REFLEKTOR JONOWY", "Upgrade Ion Reflector", 2, 123, 1, 3500, 300, 7500, 147, 13133, 23842, 13505, 127, 278, { 92, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "PODWOJNA WIEZYCZKA PLAZMY", "Double Plasma Turret Gun", 2, 89, 1, 2000, 180, 4500, 123, 13095, 23797, 13502, 59, 297, { 85, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "DPT Gun", nullptr, nullptr, nullptr } },
        { "ULEPSZONE TEMPO STRZALU PULSARU PP", "Upgrade PP Pulsar Fire Rate", 2, 90, 1, 2000, 180, 4500, 124, 13096, 23798, 13505, 93, 297, { 89, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONY ZASIEG BRONI DPT", "Upgrade DPT Gun Range", 2, 121, 1, 2000, 180, 4500, 163, 13131, 23839, 13505, 127, 297, { 90, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "PARCHER", "Parcher", 2, 94, 1, 2500, 240, 6000, 128, 13100, 23802, 13502, 161, 297, { 121, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Parcher", nullptr, nullptr, nullptr } },
        { "2 POZIOM POCISKU ENERGETYCZNEGO", "Energy Shell Level 2", 2, 122, 1, 2000, 200, 5000, 158, 13132, 23840, 13504, 161, 354, { 112, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "3 POZIOM POCISKU ENERGETYCZNEGO", "Energy Shell Level 3", 2, 122, 2, 2500, 240, 6000, 159, 13147, 23841, 13504, 263, 354, { 118, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "WYRZUTNIA BIO-MIN", "Bio-Mine Launcher", 2, 88, 1, 3000, 180, 4500, 122, 13094, 23796, 13502, 59, 316, { 85, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Bio-Mine Launcher", "manufacture of Bio-Mine", nullptr, nullptr } },
        { "WYRZUTNIA MIN SKOKOWYCH", "Jump-Mine Launcher", 2, 91, 1, 3500, 200, 5000, 125, 13097, 23799, 13502, 93, 316, { 88, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { "Jump-Mine Launcher", "manufacture of Jump-Mine", nullptr, nullptr } },
        { "POCISKI  NEURO-PARALIZUJACE", "Neuro-Paralysis Shells", 2, 113, 1, 2000, 460, 11500, 154, 13123, 23831, 13501, 195, 316, { 115, 94, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "2 POZIOM BRONI  PARALIZUJACEJ ", "Paralytic Weapon Level 2", 2, 125, 1, 2000, 360, 9000, 156, 13135, 23844, 13504, 229, 316, { 113, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "3 POZIOM BRONI  PARALIZUJACEJ", "Paralytic Weapon Level 3", 2, 125, 2, 2500, 360, 9000, 157, 13148, 23845, 13504, 263, 316, { 125, 123, 0, 0 }, { 1, 1, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "NEUTRALIZACJA POLA ENERGETYCZNEGO", "Energy Shield Neutralization", 2, 126, 1, 3000, 400, 10000, 161, 13136, 23846, 13504, 297, 316, { 125, 0, 0, 0 }, { 2, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "JONOWE POCISKI KASETOWE", "Ion Cassette Shells", 2, 111, 1, 1500, 240, 6000, 149, 13121, 23829, 13501, 93, 335, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "POCISKI BHE", "BHE Shells", 2, 112, 1, 1600, 300, 7500, 150, 13122, 23830, 13501, 127, 335, { 111, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "POCISKI BIO-KWASOWE", "Bio-Acid Shells", 2, 115, 1, 1800, 300, 7500, 151, 13125, 23833, 13501, 161, 335, { 112, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "MINY AKUSTYCZNE", "Acoustic Mines", 2, 116, 1, 1500, 240, 6000, 152, 13126, 23834, 13500, 195, 335, { 115, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONE POCISKI BIO-KWASOWE", "Upgrade Bio-Acid Shells", 2, 118, 1, 1200, 180, 4500, 153, 13128, 23836, 13504, 229, 335, { 116, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
        { "ULEPSZONY ZAIEG  POCISKU BHE", "BHE Shell Range Upgrade", 2, 124, 1, 1500, 180, 4500, 160, 13134, 23843, 13504, 263, 335, { 118, 0, 0, 0 }, { 1, 0, 0, 0 }, 0, { nullptr, nullptr, nullptr, nullptr } },
    };
    n = int(sizeof(k) / sizeof(k[0]));
    return k;
}

inline const Tech *list(int &n) { return table(n); }

// Wezel po numerze technologii i poziomie; -1 gdy nie ma.
inline int indexOf(int id, int level)
{
    int n = 0;
    const Tech *k = table(n);
    for (int i = 0; i < n; ++i)
        if (k[i].id == id && k[i].level == level) return i;
    return -1;
}

inline int countFor(int side)
{
    int n = 0;
    const Tech *k = table(n);
    int c = 0;
    for (int i = 0; i < n; ++i) if (k[i].side == side) ++c;
    return c;
}

// Szukanie po nazwie z przewodnika - tak podaja wymagania
// `cost::bldTech` i `cost::unitTech`.
inline const Tech *find(const char *name)
{
    int n = 0;
    const Tech *k = table(n);
    for (int i = 0; i < n; ++i)
        if (k[i].guide[0] && std::strcmp(k[i].guide, name) == 0)
            return &k[i];
    return nullptr;
}

inline int unlocks(int i)
{
    int n = 0;
    const Tech *k = table(n);
    if (i < 0 || i >= n) return 0;
    int c = 0;
    for (int q = 0; q < 4; ++q) if (k[i].unlock[q]) ++c;
    return c;
}

inline const char *firstUnlock(int i)
{
    int n = 0;
    const Tech *k = table(n);
    if (i < 0 || i >= n || !k[i].unlock[0]) return "-";
    return k[i].unlock[0];
}

}  // namespace tech
