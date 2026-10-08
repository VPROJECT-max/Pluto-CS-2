param(
    [Parameter(Mandatory = $true)][string]$Executable
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Executable -PathType Leaf)) {
    throw "Portable executable not found: $Executable"
}
if ([IO.Path]::GetFileName($Executable) -cne 'Pluto-portable.exe') {
    throw 'Portable executable must be named exactly Pluto-portable.exe'
}

$dumpbin = Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\2022' -Filter dumpbin.exe -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\Hostx64\\x64\\dumpbin\.exe$' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $dumpbin) {
    $command = Get-Command dumpbin.exe -ErrorAction SilentlyContinue
    if ($command) { $dumpbin = $command.Source }
}
if (-not $dumpbin) { throw 'dumpbin.exe was not found; cannot verify the portable executable' }

$imports = & $dumpbin /DEPENDENTS $Executable | Out-String
if ($LASTEXITCODE -ne 0) { throw 'dumpbin failed while inspecting dependencies' }
foreach ($dependency in @(
    'MSVCP140.dll', 'VCRUNTIME140.dll', 'VCRUNTIME140_1.dll', 'api-ms-win-crt-',
    'libcurl.dll', 'zlib1.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll')) {
    if ($imports -match [regex]::Escape($dependency)) {
        throw "Portable build requires an external runtime dependency: $dependency"
    }
}

$headers = & $dumpbin /HEADERS $Executable | Out-String
if ($LASTEXITCODE -ne 0 -or $headers -notmatch 'machine \(x64\)') {
    throw 'Portable executable is not an x64 PE image'
}

$version = (Get-Item -LiteralPath $Executable).VersionInfo
if ($version.ProductName -cne 'Pluto' -or $version.ProductVersion -cne '2.5.0') {
    throw 'Portable VERSIONINFO does not identify Pluto 2.5.0'
}
if ($version.OriginalFilename -cne 'Pluto-portable.exe') {
    throw 'Portable VERSIONINFO has the wrong original filename'
}

$mt = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Filter mt.exe -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match '\\x64\\mt\.exe$' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $mt) { throw 'mt.exe was not found; cannot inspect the embedded manifest' }
$manifest = Join-Path ([IO.Path]::GetTempPath()) ("pluto-manifest-$PID.xml")
try {
    & $mt "-inputresource:$Executable;#1" "-out:$manifest" | Out-Null
    if ($LASTEXITCODE -ne 0) { throw 'Unable to extract the portable manifest' }
    $manifestText = Get-Content -Raw -LiteralPath $manifest
    if ($manifestText -notmatch 'requestedExecutionLevel level="requireAdministrator"') {
        throw 'Portable executable does not request administrator privileges'
    }
} finally {
    Remove-Item -LiteralPath $manifest -Force -ErrorAction SilentlyContinue
}

Add-Type -AssemblyName System.Drawing
$icon = [Drawing.Icon]::ExtractAssociatedIcon([IO.Path]::GetFullPath($Executable))
try {
    if ($null -eq $icon -or $icon.Width -lt 16 -or $icon.Height -lt 16) {
        throw 'Portable executable does not contain the Pluto icon'
    }
} finally {
    if ($icon) { $icon.Dispose() }
}

Write-Output "portable dependency/resource check passed: $Executable"
