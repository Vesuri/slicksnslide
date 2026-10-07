"""Locally owned Amiga test inputs, never packaged.

Defaults live in the shared ~/.local/share/amiga; each has an environment
override (KICKSTART, WORKBENCH_ADF, INSTALLER43, WHDLOAD, SETPATCH).
"""
import os
from pathlib import Path

SHARE = Path.home()/'.local/share/amiga'

def _input(var, default):
    return Path(os.environ.get(var) or default).expanduser()

KICKSTART = _input('KICKSTART', SHARE/'Kickstarts/kick40063.A600')
WORKBENCH_ADF = _input('WORKBENCH_ADF', SHARE/'Workbenchv2.04rev37.67Workbench.adf')
INSTALLER43 = _input('INSTALLER43', SHARE/'Installer43/Installer')
WHDLOAD = _input('WHDLOAD', SHARE/'WHDLoad/C/WHDLoad')
SETPATCH = _input('SETPATCH', SHARE/'SetPatch')
