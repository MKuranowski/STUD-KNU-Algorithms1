#!/usr/bin/env python3
from typing import Iterator, TypeVar, NamedTuple
from dataclasses import dataclass, field
from functools import total_ordering
from math import dist, inf
from argparse import ArgumentParser
from time import perf_counter
from heapq import heappush, heappop


T = TypeVar("T")
Point = tuple[float, float]
DistMap = list[list[float]]


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


class DijkstraSearchNode(NamedTuple):
    point: int
    nodes_visited: int


@total_ordering
@dataclass
class DijkstraSearchEntry:
    point: int
    nodes_visited: int
    nodes_to_visit: int
    total_cost: float
    deleted: bool = False

    @property
    def node(self) -> DijkstraSearchNode:
        return DijkstraSearchNode(self.point, self.nodes_visited)

    def __eq__(self, o: object) -> bool:
        return isinstance(o, DijkstraSearchEntry) and self.node ==  o.node

    def __lt__(self, o: "DijkstraSearchEntry") -> bool:
        # return True if `self` is better than `o`
        if self.nodes_to_visit == o.nodes_to_visit and self.nodes_visited == o.nodes_visited:
            return self.total_cost < o.total_cost
        elif self.nodes_to_visit == o.nodes_to_visit:
            return self.nodes_visited > o.nodes_visited
        else:
            return self.nodes_to_visit > o.nodes_to_visit


@dataclass
class DijkstraSearch:
    pts: list[Point]
    distances: DistMap
    max_cost: float
    max_length: int | None = None

    queue: list[DijkstraSearchEntry] = field(default_factory=list)
    entries: dict[DijkstraSearchNode, DijkstraSearchEntry] = field(default_factory=dict)
    previous: dict[DijkstraSearchNode, DijkstraSearchNode] = field(default_factory=dict)

    def reconstruct_route(self, end: DijkstraSearchNode) -> list[int]:
        route: list[int] = [end.point]

        via = end
        while via.point != 0:
            via = self.previous[via]
            route.append(via.point)

        assert len(route) == end.nodes_visited
        route.reverse()

        return route

    def do(self) -> tuple[float, list[int]]:
        end = len(self.pts) - 1
        self.queue = [DijkstraSearchEntry(
            point=0,
            nodes_visited=1,
            nodes_to_visit=end,
            total_cost=0.0,
        )]
        self.entries = {self.queue[0].node: self.queue[0]}
        self.previous.clear()

        while self.queue:
            entry = heappop(self.queue)

            if entry.deleted:
                continue

            # End reached
            if entry.point == end and (self.max_length is None or entry.nodes_visited == self.max_length):
                return entry.total_cost, self.reconstruct_route(entry.node)

            # NOTE: deliberately not setting entry.deleted = True
            #       thanks to that, self.entries can do double duty as a set
            #       of visited nodes.

            for next in range(entry.point+1, len(self.pts)):
                next_entry = DijkstraSearchEntry(
                    point=next,
                    nodes_visited=entry.nodes_visited + 1,
                    nodes_to_visit=len(self.pts) - next - 1,
                    total_cost=entry.total_cost + self.distances[entry.point][next],
                )

                # Skip not permitted entries
                if next_entry.total_cost > self.max_cost or (
                    self.max_length is not None and next_entry.nodes_visited > self.max_length
                ):
                    continue

                # Check if we have an entry to this node
                existing = self.entries.get(next_entry.node)

                if existing and not existing.deleted and next_entry.total_cost > existing.total_cost:
                    # We already have ann entry and it's cheaper
                    continue
                elif existing:
                    # The existing entry is not cheaper
                    existing.deleted = True

                # Push new_entry onto the queue
                self.entries[next_entry.node] = next_entry
                self.previous[next_entry.node] = entry.node
                heappush(self.queue, next_entry)

        return inf, []


def main() -> None:
    arg_parser = ArgumentParser()
    arg_parser.add_argument("--length", type=int, help="expected route length")
    arg_parser.add_argument("file", help="input file name")
    args = arg_parser.parse_args()

    # Load points
    with open(args.file, "r") as f:
        pts = load_points(f)

    distances = calc_distances(pts)

    dijkstra = DijkstraSearch(pts, distances, inf, args.length)

    for max_fuel in [29.0, 45.0, 77.0, 150.0]:
        dijkstra.max_cost = max_fuel

        elapsed = perf_counter()
        used_fuel, route = dijkstra.do()
        elapsed = perf_counter() - elapsed

        print(f"{max_fuel:.0f} {used_fuel:.1f} ({len(route)} points)")
        for idx in route:
            pt = pts[idx]
            print(f"{pt[0]:.0f} {pt[1]:.0f}", end="\t")
        print()
        print(f"{elapsed:.5f} seconds", end="\n\n")


if __name__ == "__main__":
    main()
