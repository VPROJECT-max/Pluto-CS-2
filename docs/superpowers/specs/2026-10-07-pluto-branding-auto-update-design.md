# Pluto Branding and Automatic Update Design

## Status

Approved in chat on 2026-10-07. This design includes the later requirement that the executable request administrator privileges at launch.

## Intent and success criteria

Pluto is a personal-use Windows desktop overlay. The user needs a clean, cohesive identity and a portable executable that keeps itself current without installing packages or requiring manual file replacement.

The change succeeds when:

- the menu presents the supplied planet mark and the name `Pluto` opposite the main tabs;
- the executable and Windows window class use the supplied `.ico` asset;
- visible legacy product names are replaced by `Pluto` while upstream license attribution remains intact;
- the product version advances from `2.4.5` to `2.5.0` and remains the single source of SemVer truth;
- a Release portable executable can discover, verify, install, and relaunch a newer GitHub Release by itself;
- update failure never blocks the installed version from launching;
- a GitHub Actions release workflow builds and publishes the exact asset expected by the client;
- the executable requests elevation through a Windows manifest; and
- Debug and local development builds do not accidentally replace themselves.

## Constraints

- Target platform: 64-bit Windows.
- UI framework: the existing Dear ImGui and Starline adapter.
- Distribution: one statically linked Release executable with embedded imagery and fonts.
- Update host: the public `VPROJECT-max/Pluto-CS-2` GitHub repository.
- Release asset name: `Pluto-portable.exe`.
- Release tags: strict `vMAJOR.MINOR.PATCH`, beginning with `v2.5.0`.
- The updater must not contain a GitHub token or other reusable credential.
- Existing exact Starline control implementations remain the source for tabs, toggles, sliders, buttons, and grouped panels.
- Source changes in the existing dirty working tree must be preserved.

## Visual identity

### Header composition

The existing main tabs retain their current position and behavior. A compact brand lockup sits on the opposite side of the header:

- a 34-40 px rendering of the supplied `Minimalist Planet P Logo.png`;
- the word `Pluto` set in an embedded, redistributable geometric display face;
- a quiet `v2.5.0` secondary label;
- no new tab, card, glow panel, or decorative status data.

The mark is detailed, so it is rendered inside a controlled square and not reduced to favicon scale inside the menu. A subtle edge treatment prevents its dark background from bleeding into the header. The wordmark uses the existing restrained near-black/graphite palette with the logo's pale violet reserved as an accent.

### Embedded assets

The supplied PNG is converted into a deterministic embedded image header and loaded through the existing D3D11 image loader. A small OFL-licensed display font is embedded into the ImGui atlas for the wordmark only; existing body and control typography remains unchanged.

The supplied `Minimalist Planet P Logo.ico` is compiled into the PE resources. The same icon is assigned to both large and small window-class icon slots, even though the overlay normally hides its taskbar entry.

### Product naming

Visible startup logs, updater messages, menu branding, watermark branding, executable description, and portable filename use `Pluto`. Copyright/license notices continue to attribute the upstream project and are not removed.

## Version model

`src/core/version/AppVersion.hpp` remains the only product-version definition and becomes `2.5.0`. A tested SemVer value type parses release tags and compares major, minor, and patch numerically. Pre-release tags are ignored for automatic stable updates in this version.

The updater accepts only a tag that exactly matches `v<major>.<minor>.<patch>`. Invalid, older, equal, draft, and prerelease releases cannot trigger replacement.

## Update architecture

### Normal startup

1. Parse internal maintenance arguments before logger, engine, or renderer initialization.
2. In a Release portable run, request the latest public GitHub Release from `https://api.github.com/repos/VPROJECT-max/Pluto-CS-2/releases/latest`.
3. Set explicit GitHub API accept/version headers, a Pluto user agent, redirect handling, timeouts, TLS verification, and a response-size limit.
4. Parse the strict SemVer tag and find the exact `Pluto-portable.exe` asset.
5. If the release is newer, download it to `%LOCALAPPDATA%\Pluto\Updates\<version>\Pluto-portable.exe` through a temporary `.part` file.
6. Confirm the declared asset size and the GitHub `sha256:` digest before renaming the partial download into the staging path.
7. Start the verified staging executable in apply mode, pass the original executable path and parent process ID, then exit normally.
8. If there is no valid update—or any check fails—log the reason and continue launching the current version.

Only an executable named `Pluto-portable.exe` performs automatic checks. Debug builds and differently named development outputs log that self-update is skipped. A `--skip-update` maintenance switch provides a recovery path.

### Self-replacement

The updater is implemented inside Pluto; there is no second installed updater.

1. The staging copy starts with an internal `--apply-update` mode and waits for the original parent process to exit.
2. It moves the old executable to a sibling rollback file, copies the verified staging image to the original path, and starts the new original-path executable in cleanup mode.
3. If replacement or relaunch fails, it restores the rollback file and reports the Windows error.
4. The relaunched original deletes the staging image and rollback file when possible. Locked leftovers are scheduled for deletion on reboot.

All update paths are canonicalized and validated. The apply process may modify only the exact current executable, its exact rollback sibling, and a staging file beneath `%LOCALAPPDATA%\Pluto\Updates`. Arbitrary caller-supplied paths are rejected.

### Elevation

The executable embeds a `requestedExecutionLevel level="requireAdministrator"` application manifest. Windows therefore displays the normal UAC prompt before both ordinary use and replacement. The update flow does not disable UAC, store credentials, create services, or add startup persistence.

## Networking and integrity

The existing HTTP helper is separated into JSON retrieval and streamed file download. Downloads write directly to a temporary file rather than accumulating an executable in memory. Curl error text, HTTP status, timeouts, maximum metadata size, file-write errors, redirect behavior, and final URL are surfaced as structured results.

Integrity uses the SHA-256 digest returned with the GitHub release asset and Windows CNG for local hashing. HTTPS verification remains enabled. A missing or malformed digest is a hard update rejection, not a reason to install without verification.

The public repository removes the need for an embedded token. GitHub documents that public release resources can be read without authentication and exposes an asset `digest` field for this purpose.

## Release automation

The existing manual legacy release workflow is replaced with a Pluto workflow triggered only by tags matching `v*.*.*`.

The workflow:

1. checks out the repository with submodules;
2. validates that the tag equals the version in `AppVersion.hpp`;
3. builds the x64 Release configuration;
4. runs all project tests and portable-dependency checks;
5. invokes the Release-only portable builder;
6. publishes `dist/Pluto-portable.exe` to a GitHub Release with generated notes.

Normal pushes build and test but do not publish an update. Publishing is an intentional two-step operation: commit/push the versioned change, then push the matching SemVer tag.

## UI states and messaging

Update work happens before the renderer exists, so status uses concise logs and only uses a native error dialog when a downloaded update cannot be applied. Expected offline/no-release states remain quiet enough not to interrupt use.

Messages distinguish:

- current version;
- checking;
- no newer release;
- downloading with real byte counts;
- verification failure;
- applying and relaunching;
- rollback completed; and
- update skipped for a development build.

No fake progress values or dummy release data are shown.

## Error handling and recovery

- Network unavailable, HTTP failure, rate limiting, or invalid JSON: log and continue current version.
- No Releases yet: log once and continue current version.
- Invalid SemVer, wrong asset name, missing digest, size mismatch, or hash mismatch: reject and delete the partial file.
- Download interruption: retain no trusted staging executable; a later launch starts a fresh `.part` download.
- Cannot write beside the portable executable: keep the original untouched and report the exact path/error.
- Replacement failure after backup: restore the backup before exiting apply mode.
- Relaunch failure: preserve a usable original and report the failure.
- Cleanup failure: schedule only validated staging/backup paths for deletion on reboot.

## Testing strategy

### Unit tests

- strict SemVer parsing and ordering, including malformed/overflow input;
- release JSON selection, including missing, duplicate, wrong-name, draft, prerelease, and missing-digest assets;
- update policy for Debug, renamed development builds, equal/older/newer versions, and `--skip-update`;
- SHA-256 formatting and mismatch rejection;
- validated staging, original, and rollback path relationships;
- command-line maintenance-mode parsing;
- brand layout math for narrow and normal header widths.

### Integration tests

- local HTTP fixture for redirects, timeouts, HTTP errors, oversized JSON, partial executable downloads, declared-size mismatch, and valid download;
- replacement harness using disposable temporary executables and directories, including rollback after a forced copy/relaunch failure;
- PE checks for x64 Release static runtime, administrator manifest, icon resource, and portable dependency closure;
- source contracts that the GitHub owner/repository, asset name, product version, and workflow tag/version checks agree;
- Dear ImGui static balance checker over modified UI code.

### Release verification

Build Debug and Release, run the complete existing regression suite plus new updater/branding tests, generate the portable artifact, verify Release and portable hashes match, and inspect the PE resources. A live GitHub check may verify public metadata, but tests do not publish or replace a real executable.

## Rollout

The repository is public but currently has no pushed content or Releases. After implementation and local verification, publish the source to `main`, then create the first `v2.5.0` release through the workflow. Existing pre-updater copies will not update themselves; users receive `Pluto-portable.exe` once manually. Every later tagged release can update those copies automatically.

## Non-goals

- Background services, scheduled tasks, registry startup entries, or a machine-wide installer.
- Updating Debug builds or arbitrary renamed executables.
- Private-repository authentication or embedded GitHub credentials.
- Delta/binary-patch downloads.
- Multiple update channels or prerelease opt-in.
- Automatic publication on every push to `main`.

