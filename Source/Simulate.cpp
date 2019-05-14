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

#include "MapCmd.h"

#include <algo.h>
#include "path.h"

void Map::collect_sector_statistics()
{
    for( int s = 0; s < NUM_SECTORS; ++s )
    {
        num_fires_[s] = 0;
        num_trees_[s] = 0;
    }

    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            if( terrain(h) == Fire )
                ++num_fires_[sector(h)];
            if( terrain(h) == Trees && extra_[h] >= TREE_MATURITY )
                ++num_trees_[sector(h)];
        }
}

void Map::calculate_center()
{
    // These are used in calculating the 'center' of the city
    // (average of all hexes with civilization)
    int total_m = 0, total_n = 0, total_civilized = 0;
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            prefs_[HexCoord(m,n)] = 0; // compatibility with old pref system
            
            // Add coordinates if this space is 'civilized'
            Terrain t = terrain(HexCoord(m,n));
            if( t == Road || t == Bridge || t == Houses || t == Market )
            {
                ++total_civilized;
                total_m += m;
                total_n += n;
            }
        }

    HexCoord new_center;
    if( total_civilized > 0 )
        new_center =
            HexCoord( total_m/total_civilized, total_n/total_civilized );
    else
        new_center =
            HexCoord( Map::MSize/2, Map::NSize/2 );

    if( new_center != city_center_ )
    {
        if( valid(city_center_) ) damage(city_center_);
        city_center_ = new_center;
        if( valid(city_center_) ) damage(city_center_);
    }
}

void Map::calculate_prefs()
{
    static int pos = 0;
    for( int i = 0; i < NUM_HEXES/100; ++i )
    {
        HexCoord h1; hex_position( pos++, h1 );
        if( pos >= NUM_HEXES ) pos = 0;

        // Now we have to count the nearby objects
        int roads[5] = {0,0,0,0,0};
        int waters[5] = {0,0,0,0,0};
        int markets = 0, trees = 0, houses = 0, food = 0, labor = 0;
            
        // it should be possible to make a much faster algorithm
        // or even use influence map incremental algorithms
        int m2_left = max(h1.m-4,1), m2_right = min(h1.m+4,int(Map::MSize));
        int n2_left = max(h1.n-4,1), n2_right = min(h1.n+4,int(Map::NSize));

        int nearest_market_dist = 1000;
        HexCoord nearest_market_loc(h1);
        
        for( int m2 = m2_left; m2 <= m2_right; ++m2 )
        {
            for( int n2 = n2_left; n2 <= n2_right; ++n2 )
            {
                HexCoord h2(m2,n2);
                // h2 should be valid because it's within {left..right}

                Assert( valid(h2) );
                Terrain t2 = terrain(h2);
                int d = hex_distance(h1,h2)/10;
                if( d > 4 ) continue;

                if( t2 == Market && d < nearest_market_dist )
                {
                    // Keep track of the nearest market; note that
                    // this is skewed because of the d < ..., and
                    // certain directions are examined first.
                    nearest_market_dist = d;
                    nearest_market_loc = h2;
                }

                food += food_[h2];
                labor += labor_[h2];
                
                if( water(h2) > 0 )                    ++waters[d];
                    
                if( t2 == Road || t2 == Bridge )       ++roads[d];
                else if( t2 == Market )                ++markets;
                else if( t2 == Trees )                 ++trees;
                else if( t2 == Houses )                ++houses;
            }
        }

        // Now store the offset to the nearest market
        set_nearest_market( h1, nearest_market_loc );
        
        int sum_roads = roads[0]+roads[1]+roads[2]+roads[3]+roads[4];
        if( sum_roads == 0 ) continue;
        
        int sum_water = waters[0]+waters[1]+waters[2]+waters[3]+waters[4];

        // Commercial areas must be next to roads, and prefer to be
        // away from competition.  The road adjacency is checked later.
        C_land_value_[h1] =
            + 3*roads[1]
            + 2*min(roads[2],2)
            - 1*min(markets/2,4)
            + 1*min(houses,4)
            + 1*min(markets,4);

        // Residential areas like to be near water and other residentials
        R_land_value_[h1] = 0
            + 2*min(markets,5)
            + 1*sum_water/5
            + 6*min(sum_roads-roads[4],1)
            + 1*trees/10
            + 1*houses/2
            + 1*min(food/25,10) - 1;

        // Agricultural areas like to be near water and roads
        A_land_value_[h1] = 0
            + 3*min(sum_water,10)
            + 5*min(sum_roads,1)
            + 2*min(markets,3)
            + 1*min(labor/300,4);

        int distance_from_center = hex_distance(h1,city_center_)/10;
        if( distance_from_center <= 4 )
        {            
            // Commercial & Residential areas get a bonus for this zone
            C_land_value_[h1] += 30;
            R_land_value_[h1] += 10;
        }

        if( roads[1] == 0 )
        {
            // This space isn't adjacent to a road
            C_land_value_[h1] = 0;
        }
        
        if( terrain(h1) == Clear || terrain(h1) == Scorched )
        {
            if( C_land_value_[h1] >= R_land_value_[h1] &&
                C_land_value_[h1] >= A_land_value_[h1] )
                set_terrain(h1,Market);
            else if( R_land_value_[h1] >= A_land_value_[h1] &&
                     R_land_value_[h1] >= C_land_value_[h1] )
                set_terrain(h1,Houses);
            else if( A_land_value_[h1] >= R_land_value_[h1] &&
                     A_land_value_[h1] >= C_land_value_[h1] )
                set_terrain(h1,Farm);
        }
    }
}

#if 0
// This is the old code for auto-build mode.  It will look at each spot
// on the map and try to decide if it's better as a road (positive prefs_)
// or a building (negative prefs_).
void Map::calculate_prefs()
{
    // Reset all prefs to half their previous value
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
            prefs_.data[m][n] /= 2;

    for( int i = 0; i < NUM_HEXES; ++i )
    {
        HexCoord h; hex_position( i, h );

        // We don't want to do anything with spaces that are at the border
        if( h.m <= 1 || h.m >= Map::MSize ||
            h.n <= 1 || h.n >= Map::NSize )
        {
            // Something at the edge of the map is generally bad for a road
            prefs_.data[h.m][h.n] -= 5;
            continue;
        }
            
        // We only consider effects next to some things
        Terrain t0 = terrain(h);
        if( !( t0==Road || t0==Bridge || t0==Fire || t0==Lava || t0==Scorched )
            && ByteRandom(2) != 0 )
            continue;
        
        // Anything really near water is undesirable for roads,
        // but if it's one away, it's desirable
        if( moisture(h) >= MAX_MOISTURE-1 || water(h) > 0 )
            prefs_.data[h.m][h.n] -= 20;
        else if( moisture(h) == MAX_MOISTURE-2 )
            prefs_.data[h.m][h.n] += 4;
        else if( !erosion(h) )
            prefs_.data[h.m][h.n] -= 5;

        // Now compute the index and the neighboring hexes
        int index = 0;
        int count = 0;
        HexCoord n[6];
        int dw[6] = {0,0,0,0,0,0};
        for( int d = 0; d < 6; ++d )
        {
            // We don't have to check the validity of n[d] because
            // we specified earlier that the edge of the map wasn't considered
            n[d] = Neighbor( h, HexDirection(d) );
            int t = terrain( n[d] );
            if( t == Road || t == Bridge )
            {
                index |= (1<<d);
                ++count;
            }

            if( t0==Wall || t0==Gate || !erosion(h)
                || t0==Fire || t0 == Lava || t0 == Scorched )
                dw[d] -= 15;
            else if( t0==Trees )
                dw[d] -= 5;
        }

        if( t0 != Road && t0 != Bridge )
        {
            // We don't want auto-building if there aren't roads around
            if( index == 0 )
                prefs_.data[h.m][h.n] = 0;
            
#if 0
            // Consider destroying some area that has too many roads
            if( build_speed > 0 && 
                ( t0 == Clear || t0 == Trees || t0 == Farm || t0 == Houses ) &&
                ( count >= 4 ) )
            {
                // A crowded area
                if( time_tick_ % 1000 < 300-150*(6-count) )
                {
                    // destroy some adjacent roads, maybe making this more open
                    for( int d = 0; d < 6; ++d )
                    {
                        Terrain t2 = terrain(n[d]);
                        if( t2 == Houses || t2 == Road || t2 == Farm ||
                            t2 == Bridge || t2 == Trees )
                            // set_terrain( n[d], Clear );
                            map_commands.push( Command::build( Clear, h ) );
                    }
                    // set_terrain( h, Road );
                    map_commands.push( Command::build( Road, h ) );
                }

                // This seems like a lot but it needs to remain for a while
                // before it decays, because this code won't get run again
                // every time the prefs are calculated
                for( int d = 0; d < 6; ++d )
                    prefs_.data[n[d].m][n[d].n] -= (count-3)*25;
            }
#endif
            for( int d = 0; d < 6; ++d )
                prefs_.data[n[d].m][n[d].n] += dw[d];
            continue;
        }

        // First handle stuff that happens for all border configurations
        if( count >= 2 )
        {
            if( build_speed > 0 )
            {
                // If all the roads and all the nonroads are adjacent,
                // then we can delete the current spot's road
                int ror = (index>>1) | ((index&1)<<5);
                int xor = index ^ ror;
                int bitcount = 0;
                while( xor > 0 )
                {
                    if( xor & 1 )
                        ++bitcount;
                    xor >>= 1;
                }
                if( bitcount <= 2 && ByteRandom(2) == 0 )
                    set_terrain( h, Clear );
            }

            // Set all clear neighbors to -3
            for( int d = 0; d < 6; ++d )
                if( (index & (1<<d)) == 0 )
                    dw[d] = -3;
        }
            
        switch( count )
        {
          case 0:
          {
              // Lone road
              dw[0] = dw[2] = dw[4] = +3;
              dw[1] = dw[3] = dw[5] = -1;
              break;
          }
            
          case 1:
          {
              // End of the road
              // If it's a long road, then don't keep going straight
              int dir = 0;
              for( int d = 0; d < 6; ++d )
                  if( (index & (1<<d)) != 0 )
                      dir = d;
                
              int len = 0;
              const int rdist[8] = {2,2,3,3,3,3,4,5};
              int rlen = rdist[ByteRandom(8)];
              HexCoord h2 = h;
              while( len <= rlen && valid(h2) && 
                     ( terrain(h2) == Road || terrain(h2) == Bridge ) )
              {
                  if( terrain(h2) == Road ) 
                      ++len;
                  h2 = Neighbor( h2, HexDirection(dir) );
              }

              // As the length increases, make it less 
              // likely to go straight
              for( int d = 0; d < 6; ++d )
                  if( d != dir )
                  {
                      if( dir == (d+3)%6 )
                          dw[d] = +7 - len*2;
                      else if( dir == (d+2)%6 || dir == (d+4)%6 )
                          dw[d] = -5 + len*2;
                      else
                          dw[d] = -5;
                  }
              break;
          }
            
          case 2:
          {
              // Normal road
              for( int d = 0; d < 6; ++d )
                  if( (index & (1<<d)) == 0 )
                  {
                      if( ( index & (1<<((d+2)%6)) ) != 0
                          && ( index & (1<<((d+4)%6)) ) != 0 )
                      {
                          // This is a bent road
                          dw[d] = +7;
                      }
                      else if( ( index & (1<<((d+1)%6)) ) == 0 )
                      {
                          // Extend the empty space to dir<d> o dir<d+1>
                          HexCoord f = Neighbor(n[d],HexDirection((d+1)%6));
                          if( valid(f) )
                              prefs_.data[f.m][f.n] -= 4;
                      }
                  }
                
              break;
          }
        }
        
        {
            // Now modify prefs based on dw
            for( int d = 0; d < 6; ++d )
                prefs_.data[n[d].m][n[d].n] += dw[d];
        }
    }
        
    if( build_speed > 0 && money > 10000 )
    {
        int pos = Random<23>::generate( NUM_HEXES );
        for( int i = 0; i < NUM_HEXES; ++i )
        {
            HexCoord h; hex_position( (pos+i)%NUM_HEXES, h );

            if( terrain(h) == Clear || terrain(h) == Trees )
            {
                int p = prefs_.data[h.m][h.n];
                
                const int ranmax[5] = {10000,300,150,50,20};
                // We might want to draw something here
                if( p > 0 && ShortRandom(ranmax[build_speed]) < p )
                {
                    // Draw a road here
                    if( water(h) > 0 || !erosion(h) )
                    {
                        // Draw a bridge if the demand is high AND
                        // if either there's water here, or erosion
                        // is off (meaning it's a trench)
                        if( p > 20 )
                            map_commands.push( Command::build( Bridge, h ) );
                    }
                    else if( map_commands.count_unprotected() < MAX_CMDS/8 )
                        map_commands.push( Command::build( Road, h ) );
                    // Neighbors don't have anything done
                    for( int d = 0; d < 6; ++d )
                    {
                        HexCoord n = Neighbor( h, HexDirection(d) );
                        if( valid(n) )
                            prefs_.data[n.m][n.n] = 0;
                    }
                }
            }
        }
    }
}
#endif

// The calculators are used for economics algorithms like labor and food.
// The basic idea is that these quantities "flow" around the map, mostly
// through roads.  There are "sources" that produce something and "sinks"
// that consume something.  For example, for food, farms are sources and
// houses are sinks.
struct GenericCalculator
{
    Map& M;
    GenericCalculator( Map& M_ ): M(M_) {}

    inline bool disallowed( const HexCoord& h )
    {
        Terrain t = M.terrain(h);
        return t == Clear || t == Trees || t == Fire || t == Scorched;
    }
    
    int fluidity_( const HexCoord&, const HexCoord&, Terrain sink );
};

inline int GenericCalculator::fluidity_( const HexCoord& h1, 
                                         const HexCoord& h2, 
                                         Terrain sink )
{
    // HACK : ignore sink for now
    
    // Out of 256
    Terrain t1 = M.terrain(h1);
    Terrain t2 = M.terrain(h2);
    if( t1 == Road || t1 == Bridge || t1 == Market )
    {       
        if( t2 == Road || t2 == Bridge || t2 == Market )
            return 204;
        return 0;
    }
    else
    {
        if( t2 == Road || t2 == Bridge || t2 == Market )
            return 256;
        return 0;
    }
}

struct FoodCalculator: public GenericCalculator
{
    int source_total;
    int sink_total;

    FoodCalculator( Map& M_ )
        : GenericCalculator(M_), source_total(0), sink_total(0) {}
    ~FoodCalculator();

    void produce( const HexCoord& h );
    void absorb( const HexCoord& h, int n );

    inline void reset( const HexCoord& h )
    { M.food_[h] = 0; }
    
    inline int fluidity( const HexCoord& h1, const HexCoord& h2 )
    { return fluidity_( h1, h2, Houses ); }

    inline bool scan( const HexCoord& h )
    { return M.food_[h] > 0; }

    inline int difference( const HexCoord& h1, const HexCoord& h2 )
    { return (M.food_[h1] - M.food_[h2])/2; }
};

FoodCalculator::~FoodCalculator()
{
    M.total_food = source_total;
    M.total_fed = sink_total;

    extern int money;
    money += sink_total / 100;
}

inline void FoodCalculator::produce( const HexCoord& h )
{
    int food = M.food_[h];
    source_total += food;

    if( M.terrain(h) == Farm )
    {
        // Move the food to the nearest market
        HexCoord market( M.nearest_market(h) );
        M.food_[h] -= food;
        M.food_[market] += food;
    }
}

inline void FoodCalculator::absorb( const HexCoord& h, int n )
{
    if( n != 0 )
    {
        M.food_[h] += n;
        if( M.food_[h] < 0 )
            M.food_[h] = 0;
    }
    
    if( 0 && M.terrain(h) == Market )
    {
        int food0 = M.food_[h];
        int dfood = food0/64;   // Some food spoils
        M.food_[h] -= dfood;
    }
    
    if( M.terrain(h) == Houses )
    {
        // Houses should grab food from the nearest market
        HexCoord market = M.nearest_market(h);
        
        // Consume food
        int buildings = M.extra_[h];
        int food_available = M.food_[market]/2;
        int food_needed = M.residents(h);
        if( ByteRandom(16) == 0 )
        {
            if( food_available > food_needed*3 && buildings < 7 )
            {
                // Lots of food -- increase number of houses
                ++buildings;
                M.extra_[h] = buildings;
                M.damage(h);
            }
            else if( food_available < food_needed )
            {
                if( buildings > 0 )
                {
                    // Not enough food -- decrease number of houses
                    --buildings;
                    M.extra_[h] = buildings;
                    M.damage(h);
                }
                else if( ByteRandom(16)==0 )
                {
                    // Houses disappear
                    M.set_terrain( h, Clear );
                }
            }
        }

        int dfood = min( food_needed, food_available );
        if( dfood > 0 )
        {
            M.food_[market] -= dfood;
            sink_total += dfood;
        }
    }
}

struct LaborCalculator: public GenericCalculator
{
    int tw, tl, tj;
    LaborCalculator( Map& M_ ): GenericCalculator(M_), tw(0), tl(0), tj(0) {}
    ~LaborCalculator();

    void produce( const HexCoord& h );
    void absorb( const HexCoord& h, int n );

    inline void reset( const HexCoord& h )
    { M.labor_[h] = 0; }
    
    inline int fluidity( const HexCoord& h1, const HexCoord& h2 )
    { return fluidity_( h1, h2, Farm ); }

    inline bool scan( const HexCoord& h )
    { return M.labor_[h] > 0; }

    inline int difference( const HexCoord& h1, const HexCoord& h2 )
    { return (M.labor(h1) - M.labor(h2))/2; }
};

LaborCalculator::~LaborCalculator()
{
    // Store statistics about the labor force
    M.total_labor = tl;
    M.total_working = tw;
    M.total_jobs = tj;
}

inline void LaborCalculator::produce( const HexCoord& h )
{
    if( M.terrain(h) == Houses )
    {
        int hw = M.residents(h) + M.labor(h)*7/8;
        if( hw > 0 )
        {
            tl += hw;
            
            HexCoord market( M.nearest_market(h) );
            if( market != h )
            {
                int mw = M.labor(market);
                
                if( mw < 3000 )
                {
                    // Move some labor to the nearest market
                    int w = min(3000-mw, hw);
                    hw -= w;
                    mw += w;
                    M.set_labor( market, mw );
                }
            }

            // Put the leftover labor in the house
            M.set_labor( h, hw );
        }
    }
}

inline void LaborCalculator::absorb( const HexCoord& h, int n )
{
    if( n != 0 )
    {
        M.set_labor( h, M.labor(h) + n );
        if( M.labor(h) < 0 )
            M.set_labor( h, 0 );
    }
    
    if( M.terrain(h) == Market )
    {
        int l = M.labor(h);
        int d = l/128;   // Some workers give up looking for a job
        if( d > 0 ) M.set_labor( h, l-d );
    }

    if( M.terrain(h) == Farm )
    {
        // Farms should look for workers at the nearest market
        HexCoord market = M.nearest_market(h);

        // Grab all available workers
        int workers_available = M.labor(market)/2;
        int workers_desired = 100;
        int workers = min( workers_desired, workers_available );

        M.set_labor( market, M.labor(market) - workers );
        
        // Produce food
        M.food_[h] += workers;

        tw += workers;
        tj += workers_desired;

        // Consider self destruction, if the farm has been around for a while
        if( M.extra_[h] > 10 && ByteRandom(4)==0 && workers == 0 )
            M.set_terrain( h, Clear );
    }
}

// The calculator function runs the flow algorithm
template <class Specific>
void Calculator( Specific& S )
{
    Map& M( S.M );

    int start_hex = ShortRandom(NUM_HEXES);
    for( int i = 0; i < NUM_HEXES; ++i )
    {
        HexCoord h; hex_position((start_hex+i)%NUM_HEXES,h);
        S.produce(h);
        M.temp_[h] = 0;
    }

    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            int scale = 0;
            
            if( !S.scan( h ) ) 
                continue;

            if( S.disallowed( h ) )
            {
                S.reset( h );
                continue;
            }
        
            int moved[7];
            int per256[7];
            for( int dir = 0; dir < 7; ++dir )
            {
                HexCoord h2 = (dir==6)?
                    M.nearest_market(h) :
                    Neighbor( h, HexDirection(dir) );
                // Set the amount actually moved to 0,
                // unless we later find some
                moved[dir] = 0;

                if( !Map::valid(h2) || h2 == h ) continue;

                int f = S.fluidity(h,h2);

                // If there is some neighbor that accepts labor
                if( f > 0 )
                {
                    // Remember how much they'd like to take
                    scale += f;

                    // And calculate the amount they will actually take
                    int difference = S.difference(h,h2);
                    // If it's positive, then we have some
                    if( difference > 0 )
                    {
                        moved[dir] = difference;
                        per256[dir] = f;
                    }
                }
            }

            if( scale > 0 )
            {
                for( int dir = 0; dir < 7; ++dir )
                {
                    if( moved[dir] > 0 )
                    {
                        HexCoord h2 = (dir==6)?
                            M.nearest_market(h) :
                            Neighbor( h, HexDirection(dir) );

                        int t = moved[dir] * per256[dir] / scale;
                        M.temp_[h] -= t;
                        M.temp_[h2] += t;
                        // Record how much moved here, if desired
                    }
                }
            }
        }

    for( int i = 0; i < NUM_HEXES; ++i )
    {
        HexCoord h; hex_position((start_hex+i)%NUM_HEXES,h);
        
        // Retrieve the temp amount, which is the change in labor, and
        // then reset the temp back to 0 for other routines to use
        int t = M.temp_[h];
        M.temp_[h] = 0;
        S.absorb( h, t );
    }
}

void Map::calculate_labor()
{
    FoodCalculator F(*this);
    Calculator( F );

    LaborCalculator L(*this);
    Calculator( L );
}

void Map::add_farms()
{
    static int pos = 0;
    for( int i = 0; i < NUM_HEXES/3000; ++i )
    {
        HexCoord h; hex_position( pos++, h );
        if( pos >= NUM_HEXES ) pos = 0;

        // We are looking for adjacent Clear & Road

        // The prefs value has to be negative if this is a good site
        // Also, there should be no water here
        if( terrain( h ) == Road )
        {
            for( int dir = 0; dir < 6; ++dir )
            {
                HexCoord h2 = Neighbor( h, HexDirection(dir) );
                if( !valid(h2) ) continue;
                
                if( terrain( h2 ) == Clear &&
                    prefs_[h2] < 0 &&
                    water(h2) <= 0 )
                {
                    // The rank is negative if farms are preferable
                    // and positive if houses are preferable
                    int rank = 0;
                    
                    // Adjust the ranking based on what resources are
                    // on the adjacent road; note that these calculations
                    // ARE repeated inside the loop, but lifting them would
                    // probably slow things down, since there are lots of
                    // roads and few times that we build
                    if( food_[h] > 1000 )   rank += 2;
                    if( food_[h] < 100 )    rank -= 1;
                    if( labor_[h] > 1000 )  rank -= 2;
                    if( labor_[h] < 100 )   rank += 1;
                    
                    rank += ByteRandom(MAX_MOISTURE-1);
                    rank -= moisture(h2) - altitude(h2)/(NUM_TERRAIN_TILES/3);
                    if( rank < 0 )
                        set_terrain( h2, Farm );
                    else
                        set_terrain( h2, Houses );
                }
            }
        }

        if( terrain( h ) == Farm && ByteRandom(2) == 0 )
        {
            // Increase the age of the farm
            ++extra_[h];
            damage(h);
        }

        #if 0
        // I used to use this for increasing the population in houses,
        // but now I'm using a new algorithm elsewhere
        if( terrain( h ) == Houses )
        {
            bool gaining = bool( labor(h) < 256 );

            if( gaining )
            {
                if( extra_.data[h.m][h.n] < 7 )
                {
                    ++extra_.data[h.m][h.n];
                    damage( h );
                }
            }
            else
            {
                if( extra_.data[h.m][h.n] > 0 )
                {
                    --extra_.data[h.m][h.n];
                    damage( h );
                }
                else
                    set_terrain( h, Clear );
            }
        }
        #endif
    }
}

void Map::add_trees()
{
    static int pos = 0;
    for( int i = 0; i < NUM_HEXES/500; ++i )
    {
        HexCoord h; hex_position( pos++, h );
        if( pos >= NUM_HEXES ) pos = 0;

        Terrain t = terrain( h );

        if( t == Trees || t == Fire || t == Scorched )
        {
            int t1 = 30 + ByteRandom(20);
            if( t == Fire )
                t1 = t1 / 12;
            if( ++extra_[h] > t1 )
            {
                // Tree, Fire, or Scorched dies
                // (make sure extra does not exceed 255)
                if( t == Fire )
                    t = Scorched;
                else
                    t = Clear;
                set_terrain( h, t );
            }
            else
                damage( h );

            // Old trees may lead to young trees nearby
            if( t == Trees && extra_[h] >= TREE_MATURITY &&
                ShortRandom(MAX_MOISTURE*12) <= 2+moisture(h) )
            {
                for( int dir = 0; dir < 6; ++dir )
                {
                    HexCoord h2 = Neighbor( h, HexDirection(dir) );
                    if( !valid(h2) ) continue;
                    if( terrain( h2 ) == Clear && ByteRandom(2) == 1 )
                        set_terrain( h2, Trees );
                }
            }

            // If the fire has been burning for some time, it might spread
            if( t == Fire && extra_[h] >= 2 )
            {
                // Possibly spread to any neighboring hex
                for( int dir = 0; dir < 6; ++dir )
                {
                    HexCoord h2 = Neighbor( h, HexDirection(dir) );
                    if( !valid(h2) ) continue;
                    Terrain t2 = terrain(h2);
                    if( t2==Trees||t2==Houses||t2==Farm
                        ||t2==Road||t2==Bridge||t2==Market )
                        if( ByteRandom(4) < 3 )
                            set_terrain( h2, Fire );
                }
            }
        }
    }

    if( ShortRandom(1000) == 0 )
    {
        // Pick a random point on the map to add a tree
        HexCoord h( 1+ShortRandom(Map::MSize), 1+ShortRandom(Map::NSize) );
        if( terrain( h ) == Clear )
            set_terrain( h, Trees );
    }
}

void Map::create_volcano( const HexCoord& h )
{
    Mutex::Lock lock( mutex );
    volcano_ = h;
    volcano_time_ = 0;
}

void Map::lava_flow()
{
    if( !valid(volcano_) )
        return;

    ++volcano_time_;
    if( volcano_time_ > 120 )
    {
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                if( terrain(h) == Lava )
                    set_terrain(h,Clear);
            }
        volcano_ = HexCoord(0,0);
        return;
    }

    extern void make_mountain( Map& map, int m, int n, int h );
    if( volcano_time_ > 100 )
        make_mountain( *this, volcano_.m, volcano_.n, -1 );
    else if( volcano_time_ % 6 == 1 &&
        altitude( volcano_ ) < NUM_TERRAIN_TILES * 5 / 6 )
        make_mountain( *this, volcano_.m, volcano_.n, 15 );

    static int pos = 0;
    // Basically, lava flows from higher hexes to lower hexes, and
    // when the lava solidifies, it turns into land
    for( int i = 0; i < NUM_HEXES/4; ++i )
    {
        HexCoord h; hex_position( pos++, h );
        if( pos >= NUM_HEXES ) pos = 0;

        if( terrain(h) == Lava )
        {
            HexCoord bh = h;
            int alt = altitude( h );
            for( int d = 0; d < 6; ++d )
            {
                int dd = d;
                if( ByteRandom(15) == 0 )
                    dd = (d+5)%6;
                else if( ByteRandom(15) == 0 )
                    dd = (d+1)%6;
                HexCoord h2 = Neighbor( h, HexDirection(dd) );
                if( valid(h2) )
                {
                    int a = altitude( h2 );
                    if( a <= alt ) { alt = a; bh = h2; }
                }
            }

            if( ++extra_[h] > 5 )
                set_terrain( h, Fire );
            if( volcano_time_ < 100 )
            {
                set_terrain( bh, Lava );
                set_water( bh, 0 );
                if( altitude(bh) < NUM_TERRAIN_TILES/2 ||
                    altitude(bh) < altitude(h)-1 )
                {
                    set_altitude( bh, altitude(bh) + 1 );
                    set_erosion( bh, true );
                }
            }
        }
    }

    if( volcano_time_ < 75 )
        set_terrain( volcano_, Lava );
}

void Map::simulate_environment()
{
    Mutex::Lock lock( mutex, 1000 );
    if( !lock.locked() )
        return;

    water_flow();
    lava_flow();
    
    if( time_tick_ % ( 8/NUM_WATER_SOURCES ) == 0 )
        water_from_springs();

    // Possibly add new farms next to roads
    add_farms();

    // Possibly add new trees
    add_trees();

    // Economy calculations
    if( time_tick_ % 64 == 45 )
        calculate_labor();

    // Area preference calculations
    if( time_tick_ % 128 == 5 )
        calculate_prefs();

    if( time_tick_ % 64 == 9 )
        collect_sector_statistics();
    
    // Redistribute terrain
    if( time_tick_ % 2048 == 0 )
    {
        recalculate_histogram();
        if( histogram_disturbed > 600 )
            super_smooth_terrain();
    }
    if( time_tick_ % 16 == 0 && histogram_disturbed > 500 )
        redistribute_terrain( 1 );

    if( time_tick_ % 128 == 87 )
    {
        calculate_center();
        calculate_moisture();
    }

    // Smooth out bumps
    if( ( histogram_disturbed > 50 && time_tick_ % 4 == 3 )
        || ( time_tick_ % 16 == 3 ) )
        smooth_terrain(NUM_HEXES/40);

    // Water destroys structures
    water_evaporation();
    water_destruction();
}

void Map::simulate()
{
    if( time_tick_ == 0 ) randomize();
    ++time_tick_;
    simulate_environment();
    simulate_military();

#if 0
    // I tried out a train but never figured out where to put it in
    // the game, so it's commented out.
    train.move(this);
#endif
}

