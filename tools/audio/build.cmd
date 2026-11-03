@call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cl /nologo /O2 /EHsc /std:c++20 loopback_rms.cpp ole32.lib /Fe:loopback_rms.exe >nul && echo built
