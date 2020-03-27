#!/usr/bin/python3

# A lot done here that should probably be done in SQL...

import matplotlib.pyplot as plt
import os
import sqlite3
import sys
from collections import defaultdict

os.chdir(sys.argv[1])

DB_PATH = 'data.db'

conn = sqlite3.connect(DB_PATH)

# Get algos
c = conn.cursor()

c.execute('SELECT DISTINCT algo FROM runs')
algos = [row[0] for row in c.fetchall()]

c.execute('SELECT id FROM hypergraphs')
hypergraphs = [row[0] for row in c.fetchall()]

percentages = set()

for algo in algos:
  i = algo.find('_', len(algo) - 5)
  percent = int(algo[i + 1:])
  percentages.add(percent)

percentages = list(percentages)
percentages.sort()

c = conn.cursor()

# We need to make a graph for each hypergraph

for hypergraph in hypergraphs:
  cutoffs = defaultdict(list)
  for p in percentages:
    algs = [a for a in algos if a.endswith(f'_{str(p)}')]
    for a in algs:
      alg_name = a[:-len(str(p)) - 1]
      c.execute(f'SELECT val FROM cuts2 WHERE hypergraph_id = "{hypergraph}" ORDER BY val LIMIT 1')
      min_cut_val = c.fetchall()[0][0]
      query = f'SELECT cuts2.val FROM runs INNER JOIN cuts2 on runs.cut_id = cuts2.id WHERE algo="{a}" AND runs.hypergraph_id = "{hypergraph}"'
      c.execute(query)
      cut_vals = [e[0] / min_cut_val for e in c.fetchall()]
      cut_factor = sum(cut_vals) / len(cut_vals)
      cutoffs[alg_name].append(cut_factor)

  plt.title(hypergraph)
  plt.xlabel('Cutoff percentage')
  plt.ylabel('Cut factor')
  for alg_name in cutoffs:
    plt.plot(percentages, cutoffs[alg_name], label=alg_name)
  plt.legend()
  plt.savefig(f'cutoff_{hypergraph}.pdf')
  plt.close()
