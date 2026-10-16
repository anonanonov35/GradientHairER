# Hair Gradient 0.3.2

By **AnonAnonov**. Windows x64 cosmetic mod for Elden Ring, loaded by ModEngine 2. Adds a second hair color, gradient softness and gradient position to the appearance editor. Settings are stored per character through the game's normal appearance save path.

This is the public source project for GitHub and support review. Extract this archive and upload its contents as the repository root. No game installation is needed to compile the DLL or run the native tests. The separate `HairGradient-0.3.2.zip` is the playable release with BAT, TOML and two modified shader payloads.

## Build and test

Requirements: Windows x64, PowerShell and **Zig 0.13.0** installed separately. MinHook source and its license are vendored under `third_party/minhook`, pinned to commit `8af6b4acae5a9388fd742b56fa79ece89d96f823` from https://github.com/TsudaKageyu/minhook.

```powershell
./build.ps1 -ZigExe "path/to/zig.exe" -Test
```

Output: `out/hair_gradient.dll`. `-Test` also builds and runs six native tests covering settings/storage, legacy menu-row construction, runtime menu/captions, TOML detection, automatic appearance-hook selection and real MinHook chaining on synthetic functions. Tests do not start or modify the game. The legacy menu test remains as development history; the release uses runtime menu injection.

Use the separate mod release for installation instructions. To try a rebuilt DLL, close the game and replace only `mod/HairGradient/hair_gradient.dll` in that release; retain its two shader payloads. Rebuilt files need fresh verification. Binary-identical compiler output across different paths/environments is not claimed. Original release SHA256 hashes are in `RELEASE_HASHES.json`.

## Review map

| Path | Purpose |
| --- | --- |
| `src/runtime.cpp` | DLL lifecycle, bounded process-memory access, appearance hooks and shader registration |
| `src/settings.h` | Character storage layout and packed color/gradient controls |
| `src/runtime_menu.h`, `src/menu_patch.h` | Runtime menu copies and captions |
| `src/config_detection.h` | Read-only parsing of the active ModEngine configuration |
| `src/uma_compat.h`, `src/hook_route.h` | Standalone/UmaEars selection and cooperative hook routing |
| `src/*guards.h`, `src/hair_bounds.h` | Interface addresses/checksums and measured bounds for 36 stock hairstyles |
| `shader-tools/` | Original transformations used to modify the user's own shader bundles |
| `distribution/` | Portable BAT, TOML and INI shipped in the mod release |
| `tests/` | Native tests, without game code or game fixtures |

## Runtime behavior and limits

The DLL reads its INI and the configuration identified by `MODENGINE_CONFIG`. It writes a diagnostic log beside the DLL. It hooks only the current game process through MinHook, supplies a presentation copy of menu rows, and replaces initial registration of the Hair/Hair_cloth shader families. The second color and two sliders occupy nine reserved appearance bytes. It does not write the game executable, `regulation.bin` or message containers. It does not download files, install services or send telemetry. Source and dependencies can be inspected before use.

Use the existing offline ModEngine 2 route. Gameplay baseline: installed Elden Ring 1.17. Standalone controls/persistence and a cooperative UmaEars baseline were user-confirmed. The final automatic-startup DLL from 0.3.1 was native-tested, without a new complete gameplay test. Version 0.3.2 retains that DLL byte-for-byte and changes packaging/configuration only. 1.17.1, Convergence and rendering on downloaded custom hairstyles remain unverified. There is no executable version-number gate; nine game-interface checks and three conditional UmaEars handler checks remain.

## Shader source and game resources

This repository intentionally contains no game-derived shader binaries, original game archives, executable bytes, decompiled game source, saves or third-party hairstyle models. The readable transformation code is supplied in `shader-tools`; see its README for rebuilding from your own three extracted uncompressed `.shaderbdle` inputs. The private extraction tools and original game data are not needed for the DLL build.

The separate playable release contains only two modified game-derived shader bundles. Rights to underlying game assets remain with their owners. Do not add extracted game data or build output to this source repository; `.gitignore` covers the usual output paths.

## License and credits

Original code, launcher and documentation: MIT, copyright 2026 AnonAnonov. See `LICENSE`. MinHook and Zig-linked components retain their notices in `third_party/minhook/LICENSE.txt` and `licenses/`. See `CREDITS.md` for development references. Created with OpenAI Codex (GPT-6) assistance; no new AI-generated artwork or audio. Unofficial fan mod, not affiliated with FromSoftware or Bandai Namco.

Support reviewers can compare `RELEASE_HASHES.json` with the playable ZIP, inspect the source, build the DLL and run the included tests. Automated checks do not constitute an antivirus certification or a guarantee of compatibility with another game's build or another mod version.
