# Personal AMOLED UI and Rendering Redesign

Date: 2026-09-26

## Purpose

Redesign the existing CS2 external overlay as a polished personal tool. The result should feel quiet, precise, and dependable: AMOLED black, dark-gray hierarchy, white typography, restrained motion, and high readability. The change must preserve the working process-memory and engine behavior while improving the menu, ESP drawing, offset maintenance, configuration safety, and diagnostics.

## Success Criteria

- The menu is immediately understandable without decorative clutter.
- Common settings are reachable in at most two interactions after opening the menu.
- ESP information remains readable against bright, dark, and visually busy scenes.
- Drawing remains stable at different window sizes and Windows DPI settings.
- Existing configuration values continue to load, with safe defaults for new settings.
- New cs2-dumper data can be validated and synchronized from `offsets_input/output` with one command.
- Bad or incomplete offset data fails with an actionable report and never silently generates a partial header.
- Debug and Release configurations build successfully without launching the application.

## Non-Goals

- Rewriting the process-memory layer, cache architecture, or engine attachment flow.
- Adding aim assistance, automation, or unrelated gameplay features.
- Replacing Dear ImGui or DirectX 11.
- Adding an online account, cloud synchronization, telemetry, or network dependency.
- Making runtime behavior depend on the presence of the raw cs2-dumper folder.
- Optimizing for public distribution or onboarding unknown users.

## Selected Approach

Use a focused modular rebuild around the existing ImGui and renderer integration.

A theme-only pass was rejected because the current large menu implementation would remain difficult to maintain and inconsistent. A full application rewrite was rejected because the renderer and engine already work and rewriting them would introduce risk without improving the personal-use experience.

The redesign will keep the current renderer lifecycle and cache snapshot interface. It will replace the menu presentation, centralize visual primitives, add safer configuration handling, and introduce a deterministic offset synchronization tool.

## Visual Language

### Palette

- Canvas: near-black `#050505`.
- Primary surface: `#0A0A0A`.
- Raised surface: `#111111`.
- Hovered surface: `#171717`.
- Strong border: `#2A2A2A`.
- Quiet border: `#1C1C1C`.
- Primary text: `#F3F3F3`.
- Secondary text: `#A0A0A0`.
- Disabled text: `#626262`.
- Neutral active accent: `#E6E6E6` on dark surfaces.
- Semantic colors are reserved for state: green for healthy/success, amber for warning, and red for danger.

The interface will not use gradients, glow, neon, glass effects, animated backgrounds, or decorative sci-fi shapes.

### Typography and Spacing

- Use one clean UI font family with a compact but comfortable default size.
- Use size, weight, and whitespace—not color effects—to establish hierarchy.
- Use a consistent 4-pixel spacing grid.
- Use small radii consistently; controls should feel precise rather than soft or playful.
- Labels use sentence case. Technical identifiers may remain monospace where useful.

### Motion

- Limit animation to short state transitions for hover, selection, and visibility.
- Avoid continuous ambient animation.
- Provide an effective reduced-motion mode by making every transition optional through one theme setting.

## Information Architecture

The main window will use a narrow left navigation rail and a single content region.

1. **Visuals** — player ESP, colors, visibility rules, drawing style, and live preview.
2. **World** — radar, spectators, bomb information, crosshair, velocity, and related overlays.
3. **System** — overlay behavior, display affinity, VSync, CPU behavior, hotkeys, and diagnostics summary.
4. **Config** — profile selection, save/load/reset actions, import/export, and configuration status.

The navigation rail contains only the product mark, four labeled destinations, and a compact status indicator. It will not contain decorative blocks or nested navigation.

Each page uses a consistent structure:

- Page title and one-line description.
- Optional search/filter field when the page has many settings.
- Settings grouped into simple cards with a heading and optional reset action.
- A right-side preview pane only on Visuals, where immediate visual feedback is valuable.

## Menu Interaction Design

### Controls

Create a small shared widget set for toggles, sliders, selectors, color controls, keybind capture, section headers, help markers, and compact status pills. Every widget follows the same spacing, label placement, disabled state, and hover behavior.

### Discoverability

- Show a concise tooltip for settings whose effect is not obvious.
- Disable dependent controls visibly instead of hiding them.
- Show the reason when a control is unavailable.
- Keep destructive reset actions separated from ordinary actions and require a lightweight confirmation.

### Search

Search matches display labels and a small set of explicit keywords. Filtering happens locally and does not mutate configuration. Clearing the query restores the page immediately.

### Live ESP Preview

The preview uses a deterministic mock player rather than live game data. It reflects enabled drawing elements, colors, thickness, text sizing, bar styles, and distance fading. This gives immediate feedback while the game is closed and makes style changes testable.

### Feedback

Replace decorative toast behavior with a restrained notification queue. Notifications identify the action and outcome, remain visible briefly, and never obscure primary controls. Repeated identical notifications are coalesced.

## Code Organization

The existing menu and ESP public entry points remain stable so `Renderer` does not need architectural changes.

Proposed responsibilities:

- `gui/theme/Theme` — palette, spacing, typography sizes, radii, and ImGui style application.
- `gui/widgets/Widgets` — reusable ImGui controls with consistent behavior.
- `gui/frontend/menu/Menu` — window lifecycle, navigation, page selection, and composition only.
- `gui/frontend/menu/pages/*` — one module for each top-level page.
- `gui/frontend/preview/EspPreview` — deterministic preview model and drawing.
- `gui/frontend/esp/DrawPrimitives` — outlined lines, text, boxes, bars, arrows, and clamping helpers.
- `gui/frontend/esp/Esp` — translates a cache snapshot and configuration into primitive calls.
- `config/Config` — serialization, defaults, schema version, and migration.
- `tools/SyncOffsets.ps1` — validates dump inputs and generates the source header.
- `core/offsets/GeneratedOffsets.hpp` — generated constants consumed by runtime initialization.

Large embedded font and image data remain separate from behavioral code.

## ESP Drawing Improvements

### Primitive Rules

- Snap one-pixel strokes to appropriate pixel centers to avoid blurred boxes and lines.
- Draw a restrained dark outline behind critical text and line work for contrast.
- Use a shared text function for consistent alignment, font choice, outline, and clamping.
- Clamp off-screen labels and indicators to safe screen margins.
- Avoid allocating strings or rebuilding static geometry unnecessarily in the per-frame path.

### Player Presentation

- Offer full box and corner-box styles with configurable thickness.
- Make health and ammo bars use consistent dimensions and optional numeric labels.
- Use skeleton joints and segments with consistent thickness and outline behavior.
- Add optional off-screen indicators with distance and direction.
- Fade non-critical details over a configurable distance while preserving essential target identification.
- Resolve common label collisions using fixed anchor slots around the player bounds.
- Keep team, enemy, visible, and non-visible colors configurable while providing neutral defaults.

### Stability and Performance

- Compute player bounds once per player per frame and reuse them.
- Cull invalid, behind-camera, and sufficiently distant entities before expensive drawing.
- Cache text measurements when their content and font have not changed.
- Never let optional rendering failure prevent the remaining overlay from drawing.

## Offset Synchronization

### Inputs

The synchronization tool reads:

- `offsets_input/output/offsets.json`
- `offsets_input/output/client_dll.json`

Other dump files remain available for future expansion but are not required unless a consumed field comes from them.

### Validation

The tool validates all required dynamic addresses and schema fields before writing anything. It verifies:

- Required files exist and contain valid JSON.
- Every required symbol is present at the expected object path.
- Values are non-zero integers within an expected range.
- Duplicate symbol names are resolved through explicit class paths, never first-match search.
- The generated result contains the full required set.

Validation errors list the missing or invalid symbol and its expected source path. The existing generated header remains untouched on failure.

### Generation

On successful validation, the tool writes a deterministic `GeneratedOffsets.hpp` containing source metadata and typed constants. Runtime `Dumper` initialization copies these generated constants into the existing mutable offset values and logs the selected values.

The raw dump folder is a development input, not a runtime dependency. Builds remain reproducible from the checked-in generated header.

### Verification Mode

The script supports a check-only mode that compares dump data with the generated header and exits non-zero on mismatch. This mode is used during verification without modifying files.

## Configuration Model

- Add an integer schema version at the root of `config.json`.
- Load through a temporary settings object, validate values, then publish the completed configuration.
- Migrate known older schemas while preserving recognized user values.
- Clamp numeric settings to safe ranges.
- Preserve unknown keys when practical so newer settings are not destroyed by an older build.
- Write atomically through a temporary file and replacement operation to avoid truncated configuration after interruption.
- Provide per-section reset and full reset, both based on centralized defaults.

## Diagnostics and Error Handling

Startup logging will identify:

- Executable configuration and build timestamp.
- Loaded configuration path and schema version.
- Offset source metadata and key dynamic values.
- Game window and overlay client dimensions.
- Renderer and ImGui initialization outcome.

Debug builds may expose a compact diagnostics panel showing snapshot age, cached player count, overlay dimensions, frame time, and last configuration or offset error. Release builds keep detailed diagnostics in logs and show only actionable menu status.

Errors follow these rules:

- Fatal initialization failures stop cleanly with a specific reason.
- Recoverable optional-feature failures disable that feature and preserve the rest of the overlay.
- User-facing messages describe what action can resolve the problem.
- Repeated per-frame failures are rate-limited in logs.

## Data Flow

At runtime:

1. Engine/cache produces the existing immutable snapshot copy.
2. `Esp` filters snapshot entities and computes reusable per-player layout data.
3. `Esp` selects presentation values from configuration.
4. `DrawPrimitives` emits ImGui draw-list commands.
5. Renderer submits the completed draw data through the existing DirectX 11 path.

For settings:

1. Menu widgets edit the in-memory configuration.
2. Preview reads the same configuration and draws a deterministic sample.
3. Explicit save or existing save triggers serialize a validated snapshot atomically.

For offsets:

1. User places fresh cs2-dumper output in `offsets_input/output`.
2. Synchronization tool validates every consumed symbol.
3. Tool generates the complete header atomically.
4. Debug and Release builds embed the same generated values.

## Testing and Verification

### Automated

- Offset synchronization fixture tests: valid dump, missing key, wrong class path, malformed JSON, and no-change check mode.
- Configuration tests: old-schema migration, round trip, missing values, out-of-range values, and interrupted-write protection.
- Pure drawing-helper tests for bounds, distance alpha, clamping, anchor placement, and corner-box geometry.
- Build both `Debug|x64` and `Release|x64`.
- Inspect both binaries for the generated-offset initialization markers.

### Visual

- Render the deterministic preview at common 16:9, 16:10, and ultrawide sizes.
- Verify 100%, 125%, and 150% DPI behavior.
- Check bright, dark, and noisy preview backgrounds.
- Verify labels do not overlap in representative combinations.

The application will not be launched automatically. Any live game verification remains a user-run smoke test.

## Implementation Order

1. Add characterization tests for configuration and geometry behavior.
2. Add theme tokens and shared drawing primitives without changing public entry points.
3. Add deterministic ESP preview.
4. Recompose the menu into the new information architecture.
5. Migrate ESP rendering to shared primitives and add readability options.
6. Add configuration schema migration and atomic persistence.
7. Add validated offset synchronization and generated header.
8. Add diagnostics and perform full build/static/visual verification.

## Completion Boundary

The redesign is complete when the approved interface and drawing behavior are implemented, offset and configuration workflows are validated, both x64 configurations build, automated checks pass, and visual preview verification is recorded. Live game behavior is not claimed until the user performs the final smoke test.
