# Homework 5 - Solution description

Course: Algorithms 1 (COMP0319-001)
Student: Mikolaj Kuranowski (2020427681)

## Part 2

The whole solution is based on the following source: <https://stackoverflow.com/a/8929356>

Essentially, we consider a transformed graph where:
- a node is a pair of `(point, nodes visited to get to said point)`
- an edge between `(p1, nv1)` and `(p2, nv2)` exists if:
    1. p2 > p1 (lexicographically, to ensure we visit every node once and only with x increasing), and
    2. nv2 == nv1 + 1 (the number of nodes visited increases by one).

  and has the same cost as the distance between p1 and p2.

However, the aforementioned source only considers a route for which the minimal cost and
minimal number of resources is used. For such a problem Dijkstra's algorithm may be used.
Here however, we want to maximize the number of nodes visited, while also minimizing the
fuel used. For this reason, we are going to modify Dijkstra's algorithm to suit the problem.

The main loop in Dijkstra's algorithm picks an entry with the smallest distance.
The first modification is to pick an entry with the biggest amount of nodes visited,
and then smallest amount of fuel used.

This however is not enough, as Dijkstra's algorithm greediness fails completely at finding
max-cost routes (aka on graphs with inverted edge costs). To overcome this, we will pop
entries from the queue that can visit the most amount of nodes first. This ensures that
the longest possible routes are visited.

Another (very small) modification is to reject entries that exceed given maximum cost.

The modified algorithm (called here and in the source **priority search**) will guarantee
that we find a solution since:
1. all the longest candidates have been searched
2. nodes with maximum number of visited nodes are prioritized (important for end condition)
3. nodes with minimum cost are prioritized (important for end condition)
4. entries that exceed given maximum cost are rejected

## Part 1

This part can be solved using exactly the same algorithm as part 2, with 2 very trivial modifications:

1. entries exceeding the length limit are rejected
2. algorithm halts not when any node that corresponds to the end point is visited,
  but when precisely (end point, expected length) node is visited.

## Part 3

Converting the given problem at hand in the same way as in parts 1 and 2
doesn't work, as a graph with |V| ~ 2^number_of_points is created.

My proposed solution approximation involves running the algorithm from previous parts twice:
1. forward with a fuel cap of `some_ratio * max_fuel`
2. backwards, with a fuel cap of `max_fuel - fuel_used_for_forward_leg`,
  and disallowing any points that were visited in the forward leg.

I have run said algorithm for different ratios (see tables below), and the ratio 0.5 seems to yield
best results.

Note that fuel caps are clamped to always allow a straight line between start and end points
in both forward and backward leg.


### hw5_0.txt

| Ratio  | Max Cost 300 | Max Cost 450 | Max Cost 850 | Max Cost 1150 |
|--------|--------------|--------------|--------------|---------------|
|   0.0  |           18 |           36 |           59 |            70 |
|   0.1  |           18 |           36 |           59 |            70 |
|   0.2  |           18 |           36 |           70 |            81 |
|   0.3  |           18 |           36 |           73 |            84 |
|   0.4  |           18 |           49 |       **75** |            86 |
|   0.45 |           18 |       **50** |       **75** |        **87** |
|   0.5  |           25 |       **50** |           74 |        **87** |
|   0.55 |       **26** |       **50** |       **75** |            85 |
|   0.6  |       **26** |           48 |       **75** |            86 |
|   0.7  |       **26** |           45 |           72 |            85 |
|   0.8  |       **26** |           45 |           69 |            82 |
|   0.9  |       **26** |           45 |           66 |            76 |
|   1.0  |       **26** |           45 |           66 |            76 |

### hw5_1.txt

| Ratio | Max Cost 300 | Max Cost 450 | Max Cost 850 | Max Cost 1150 |
|-------|--------------|--------------|--------------|---------------|
|   0.0 |           20 |           37 |           57 |            67 |
|   0.1 |           20 |           37 |           57 |            67 |
|   0.2 |           20 |           37 |           69 |            80 |
|   0.3 |           20 |           37 |           73 |            83 |
|   0.4 |           20 |           50 |       **74** |        **85** |
|   0.5 |       **27** |       **52** |       **74** |        **85** |
|   0.6 |           24 |           51 |       **74** |            84 |
|   0.7 |           24 |           44 |           73 |            80 |
|   0.8 |           24 |           44 |           70 |            80 |
|   0.9 |           24 |           44 |           64 |            75 |
|   1.0 |           24 |           44 |           64 |            75 |

### hw5_2.txt

| Ratio | Max Cost 300 | Max Cost 450 | Max Cost 850 | Max Cost 1150 |
|-------|--------------|--------------|--------------|---------------|
|   0.0 |           21 |           40 |           63 |            74 |
|   0.1 |           21 |           40 |           63 |            74 |
|   0.2 |           21 |           40 |           73 |            87 |
|   0.3 |           21 |           40 |           82 |            88 |
|   0.4 |           21 |           51 |           78 |        **90** |
|   0.5 |       **28** |       **57** |       **79** |            89 |
|   0.6 |           26 |           52 |       **79** |            89 |
|   0.7 |           26 |           46 |           77 |            87 |
|   0.8 |           26 |           46 |           75 |            86 |
|   0.9 |           26 |           46 |           65 |            79 |
|   1.0 |           26 |           46 |           65 |            79 |


