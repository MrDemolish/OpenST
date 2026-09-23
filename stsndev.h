// Wygenerowane przez tools/gen_sounds.py - nie edytowac.
//
// Numery dzwiekow, kazdy odczytany z exe: interfejs gra je
// przez `FUN_005252C0(id)` (wszystkie wywolania ida przez
// zaslepke skoku, nie wprost), a obiekty przez
// `vtbl+0x90(rodzaj, id)` - rodzaj 3 to wlasny dzwiek
// obiektu, 4 pozycyjny, 5 interfejsowy, 6 kwestia lektora.
//
// Zaznaczenie, rozkaz i alarm lodzi licza sie osobno,
// z tablicy w stunit.h.
#pragma once

namespace sndev {

// klikniecie przycisku
// kazdy *PanelTy::GetMessage
const int SFX_CLICK        =  174;   // mmbt_003

// otwarcie okna
// OptPanelTy/PausePanelTy::SwitchPanel
const int SFX_PANEL        =  175;   // icpa_001

// intercom
// IntercomPanelTy::SwitchIntercomPanel
const int SFX_INTERCOM     =  176;   // icpa_001

// przelaczenie podgladu
// CPanelTy::PaintTV
const int SFX_TV           =  177;   // icpa_002

// podglad - drugi dzwiek
// CPanelTy::PaintTV
const int SFX_TV2          =  178;   // icpa_002

// odprawa
// CPanelTy::PlayBrief, CPanelTy::PaintTV
const int SFX_BRIEF        =   30;   // mhbt_001

// zaladunek surowca
// LoadRC  vtbl+0x90(3,0xfb)
const int SFX_LOAD         =  251;   // load_001

// rozladunek surowca
// LoadRC/UnLoadRC  vtbl+0x90(3,0x15f)
const int SFX_UNLOAD       =  351;   // load_001

// zaladunek - drugi dzwiek
// LoadRC  vtbl+0x90(3,0x160)
const int SFX_LOAD2        =  352;   // load_001

// postawienie miny
// SetMine vtbl+0x90(3,0xe6)
const int SFX_MINE         =  230;   // min_001

// postawienie miny (druga rasa)
// SetMine vtbl+0x90(3,0x14a)
const int SFX_MINE2        =  330;   // min_001

// przejmowanie budynku
// Capture vtbl+0x90(3,0xed)
const int SFX_CAPTURE      =  237;   // BRAK

// przejmowanie (druga rasa)
// Capture vtbl+0x90(3,0x151)
const int SFX_CAPTURE2     =  337;   // BRAK

// budynek przejety
// Capture vtbl+0x90(3,0x1d2)
const int SFX_CAPTURED     =  466;   // BRAK

// uzupelnienie energii
// Recharge vtbl+0x90(3,0x1b2)
const int SFX_RECHARGE     =  434;   // char_001

// wrota doku - otwarcie
// FUN_004CEB00 vtbl+0x90(3,0x1f9)
const int SFX_DOCK_OPEN    =  505;   // lado_001

// wrota doku - otwarcie (2)
// FUN_004CEB00 vtbl+0x90(3,0x2c1)
const int SFX_DOCK_OPEN2   =  705;   // lado_001

// wrota doku - zamkniecie
// FUN_004E16D0 vtbl+0x90(3,0x3a9)
const int SFX_DOCK_SHUT    =  937;   // laco_001

// wrota doku - zamkniecie (2)
// FUN_004E16D0 vtbl+0x90(3,0x3b8)
const int SFX_DOCK_SHUT2   =  952;   // laco_001

// wyczerpane zloze korium (WS)
// FUN_004E1930 tobj 57/94, rasa 1
const int SFX_DRY_COR_WS   =  542;   // BRAK

// wyczerpane zloze korium (BO)
// FUN_004E1930 tobj 57/94, rasa 2
const int SFX_DRY_COR_BO   =  740;   // bor_088 bor_089

// wyczerpane zloze korium (SI)
// FUN_004E1930 tobj 57/94, rasa 3
const int SFX_DRY_COR_SI   =  933;   // BRAK

// wyczerpane zloze metalu (WS)
// FUN_004E1930 tobj 79, rasa 1
const int SFX_DRY_MET_WS   =  636;   // bos_087 bos_088

// wyczerpane zloze metalu (BO)
// FUN_004E1930 tobj 79, rasa 2
const int SFX_DRY_MET_BO   =  843;   // BRAK

// postawienie budynku / praca rusztowania
// TLOEmbryoTy::Create vtbl+0x90(3,0x360)
const int SFX_BUILD_START  =  864;   // embr_001

// rusztowanie zaczyna sie zwijac (postep > 99)
// TLOEmbryoTy__Step faza 3 vtbl+0x90(3,0x361)
const int SFX_EMBRYO_FOLD  =  865;   // embr_001

// zamiana sekwencji
// TLOEmbryoTy__Step vtbl+0x90(3,0x362)
const int SFX_EMBRYO2      =  866;   // embr_002

// koniec budowy
// TLOEmbryoTy__Step vtbl+0x90(3,0x363)
const int SFX_EMBRYO3      =  867;   // embr_003 embr_004

// petla otoczenia (opcja OTOCZENIE, napis 16126)
// STAppC::StartGame SoundMngr(..,1,..,0x4b7,..)
const int SFX_AMBIENT      = 1207;   // surn_001

// -------------------------- kwestie lektora ----------------------
//
// `baza + rasa`, rasa 0 WS, 1 BO, 2 SI. Opis przy kazdej to
// **to, co slychac na nagraniu** - transkrypcja z
// sound/transcripts.tsv, nie domysl z numeru.
const int SAY_WIN          =   50;   // Wypelniles misje
const int SAY_LOSE         =   53;   // Nie wypelniles misji
const int SAY_MESSAGE      =   56;   // Nowa wiadomosc
const int SAY_ATTACKED     =   59;   // Atakuja nas
const int SAY_NO_AMMO      =   62;   // Za malo amunicji
const int SAY_NO_CORIUM    =   65;   // Za malo korium
const int SAY_NO_GOLD      =   68;   // Za malo zlota
const int SAY_NO_METAL     =   71;   // Za malo metalu
const int SAY_NO_AIR       =   74;   // Za malo powietrza
const int SAY_NO_ENERGY    =   77;   // Za malo energii
const int SAY_NO_SILICON   =   80;   // Za malo silikonu
const int SAY_UNIT_LOST    =   83;   // Straciles jednostke
const int SAY_PARALYSED    =   86;   // Jednostka sparalizowana
const int SAY_RESEARCH     =   89;   // Ukonczono badanie
const int SAY_NEW_BOAT     =   92;   // Mozliwosc produkcji nowej klasy lodzi podwodnych
const int SAY_NEW_BLD      =   95;   // Mozliwosc produkcji nowej struktury
const int SAY_BUILT        =   98;   // Budowa ukonczona
const int SAY_BOAT_DONE    =  101;   // Ukonczono produkcje lodzi podwodnej
const int SAY_DISMANTLED   =  104;   // Budynek rozmontowany
const int SAY_CONTAINER    =  107;   // Container gotowy
const int SAY_CONVERTED    =  110;   // Ukonczono konwersje surowca
const int SAY_HACKED       =  113;   // Wlamanie do sieci udane
const int SAY_DB_OPEN      =  116;   // Dostepna wroga baza danych
const int SAY_DB_SHUT      =  119;   // Skonczyl sie dostep do bazy danych wroga
const int SAY_CAPTURED     =  122;   // Przejales obiekt wroga
const int SAY_CAPTURE_NO   =  125;   // Przejecie nieudane
const int SAY_TELEPORT     =  128;   // Wykryto teleportacje
const int SAY_TELEPORT_NO  =  131;   // Teleportacja nieudana
const int SAY_MINE_SEEN    =  134;   // Dostrzezono mine glebinowa
const int SAY_SHIELD       =  146;   // Wykryto oslone energetyczna
const int SAY_NUKE         =  152;   // Wrog wystrzelil pociski nuklearne
const int SAY_VACUUM       =  155;   // Wrog wystrzelil bombe prozniowa
const int SAY_LASERBOMB    =  158;   // Wrog wystrzelil bombe laserowa

inline int say(int base, int side)
{
    return base + (side < 0 || side > 2 ? 0 : side);
}

// ------------------------- tablice budynkow ----------------------
//
// Indeksem jest **grupa budynku, czyli TOBJ - 50**. Wynika to
// z samych danych: niezerowe wpisy tablicy postoju wypadaja
// dokladnie na wiezyczkach (TOBJ 62, 63, 70, 71, 74, 75).
const int GROUPS = 34;

// TLOBaseTy::fireProc, rodzaj 3: wiezyczka stojac wybiera nowy
// kierunek co 25..100 tikow i wtedy to gra.
inline int bldIdle(int tobj)
{
    static const int k[GROUPS] = {
           0,    0,    0,    0,    0,    0,    0,    0,
           0,    0,    0,    0,  581,  590,    0,    0,
           0,    0,    0,    0,  779,  788,    0,    0,
         810,  819,    0,    0,    0,    0,    0,  656,
           0,    0
    };
    const int g = tobj - 50;
    return g >= 0 && g < GROUPS ? k[g] : 0;
}

// TLOBaseTy::SetActivity, rodzaj 4: budynek staje sie aktywny.
// Rasa 1 WS, 2 BO; Silikony nie maja w tej tablicy nic.
inline int bldActive(int tobj, int side)
{
    static const int k[GROUPS][2] = {
        {  501,  701 },
        {  508,  708 },
        {  514,  714 },
        {  519,  719 },
        {  527,  727 },
        {  535,  733 },
        {    0,    0 },
        {  541,  739 },
        {  550,  748 },
        {  555,  753 },
        {  563,  761 },
        {  568,  766 },
        {  574,    0 },
        {  583,    0 },
        {  592,    0 },
        {  599,    0 },
        {  607,    0 },
        {  615,    0 },
        {  622,    0 },
        {  629,    0 },
        {    0,  772 },
        {    0,  781 },
        {    0,  791 },
        {    0,  796 },
        {    0,  803 },
        {    0,  812 },
        {    0,  821 },
        {    0,  827 },
        {    0,  835 },
        {  635,  841 },
        {  644,  850 },
        {  649,    0 },
        {  658,  855 },
        {    0,    0 },
    };
    const int g = tobj - 50;
    if (g < 0 || g >= GROUPS || side < 0 || side > 1) return 0;
    return k[g][side];
}

// Wyczerpane zloze: `FUN_004E1930` gra to przy porcji, ktora
// zdjela ze zloza reszte - warunek w exe to
// `ilosc == 0 && zdjeto != 0`. Na nagraniu slychac
// "Wyczerpano zloze metalu" - i to stad nazwa, a nie
// z numeru. Archiwum ma nagranie tylko dla 636 i 740,
// reszta jest cicha takze w oryginale.
inline int drySfx(int tobj, int side)
{
    if (tobj == 57 || tobj == 94) {
        static const int k[3] = { SFX_DRY_COR_WS, SFX_DRY_COR_BO,
                                  SFX_DRY_COR_SI };
        return k[side < 0 || side > 2 ? 0 : side];
    }
    if (tobj == 79) {
        static const int k[2] = { SFX_DRY_MET_WS, SFX_DRY_MET_BO };
        return side == 1 ? k[1] : k[0];
    }
    return 0;
}

struct Ev { int id; const char *name; const char *rec; };

inline const Ev *table(int &n)
{
    static const Ev k[] = {
        { 174, "SFX_CLICK", "mmbt_003" },
        { 175, "SFX_PANEL", "icpa_001" },
        { 176, "SFX_INTERCOM", "icpa_001" },
        { 177, "SFX_TV", "icpa_002" },
        { 178, "SFX_TV2", "icpa_002" },
        { 30, "SFX_BRIEF", "mhbt_001" },
        { 251, "SFX_LOAD", "load_001" },
        { 351, "SFX_UNLOAD", "load_001" },
        { 352, "SFX_LOAD2", "load_001" },
        { 230, "SFX_MINE", "min_001" },
        { 330, "SFX_MINE2", "min_001" },
        { 237, "SFX_CAPTURE", "" },
        { 337, "SFX_CAPTURE2", "" },
        { 466, "SFX_CAPTURED", "" },
        { 434, "SFX_RECHARGE", "char_001" },
        { 505, "SFX_DOCK_OPEN", "lado_001" },
        { 705, "SFX_DOCK_OPEN2", "lado_001" },
        { 937, "SFX_DOCK_SHUT", "laco_001" },
        { 952, "SFX_DOCK_SHUT2", "laco_001" },
        { 542, "SFX_DRY_COR_WS", "" },
        { 740, "SFX_DRY_COR_BO", "bor_088" },
        { 933, "SFX_DRY_COR_SI", "" },
        { 636, "SFX_DRY_MET_WS", "bos_087" },
        { 843, "SFX_DRY_MET_BO", "" },
        { 864, "SFX_BUILD_START", "embr_001" },
        { 865, "SFX_EMBRYO_FOLD", "embr_001" },
        { 866, "SFX_EMBRYO2", "embr_002" },
        { 867, "SFX_EMBRYO3", "embr_003" },
        { 1207, "SFX_AMBIENT", "surn_001" },
    };
    n = int(sizeof(k) / sizeof(k[0]));
    return k;
}

}  // namespace sndev
