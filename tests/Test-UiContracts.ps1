param([switch]$ThemeOnly)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot

function Require-File([string]$relative) {
    $path = Join-Path $repo $relative
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing required UI module: $relative" }
    return $path
}

$theme = Get-Content -Raw -LiteralPath (Require-File 'src/gui/theme/Theme.hpp')
foreach ($token in @('Canvas', 'Surface', 'Raised', 'Text', 'Secondary', 'NavigationWidth')) {
    if ($theme -notmatch [regex]::Escape($token)) { throw "Theme token missing: $token" }
}
if ($ThemeOnly) { Write-Output 'theme contracts passed'; exit 0 }

$menu = (Get-Content -Raw -LiteralPath (Require-File 'src/gui/frontend/menu/Menu.cpp')) +
    (Get-Content -Raw -LiteralPath (Require-File 'src/gui/frontend/menu/MenuShell.cpp'))
foreach ($page in @('Visuals', 'World', 'System', 'Config')) {
    if ($menu -notmatch [regex]::Escape($page)) { throw "Menu dispatch missing: $page" }
}

$visuals = Get-Content -Raw -LiteralPath (Require-File 'src/gui/frontend/menu/pages/VisualsPage.cpp')
$world = Get-Content -Raw -LiteralPath (Require-File 'src/gui/frontend/menu/pages/WorldPage.cpp')
$system = Get-Content -Raw -LiteralPath (Require-File 'src/gui/frontend/menu/pages/SystemPage.cpp')
$null = Require-File 'src/gui/frontend/menu/pages/ConfigPage.cpp'
$null = Require-File 'src/gui/frontend/preview/EspPreview.cpp'

foreach ($key in @('team','box','armor','health','skeleton','head_tracker','health_number','box_style','box_thickness','outline','offscreen_indicators','fade_start','fade_end','max_distance','skeleton_thickness','text_scale','bar_thickness','spotted','tracers','chams','eye_ray','visible_check')) {
    if ($visuals -notmatch "cfg::esp::$key") { throw "Visuals setting missing: cfg::esp::$key" }
}
foreach ($key in @('spectators','bomb','crosshair','radar','velocity')) {
    if ($world -notmatch "cfg::world::$key") { throw "World setting group missing: cfg::world::$key" }
}
foreach ($key in @('watermark','streamproof','vsync','free_cpu','force_third_person','defusal_notification','debug_overlay')) {
    if ($system -notmatch "cfg::settings::$key") { throw "System setting missing: cfg::settings::$key" }
}

$overlays = Get-Content -Raw -LiteralPath (Require-File 'src/gui/frontend/overlays/Overlays.cpp')
if ($overlays -match 'C:\\Users\\') { throw 'Overlay source contains a machine-specific user path' }
if ($overlays -notmatch 'cfg::settings::debug_overlay') { throw 'Debug overlay is not controlled by a persisted setting' }

Write-Output 'ui contracts passed'
