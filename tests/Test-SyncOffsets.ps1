$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$tool = Join-Path $repo 'tools/SyncOffsets.ps1'
$source = Join-Path $repo 'offsets_input/output'
$root = Join-Path ([System.IO.Path]::GetTempPath()) ("external-esp-offset-tests-" + [guid]::NewGuid())
$inputDir = Join-Path $root 'input'
$output = Join-Path $root 'GeneratedOffsets.hpp'

function Get-Sha256([string]$path) {
    $stream = [System.IO.File]::OpenRead($path)
    try {
        $sha = [System.Security.Cryptography.SHA256]::Create()
        try { return ([System.BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '') }
        finally { $sha.Dispose() }
    } finally { $stream.Dispose() }
}

function Invoke-Sync([switch]$Check) {
    $syncArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', ('"' + $tool + '"'), '-InputDirectory', ('"' + $inputDir + '"'), '-OutputFile', ('"' + $output + '"'))
    if ($Check) { $syncArgs += '-Check' }
    $stdout = Join-Path $root 'sync.stdout'
    $stderr = Join-Path $root 'sync.stderr'
    $process = Start-Process -FilePath 'powershell.exe' -ArgumentList $syncArgs -NoNewWindow -Wait -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    return $process.ExitCode
}

function Assert-FailurePreserves([scriptblock]$Mutation) {
    & $Mutation
    $before = Get-Sha256 $output
    if ((Invoke-Sync) -eq 0) { throw 'Invalid input unexpectedly succeeded' }
    $after = Get-Sha256 $output
    if ($before -ne $after) { throw 'Failed generation changed the destination' }
}

try {
    New-Item -ItemType Directory -Path $inputDir -Force | Out-Null
    Copy-Item (Join-Path $source 'offsets.json') $inputDir
    Copy-Item (Join-Path $source 'client_dll.json') $inputDir
    Copy-Item (Join-Path $source 'info.json') $inputDir

    if ((Invoke-Sync) -ne 0) {
        if (Test-Path (Join-Path $root 'sync.stderr')) { Get-Content (Join-Path $root 'sync.stderr') | Write-Host }
        throw 'Valid supplied dumps failed generation'
    }
    if (-not (Test-Path -LiteralPath $output)) { throw 'Generator did not create output' }
    if ((Invoke-Sync -Check) -ne 0) { throw 'Check mode rejected matching output' }

    Assert-FailurePreserves {
        Remove-Item -LiteralPath (Join-Path $inputDir 'client_dll.json')
    }
    Copy-Item (Join-Path $source 'client_dll.json') $inputDir

    Assert-FailurePreserves {
        Set-Content -LiteralPath (Join-Path $inputDir 'client_dll.json') -Value '{invalid'
    }
    Copy-Item (Join-Path $source 'client_dll.json') $inputDir -Force

    Assert-FailurePreserves {
        $document = Get-Content -Raw (Join-Path $inputDir 'offsets.json') | ConvertFrom-Json
        $document.'client.dll'.PSObject.Properties.Remove('dwViewMatrix')
        $document | ConvertTo-Json -Depth 20 | Set-Content (Join-Path $inputDir 'offsets.json')
    }
    Copy-Item (Join-Path $source 'offsets.json') $inputDir -Force

    Assert-FailurePreserves {
        $document = Get-Content -Raw (Join-Path $inputDir 'offsets.json') | ConvertFrom-Json
        $document.'client.dll'.dwViewMatrix = 0
        $document | ConvertTo-Json -Depth 20 | Set-Content (Join-Path $inputDir 'offsets.json')
    }
    Copy-Item (Join-Path $source 'offsets.json') $inputDir -Force

    $classes = Get-Content -Raw (Join-Path $inputDir 'client_dll.json') | ConvertFrom-Json
    $classes.'client.dll'.classes.C_BaseEntity.fields | Add-Member -NotePropertyName m_hPawn -NotePropertyValue 999 -Force
    $classes | ConvertTo-Json -Depth 100 | Set-Content (Join-Path $inputDir 'client_dll.json')
    if ((Invoke-Sync) -ne 0) {
        if (Test-Path (Join-Path $root 'sync.stderr')) { Get-Content (Join-Path $root 'sync.stderr') | Write-Host }
        throw 'Class-qualified duplicate-field case failed'
    }
    if ((Get-Content -Raw $output) -notmatch 'm_hPawn = 0x6BC') { throw 'Generator used an ambiguous field name instead of the exact class path' }

    Add-Content -LiteralPath $output -Value '// mismatch'
    $before_check = Get-Sha256 $output
    if ((Invoke-Sync -Check) -eq 0) { throw 'Check mode accepted mismatched output' }
    if ($before_check -ne (Get-Sha256 $output)) { throw 'Check mode modified output' }

    Write-Output 'offset sync tests passed'
} finally {
    if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
}
