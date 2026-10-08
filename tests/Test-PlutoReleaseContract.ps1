param()

$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))

function Assert-Match {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if ($Text -notmatch $Pattern) { throw $Message }
}

function Assert-Equal {
    param([string]$Actual, [string]$Expected, [string]$Message)
    if ($Actual -cne $Expected) { throw "$Message (expected '$Expected', got '$Actual')" }
}

$versionHeader = Get-Content -Raw (Join-Path $repo 'src\core\version\AppVersion.hpp')
$updater = Get-Content -Raw (Join-Path $repo 'src\updater\Updater.cpp')
$builder = Get-Content -Raw (Join-Path $repo 'tools\BuildPortable.ps1')
$releaseWorkflow = Get-Content -Raw (Join-Path $repo '.github\workflows\release.yml')
$mainWorkflow = Get-Content -Raw (Join-Path $repo '.github\workflows\auto_build.yml')
$project = Get-Content -Raw (Join-Path $repo 'src\src.vcxproj')
$httpHelper = Get-Content -Raw (Join-Path $repo 'src\updater\http\HttpHelper.cpp')

$versionMatch = [regex]::Match($versionHeader, 'current_text\s*\{\s*"(?<version>\d+\.\d+\.\d+)"\s*\}')
if (-not $versionMatch.Success) { throw 'Unable to parse AppVersion current_text' }
Assert-Equal $versionMatch.Groups['version'].Value '2.5.0' 'Pluto product version mismatch'

$endpointMatch = [regex]::Match($updater, 'https://api\.github\.com/repos/VPROJECT-max/Pluto-CS-2/releases/latest')
if (-not $endpointMatch.Success) { throw 'Updater latest-release endpoint mismatch' }

Assert-Match $builder "Join-Path \`$OutputDirectory 'Pluto-portable\.exe'" 'Portable builder output name mismatch'
Assert-Match $releaseWorkflow "tags:\s*- 'v\*\.\*\.\*'" 'Release workflow must trigger on stable SemVer tags'
Assert-Match $releaseWorkflow 'gh release create' 'Release workflow must publish with GitHub CLI'
Assert-Match $releaseWorkflow 'dist[/\\]Pluto-portable\.exe' 'Release workflow asset name mismatch'
Assert-Match $releaseWorkflow 'contents:\s*write' 'Release workflow requires contents write permission'
Assert-Match $releaseWorkflow 'function Invoke-Checked' 'Release workflow must fail immediately when any verification process fails'
Assert-Match $releaseWorkflow 'Write-Host\s+"Running:' 'Release workflow must identify each verification process before launch'
Assert-Match $releaseWorkflow 'WaitForExit\(' 'Release workflow must bound every verification process'
Assert-Match $releaseWorkflow "Invoke-Checked 'pwsh'" 'Release workflow must keep nested scripts on PowerShell 7'
Assert-Match $mainWorkflow 'branches:\s*- main' 'Main workflow must build the main branch'
Assert-Match $mainWorkflow 'function Invoke-Checked' 'Main workflow must fail immediately when any verification process fails'
Assert-Match $mainWorkflow 'Write-Host\s+"Running:' 'Main workflow must identify each verification process before launch'
Assert-Match $mainWorkflow 'WaitForExit\(' 'Main workflow must bound every verification process'
Assert-Match $mainWorkflow "Invoke-Checked 'pwsh'" 'Main workflow must keep nested scripts on PowerShell 7'
if ($mainWorkflow -match 'gh release create|action-gh-release') {
    throw 'Main workflow must not publish a release'
}
Assert-Match $project '<UACExecutionLevel>RequireAdministrator</UACExecutionLevel>' 'Project must require administrator privileges'
Assert-Match $project '<ResourceCompile Include="Pluto\.rc"' 'Project must compile Pluto resources'
Assert-Match $httpHelper 'starts_with\("https://"\)\s*\?\s*"https"' 'HTTPS requests must reject plaintext redirects'
Assert-Match $httpHelper 'CURLINFO_EFFECTIVE_URL' 'Updater HTTP results must expose the effective URL'

Write-Output 'Pluto release contract passed'
