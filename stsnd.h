// The menu's sound effects, out of the game's own SOUND\SOUNDS archive.
//
// Two record types matter:
//
//   type 23  SND_<id>  a descriptor: 25 bytes of header, then the NUL
//                      terminated name of the recording to play
//   type 2             the recording: a WAVEFORMATEX with a small tail and
//                      then the samples - a WAV with the RIFF wrapper removed
//
//     +0   wFormatTag, nChannels, nSamplesPerSec, nAvgBytesPerSec,
//          nBlockAlign, wBitsPerSample, cbSize      (18 bytes)
//     +18  u32  length of the sample data
//
// The header length is not guessed at: it is the record size minus that
// length, which is how st_audio in the asset studio reads the same records.
//
// The menu asks for ids 1 and 2 (MainMenuTy::SetMode and ::CloseButtons), both
// of which resolve to mmbt_001 - the same movement sound going either way.
#pragma once
#include <windows.h>
#include <mmsystem.h>

#include <string>
#include <vector>
#include <map>

#include <memory>

#include "stark.h"
#include "stmix.h"

namespace snd {

class Bank {
public:
    bool open(const std::string &gameDir)
    {
        // `DATA\TASKS` trzyma **glos intercomu**: `DEFAULT_BIP` (0,1 s
        // sygnal) oraz `DEFAULT_BO` i `DEFAULT_SI` - 6,8 s i 13,6 s mowy
        // 44 kHz stereo. To jest narrator gry; w kampanii odpalaja go akcje
        // skryptu misji `ActPlayBriefing` i `ActPlaySound`, a nie zdarzenia
        // rozgrywki.
        task.open(gameDir + "\\DATA\\TASKS.DKX", gameDir + "\\DATA\\TASKS.DKD");
        return ar.open(gameDir + "\\SOUND\\SOUNDS.DKX", gameDir + "\\SOUND\\SOUNDS.DKD");
    }

    bool ok() const { return ar.count() > 0; }

    // ------------------------------ mikser -------------------------------
    //
    // `PlaySoundA` ma jeden kanal, wiec kazdy dzwiek ucinal poprzedni.
    // Wszystko idzie teraz przez `snd::Mixer` - jedno urzadzenie waveOut
    // 22050/16/mono i dwanascie glosow naraz.
    Mixer mix;

    bool openMixer() { return mix.open(); }
    void pump() { mix.pump(); }

    // Nagranie z archiwum jako PCM 22050 mono. Rekordy sa 16 bitowe, ale
    // kwestie intercomu maja 44 kHz i dwa kanaly - sprowadzamy je raz, przy
    // dekodowaniu, zeby mikser mial jeden format.
    std::shared_ptr<const Clip> clip(const ark::Archive &src,
                                     const std::string &name) const
    {
        auto it = clips.find(name);
        if (it != clips.end()) return it->second;
        auto c = std::make_shared<Clip>();
        std::vector<uint8_t> r = src.read(name);
        if (r.size() >= 26) {
            uint16_t tag = ark::rd16(&r[0]), ch = ark::rd16(&r[2]);
            uint32_t rate = ark::rd32(&r[4]);
            uint16_t bits = ark::rd16(&r[14]);
            uint32_t len = ark::rd32(&r[18]);
            if (tag == 1 && ch >= 1 && ch <= 2 && len > 0 && len <= r.size()
                && (bits == 8 || bits == 16)) {
                size_t start = r.size() - len;
                int step = rate >= 2 * Mixer::RATE ? 2 : 1;   // 44100 -> 22050
                size_t bytes = size_t(bits / 8) * ch;
                size_t frames = size_t(len) / (bytes ? bytes : 1);
                c->pcm.reserve(frames / size_t(step) + 1);
                for (size_t f = 0; f < frames; f += size_t(step)) {
                    int sum = 0;
                    for (int k = 0; k < ch; ++k) {
                        size_t o = start + f * bytes + size_t(k) * (bits / 8);
                        if (o + size_t(bits / 8) > r.size()) break;
                        sum += bits == 16
                             ? int(int16_t(uint16_t(r[o]) | (uint16_t(r[o + 1]) << 8)))
                             : (int(r[o]) - 128) * 256;
                    }
                    c->pcm.push_back(int16_t(sum / ch));
                }
            }
        }
        clips.emplace(name, c);
        return c;
    }

    // Nagranie po nazwie z archiwum zadan - narrator i sygnal intercomu.
    bool playTask(const std::string &name)
    {
        auto it = named.find(name);
        if (it == named.end()) {
            std::vector<uint8_t> buf;
            if (!wrapFrom(task, name, buf)) buf.clear();
            it = named.emplace(name, std::move(buf)).first;
        }
        (void)it;
        return mix.start(clip(task, name), 1.0f);
    }

    // **Odprawa misji jest w samej mapie.** Rekord `TaskSpeach` (typ 2)
    // ma 30 z 33 map misji; potyczki go nie maja i dostaja zastepcza
    // kwestie rasy z DATA\TASKS.
    bool openMap(const std::string &dkx, const std::string &dkd)
    {
        mapNamed.clear();
        return mapAr.open(dkx, dkd, true);
    }

    bool hasMapSpeech() const
    {
        std::vector<uint8_t> buf;
        return wrapFrom(mapAr, "TaskSpeach", buf);
    }

    bool playMapSpeech(float gain = 1.0f)
    {
        if (!hasMapSpeech()) return false;
        return mix.start(clip(mapAr, "TaskSpeach"), gain);
    }

    // Ile ta kwestia trwa - ekran odprawy musi wiedziec, jak dlugo trzymac
    // glowe w stanie SPEAK. Nagranie jest zdekodowane do formatu miksera
    // (22050 Hz, mono), wiec sekundy to po prostu probki przez RATE.
    float mapSpeechSecs()
    {
        if (!hasMapSpeech()) return 0.0f;
        auto c = clip(mapAr, "TaskSpeach");
        if (!c || !c->ok()) return 0.0f;
        return float(c->pcm.size()) / 22050.0f;   // format miksera
    }

    void stopMapSpeech() { mix.silence(); }

    // Samo istnienie rekordu nie wystarczy: `DEFAULT_WS` jest w archiwum,
    // ale jako **obrazek**, nie nagranie - dzwieku dla Bialych Rekinow gra
    // nie ma. Sprawdzamy wiec, czy da sie z niego zrobic WAV.
    bool hasTask(const std::string &name) const
    {
        std::vector<uint8_t> buf;
        return wrapFrom(task, name, buf);
    }

    // SND_<id> is not one recording but a **set** of them:
    //
    //   +8   u32  how many recordings
    //   +24  presence mask, **ceil(count / 8) bytes**
    //   +..  the names, each NUL terminated
    //
    // The mask cannot be skipped over blindly. A set of seventeen has a three
    // byte mask and the names start at +27; reading them from +25 glued a
    // stray 0xFF onto the first name, which the printable filter below then
    // threw away - so the longest sets lost a line and the count was wrong.
    std::vector<std::string> resolveAll(int id) const
    {
        char key[24];
        std::snprintf(key, sizeof(key), "SND_%d", id);
        std::vector<uint8_t> d = ar.read(key);
        std::vector<std::string> out;
        if (d.size() <= 25) return out;
        uint32_t cnt = uint32_t(d[8]) | (uint32_t(d[9]) << 8)
                     | (uint32_t(d[10]) << 16) | (uint32_t(d[11]) << 24);
        if (cnt == 0 || cnt > 64) cnt = 1;
        size_t i = 24 + (cnt + 7) / 8;
        while (i < d.size()) {
            std::string name;
            while (i < d.size() && d[i]) name += char(d[i++]);
            ++i;
            if (name.empty()) break;
            bool clean = true;
            for (char c : name) if (c < 32 || c > 126) clean = false;
            if (clean) out.push_back(name);
        }
        return out;
    }

    std::string resolve(int id) const
    {
        std::vector<std::string> v = resolveAll(id);
        return v.empty() ? std::string() : v[0];
    }

    // Play by sound id, asynchronously.
    //
    // The wrapped WAV is cached. Building it means two archive reads and two
    // LZSS decompressions - the descriptor and then sixty kilobytes of samples
    // - and doing that inside the click handler was enough to stutter the
    // frame every time a unit was selected.
    bool play(int id, float gain = 1.0f)
    {
        std::vector<std::string> names = resolveAll(id);
        if (names.empty()) return false;
        // Which line of the set, so the same unit does not repeat itself.
        pick = pick * 1103515245u + 12345u;
        return mix.start(clip(ar, names[(pick >> 16) % names.size()]), gain);
    }

    Mixer::Handle playTracked(int id, float gain=1.0f, bool loop=false)
    {
        if(!mix.ok()) return 0;
        auto names=resolveAll(id);
        if(names.empty()) return 0;
        pick=pick*1103515245u+12345u;
        return mix.startTracked(clip(ar,names[(pick>>16)%names.size()]),gain,loop);
    }
    void stopTracked(Mixer::Handle handle) { mix.stop(handle); }

    // Warm the cache outside the click path, so the first click is quick too.
    void preload(int id)
    {
        for (const std::string &n : resolveAll(id)) clip(ar, n);
    }

    void stop() { mix.silence(); }

    // Petla otoczenia. `STAppC::StartGame` wola `SoundMngr(.., 1, .., 0x4b7, ..)`
    // - rodzaj **1**, numer **1207**, nagranie `surn_001`. To jedyny dzwiek
    // grany rodzajem 1 i jedyny, ktory ma chodzic bez konca.
    bool ambient(int id, float gain = 1.0f)
    {
        std::vector<std::string> names = resolveAll(id);
        if (names.empty()) return false;
        return mix.startAmbient(clip(ar, names[0]), gain);
    }

    void stopAmbient() { mix.stopAmbient(); }
    bool ambientOn() const { return mix.ambientOn(); }

    // A record as a playable RIFF WAV, stored format untouched.
    bool wrap(const std::string &name, std::vector<uint8_t> &out) const
    { return wrapFrom(ar, name, out); }

    bool wrapFrom(const ark::Archive &src, const std::string &name,
                  std::vector<uint8_t> &out) const
    {
        std::vector<uint8_t> r = src.read(name);
        if (r.size() < 26) return false;
        uint16_t tag = ark::rd16(&r[0]), ch = ark::rd16(&r[2]);
        uint32_t rate = ark::rd32(&r[4]), avg = ark::rd32(&r[8]);
        uint16_t align = ark::rd16(&r[12]), bits = ark::rd16(&r[14]);
        uint16_t cb = ark::rd16(&r[16]);
        uint32_t len = ark::rd32(&r[18]);
        if (len == 0 || len > r.size()) return false;
        size_t start = r.size() - len;

        std::vector<uint8_t> fmt;
        auto put16 = [&](std::vector<uint8_t> &v, uint16_t x) {
            v.push_back(uint8_t(x)); v.push_back(uint8_t(x >> 8));
        };
        auto put32 = [&](std::vector<uint8_t> &v, uint32_t x) {
            for (int i = 0; i < 4; ++i) v.push_back(uint8_t(x >> (8 * i)));
        };
        put16(fmt, tag); put16(fmt, ch); put32(fmt, rate);
        put32(fmt, avg); put16(fmt, align); put16(fmt, bits);
        if (cb) {
            put16(fmt, cb);
            for (uint16_t i = 0; i < cb && 30u + i < r.size(); ++i) fmt.push_back(r[30 + i]);
        } else if (tag != 1) {
            put16(fmt, 0);
        }

        std::vector<uint8_t> body;
        const char *fourcc = "fmt ";
        body.insert(body.end(), fourcc, fourcc + 4);
        put32(body, uint32_t(fmt.size()));
        body.insert(body.end(), fmt.begin(), fmt.end());
        if (fmt.size() & 1) body.push_back(0);
        const char *dc = "data";
        body.insert(body.end(), dc, dc + 4);
        put32(body, len);
        body.insert(body.end(), r.begin() + long(start), r.begin() + long(start + len));
        if (len & 1) body.push_back(0);

        out.clear();
        const char *riff = "RIFF";
        out.insert(out.end(), riff, riff + 4);
        put32(out, uint32_t(4 + body.size()));
        const char *wave = "WAVE";
        out.insert(out.end(), wave, wave + 4);
        out.insert(out.end(), body.begin(), body.end());
        return true;
    }

private:
    ark::Archive ar;
    mutable std::map<std::string, std::shared_ptr<const Clip>> clips;
    ark::Archive task;                    // DATA\TASKS - glos intercomu
    ark::Archive mapAr;                   // wczytana mapa - odprawa misji
    std::unordered_map<std::string, std::vector<uint8_t>> mapNamed;
    mutable std::map<std::string, std::vector<uint8_t>> named;
    std::vector<uint8_t> held;
    std::map<int, std::vector<std::vector<uint8_t>>> ready;
    mutable uint32_t pick = 12345;
};

}  // namespace snd
