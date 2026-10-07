# Pluto Branding and Automatic Update Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Ship Pluto v2.5.0 with the supplied visual identity, administrator manifest, and a verified one-executable GitHub Releases self-update path.

**Architecture:** Pure C++20 modules own SemVer parsing, release selection, update policy, integrity, and maintenance-command validation. Windows-specific orchestration streams a GitHub Release asset to a constrained `%LOCALAPPDATA%` staging directory, verifies GitHub's SHA-256 digest, and uses the downloaded Pluto executable itself as the post-exit replacement helper. The existing Starline adapter gains only a right-aligned brand-render hook; D3D11 texture/font ownership stays in the existing menu lifecycle.

**Tech Stack:** C++20, Win32, Windows CNG, libcurl, nlohmann/json, Dear ImGui/D3D11, MSBuild, PowerShell, GitHub Actions/Releases.

**Spec:** `docs/superpowers/specs/2026-10-07-pluto-branding-auto-update-design.md`

## Global Constraints

- Target platform is 64-bit Windows.
- Product version is `2.5.0`; stable release tag is `v2.5.0`.
- Public update repository is `VPROJECT-max/Pluto-CS-2`.
- Release asset name is exactly `Pluto-portable.exe`.
- Release remains statically linked and self-contained; no installed updater, package manager, service, scheduled task, or embedded credential.
- Only an exact Release portable filename may self-update; Debug and renamed development outputs skip.
- Every downloaded executable requires declared-size and SHA-256 digest verification before execution.
- Update failures fail open into the current executable; replacement failures roll back.
- The executable manifest requests administrator privileges.
- Existing exact Starline controls are reused; no replacement checkbox, toggle, slider, button, or tab implementation is introduced.
- Preserve all existing dirty-tree work and upstream license attribution.

## Review Focus

- Malformed or huge release metadata must be rejected without blocking current startup; Task 2 exercises invalid JSON shapes and Task 3 caps network responses.
- A staged executable outside `%LOCALAPPDATA%\Pluto\Updates` must never enter apply mode; Task 4 tests traversal and sibling-path attacks.
- Interrupted or corrupted downloads must never become executable staging files; Tasks 3 and 4 test `.part`, size, and hash failure behavior.
- Replacement failure after the old file moves must restore the original; Task 4's disposable harness forces copy and relaunch failure.
- A tag/version/asset naming mismatch must fail CI before publication; Task 6 tests the cross-file release contract.

---

### Task 1: Strict product version model

**Files:**
- Create: `src/core/version/SemanticVersion.hpp`
- Create: `src/core/version/SemanticVersion.cpp`
- Modify: `src/core/version/AppVersion.hpp`
- Create: `tests/UpdaterModelTests.cpp`
- Create: `tests/UpdaterModelTests.vcxproj`

**Interfaces:**
- Produces: `std::optional<app_version::SemanticVersion> ParseTag(std::string_view)`.
- Produces: defaulted comparison and `std::string ToString() const` on `SemanticVersion`.
- Produces: `app_version::current == SemanticVersion{2, 5, 0}` and `app_version::current_text == "2.5.0"`.

- [ ] **Step 1: Write strict SemVer tests first**

```cpp
const auto current = app_version::ParseTag("v2.5.0");
assert(current == app_version::SemanticVersion{2, 5, 0});
assert(!app_version::ParseTag("2.5.0"));
assert(!app_version::ParseTag("v2.5"));
assert(!app_version::ParseTag("v2.5.0-beta"));
assert(!app_version::ParseTag("v4294967296.0.0"));
assert(app_version::SemanticVersion{2, 5, 0} > app_version::SemanticVersion{2, 4, 99});
static_assert(app_version::current == app_version::SemanticVersion{2, 5, 0});
```

- [ ] **Step 2: Build the test and verify RED**

Run: `MSBuild.exe tests\UpdaterModelTests.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64`

Expected: compile failure because `SemanticVersion.hpp`, `ParseTag`, and the new current-version constants do not exist.

- [ ] **Step 3: Implement the minimal strict parser**

```cpp
namespace app_version {
struct SemanticVersion {
    std::uint32_t major{};
    std::uint32_t minor{};
    std::uint32_t patch{};
    auto operator<=>(const SemanticVersion&) const = default;
    [[nodiscard]] std::string ToString() const;
};
[[nodiscard]] std::optional<SemanticVersion> ParseTag(std::string_view tag);
inline constexpr SemanticVersion current{2, 5, 0};
inline constexpr std::string_view current_text{"2.5.0"};
}
```

Parse digits manually with checked `std::uint32_t` accumulation so locale, signs, whitespace, suffixes, and overflow are rejected.

- [ ] **Step 4: Build and run GREEN**

Run the test project, then `tests\UpdaterModelTests.exe`.

Expected: `updater model tests passed`.

- [ ] **Step 5: Commit Task 1 files only**

```powershell
git add src/core/version/SemanticVersion.hpp src/core/version/SemanticVersion.cpp src/core/version/AppVersion.hpp tests/UpdaterModelTests.cpp tests/UpdaterModelTests.vcxproj
git commit -m "feat: add strict Pluto semantic versions"
```

### Task 2: GitHub release model and update policy

**Files:**
- Create: `src/updater/ReleaseModel.hpp`
- Create: `src/updater/ReleaseModel.cpp`
- Modify: `tests/UpdaterModelTests.cpp`
- Modify: `tests/UpdaterModelTests.vcxproj`

**Interfaces:**
- Consumes: `app_version::SemanticVersion` and `ParseTag` from Task 1.
- Produces: `updater::ReleaseAsset`, `updater::ReleaseInfo`, `ParseLatestRelease(const nlohmann::json&, std::string&)`, and `ShouldInstall(current, release)`.

- [ ] **Step 1: Add failing release-selection tests**

```cpp
const json valid = {
    {"tag_name", "v2.5.1"}, {"draft", false}, {"prerelease", false},
    {"assets", json::array({{
        {"name", "Pluto-portable.exe"}, {"browser_download_url", "https://github.com/VPROJECT-max/Pluto-CS-2/releases/download/v2.5.1/Pluto-portable.exe"},
        {"size", 42}, {"digest", "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"}
    }})}
};
std::string error;
const auto release = updater::ParseLatestRelease(valid, error);
assert(release && release->version == app_version::SemanticVersion{2, 5, 1});
assert(updater::ShouldInstall(app_version::current, *release));
```

Add separate assertions for draft, prerelease, equal/older version, missing asset, duplicate exact assets, wrong asset name, non-HTTPS URL, missing/invalid digest, negative/zero/overflow size, and wrong JSON types.

- [ ] **Step 2: Build and verify RED**

Expected: compile failure because `ReleaseModel` is absent.

- [ ] **Step 3: Implement release parsing without network or filesystem effects**

```cpp
struct ReleaseAsset {
    std::string download_url;
    std::uint64_t size{};
    std::string sha256_hex;
};
struct ReleaseInfo {
    app_version::SemanticVersion version;
    ReleaseAsset asset;
};
[[nodiscard]] std::optional<ReleaseInfo> ParseLatestRelease(const nlohmann::json&, std::string& error);
[[nodiscard]] bool ShouldInstall(app_version::SemanticVersion current, const ReleaseInfo&) noexcept;
```

Require one exact asset, lowercase-normalize only the digest hex, and accept only `https://github.com/` browser download URLs.

- [ ] **Step 4: Build and run GREEN**

Expected: all Task 1 and Task 2 cases pass.

- [ ] **Step 5: Commit Task 2 files only**

```powershell
git add src/updater/ReleaseModel.hpp src/updater/ReleaseModel.cpp tests/UpdaterModelTests.cpp tests/UpdaterModelTests.vcxproj
git commit -m "feat: validate Pluto GitHub releases"
```

### Task 3: Bounded HTTP and file-integrity primitives

**Files:**
- Modify: `src/updater/http/HttpHelper.hpp`
- Modify: `src/updater/http/HttpHelper.cpp`
- Create: `src/updater/FileIntegrity.hpp`
- Create: `src/updater/FileIntegrity.cpp`
- Create: `tests/UpdaterIoTests.cpp`
- Create: `tests/UpdaterIoTests.vcxproj`
- Create: `tests/http_fixture.ps1`

**Interfaces:**
- Produces: `HttpResult HttpHelper::GetJson(std::string_view, json&, std::size_t max_bytes)`.
- Produces: `HttpResult HttpHelper::Download(std::string_view, const std::filesystem::path&, std::uint64_t max_bytes)`.
- Produces: `std::optional<std::string> updater::Sha256File(const std::filesystem::path&, std::string&)`.
- Produces: `bool updater::VerifyFile(path, expected_size, expected_sha256, error)`.

- [ ] **Step 1: Write failing integrity and bounded-download tests**

```cpp
WriteBytes(temp / "payload.bin", "abc");
std::string error;
assert(updater::Sha256File(temp / "payload.bin", error)
    == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
assert(updater::VerifyFile(temp / "payload.bin", 3, expected_hash, error));
assert(!updater::VerifyFile(temp / "payload.bin", 4, expected_hash, error));
assert(!updater::VerifyFile(temp / "payload.bin", 3, wrong_hash, error));
```

The PowerShell fixture binds localhost and serves valid JSON, a redirect, an oversized body, a partial body, and a binary body. The C++ test asserts redirect success, metadata cap rejection, non-200 preservation, partial-file cleanup, and exact binary bytes.

- [ ] **Step 2: Build/run and verify RED**

Expected: missing integrity and HTTP result APIs.

- [ ] **Step 3: Implement streamed bounded transfers and CNG SHA-256**

```cpp
struct HttpResult {
    long status_code{};
    CURLcode curl_code{CURLE_OK};
    std::string error;
    [[nodiscard]] bool ok() const noexcept { return curl_code == CURLE_OK && status_code == 200; }
};
```

Set `CURLOPT_FOLLOWLOCATION`, `CURLOPT_MAXREDIRS`, `CURLOPT_CONNECTTIMEOUT_MS`, `CURLOPT_TIMEOUT_MS`, `CURLOPT_USERAGENT`, GitHub accept/API-version headers, `CURLOPT_SSL_VERIFYPEER=1`, and `CURLOPT_SSL_VERIFYHOST=2`. Abort callbacks above the configured byte cap. Download to the caller's `.part` path and delete it after any transfer/write failure. Hash in chunks using `BCryptOpenAlgorithmProvider`, `BCryptCreateHash`, `BCryptHashData`, and `BCryptFinishHash` with RAII handle wrappers.

- [ ] **Step 4: Run GREEN against the local fixture**

Expected: fixture tests pass without internet access and leave no partial files.

- [ ] **Step 5: Commit Task 3 files only**

```powershell
git add src/updater/http/HttpHelper.hpp src/updater/http/HttpHelper.cpp src/updater/FileIntegrity.hpp src/updater/FileIntegrity.cpp tests/UpdaterIoTests.cpp tests/UpdaterIoTests.vcxproj tests/http_fixture.ps1
git commit -m "feat: add bounded verified update downloads"
```

### Task 4: Startup update orchestration and rollback-safe replacement

**Files:**
- Create: `src/updater/UpdatePaths.hpp`
- Create: `src/updater/UpdatePaths.cpp`
- Create: `src/updater/SelfReplace.hpp`
- Create: `src/updater/SelfReplace.cpp`
- Rewrite: `src/updater/Updater.hpp`
- Rewrite: `src/updater/Updater.cpp`
- Modify: `src/main.cpp`
- Modify: `src/src.vcxproj`
- Modify: `src/src.vcxproj.filters`
- Modify: `tests/UpdaterModelTests.cpp`
- Create: `tests/SelfReplaceTests.cpp`
- Create: `tests/SelfReplaceTests.vcxproj`

**Interfaces:**
- Consumes: ReleaseModel, HttpHelper, FileIntegrity, and product version.
- Produces: `updater::StartupAction Updater::ProcessStartup(std::span<const std::wstring_view>)`.
- Produces: `UpdatePaths BuildUpdatePaths(module_path, local_app_data, version, error)` with validated staging/original/rollback paths.
- Produces: `int ApplyVerifiedUpdate(const ApplyRequest&)` and `void CleanupUpdate(const CleanupRequest&)`.

- [ ] **Step 1: Add failing policy, command, and path-containment tests**

```cpp
assert(updater::IsPortableRelease(L"C:\\Tools\\Pluto-portable.exe", false));
assert(!updater::IsPortableRelease(L"C:\\Tools\\cs2-external-esp.exe", false));
assert(!updater::IsPortableRelease(L"C:\\Tools\\Pluto-portable.exe", true));
assert(ParseStartupArguments({L"Pluto-portable.exe", L"--skip-update"}).mode == Mode::skip_update);
assert(!ValidateApplyRequest(outside_staging_request, error));
assert(ValidateApplyRequest(valid_temp_request, error));
```

The disposable integration test copies a harmless fixture executable into a temporary original/staging pair, exercises wait/backup/copy/rollback helpers without touching the real program, and forces failure through injected filesystem/process operations.

- [ ] **Step 2: Build and verify RED**

Expected: missing path, command, and replacement APIs.

- [ ] **Step 3: Implement minimal startup modes and validated replacement**

```cpp
enum class StartupAction { continue_launch, exit_success, exit_failure };
class Updater final {
public:
    [[nodiscard]] static StartupAction ProcessStartup(std::span<const std::wstring_view> arguments);
};
```

Normal Release portable flow fetches latest metadata, selects a newer release, downloads `.part`, verifies it, moves it to staging, launches staging with `--apply-update`, and returns `exit_success`. Apply mode waits for the parent, moves original to `.rollback`, copies staging to original, launches `--cleanup-update`, and restores rollback on failure. Cleanup validates and deletes/schedules only the expected staging and rollback paths, then continues normal app startup without another update check.

- [ ] **Step 4: Replace legacy startup blocking behavior**

Change `main` to `wmain(int argc, wchar_t* argv[])`, build a non-owning `std::wstring_view` argument span, process updater maintenance modes before engine initialization, and never prompt to ignore a network/status failure. Preserve logger teardown for normal application paths.

- [ ] **Step 5: Build and run GREEN**

Expected: model, I/O, and self-replacement tests pass; a renamed Debug output logs/skips without network.

- [ ] **Step 6: Commit Task 4 files only**

```powershell
git add src/updater src/main.cpp src/src.vcxproj src/src.vcxproj.filters tests/UpdaterModelTests.cpp tests/SelfReplaceTests.cpp tests/SelfReplaceTests.vcxproj
git commit -m "feat: install verified Pluto updates on restart"
```

### Task 5: Pluto visual identity, resources, and administrator manifest

**Files:**
- Create: `assets/UpdatePlutoBrand.ps1`
- Create: `src/assets/images/PlutoLogoImage.hpp` (generated)
- Create: `src/assets/fonts/PlutoWordmarkFont.hpp` (generated)
- Create: `src/gui/frontend/brand/PlutoBrand.hpp`
- Create: `src/gui/frontend/brand/PlutoBrand.cpp`
- Modify: `src/gui/frontend/menu/MenuShell.cpp`
- Modify: `src/gui/renderer/Renderer.cpp`
- Modify: `starline-imgui-menu-GOOD FOR CS 2/external_esp_menu.cpp`
- Modify: `starline-imgui-menu-GOOD FOR CS 2/external_esp_menu.h`
- Create: `src/resource.h`
- Create: `src/Pluto.rc`
- Modify: `src/gui/renderer/window/Window.cpp`
- Modify: `src/src.vcxproj`
- Modify: `src/src.vcxproj.filters`
- Modify: `src/core/version/AppVersion.hpp`
- Modify: `tests/OverlayPresentationTests.cpp`
- Create: `tests/PlutoBrandTests.cpp`
- Create: `tests/PlutoBrandTests.vcxproj`

**Interfaces:**
- Produces: `bool PlutoBrand::Initialize(ID3D11Device*, ImFontAtlas&)`, `void Shutdown()`, and `void RenderHeaderLockup(...)`.
- Produces: pure `BrandLayout CalculateBrandLayout(header, logo_size, wordmark_size, version_size)` for tests.
- Consumes: supplied root PNG/ICO and the existing `ImageLoader`.

- [ ] **Step 1: Write failing version and layout tests**

```cpp
static_assert(app_version::current == app_version::SemanticVersion{2, 5, 0});
const auto normal = PlutoBrand::CalculateLayout({0, 0, 1150, 60}, {38, 38}, {72, 22}, {42, 14});
assert(normal.logo.max_x <= 1130.0f);
assert(normal.version.min_x > normal.wordmark.max_x);
assert(normal.logo.min_x > 4 * 70.0f + 20.0f);
const auto narrow = PlutoBrand::CalculateLayout({0, 0, 420, 60}, {38, 38}, {72, 22}, {42, 14});
assert(!narrow.show_version);
assert(narrow.logo.min_x > 300.0f);
```

- [ ] **Step 2: Build and verify RED**

Expected: missing PlutoBrand implementation and v2.5.0 assertion failure.

- [ ] **Step 3: Generate deterministic embedded assets**

`UpdatePlutoBrand.ps1` verifies the source files, converts the PNG to a contained 256×256 RGBA PNG, generates a byte-array header, fetches a pinned OFL font source only when absent, records its license, subsets/embeds the `Pluto`/version glyphs, and emits stable headers. Re-running must produce identical SHA-256 values.

- [ ] **Step 4: Implement the right-aligned header lockup**

Initialize the logo SRV and wordmark font in `MenuShell`; shut down the SRV before D3D teardown. Call only `PlutoBrand::RenderHeaderLockup` from the working Starline adapter after the four existing tabs. Use a 38 px controlled logo square, wordmark, and subdued SemVer label. Hide only the version label when width is insufficient; never overlap tabs.

- [ ] **Step 5: Embed executable metadata, icon, and elevation**

`Pluto.rc` defines the supplied ICO and VERSIONINFO strings (`Pluto`, `Pluto portable overlay`, `2.5.0`). Set `UACExecutionLevel` to `RequireAdministrator` for x64 Debug and Release. Load `IDI_PLUTO_ICON` into `WNDCLASSEX::hIcon` and `hIconSm`; rename the window class/title to stable Pluto names.

- [ ] **Step 6: Rename visible TokyoZK/external-ESP branding**

Rename the watermark entry point to `RenderPlutoWatermark`, render `Pluto`, and update startup/menu/updater logs and portable output names. Preserve upstream license comments and internal technical namespaces where renaming would create unrelated churn.

- [ ] **Step 7: Build and run GREEN plus ImGui checker**

Run brand/version tests and `check_imgui_cpp.py` over the adapter and brand renderer. Inspect generated asset hashes twice, and verify `mt.exe`/PowerShell resource inspection finds `requireAdministrator` and the Pluto icon/version strings.

- [ ] **Step 8: Commit Task 5 files only**

```powershell
git add assets/UpdatePlutoBrand.ps1 src/assets src/gui/frontend/brand src/gui/frontend/menu/MenuShell.cpp src/gui/renderer src/resource.h src/Pluto.rc src/src.vcxproj src/src.vcxproj.filters src/core/version/AppVersion.hpp "starline-imgui-menu-GOOD FOR CS 2/external_esp_menu.cpp" "starline-imgui-menu-GOOD FOR CS 2/external_esp_menu.h" tests/OverlayPresentationTests.cpp tests/PlutoBrandTests.cpp tests/PlutoBrandTests.vcxproj
git commit -m "feat: apply the Pluto visual identity"
```

### Task 6: Release-only packaging and GitHub publication contract

**Files:**
- Modify: `tools/BuildPortable.ps1`
- Modify: `tests/Test-PortableBuild.ps1`
- Create: `tests/Test-PlutoReleaseContract.ps1`
- Rewrite: `.github/workflows/release.yml`
- Modify: `.github/workflows/auto_build.yml`
- Modify: `README.md`

**Interfaces:**
- Consumes: `AppVersion.hpp`, `Pluto-portable.exe`, and all test projects.
- Produces: a tag-triggered GitHub Release whose exact asset and digest are discoverable by the updater.

- [ ] **Step 1: Write the failing release-contract test**

The PowerShell test parses the version header and workflows, then asserts:

```powershell
$version | Should-Be '2.5.0'
$releaseAsset | Should-Be 'Pluto-portable.exe'
$latestEndpoint | Should-Be 'https://api.github.com/repos/VPROJECT-max/Pluto-CS-2/releases/latest'
$releaseWorkflow | Should-Match "tags:\s*- 'v\*\.\*\.\*'"
$releaseWorkflow | Should-Match 'gh release create'
$project | Should-Match '<UACExecutionLevel>RequireAdministrator</UACExecutionLevel>'
```

Use ordinary explicit assertion functions rather than adding a PowerShell package dependency.

- [ ] **Step 2: Run and verify RED**

Expected: old portable name, old workflow tag scheme, and missing Pluto contract fail.

- [ ] **Step 3: Update the local portable builder**

Build x64 Release only, copy atomically to `dist\Pluto-portable.exe`, run dependency/resource checks, print SHA-256, and reject any Debug input. Preserve the one-file output.

- [ ] **Step 4: Replace release automation**

Trigger on pushed tags `v*.*.*`; checkout submodules; set up MSBuild; compare the tag against `AppVersion.hpp`; build tests and the x64 Release; run all test executables and PowerShell contracts; call `tools\BuildPortable.ps1`; publish with:

```powershell
gh release create $env:GITHUB_REF_NAME `
  'dist/Pluto-portable.exe' `
  --verify-tag --generate-notes --title "Pluto $env:GITHUB_REF_NAME"
```

Set workflow `permissions: contents: write`. The normal main workflow builds/tests only and does not release.

- [ ] **Step 5: Run GREEN**

Run the release contract and portable dependency tests locally. Parse both workflow files as YAML when a parser is available and otherwise validate their structural contracts textually.

- [ ] **Step 6: Commit Task 6 files only**

```powershell
git add tools/BuildPortable.ps1 tests/Test-PortableBuild.ps1 tests/Test-PlutoReleaseContract.ps1 .github/workflows/release.yml .github/workflows/auto_build.yml README.md
git commit -m "ci: publish versioned Pluto portable releases"
```

### Task 7: Full verification and first-release readiness

**Files:**
- Modify only files required to fix failures found by the gates.
- Do not create a real GitHub Release until every local gate passes.

**Interfaces:**
- Produces: verified `x64\Debug\cs2-external-esp.exe`, `x64\Release\cs2-external-esp.exe`, and `dist\Pluto-portable.exe`.

- [ ] **Step 1: Re-run deterministic asset generation**

Run `assets\UpdateEspPreview.ps1` and `assets\UpdatePlutoBrand.ps1` twice; compare header SHA-256 values across runs.

- [ ] **Step 2: Build every test project and execute every test**

Build/run `UpdaterModelTests`, `UpdaterIoTests`, `SelfReplaceTests`, `PlutoBrandTests`, `OverlayPresentationTests`, `LocalAccountTests`, `ConfigDocumentTests`, and `GeometryTests`. Run offset synchronization, Starline integration, UI contracts, Pluto release contract, and portable dependency scripts.

- [ ] **Step 3: Build Debug and Release**

Use MSBuild Rebuild x64 Debug and x64 Release. Record warnings and fail on every error. Confirm Debug skips updater policy; do not run the actual overlay/game integration during automated verification.

- [ ] **Step 4: Build the portable and inspect resources**

Run `tools\BuildPortable.ps1`. Confirm Release and portable SHA-256 equality, the static dependency contract, `requireAdministrator`, Pluto icon group, Pluto VERSIONINFO, and exact output name.

- [ ] **Step 5: Run UI static checks and source scans**

Run the ImGui balance checker on all changed ImGui files. Search product-facing source and generated resources for `TokyoZK`, legacy updater URLs, legacy portable names, dummy update data, and mismatched versions. Preserve required upstream license/attribution strings.

- [ ] **Step 6: Audit repository contents before public push**

List all staged/untracked content, exclude build products, local accounts, logs, temp files, IDE state, and machine-specific data, and scan tracked text for tokens/private keys. Include all source/assets/tests needed for a clean GitHub Actions checkout to build the verified artifact.

- [ ] **Step 7: Create a cohesive source commit without destroying existing work**

Stage the verified implementation and required earlier uncommitted source/assets deliberately. Review `git diff --cached --stat` and `git diff --cached` before committing. Do not reset, clean, or overwrite unrelated user files.

- [ ] **Step 8: Publish source and first release when authenticated**

Push `main`, push tag `v2.5.0`, then watch the GitHub Actions release job. Confirm the public latest-release API returns tag `v2.5.0`, asset `Pluto-portable.exe`, expected size, and non-empty `sha256:` digest. If credentials are unavailable, stop after producing the verified local artifact and exact push/tag commands; never embed a token.

- [ ] **Step 9: Report evidence**

Report build/test outcomes, artifact paths and hashes, whether publication occurred, and the important limitation that the in-game visual result was not manually exercised unless it actually was.
