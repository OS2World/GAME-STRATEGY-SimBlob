//
// Copyright (C) 1999 Amit J. Patel
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  Amit J. Patel makes no
// representations about the suitability of this software for any
// purpose.  It is provided "as is" without express or implied warranty.
//

#include "std.h"
#include <stack.h>

#include "Notion.h"
#include "Figment.h"

#include "HexCoord.h"
#include "Unit.h"
#include "Map.h"

#include "Path.h"

// Let's create some typedefs so that we can change which data
// structures are being used.  In the future, these will be
// template parameters.
typedef HexCoord Location;
typedef HexDirection Direction;

// The mark array marks directions on the map.  The direction points
// to the spot that is the previous spot along the path.  By starting
// at the end, we can trace our way back to the start, and have a path.
// It also stores 'f' values for each space on the map.  These are used
// to determine whether something is in OPEN or not.  It stores 'g'
// values to determine whether costs need to be propagated down.
struct Marking
{
    HexDirection direction:3;   // !DirNone means OPEN || CLOSED
    int f:14;                   // >= 0 means OPEN
    int g:14;                   // >= 0 means OPEN || CLOSED
    Marking(): direction(DirNone), f(-1), g(-1) {}
};
static MapArray<Marking>& mark = *(new MapArray<Marking>(Marking()));

// Path_div is used to modify the heuristic.  The lower the number,
// the higher the heuristic value.  This gives us worse paths, but
// it finds them faster.  This is a variable instead of a constant
// so that I can adjust this dynamically, depending on how much CPU
// time I have.  The more CPU time there is, the better paths I should
// search for.
Subject<int> path_div(6);

// Since this module is being used in a multithreaded program,
// we lock the global data structures used by A*:
static Mutex astar_mutex;

struct Node
{
    Location loc;   // location on the map, in hex coordinates
    int g;          // g in A* represents how far we've already gone
    int h;          // h in A* represents an estimate of how far is left

    Node(): loc(), g(0), h(0) {}
    ~Node() {}
};

bool operator < (const Node& a, const Node& b)
{
    // To compare two nodes, we compare the `f' value, which is the
    // sum of the g and h values.  To break ties, we compare the `h' value.
    // (Tie breaking can speed up A*.)  By breaking ties with `h' instead
    // of `g', we can produce the desired search strategy:  when A and B
    // are equal, and A is chosen, then A produces node C.  We want to
    // expand node C instead of node B, so we do that by picking the
    // one with the lower `h' value.
    int af = a.g + a.h;
    int bf = b.g + b.h;
    return (af < bf) || (af == bf && a.h < b.h);
}

bool operator == (const Node& a, const Node& b)
{
    // Two nodes are equal if their components are equal
    return (a.loc == b.loc) && (a.g == b.g) && (a.h == b.h);
}

inline Direction ReverseDirection(Direction d)
{
    // With hexagons, I'm numbering the directions 0 = N, 1 = NE,
    // and so on (clockwise).  To flip the direction, I can just
    // add 3, mod 6.
    return HexDirection( ( 3+int(d) ) % 6 );
}

// greater<Node> is an STL thing to create a 'comparison' object out of
// the greater-than operator, and call it comp.
greater<Node> comp;

// Let's define our priority queue implementation.
// I'm using a priority queue implemented as a heap.  STL has some nice
// heap manipulation functions.  (Look at the source to `priority_queue'
// for details.)  I didn't use priority_queue because later on, I need
// to traverse the entire data structure to update certain elements; the
// abstraction layer on priority_queue wouldn't let me do that.
typedef vector<Node> Container;

// * Things in OPEN are in the open container (which is a heap),
//   and also their mark[...].f value is nonnegative.
// * Things in CLOSED are in the visited container (which is unordered),
//   and also their mark[...].direction value is not DirNone.
class PriorityQueue
{
  public:
    // Remember which nodes are in the OPEN set, and remember
    // which nodes we visited, so that we can clear the mark array
    // at the end.  This is the 'CLOSED' set plus the 'OPEN' set.
    Container open, visited;

    PriorityQueue() {}
    ~PriorityQueue() {}

    void reset();

    inline bool empty() const { return open.empty(); }
    
    inline void insert(Node& N, Direction dir = DirNone)
    {
        // Now add this node to OPEN
        open.push_back(N);
        push_heap(open.begin(), open.end(), comp);
        
        // Add this node to VISITED (OPEN|CLOSED) if it's not already there:
        if( !is_visited(N.loc) )
            visited.push_back(N);

        // Set the marking array to indicate that the node is OPEN
        mark[N.loc].direction = dir;
        mark[N.loc].f = N.g+N.h;
        mark[N.loc].g = N.g;
    }
    
    void get_first(Node& N)
    {
        N = open.front();
        pop_heap(open.begin(), open.end(), comp);
        open.pop_back();

        // This node is no longer in open:
        mark[N.loc].f = -1;
    }

    Container::iterator find_in_open(const HexCoord& h);

    inline bool is_visited(const HexCoord& h)
    {
        return mark[h].g != -1;
    }
    
    inline bool is_open(const HexCoord& h)
    {
        return mark[h].f != -1;
    }

    inline int g_value(const HexCoord& h)
    {
        // This should only be called if g is in OPEN
        Assert( is_open(h) );
        return mark[h].g;
    }

    Node decrease_key(const HexCoord& h, int new_g, Direction dir);
};

Container::iterator PriorityQueue::find_in_open(const HexCoord& hn)
{
    // Only search for this node if we know it's in the OPEN set
    if( Map::valid(hn) && is_open(hn) ) 
    {
        for( Container::iterator i = open.begin(); i != open.end(); ++i )
        {
            if( (*i).loc == hn )
                return i;
        }
    }
    return open.end();
}

Node PriorityQueue::decrease_key(const HexCoord& h, int new_g, Direction dir)
{
    Container::iterator i = find_in_open(h);
    Assert( i != open.end() );
    Assert( g_value(h) == (*i).g );
    
    // Push this thing UP in the heap (only up allowed!)
    (*i).g = new_g;
    push_heap(open.begin(), i+1, comp);
    
    // Set its direction to the parent node
    mark[h].g = new_g;
    mark[h].f = new_g + (*i).h;
    mark[h].direction = dir;

    return (*i);
}
                    
void PriorityQueue::reset()
{
    // Erase the mark array, for all items in open or visited
    for( Container::iterator o = open.begin(); o != open.end(); ++o )
    {
        HexCoord h = (*o).loc;
        mark[h].direction = DirNone;
        mark[h].f = -1;
        mark[h].g = -1;
    }
    for( Container::iterator v = visited.begin(); v != visited.end(); ++v )
    {
        HexCoord h = (*v).loc;
        mark[h].direction = DirNone;
        mark[h].g = -1;
        Assert( !is_open( h ) );
    }
}
    
// Here's the class that implements A*.  I take a map, two points
// (A and B), and then output the path in the `path' vector, when
// find_path is called.
template <class Heuristic>
struct AStar
{
    PathStats stats;
    Heuristic& heuristic;
    PriorityQueue pq;
    Map& map;
    HexCoord source, destination;
    Mutex::Lock lock;
    
    AStar(Heuristic& h, Map& m, HexCoord a, HexCoord b)
        : heuristic(h), map(m), source(a), destination(b), lock(astar_mutex) {}
    ~AStar();

    // Main function:
    void find_path(vector<HexCoord>& path);

    // Helpers:
    void propagate_down(Node H);
    Container::iterator find_in_open(HexCoord h);
};

template<class Heuristic>
AStar<Heuristic>::~AStar()
{
    pq.reset();
}

// This is the 'propagate down' stage of the algorithm, which I'm not
// sure I did right.
template <class Heuristic>
void AStar<Heuristic>::propagate_down(Node H)
{
    // Keep track of the nodes that we still have to consider
    Container examine;
    while( !examine.empty() )
    {
        // Remove one node from the list
        Node N = examine.back();
        examine.pop_back();

        // Examine its neighbors
        for( int dir = 0; dir < 6; ++dir )
        {
            Direction d = Direction(dir);
            HexCoord hn = Neighbor(N.loc, d);
            if( pq.is_open(hn) )
            {
                // This node is in OPEN             
                int new_g = N.g + heuristic.kost(map, N.loc, d, hn);

                // Compare this `g' to the stored `g' in the array
                if( new_g < pq.g_value(hn) )
                {
                    Node new_N = pq.decrease_key(hn, new_g,
                                                 ReverseDirection(d));
                    // Remember to reset this node's children too
                    examine.push_back(new_N);
                }
                else
                {
                    // The new node is no better, so stop here
                }
            }
            else
            {
                // Either it's in closed, or it's not visited yet
            }
        }
    }
}

template <class Heuristic>
void AStar<Heuristic>::find_path(vector<HexCoord>& path)
{
    Node N;
    {
        // insert the original node
        N.loc = source;
        N.g = 0;
        N.h = heuristic.dist(map,source,destination);
        pq.insert(N);
        stats.nodes_added++;
    }

    // While there are still nodes to visit, visit them!
    while( !pq.empty() )
    {
        pq.get_first(N);
        stats.nodes_removed++;
        
        // If we've looked too long, then our semi-goal is now the current
        if( stats.nodes_removed >= heuristic.abort_path )
        {
            // Let's find the spot with the best H*2 + G,
            // because we don't want N (the best H+G) when we're aborting
            // Here we emphasize distance from the goal much more, so
            // that although we don't have a great path, we'll have something
            // closer to the goal (hopefully).
            for( Container::iterator i = pq.visited.begin();
                 i != pq.visited.end(); ++i )
                if( (*i).h * 2 + (*i).g < N.h * 2 + N.g )
                    N = (*i);
            
            destination = N.loc;
        }

        // If we're at the goal, then exit
        if( N.loc == destination )
            break;

        // OPERATION: some kind of neighbor-iterator
        // Look at your neighbors.
        // NOTE: We really should look at all neighbors except for the PARENT
        for( int dci = 0; dci < 6; ++dci )
        {
            Direction d = Direction(dci);
            HexCoord hn = Neighbor(N.loc, d);
            // If it's off the end of the map, then don't keep scanning
            if( !map.valid(hn) )
                continue;

            int k = heuristic.kost(map, N.loc, d, hn);
            Node N2;
            N2.loc = hn;
            N2.g = N.g + k;
            N2.h = heuristic.dist(map, hn, destination);

            if( !pq.is_visited(hn) )
            {
                // The space is not marked
                pq.insert(N2, ReverseDirection(d));
                stats.nodes_added++;
            }
            else
            {               
                // We know it's in OPEN or CLOSED...
                // It's in OPEN, and our new g is better, update
                if( pq.is_open(hn) && N2.g < pq.g_value(hn) )
                {
                    // Search for hn in open
                    Container::iterator find1 = pq.find_in_open(hn);
                    Assert( find1 != pq.open.end() );
                        
                    // Replace *find1's g with N2.g in the list&map
                    mark[hn].direction = ReverseDirection(d);
                    mark[hn].g = N2.g;
                    mark[hn].f = N2.g+N2.h;
                    (*find1).g = N2.g;
                    push_heap(pq.open.begin(), find1+1, comp);
                    // propagate_down( *find1 );
                }
            }
        }
    }

    if( N.loc == destination && N.g < MAXIMUM_PATH_LENGTH )
    {
        stats.path_cost = N.g;
        // We have found a path, so let's copy it into `path'
        HexCoord h = destination;
        while( h != source )
        {
            Direction dir = mark[h].direction;
            path.push_back(h);
            h = Neighbor(h, dir);
            stats.path_length++;
        }
        path.push_back(source);
        // path now contains the hexes in which the unit must travel ..
        // backwards (like a stack)
    }
    else
    {
        // No path
    }

    stats.nodes_left = pq.open.size();
    stats.nodes_visited = pq.visited.size();
}

////////////////////////////////////////////////////////////////////////
// Specific instantiations of A* for different purposes

// UnitMovement is for moving units (soldiers, builders, firefighters)
struct UnitMovement
{
    HexCoord source;
    Unit* unit;
    int abort_path;

    inline static int dist(const HexCoord& a, const HexCoord& b)
    {
        // The **Manhattan** distance is what should be used in A*'s heuristic
        // distance estimate, *not* the straight-line distance.  This is because
        // A* wants to know the estimated distance for its paths, which involve
        // steps along the grid.  (Of course, if you assign 1.4 to the cost of
        // a diagonal, then you should use a distance function close to the
        // real distance.)

        // Here I compute a ``Manhattan'' distance for hexagons.  Nifty, eh?
        int a1 = 2*a.m;
        int a2 =  2*a.n+a.m%2 - a.m;
        int a3 = -2*a.n-a.m%2 - a.m; // == -a1-a2
        int b1 = 2*b.m;
        int b2 =  2*b.n+b.m%2 - b.m;
        int b3 = -2*b.n-b.m%2 - b.m; // == -b1-b2

        // One step on the map is 10 in this function
        return 5*max(abs(a1-b1), max(abs(a2-b2), abs(a3-b3)));
    }

    inline int dist(Map& m, const HexCoord& a, const HexCoord& b)
    {
        int d = dist(a, b);
        if( d > 40 )
        {
            // Only add a correction for longer distances
            double dx1 = a.x() - b.x();
            double dy1 = a.y() - b.y();
            double dx2 = source.x() - b.x();
            double dy2 = source.y() - b.y();
            double cross = dx1*dy2-dx2*dy1;
            if( cross < 0 ) cross = -cross;

            d += int(cross*0.0001);
        }
        return d;
    }
    
    inline int kost(Map& m, const HexCoord& a, 
                    HexDirection d, const HexCoord& b, int pd = -1)
    {
        // This is the cost of moving one step.  To get completely accurate
        // paths, this must be greater than or equal to the change in the
        // distance function when you take a step.

        if( pd == -1 ) pd = path_div;
        
        // Check for neighboring moving obstacles
        int occ = m.occupied_[b];
        if( ( occ != -1 && m.units[occ] != unit ) &&
            ( !m.units[occ]->moving() || ( source == a && d != DirNone ) ) )
                return MAXIMUM_PATH_LENGTH;

        // Roads are faster (twice as fast), and cancel altitude effects
        Terrain t1 = m.terrain(a);
        Terrain t2 = m.terrain(b);
        //      int rd = int((t2==Road||t2==Bridge)&&(t1==Road||t2==Bridge));
        // It'd be better theoretically for roads to work only when both
        // hexes are roads, BUT the path finder works faster when
        // it works just when the destination is a road, because it can
        // just step onto a road and know it's going somewhere, as opposed
        // to having to step on the road AND take another step.
        int rd = int(t2==Road || t2==Bridge);
        int rdv = ( 5 - 10 * rd ) * (pd - 3) / 5;
        // Slow everyone down on gates, canals, or walls
        if( t2 == Wall )
            rdv += 150;
        else if( t2 == Gate || t2 == Canal )
            rdv += 50;
        // Slow down everyone on water, unless it's on a bridge
        if( t2 != Bridge && m.water(b) > 0 )
            rdv += 30;
        // If there's no road, I take additional items into account
        if( !rd )
        {
            // One thing we can do is penalize for getting OFF a road
            if( t1==Road || t1==Bridge )
                rdv += 15;
            // I take the difference in altitude and use that as a cost,
            // rounded down, which means that small differences cost 0
            // ALTITUDE_SCALE is NUM_TERRAIN_TILES/x, so da is at most x.
            int da = (m.altitude(b)-m.altitude(a))/ALTITUDE_SCALE;
            if( da > 0 )
                rdv += da * (pd-3);
        }
        return 10 + rdv;
    }
};

// Some useful functions are exported to be used without the pathfinder

int hex_distance(HexCoord a, HexCoord b)
{
    return UnitMovement::dist(a, b);
}

int movement_cost(Map& m, HexCoord a, HexCoord b, Unit* unit)
{
    UnitMovement um;
    um.unit = unit;
    return um.kost(m, a, DirNone, b, 8);
}

// BuildingMovement is for drawing straight lines (!)
struct BuildingMovement
{
    HexCoord source;
    int abort_path;
    
    inline int dist(Map&, const HexCoord& a, const HexCoord& b)
    {
        double dx1 = a.x() - b.x();
        double dy1 = a.y() - b.y();
        double dd1 = dx1*dx1+dy1*dy1;
        // The cross product will be high if two vectors are not colinear
        // so we can calculate the cross product of [current->goal] and
        // [source->goal] to see if we're staying along the [source->goal]
        // vector.  This will help keep us in a straight line.
        double dx2 = source.x() - b.x();
        double dy2 = source.y() - b.y();
        double cross = dx1*dy2-dx2*dy1;
        if( cross < 0 ) cross = -cross;
        return int( dd1 + cross );
    }

    inline int kost(Map&, const HexCoord& /* first */ , 
                    HexDirection, const HexCoord& /* second */)
    {
        return 0;
    }
};

//////////////////////////////////////////////////////////////////////
// These functions call AStar with the proper heuristic object

PathStats FindUnitPath(Map& map, HexCoord A, HexCoord B, 
                       vector<HexCoord>& path, Unit* unit, int cutoff)
{
    UnitMovement um;    
    um.source = A;
    um.unit = unit;
    um.abort_path = BasePathCutoff + cutoff * hex_distance(A,B) / 10;

    AStar<UnitMovement> finder(um, map, A, B);

    // If the goal node is unreachable, don't even try
    if( um.kost(map, A, DirNone, B) == MAXIMUM_PATH_LENGTH )
    {
        // Check to see if a neighbor is reachable.  This is specific
        // to SimBlob and not something for A* -- I want to find a path
        // to a neighbor if the original goal was unreachable (perhaps it
        // is occupied or unpassable).
        int cost = MAXIMUM_PATH_LENGTH;
        HexCoord Bnew = B;
        for( int d = 0; d < 6; ++d )
        {
            HexCoord hn = Neighbor(B, HexDirection(d));
            int c = um.kost(map, A, DirNone, hn);
            if( c < cost )
            {
                // This one is closer, hopefully
                Bnew = B;
                cost = c;
            }
        }
        // New goal
        B = Bnew;

        if( cost == MAXIMUM_PATH_LENGTH )
        {
            // No neighbor was good
            return finder.stats;
        }
    }

    clock_t c0 = clock();
    finder.find_path(path);
    int num_searches = 1;
    if( finder.stats.path_length > 2000 )
    {
        vector<HexCoord> path2;
        for( int i = 0; i < 999; ++i )
        {
            AStar<UnitMovement> finder2(um, map, A, B);

            path2.clear();
            finder2.find_path(path2);
            num_searches++;
        }
        clock_t c1 = clock();
        
        char s[256];
        sprintf(s,"Timing for %d searches: %7.3f seconds, %d pathdiv, %d steps, %d visited, %d added, %d removed, %d left, %d cost", num_searches, double(c1-c0)/CLK_TCK, int(path_div), finder.stats.path_length, finder.stats.nodes_visited, finder.stats.nodes_added, finder.stats.nodes_removed, finder.stats.nodes_left, finder.stats.path_cost );
        void Log( const char* text1, const char* text2 );
        Log( "A*", s );
    }
    
    return finder.stats;
}

PathStats FindBuildPath(Map& map, HexCoord A, HexCoord B,
                        vector<HexCoord>& path)
{
    BuildingMovement bm;
    bm.source = A;
    bm.abort_path = hex_distance(A,B)*2;

    AStar<BuildingMovement> finder(bm, map, A, B);
    finder.find_path(path);
    return finder.stats;
}

