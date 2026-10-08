param()

$ErrorActionPreference = 'Stop'
$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$sourcePng = Join-Path $projectRoot 'Minimalist Planet P Logo.png'
$sourceIco = Join-Path $projectRoot 'Minimalist Planet P Logo.ico'
$sourceFont = Join-Path $projectRoot 'src\external\imgui\misc\fonts\Karla-Regular.ttf'
$imageHeader = Join-Path $projectRoot 'src\assets\images\PlutoLogoImage.hpp'
$fontHeader = Join-Path $projectRoot 'src\assets\fonts\PlutoWordmarkFont.hpp'
$fontLicense = Join-Path $projectRoot 'src\assets\fonts\Karla-OFL.txt'
$iconOutput = Join-Path $projectRoot 'src\assets\Pluto.ico'
$normalizedPng = Join-Path ([IO.Path]::GetTempPath()) 'pluto-brand-logo-256.png'

foreach ($required in @($sourcePng, $sourceIco, $sourceFont)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing Pluto brand source: $required"
    }
}

function Write-ByteHeader {
    param(
        [Parameter(Mandatory)][byte[]]$Bytes,
        [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][string]$Symbol
    )
    $builder = [Text.StringBuilder]::new()
    [void]$builder.AppendLine('#pragma once')
    [void]$builder.AppendLine()
    [void]$builder.AppendLine('#include <cstddef>')
    [void]$builder.AppendLine()
    [void]$builder.AppendLine('namespace pluto_assets {')
    [void]$builder.AppendLine("inline constexpr unsigned char ${Symbol}[] = {")
    for ($offset = 0; $offset -lt $Bytes.Length; $offset += 16) {
        $last = [Math]::Min($offset + 15, $Bytes.Length - 1)
        $values = for ($index = $offset; $index -le $last; $index++) {
            '0x{0:X2}' -f $Bytes[$index]
        }
        [void]$builder.Append('    ')
        [void]$builder.Append(($values -join ', '))
        [void]$builder.AppendLine(',')
    }
    [void]$builder.AppendLine('};')
    [void]$builder.AppendLine("inline constexpr std::size_t ${Symbol}Size = sizeof($Symbol);")
    [void]$builder.AppendLine('} // namespace pluto_assets')
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Path)) | Out-Null
    [IO.File]::WriteAllText($Path, $builder.ToString(), [Text.UTF8Encoding]::new($false))
}

Add-Type -AssemblyName System.Drawing
$sourceStream = [IO.File]::OpenRead($sourcePng)
try {
    $sourceImage = [Drawing.Image]::FromStream($sourceStream, $true, $true)
    try {
        $bitmap = [Drawing.Bitmap]::new(256, 256, [Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.Clear([Drawing.Color]::Transparent)
                $graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
                $graphics.CompositingQuality = [Drawing.Drawing2D.CompositingQuality]::HighQuality
                $graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::HighQuality
                $scale = [Math]::Min(256.0 / $sourceImage.Width, 256.0 / $sourceImage.Height)
                $width = [Math]::Max(1, [int][Math]::Round($sourceImage.Width * $scale))
                $height = [Math]::Max(1, [int][Math]::Round($sourceImage.Height * $scale))
                $x = [int]((256 - $width) / 2)
                $y = [int]((256 - $height) / 2)
                $graphics.DrawImage($sourceImage, [Drawing.Rectangle]::new($x, $y, $width, $height))
            } finally {
                $graphics.Dispose()
            }
            $bitmap.Save($normalizedPng, [Drawing.Imaging.ImageFormat]::Png)
        } finally {
            $bitmap.Dispose()
        }
    } finally {
        $sourceImage.Dispose()
    }
} finally {
    $sourceStream.Dispose()
}

try {
    Write-ByteHeader ([IO.File]::ReadAllBytes($normalizedPng)) $imageHeader 'PlutoLogoPng'
    Write-ByteHeader ([IO.File]::ReadAllBytes($sourceFont)) $fontHeader 'PlutoWordmarkTtf'
    Copy-Item -LiteralPath $sourceIco -Destination $iconOutput -Force
    $licenseText = @'
Karla-Regular.ttf by Jonathan Pinhorn
Licensed under the SIL Open Font License, Version 1.1.
Source copy: Dear ImGui misc/fonts/Karla-Regular.ttf
License reference: https://openfontlicense.org/open-font-license-official-text/
'@
    [IO.File]::WriteAllText($fontLicense, $licenseText, [Text.UTF8Encoding]::new($false))
} finally {
    Remove-Item -LiteralPath $normalizedPng -Force -ErrorAction SilentlyContinue
}

$imageHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $imageHeader).Hash.ToLowerInvariant()
$fontHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $fontHeader).Hash.ToLowerInvariant()
Write-Output "PlutoLogoImage.hpp sha256:$imageHash"
Write-Output "PlutoWordmarkFont.hpp sha256:$fontHash"
