param(
    [Parameter(Mandatory = $true)][string]$InputDirectory,
    [Parameter(Mandatory = $true)][string]$OutputFile,
    [switch]$Check
)

$ErrorActionPreference = 'Stop'

$files = @{
    offsets = Join-Path $InputDirectory 'offsets.json'
    client  = Join-Path $InputDirectory 'client_dll.json'
    info    = Join-Path $InputDirectory 'info.json'
}
$errors = [System.Collections.Generic.List[string]]::new()
$documents = @{}

function Get-Sha256([string]$path) {
    $stream = [System.IO.File]::OpenRead($path)
    try {
        $sha = [System.Security.Cryptography.SHA256]::Create()
        try { return ([System.BitConverter]::ToString($sha.ComputeHash($stream))).Replace('-', '') }
        finally { $sha.Dispose() }
    } finally { $stream.Dispose() }
}

foreach ($name in @('offsets', 'client', 'info')) {
    $path = $files[$name]
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
        $errors.Add("Missing required dump file: $path")
        continue
    }
    try {
        $documents[$name] = Get-Content -Raw -LiteralPath $path | ConvertFrom-Json
    } catch {
        $errors.Add("Malformed JSON in $path`: $($_.Exception.Message)")
    }
}

$mapping = @(
    @{ Name='viewMatrix'; File='offsets'; Path=@('client.dll','dwViewMatrix') },
    @{ Name='globalVars'; File='offsets'; Path=@('client.dll','dwGlobalVars') },
    @{ Name='entityList'; File='offsets'; Path=@('client.dll','dwEntityList') },
    @{ Name='localPlayerController'; File='offsets'; Path=@('client.dll','dwLocalPlayerController') },
    @{ Name='plantedC4'; File='offsets'; Path=@('client.dll','dwPlantedC4') },
    @{ Name='weaponC4'; File='offsets'; Path=@('client.dll','dwWeaponC4') },
    @{ Name='buildNumber'; File='offsets'; Path=@('engine2.dll','dwBuildNumber') },

    @{ Name='m_iPing'; File='client'; Path=@('client.dll','classes','CCSPlayerController','fields','m_iPing') },
    @{ Name='m_hPawn'; File='client'; Path=@('client.dll','classes','CBasePlayerController','fields','m_hPawn') },
    @{ Name='m_steamID'; File='client'; Path=@('client.dll','classes','CBasePlayerController','fields','m_steamID') },
    @{ Name='m_iszPlayerName'; File='client'; Path=@('client.dll','classes','CBasePlayerController','fields','m_iszPlayerName') },
    @{ Name='m_bIsLocalPlayerController'; File='client'; Path=@('client.dll','classes','CBasePlayerController','fields','m_bIsLocalPlayerController') },
    @{ Name='m_pInGameMoneyServices'; File='client'; Path=@('client.dll','classes','CCSPlayerController','fields','m_pInGameMoneyServices') },
    @{ Name='m_iAccount'; File='client'; Path=@('client.dll','classes','CCSPlayerController_InGameMoneyServices','fields','m_iAccount') },

    @{ Name='m_vOldOrigin'; File='client'; Path=@('client.dll','classes','C_BasePlayerPawn','fields','m_vOldOrigin') },
    @{ Name='m_iHealth'; File='client'; Path=@('client.dll','classes','C_BaseEntity','fields','m_iHealth') },
    @{ Name='m_iTeamNum'; File='client'; Path=@('client.dll','classes','C_BaseEntity','fields','m_iTeamNum') },
    @{ Name='m_bIsScoped'; File='client'; Path=@('client.dll','classes','C_CSPlayerPawn','fields','m_bIsScoped') },
    @{ Name='m_ArmorValue'; File='client'; Path=@('client.dll','classes','C_CSPlayerPawn','fields','m_ArmorValue') },
    @{ Name='m_bIsDefusing'; File='client'; Path=@('client.dll','classes','C_CSPlayerPawn','fields','m_bIsDefusing') },
    @{ Name='m_vecAbsVelocity'; File='client'; Path=@('client.dll','classes','C_BaseEntity','fields','m_vecAbsVelocity') },
    @{ Name='m_pGameSceneNode'; File='client'; Path=@('client.dll','classes','C_BaseEntity','fields','m_pGameSceneNode') },
    @{ Name='m_entitySpottedState'; File='client'; Path=@('client.dll','classes','C_CSPlayerPawn','fields','m_entitySpottedState') },
    @{ Name='m_bSpottedByMask'; File='client'; Path=@('client.dll','classes','EntitySpottedState_t','fields','m_bSpottedByMask') },
    @{ Name='m_flFlashOverlayAlpha'; File='client'; Path=@('client.dll','classes','C_CSPlayerPawnBase','fields','m_flFlashOverlayAlpha') },
    @{ Name='m_angEyeAngles'; File='client'; Path=@('client.dll','classes','C_CSPlayerPawn','fields','m_angEyeAngles') },
    @{ Name='m_pWeaponServices'; File='client'; Path=@('client.dll','classes','C_BasePlayerPawn','fields','m_pWeaponServices') },
    @{ Name='m_hActiveWeapon'; File='client'; Path=@('client.dll','classes','CPlayer_WeaponServices','fields','m_hActiveWeapon') },
    @{ Name='m_AttributeManager'; File='client'; Path=@('client.dll','classes','C_EconEntity','fields','m_AttributeManager') },
    @{ Name='m_Item'; File='client'; Path=@('client.dll','classes','C_AttributeContainer','fields','m_Item') },
    @{ Name='m_iItemDefinitionIndex'; File='client'; Path=@('client.dll','classes','C_EconItemView','fields','m_iItemDefinitionIndex') },
    @{ Name='m_iClip1'; File='client'; Path=@('client.dll','classes','C_BasePlayerWeapon','fields','m_iClip1') },
    @{ Name='m_bInReload'; File='client'; Path=@('client.dll','classes','C_CSWeaponBase','fields','m_bInReload') },
    @{ Name='m_pObserverServices'; File='client'; Path=@('client.dll','classes','C_BasePlayerPawn','fields','m_pObserverServices') },

    @{ Name='m_bC4Activated'; File='client'; Path=@('client.dll','classes','C_PlantedC4','fields','m_bC4Activated') },
    @{ Name='m_nBombSite'; File='client'; Path=@('client.dll','classes','C_PlantedC4','fields','m_nBombSite') },
    @{ Name='m_bBeingDefused'; File='client'; Path=@('client.dll','classes','C_PlantedC4','fields','m_bBeingDefused') },
    @{ Name='m_flDefuseCountDown'; File='client'; Path=@('client.dll','classes','C_PlantedC4','fields','m_flDefuseCountDown') },
    @{ Name='m_vecAbsOrigin'; File='client'; Path=@('client.dll','classes','CGameSceneNode','fields','m_vecAbsOrigin') },
    @{ Name='m_modelState'; File='client'; Path=@('client.dll','classes','CSkeletonInstance','fields','m_modelState') },
    @{ Name='m_iObserverMode'; File='client'; Path=@('client.dll','classes','CPlayer_ObserverServices','fields','m_iObserverMode') },
    @{ Name='m_hObserverTarget'; File='client'; Path=@('client.dll','classes','CPlayer_ObserverServices','fields','m_hObserverTarget') },
    @{ Name='m_bForcedObserverMode'; File='client'; Path=@('client.dll','classes','CPlayer_ObserverServices','fields','m_bForcedObserverMode') }
)

function Read-ExactValue($document, [string[]]$path) {
    $value = $document
    foreach ($segment in $path) {
        if ($null -eq $value) {
            return $null
        }
        if ($value -is [System.Collections.IDictionary]) {
            if (-not $value.Contains($segment)) { return $null }
            $value = $value[$segment]
        } else {
            $property = $value.PSObject.Properties[$segment]
            if ($null -eq $property) { return $null }
            $value = $property.Value
        }
    }
    return $value
}

$values = [ordered]@{}
if ($errors.Count -eq 0) {
    foreach ($entry in $mapping) {
        $value = Read-ExactValue $documents[$entry.File] $entry.Path
        $displayPath = ($entry.Path -join '.')
        if ($null -eq $value) {
            $errors.Add("Missing symbol $($entry.Name) at $($entry.File):$displayPath")
            continue
        }
        if (-not ($value -is [byte] -or $value -is [int16] -or $value -is [int32] -or $value -is [int64] -or $value -is [uint16] -or $value -is [uint32] -or $value -is [uint64])) {
            $errors.Add("Non-integer symbol $($entry.Name) at $displayPath")
            continue
        }
        $number = [int64]$value
        if ($number -le 0 -or $number -gt 0x7FFFFFFF) {
            $errors.Add("Implausible symbol $($entry.Name)=$number at $displayPath")
            continue
        }
        $values[$entry.Name] = $number
    }
}

if ($errors.Count -gt 0) {
    $errors | ForEach-Object { Write-Error $_ }
    exit 1
}

$timestamp = [string](Read-ExactValue $documents.info @('timestamp'))
$build = Read-ExactValue $documents.info @('build_number')
if ([string]::IsNullOrWhiteSpace($timestamp) -or $null -eq $build) {
    Write-Error 'info.json must contain timestamp and build_number'
    exit 1
}

$offsetHash = Get-Sha256 $files.offsets
$clientHash = Get-Sha256 $files.client
$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('#pragma once')
$lines.Add('')
$lines.Add('#include <cstdint>')
$lines.Add('')
$lines.Add('namespace generated_offsets {')
$lines.Add("inline constexpr const char* source_timestamp = `"$timestamp`";")
$lines.Add("inline constexpr const char* offsets_sha256 = `"$offsetHash`";")
$lines.Add("inline constexpr const char* client_dll_sha256 = `"$clientHash`";")
$lines.Add(('inline constexpr std::uint32_t source_build_number = {0};' -f ([uint32]$build)))
$lines.Add('')
foreach ($entry in $mapping) {
    $hex = '0x{0:X}' -f ([uint64]$values[$entry.Name])
    $lines.Add("inline constexpr std::uint32_t $($entry.Name) = $hex;")
}
$lines.Add('')
$lines.Add('} // namespace generated_offsets')
$content = [string]::Join("`n", $lines) + "`n"

$fullOutput = [System.IO.Path]::GetFullPath($OutputFile)
if ($Check) {
    if (-not (Test-Path -LiteralPath $fullOutput -PathType Leaf)) {
        Write-Error "Generated header does not exist: $fullOutput"
        exit 1
    }
    if ([System.IO.File]::ReadAllText($fullOutput) -cne $content) {
        Write-Error 'Generated offset header is out of date'
        exit 1
    }
    Write-Output 'generated offsets are current'
    exit 0
}

$parent = Split-Path -Parent $fullOutput
if (-not [string]::IsNullOrEmpty($parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
$temporary = "$fullOutput.tmp"
$backup = "$fullOutput.replace-backup.tmp"
try {
    [System.IO.File]::WriteAllText($temporary, $content, [System.Text.UTF8Encoding]::new($false))
    if (Test-Path -LiteralPath $fullOutput) {
        [System.IO.File]::Replace($temporary, $fullOutput, $backup, $true)
        if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup -Force }
    } else {
        [System.IO.File]::Move($temporary, $fullOutput)
    }
} catch {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
    if (Test-Path -LiteralPath $backup) { Remove-Item -LiteralPath $backup -Force }
    Write-Error "Failed to atomically replace generated header: $($_.Exception.Message)"
    exit 1
}

Write-Output "generated offsets: $fullOutput"
