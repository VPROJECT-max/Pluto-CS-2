param(
    [string]$OutputDirectory = 'dist',
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'
if ($Configuration -cne 'Release') {
    throw 'Pluto portable packaging accepts only the x64 Release configuration'
}

$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$solution = Join-Path $repo 'cs2-external-esp.sln'
$msbuildCandidates = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe',
    'C:\Program Files\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'
)
$msbuild = $msbuildCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $msbuild) {
    $command = Get-Command msbuild.exe -ErrorAction SilentlyContinue
    if ($command) { $msbuild = $command.Source }
}
if (-not $msbuild) { throw 'MSBuild was not found. Install Visual Studio 2022 C++ Build Tools.' }

& $msbuild $solution /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Release build failed with exit code $LASTEXITCODE" }

$built = Join-Path $repo 'x64\Release\cs2-external-esp.exe'
if (-not (Test-Path -LiteralPath $built -PathType Leaf)) {
    throw "Release build output was not created: $built"
}
if (-not [IO.Path]::IsPathRooted($OutputDirectory)) {
    $OutputDirectory = Join-Path $repo $OutputDirectory
}
[IO.Directory]::CreateDirectory($OutputDirectory) | Out-Null
$destination = Join-Path $OutputDirectory 'Pluto-portable.exe'
$temporary = Join-Path $OutputDirectory 'Pluto-portable.exe.part'

if (-not ('Pluto.NativeFile' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
namespace Pluto {
    public static class NativeFile {
        [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
        public static extern bool MoveFileEx(string existing, string replacement, uint flags);
    }
}
'@
}

try {
    [IO.File]::Copy($built, $temporary, $true)
    $moveReplaceExisting = 0x1
    $moveWriteThrough = 0x8
    if (-not [Pluto.NativeFile]::MoveFileEx(
        $temporary, $destination, $moveReplaceExisting -bor $moveWriteThrough)) {
        $code = [Runtime.InteropServices.Marshal]::GetLastWin32Error()
        throw "Atomic portable publication failed with Win32 error $code"
    }
} finally {
    Remove-Item -LiteralPath $temporary -Force -ErrorAction SilentlyContinue
}

& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $repo 'tests\Test-PortableBuild.ps1') -Executable $destination
if ($LASTEXITCODE -ne 0) { throw 'Portable verification failed' }

$artifact = Get-Item -LiteralPath $destination
$hash = (Get-FileHash -Algorithm SHA256 -LiteralPath $destination).Hash.ToLowerInvariant()
$sizeMiB = [Math]::Round($artifact.Length / 1MB, 2)
Write-Output "portable executable: $destination"
Write-Output "size: $sizeMiB MiB"
Write-Output "sha256:$hash"
