#!/usr/bin/env python3
from typing import Iterator, TypeVar, NamedTuple, Container
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


def calc_distances(pts: list[Point]) -> DistMap:
    return [[dist(start, end) for end in pts] for start in pts]


def clamp(x: float, lo: float, hi: float) -> float:
    return max(min(x, hi), lo)



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
        return isinstance(o, DijkstraSearchEntry) and self.node == o.node

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
    disallowed_points: Container[int] = field(default_factory=frozenset)

    queue: list[DijkstraSearchEntry] = field(default_factory=list)
    entries: dict[DijkstraSearchNode, DijkstraSearchEntry] = field(default_factory=dict)
    previous: dict[DijkstraSearchNode, DijkstraSearchNode] = field(default_factory=dict)

    def reconstruct_route(self, end: DijkstraSearchNode, start_idx: int) -> list[int]:
        route: list[int] = [end.point]

        via = end
        while via.point != start_idx:
            via = self.previous[via]
            route.append(via.point)

        assert len(route) == end.nodes_visited
        route.reverse()

        return route

    def do(self, start: int, end: int) -> tuple[float, list[int]]:
        backwards = start > end

        self.queue = [
            DijkstraSearchEntry(
                point=start,
                nodes_visited=1,
                nodes_to_visit=abs(end - start),
                total_cost=0.0,
            )
        ]
        self.entries = {self.queue[0].node: self.queue[0]}
        self.previous.clear()

        while self.queue:
            entry = heappop(self.queue)

            if entry.deleted:
                continue

            # End reached
            if entry.point == end and (
                self.max_length is None or entry.nodes_visited == self.max_length
            ):
                return entry.total_cost, self.reconstruct_route(entry.node, start)

            # NOTE: deliberately not setting entry.deleted = True
            #       thanks to that, self.entries can do double duty as a set
            #       of visited nodes.

            # Figure out the next available nodes
            if backwards:
                next_points = range(entry.point - 1, end - 1, -1)
            else:
                next_points = range(entry.point + 1, end + 1, 1)
            next_points = filter(lambda i: i not in self.disallowed_points, next_points)

            for next in next_points:
                next_entry = DijkstraSearchEntry(
                    point=next,
                    nodes_visited=entry.nodes_visited + 1,
                    nodes_to_visit=abs(end - next),
                    total_cost=entry.total_cost + self.distances[entry.point][next],
                )

                # Skip not permitted entries
                if next_entry.total_cost > self.max_cost or (
                    self.max_length is not None and next_entry.nodes_visited > self.max_length
                ):
                    continue

                # Check if we have an entry to this node
                existing = self.entries.get(next_entry.node)

                if (
                    existing
                    and not existing.deleted
                    and next_entry.total_cost > existing.total_cost
                ):
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
    arg_parser.add_argument(
        "-r",
        "--ratio",
        type=float,
        default=0.5,
        help="how much fuel should be used for the forward leg (from 0.0 to 1.0)",
    )

    arg_parser.add_argument("file", help="input file name")
    args = arg_parser.parse_args()

    # Load the ratio
    ratio: float = args.ratio
    if ratio < 0.0 or ratio > 1.0:
        raise ValueError(f"invalid ratio: {ratio}")

    # Load points
    with open(args.file, "r") as f:
        pts = load_points(f)
    distances = calc_distances(pts)

    start = 0
    end = len(pts)-1

    # Perform the searches
    dijkstra = DijkstraSearch(pts, distances, inf)
    # max_fuels = [29.0, 45.0, 77.0, 150.0]  # for 13 points
    # max_fuels = [77.0, 150.0, 273.0, 348.0]  # for 20 points
    max_fuels = [300.0, 450.0, 850.0, 1150.0]  # for 100 points

    for max_fuel in max_fuels:
        assert max_fuel >= 2*distances[0][-1]
        fuel_forward = clamp(max_fuel * ratio, distances[0][-1], max_fuel - distances[0][-1])

        elapsed = perf_counter()

        # Find the forward route
        dijkstra.max_cost = fuel_forward
        dijkstra.disallowed_points = frozenset()
        used_fuel_forward, route_forward = dijkstra.do(start, end)

        # Find the backwards route
        dijkstra.max_cost = max_fuel - used_fuel_forward
        dijkstra.disallowed_points = frozenset(route_forward[1:-1])
        used_fuel_backward, route_backward = dijkstra.do(end, start)

        # Combine the result
        used_fuel = used_fuel_forward + used_fuel_backward
        route = [*route_forward, *route_backward[1:]]

        elapsed = perf_counter() - elapsed

        print(f"{max_fuel:.0f} {used_fuel:.1f} ({len(route)} points)")
        # for idx in route:
        #     pt = pts[idx]
        #     print(f"{pt[0]:.0f} {pt[1]:.0f}", end="\t")
        # print()
        print(f"{elapsed:.5f} seconds", end="\n\n")


if __name__ == "__main__":
    main()
