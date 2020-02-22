#!/usr/bin/python3

import argparse
import os
from typing import List


def parse_value(line: str) -> int:
    line = line.strip()
    return int(line.split(' ')[-1])


def parse_partition(line: str) -> List[int]:
    line = line.strip()
    return [int(i) for i in line.split(' ')[2:]]


parser = argparse.ArgumentParser(
    description="""Gather information from .cut and .hgr files and write to stdout as a CSV file""")

parser.add_argument('srcdir', type=str,
                    help='Path to the folder containing the files')

args = parser.parse_args()

INPUT_DIR = args.srcdir
CUT_EXTENSION = 'cut'

cut_filenames = [f for f in os.listdir(INPUT_DIR) if f.endswith(CUT_EXTENSION)]

print('skewedness = # in larger partition / # vertices')
print('name', '# vertices', '# edges', 'cut value', 'skewedness',
      '# in smaller partition', '# in larger partition', split=',')
for cut_filename in cut_filenames:
    hypergraph_name = cut_filename[:-(len(CUT_EXTENSION) + 1)]
    hypergraph_filename = os.path.join(INPUT_DIR, hypergraph_name + '.hgr')
    cut_filename = os.path.join(INPUT_DIR, cut_filename)
    with open(cut_filename) as cut_file:
        value = parse_value(cut_file.readline())
        p1 = parse_partition(cut_file.readline())
        p2 = parse_partition(cut_file.readline())

    # Get num_vertices, num_edges from hypergraph file
    with open(hypergraph_filename) as h_file:
        num_edges, num_vertices = h_file.readline().strip().split(' ')
        num_edges, num_vertices = int(num_edges), int(num_vertices)

    assert (num_vertices == len(p1) + len(p2))

    if len(p1) > len(p2):
        p1, p2 = p2, p1

    # name, num_vertices, num_edges, cut value, skewedness, num in smaller part, num in larger part
    skewedness = round(len(p2) / num_vertices, 3)
    print(hypergraph_name, num_vertices, num_edges,
          value, skewedness, len(p1), len(p2), sep=',')
