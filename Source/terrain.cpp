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

#include "Notion.h"
#include "Map.h"
#include "Map_Const.h"

#include <algo.h>

void Map::smooth_terrain( int num_hexes )
{
    static int pos = 0;
    for( int i = 0; i < num_hexes; ++i )
    {       
        HexCoord h; hex_position( pos++, h );
        if( pos >= NUM_HEXES ) pos = 0;

        // Don't change anything if erosion isn't allowed here
        if( !erosion( h ) )
            continue;

        // Now change altitudes based on soil moisture and altitude
        // Also, if the hex has water, then it only is smoothed with
        // adjacent spaces that also have water.  Water and land
        // erosion don't mix.
        int alt0 = altitude(h);
        int alt1 = 0;
        bool w0 = (water(h) != 0);
        int count = 0;
        for( int d = 0; d < 6; ++d )
        {
            HexCoord h2 = Neighbor( h, HexDirection(d) );
            if( !Map::valid(h2) || !erosion(h2) )
                continue;
            bool w2 = (water(h2) != 0);
            if( w0 != w2 )
                continue;
            alt1 += altitude( h2 );
            ++count;
        }
        if( count == 0 ) continue;

        alt1 += (count/2);
        alt1 /= count;

        if( w0 || moisture( h ) > (3+MAX_MOISTURE)/4 )
            // Near rivers, erosion is very slow
            set_altitude( h, (alt0*129 + alt1 + 65) / 130 );
        else if( alt0 > MIN_DESERT && water( h ) <= 0 )
        {
            Terrain t = terrain(h);
            if( t == Farm || t == Houses )
                // civilization leads to high erosion
                set_altitude( h, (alt0*5 + alt1 + 3) / 6 );
            else if( alt0 > MAX_DESERT || t == Trees || t == Wall )
                // mountain erosion is medium
                set_altitude( h, (alt0*49 + alt1 + 25) / 50 );
            else
                // desert erosion is slow
                set_altitude( h, (alt0*79 + alt1 + 40) / 80 );
        }
        else
            // lowland erosion is fast, to smooth out valleys
            set_altitude( h, (alt0*15 + alt1 + 8) / 16 );
    }
}

void Map::recalculate_histogram()
{
    for( int a = 0; a < NUM_TERRAIN_TILES; ++a )
        alt[a] = 0;
    int total_neg = 0;
    int total_pos = 0;

    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            int a = altitude( h );
            if( a < 0 )
            {
                a = 0;
                set_altitude( h, a );
            }
            if( a >= NUM_TERRAIN_TILES )
            {
                a = NUM_TERRAIN_TILES-1;
                set_altitude( h, a );
            }
            ++alt[a];
        }
    
    // Build a summation histogram out of the population histogram
    //   alt[i] == number of spots with altitude <= i
    for( int a = NUM_TERRAIN_TILES-1; a >= 0; --a )
    {
        for( int j = 0; j < a; ++j )
            alt[a] += alt[j];
    }

    // Build a desired summation histogram
    int total = alt[NUM_TERRAIN_TILES-1];
    for( int a = 0; a < NUM_TERRAIN_TILES; ++a )
    {
        const int NUM_OCEAN = total/10;
        const int NUM_PLAINS = total/10;
        if( a < NUM_TERRAIN_TILES/10 )
            out[a] = NUM_OCEAN + a*NUM_PLAINS/(NUM_TERRAIN_TILES/10);
        else
            out[a] = NUM_OCEAN + NUM_PLAINS + a*
                (total-NUM_OCEAN-NUM_PLAINS)/(NUM_TERRAIN_TILES-2);
        // Linear:
        // out[a] = a*total/(NUM_TERRAIN_TILES-2);
        if( alt[a] < out[a] )
            total_neg += out[a]-alt[a];
        else
            total_pos += alt[a]-out[a];
    }
    histogram_disturbed = total_neg + total_pos;
}

void Map::redistribute_terrain( int percentage )
{
    // Let out[a]==a*MSize*NSize/NUM_TERRAIN_TILES for now
    for( int j = 0; j < MSize*NSize*percentage/100
             && histogram_disturbed > 0; ++j )
    {
        HexCoord h( 1+ShortRandom(MSize), 1+ShortRandom(NSize) );
        int a = altitude(h);
        
        if( a > 0 && a < NUM_TERRAIN_TILES && alt[a] < out[a] )
        {
            set_altitude( h, a-1 );
            ++alt[a];
            --histogram_disturbed;
        }
        else if( a < NUM_TERRAIN_TILES-1 && a >= 0 && alt[a] > out[a] )
        {
            set_altitude( h, a+1 );
            --alt[a];
            --histogram_disturbed;
        }
    }
}

