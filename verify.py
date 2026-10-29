"""Check source/resource hashes and inspect PE sections without executing a DLL."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CORE = ('.text', '.rdata', '.data', '.pdata', '.tls', '.reloc')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def pe_info(path):
    data = path.read_bytes()
    assert data[:2] == b'MZ'
    at = struct.unpack_from('<I', data, 60)[0]
    assert data[at:at + 4] == b'PE\0\0'
    machine, count = struct.unpack_from('<HH', data, at + 4)
    optional = at + 24
    assert machine == 0x8664 and struct.unpack_from('<H', data, optional)[0] == 0x20b
    table = optional + struct.unpack_from('<H', data, at + 20)[0]
    sections = {}
    for i in range(count):
        row = table + i * 40
        name = data[row:row + 8].split(b'\0')[0].decode('ascii')
        virtual_size, rva, size, offset = struct.unpack_from('<4I', data, row + 8)
        assert offset + size <= len(data)
        sections[name] = dict(rva=rva, virtual_size=virtual_size, raw_size=size,
                              sha256=sha(data[offset:offset + size]))
    return dict(sha256=sha(data), entry_point=struct.unpack_from('<I', data, optional + 16)[0],
                sections=sections)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--dll', type=Path, help='Original released DLL')
    parser.add_argument('--rebuilt', type=Path, help='Rebuilt DLL; compare functional PE sections')
    parser.add_argument('--mod-root', type=Path, help='Root of the separate extracted playable release')
    args = parser.parse_args()
    count = 0
    for line in (ROOT / 'SHA256SUMS.txt').read_text('utf-8').splitlines():
        expected, name = line.split('  ', 1)
        path = (ROOT / name).resolve()
        assert path.is_relative_to(ROOT) and sha(path.read_bytes()) == expected, name
        count += 1
    manifest = json.loads((ROOT / 'release-manifest.json').read_text('utf-8'))
    result = dict(source_files_verified=count)
    reference = manifest['released_runtime']
    if args.dll:
        assert sha(args.dll.read_bytes()) == reference['sha256'], 'Released DLL hash mismatch'
        result['released_dll_exact'] = True
    if args.rebuilt:
        actual = pe_info(args.rebuilt)
        exact = {name: actual['sections'].get(name) == reference['pe']['sections'].get(name)
                 for name in CORE}
        result['rebuilt_core_sections_exact'] = exact
        result['rebuilt_entry_point_exact'] = actual['entry_point'] == reference['pe']['entry_point']
        result['rebuilt_full_file_exact'] = actual['sha256'] == reference['sha256']
        assert all(exact.values()) and result['rebuilt_entry_point_exact'], 'Rebuilt functional code/data differs'
    if args.mod_root:
        resources = json.loads((ROOT / 'resources-manifest.json').read_text('utf-8'))
        base = args.mod_root.resolve()
        for entry in resources['files']:
            path = (base / entry['path']).resolve()
            assert path.is_relative_to(base) and path.stat().st_size == entry['bytes']
            assert sha(path.read_bytes()) == entry['sha256'], entry['path']
        result['resource_files_verified'] = len(resources['files'])
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
