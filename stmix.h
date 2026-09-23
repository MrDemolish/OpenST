// Mikser dzwiekow: wiele glosow naraz, na jednym urzadzeniu waveOut.
//
// **Dlaczego w ogole.** `PlaySoundA` ma jeden kanal - kazdy nowy dzwiek
// ucinal poprzedni. W grze dzwieki nakladaja sie: kwestia jednostki gra
// razem z iskrami budowy, klikniecie nie przerywa narratora.
//
// Urzadzenie jest jedno i stale: **22050 Hz, 16 bitow, mono** - taki format
// ma wiekszosc nagran w `SOUNDS.DKX`. Nagrania 44 kHz i stereo (kwestie
// intercomu z `TASKS`) sprowadza sie do niego przy dekodowaniu, raz.
//
// Bufory karmi `pump()`, wolane raz na klatke z petli gry. Callback
// `waveOutProc` odpada, bo z jego wnetrza nie wolno wolac funkcji waveOut,
// a osobny watek byl niepotrzebny: cztery bufory po 93 ms daja prawie
// czterysta milisekund zapasu, a klatka trwa kilkanascie.
#pragma once
#include <windows.h>
#include <mmsystem.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace snd {

// Zdekodowane nagranie: 16 bitow, mono, 22050 Hz.
struct Clip {
    std::vector<int16_t> pcm;
    bool ok() const { return !pcm.empty(); }
};

class Mixer {
public:
    static const int RATE = 22050;
    static const int BUFS = 4;
    static const int FRAMES = 2048;        // 93 ms na bufor
    static const int VOICES = 12;

    bool open()
    {
        if (dev) return true;
        WAVEFORMATEX wf{};
        wf.wFormatTag = WAVE_FORMAT_PCM;
        wf.nChannels = 1;
        wf.nSamplesPerSec = RATE;
        wf.wBitsPerSample = 16;
        wf.nBlockAlign = 2;
        wf.nAvgBytesPerSec = RATE * 2;
        if (waveOutOpen(&dev, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL)
            != MMSYSERR_NOERROR) {
            dev = nullptr;
            return false;
        }
        for (int i = 0; i < BUFS; ++i) {
            buf[i].assign(FRAMES, 0);
            std::memset(&hdr[i], 0, sizeof(hdr[i]));
            hdr[i].lpData = reinterpret_cast<LPSTR>(buf[i].data());
            hdr[i].dwBufferLength = DWORD(FRAMES * 2);
            if (waveOutPrepareHeader(dev, &hdr[i], sizeof(hdr[i]))
                != MMSYSERR_NOERROR) {
                close();
                return false;
            }
            hdr[i].dwFlags |= WHDR_DONE;   // wolny, `pump` go wypelni
        }
        return true;
    }

    void close()
    {
        silence(); stopAmbient();
        if (!dev) return;
        waveOutReset(dev);
        for (int i = 0; i < BUFS; ++i)
            waveOutUnprepareHeader(dev, &hdr[i], sizeof(hdr[i]));
        waveOutClose(dev);
        dev = nullptr;
    }

    ~Mixer() { close(); }

    bool ok() const { return dev != nullptr; }

    // Nowy glos. Gdy wszystkie zajete, ustepuje ten, ktory jest najdalej -
    // najstarszy dzwiek traci najmniej.
    bool start(const std::shared_ptr<const Clip> &c, float gain = 1.0f)
    {
        return dev && startTracked(c,gain)!=0;
    }

    using Handle = uint64_t;
    // A handle identifies a playback instance, not a reusable voice slot.
    // This also supports offline PCM rendering without a waveOut device.
    Handle startTracked(const std::shared_ptr<const Clip> &c, float gain=1.0f, bool loop=false)
    {
        if (!c || !c->ok()) return 0;
        int slot = -1;
        for (int i = 0; i < VOICES; ++i)
            if (!v[i].clip) { slot = i; break; }
        if (slot < 0) {
            size_t best = 0;
            for (int i = 0; i < VOICES; ++i) {
                size_t left = v[i].clip->pcm.size() - v[i].pos;
                if (slot < 0 || left < best) { slot = i; best = left; }
            }
        }
        v[slot].clip = c;
        v[slot].pos = 0;
        v[slot].gain = gain;
        v[slot].loop = loop;
        if(++serial==0) ++serial;
        v[slot].handle=serial;
        ++started;
        return serial;
    }

    bool active(Handle handle) const {
        if(!handle) return false;
        for(const auto &voice:v) if(voice.clip && voice.handle==handle) return true;
        return false;
    }
    void stop(Handle handle) {
        if(!handle) return;
        for(auto &voice:v) if(voice.handle==handle) voice.clip.reset();
    }

    // **Petla tla ma wlasny glos**, poza dwunastoma. Gdyby szla przez
    // `start()`, zajmowalaby jedno z gniazd na stale, a przy dwunastu
    // dzwiekach naraz `start()` odebralby jej je jako "najdalszy" glos -
    // tlo ucichaloby dokladnie wtedy, gdy dzieje sie najwiecej.
    bool startAmbient(const std::shared_ptr<const Clip> &c, float gain = 1.0f)
    {
        if (!dev || !c || !c->ok()) return false;
        amb.clip = c;
        amb.pos = 0;
        amb.gain = gain;
        return true;
    }

    void stopAmbient() { amb.clip.reset(); }
    bool ambientOn() const { return amb.clip != nullptr; }

    // Ile glosow gra w tej chwili.
    int voices() const
    {
        int n = 0;
        for (int i = 0; i < VOICES; ++i) if (v[i].clip) ++n;
        return n;
    }

    long long playedCount() const { return started; }

    void silence()
    {
        for (int i = 0; i < VOICES; ++i) v[i].clip.reset();
    }

    // Wypelnia bufory, ktore urzadzenie juz oddalo. Wolac raz na klatke.
    void pump()
    {
        if (!dev) return;
        for (int i = 0; i < BUFS; ++i) {
            if (!(hdr[i].dwFlags & WHDR_DONE)) continue;
            if (hdr[i].dwFlags & WHDR_INQUEUE) continue;
            render(buf[i]);
            hdr[i].dwFlags &= ~WHDR_DONE;
            hdr[i].dwBufferLength = DWORD(FRAMES * 2);
            if (waveOutWrite(dev, &hdr[i], sizeof(hdr[i])) != MMSYSERR_NOERROR)
                hdr[i].dwFlags |= WHDR_DONE;
        }
    }

private:
    struct Voice {
        std::shared_ptr<const Clip> clip;
        size_t pos = 0;
        float gain = 1.0f;
        Handle handle = 0;
        bool loop = false;
    };

public:
    // Suma glosow z obcinaniem. Bez obcinania glosne nagrania trzaskaja,
    // gdy zejda sie trzy naraz.
    void render(std::vector<int16_t> &out)
    {
        std::vector<int> acc(out.size(), 0);
        if (amb.clip) {                 // tlo: w kolko, bez konca
            const std::vector<int16_t> &p = amb.clip->pcm;
            const int g = int(amb.gain * 256.0f);
            for (size_t k = 0; k < out.size(); ++k) {
                if (amb.pos >= p.size()) amb.pos = 0;
                acc[size_t(k)] += (int(p[amb.pos++]) * g) >> 8;
            }
        }
        for (int i = 0; i < VOICES; ++i) {
            if (!v[i].clip) continue;
            const std::vector<int16_t> &p = v[i].clip->pcm;
            const int g = int(v[i].gain * 256.0f);
            for(size_t k=0;k<out.size();++k) {
                acc[k]+=(int(p[v[i].pos++])*g)>>8;
                if(v[i].pos>=p.size()) {
                    if(v[i].loop) v[i].pos=0;
                    else { v[i].clip.reset(); break; }
                }
            }
        }
        for (size_t k = 0; k < out.size(); ++k) {
            int s = acc[size_t(k)];
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            out[size_t(k)] = int16_t(s);
        }
    }

private:
    HWAVEOUT dev = nullptr;
    WAVEHDR hdr[BUFS]{};
    std::vector<int16_t> buf[BUFS];
    Voice v[VOICES];
    Voice amb;                          // petla tla, poza pula
    long long started = 0;
    Handle serial = 0;
};

}  // namespace snd
