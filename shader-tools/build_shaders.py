"""Build only two modified hair families from separately supplied game data."""
import argparse
import hashlib
import json
import os
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input-dir', type=Path, required=True)
    parser.add_argument('--dxc-dir', type=Path, required=True, help='Directory containing dxc.exe, dxcompiler.dll and dxil.dll')
    parser.add_argument('--output-dir', type=Path, default=Path('shader-output'))
    args = parser.parse_args()
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    os.environ['HG_DXC_DIR'] = str(args.dxc_dir.resolve())
    os.environ['HG_SHADER_WORK'] = str(output / 'work')
    from soulstruct.containers import Binder
    from shader_patch import patch_bundle
    from shader_payload import pack, unpack
    eye = Binder.from_path(args.input_dir / 'P[ChrCustomize][EyePatch].shaderbdle')
    report = []
    for stem, filename in (('P[ChrCustomize][Hair]', 'hair.hgz'), ('P[ChrCustomize][Hair]_cloth', 'hair_cloth.hgz')):
        source = args.input_dir / (stem + '.shaderbdle')
        bundle = Binder.from_path(source)
        counts = patch_bundle(bundle, eye)
        raw = bytes(bundle)
        assert [e.data for e in Binder.from_bytes(raw).entries] == [e.data for e in bundle.entries]
        payload = pack(raw)
        assert unpack(payload) == raw
        (output / filename).write_bytes(payload)
        report.append(dict(file=filename, counts=counts, input_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                           sha256=hashlib.sha256(payload).hexdigest(), decoded_sha256=hashlib.sha256(raw).hexdigest()))
    (output / 'shader-build-report.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
