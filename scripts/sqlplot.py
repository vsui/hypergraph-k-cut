#!/usr/bin/python3

import matplotlib.pyplot as plt
import os
import sqlite3
import sys
from typing import Tuple, List

os.chdir(sys.argv[1])

DB_PATH = 'data.db'

conn = sqlite3.connect(DB_PATH)

# Get algos
c = conn.cursor()

c.execute('SELECT DISTINCT algo FROM runs')
algos = [row[0] for row in c.fetchall()]


def make_select(algo_name: str) -> str:
    return f"(SELECT hypergraph_id, AVG(time_elapsed_ms) as {algo_name}_time from runs WHERE algo='{algo_name}' GROUP BY hypergraph_id) AS {algo_name}"


# Make CSV
query = 'SELECT hypergraphs.id, hypergraphs.size, hypergraphs.num_vertices'
for algo in algos:
    query += f', {algo}_time'
query += '\nFROM\n'
query += make_select(algos[0]) + "\n"
for i in range(1, len(algos)):
    query += 'INNER JOIN\n'
    query += make_select(algos[i]) + '\n'
    query += f'ON {algos[i - 1]}.hypergraph_id = {algos[i]}.hypergraph_id\n'
query += f'INNER JOIN hypergraphs ON {algos[-1]}.hypergraph_id = hypergraphs.id\n'
query += 'ORDER BY size'
c = conn.cursor()
c.execute(query)

csv = open('data.csv', 'w+')
col_names = [d[0] for d in c.description]
print(','.join(col_names), file=csv)
for row in c.fetchall():
    row = [str(e) for e in row]
    print(','.join(row), file=csv)


def make_query(algo: str) -> str:
    return f'SELECT hypergraphs.id, hypergraphs.size, {algo}_time FROM {make_select(algo)} INNER JOIN hypergraphs ON {algo}.hypergraph_id = hypergraphs.id ORDER BY size'


def get_series(algo: str) -> Tuple[List[int], List[int]]:
    c = conn.cursor()
    c.execute(make_query(algo))
    rows = c.fetchall()
    return (
        [row[1] for row in rows],
        [row[2] for row in rows]
    )


def make_plot(filename: str, title: str, filter=None):
    plt.title(title)
    plt.xlabel('Hypergraph size')
    plt.ylabel('Discovery time (ms)')
    for algo in algos:
        if filter is not None and not filter(algo):
            continue
        xs, ys = get_series(algo)
        plt.plot(xs, ys, label=algo)
    plt.legend()
    plt.savefig(filename)
    plt.close()


make_plot('full.pdf', 'Full results')
make_plot('value.pdf', 'Value results', lambda algo: algo.endswith('val'))
make_plot('notvalue.pdf', 'Not value results',
          lambda algo: not algo.endswith('val'))
make_plot('mwvalue.pdf', 'Value results', lambda algo: algo.endswith(
    'val') and algo not in ('qval', 'kwval'))
make_plot('mwnotvalue.pdf', 'Not value results',
          lambda algo: not algo.endswith('val') and algo not in ('q', 'kw'))
