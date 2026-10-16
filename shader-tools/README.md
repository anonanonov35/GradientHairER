# Shader transformations

These are the original Python transformations used for the two gradient hair shader families. Original compiled game shaders, DXIL disassembly and game assets are not included. Input extraction is a separate operation against your own game data.

Requirements: Windows x64, Python 3.11 or newer, `soulstruct==2.3.2`, and Microsoft DirectXShaderCompiler's x64 `dxc.exe`, `dxcompiler.dll` and `dxil.dll` from https://github.com/microsoft/DirectXShaderCompiler. The DLL build does not require Python, Soulstruct or DXC.

Place three original uncompressed shader bundles extracted from your own game in a private directory outside the public repository:

```text
P[ChrCustomize][Hair].shaderbdle
P[ChrCustomize][Hair]_cloth.shaderbdle
P[ChrCustomize][EyePatch].shaderbdle
```

From the repository root:

```powershell
python -m pip install soulstruct==2.3.2
python shader-tools/build_shaders.py --input-dir "path/to/private-inputs" --dxc-dir "path/to/dxc/bin/x64" --output-dir shader-output
```

The script writes `hair.hgz`, `hair_cloth.hgz`, a checksum report and private working DXIL files to the output directory. HGZ is an HGZ1 header plus Windows LZNT1-compressed modified BND4 content. A full compress/decompress round trip is verified.

`shader_patch.py` adds original model-space Y through TEXCOORD15/register 12, reads two colors and packed bounds/controls from material constants, blends them before native hair shading, and expands material metadata using an existing color descriptor. `dxil.py` uses Microsoft's assembler and validator. Only Hair and Hair_cloth families are rebuilt; no game-wide shader archive is emitted.

Shader transformations are input- and compiler-sensitive. The published payloads were built from the installed 1.17 game inputs. The public script is checked against those local inputs during packaging; this does not imply support for a different game build. Never commit the inputs, generated disassembly or generated bundles. Copyright in underlying game resources remains with its owners.
