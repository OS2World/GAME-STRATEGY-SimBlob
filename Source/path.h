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

#ifndef Path_h
#define Path_h

#include "std.h"
#include "Map.h"
#include "stl.h"

#define ALTITUDE_SCALE (NUM_TERRAIN_TILES/16)
#define MAXIMUM_PATH_LENGTH 100000

// Statistics about the path are kept in this structure
struct PathStats
{
    int nodes_added;
    int nodes_removed;
    int nodes_visited;
    int nodes_left;
    int path_length;
    int path_cost;
    PathStats()
        :nodes_added(0), nodes_removed(0),
         nodes_visited(0), nodes_left(0),
         path_length(0), path_cost(0)
    {}
};

const int PathCutoff = 35;      // default cutoff
const int BasePathCutoff = PathCutoff*15;
PathStats FindUnitPath( Map& map, HexCoord A, HexCoord B,
                        vector<HexCoord>& path, Unit* unit,
                        int cutoff = PathCutoff );
PathStats FindBuildPath( Map& map, HexCoord A, HexCoord B,
                    vector<HexCoord>& path );

int hex_distance( HexCoord a, HexCoord b );
int movement_cost( Map& m, HexCoord a, HexCoord b, Unit* unit );

#endif

