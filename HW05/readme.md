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
