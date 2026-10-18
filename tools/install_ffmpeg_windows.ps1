param([string]$Destination = (Join-Path $PSScriptRoot '../out/dependencies/ffmpeg'))
$ErrorActionPreference = 'Stop'
# Pinned LGPL shared build, including headers and MSVC import libraries.
$version = 'ffmpeg-n8.1.3-18-g46d899d7a0-win64-lgpl-shared-8.1'
$sha256 = 'EB9346BDB3312D51CE5BFCF78041FD17E3DB91F9FFF9446D0D758406B85D2A51'
$url = "https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2026-10-18-13-01/$version.zip"
New-Item -ItemType Directory -Force -Path $Destination | Out-Null
$archive = Join-Path $Destination "$version.zip"
if (!(Test-Path -LiteralPath $archive)) { Invoke-WebRequest -Uri $url -OutFile $archive }
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $sha256) { throw 'FFmpeg archive SHA256 mismatch' }
$root = Join-Path $Destination $version
if (!(Test-Path -LiteralPath (Join-Path $root 'include/libavformat/avformat.h'))) {
 Expand-Archive -LiteralPath $archive -DestinationPath $Destination
}
$root = (Resolve-Path -LiteralPath $root).Path
if ($env:GITHUB_ENV) { "FFMPEG_ROOT=$root" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8 }
Write-Output $root
