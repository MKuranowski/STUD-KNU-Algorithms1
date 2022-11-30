#!/usr/bin/env python3
from typing import Iterator, TypeVar, Sequence
from dataclasses import dataclass
from math import dist, inf
from itertools import combinations, pairwise
from argparse import ArgumentParser
from time import perf_counter

T = TypeVar("T")
Point = tuple[float, float]
DistMap = list[list[float]]


@dataclass
class Guess:
    fuel: float
    route: list[int]

    def update(self, other: "Guess") -> None:
        if len(other.route) > len(self.route) or (
            len(other.route) == len(self.route) and other.fuel < self.fuel
        ):
            self.fuel = other.fuel
            self.route = other.route


def load_points(f: Iterator[str]) -> list[Point]:
    expected_len = int(next(f).strip())
    points = [tuple(map(float, i.strip().split())) for i in f]
    assert len(points) == expected_len
    assert all(len(pt) == 2 for pt in points)
    points.sort()
    return points


def calc_distances(pts: list[Point]) -> DistMap:
    return [[dist(start, end) for end in pts] for start in pts]


def all_combinations(pool: Sequence[T]) -> Iterator[tuple[T, ...]]:
    for r in range(0, len(pool)+1):
        yield from combinations(pool, r)

def do_naive_search(
    pts: list[Point],
    distances: DistMap,
    max_fuel: float,
) -> Guess:
    through_indices = list(range(1, len(pts)-1))
    all_through_points = all_combinations(through_indices)

    best = Guess(inf, [])

    for through in all_through_points:
        for through_forward in all_combinations(through):
            tf_set = set(through_forward)
            through_backward = (i for i in through if i not in tf_set)

            route = [0, *through_forward, len(pts)-1, *through_backward, 0]
            fuel_used = sum(distances[i][j] for i, j in pairwise(route))

            if fuel_used > max_fuel:
                continue

            best.update(Guess(fuel_used, route))

    return best


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument("file", help="input file name")
    args = arg_parser.parse_args()

    # Load points
    with open(args.file, "r") as f:
        pts = load_points(f)

    distances = calc_distances(pts)

    for max_fuel in [29.0, 45.0, 77.0, 150.0]:
        elapsed = perf_counter()
        best = do_naive_search(pts, distances, max_fuel)
        elapsed = perf_counter() - elapsed

        print(f"{max_fuel:.0f} {best.fuel:.1f} ({len(best.route)} points)")
        for idx in best.route:
            pt = pts[idx]
            print(f"{pt[0]:.0f} {pt[1]:.0f}", end="\t")
        print()
        print(f"{elapsed:.5f} seconds", end="\n\n")


if __name__ == "__main__":
    main()
