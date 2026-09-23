#pragma once
// Wygenerowane przez tools/gen_tv.py - nie edytuj recznie.
//
// Nazwa rekordu podgladu `TV_*` dla typu obiektu, wprost z exe:
// `FUN_00526100` to switch po typie, a `param_1[7]` w nim to rasa
// w numeracji g_byGameMode (1 WS, 2 BO, 3 SI).

namespace tv {

struct Ent { int type; const char *ws, *bo, *si; };

inline const Ent *unitTable(int &n)
{
    static const Ent t[] = {
        { 1, "TV_SENTINEL", "TV_SENTINEL", "TV_SENTINEL" },
        { 2, "TV_HUNTER", "TV_HUNTER", "TV_HUNTER" },
        { 3, "TV_CRUISER", "TV_CRUISER", "TV_CRUISER" },
        { 4, "TV_DCBOMBER", "TV_DCBOMBER", "TV_DCBOMBER" },
        { 5, "TV_MINELAYER", "TV_MINELAYER", "TV_MINELAYER" },
        { 6, "TV_RAIDER1", "TV_RAIDER1", "TV_RAIDER1" },
        { 7, "TV_REPPLATFORM", "TV_REPPLATFORM", "TV_REPPLATFORM" },
        { 8, "TV_TRANSPORT1", "TV_TRANSPORT1", "TV_TRANSPORT1" },
        { 9, "TV_CYBERWORM", "TV_CYBERWORM", "TV_CYBERWORM" },
        { 10, "TV_TERMINATOR", "TV_TERMINATOR", "TV_TERMINATOR" },
        { 11, "TV_LIBERATOR", "TV_LIBERATOR", "TV_LIBERATOR" },
        { 12, "TV_CONSTRPLATFORM1", "TV_CONSTRPLATFORM1", "TV_CONSTRPLATFORM1" },
        { 13, "TV_CYBERKILLER", "TV_CYBERKILLER", "TV_CYBERKILLER" },
        { 14, "TV_DESTROYER", "TV_DESTROYER", "TV_DESTROYER" },
        { 15, "TV_HCRUISER", "TV_HCRUISER", "TV_HCRUISER" },
        { 16, "TV_INVADER", "TV_INVADER", "TV_INVADER" },
        { 17, "TV_DEFENDER", "TV_DEFENDER", "TV_DEFENDER" },
        { 18, "TV_RAIDER2", "TV_RAIDER2", "TV_RAIDER2" },
        { 19, "TV_REPPOWPLATFORM", "TV_REPPOWPLATFORM", "TV_REPPOWPLATFORM" },
        { 20, "TV_TRANSPORT2", "TV_TRANSPORT2", "TV_TRANSPORT2" },
        { 21, "TV_CYBERDOLPHIN", "TV_CYBERDOLPHIN", "TV_CYBERDOLPHIN" },
        { 22, "TV_PHANTOM", "TV_PHANTOM", "TV_PHANTOM" },
        { 23, "TV_AVENGER", "TV_AVENGER", "TV_AVENGER" },
        { 24, "TV_CONSTRPLATFORM2", "TV_CONSTRPLATFORM2", "TV_CONSTRPLATFORM2" },
        { 25, "TV_CAPSULE", "TV_CAPSULE", "TV_CAPSULE" },
        { 26, "TV_TRANSPORT3", "TV_TRANSPORT3", "TV_TRANSPORT3" },
        { 27, "TV_SUPPLYSUB", "TV_SUPPLYSUB", "TV_SUPPLYSUB" },
        { 28, "TV_PPROBE", "TV_PPROBE", "TV_PPROBE" },
        { 29, "TV_REPLINISHER", "TV_REPLINISHER", "TV_REPLINISHER" },
        { 30, "TV_SHSSUB", "TV_SHSSUB", "TV_SHSSUB" },
        { 31, "TV_DREDNOUGHT", "TV_DREDNOUGHT", "TV_DREDNOUGHT" },
        { 32, "TV_ESCORT", "TV_ESCORT", "TV_ESCORT" },
        { 33, "TV_ASSAULTER", "TV_ASSAULTER", "TV_ASSAULTER" },
        { 34, "TV_USUPPER", "TV_USUPPER", "TV_USUPPER" },
        { 35, "TV_GHOSTMAKER", "TV_GHOSTMAKER", "TV_GHOSTMAKER" },
        { 36, "TV_EXPLORER", "TV_EXPLORER", "TV_EXPLORER" },
        { 37, "TV_STEALTHSCOUT", "TV_STEALTHSCOUT", "TV_STEALTHSCOUT" },
        { 38, "TV_FLAGWS", "TV_FLAGWS", "TV_FLAGWS" },
        { 39, "TV_RAIDER2", "TV_RAIDER2", "TV_RAIDER2" },
        { 40, "TV_USUPPER", "TV_USUPPER", "TV_USUPPER" },
    };
    n = int(sizeof(t) / sizeof(t[0]));
    return t;
}

inline const Ent *bldTable(int &n)
{
    static const Ent t[] = {
        { 50, "TV_WSDOCKYARD", "TV_BODOCKYARD", "TV_BODOCKYARD" },
        { 51, "TV_WSRDOCK", "TV_BORDOCK", "TV_BORDOCK" },
        { 52, "TV_WSMFACTORY", "TV_BOMFACTORY", "TV_BOMFACTORY" },
        { 53, "TV_WSREASLAB", "TV_BOREASLAB", "TV_BOREASLAB" },
        { 54, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 55, "TV_WSTELEPORT", "TV_BOTELEPORT", "TV_BOTELEPORT" },
        { 56, "TV_BOMMINE", "TV_BOMMINE", "TV_BOMMINE" },
        { 57, "TV_WSCMINE", "TV_BOCMINE", "TV_BOCMINE" },
        { 58, "TV_WSGOLDPLANT", "TV_BOGOLDPLANT", "TV_BOGOLDPLANT" },
        { 59, "TV_WSDEPOT", "TV_BODEPOT", "TV_BODEPOT" },
        { 60, "TV_WSINFOCENTER", "TV_BOINFOCENTER", "TV_BOINFOCENTER" },
        { 61, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 62, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 63, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 64, "TV_CCENTEREWS", "TV_CCENTEREWS", "TV_CCENTEREWS" },
        { 65, "TV_SHARKCTRL", "TV_SHARKCTRL", "TV_SHARKCTRL" },
        { 66, "TV_USGENERATOR", "TV_USGENERATOR", "TV_USGENERATOR" },
        { 67, "TV_PSYCHOTRON", "TV_PSYCHOTRON", "TV_PSYCHOTRON" },
        { 68, "TV_PLASMATRON", "TV_PLASMATRON", "TV_PLASMATRON" },
        { 69, "TV_TLS", "TV_TLS", "TV_TLS" },
        { 70, "TV_SWIM2", "TV_SWIM2", "TV_SWIM2" },
        { 71, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 72, "TV_PSTATION", "TV_PSTATION", "TV_PSTATION" },
        { 73, "TV_CCENTERBO", "TV_CCENTERBO", "TV_CCENTERBO" },
        { 74, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 75, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 76, "TV_ISO", "TV_ISO", "TV_ISO" },
        { 77, "TV_PPROTECT", "TV_PPROTECT", "TV_PPROTECT" },
        { 78, "TV_LBL", "TV_LBL", "TV_LBL" },
        { 79, "TV_WSMMINE", "TV_BOMMINE", "TV_BOMMINE" },
        { 80, "TV_WSAIRPLANT", "TV_BOAIRPLANT", "TV_BOAIRPLANT" },
        { 81, "TV_SWIM1", "TV_SWIM1", "TV_SWIM1" },
        { 82, "TV_WSRCTELEPORT", "TV_BORCTELEPORT", "TV_BORCTELEPORT" },
        { 83, "TV_COMMANDHUB", "TV_COMMANDHUB", "TV_COMMANDHUB" },
        { 84, "TV_CMDHUB_MOB", "TV_CMDHUB_MOB", "TV_CMDHUB_MOB" },
        { 85, "TV_CMDHUB_MIL", "TV_CMDHUB_MIL", "TV_CMDHUB_MIL" },
        { 86, "TV_CMDHUB_EN", "TV_CMDHUB_EN", "TV_CMDHUB_EN" },
        { 87, "TV_CMDHUB_PROT", "TV_CMDHUB_PROT", "TV_CMDHUB_PROT" },
        { 88, "TV_CMDHUB_DEF", "TV_CMDHUB_DEF", "TV_CMDHUB_DEF" },
        { 89, "TV_CMDHUB_PROSP", "TV_CMDHUB_PROSP", "TV_CMDHUB_PROSP" },
        { 90, "TV_CMDHUB_BIOP", "TV_CMDHUB_BIOP", "TV_CMDHUB_BIOP" },
        { 91, "TV_ARSENAL", "TV_ARSENAL", "TV_ARSENAL" },
        { 92, "TV_PROTOPLASMAGEN", "TV_PROTOPLASMAGEN", "TV_PROTOPLASMAGEN" },
        { 93, "TV_BIOSONAR", "TV_BIOSONAR", "TV_BIOSONAR" },
        { 94, "TV_SCORIUMMINE", "TV_SCORIUMMINE", "TV_SCORIUMMINE" },
        { 95, "TV_ENERGYCONVERTER", "TV_ENERGYCONVERTER", "TV_ENERGYCONVERTER" },
        { 96, "TV_CORIUMSILO", "TV_CORIUMSILO", "TV_CORIUMSILO" },
        { 97, "TV_ENERGYACCUMULATOR", "TV_ENERGYACCUMULATOR", "TV_ENERGYACCUMULATOR" },
        { 98, "TV_REPLENISHPOD", "TV_REPLENISHPOD", "TV_REPLENISHPOD" },
        { 99, "TV_RECYCLOTRON", "TV_RECYCLOTRON", "TV_RECYCLOTRON" },
        { 100, "TV_SILICONEXTRACTOR", "TV_SILICONEXTRACTOR", "TV_SILICONEXTRACTOR" },
        { 101, "TV_GAMMAOSCILLATOR", "TV_GAMMAOSCILLATOR", "TV_GAMMAOSCILLATOR" },
        { 102, "TV_GASCANNON", "TV_GASCANNON", "TV_GASCANNON" },
        { 103, "TV_PARALYSER", "TV_PARALYSER", "TV_PARALYSER" },
        { 104, "TV_IONREFLECTOR", "TV_IONREFLECTOR", "TV_IONREFLECTOR" },
        { 105, "TV_JUMPMINE", "TV_JUMPMINE", "TV_JUMPMINE" },
        { 106, "TV_BIOACID", "TV_BIOACID", "TV_BIOACID" },
        { 107, "TV_SPLASMACANNON", "TV_SPLASMACANNON", "TV_SPLASMACANNON" },
        { 108, "TV_GATE1", "TV_GATE1", "TV_GATE1" },
        { 109, "TV_IONFIELDGEN", "TV_IONFIELDGEN", "TV_IONFIELDGEN" },
        { 110, "TV_MOLECULARREP", "TV_MOLECULARREP", "TV_MOLECULARREP" },
        { 111, "TV_TELESHIELD", "TV_TELESHIELD", "TV_TELESHIELD" },
        { 112, "TV_GLSATLUNCHER", "TV_GLSATLUNCHER", "TV_GLSATLUNCHER" },
        { 113, "TV_PARCHER", "TV_PARCHER", "TV_PARCHER" },
        { 114, "TV_VBLAUNCHER", "TV_VBLAUNCHER", "TV_VBLAUNCHER" },
        { 115, "TV_QPARALISER", "TV_QPARALISER", "TV_QPARALISER" },
    };
    n = int(sizeof(t) / sizeof(t[0]));
    return t;
}

inline const Ent *otherTable(int &n)
{
    static const Ent t[] = {
        { 166, "TV_DMINE", "TV_DMINE", "TV_DMINE" },
        { 167, "TV_LSNARE", "TV_LSNARE", "TV_LSNARE" },
        { 189, "TV_AMINE", "TV_AMINE", "TV_AMINE" },
        { 221, "TV_CORIUMSRC", "TV_CORIUMSRC", "TV_CORIUMSRC" },
        { 222, "TV_METALSRC", "TV_METALSRC", "TV_METALSRC" },
        { 224, "TV_TERMOSRC", "TV_TERMOSRC", "TV_TERMOSRC" },
        { 253, "TV_ARTEFACTWS", "TV_ARTEFACTSI", "TV_ARTEFACTSI" },
        { 254, "TV_CONTAINER1", "TV_CONTAINER1", "TV_CONTAINER1" },
    };
    n = int(sizeof(t) / sizeof(t[0]));
    return t;
}

inline const char *pick(const Ent *t, int n, int type, int side)
{
    for (int i = 0; i < n; ++i)
        if (t[i].type == type)
            return side == 1 ? t[i].bo : (side == 2 ? t[i].si : t[i].ws);
    return nullptr;
}

// side: 0 WS, 1 BO, 2 SI - konwencja remake'u, nie g_byGameMode.
inline const char *forUnit(int type, int side)
{ int n = 0; const Ent *t = unitTable(n); return pick(t, n, type, side); }
inline const char *forBld(int tobj, int side)
{ int n = 0; const Ent *t = bldTable(n); return pick(t, n, tobj, side); }
inline const char *forOther(int type, int side)
{ int n = 0; const Ent *t = otherTable(n); return pick(t, n, type, side); }

}  // namespace tv
