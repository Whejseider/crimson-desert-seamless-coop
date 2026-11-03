# Audio test tool (T33)

`loopback_rms.exe <seconds> [out.csv]` records what the PC plays (WASAPI loopback of the default output device) and
prints the high-passed loudness (dBFS) per 100 ms and the count of sharp onsets. Build with `build.cmd` (VS 2022
Build Tools). Used by `build/port3479/audio_test.sh`.
