# Downloads Ultimate ASI Loader (MIT, ThirteenAG) from its official GitHub release and
# checks the SHA-256 that GitHub publishes for the asset. The DLL is cached under
# build\cache\ (git-ignored) and never committed.
#
#   .\tools\fetch-asi-loader.ps1            # prints the path of the verified dinput8.dll
#
# v9.7.4 is the version cd-coop-3446 was live-tested with (shipped as winmm.dll).
# To move to a newer release, change $Version and $Sha256 together (gh release view
# <tag> -R ThirteenAG/Ultimate-ASI-Loader --json assets shows the digests).

param(
    [string]$Version = 'v9.7.4',
    [string]$Asset = 'Ultimate-ASI-Loader-NoPDB_x64.zip',
    [string]$Sha256 = 'e5860e7d9a1805267535b65749575b5e406cc6ea3325c7392189c578815045d1'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$cache = Join-Path $root "build\cache\ultimate-asi-loader\$Version"
$dll = Join-Path $cache 'dinput8.dll'
if (Test-Path $dll) { return $dll }

New-Item -ItemType Directory -Force $cache | Out-Null
$zip = Join-Path $cache $Asset
$url = "https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/$Version/$Asset"
Write-Host "Downloading $url"
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
$got = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
if ($got -ne $Sha256) {
    Remove-Item $zip -Force
    throw "SHA-256 mismatch for $Asset`: expected $Sha256, got $got. Nothing was installed."
}
Expand-Archive -Path $zip -DestinationPath (Join-Path $cache 'unzipped') -Force
$found = Get-ChildItem (Join-Path $cache 'unzipped') -Recurse -Filter 'dinput8.dll' | Select-Object -First 1
if (-not $found) { throw "No dinput8.dll inside $Asset" }
Copy-Item $found.FullName $dll
$license = Get-ChildItem (Join-Path $cache 'unzipped') -Recurse -Include 'LICENSE*', 'license*' | Select-Object -First 1
if ($license) { Copy-Item $license.FullName (Join-Path $cache 'LICENSE.txt') }
Write-Host "Verified $Asset ($Version), SHA-256 $got"
return $dll
