"""Portable shader build paths; no game installation or private paths embedded."""
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
WORKSPACE = ROOT
LAB = Path(os.environ.get('HG_SHADER_WORK', str(ROOT / 'shader-output' / 'work'))).resolve()
DXC_DIR = Path(os.environ.get('HG_DXC_DIR', str(ROOT / '.tools' / 'dxc' / 'bin' / 'x64'))).resolve()
LAB.mkdir(parents=True, exist_ok=True)
