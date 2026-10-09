$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$working = Join-Path $repo 'starline-imgui-menu-GOOD FOR CS 2'

function Get-NormalizedSourceHash([string]$Path) {
    $text = [IO.File]::ReadAllText($Path)
    $normalized = $text.Replace("`r`n", "`n").Replace("`r", "`n")
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes($normalized)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash($bytes))).Replace('-', '') }
    finally { $sha.Dispose() }
}

$primitiveHashes = @{
    'menu_framework.cpp' = '042CF46C76DA1DFB41EA5BB94C911B5266301399EC290C87A49D109E8981017E'
    'loader_framework.cpp' = '90789998108D80C7D8B1E47CECA788815B219A7FCEF16C5ABF775BE8A6C590AE'
}
foreach ($name in $primitiveHashes.Keys) {
    $workingHash = Get-NormalizedSourceHash (Join-Path $working $name)
    if ($primitiveHashes[$name] -cne $workingHash) { throw "Reference UI primitive source changed: $name" }
}

foreach ($name in @('external_esp_menu.cpp', 'external_esp_menu.h', 'local_account_loader.cpp', 'local_account_loader.h', 'local_account.cpp', 'local_account.hpp')) {
    if (-not (Test-Path -LiteralPath (Join-Path $working $name) -PathType Leaf)) {
        throw "Missing Starline integration module: $name"
    }
}

$stbPath = 'starline-imgui-menu-GOOD FOR CS 2/stb_image.h'
& git -C $repo ls-files --error-unmatch -- $stbPath *> $null
if ($LASTEXITCODE -ne 0) {
    throw 'stb_image.h must be tracked so clean Release builds can compile ImageLoader.cpp'
}
$imageLoader = Get-Content -Raw -LiteralPath (Join-Path $repo 'src/assets/images/ImageLoader.cpp')
if ($imageLoader -notmatch '#include\s+"\.\./\.\./\.\./starline-imgui-menu-GOOD FOR CS 2/stb_image\.h"') {
    throw 'ImageLoader.cpp must include the tracked stb_image.h by an explicit path'
}

$project = Get-Content -Raw -LiteralPath (Join-Path $repo 'src/src.vcxproj')
foreach ($name in @('external_esp_menu.cpp', 'local_account_loader.cpp', 'local_account.cpp', 'menu_framework.cpp', 'loader_framework.cpp', 'imgui_text_renderer.cpp')) {
    if ($project -notmatch [regex]::Escape($name)) { throw "Main project does not compile Starline module: $name" }
}

$shell = Get-Content -Raw -LiteralPath (Join-Path $repo 'src/gui/frontend/menu/MenuShell.cpp')
if ($shell -notmatch 'RenderExternalEsp') { throw 'Menu shell does not render the Starline ESP menu' }

$renderer = Get-Content -Raw -LiteralPath (Join-Path $repo 'src/gui/renderer/Renderer.cpp')
if ($renderer -notmatch 'LocalAccountLoader') { throw 'Renderer does not gate startup through the local account loader' }

$main = Get-Content -Raw -LiteralPath (Join-Path $repo 'src/main.cpp')
if ($main -match '__TIMESTAMP__') { throw 'Startup banner still reports the source-file timestamp instead of the build time' }
if ($main -notmatch '__DATE__' -or $main -notmatch '__TIME__') { throw 'Startup banner does not report the actual compilation date and time' }

$adapter = Get-Content -Raw -LiteralPath (Join-Path $working 'external_esp_menu.cpp')
foreach ($tab in @('Visuals', 'World', 'System', 'Config')) {
    if ($adapter -notmatch ('Tab\("' + $tab + '"')) { throw "Missing active menu tab: $tab" }
}
foreach ($removed in @('Player Shapes', 'Aimbot', 'Changer', 'Skin Changer', 'Anti Aim')) {
    if ($adapter -match [regex]::Escape($removed)) { throw "Unrequested menu content is active: $removed" }
}

Write-Output 'Starline integration contracts passed'
