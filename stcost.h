// Koszty i czasy budowy - wygenerowane przez tools/gen_cost.py.
//
// Nazwy pochodza z Editor\AiScript.dfn, liczby z przewodnika
// (guide/stats.tsv). Dla Silikonow kolumna `metal` to w rzeczywistosci
// krzem, a `zloto` to energia - gra trzyma je w tych samych licznikach.
#pragma once

namespace cost {

struct Price { int corium, metal, secs; };

// Kto co buduje: 12 Konstruktor (WS), 24 Asembler (BO), 25 Prototyp (SI).
inline int builderFor(int side) { return side == 0 ? 12 : (side == 1 ? 24 : 25); }
inline int sideOfBuilder(int type)
{ return type == 12 ? 0 : (type == 24 ? 1 : (type == 25 ? 2 : -1)); }

// Lodzie, indeks = TOBJ 1..40.
inline Price unitPrice(int type)
{
    switch (type) {
    case 1: return { 40, 200, 20 };   // WS SENTINEL
    case 2: return { 80, 400, 30 };   // WS HUNTER
    case 3: return { 300, 1000, 70 };   // WS CRUISER
    case 4: return { 250, 1000, 50 };   // WS DC_BOMBER
    case 5: return { 90, 700, 40 };   // WS MINE_LAYER
    case 6: return { 100, 800, 40 };   // WS MARAUDER
    case 7: return { 0, 800, 30 };   // WS REPSUB
    case 8: return { 0, 600, 20 };   // WS TRANSUB
    case 9: return { 20, 200, 30 };   // WS CYBERWORM
    case 10: return { 600, 1000, 60 };   // WS TERMINATOR
    case 11: return { 150, 700, 60 };   // WS LIBERATOR
    case 12: return { 0, 1000, 30 };   // WS CONSTRUCTOR
    case 13: return { 50, 300, 20 };   // BO FIGHTER
    case 14: return { 80, 500, 40 };   // BO DESTROYER
    case 15: return { 300, 1200, 70 };   // BO HEAVY_CRUISER
    case 16: return { 150, 800, 50 };   // BO INVADER
    case 17: return { 80, 700, 40 };   // BO DEFENDER
    case 18: return { 60, 700, 40 };   // BO RAIDER
    case 19: return { 0, 900, 30 };   // BO REPAIR_PLATFORM
    case 20: return { 0, 600, 20 };   // BO CARGO_SUB
    case 21: return { 200, 400, 35 };   // BO CYBERDOLPHIN
    case 22: return { 250, 1000, 60 };   // BO PHANTOM
    case 23: return { 200, 1200, 60 };   // BO AVENGER
    case 24: return { 0, 1000, 30 };   // BO ASSEMBLER
    case 25: return { 0, 50, 7 };   // SI CAPSULE_PROTOTYPE
    case 26: return { 0, 100, 15 };   // SI TRANSPORT
    case 27: return { 0, 400, 40 };   // SI SUPPLIER
    case 28: return { 400, 600, 70 };   // SI PARALYSIS_PROBE
    case 29: return { 40, 600, 40 };   // SI REPLENISHER
    case 30: return { 50, 300, 20 };   // SI SHS_SUB
    case 31: return { 110, 1000, 50 };   // SI DREADNOUGHT
    case 32: return { 180, 800, 45 };   // SI ESCORT
    case 33: return { 200, 700, 60 };   // SI BIO_ACID_ASSAULTER
    case 34: return { 100, 500, 40 };   // SI USURPER
    case 35: return { 150, 600, 40 };   // SI VERMIN
    case 36: return { 40, 200, 20 };   // SI EXPLORER
    case 37: return { 40, 500, 20 };   // BO STEALTH_SCOUT
    case 38: return { 0, 0, 0 };   // WS WS_FLAGSHIP
    case 39: return { 0, 0, 0 };   // BO BO_FLAGSHIP
    case 40: return { 0, 0, 0 };   // SI SI_FLAGSHIP
    default: break;
    }
    return { 0, 0, 0 };
}

// Budynki: numer TOBJ i strona 0=WS 1=BO 2=SI, bo jeden numer to inny
// budynek u kazdej rasy.
inline Price bldPrice(int tobj, int side)
{
    switch (tobj * 4 + side) {
    case 200: return { 0, 1800, 90 };   // WS SUBCENTER
    case 201: return { 0, 1800, 90 };   // BO DOCKYARD
    case 204: return { 0, 700, 30 };   // WS REPCENTER
    case 205: return { 0, 700, 30 };   // BO REPAIR_DOCK
    case 208: return { 400, 1000, 40 };   // WS ARMCENTER
    case 209: return { 400, 1000, 40 };   // BO MUNITIONS_FACTORY
    case 212: return { 300, 1400, 60 };   // WS TECHCENTER
    case 213: return { 300, 1400, 60 };   // BO RESEARCH_LABORATORY
    case 216: return { 0, 700, 15 };   // WS SONAR
    case 217: return { 0, 700, 15 };   // BO ASDIC
    case 220: return { 450, 1500, 90 };   // WS TRANCENTER
    case 221: return { 450, 1500, 90 };   // BO TELEPORTER
    case 228: return { 0, 300, 15 };   // WS CORIUM_EXTRACTOR
    case 229: return { 0, 300, 15 };   // BO CORIUM_MINE
    case 232: return { 0, 600, 15 };   // WS GOLD_EXTRACTOR
    case 233: return { 0, 600, 15 };   // BO GOLD_SUBLIMATOR
    case 236: return { 0, 300, 40 };   // WS DEPOT
    case 237: return { 0, 300, 40 };   // BO SILO
    case 240: return { 0, 500, 60 };   // WS INFOCENTER
    case 241: return { 0, 500, 60 };   // BO CENTCOMP
    case 244: return { 100, 800, 20 };   // WS DISPERSER
    case 245: return { 100, 800, 20 };   // BO ANTI_SONAR_SHIELD
    case 248: return { 40, 500, 20 };   // WS HF_CANNON
    case 252: return { 80, 800, 30 };   // WS STOLP
    case 256: return { 0, 600, 60 };   // WS CYBERCENTER
    case 260: return { 0, 500, 25 };   // WS SHARK_CONTROL
    case 264: return { 300, 600, 25 };   // WS ULTRASONIC_GENERATOR
    case 268: return { 300, 900, 45 };   // WS PSYCHOTRON
    case 272: return { 1000, 600, 60 };   // WS PLASMATRON
    case 276: return { 2000, 1000, 60 };   // WS TLS
    case 281: return { 50, 400, 20 };   // BO LIGHT_LASER
    case 285: return { 120, 600, 25 };   // BO MAGNETIC_MINE_LAUNCHER
    case 289: return { 250, 500, 30 };   // BO POWER_STATION
    case 293: return { 200, 600, 30 };   // BO CYBER_LABORATORY
    case 297: return { 100, 800, 30 };   // BO HEAVY_LASER
    case 301: return { 80, 700, 30 };   // BO ELECTRO_MAGNETIC_LAUNCHER
    case 305: return { 400, 1000, 40 };   // BO PROTECTIVE_SHIELD_GENERATOR
    case 309: return { 1000, 1800, 90 };   // BO POWER_PROTECTOR
    case 313: return { 3000, 1000, 60 };   // BO LASER_BOMB_LAUNCHER
    case 316: return { 0, 200, 15 };   // WS METAL_EXTRACTOR
    case 317: return { 0, 200, 15 };   // BO METAL_MINE
    case 320: return { 0, 500, 15 };   // WS AIR_EXTRACTOR
    case 321: return { 0, 500, 15 };   // BO O2_SUBLIMATOR
    case 324: return { 1000, 900, 30 };   // WS PLASMA_CANNON
    case 328: return { 0, 700, 40 };   // WS TRADECENTER
    case 329: return { 0, 700, 60 };   // BO MARKET
    case 334: return { 300, 1200, 45 };   // SI COMMAND_HUB
    case 338: return { 200, 600, 60 };   // SI MOBILITY_HUB_MODULE
    case 342: return { 200, 600, 60 };   // SI SUBMARINE_HUB_MODULE
    case 346: return { 200, 600, 60 };   // SI ENERGY_HUB_MODULE
    case 350: return { 200, 600, 60 };   // SI REGENERATION_HUB_MODULE
    case 354: return { 200, 600, 60 };   // SI STRUCTURE_HUB_MODULE
    case 358: return { 200, 600, 60 };   // SI INTELLIGENCE_HUB_MODULE
    case 362: return { 200, 600, 60 };   // SI SUPER_TECH_HUB_MODULE
    case 366: return { 400, 900, 45 };   // SI ARSENAL
    case 370: return { 0, 1500, 90 };   // SI PROTOPLASM_GENERATOR
    case 374: return { 50, 600, 30 };   // SI BIOSONAR_STATION
    case 378: return { 0, 400, 20 };   // SI CORIUM_COLLECTOR
    case 382: return { 200, 800, 40 };   // SI ENERGY_CONVERTER
    case 386: return { 0, 300, 20 };   // SI CORIUM_SILO
    case 390: return { 0, 800, 30 };   // SI ENERGY_ACCUMULATOR
    case 394: return { 50, 600, 30 };   // SI REPLENISH_POD
    case 398: return { 200, 1000, 45 };   // SI RECYCLOTRON
    case 402: return { 0, 700, 30 };   // SI SILICON_EXTRACTOR
    case 406: return { 500, 800, 30 };   // SI SOLITON_OSCILLATOR
    case 410: return { 300, 900, 25 };   // SI GAS_SHELL_LAUNCHER
    case 414: return { 200, 800, 20 };   // SI DPT_GUN
    case 418: return { 200, 700, 30 };   // SI ION_REFLECTOR
    case 422: return { 300, 900, 35 };   // SI JUMP_MINE_LAUNCHER
    case 426: return { 300, 800, 35 };   // SI BIO_MINE_LAUNCHER
    case 430: return { 50, 500, 20 };   // SI PP_PULSAR
    case 434: return { 100, 1000, 60 };   // SI GATE
    case 438: return { 400, 900, 50 };   // SI ION_FIELD_GENERATOR
    case 442: return { 500, 1000, 90 };   // SI MOLECULAR_REPAIR_FACILITY
    case 444: return { 200, 600, 60 };   // WS TELESHIELD
    case 450: return { 3000, 1100, 50 };   // SI GAS_LASER_SATELLITE_LAUNCHER
    case 454: return { 300, 900, 35 };   // SI PARCHER
    case 458: return { 3000, 800, 90 };   // SI VACUUM_BOMB_LAUNCHER
    case 462: return { 600, 1000, 60 };   // SI QUANTUM_PARALYZER
    default: break;
    }
    return { 0, 0, 0 };
}

// Ktorej cywilizacji jest lodz: 0 WS, 1 BO, 2 SI, -1 gdy nie wiadomo.
inline int unitSide(int type)
{
    switch (type) {
    case 1: return 0;
    case 2: return 0;
    case 3: return 0;
    case 4: return 0;
    case 5: return 0;
    case 6: return 0;
    case 7: return 0;
    case 8: return 0;
    case 9: return 0;
    case 10: return 0;
    case 11: return 0;
    case 12: return 0;
    case 13: return 1;
    case 14: return 1;
    case 15: return 1;
    case 16: return 1;
    case 17: return 1;
    case 18: return 1;
    case 19: return 1;
    case 20: return 1;
    case 21: return 1;
    case 22: return 1;
    case 23: return 1;
    case 24: return 1;
    case 25: return 2;
    case 26: return 2;
    case 27: return 2;
    case 28: return 2;
    case 29: return 2;
    case 30: return 2;
    case 31: return 2;
    case 32: return 2;
    case 33: return 2;
    case 34: return 2;
    case 35: return 2;
    case 36: return 2;
    case 37: return 1;
    case 38: return 0;
    case 39: return 1;
    case 40: return 2;
    default: break;
    }
    return -1;
}

// Zakladka okna budowania: 0 uzytkowe, 1 wiezyczki, 2 surowce,
// 3 specjalne. Sekcje wprost z przewodnika, nie zgadywane.
inline int bldCategory(int tobj, int side)
{
    switch (tobj * 4 + side) {
    case 200: return 0;
    case 201: return 0;
    case 204: return 0;
    case 205: return 0;
    case 208: return 0;
    case 209: return 0;
    case 212: return 0;
    case 213: return 0;
    case 216: return 0;
    case 217: return 0;
    case 220: return 3;
    case 221: return 3;
    case 228: return 2;
    case 229: return 2;
    case 232: return 2;
    case 233: return 2;
    case 236: return 2;
    case 237: return 2;
    case 240: return 0;
    case 241: return 0;
    case 244: return 3;
    case 245: return 3;
    case 248: return 1;
    case 252: return 1;
    case 256: return 0;
    case 260: return 3;
    case 264: return 1;
    case 268: return 3;
    case 272: return 3;
    case 276: return 3;
    case 281: return 1;
    case 285: return 1;
    case 289: return 0;
    case 293: return 0;
    case 297: return 1;
    case 301: return 1;
    case 305: return 3;
    case 309: return 3;
    case 313: return 3;
    case 316: return 2;
    case 317: return 2;
    case 320: return 2;
    case 321: return 2;
    case 324: return 1;
    case 328: return 2;
    case 329: return 2;
    case 334: return 0;
    case 338: return 0;
    case 342: return 0;
    case 346: return 0;
    case 350: return 0;
    case 354: return 0;
    case 358: return 0;
    case 362: return 0;
    case 366: return 0;
    case 370: return 0;
    case 374: return 0;
    case 378: return 2;
    case 382: return 2;
    case 386: return 2;
    case 390: return 2;
    case 394: return 0;
    case 398: return 2;
    case 402: return 2;
    case 406: return 1;
    case 410: return 1;
    case 414: return 1;
    case 418: return 1;
    case 422: return 1;
    case 426: return 1;
    case 430: return 1;
    case 434: return 3;
    case 438: return 3;
    case 442: return 3;
    case 444: return 3;
    case 450: return 3;
    case 454: return 1;
    case 458: return 3;
    case 462: return 3;
    default: break;
    }
    return 0;
}

// Nazwy do pokazania. Tablica w exe pod 0x007B8330 trzyma nazwy REKORDOW
// (dyaws_body, repd_body), nie napisy, wiec nazwa idzie z przewodnika.
// Wymagana technologia - z linijki `Prerequisites:` w przewodniku.
// Pusty napis znaczy brak wymagan.
inline const char *bldTech(int tobj, int side)
{
    switch (tobj * 4 + side) {
    case 216: return "Long Range Sonar";   // WS SONAR
    case 217: return "Long Range ASDIC";   // BO ASDIC
    case 220: return "Teleportation Technology";   // WS TRANCENTER
    case 221: return "Teleportation Technology";   // BO TELEPORTER
    case 240: return "Research Enemy Cipher Key";   // WS INFOCENTER
    case 241: return "Research Enemy Cipher Key";   // BO CENTCOMP
    case 244: return "Disperser Screen Technology";   // WS DISPERSER
    case 245: return "Disperser Screen Technology";   // BO ANTI_SONAR_SHIELD
    case 248: return "Hydro-Fusion Technology";   // WS HF_CANNON
    case 252: return "Medium Torpedo Technology";   // WS STOLP
    case 256: return "Cyber Technology";   // WS CYBERCENTER
    case 260: return "Shark Control Technology";   // WS SHARK_CONTROL
    case 264: return "Ultrasonic Technology";   // WS ULTRASONIC_GENERATOR
    case 268: return "Psy Technology";   // WS PSYCHOTRON
    case 272: return "Corium 296";   // WS PLASMATRON
    case 276: return "Thermo-Nuclear Technology";   // WS TLS
    case 281: return "Light Laser Technology";   // BO LIGHT_LASER
    case 285: return "Magnetic-Mine Technology";   // BO MAGNETIC_MINE_LAUNCHER
    case 289: return "Research Energy Supply";   // BO POWER_STATION
    case 293: return "Cyber Technology";   // BO CYBER_LABORATORY
    case 297: return "Gas-Laser Technology";   // BO HEAVY_LASER
    case 301: return "Electro-Magnetic Torpedoes";   // BO ELECTRO_MAGNETIC_LAUNCHER
    case 305: return "Protective Screen Technology";   // BO PROTECTIVE_SHIELD_GENERATOR
    case 309: return "Energy Shield";   // BO POWER_PROTECTOR
    case 313: return "Laser Bomb Technology";   // BO LASER_BOMB_LAUNCHER
    case 324: return "Plasma Cannon Technology";   // WS PLASMA_CANNON
    case 374: return "BioSonar Technology";   // SI BIOSONAR_STATION
    case 382: return "Transform Corium to Energy";   // SI ENERGY_CONVERTER
    case 398: return "Annihilation Technology";   // SI RECYCLOTRON
    case 406: return "Soliton Oscillator";   // SI SOLITON_OSCILLATOR
    case 410: return "Gas Shell Launcher";   // SI GAS_SHELL_LAUNCHER
    case 414: return "Double Plasma Turret Gun";   // SI DPT_GUN
    case 418: return "Ion Reflector";   // SI ION_REFLECTOR
    case 422: return "Jump-Mine Launcher";   // SI JUMP_MINE_LAUNCHER
    case 426: return "Bio-Mine Launcher";   // SI BIO_MINE_LAUNCHER
    case 430: return "Polarized Plasma Pulsar";   // SI PP_PULSAR
    case 434: return "Teleportation Gate";   // SI GATE
    case 438: return "Ion Field Generator";   // SI ION_FIELD_GENERATOR
    case 442: return "Molecular Repair Facility";   // SI MOLECULAR_REPAIR_FACILITY
    case 444: return "Teleshield";   // WS TELESHIELD
    case 450: return "Gas Laser Satellite";   // SI GAS_LASER_SATELLITE_LAUNCHER
    case 454: return "Parcher";   // SI PARCHER
    case 458: return "Vacuum Bomb";   // SI VACUUM_BOMB_LAUNCHER
    case 462: return "Quantum Paralyzer";   // SI QUANTUM_PARALYZER
    default: break;
    }
    return "";
}

inline const char *unitTech(int type)
{
    switch (type) {
    case 3: return "Large Torpedo Technology";   // WS CRUISER
    case 4: return "Depth Bomb Technology";   // WS DC_BOMBER
    case 5: return "Depth-Mine Technology";   // WS MINE_LAYER
    case 6: return "Marauder Equipment";   // WS MARAUDER
    case 7: return "Mobile RepSub";   // WS REPSUB
    case 9: return "Cyber Technology";   // WS CYBERWORM
    case 10: return "Plasma Generator";   // WS TERMINATOR
    case 11: return "Anti-Phantom Technology";   // WS LIBERATOR
    case 14: return "Splinter Torpedo Technology";   // BO DESTROYER
    case 15: return "Ruby Laser Technology";   // BO HEAVY_CRUISER
    case 16: return "Cassette Shell Technology";   // BO INVADER
    case 17: return "Laser Snare Technology";   // BO DEFENDER
    case 18: return "Raider Equipment";   // BO RAIDER
    case 19: return "Mobile Repair Platform";   // BO REPAIR_PLATFORM
    case 21: return "Cyber Technology";   // BO CYBERDOLPHIN
    case 22: return "Phantom Equipment";   // BO PHANTOM
    case 23: return "Paralyzing Rays Technology";   // BO AVENGER
    case 28: return "Neuro-Paralysis Shells";   // SI PARALYSIS_PROBE
    case 29: return "Energy Transmitter";   // SI REPLENISHER
    case 31: return "BHE Shells";   // SI DREADNOUGHT
    case 32: return "Ion Cassette Shells";   // SI ESCORT
    case 33: return "Bio-Acid Shells";   // SI BIO_ACID_ASSAULTER
    case 34: return "Enemy Capture";   // SI USURPER
    case 35: return "Psy-Shield";   // SI VERMIN
    case 36: return "High Range Explorer";   // SI EXPLORER
    case 37: return "Stealth Scout Technology";   // BO STEALTH_SCOUT
    default: break;
    }
    return "";
}

inline const char *bldLabel(int tobj, int side)
{
    switch (tobj * 4 + side) {
    case 200: return "Subcenter";
    case 201: return "Dockyard";
    case 204: return "Repcenter";
    case 205: return "Repair Dock";
    case 208: return "Armcenter";
    case 209: return "Munitions Factory";
    case 212: return "Techcenter";
    case 213: return "Research Laboratory";
    case 216: return "Sonar";
    case 217: return "Asdic";
    case 220: return "Trancenter";
    case 221: return "Teleporter";
    case 228: return "Corium Extractor";
    case 229: return "Corium Mine";
    case 232: return "Gold Extractor";
    case 233: return "Gold Sublimator";
    case 236: return "Depot";
    case 237: return "Silo";
    case 240: return "Infocenter";
    case 241: return "Centcomp";
    case 244: return "Disperser";
    case 245: return "Anti Sonar Shield";
    case 248: return "Hf Cannon";
    case 252: return "Stolp";
    case 256: return "Cybercenter";
    case 260: return "Shark Control";
    case 264: return "Ultrasonic Generator";
    case 268: return "Psychotron";
    case 272: return "Plasmatron";
    case 276: return "Tls";
    case 281: return "Light Laser";
    case 285: return "Magnetic Mine Launcher";
    case 289: return "Power Station";
    case 293: return "Cyber Laboratory";
    case 297: return "Heavy Laser";
    case 301: return "Electro Magnetic Launcher";
    case 305: return "Protective Shield Generator";
    case 309: return "Power Protector";
    case 313: return "Laser Bomb Launcher";
    case 316: return "Metal Extractor";
    case 317: return "Metal Mine";
    case 320: return "Air Extractor";
    case 321: return "O2 Sublimator";
    case 324: return "Plasma Cannon";
    case 328: return "Tradecenter";
    case 329: return "Market";
    case 334: return "Command Hub";
    case 338: return "Mobility Hub Module";
    case 342: return "Submarine Hub Module";
    case 346: return "Energy Hub Module";
    case 350: return "Regeneration Hub Module";
    case 354: return "Structure Hub Module";
    case 358: return "Intelligence Hub Module";
    case 362: return "Super Tech Hub Module";
    case 366: return "Arsenal";
    case 370: return "Protoplasm Generator";
    case 374: return "Biosonar Station";
    case 378: return "Corium Collector";
    case 382: return "Energy Converter";
    case 386: return "Corium Silo";
    case 390: return "Energy Accumulator";
    case 394: return "Replenish Pod";
    case 398: return "Recyclotron";
    case 402: return "Silicon Extractor";
    case 406: return "Soliton Oscillator";
    case 410: return "Gas Shell Launcher";
    case 414: return "Dpt Gun";
    case 418: return "Ion Reflector";
    case 422: return "Jump Mine Launcher";
    case 426: return "Bio Mine Launcher";
    case 430: return "Pp Pulsar";
    case 434: return "Gate";
    case 438: return "Ion Field Generator";
    case 442: return "Molecular Repair Facility";
    case 444: return "Teleshield";
    case 450: return "Gas Laser Satellite Launcher";
    case 454: return "Parcher";
    case 458: return "Vacuum Bomb Launcher";
    case 462: return "Quantum Paralyzer";
    default: break;
    }
    return nullptr;
}

// Co wolno postawic danej rasie - lista numerow TOBJ w kolejnosci z
// przewodnika, czyli tej samej co w panelu gry.
inline const int *bldListWS(int &n)   // 24 budynkow
{
    static const int k[] = { 50, 51, 52, 53, 54, 55, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 79, 80, 81, 82, 111 };
    n = 24;
    return k;
}
inline const int *bldListBO(int &n)   // 23 budynkow
{
    static const int k[] = { 50, 51, 52, 53, 54, 55, 57, 58, 59, 60, 61, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 82 };
    n = 23;
    return k;
}
inline const int *bldListSI(int &n)   // 32 budynkow
{
    static const int k[] = { 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 112, 113, 114, 115 };
    n = 32;
    return k;
}

// Lodzie danej rasy - to, co schodzi z pochylni glownego budynku.
inline const int *unitListWS(int &n)   // 13 lodzi
{
    static const int k[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 38 };
    n = 13;
    return k;
}
inline const int *unitListBO(int &n)   // 14 lodzi
{
    static const int k[] = { 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 37, 39 };
    n = 14;
    return k;
}
inline const int *unitListSI(int &n)   // 13 lodzi
{
    static const int k[] = { 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 40 };
    n = 13;
    return k;
}

inline const int *unitList(int side, int &n)
{ return side == 0 ? unitListWS(n) : (side == 1 ? unitListBO(n) : unitListSI(n)); }

inline const int *bldList(int side, int &n)
{ return side == 0 ? bldListWS(n) : (side == 1 ? bldListBO(n) : bldListSI(n)); }

} // namespace cost
