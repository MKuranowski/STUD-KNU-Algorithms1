#!/usr/bin/env python3
import argparse
from random import randint

arg_parser = argparse.ArgumentParser()
arg_parser.add_argument("count", type=int, help="number of points to generate")
count: int = arg_parser.parse_args().count

print(count)
print("0 0")
for _ in range(count - 2):
    print(randint(1, count-1), randint(1, count-1))
print(count, count)
