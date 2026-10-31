# Shared by install.ps1 and uninstall.ps1: finds the game's bin64 folder through Steam.

$CoopPlusAppId = '3321460'  # Crimson Desert on Steam

function Find-CrimsonDesertBin64 {
    $roots = @()
    foreach ($key in 'HKCU:\Software\Valve\Steam', 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam', 'HKLM:\SOFTWARE\Valve\Steam') {
        try {
            $p = Get-ItemProperty -Path $key -ErrorAction Stop
            foreach ($name in 'SteamPath', 'InstallPath') {
                if ($p.$name) { $roots += ($p.$name -replace '/', '\') }
            }
        } catch {}
    }
    $libraries = @()
    foreach ($r in ($roots | Select-Object -Unique)) {
        $libraries += $r
        $vdf = Join-Path $r 'steamapps\libraryfolders.vdf'
        if (Test-Path $vdf) {
            foreach ($m in [regex]::Matches((Get-Content $vdf -Raw), '"path"\s+"([^"]+)"')) {
                $libraries += ($m.Groups[1].Value -replace '\\\\', '\')
            }
        }
    }
    foreach ($lib in ($libraries | Select-Object -Unique)) {
        $manifest = Join-Path $lib "steamapps\appmanifest_$CoopPlusAppId.acf"
        if (Test-Path $manifest) {
            $installdir = 'Crimson Desert'
            $m = [regex]::Match((Get-Content $manifest -Raw), '"installdir"\s+"([^"]+)"')
            if ($m.Success) { $installdir = $m.Groups[1].Value }
            $bin = Join-Path $lib "steamapps\common\$installdir\bin64"
            if (Test-Path (Join-Path $bin 'CrimsonDesert.exe')) { return $bin }
        }
    }
    return $null
}
