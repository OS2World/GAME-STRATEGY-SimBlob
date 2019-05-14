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

#include "Map.h"
#include "MapCmd.h"
// #include "Map_const.h"

#include "Path.h"

CommandQueue map_commands;
inline int MAX_BUILDERS( Map* map )
{
    // One builder, plus another for each 100,000 in population
    return 1 + (map->total_labor / 100000);
}

void CommandQueue::push( const Command& x ) 
{
    Mutex::Lock lock(mutex);
    c.push_back(x);
    count++;
}

Command CommandQueue::pop()
{
    Mutex::Lock lock(mutex);
    count--;
    Command cmd = c.front();
    c.pop_front();
    return cmd;
}

void Map::process_commands()
{
    int commands_processed = 0;
    while( !map_commands.empty() && commands_processed++ < 5 )
    {
        Command c = map_commands.pop();
        switch( c.type )
        {
          case Command::MakeRoads:
          {
              for( int m = 1; m <= Map::MSize; ++m )
                  for( int n = 1; n <= Map::NSize; ++n )
                  {
                      // Put a hex grid on top
                      int mp = m % 4, np = n % 6;
                      if( ( mp == 0 && abs(np-4) <= 1 ) || 
                          ( mp == 2 && abs(np-1) <= 1 ) ||
                          ( ( mp == 1 || mp == 3 ) &&
                            ( np == 5 || np == 2 ) ) )
                      {
                          HexCoord h(m,n);
                          if( terrain(h) == Clear
                              || terrain(h) == Farm
                              || terrain(h) == Houses
                              || terrain(h) == Market )
                              set_terrain( h, Road );
                      }
                  }

              for( int u = 0; u < 40; ++u )
                  for( int v = -20; v <= 25; ++v )
                  {
                      // Basis vectors are (+8,+6) and (+10,-3)
                      int m0 = 8*u + 10*v, n0 = 1 + 6*u - 3*v;
                      HexCoord h0(m0,n0);
                          
                      // Now go around this area and wipe out bonuses
                      // within distance 3
                      for( int m = m0-4; m <= m0+4; ++m )
                          for( int n = n0-4; n <= n0+4; ++n )
                          {
                              HexCoord h(m,n);
                              if(!valid(h)) continue;

                              if( hex_distance(h,h0)/10 <= 3 )
                                  if( terrain(h) == Road )
                                      set_terrain( h, Clear );
                          }
                  }
                          
              break;
          }
              
          case Command::EraseAll:
          {
              for( int m = 1; m <= Map::MSize; ++m )
                  for( int n = 1; n <= Map::NSize; ++n )
                  {
                      HexCoord h(m,n);
                      set_water( h, 0 );
                      heat_[h] = 0;
                      food_[h] = 0;
                      labor_[h] = 0;
                  }
              break;
          }

          case Command::CreateBlob:
          {
              if( occupied_[city_center_] >= 0 )
              {
                  // We should wait a while
                  map_commands.push(c);
                  break;
              }

              Unit* u = Unit::make( this, Unit::Builder, city_center_ );
              u->set_dest( this, c.location );                
              break;
          }
              
          case Command::SetTerrain:
          {
              Mutex::Lock lock( unit_mutex );
              Unit* u = NULL;
              bool postpone = false;
              // Try to find an idle soldier
              for( vector<Unit*>::iterator j = units.begin();
                   j != units.end(); ++j )
              {
                  Unit* unit = *j;
                  if( unit->dead() ) continue;
                  if( unit->type == Unit::Builder )
                  {
                      // This is a builder
                      if( !unit->moving() )
                      {
                          // It's not doing anything
                          if( !u ) u = unit;
                          HexCoord h0 = u->hexloc();
                          HexCoord h1 = unit->hexloc();
                          if( hex_distance( h0, c.location ) > 
                              hex_distance( h1, c.location ) )
                          {
                              // The new unit is closer
                              u = unit;
                          }
                      }
                      else
                      {
                          // Look at non-idle blobs and then
                          // postpone this command if the blob
                          // is going to a nearby place
                          /* SCALE: 10 = one step */
                          if( hex_distance( c.location,
                                            unit->final_dest ) < 90 )
                          {
                              postpone = true;
                              break;
                          }
                      }
                  }
              }
                      
              // Now look for either:
              //   1.  A job that's not taken yet, and is at
              //       the same location
              //   2.  A job entry that's blank
              // The last_k_pos tells us to cycle through
              // instead of always looking for the first one
              static int last_k_pos = 0;
              int k2 = jobs.size();
              int k3 = jobs.size();
              int k;
              for( k = 0; k < jobs.size(); ++k )
              {
                  // Is this job untaken and at the same location?
                  if( jobs[k].location == c.location &&
                      jobs[k].build != -1 )
                  {
                      if( jobs[k].blob == -1 )
                          // can't reuse this yet, so just make no-op
                          jobs[k].build = DO_NOTHING;
                      else
                          break;
                  }
                  // Is this job entry blank?
                  if( jobs[k].build == -1 )
                  {
                      if( k <= last_k_pos )
                      {
                          if( k2 == jobs.size() ) k2 = k;
                      }
                      else
                      {
                          if( k3 == jobs.size() ) k3 = k;
                      }
                  }
              }

              // If we didn't find something in the same location..
              if( k == jobs.size() )
              {
                  // k3 is the first after last_k_pos;
                  // k2 is the first before last_k_pos
                  k = ( k3 == jobs.size() )? k2 : k3;
                  last_k_pos = k;
              }

              // Now if we didn't find anything in the array, add one
              if( k == jobs.size() )
              {
                  if( k >= MAX_CMDS*5 )
                  {
                      // We can't fit any more commands now
                      map_commands.push(c);
                      continue;
                  }
                  jobs.push_back( Job() );
              }

              // Set this job's characteristics
              extern void JobSet( int i, const HexCoord& loc, Terrain t );
              JobSet( k, c.location, c.terrain );
              damage( c.location );

              // Check to see if this unit has done its job yet
              if( u != NULL && u->too_busy() )
                  postpone = true;
                      
              if( u == NULL )
              {
                  // Count the number of builders
                  int num_builders = 0;
                  for( vector<Unit*>::iterator j = units.begin();
                       j != units.end(); ++j )
                      if( !(*j)->dead() &&
                          (*j)->type == Unit::Builder )
                          num_builders++;

                  // If there aren't too many builders, make one
                  if( num_builders < MAX_BUILDERS(this) &&
                      occupied_[city_center_] == -1 )
                      u = Unit::make( this, Unit::Builder, city_center_ );
                  else
                      postpone = true;
              }
                      
              if( postpone )
              {
                  // We don't assign any blobs to this task yet
                  break;
              }

              // DISABLED UNTIL WE GET A CENTRAL SCHEDULER
                  
              // Tell the blob to take this job
              // u->accept_job( k );

              break;
          }
        }
    }        
}
