// Co-op+ (T33) test tool: records what the PC plays (WASAPI loopback of the default output) for N seconds and
// prints, per 100 ms, the loudness (RMS, dBFS) and the count of sharp onsets (a block much louder than the one before:
// footsteps, blows). Used to tell whether the other player's character makes sound: run once with the copy walking
// around a still player and once without it, same place.
//
//   loopback_rms.exe <seconds> [out.csv]
#include <windows.h>
#include <audioclient.h>
#include <mmdeviceapi.h>

#include <cmath>
#include <cstdio>
#include <vector>

int main(int argc, char** argv) {
    const int seconds = argc > 1 ? std::atoi(argv[1]) : 10;
    FILE* csv = argc > 2 ? std::fopen(argv[2], "w") : nullptr;
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* en = nullptr;
    IMMDevice* dev = nullptr;
    IAudioClient* ac = nullptr;
    IAudioCaptureClient* cap = nullptr;
    WAVEFORMATEX* wf = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator),
                                reinterpret_cast<void**>(&en))) ||
        FAILED(en->GetDefaultAudioEndpoint(eRender, eConsole, &dev)) ||
        FAILED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&ac))) ||
        FAILED(ac->GetMixFormat(&wf)) ||
        FAILED(ac->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 10000000, 0, wf, nullptr)) ||
        FAILED(ac->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&cap)))) {
        std::printf("audio init failed\n");
        return 1;
    }
    // The shared mix format is 32-bit float on Windows 10/11.
    const int ch = wf->nChannels, rate = wf->nSamplesPerSec, block = rate / 10;
    ac->Start();
    std::vector<float> mono;
    const ULONGLONG end = GetTickCount64() + static_cast<ULONGLONG>(seconds) * 1000;
    while (GetTickCount64() < end) {
        Sleep(10);
        UINT32 n = 0;
        while (SUCCEEDED(cap->GetNextPacketSize(&n)) && n) {
            BYTE* data;
            UINT32 frames;
            DWORD flags;
            if (FAILED(cap->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) break;
            const float* f = reinterpret_cast<const float*>(data);
            for (UINT32 i = 0; i < frames; ++i) {
                float s = 0;
                for (int c = 0; c < ch; ++c) s += (flags & AUDCLNT_BUFFERFLAGS_SILENT) ? 0 : f[i * ch + c];
                mono.push_back(s / ch);
            }
            cap->ReleaseBuffer(frames);
        }
    }
    ac->Stop();
    double prev = 0, sum_db = 0;
    int onsets = 0, blocks = 0;
    for (size_t at = 0; at + block <= mono.size(); at += block) {
        // High-passed (first difference): footsteps and blows are sharp, wind and water are not.
        double e = 0;
        for (int i = 1; i < block; ++i) {
            const double d = mono[at + i] - mono[at + i - 1];
            e += d * d;
        }
        const double rms = std::sqrt(e / block);
        const double db = 20 * std::log10(rms + 1e-9);
        if (blocks && rms > prev * 2.5 && db > -60) ++onsets;
        if (csv) std::fprintf(csv, "%d,%.2f\n", blocks, db);
        sum_db += db;
        prev = rms;
        ++blocks;
    }
    std::printf("blocks %d  mean %.1f dBFS (high-passed)  onsets %d\n", blocks, blocks ? sum_db / blocks : -999.0, onsets);
    if (csv) std::fclose(csv);
    return 0;
}
