// Music - MUSIC\MUSIC.
//
// The records look like the sound bank's (a WAVEFORMATEX at +0, the sample
// length at +18, the samples at the end), but the format tag is 2, not 1:
// MS ADPCM, stereo, 44100 Hz, 4 bits a sample, 2048 byte blocks. Sound effects
// are plain 16 bit mono at 22050.
//
// Two reasons this decodes the ADPCM itself instead of handing the RIFF to the
// system:
//
//   - PlaySound is one channel. Music through it would cut off every unit
//     acknowledgement, and every acknowledgement would cut off the music.
//   - waveOut takes raw PCM, so decoding here means no dependency on whichever
//     ACM codec happens to be installed.
//
// The decoder is the standard one: each block carries a predictor index, a
// delta and two priming samples per channel, then nibbles, and each nibble is
// a correction on top of the two-tap prediction.
#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <utility>

#include "stark.h"

namespace mus {

// The coefficient pairs every MS ADPCM stream starts from; a file may override
// them in its extra format bytes, which is why they are read from there.
constexpr int ADAPT[16] = { 230, 230, 230, 230, 307, 409, 512, 614,
                            768, 614, 512, 409, 307, 230, 230, 230 };

struct Track {
    int channels = 0, rate = 0;
    std::vector<int16_t> pcm;
    bool ok() const { return channels > 0 && !pcm.empty(); }
};

inline int16_t clamp16(int v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return int16_t(v);
}

// One channel's next sample from a nibble.
struct AdpcmState {
    int coef1 = 0, coef2 = 0, delta = 0, s1 = 0, s2 = 0;

    int16_t step(int nib)
    {
        int sign = nib & 8 ? nib - 16 : nib;
        int pred = (s1 * coef1 + s2 * coef2) / 256 + sign * delta;
        int16_t out = clamp16(pred);
        delta = (ADAPT[nib] * delta) / 256;
        if (delta < 16) delta = 16;
        s2 = s1;
        s1 = out;
        return out;
    }
};

// A whole record to PCM. Returns false when the record is not ADPCM or the
// block layout does not add up, rather than producing noise.
inline bool decode(const std::vector<uint8_t> &r, Track &t)
{
    if (r.size() < 32) return false;
    uint16_t tag = ark::rd16(&r[0]), ch = ark::rd16(&r[2]);
    uint32_t rate = ark::rd32(&r[4]);
    uint16_t align = ark::rd16(&r[12]), bits = ark::rd16(&r[14]);
    uint16_t cb = ark::rd16(&r[16]);
    uint32_t len = ark::rd32(&r[18]);
    if (tag != 2 || bits != 4 || (ch != 1 && ch != 2)) return false;
    if (len == 0 || len > r.size() || align < 7 * ch) return false;

    // The header is WAVEFORMATEX, then the sample length, then a few bytes
    // that differ between the sound bank and the music, and the ADPCM
    // coefficient block last - so find it from the end of the header, not from
    // a fixed offset. On GM_WAR101 the header is 62 bytes and cbSize 32, which
    // puts the block at +30, eight bytes past where a fixed +22 would look.
    size_t data = r.size() - len;
    if (cb < 4 || size_t(cb) > data) return false;
    size_t ext = data - cb;
    int ncoef = ark::rd16(&r[ext + 2]);
    if (ncoef <= 0 || ncoef > 32) return false;
    if (ext + 4 + size_t(ncoef) * 4 > data) return false;
    std::vector<std::pair<int, int>> coef(size_t(ncoef), { 0, 0 });
    for (int i = 0; i < ncoef; ++i) {
        coef[size_t(i)].first  = int16_t(ark::rd16(&r[ext + 4 + size_t(i) * 4]));
        coef[size_t(i)].second = int16_t(ark::rd16(&r[ext + 6 + size_t(i) * 4]));
    }

    t.channels = ch;
    t.rate = int(rate);
    t.pcm.clear();
    t.pcm.reserve(len * 2);

    for (size_t b = data; b + align <= r.size(); b += align) {
        const uint8_t *p = &r[b];
        AdpcmState st[2];
        for (int c = 0; c < ch; ++c) {
            int idx = p[c];
            if (idx >= ncoef) return false;
            st[c].coef1 = coef[size_t(idx)].first;
            st[c].coef2 = coef[size_t(idx)].second;
        }
        const uint8_t *q = p + ch;
        for (int c = 0; c < ch; ++c) { st[c].delta = int16_t(ark::rd16(q)); q += 2; }
        for (int c = 0; c < ch; ++c) { st[c].s1    = int16_t(ark::rd16(q)); q += 2; }
        for (int c = 0; c < ch; ++c) { st[c].s2    = int16_t(ark::rd16(q)); q += 2; }
        // The two priming samples come out before any nibble is read.
        for (int c = 0; c < ch; ++c) t.pcm.push_back(int16_t(st[c].s2));
        for (int c = 0; c < ch; ++c) t.pcm.push_back(int16_t(st[c].s1));
        const uint8_t *end = p + align;
        int c = 0;
        while (q < end) {
            uint8_t byte = *q++;
            t.pcm.push_back(st[c].step(byte >> 4));
            c = (c + 1) % ch;
            t.pcm.push_back(st[c].step(byte & 0x0F));
            c = (c + 1) % ch;
        }
    }
    return !t.pcm.empty();
}

// waveOut playback on its own device, so PlaySound keeps the effects.
class Player {
public:
    bool open(const std::string &gameDir)
    {
        return ar.open(gameDir + "\\MUSIC\\MUSIC.DKX", gameDir + "\\MUSIC\\MUSIC.DKD");
    }

    bool ok() const { return ar.count() > 0; }
    size_t count() const { return ar.count(); }
    const std::string &playing() const { return name; }
    std::vector<uint8_t> raw(const std::string &n) const { return ar.read(n); }

    bool play(const std::string &track, bool loop = true)
    {
        stop();
        Track t;
        if (!decode(ar.read(track), t) || !t.ok()) return false;

        WAVEFORMATEX wf{};
        wf.wFormatTag = WAVE_FORMAT_PCM;
        wf.nChannels = WORD(t.channels);
        wf.nSamplesPerSec = DWORD(t.rate);
        wf.wBitsPerSample = 16;
        wf.nBlockAlign = WORD(t.channels * 2);
        wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
        if (waveOutOpen(&dev, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
            dev = nullptr;
            return false;
        }
        pcm = std::move(t.pcm);
        std::memset(&hdr, 0, sizeof(hdr));
        hdr.lpData = reinterpret_cast<LPSTR>(pcm.data());
        hdr.dwBufferLength = DWORD(pcm.size() * sizeof(int16_t));
        // waveOut can loop a buffer by itself, so nothing has to feed it.
        hdr.dwLoops = loop ? 0xFFFFFFFFu : 1;
        hdr.dwFlags = loop ? (WHDR_BEGINLOOP | WHDR_ENDLOOP) : 0;
        if (waveOutPrepareHeader(dev, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR ||
            waveOutWrite(dev, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR) {
            stop();
            return false;
        }
        name = track;
        return true;
    }

    void stop()
    {
        if (!dev) return;
        waveOutReset(dev);
        waveOutUnprepareHeader(dev, &hdr, sizeof(hdr));
        waveOutClose(dev);
        dev = nullptr;
        name.clear();
        pcm.clear();
    }

    ~Player() { stop(); }

private:
    ark::Archive ar;
    HWAVEOUT dev = nullptr;
    WAVEHDR hdr{};
    std::vector<int16_t> pcm;
    std::string name;
};

}  // namespace mus
