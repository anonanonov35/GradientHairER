# Credits

- **AnonAnonov** — mod author, direction and manual in-game testing.
- **OpenAI Codex (GPT-6)** — AI assistance with implementation, reverse engineering, verification and packaging. No new generated art/audio assets.
- [MinHook](https://github.com/TsudaKageyu/minhook), Tsuda Kageyu; HDE portions by Vyacheslav Patkov — native hooks. Full binary redistribution notice: `MinHook-LICENSE.txt`.
- [ModEngine 2](https://github.com/soulsmods/ModEngine2) — external runtime loader. Download separately; not bundled.
- [Soulstruct](https://github.com/Grimrukh/soulstruct) — development-time container and parameter tooling; not bundled.
- [SoulsFormatsNEXT](https://github.com/soulsmods/SoulsFormatsNEXT) — format references during development; not bundled.
- [Microsoft DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler) — development-time DXIL assembly/validation; not bundled.
- [Zig](https://ziglang.org/) — development-time native compiler; not bundled.
- [elden-x](https://github.com/ThomasJClark/elden-x) and [elden-ring-transmog](https://github.com/ThomasJClark/elden-ring-transmog) — public development references for native parameter/message interfaces; not bundled.
- FromSoftware / Bandai Namco — Elden Ring and underlying game resources. This is an unofficial fan mod.

The separate playable release contains the 0.3.1 runtime unchanged, two modified hair shader bundles, native runtime-menu source, documentation and licenses. Its accepted gameplay baseline and later automatic-startup native tests are distinguished in the README. It contains no regulation.bin, message containers, game executables, loaders, saves, original model/texture archives, extracted stock resources or development toolchains. The two modified shader payloads are game-derived mod resources; rights to underlying game assets remain with their owners.

Zig-linked library notices are included in `licenses/`: libc++, libc++abi, libunwind and MinGW. These components retain their respective license terms.
