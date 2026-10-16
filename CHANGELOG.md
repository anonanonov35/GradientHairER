# Changelog

## 0.3.2 — 2026-10-16

- Enabled the optional `mod` loose-file directory in the standalone TOML, so downloaded hairstyles installed in `mod/parts` can load through the included BAT.
- Added custom-hairstyle installation guidance; hairstyle assets are not bundled and their rendering is not yet verified.
- Added a separate GitHub-ready source archive with build instructions, native tests, shader transformation code and release hashes for support review.
- DLL and both shader payloads remain byte-identical to 0.3.1. No gameplay code was rebuilt or changed in this packaging release.

## 0.3.1 — 2026-10-15

- Fixed the appearance-hook conflict that prevented UmaEars from applying ears when Hair Gradient loaded first.
- Preserves UmaEars handlers and engine hooks; compatibility accepted by the user with ERSC and Freecam DLLs loaded.
- Adds automatic standalone/UmaEars startup selection through hair-gradient.ini (uma_ears=-1).
- Checks the active ModEngine external_dlls array, allowing UmaEars to load after slow DLLs; comments and disabled examples are ignored.
- Standalone, delayed/preloaded UmaEars, timeout and incompatible-handler paths passed native tests. No new full gameplay retest of the final auto-detection DLL.
- Retains both accepted shader payloads, native editor controls, storage format and the UmaEars-style package layout.

## 0.3.0 — 2026-10-15

- Removed regulation.bin and both message-container overrides.
- Added native editor controls through a presentation copy of loaded menu rows; original rows remain intact and no reserved blank rows are consumed.
- Supplies Russian/English captions and help text directly from the DLL.
- All controls, finishing the editor and persistence after a game restart confirmed by the user on 1.17.
- Packages the exact accepted DLL and two HGZ shader payloads, without rebuilding gameplay code.
- UmaEars-style layout: mod/HairGradient, root config_hair_gradient.toml and launch_hair_gradient.bat, relative paths, no installer.
- Uses the existing ModEngine 2 directly from BAT; no generated configuration, runtime PowerShell or automatic save backups. Back up saves manually.
- Nine interface guards remain; no executable version metadata gate.

## 0.2.3 — 2026-10-15

- Removed version metadata gates from both the portable launcher and native DLL.
- Different PE timestamps and image sizes no longer block startup by themselves.
- Kept native function-interface validation and bounded reads before hooking.
- Kept all five resource files from the accepted compact baseline.
- App 1.17 remains the gameplay-tested baseline; 1.17.1 is unverified.

## 0.2.2 — 2026-10-15

- First publication package by AnonAnonov.
- Native appearance-editor controls for second hair color, softness and position.
- Compact shader replacement at initial Hair/Hair_cloth registration.
- Restored gradient rendering after the unsuccessful sparse-overlay prototype.
- Portable BAT launch, automatic path discovery, compatibility/integrity checks and separate save backups.
- English/Russian instructions, runtime source, licenses and checksums.

In 0.2.2 the runtime DLL and game resources were unchanged from the accepted compact build. Earlier local experiments are not included.
