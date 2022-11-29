#!/usr/bin/env python3
from typing import Iterator, TypeVar
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


def distance_between(pts: list[Point], start: int, end: int) -> float:
    if start > end:
        return inf
    elif start == end:
        return 0
    else:
        return dist(pts[start], pts[end])


def calc_distances(pts: list[Point]) -> DistMap:
    return [
        [distance_between(pts, start, end) for end, _ in enumerate(pts)]
        for start, _ in enumerate(pts)
    ]


def all_combinations(pool: list[T]) -> Iterator[tuple[T, ...]]:
    for r in range(0, len(pool)+1):
        yield from combinations(pool, r)


def do_naive_search(
    pts: list[Point],
    distances: DistMap,
    max_fuel: float,
    route_len: int | None = None,
) -> Guess:
    through_indices = list(range(1, len(pts)-1))
    all_through_points = all_combinations(through_indices) \
        if route_len is None else combinations(through_indices, route_len - 2)

    best = Guess(inf, [])

    for through in all_through_points:
        route = [0, *through, len(pts)-1]
        fuel_used = sum(distances[i][j] for i, j in pairwise(route))

        if fuel_used > max_fuel:
            continue

        best.update(Guess(fuel_used, route))

    return best


def main():
    arg_parser = ArgumentParser()
    arg_parser.add_argument("--length", type=int, help="expected route length")
    arg_parser.add_argument("file", help="input file name")
    args = arg_parser.parse_args()

    # Load points
    with open(args.file, "r") as f:
        pts = load_points(f)

    distances = calc_distances(pts)

    for max_fuel in [29.0, 45.0, 77.0, 150.0]:
        elapsed = perf_counter()
        best = do_naive_search(pts, distances, max_fuel, args.length)
        elapsed = perf_counter() - elapsed

        print(f"{max_fuel:.0f} {best.fuel:.1f} ({len(best.route)} points)")
        for idx in best.route:
            pt = pts[idx]
            print(f"{pt[0]:.0f} {pt[1]:.0f}", end="\t")
        print()
        print(f"{elapsed:.5f} seconds", end="\n\n")


if __name__ == "__main__":
    main()
