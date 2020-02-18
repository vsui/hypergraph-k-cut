#!/usr/bin/python3

import argparse
import os
import subprocess
import tarfile
import tempfile
from pathlib import Path
from typing import List

parser = argparse.ArgumentParser(
    description='Run k-core decomposition on hypergraphs in a TAR file and report decompositions with interesting cuts.')
parser.add_argument('tarfile', type=str,
                    help='Path to the tarfile containing the hypergraphs')
parser.add_argument('--bin', dest='bin', type=str,
                    help='Path to the `hkcore` binary. Otherwise this script will build it automatically.')
parser.add_argument('-o', dest='outdir', type=str,
                    help='Path for output files', required=True)
args = parser.parse_args()

TARFILE = os.path.join(os.getcwd(), args.tarfile)
PROJECT_ROOT = Path(__file__).cwd() / '..'
BINARY_PATH = None if args.bin is None else os.path.join(os.getcwd(), args.bin)
OUTPUT_PATH = Path(args.outdir)

if not OUTPUT_PATH.exists():
    print('Error: output path does not exist')
    exit(1)


class TarFileInfoIterator:
    def __init__(self, tfile: tarfile.TarFile):
        self._tfile = tfile

    def __iter__(self) -> 'TarFileInfoIterator':
        return self

    def __next__(self) -> tarfile.TarInfo:
        info = self._tfile.next()
        if info is None:
            raise StopIteration()
        return info


with tarfile.open(TARFILE) as tf, tempfile.TemporaryDirectory() as tempdir:
    olddir = os.getcwd()
    os.chdir(tempdir)

    # Setup project
    if BINARY_PATH is None:
        # TODO it is annoying to have to download googletest. Possible to avoid?
        subprocess.Popen(['cmake', PROJECT_ROOT]).wait()
        subprocess.Popen(['make', 'hkcore']).wait()
        BINARY_PATH = './hkcore'

    # Sort TarInfos so we can process them smallest to largest
    tinfos: List[tarfile.TarInfo] = [
        info for info in TarFileInfoIterator(tf) if info.isfile()]
    tinfos.sort(key=lambda info: info.get_info()['size'])

    for info in tinfos:
        print(f'Extracting {info.name} for analysis...')
        tf.extract(info)
        p = subprocess.Popen([BINARY_PATH, info.name, os.path.join(olddir, OUTPUT_PATH)])
        if p.wait() != 0:
            print('Process exited with non-zero exit code')
        print(f'Done with {info.name}')
        subprocess.Popen(['ls']).wait()
        print(f'Writing files to {OUTPUT_PATH}')

print('Done')
