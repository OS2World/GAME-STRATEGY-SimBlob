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
#include "Path.h"
#include "Unit.h"

#include <algo.h>

const int NUM_WAIT_STEPS = 30;
// Train train;

//////////////////////////////////////////////////////////////////////
// JOBS
vector<Job> jobs;
int num_jobs = 0;

void JobSet( int i, const HexCoord& location, Terrain t )
{
    if( jobs[i].build == -1 ) ++num_jobs;
    jobs[i].blob = -1;
    jobs[i].location = location;
    jobs[i].build = t;
}

void JobKill( int i )
{
    if( jobs[i].build != -1 ) --num_jobs;
    jobs[i].build = -1;
}

void JobBlobAccepted( int i, int b )
{
    jobs[i].blob = b;           
}

void JobBlobAborted( int i )
{
    // A blob no longer wants to work on this job
    jobs[i].blob = -1;
}

void JobBlobFinished( int i )
{
    jobs[i].blob = -1;
    JobKill( i );
}

//////////////////////////////////////////////////////////////////////

PathStats stats;

// Notes:

//  agitated -> 0    while moving
//  agitated == 0    means we might take more jobs
//  agitated == 63   means we should move anywhere
//  agitated++       when we are not moving and have nothing to do

static void m_add( Map* map, Unit* unit, const HexCoord& h )
{
    if( map->occupied_[h] == -1 )
    {
        int pos = unit->pos(map);
        map->occupied_[h] = pos;
        map->damage( h );
    }
    else
        DosBeep(100,100);
}

static void m_del( Map* map, Unit* unit, const HexCoord& h )
{
    if( map->units[map->occupied_[h]] == unit )
    {
        map->occupied_[h] = -1;
        map->damage( h );
    }
    else
        DosBeep(1000,100);
}

static void m_mov( Map* map, Unit* unit, 
                   const HexCoord& h1, const HexCoord& h2 )
{
    if( h1 != h2 )
    {
        m_del( map, unit, h1 );
        m_add( map, unit, h2 );
    }
}

static unsigned int next_unit_id = 1;

Unit* Unit::make( Map* map, Unit::Type t, const HexCoord& h )
{
    // Don't make a new unit if there's a dead one available
    Unit* unit = new Unit( map->units.size() );
    map->units.push_back( unit );
    unit->reincarnate( map, t, h );
    return unit;
}

void Unit::destroy( Unit* unit, Map* map )
{
    // Don't delete it .. just kill it!
    unit->die( map );
}

void Unit::reincarnate( Map* map, Unit::Type t, HexCoord h )
{
    if( id != DEAD_ID ) Throw("Live unit reincarnated");

    // First reinitialize everything
    source = Point(h.x(),h.y());
    dest = source;
    loc = h;
    prev_loc = h;
    home = HexCoord(0,0);
    final_dest = HexCoord(0,0);
    num_attempts = 0;
    agitated = 0;
    j = 0;
    nsteps = 0;
    path.clear();

    type = t;
    for( int i = 0; i < MaxJobs; i++ )
        jobs[i] = -1;

    // Now make this unit alive
    id = next_unit_id++;
    m_add( map, this, h );
}

void Unit::die( Map* map )
{
    if( id == DEAD_ID ) Throw("Dead unit died");

    // cancel jobs
    for( int i = 0; i < MaxJobs; i++ )
    {
        if( jobs[i] != -1 )
        {
            // Mark this job as not being done by this blob anymore
            JobBlobAborted( jobs[i] );
            jobs[i] = -1;
        }
    }
    
    // stop walking
    m_del( map, this, loc );
    id = DEAD_ID;
    type = Idle;
    loc = HexCoord(0,0);
}

//////////////////////////////////////////////////////////////////////

Unit::Unit( int index_ )
    : index(index_), id(DEAD_ID), loc(0,0), type(Idle)
{
}

int Unit::pos( Map* )
{
    return index;
}

Point Unit::gridloc()
{
    if( j < nsteps && nsteps > 0 )
    {
        int k = j+1;
        return Point( (k*dest.x + (nsteps-k)*source.x) / nsteps,
                      (k*dest.y + (nsteps-k)*source.y) / nsteps );
    }
    else
        return source;
}

HexCoord Unit::hexloc()
{
    return loc;
}

HexCoord Unit::hexloc_left()
{
    if( moving() )
        return prev_loc;
    else
        return hexloc();
}

HexCoord Unit::hexloc_right()
{
    if( moving() )
        return path[path.size()-1];
    else
        return hexloc();
}

void Unit::handle_blocked_movement( Map* map )
{
    int n = path.size();    
    if( wait_steps > 0 )
        wait_steps--;
    else if( n > 0 && path[n-1] != path[0] )
    {
        if( n > 15 )
            update_path( map, 10 );
        else
            set_dest( map, final_dest );
    }
    else
        stop();
}

void Unit::step( Map* map )
{
    const int speed = 4;
    int n = path.size();

    if( n == 0 )
        return;

    // Check to see if we've run into a wall or water
    if( j >= nsteps / 2 )
    {
        // n-1 is the immediate destination; 0 is the long term destination
        int i = max(n-2,0);
        if( j <= nsteps/2 )
            i = max(n-1,0);
        HexCoord A = path[i];

        bool blocked = false;
        int occ = map->occupied_[A];
        if( occ != -1 && map->units[occ] != this )
        {
            blocked = true;
            if( map->units[occ]->wait_steps < NUM_WAIT_STEPS && 
                map->units[occ]->wait_steps > NUM_WAIT_STEPS / 3 )
            {
                // The other guy's waiting for us, so don't wait at all
                wait_steps = 0;
            }
        }       

        if( blocked )
        {
            handle_blocked_movement( map );
            return;
        }

        // Move from one hex to the next at the halfway point
        if( j == nsteps / 2 )
        {
            prev_loc = loc;
            loc = path[n-1];
            m_mov( map, this, prev_loc, loc );
        }
    }

    wait_steps = NUM_WAIT_STEPS;
    j++;

    while( j >= nsteps && n > 0 )
    {
        j = 0;
        path.pop_back();
        source = dest;
        n = path.size();
        int l = max(0,n-1);
        int r = max(n-2,0);
        dest = Point( ( path[l].x() + path[r].x() ) / 2,
                      ( path[l].y() + path[r].y() ) / 2 );
        // dest should be on the border between two hexes
        // unless it's an endpoint

        int dx = dest.x - source.x;
        int dy = dest.y - source.y;
        int dist = int( sqrt( (dx*dx) + (dy*dy) ) );

        // This is 10 times the cost
        int kost = movement_cost( *map, path[l], path[r], this );
        if( kost == MAXIMUM_PATH_LENGTH )
        {
            // We can't take the next step
            handle_blocked_movement( map );
            return;
        }
        nsteps = ( kost * dist / speed + 5 ) / 10;
    }

    if( n <= 0 )
    {
        stop();
        if( final_dest != hexloc() && num_attempts < 30 )
            set_dest( map, final_dest );
    }
}

void Unit::stop()
{
    // stop micro-movement
    dest = gridloc();
    source = dest;
    j = 0;
    nsteps = 0;

    // Move the blob a little closer to the center of the hex
    source.x = ( source.x*2 + loc.x() ) / 3;
    source.y = ( source.y*2 + loc.y() ) / 3;
    dest = source;

    // stop waiting
    wait_steps = NUM_WAIT_STEPS;

    // stop macro-movement
    path.clear();
}

static bool is_neighbor( HexCoord A, HexCoord B )
{
    for( int d = 0; d < 6; d++ )
        if( B == Neighbor( A, HexDirection(d) ) )
            return true;
    return false;
}

void Unit::set_dest( Map* map, HexCoord B )
{
    agitated = 0;
    
    if( final_dest != B )
    {
        final_dest = B;
        num_attempts = 0;
    }
    else
        num_attempts++;

    HexCoord A = hexloc();
    if( map->valid( B ) )
    {
        stop();

        if( map->valid( A ) )
        {
            // Set up macro-movement
            int init_size = path.size();
            if( init_size == 0 )
                path.reserve(16); // keep it small
            
            // Increase the cutoff for the path when we are retrying repeatedly
            stats = FindUnitPath( *map, A, B, path, this,
                                  PathCutoff*(2+num_attempts)/2 );
            int n = path.size();

            // Check if we got a valid path
            if( n > init_size )
            {
                // Set up micro-movement
                Point s = gridloc();

                source = s;
                dest = s;
                j = 0;
                nsteps = 0;
            }
            else
                stop();
        }
    }
}

void Unit::update_path( Map* map, int steps_to_update )
{
    int n = path.size();
    HexCoord A = path[n-1];

    // Erase part of the path
    path.erase( path.begin()+n-steps_to_update, path.end() );
    n -= steps_to_update;

    // Prepend a new subpath to our main path, with a smaller cutoff
    FindUnitPath( *map, A, path[n-1], path, this, PathCutoff/2 );

    // Check to see if the path is okay
    if( path.size() <= n )
    {
        // Redo the whole path
        set_dest( map, final_dest );
    }
    else if( !is_neighbor( path[n-1], path[n] ) )
    {
        // A good graft could not be made, so let's cancel the remaining path
        path.erase( path.begin(), path.begin() + n );
    }

    // Reset the micromovement
    j = 0;
    nsteps = 0;
}

// Unit commands to perform jobs

bool Unit::busy()
{
    for( int k = 0; k < MaxJobs; k++ )
        if( jobs[k] != -1 )
            return true;
    return false;
}

bool Unit::too_busy()
{
    for( int k = 0; k < MaxJobs; k++ )
        if( jobs[k] == -1 )
            return false;
    return true;
}

void Unit::accept_job( int jb )
{
    for( int k = 0; k < MaxJobs; k++ )
        if( jobs[k] == -1 )
        {
            jobs[k] = jb;
            JobBlobAccepted(jb, index);
            return;
        }
    Throw("NO JOB SPACE");
}

void Unit::perform_movement( Map* map )
{
    if( !moving() ) return;

    Point p1 = gridloc();
    step( map );
    Point p2 = gridloc();

    if( p1.x != p2.x || p1.y != p2.y )
    {
        // We actually changed position
        HexCoord h(hexloc());
        
        // If there is a fire here, the unit will put out the fire
        if( type == Firefighter && map->terrain(h) == Fire )
            map->set_terrain( h, Scorched );
    }
}

void Unit::perform_jobs( Map* map )
{
    bool is_moving = moving();
    HexCoord h(hexloc());
    int incomplete_job = -1;
    for( int k = 0; k < MaxJobs; k++ )
    {
        if( jobs[k] == -1 ) continue;
        if( jobs[k] < 0 ) Throw("INVALID JOB ID");
        
        if( ::jobs[jobs[k]].build == DO_NOTHING )
        {
            // This job is no longer needed
            if( final_dest == ::jobs[jobs[k]].location )
            {
                // This is where we were going, so cancel movement
                stop();
            }
            JobKill( jobs[k] );
            jobs[k] = -1;
            continue;
        }
        
        HexCoord jobloc = ::jobs[jobs[k]].location;
        if( h == jobloc ) 
        {
            // The unit got to or near its destination, so it should draw
            // something here
            Terrain terrain = Terrain(::jobs[jobs[k]].build);

            // Charge the player for building here
            switch( terrain )
            {
              case Clear: money -= 5; break;
              case Road: money -= 30; break;
              case Bridge: money -= 80; break;
              case Canal: money -= 50; break;
              case Wall: money -= 100; break;
              case Gate: money -= 200; break;
              case WatchFire: money -= 500; break;
              case Fire: money -= 2; break;
              case Trees: money -= 40; break;
              default: break; // No charge
            }

            map->set_terrain( jobloc, terrain );
            // Kill the job in the job listing
            JobBlobFinished( jobs[k] );
            // Tell the blob to not do this job anymore
            jobs[k] = -1;
            continue;
        }

        // Consider this job as the next in line
        if( !is_moving )
            if( incomplete_job == -1 ||
                hex_distance( h, ::jobs[jobs[k]].location ) <
                hex_distance( h, ::jobs[incomplete_job].location ) )
            {
                // Either we have nothing to do, or this job is closer
                // than what we were planning to do next
                incomplete_job = jobs[k];
            }
    }

    // Now, if we're not moving, we should see if there's anything to do
    if( !is_moving )
    {
        if( incomplete_job != -1 )
        {
            // If we have jobs to do, go do one!
            set_dest( map, ::jobs[incomplete_job].location );
            agitated = 0;
        }
        else if( type == Builder && money > 0 && agitated % 8 == 0 )
        {
            // If there are no jobs to do, fill up our queue,
            // choosing close jobs first.  Pick up to jobs_acceptable
            // jobs to do.
            int jobs_acceptable = MaxJobs;
            int jobs_accepted = 0;
            HexCoord last_job( h );
            for( int factor = 256; factor >= 1 && jobs_accepted == 0;
                 factor /= 4 )
            {
                // Stop worrying about finding jobs if we found just one...
                // but pick up as many as possible in a particular range
                for( int k = 0; k < ::jobs.size() &&
                         jobs_accepted < jobs_acceptable; ++k )
                {
                    if( ::jobs[k].build != -1 &&
                        ::jobs[k].blob == -1 &&
                        hex_distance( last_job, ::jobs[k].location )
                        < 6000 / factor )
                    {
                        // This is a real job that hasn't been taken yet
                        accept_job( k );
                        last_job = ::jobs[k].location;
                        map->damage( last_job );
                        jobs_accepted++;
                    }
                }
            }
        }
    }
    
    // If we're STILL not moving, just move randomly
    if( !moving() )
    {
        if( agitated >= 63 )
        {
            HexDirection dir = HexDirection(ByteRandom(6));
            HexCoord hn = Neighbor( h, dir );
            if( Map::valid(hn) )
                set_dest( map, hn );
        }
        else
            ++agitated;
    }   
}

//////////////////////////////////////////////////////////////////////

WatchtowerFire::WatchtowerFire( HexCoord h )
    :location(h)
{
}

//////////////////////////////////////////////////////////////////////

#if 0
// There is a train object that isn't part of the simulation... if
// someone wants a train in some variant of the game, this might be
// a starting point
Train::Train()
    :speed(0.0), location(0.0)
{
}

void Train::begin_moving( Map* map, const HexCoord& A, const HexCoord& B )
{
    path.clear();
    FindUnitPath( *map, A, B, path, NULL, PathCutoff*10 );
    speed = 0.0;
    location = 10.0*path.size() - NUMCARS*CARDIST - 1;
}

void Train::move( Map* map )
{
    double a = 0.0;
    double speed_desired = 0.05 + min(150.0, location) / 30.0;

    int pos = int(location);

    int i = (pos+5)/10;         // start of train
    int j = (pos+CARDIST*NUMCARS-5)/10; // end of train
    if( i+1 < path.size() && j+1 < path.size() )
    {
        int slope = map->altitude(path[j]) - map->altitude(path[i]);
        a += slope*speed*0.1;             // gravity (hills)
        // we multiply by speed because the difference in potential
        // energy is the difference in altitude divided by distance
        // (=slope) multiplied by how far we went last tick
    }
    a -= speed*speed*0.2;       // wind resistance
    
    double vdiff = speed_desired - speed;
    double da = vdiff-a;
    a += da * 0.1;              // engine power
    speed += a * 0.1;
    if( speed < 0.0 )   speed = 0.0;
    location -= speed;
    if( location < 0.0 )
    {
        location = 0.0;
        speed = 0.0;
    }
}

HexCoord Train::car_loc(int i)
{
    if( i < 0 || i >= NUMCARS ) return HexCoord();
    return path[int((location + i*CARDIST)/10.0)];
}

#endif
