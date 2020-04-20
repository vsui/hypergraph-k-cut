#!/usr/bin/env python3
"""
usage: sqlplot-cutoff.py <source> <destination>

Reads the artifacts from the source directory and outputs plots to the destination directory.
Creates the destination directory if necessary.

Specifically references the 'data.txt' file
"""

import matplotlib.pyplot as plt
import os
import sys

src = sys.argv[1]
dest = sys.argv[2]

if not os.path.exists(dest):
    os.mkdir(dest)
elif not os.path.isdir(dest):
    print(f'Error: {dest} already exists but is not a directory')

hypergraph_files = (file for file in os.listdir(src) if file.endswith('data.txt'))

for filename in hypergraph_files:
  name = filename[:-len('.data.txt')]

  plt.title(name)
  plt.xlabel('Cutoff percentage')
  plt.ylabel('Suboptimality factor')

  file = open(os.path.join(src, filename))
  cutoffs = [float(cutoff) for cutoff in file.readline().strip().split(',')[1:]]

  plt.axhline(y=1, color='gray', linestyle=':')

  for line in file:
    line = line.strip()
    algo = line.split(',')[0]
    if algo == 'KK':
      continue

    factors = [float(factor) for factor in line.split(',')[1:]]

    # Cutoff the instances that could not finish in time
    cutoff_factor = [(cutoff, factor) for cutoff, factor in zip(cutoffs, factors) if factor < 1e10]

    factors_filtered = [factor for cutoff, factor in cutoff_factor]
    cutoffs_filtered = [cutoff for cutoff, factor in cutoff_factor]

    plt.plot(cutoffs_filtered, factors_filtered, label=algo)

  plt.legend()
  plt.savefig(os.path.join(dest, f'cutoff_{name}.pdf'))
  plt.close()
