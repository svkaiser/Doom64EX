#!/usr/bin/python3

import os
import subprocess
import sys

destdir = os.environ.get('DESTDIR', '')

if not destdir and len(sys.argv) > 1:
    datadir = sys.argv[1]

    print('Updating icon cache...')
    subprocess.call(['gtk-update-icon-cache', '-qtf', os.path.join(datadir, 'icons', 'hicolor')])

    print('Updating desktop database...')
    subprocess.call(['update-desktop-database', '-q', os.path.join(datadir, 'applications')])
