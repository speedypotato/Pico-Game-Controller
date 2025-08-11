# -*- mode: python ; coding: utf-8 -*-

from pathlib import Path

block_cipher = None

ROOT = Path.cwd()
ICON = ROOT  / 'tools' / 'icon.ico'
ENTRY = ROOT / 'tools' / 'effect_selector.py'

# Hidden imports to be safe on some environments
hidden = ['hid', 'tkinter']


a = Analysis(
    [str(ENTRY)],
    pathex=[str(ROOT)],
    binaries=[],
    datas=[],
    hiddenimports=hidden,
    hookspath=[],
    hooksconfig={},
    runtime_hooks=[],
    excludes=[],
    noarchive=False,
)
pyz = PYZ(a.pure, a.zipped_data, cipher=block_cipher)

exe = EXE(
    pyz,
    a.scripts,
    a.binaries,
    a.zipfiles,
    a.datas,
    [],
    name='PicoGameConfig',
    debug=False,
    bootloader_ignore_signals=False,
    strip=False,
    upx=True,
    upx_exclude=[],
    runtime_tmpdir=None,
    console=False,
    icon=(str(ICON) if ICON.exists() else None),
)

# One-file build (no COLLECT when building --onefile)
