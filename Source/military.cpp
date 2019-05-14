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
#include "Path.h"
#include "Map_Const.h"

#include <algo.h>

const int heat_threshold = 2000;
int heat_pref( int h )
{
    if( h <= heat_threshold )
        return (h-heat_threshold);
    else 
        return (heat_threshold-h)*5-100;
}

#if 0
// This function has the old version of the firefighter movement algorithm
// There's a new algorithm below, which is more general and can (in theory)
// handle movement for firefighters, soldiers, and builders.  However I have
// not implemented the new algorithm for soldiers and builders.
void Map::simulate_military()
{
    Mutex::Lock lock( unit_mutex, 200 );
    if( !lock.locked() )
        return;

    static int phase = 0;
    phase++;
    
    if( 1 )
    {
        for( vector<WatchtowerFire>::iterator fi = watchtowers_.begin();
             fi != watchtowers_.end(); ++fi )
        {
            HexCoord h = (*fi).location;
            if( extra_[h] > 0 )
                extra_[h]--;
            else if( heat_[h] > heat_threshold / 40 && occupied_[h] == -1 )
            {
                Unit* unit = Unit::make( this, Unit::Firefighter, h );
                unit->home = h;
                extra_[h] = 150;
                heat_[h] = -10000;
            }			
        }
	
        if( phase % 7 == 0 )
        {
            // Subtract heat
            for( vector<Unit*>::iterator u = units.begin();
                 u != units.end(); ++u )
            {
                Unit* unit = *u;
                if( unit->dead() ) continue;
                HexCoord h = unit->hexloc();
                heat_[h] = heat_[h]/3 + 2;
            }

            static int pos = 0;
            for( int i = 0; i < NUM_HEXES/10; i++ )
            {
                HexCoord h; hex_position( pos++, h );
                if( pos >= NUM_HEXES ) pos = 0;

                int heat = heat_[h];

                // Watery areas reduce heat
                if( water_[h] == 0 )
                    heat_[h] = heat*95/100;
                else
                    heat_[h] = heat*50/100;

                // Terrain effects
                if( terrain(h) == Wall )
                    heat_[h] -= 300;
                else if( terrain(h) == Fire )
                    heat_[h] = heat_threshold;
                else if( terrain(h) == Lava )
                    heat_[h] = -10000;

                // Diffusion
                for( int dir = 0; dir < 6; dir++ )
                {
                    HexCoord h2 = Neighbor(h,HexDirection((dir+phase)%6));
                    if( valid(h2) && terrain(h2) != Wall )
                    {
                        int heat2 = heat_[h2];
                        int delta = (heat-heat2)/8;
                        if( delta > 0 )
                        {
                            heat_[h] -= delta;
                            heat_[h2] += delta;
                        }
                    }
                }
            }
        }
    }
    
    for( vector<Unit*>::iterator u = units.begin(); u != units.end(); ++u )
    {
        Unit& unit = *(*u);
        if( unit.dead() ) continue;
		
        unit.perform_movement( this );
        unit.perform_jobs( this );

        if( !unit.moving() && unit.type == Unit::Firefighter )
        {
            HexCoord h( unit.hexloc() );
            int bpref = heat_pref(heat_[h]);
            int bdir = -1;
            if( num_fires_ > 0 )
                for( int dir = 0; dir < 6; dir++ )
                {
                    HexCoord h2 = Neighbor(h,HexDirection((dir+phase)%6));
                    if( valid(h2) && 
                        ( occupied_[h2] == -1 ||
                          units[occupied_[h2]] == &unit ) )
                    {
                        int pref = heat_pref(heat_[h2]);
                        if( pref > bpref ||
                            ( pref==bpref && ByteRandom(3)==0 ) )
                        {
                            bpref = pref;
                            bdir = (dir+phase)%6;
                        }
                    }
                }
            
            if( bdir != -1 )
                unit.set_dest( this, Neighbor(h,HexDirection(bdir)) );
            else
            {
                // The unit is just sitting there
                if( unit.home == h )
                {
                    if( terrain(h) == WatchFire )
                    {
                        // The unit can go back in, so kill it (set id to -1)
                        unit.die(this);
                    }
                    else if( watchtowers_.size() > 0 )
                    {
                        // The unit's home is gone but it could go to another
                        // place
                        unit.home = watchtowers_[0].location;
                    }
                }
                else if( valid(unit.home) && h != unit.home )
                {
                    // The unit can walk home, if it's been waiting long
                    if( unit.num_attempts++ > 30 )
                        unit.set_dest( this, unit.home );
                }
            }
        }
    }
}
#endif

#if 1
void Map::simulate_military()
{
    Mutex::Lock lock( unit_mutex, 200 );
    if( !lock.locked() )
        return;

    // Count the blobs in each sector
    SectorArray<int> blobs_in_sector(0);
    for( vector<Unit*>::iterator u = units.begin(); u != units.end(); ++u )
    {
        Unit& unit( **u );
        if( !unit.dead() && valid(unit.final_dest) && unit.type == Unit::Firefighter )
            ++blobs_in_sector[sector(unit.final_dest)];
    }

    // Create new firefighters
    for( vector<WatchtowerFire>::iterator fi = watchtowers_.begin();
         fi != watchtowers_.end(); ++fi )
    {
        HexCoord h = (*fi).location;
        if( extra_[h] > 0 )
        {
            // Still waiting before we can launch another firefighter
            extra_[h]--;
        }
        else
        {
            // Scan nearby area
            const int D = 10;
            bool fires = false, blobs = false;
            for( int m = h.m-D; m <= h.m+D; m += D )
                for( int n = h.n-D; n <= h.n+D; n += D )
                {
                    HexCoord h2(m,n);
                    if( !valid(h2) ) continue;

                    if( num_fires_[sector(h2)] > 0 )
                        fires = true;
                    if( blobs_in_sector[sector(h2)] > 0 )
                        blobs = true;
                }

            if( fires && !blobs && occupied_[h] == -1 )
            {
                Unit* unit = Unit::make( this, Unit::Firefighter, h );
                unit->home = h;
                extra_[h] = 255;
            }
        }			
    }
	
    // Move military units around
    for( vector<Unit*>::iterator u = units.begin(); u != units.end(); ++u )
    {
        Unit& unit( **u );
        if( unit.dead() ) continue;

        unit.perform_movement( this );
        unit.perform_jobs( this );

        // Firefighter scheduling -- later extend this to all blobs
        if( unit.type == Unit::Firefighter
            && terrain(unit.final_dest) != WatchFire
            && num_fires_[sector(unit.final_dest)] == 0 )
        {
            // The unit is going somewhere but the fires have been put out
            unit.stop();
        }

        if( !unit.moving() && unit.type == Unit::Firefighter )
        {
            HexCoord h( unit.hexloc() );

            int best_ranking = INT_MIN;
            int best_s = -1;
            int best_dist = 0;

            if( num_fires_[sector(h)] > 0 )
            {
                best_s = sector(h);
                best_dist = 0;
            }
            else
                for( int s = 0; s < NUM_SECTORS; ++s )
                {
                    if( num_fires_[s] > 0 )
                    {
                        int blobs_here = blobs_in_sector[s];
                        if( sector(unit.final_dest) == s ) --blobs_here;
                        int dist = hex_distance( h, sector_center(s) ) / 10;
                        int ranking =
                            num_fires_[s]/( 1 + 8*blobs_here ) - dist*dist/128;
                    
                        if( ranking > best_ranking )
                        {
                            best_ranking = ranking;
                            best_s = s;
                            best_dist = dist;
                        }
                    }
                }
                        
            if( best_s != -1 )
            {
                // Okay, let's go to this sector
                if( best_dist > SECTOR_X_SIZE+SECTOR_Y_SIZE )
                {
                    // If the sector is far away, go to the center of it
                    unit.set_dest( this, sector_center(best_s) );
                }
                else
                {
                    // Find the nearest hex with a fire
                    HexCoord closest_h(h);
                    int closest_dist = INT_MAX;
                    for( SectorIterator sh = sector_begin(best_s);
                         sh != sector_end(best_s); ++sh )
                    {
                        HexCoord hj(*sh);
                        if( terrain(hj) == Fire )
                        {
                            int dist = hex_distance(h, hj);
                            if( dist < closest_dist )
                            {
                                closest_dist = dist;
                                closest_h = hj;
                            }
                        }
                    }
                
                    if( closest_h == h )
                    {
                        // No fires?  Reset ..
                        num_fires_[best_s] = 0;
                    }
                    else
                        unit.set_dest( this, closest_h );
                }
            }
            else
            {
                // The unit is just sitting there
                if( unit.home == h )
                {
                    if( terrain(h) == WatchFire )
                    {
                        // The unit can go back in, so kill it (set id to -1)
                        unit.die(this);
                    }
                    else if( watchtowers_.size() > 0 )
                    {
                        // The unit's home is gone but it could go to another
                        // place
                        unit.home = watchtowers_[0].location;
                    }
                }
                else if( valid(unit.home) && h != unit.home )
                {
                    // The unit can walk home, if it's been waiting long
                    if( unit.num_attempts++ > 30 )
                        unit.set_dest( this, unit.home );
                }
            }
        }
    }
}
#endif

