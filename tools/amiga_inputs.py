"""Locally owned Amiga test inputs and the emulator, never packaged.

Defaults are AmigaXDev's shared ones (amigaxdev.env); each has the shared env.sh
environment override (KICKSTART, WORKBENCH_ADF, INSTALLER43, WHDLOAD_BINARY,
SETPATCH, FSUAE_ARM). FSUAE, as set by amiga/env.sh, picks another emulator.
"""
import os
from pathlib import Path

import _amigaxdev  # noqa: F401  (puts AMIGA_PYLIB on sys.path)
from amigaxdev.env import path

SHARE = Path(path('AMIGA_SHARE'))
KICKSTART = Path(path('KICKSTART'))
WORKBENCH_ADF = Path(path('WORKBENCH_ADF'))
INSTALLER43 = Path(path('INSTALLER43'))
WHDLOAD = Path(path('WHDLOAD_BINARY'))
SETPATCH = Path(path('SETPATCH'))
FSUAE = os.environ.get('FSUAE') or path('FSUAE_ARM')
