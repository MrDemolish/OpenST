// Nazwy animacji budynkow i to, ktore z nich chodza CIAGLE.
//
// Wszystko wprost z tablicy pod 0x007B8330: wpis ma DWANASCIE bajtow i trzy
// wskazniki - bryla, podstawa nazwy barwy gracza i **animacja**. Trzy wpisy na
// numer TOBJ (WS, BO, SI), czyli 36 bajtow na budynek; indeks liczy sie jako
// `(rasa + grupa*3) * 12`, tak jak w TLOBaseTy::LoadImages.
//
// Animacja ladowana jest do sekwencji 12; osobna tablica pod 0x00790C2C mowi,
// czy zostaje **wystartowana na stale**. Tylko 12 z 66 grup ma tam jedynke -
// reszta ma animacje wczytana, ale odpalana zdarzeniem (wydobywak rusza lukiem
// tylko przy przeladunku, moduly CHub przy pracy).
//
// Osobna tablica pod 0x00791A10 zaznacza budynki z osobnym paskiem `_cover`.
#pragma once

namespace units {

constexpr int ANI_GROUPS = 66;
inline const char *buildingAni(int tobj, int side)
{
    static const char *kA[ANI_GROUPS][3] = {
        { nullptr         , nullptr         , nullptr          },   // 50
        { "repd_ani"      , "repd_ani"      , nullptr          },   // 51
        { "mfacws_ani"    , "mfacbo_ani"    , nullptr          },   // 52
        { "rlabws_ani"    , "rlabbo_ani"    , nullptr          },   // 53
        { nullptr         , nullptr         , nullptr          },   // 54
        { "telews_ani"    , "telebo_ani"    , nullptr          },   // 55
        { nullptr         , nullptr         , nullptr          },   // 56
        { "coriws_ani"    , "coribo_ani"    , nullptr          },   // 57
        { "sublws_ani"    , "sublbo_ani"    , nullptr          },   // 58
        { nullptr         , nullptr         , nullptr          },   // 59
        { nullptr         , nullptr         , nullptr          },   // 60
        { "despws_ani"    , "despbo_ani"    , nullptr          },   // 61
        { nullptr         , nullptr         , nullptr          },   // 62
        { nullptr         , nullptr         , nullptr          },   // 63
        { nullptr         , nullptr         , nullptr          },   // 64
        { "sha_ani"       , "sha_ani"       , nullptr          },   // 65
        { "ultr_ani"      , "ultr_ani"      , nullptr          },   // 66
        { "psyh_ani"      , "psyh_ani"      , nullptr          },   // 67
        { nullptr         , nullptr         , nullptr          },   // 68
        { "tls_ani"       , "tls_ani"       , nullptr          },   // 69
        { nullptr         , nullptr         , nullptr          },   // 70
        { nullptr         , nullptr         , nullptr          },   // 71
        { nullptr         , nullptr         , nullptr          },   // 72
        { nullptr         , nullptr         , nullptr          },   // 73
        { nullptr         , nullptr         , nullptr          },   // 74
        { nullptr         , nullptr         , nullptr          },   // 75
        { nullptr         , nullptr         , nullptr          },   // 76
        { nullptr         , nullptr         , nullptr          },   // 77
        { "htec_ani"      , "htec_ani"      , nullptr          },   // 78
        { "mminews_ani"   , nullptr         , nullptr          },   // 79
        { "airws_ani"     , "airbo_ani"     , nullptr          },   // 80
        { nullptr         , nullptr         , nullptr          },   // 81
        { nullptr         , nullptr         , nullptr          },   // 82
        { nullptr         , nullptr         , "comh_ani"       },   // 83
        { nullptr         , nullptr         , "chmob_ani"      },   // 84
        { nullptr         , nullptr         , "chmil_ani"      },   // 85
        { nullptr         , nullptr         , "chen_ani"       },   // 86
        { nullptr         , nullptr         , "chre_ani"       },   // 87
        { nullptr         , nullptr         , "chdef_ani"      },   // 88
        { nullptr         , nullptr         , "chpro_ani"      },   // 89
        { nullptr         , nullptr         , "chbio_ani"      },   // 90
        { nullptr         , nullptr         , "ars_ani"        },   // 91
        { nullptr         , nullptr         , nullptr          },   // 92
        { nullptr         , nullptr         , "bson_ani"       },   // 93
        { nullptr         , nullptr         , "corisi_ani"     },   // 94
        { nullptr         , nullptr         , nullptr          },   // 95
        { nullptr         , nullptr         , nullptr          },   // 96
        { nullptr         , nullptr         , nullptr          },   // 97
        { nullptr         , nullptr         , nullptr          },   // 98
        { nullptr         , nullptr         , nullptr          },   // 99
        { nullptr         , nullptr         , "siext_ani"      },   // 100
        { nullptr         , nullptr         , nullptr          },   // 101
        { nullptr         , nullptr         , nullptr          },   // 102
        { nullptr         , nullptr         , nullptr          },   // 103
        { nullptr         , nullptr         , nullptr          },   // 104
        { nullptr         , nullptr         , nullptr          },   // 105
        { nullptr         , nullptr         , nullptr          },   // 106
        { nullptr         , nullptr         , nullptr          },   // 107
        { nullptr         , nullptr         , nullptr          },   // 108
        { nullptr         , nullptr         , "ifgen_ani"      },   // 109
        { nullptr         , nullptr         , nullptr          },   // 110
        { "atelews_ani"   , nullptr         , nullptr          },   // 111
        { nullptr         , nullptr         , "glsat_ani"      },   // 112
        { nullptr         , nullptr         , nullptr          },   // 113
        { nullptr         , nullptr         , nullptr          },   // 114
        { nullptr         , nullptr         , "qpara_ani"      },   // 115
    };
    int g = tobj - 50;
    if (g < 0 || g >= ANI_GROUPS || side < 0 || side > 2) return nullptr;
    return kA[g][side];
}

// Czy animacja tego budynku chodzi bez przerwy (0x00790C2C).
inline bool buildingAniLoops(int tobj)
{
    static const unsigned char kL[ANI_GROUPS] = {
        0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    int g = tobj - 50;
    return g >= 0 && g < ANI_GROUPS && kL[g] != 0;
}

// Czy budynek ma osobny pasek `_cover` (0x00791A10).
inline bool buildingHasCover(int tobj)
{
    static const unsigned char kC[ANI_GROUPS] = {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0
    };
    int g = tobj - 50;
    return g >= 0 && g < ANI_GROUPS && kC[g] != 0;
}

inline void buildingAniRange(int type,int side,int &first,int &last) {
    static const int ranges[ANI_GROUPS][3][2] = {
        {{0,0},{0,0},{0,0}},
        {{0,9},{10,19},{0,0}},
        {{0,9},{0,9},{0,9}},
        {{0,4},{0,9},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,14},{0,14},{0,0}},
        {{0,19},{0,19},{0,0}},
        {{0,19},{0,19},{0,0}},
        {{0,3},{0,9},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,17},{0,9},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,4},{0,4},{0,4}},
        {{0,17},{0,17},{0,17}},
        {{0,14},{0,14},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,4},{0,4},{0,4}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{40,40},{40,40},{40,40}},
        {{0,0},{0,0},{0,0}},
        {{0,10},{0,10},{0,10}},
        {{0,19},{0,0},{0,0}},
        {{0,9},{0,14},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,19}},
        {{0,0},{0,0},{0,15}},
        {{0,0},{0,0},{0,15}},
        {{0,0},{0,0},{0,15}},
        {{0,0},{0,0},{0,11}},
        {{0,0},{0,0},{0,15}},
        {{0,0},{0,0},{0,17}},
        {{0,0},{0,0},{0,19}},
        {{0,0},{0,0},{0,23}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,19}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,16}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,19}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,14}},
        {{0,0},{0,0},{0,0}},
        {{0,9},{0,0},{0,0}},
        {{0,0},{0,0},{0,6}},
        {{0,0},{0,0},{0,0}},
        {{0,0},{0,0},{0,13}},
        {{0,0},{0,0},{0,14}},
    };
    first=last=0;
    if(type>=50 && type<=115 && side>=0 && side<3) {
        first=ranges[type-50][side][0]; last=ranges[type-50][side][1];
    }
}
}   // namespace units
