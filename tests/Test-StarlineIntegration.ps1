$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$working = Join-Path $repo 'starline-imgui-menu-GOOD FOR CS 2'

$primitiveHashes = @{
    'menu_framework.cpp' = '465A78D409F6844A805215D07DBB98A623C33966EAD610C6828C4467F5DB6A15'
    'loader_framework.cpp' = 'D549C014AC63BDFEC13716BA76384F46F631B8DA8ED82254EBE5ECF743E8278A'
}
foreach ($name in $primitiveHashes.Keys) {
    $workingHash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $working $name)).Hash
    if ($primitiveHashes[$name] -cne $workingHash) { throw "Reference UI primitive source changed: $name" }
}

foreach ($name in @('external_esp_menu.cpp', 'external_esp_menu.h', 'local_account_loader.cpp', 'local_account_loader.h', 'local_account.cpp', 'local_account.hpp')) {
    if (-not (Test-Path -LiteralPath (Join-Path $working $name) -PathType Leaf)) {
        throw "Missing Starline integration module: $name"
    }
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
