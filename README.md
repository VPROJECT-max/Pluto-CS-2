# Pluto

Pluto is a Windows x64 external overlay for Counter-Strike 2 with a clean AMOLED-style Starline interface, live ESP preview, radar, spectator and bomb panels, local-only account gate, generated offsets, and a verified one-file updater.

## Use

1. Download `Pluto-portable.exe` from this repository’s [Releases](https://github.com/VPROJECT-max/Pluto-CS-2/releases).
2. Start Counter-Strike 2 in borderless/windowed fullscreen.
3. Double-click `Pluto-portable.exe` and accept the administrator prompt.
4. Use Insert or Right Shift to open and close the menu. End closes Pluto.

The portable is self-contained—there are no packages or Visual C++ redistributables to install manually. It checks this public repository’s latest stable GitHub Release at startup. A newer exact `Pluto-portable.exe` asset is downloaded to `%LOCALAPPDATA%\Pluto\Updates`, checked against GitHub’s declared size and SHA-256 digest, then installed with rollback protection. Debug and renamed development executables never self-update.

Local account data remains on the current PC and is protected with Windows DPAPI. It is not uploaded to GitHub or a remote account service.

## Build

Requirements: Windows x64 and Visual Studio 2022 with Desktop development with C++.

```powershell
git clone --recursive https://github.com/VPROJECT-max/Pluto-CS-2.git
cd Pluto-CS-2
powershell -NoProfile -ExecutionPolicy Bypass -File tools\BuildPortable.ps1
```

The verified artifact is written to `dist\Pluto-portable.exe`. The builder accepts x64 Release only and validates static runtime dependencies, architecture, version metadata, icon, and the `requireAdministrator` manifest.

Stable releases use SemVer tags such as `v2.5.0`. The tag must match `src/core/version/AppVersion.hpp`; GitHub Actions runs every test before publishing the exact portable asset consumed by the updater.

## Important

This project is provided as-is for personal, educational, and non-commercial use. Software that reads another process’s memory may trigger anti-virus or anti-cheat products. Review and build the source yourself when you need maximum assurance. You are responsible for how and where you use it.

## Credits and license

Pluto is derived from the `cs2-external-esp` project by [IMXNOOBX](https://github.com/IMXNOOBX/cs2-external-esp) and contributors, with Starline menu integration and additional Pluto-specific work. Offset data tooling credits [a2x/cs2-dumper](https://github.com/a2x/cs2-dumper). The supplied Pluto visual assets remain attributed to their provider; Karla is used under the SIL Open Font License 1.1.

The project remains licensed under [CC BY-NC 4.0](LICENSE). Preserve upstream attribution and do not sell or sublicense the original or modified work.
