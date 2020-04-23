#!/usr/bin/env python3
"""
usage: sqlplot-cutoff.py <source> <destination>

Reads the artifacts from the source directory and outputs plots to the destination directory.
Creates the destination directory if necessary.

Specifically references the 'data.txt' file
"""


def extract_vertices_from_name(name: str) -> str:
    """Takes the number of vertices from a string of the form:

    'uniformplanted_{num_vertices}_{k}_{rank}_{m1}_{m2}'
    """
    suffix = name[len('uniformplanted_'):]
    return suffix[:suffix.find('_')]


import matplotlib.pyplot as plt
import os
import sys
from itertools import takewhile

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

    factors = [float(factor) for factor in line.split(',')[1:]]

    # Cutoff the instances that could not finish in time
    # Technically this could have actually finished in time but just been a very large cut, but unlikely
    cutoff_factor = [(cutoff, factor) for cutoff, factor in zip(cutoffs, factors) if factor < 1e10]

    # Only plot the first cutoff that is 1
    class Predicate:
        def __init__(self):
            self.seen_cut_factor_one = False

        def __call__(self, tup):
            cutoff, factor = tup
            if self.seen_cut_factor_one:
                return False
            if factor <= 1.0:
                self.seen_cut_factor_one = True
            return True
    cutoff_factor = list(takewhile(Predicate(), cutoff_factor))

    cutoffs_filtered = [cutoff for cutoff, factor in cutoff_factor]
    factors_filtered = [factor for cutoff, factor in cutoff_factor]

    if len(cutoff_factor) > 1:
        plt.plot(cutoffs_filtered, factors_filtered, label=algo)
    else:
        plt.plot(cutoffs_filtered, factors_filtered, marker='.', label=algo)

  plt.legend()
  plt.savefig(os.path.join(dest, f'{extract_vertices_from_name(name)}'))
  plt.close()
