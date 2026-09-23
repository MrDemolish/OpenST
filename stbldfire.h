// Generated from state 5 (004c96e0), trigger 00791800 and flag 007915f0.
#pragma once
namespace bldfire {
struct Def { const char *body; int emit; };
inline Def get(int type, int side) {
    if(side<0 || side>2) return {nullptr,0};
    switch(type) {
    case 62: {
        static const char *body[] = {"hfc_body","hfc_body",nullptr};
        return {body[side],2};
    }
    case 70: {
        static const char *body[] = {"lla_body","lla_body",nullptr};
        return {body[side],2};
    }
    case 71: {
        static const char *body[] = {"cas_body","cas_body",nullptr};
        return {body[side],2};
    }
    case 74: {
        static const char *body[] = {"hla_body","hla_body",nullptr};
        return {body[side],2};
    }
    case 75: {
        static const char *body[] = {"emc_body","emc_body",nullptr};
        return {body[side],2};
    }
    default: return {nullptr,0};
    }
}
}
