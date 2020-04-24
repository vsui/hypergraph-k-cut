#!/usr/bin/env python3
"""This is a script for making graphs of constant rank instances with varying rank

Usage:
  <script> <src_dir> <dest_dir>

Where <src_dir> is a directory with subfolders
```
constant02
constant04
...
```

And each subfolder has a file `data.csv` with the CXY run in it.
This combines the graphs of differents ranks on the same plot

Outputs a graph in a file named 'output.pdf'
"""

import matplotlib.pyplot as plt
import os
import sqlite3
import sys


def get_series_from_folder2(path):
    """Get list of (size, cxy_time) from a sqlite3 file"""
    conn = sqlite3.connect(os.path.join(path, 'data.db'))

    c = conn.cursor()

    # This assumes that all runs are CXY
    c.execute('''
            SELECT hypergraphs.size, avg(time_elapsed_ms) AS time FROM
            runs JOIN hypergraphs ON hypergraphs.id = runs.hypergraph_id
            GROUP BY runs.hypergraph_id
            ORDER BY hypergraphs.size
            ''')

    return c.fetchall()


# exit()


def get_series_from_folder(path):
    """Get list of (size, cxy_time) from a folder 'say k=2'"""
    return get_series_from_file(os.path.join(path, 'data.csv'))


def get_series_from_file(path):
    """Get list of (size, cxy_time) from a CSV file"""
    with open(path) as f:
        f.readline()  # Discard header line

        series = []

        for line in f:
            line = line.strip()
            size = float(line.split(',')[1])
            time = float(line.split(',')[3])
            series.append((size, time))

    return series


def unzip(l):
    """Unzips a list of tuples into a tuple of two lists

    e.g. [(1,2), (3, 4)] -> ([1, 3], [2, 4])
    """
    xs = [t[0] for t in l]
    ys = [t[1] for t in l]

    return xs, ys


# plt.title(f'Discovery time of CXY on planted instances with different ranks, {os.path.basename(sys.argv[1])}')

folders = [os.path.join(sys.argv[1], folder) for folder in os.listdir(
    sys.argv[1]) if folder.startswith('constant')]

plt.xlabel('Hypergraph size')
plt.ylabel('Discovery time (ms)')

min_x, max_x = None, None

folders.sort(key=lambda name: int(name[-2:]))

print(folders)
for folder in folders:
    xs, ys = unzip(get_series_from_folder2(folder))
    print(xs, ys)
    plt.plot(xs, ys, label=f'rank={int(folder[-2:])}', marker='.')
    min_x = xs[0] if min_x is None else max(min_x, xs[0])
    max_x = xs[-1] if max_x is None else min(max_x, xs[-1])
plt.xlim(min_x, max_x)
plt.legend()
plt.savefig(os.path.join(sys.argv[2]))
plt.close()
