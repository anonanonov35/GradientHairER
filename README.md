# Hair Gradient — source 0.3.3

Independent source project for GitHub and DLL review. Unpack this archive and upload its contents as the repository root. This is source code, not an installable mod archive.

Adds a second hair color, gradient position and softness to the appearance editor. Works standalone or cooperatively with Uma through UmaAppearanceInterfaceV1, with a guarded legacy fallback. Uma runtime source or DLL is not required to build this project.

## Build and test

Windows x64, PowerShell and **Zig 0.13.0** are required. MinHook is vendored at commit `8af6b4acae5a9388fd742b56fa79ece89d96f823`. No game installation, other mod, private build directory or network download is needed for the DLL build and native tests.

```powershell
./build.ps1 -ZigExe "C:/Tools/zig/zig.exe" -Test
```

Output: `out/hair_gradient.dll`. Tests use synthetic data and project-authored solver fixtures; they do not launch or modify the game. Build scripts restore previous cache variables and use their own repository directory, including when invoked from another directory.

## Review map

- `src/`: complete project-specific DLL translation units and headers.
- `compat/eldenring_1_17/`: numeric interface addresses, checksums and measured compatibility metadata; no game executable instructions or decompiled game functions.
- `third_party/minhook/`, `licenses/`: original dependency source and notices.
- `tests/`: native tests and synthetic/project-authored fixtures.
- `distribution/`: default INI and portable loader examples, without loaders or DLLs.
- `release-manifest.json`: released DLL identity, compiler settings and local rebuild results.
- `resources-manifest.json`: names, sizes and SHA-256 of separate playable-release files; no resource contents.
- `verify.py`, `SHA256SUMS.txt`: verify the exported source and optionally compare a release or rebuild.

`shader-tools/` contains the original Python transformations and portable build wrapper for the two gradient shader families. Their README describes separate private inputs and dependencies. The released HGZ binaries are inventoried but are not included here.

```powershell
python verify.py
python verify.py --dll "path/to/released/hair_gradient.dll"
python verify.py --rebuilt "out/hair_gradient.dll"
python verify.py --mod-root "path/to/extracted/playable-release"
```

Python is optional for inspection and uses only its standard library. Rebuild verification compares the entry point and six functional PE sections. Full-file identity is reported separately: linker timestamps, debug identifiers and debug paths can differ. See `release-manifest.json` for the actual result, not a claim of full-file reproduction. A rebuild does not automatically inherit previous gameplay verification.

## Scope and behavior

The runtime is designed for the existing offline Elden Ring mod-loader route. Development baseline: Elden Ring 1.17 on Windows x64. It installs in-process hooks, supplies its own menu presentation and updates character appearance. It reads an adjacent INI and writes a diagnostic log. It contains no network client, telemetry uploader, downloader, service installer or unrelated-process injection. Review the source for the complete behavior. This export does not certify compatibility with arbitrary game updates or global mods.

Models, textures, game-derived shader binaries, game archives, saves, binary DLLs, original game code, extracted disassembly and private development history are excluded. The DLL source project is complete within this scope; asset conversion/recreation is a separate task. No repository is created or uploaded automatically.

## Credits and licensing

Direction and game testing: AnonAnonov. Implementation assistance: OpenAI Codex (GPT-6 family). No artwork or audio was generated for this export. MinHook: Tsuda Kageyu and contributors, including HDE by Vyacheslav Patkov. Zig is a separate build dependency. ModEngine2/ME3 are separate loaders. Underlying game content remains with its respective owners.

Original project code is MIT-licensed by AnonAnonov; see LICENSE. Third-party dependencies retain their notices. See `THIRD_PARTY.md` and `RESOURCES.md`. Publishing source does not grant rights to the separately distributed models or game resources.
