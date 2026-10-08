param(
    [string]$ImagePath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'esp .avif'),
    [string]$OutputPath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'src/assets/images/EspPreviewImage.hpp')
)

$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $ImagePath -PathType Leaf)) {
    throw "Cannot find ESP preview image: $ImagePath"
}

$ffmpeg = (Get-Command ffmpeg -ErrorAction Stop).Source
$tempPng = Join-Path ([System.IO.Path]::GetTempPath()) ("external-esp-preview-" + [guid]::NewGuid() + '.png')

try {
    & $ffmpeg -loglevel error -y -i $ImagePath -frames:v 1 -update 1 $tempPng
    if ($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $tempPng -PathType Leaf)) {
        throw 'ffmpeg failed to decode the supplied ESP preview image'
    }

    $bytes = [System.IO.File]::ReadAllBytes($tempPng)
    $lines = for ($offset = 0; $offset -lt $bytes.Length; $offset += 20) {
        $last = [Math]::Min($offset + 19, $bytes.Length - 1)
        '    ' + (($bytes[$offset..$last] | ForEach-Object { '0x{0:X2}' -f $_ }) -join ', ') + ','
    }
    $content = @(
        '// Generated from esp .avif by assets/UpdateEspPreview.ps1.'
        '#pragma once'
        ''
        'inline constexpr unsigned char esp_preview_png[] = {'
        $lines
        '};'
        "inline constexpr int esp_preview_png_len = $($bytes.Length);"
        ''
    ) -join "`r`n"

    [System.IO.File]::WriteAllText($OutputPath, $content)
    Write-Output "updated ESP preview asset: $OutputPath ($($bytes.Length) bytes)"
}
finally {
    Remove-Item -LiteralPath $tempPng -Force -ErrorAction SilentlyContinue
}
