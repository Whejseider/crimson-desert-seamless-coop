# Publishes a playtest build: the player zip and the mod-manager zip from dist\ as a GitHub Release on the PUBLIC
# downloads repo (only releases and a README live there; the project repo stays private).
#
#   .\tools\publish-playtest.ps1                  # newest zips in dist\, notes from the CHANGELOG
#   .\tools\publish-playtest.ps1 -Notes "..."     # your own release notes
#
# Testers always get the newest build from https://github.com/<owner>/<repo>/releases/latest.
# Needs the GitHub CLI (gh) signed in. Never uploads the -symbols-KEEP-PRIVATE zip.
param(
    [string]$Repo = 'ashersheets-jpg/crimson-desert-coop-playtest',
    [string]$Notes = ''
)
$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$dist = Join-Path $root 'dist'
$zip = Get-ChildItem $dist -Filter 'CrimsonDesert-SeamlessCoop-*.zip' |
    Where-Object { $_.Name -notmatch 'symbols|mod-manager' } | Sort-Object LastWriteTime | Select-Object -Last 1
if (-not $zip) { throw 'No player zip in dist\ (run tools\package-coopplus.ps1 first)' }
$manager = Join-Path $dist ($zip.BaseName + '-mod-manager.zip')
$version = $zip.BaseName -replace '^CrimsonDesert-SeamlessCoop-', ''
$tag = 'v' + ($version -replace '\+', '-')

# The downloads repo, once. (gh writes "not found" to stderr, which Stop would turn into an error.)
$ErrorActionPreference = 'Continue'
& gh repo view $Repo *> $null
$exists = $LASTEXITCODE -eq 0
$ErrorActionPreference = 'Stop'
if (-not $exists) {
    & gh repo create $Repo --public --description 'Crimson Desert Seamless Co-op+ playtest downloads (unofficial fan mod)'
    if ($LASTEXITCODE -ne 0) { throw "could not create $Repo" }
}
# Its README (the install steps), refreshed with every release.
$tmp = Join-Path ([IO.Path]::GetTempPath()) ('coopplus-pub-' + [guid]::NewGuid().ToString('N'))
& gh repo clone $Repo $tmp -- --quiet
Copy-Item (Join-Path $root 'docs\release\playtest-repo-README.md') (Join-Path $tmp 'README.md') -Force
Copy-Item (Join-Path $root 'docs\release\FEATURES.md') (Join-Path $tmp 'FEATURES.md') -Force
Push-Location $tmp
try {
    & git -c core.autocrlf=false add README.md FEATURES.md  # (a CRLF warning on stderr would stop the script)
    & git diff --cached --quiet
    if ($LASTEXITCODE -ne 0) {
        & git commit -q -m "README and feature list for $tag"
        & git push -q origin HEAD
    }
} finally { Pop-Location; Remove-Item -Recurse -Force $tmp }

if (-not $Notes) {
    $Notes = "Playtest build $version. Both players install this same zip.`n`n" +
             "Install: download the first file below, then follow the README (https://github.com/$Repo#install).`n`n" +
             "The second file (-mod-manager) is for manual / Vortex / MO2 installs.`n`n" +
             "What to test: FEATURES.md (items marked Needs testing). Report with Diagnose Co-op+.cmd."
}
$files = @($zip.FullName)
if (Test-Path $manager) { $files += $manager }
# Notes through a file: Windows PowerShell 5.1 splits a native argument at embedded double quotes.
$notesFile = Join-Path ([IO.Path]::GetTempPath()) ('coopplus-notes-' + [guid]::NewGuid().ToString('N') + '.md')
[IO.File]::WriteAllText($notesFile, $Notes, (New-Object Text.UTF8Encoding($false)))
& gh release create $tag @files --repo $Repo --title "Co-op+ playtest $version" --notes-file $notesFile --latest
Remove-Item $notesFile -ErrorAction SilentlyContinue
if ($LASTEXITCODE -ne 0) { throw 'release failed' }
Write-Host "Published: https://github.com/$Repo/releases/tag/$tag"
Write-Host "Always-newest link for Discord: https://github.com/$Repo/releases/latest"
