#!/usr/bin/python3

# A lot done here that should probably be done in SQL...

import matplotlib.pyplot as plt
import os
import sys

os.chdir(sys.argv[1])

hypergraph_files = (file for file in os.listdir('.') if file.endswith('data.txt'))

for filename in hypergraph_files:
  name = filename[:-len('.data.txt')]

  plt.title(name)
  plt.xlabel('Cutoff percentage')
  plt.ylabel('Suboptimality factor')

  file = open(filename)
  cutoffs = [float(cutoff) for cutoff in file.readline().strip().split(',')[1:]]

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

  plt.axhline(y=1, color='r')
  plt.legend()
  plt.savefig(f'cutoff_{name}.pdf')
  plt.close()
