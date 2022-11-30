package main

import (
	"bufio"
	"container/heap"
	"errors"
	"fmt"
	"io"
	"math"
	"math/bits"
	"os"
	"sort"
	"time"
)

// Generic helpers

func Reverse[S ~[]T, T any](s S) {
	for l, r := 0, len(s)-1; l < r; l, r = l+1, r-1 {
		s[l], s[r] = s[r], s[l]
	}
}

// Point type

type Point struct{ X, Y float64 }

func (a Point) DistanceFrom(b Point) float64 { return math.Hypot(a.X-b.X, a.Y-b.Y) }

type Points []Point

func (x Points) Len() int      { return len(x) }
func (x Points) Swap(i, j int) { x[i], x[j] = x[j], x[i] }
func (x Points) Less(i, j int) bool {
	if x[i].X == x[j].X {
		return x[i].Y < x[j].Y
	}
	return x[i].X < x[j].X
}

func LoadPoints(rd io.Reader) (pts Points) {
	r := bufio.NewReader(rd)

	// Read the expected number of points
	line, err := r.ReadString('\n')
	if err != nil {
		panic(fmt.Errorf("failed to read from file: %w", err))
	}
	expectedPoints := 0
	_, err = fmt.Sscanln(line, &expectedPoints)
	if err != nil {
		panic(fmt.Errorf("failed to read number of points: %w", err))
	}

	pts = make(Points, 0, expectedPoints)

	// Read individual points
	for {
		line, err := r.ReadString('\n')
		if errors.Is(err, io.EOF) {
			break
		} else if err != nil {
			panic(fmt.Errorf("failed to read from file: %w", err))
		} else if line == "\n" {
			continue // skip empty lines
		}

		pt := Point{}
		_, err = fmt.Sscanln(line, &pt.X, &pt.Y)
		if err != nil {
			panic(fmt.Errorf("failed to read point: %w", err))
		}

		pts = append(pts, pt)
	}

	if len(pts) != expectedPoints {
		panic(fmt.Errorf("expected %d points, got %d", expectedPoints, len(pts)))
	}

	// Sort the points
	sort.Sort(pts)

	return
}

func LoadPointsFromFile(name string) Points {
	var f *os.File
	var err error

	// Close the file once we're done
	defer func() {
		if f != nil && f != os.Stdin {
			f.Close()
		}
	}()

	if name != "" {
		f, err = os.Open(name)
		if err != nil {
			panic(fmt.Errorf("failed to open %s: %w", name, err))
		}
	} else {
		f = os.Stdin
	}

	return LoadPoints(f)
}

// Helper structure to pre-calculate distances

type DistanceMap = [][]float64

func CalculateDistances(pts Points) (m DistanceMap) {
	m = make([][]float64, len(pts))
	for i, from := range pts {
		m[i] = make([]float64, len(pts))
		for j, to := range pts {
			m[i][j] = from.DistanceFrom(to)
		}
	}
	return
}

// BitSet type

const (
	BITSET_BACKING_ARR_LEN = 2
	BITSET_MAX             = 64*BITSET_BACKING_ARR_LEN - 1
)

type BitSet [BITSET_BACKING_ARR_LEN]uint64

func (b BitSet) Len() int {
	sum := 0
	for i := 0; i < BITSET_BACKING_ARR_LEN; i++ {
		sum += bits.OnesCount64(b[i])
	}
	return sum
}

func (b BitSet) Has(i int) bool {
	if i < 0 || i > BITSET_MAX {
		panic(fmt.Errorf("bitset value out of range: %d", i))
	}

	arr, bit := i%64, i/64
	return b[arr]&(1<<bit) != 0
}

func (b *BitSet) Set(i int) {
	if i < 0 || i > BITSET_MAX {
		panic(fmt.Errorf("bitset value out of range: %d", i))
	}

	arr, bit := i%64, i/64
	b[arr] |= (1 << bit)
}

func (b *BitSet) Clear(i int) {
	if i < 0 || i > BITSET_MAX {
		panic(fmt.Errorf("bitset value out of range: %d", i))
	}

	arr, bit := i%64, i/64
	b[arr] &^= (1 << bit)
}

// Priority Search helper structures

type Node struct{ Point, NodesVisited int }

type Entry struct {
	Node
	NodesToVisit int
	TotalCost    float64
	QueueIndex   int
}

func (a *Entry) IsBetterThan(b *Entry) bool {
	if a.NodesToVisit == b.NodesVisited && a.NodesVisited == b.NodesVisited {
		return a.TotalCost < b.TotalCost
	} else if a.NodesToVisit == b.NodesToVisit {
		return a.NodesVisited > b.NodesVisited
	}
	return a.NodesToVisit > b.NodesToVisit
}

type QueueSlice []*Entry

func (s QueueSlice) Len() int { return len(s) }

func (s QueueSlice) Less(i, j int) bool { return s[i].IsBetterThan(s[j]) }

func (s QueueSlice) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
	s[i].QueueIndex = i
	s[j].QueueIndex = j
}

func (s *QueueSlice) Push(x any) {
	e := x.(*Entry)
	e.QueueIndex = s.Len()
	*s = append(*s, e)
}

func (s *QueueSlice) Pop() any {
	last := s.Len() - 1
	e := (*s)[last]
	(*s)[last] = nil // for gc
	*s = (*s)[:last]
	e.QueueIndex = -1
	return e
}

// Priority Search itself

type PrioritySearch struct {
	Pts       Points
	Dist      DistanceMap
	MaxCost   float64
	MaxLength int

	queue    *QueueSlice
	entries  map[Node]*Entry
	previous map[Node]Node
}

func (s *PrioritySearch) clearState() {
	s.queue = new(QueueSlice)
	s.entries = make(map[Node]*Entry)
	s.previous = make(map[Node]Node)
}

func (s *PrioritySearch) pushStart() {
	start := &Entry{
		Node:         Node{0, 1},
		NodesToVisit: len(s.Pts) - 1,
		TotalCost:    0.0,
	}

	s.entries[start.Node] = start
	heap.Push(s.queue, start)
}

func (s *PrioritySearch) reconstructPath(end Node) (p []Point) {
	p = make([]Point, 0, end.NodesVisited)
	for end.NodesVisited > 0 {
		p = append(p, s.Pts[end.NodesVisited-1])
		end = s.previous[end]
	}
	return
}

func (s *PrioritySearch) Do() (Entry, []Point) {
	s.clearState()
	s.pushStart()

	for s.queue.Len() > 0 {
		popped := heap.Pop(s.queue).(*Entry)

		// End reached
		if popped.Point == len(s.Pts)-1 && (s.MaxLength < 0 || popped.NodesVisited == s.MaxLength) {
			return *popped, s.reconstructPath(popped.Node)
		}

		for nextIdx := popped.Point + 1; nextIdx < len(s.Pts); nextIdx++ {
			nextNode := Node{nextIdx, popped.NodesVisited + 1}

			// Skip entries not permitted by the maxLength limit
			if s.MaxLength >= 0 && nextNode.NodesVisited > s.MaxLength {
				continue
			}

			altCost := popped.TotalCost + s.Dist[popped.Point][nextIdx]

			// Skip entries not permitted by the maxCost limit
			if altCost > s.MaxCost {
				continue
			}

			// Check if an entry to this node already exists
			entry := s.entries[nextNode]

			if entry == nil {
				// No existing entry - create one
				entry = &Entry{
					Node:         nextNode,
					NodesToVisit: len(s.Pts) - nextIdx - 1,
					TotalCost:    altCost,
				}

				s.entries[nextNode] = entry
				s.previous[nextNode] = popped.Node
				heap.Push(s.queue, entry)

			} else if altCost < entry.TotalCost {
				// Entry exists, but is more expensive - update it
				entry.TotalCost = altCost

				s.previous[nextNode] = popped.Node
				heap.Fix(s.queue, entry.QueueIndex)

			} else {
				// Entry exists and is cheaper - leave as is
			}
		}
	}

	// No path
	return Entry{TotalCost: math.Inf(1), QueueIndex: -1}, nil
}

func main() {
	// Figure out the input file
	var inputFile string
	if len(os.Args) == 1 {
		// read from stdin - leave inputFile as ""
	} else if len(os.Args) == 2 {
		inputFile = os.Args[1]
	} else {
		panic("usage: ./hw5_3_go [input.txt]")
	}

	// Load points
	points := LoadPointsFromFile(inputFile)
	distances := CalculateDistances(points)

	ps := &PrioritySearch{Pts: points, Dist: distances, MaxLength: -1}

	for _, maxCost := range []float64{29.0, 45.0, 77.0, 150.0} {
		ps.MaxCost = maxCost

		start := time.Now()
		solution, route := ps.Do()
		elapsed := time.Now().Sub(start)

		fmt.Printf("%.0f %.1f (%d points)\n", maxCost, solution.TotalCost, len(route))
		for _, pt := range route {
			fmt.Printf("%.0f %.0f\t", pt.X, pt.Y)
		}
		fmt.Printf("\n%.5f seconds\n\n", elapsed.Seconds())
	}
}
