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

bool flooding = false;
int flood_timing = 3000;

void Map::calculate_moisture()
{
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);

            // First, adjust the soil moisture levels
            if( water(h) > 0 )
                temp_[h] = MAX_MOISTURE;
            else
            {
                // Allow the moisture to go down by at most one
                int maxmoisture = moisture(h);
                if( maxmoisture < 1 )
                    maxmoisture = 1;
                for( int d = 0; d < 6; ++d )
                {
                    HexCoord h2 = Neighbor( h, HexDirection(d) );
                    if( !valid(h2) ) continue;
                    int mst = moisture_[h2];
                    if( mst > maxmoisture )
                        maxmoisture = mst;
                }
                temp_[h] = maxmoisture-1;
            }
        }

    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            set_moisture( h, temp_[h] );
            temp_[h] = 0;
        }
}

void Map::water_evaporation()
{
    static int pos = 0;
    for( int i = 0; i < NUM_HEXES/100; ++i )
    {
        HexCoord h; hex_position( pos++, h );
        if( pos >= NUM_HEXES ) pos = 0;

        int w = water(h);
        if( w > 0 )
            set_water( h, w*15/16 );
    }
}

void Map::water_flow()
{
    static int pos = 0;
    for( int i = 0; i < NUM_HEXES/5; ++i )
    {       
        HexCoord h; hex_position( pos++, h );
        if( pos >= NUM_HEXES ) pos = 0;

        int w0 = water(h);
        int a0 = altitude(h);
        if( w0 <= 0 || a0 < 0 ) continue;

        int h_terrain = terrain(h);
        int wall_height = WALL_HEIGHT*(h_terrain==Wall);
        int canal_depth = erosion(h)? 0 : CANAL_DEPTH;
        int alt0 = w0 + WATER_MULT*( a0 + wall_height + canal_depth );
        
        int sbdir = -1;
        int sbd = -NUM_TERRAIN_TILES;
        int bdir = -1;
        int bd = 0;

        for( int dir = 0; dir < 6; ++dir )
        {
            HexCoord h2 = Neighbor( h, HexDirection(dir) );
            if( !valid(h2) )
            {
                // Allow stuff to flow off the map
                if( alt0 > 0 )
                {
                    bd = alt0;
                    bdir = dir;
                }
                continue;
            }

            int a = altitude( h2 );
            int w = water( h2 );
            int t2 = terrain( h2 );

            int wall_height_1 = WALL_HEIGHT*(t2==Wall);
            int canal_depth_1 = erosion(h2)? 0 : CANAL_DEPTH;
            int alt1 = ((w>2)?(2+(w-2)/2):w) // let deep water flow faster
                + WATER_MULT*( a + wall_height_1 + canal_depth_1 );

            int d = alt0 - alt1;

            // If a gate is full of water, it acts like a wall
            if( t2 == Gate && w >= 12 )
                d -= WATER_MULT*WALL_HEIGHT;
            if( h_terrain == Gate )
            {
                HexCoord h3( Neighbor( h, HexDirection((dir+3)%6) ) );
                if( valid(h3) ) d += water(h3);
            }

            // Make water tend to stay the way it was going
            if( w > 0 )
                d += 2;

            // See if this direction is better than the others
            if( d >= 0 && d > bd )
            {
                bd = d;
                bdir = dir;
            }

            // Keep the second best direction
            if( d > sbd )
            {
                sbd = d;
                sbdir = dir;
            }
        }

        // w0 is the max amount of water that can be transferred
#if 0
        // This code makes water flow faster in clear areas, which
        // have no vegetation to slow the water runoff
        if( h_terrain == Clear )
            w0 = (w0+1)/2;
        else
            w0 = (w0+3)/5;
#endif
        if( w0 > 2*bd/3 ) w0 = 2*bd/3;

        if( bdir != -1 )
        {
            // Water flows from h to bh
            HexCoord bh = Neighbor( h, HexDirection(bdir) );
            bool bh_valid = valid(bh);
            
            // The energy is the amount of water multiplied by the drop
            int energy = bd;
            if( bd > w0 ) bd = w0;
            energy *= (bd/2);

            if( bh_valid )
            {
                // If the water is filling a previously empty hex,
                // XOR if it is emptying a previously full hex,
                // it can only go through if there's enough force
                if( (water(h) <= bd) != (water(bh) == 0) ) 
                    bd = max( bd-10, 0 );
                
                // Only a certain amount of water can flow through a Gate
                if( terrain_[bh] == Gate && bd+water_[bh] > 5 )
                    bd = 5-water_[bh];
            }
            
            if( bd > 0 )
            {               
                set_water( h, water(h) - bd );
                if( bh_valid ) set_water( bh, water(bh) + bd );

                // Erosion
                HexCoord nh = Neighbor( h, HexDirection((bdir/*+1+4*(bd%2)*/)%6) );
                if( bh_valid && valid(nh) && terrain(nh) != Wall && (flags_[nh]&FLAG_EROSION ) )
                {
                    // Move some stuff to the opposite shore
                    int a = (energy+5)/(1+2*WATER_MULT);

                    int alt1 = altitude( bh );
                    int alt2 = altitude( nh );
                    // Only do this if the opposite shore isn't too high
                    // and also if it's steep enough
                    // and also if we're not down in the valley
                    if( a > 0
                        && alt1 > 1+alt2 
                        && alt2+a < NUM_TERRAIN_TILES 
                        && alt2+a < alt1+3
                        && alt2 > 10 )
                    {
                        set_altitude( bh, alt1 - a );
                        set_altitude( nh, alt2 + a );
                    }
                }
            }
        }
        else if( w0 < 4 && w0 > 0 )
        {
            // Small amounts of water that don't flow .. get absorbed
            set_water( h, water(h)-1 );
        }
        else if( sbdir >= 0 && bd <= 0 && (flags_[h]&FLAG_EROSION) )
        {
            // No neighbor is lower, so lower the best one if possible
            // and raise the current hex
            HexCoord sbh = Neighbor( h, HexDirection(sbdir) );
            if( valid(sbh) )
            {
                int t2 = terrain( sbh );
                if( t2 != Wall && t2 != Gate )
                {
                    int a = altitude( sbh );
                    if( a >= 2 && altitude(h) < a )
                    {
                        set_altitude( sbh, a-1 );
                        set_altitude( h, altitude(h)+1 );
                    }
                }
            }
        }
    }
}

void Map::water_destruction()
{
    static int pos = 0;
    for( int i = 0; i < NUM_HEXES/30; ++i )
    {
        HexCoord h; hex_position( pos++, h );
        if( pos >= NUM_HEXES ) pos = 0;
        
        if( water(h) > 8 )
        {
            Terrain t = terrain( h );
            if( t == Farm || t == Fire || t == Trees
                || t == Scorched || t == Market )
                set_terrain( h, Clear );
            else if( t == Houses )
            {
                if( extra_[h] > 0 )
                    --extra_[h];
                else
                    set_terrain( h, Clear );
            }
            else if( ( t == Wall || t == Road )
                     && ByteRandom(10) == 0 )
                set_terrain( h, Clear );
            else if( t == Gate && water(h) > 5
                     && ByteRandom(10) == 0 )
                set_terrain( h, Clear );
        }
    }
}

void Map::water_from_springs()
{
    int j = time_tick_ / ( 8 / NUM_WATER_SOURCES ) % NUM_WATER_SOURCES;

    // Seasons:  WWW SPR SUM AUT W
    const int flow_level[MONTHS_PER_YEAR] = 
    {14,15,14, 13,10,6, 4,2,1, 2,4,8, 10};
    int flow = drought?0:flow_level[month()]*6;

    static int flood_cycle = 0;

    if( j == 0 )
        flood_cycle++;
    if( flood_cycle > flood_timing )
    {
        // Turn flood on or off
        if( flooding )
            flood_timing = 500+ShortRandom(1000);
        else
            flood_timing = 30+ShortRandom(100);
        flood_cycle = 0;
        flooding = !flooding;
    }

    if( flooding )
    {
        int phase = flood_cycle;
        // For 100 ticks, phase should go up, then phase should go down
        if( phase > flood_timing/2 ) phase = flood_timing-phase;
        if( phase < 0 ) phase = 0;
        flow = 3 + flow*(5+phase)/5;
        flooding = true;
    }

    // Each time this function runs, we add some water to one of the springs
    set_water( water_sources_[j], water(water_sources_[j])+flow );

#if 0
    // This code will push water sources up higher so that they don't
    // erode completely
    if( ByteRandom(128) == 0 )
    {
        int a = altitude( water_sources_[j] );
        HexCoord h = water_sources_[j];
        if( h.m > Map::MSize*2/3 )
            h.m += ByteRandom(5);
        else if( h.m < Map::MSize/3 )
            h.m -= ByteRandom(5);
        if( h.n > Map::NSize*2/3 )
            h.n += ByteRandom(5);
        else if( h.n < Map::NSize/3 )
            h.n -= ByteRandom(5);
            
        h.m += ByteRandom(4)-ByteRandom(4);
        h.n += ByteRandom(4)-ByteRandom(4);
        int da = ByteRandom(5) - 2;
        if( water(h) > 0 )
            da += 3;
        if( a > NUM_TERRAIN_TILES-10 )
            da -= 5;
        if( a < NUM_TERRAIN_TILES/2 )
            da += 10;
        extern void make_mountain( Map& map, int m, int n, int h );
        make_mountain( *this, h.m, h.n, da );
    }
#endif
}
