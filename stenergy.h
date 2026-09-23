// WYGENEROWANE przez tools/gen_energy.py - nie edytowac recznie.
//
// ST.exe: lodzie 007e0620, budynki 007e23c0.
// Zero oznacza brak zapasu. Regeneracja: stregeneration.h (004d7040),
// nie dawna interpolacja przykladu Escorty z przewodnika.
#pragma once

namespace energy {

inline int stored(int tobj)
{
    switch (tobj) {
    case 25: return 100;   // CAPSULE_PROTOTYPE
    case 26: return 100;   // TRANSPORT
    case 27: return 100;   // SUPPLIER
    case 28: return 400;   // PARALYSIS_PROBE
    case 29: return 400;   // REPLENISHER
    case 30: return 100;   // SHS_SUB
    case 31: return 500;   // DREADNOUGHT
    case 32: return 400;   // ESCORT
    case 33: return 300;   // BIO_ACID_ASSAULTER
    case 34: return 300;   // USURPER
    case 35: return 400;   // VERMIN
    case 36: return 100;   // EXPLORER
    case 40: return 500;   // SI_FLAGSHIP
    case 83: return 800;   // COMMAND_HUB
    case 84: return 200;   // MOBILITY_HUB_MODULE
    case 85: return 200;   // SUBMARINE_HUB_MODULE
    case 86: return 200;   // ENERGY_HUB_MODULE
    case 87: return 200;   // REGENERATION_HUB_MODULE
    case 88: return 200;   // STRUCTURE_HUB_MODULE
    case 89: return 200;   // INTELLIGENCE_HUB_MODULE
    case 90: return 200;   // SUPER_TECH_HUB_MODULE
    case 91: return 600;   // ARSENAL
    case 92: return 500;   // PROTOPLASM_GENERATOR
    case 93: return 200;   // BIOSONAR_STATION
    case 94: return 100;   // CORIUM_COLLECTOR
    case 95: return 400;   // ENERGY_CONVERTER
    case 96: return 100;   // CORIUM_SILO
    case 97: return 400;   // ENERGY_ACCUMULATOR
    case 98: return 200;   // REPLENISH_POD
    case 99: return 400;   // RECYCLOTRON
    case 100: return 200;   // SILICON_EXTRACTOR
    case 101: return 800;   // SOLITON_OSCILLATOR
    case 102: return 400;   // GAS_SHELL_LAUNCHER
    case 103: return 400;   // DPT_GUN
    case 104: return 600;   // ION_REFLECTOR
    case 105: return 500;   // JUMP_MINE_LAUNCHER
    case 106: return 500;   // BIO_MINE_LAUNCHER
    case 107: return 300;   // PP_PULSAR
    case 108: return 500;   // GATE
    case 109: return 500;   // ION_FIELD_GENERATOR
    case 110: return 700;   // MOLECULAR_REPAIR_FACILITY
    case 112: return 800;   // GAS_LASER_SATELLITE_LAUNCHER
    case 113: return 500;   // PARCHER
    case 114: return 600;   // VACUUM_BOMB_LAUNCHER
    case 115: return 900;   // QUANTUM_PARALYZER
    default: return 0;
    }
}

}  // namespace energy
