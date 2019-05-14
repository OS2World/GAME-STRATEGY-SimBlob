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
#include "stl.h"
#include "algo.h"

#include "Notion.h"
#include "Map.h"
#include "Map_Const.h"

// This instance of the neighbor array is used for Neighbor()
NEIGHBOR_DECL;

vector<Unit*> Map::units;
vector<WatchtowerFire> Map::watchtowers_;
vector<HexCoord> Map::selected;
HexCoord Map::select_begin, Map::select_end;

//////////////////////////////////////////////////////////////////////
// Random order for map iteration
unsigned short iterator_order[NUM_HEXES];
struct RandomNumberGen
{
    // ASSUME that 22 bits is enough!
    int operator ()(int n) { return Random<22>::generate(n); }
};

static void initialize_order()
{
    vector<HexCoord> hexes(NUM_HEXES);
    int i = 0;
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
            hexes[i++] = HexCoord(m,n);

    RandomNumberGen gen;
    random_shuffle( hexes.begin(), hexes.end(), gen );
    for( i = 0; i < NUM_HEXES; i++ )
        iterator_order[i] = ( hexes[i].m ) | ( hexes[i].n << 8 );
}

//////////////////////////////////////////////////////////////////////
Map::Map()
    : terrain_(Canal), altitude_(NUM_TERRAIN_TILES-1), water_(0),
      damage_(0), time_tick_(0), nearest_market_(0),
      C_land_value_(0), R_land_value_(0), A_land_value_(0),
      extra_(0), moisture_(0), prefs_(0), 
      labor_(0), total_working(0), total_labor(0), total_jobs(0),
      food_(0), total_food(0), total_fed(0), flags_(0),
      game_speed(40), drought(false), heat_(0), 
      volcano_(0,0), volcano_time_(0), histogram_disturbed(0),
      temp_(0), occupied_(-1), city_center_(MSize/2,NSize/2),
      num_fires_(0), num_trees_(0), num_jobs_(0)
{
    randomize();
    initialize_order();

    units.reserve(1000);
    water_sources_ = new HexCoord[NUM_WATER_SOURCES];
    for( int m = 1; m <= Map::MSize; ++m )
        for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            terrain_[h] = Clear;
            flags_[h] = FLAG_EROSION;
        }

    for( int k = 0; k < NUM_WATER_SOURCES; ++k )
    {
        // Place the water sources in an ellipse, initially,
        // to guarantee that they're somewhat evenly distributed
        double angle = k * 2*M_PI / NUM_WATER_SOURCES;
        double m0 = Map::MSize/2;
        double n0 = Map::NSize/2;
        double rm = Map::MSize/2 * 3/4;
        double rn = Map::NSize/2 * 3/4;
    
        water_sources_[k] =
            HexCoord( m0 + int(rm*cos(angle)), n0 + int(rn*sin(angle)) );
    }
}

Map::~Map()
{
    delete[] water_sources_;
}

void Map::damage_neighboring( const HexCoord& h, Terrain terr )
{
    NEIGHBOR_DECL;
    const POINTL *boundary = neighbors[h.m%2];
    for( int i = 0; i < 6; i++ )
    {
        int dm = boundary[i].x;
        int dn = boundary[i].y;
        HexCoord h2( h.m+dm, h.n+dn );
        if( valid(h2) && terrain_[h2] == terr )
            damage_[h2] = time_tick_+1;
    }
}

inline void Map::damage_neighboring_roads( const HexCoord& h )
{
    CHECK_VALIDITY(h);
    damage_neighboring( h, Road );
    damage_neighboring( h, Bridge );
}

inline void Map::damage_neighboring_walls( const HexCoord& h )
{
    CHECK_VALIDITY(h);
    damage_neighboring( h, Wall );
}

struct MapCmp
{
    HexCoord h;
    MapCmp(HexCoord _h): h(_h) {}
    bool operator () ( const WatchtowerFire& f )
    { return f.location == h; }
};
            
void Map::set_terrain( const HexCoord& h, Terrain terr )
{
    CHECK_VALIDITY(h);
    Terrain hexterrain = terrain_[h];
    if( hexterrain != terr )
    {
        if( hexterrain == WatchFire )
        {
            vector<WatchtowerFire>::iterator i =
                find_if( watchtowers_.begin(), watchtowers_.end(), MapCmp(h) );

            if( i != watchtowers_.end() )
                    watchtowers_.erase( i, i+1 );
        }
        
        if( hexterrain == Road || terr == Road
            || hexterrain == Bridge || terr == Bridge )
            damage_neighboring_roads( h );
        if( hexterrain == Wall || terr == Wall 
            || hexterrain == Gate || terr == Gate 
            || hexterrain == Canal || terr == Canal )
            damage_neighboring_walls( h );

        if( hexterrain == Fire )
        {
            // Erasing fire decreases the fire count
            --num_fires_[sector(h)];
        }
        else if( terr == Fire )
        {
            // Adding fire increases the fire count
            // (Note -- it's all recalculated periodically anyway)
            ++num_fires_[sector(h)];
        }
        
        if( terr == Canal )
        {
            if( flags_[h] & FLAG_EROSION )
                set_erosion( h, false );
        }
        else
        {
            terrain_[h] = terr;
            extra_[h] = 0;
            if( terr == WatchFire )
            {
                watchtowers_.push_back( WatchtowerFire(h) );
            }
        }
        damage_[h] = time_tick_+1;
    }
    else if( terr == Clear )
    {
        // If the terrain is already clear, then we can `clear' it again
        // by getting rid of the canal
        if( ( flags_[h] & FLAG_EROSION ) == 0 )
        {
            set_erosion( h, true );
            damage_[h] = time_tick_+1;
        }
    }
}

int Map::year() const
{
    return 637 + ( ( time_tick_ / TICKS_PER_DAY ) / DAYS_PER_MONTH ) / MONTHS_PER_YEAR;
}

int Map::month() const
{
    return ( ( time_tick_ / TICKS_PER_DAY ) / DAYS_PER_MONTH ) % MONTHS_PER_YEAR;
}

int Map::day() const
{
    return 1 + ( time_tick_ / TICKS_PER_DAY ) % DAYS_PER_MONTH;
}

int Map::days_in_year() const
{
    return MONTHS_PER_YEAR * DAYS_PER_MONTH;
}

int Map::day_of_year() const
{
    return 1 + ( time_tick_ / TICKS_PER_DAY ) % days_in_year();
}

const char* Map::monthname() const
{
    const char* names[MONTHS_PER_YEAR] = {
    "Hol",
    "Jan","Feb","Mar","Apr","May","Jun",
    "Jul","Aug","Sep","Oct","Nov","Dec"};
    return names[month()];
}

HexCoord PointToHex( int xp, int yp )
{
    xp -= HexCoord(1,0).left();
    yp -= HexCoord(0,0).y();
    int row = 1 + yp / 12;
    int col = 1 + xp / 21;
    int diagonal[2][12] = {
        {7,6,6,5,4,4,3,3,2,1,1,0},
        {0,1,1,2,3,3,4,4,5,6,6,7}
    };

    if( diagonal[(row+col)%2][yp%12] >= xp%21 )
        col--;
    return HexCoord( col, (row-(col%2))/2 );
}

/*
HexCoord PointToHex( int xp, int yp )
{
    double x = 1.0 * ( xp - HexCoord(0,0).x() ) / HexXSpacing;
    double y = 1.0 * ( yp - HexCoord(0,0).y() ) / HexYSpacing;
    double z = -0.5 * x - y;
           y = -0.5 * x + y;
    int ix = floor(x+0.5);
    int iy = floor(y+0.5);
    int iz = floor(z+0.5);
    int s = ix+iy+iz;
    if( s )
    {
        double abs_dx = fabs(ix-x);
        double abs_dy = fabs(iy-y);
        double abs_dz = fabs(iz-z);
        if( abs_dx >= abs_dy && abs_dx >= abs_dz )
            ix -= s;
        else if( abs_dy >= abs_dx && abs_dy >= abs_dz )
            iy -= s;
        else
            iz -= s;
    }
    return HexCoord( ix, ( iy - iz + (1-ix%2) ) / 2 );
}
*/

void VectorDir( HexDirection d, int& dx, int& dy )
{
    int DX = HexXSpacing, DY = HexYSpacing;
    if( d == DirN ) { dx = 0; dy = DY; }
    else if( d == DirNE ) { dx = DX; dy = DY/2; }
    else if( d == DirSE ) { dx = DX; dy = -DY/2; }
    else if( d == DirS ) { dx = 0; dy = -DY; }
    else if( d == DirSW ) { dx = -DX; dy = -DY/2; }
    else if( d == DirNW ) { dx = -DX; dy = DY/2; }
}

// Selection handling routines
void Map::cancel_selection()
{
    damage_selection();

    for( int i = 0; i < 10; i++ )
    {
        Mutex::Lock lock( selection_mutex, 500 );
        if( lock.locked() )
        {
            selected.erase( selected.begin(), selected.end() );
            break;
        }
    }
}

void Map::set_begin( HexCoord h )
{
    cancel_selection();

    for( int i = 0; i < 10; i++ )
    {
        Mutex::Lock lock( selection_mutex, 500 );
        if( lock.locked() )
        {
            select_begin = h;
            break;
        }
    }
}

#include "Path.h"

void Map::set_end( HexCoord h )
{
    cancel_selection();

    {
        Mutex::Lock lock( selection_mutex, 500 );
        if( lock.locked() )
        {           
            select_end = h;
            selected.erase( selected.begin(), selected.end() );

            extern Terrain current_tool;
            FindBuildPath( *this, select_end, select_begin, selected );
        }
    }
    damage_selection();
}

void Map::damage_selection()
{
    for( int i = 0; i < 10; i++ )
    {
        Mutex::Lock lock( selection_mutex, 500 );
        if( lock.locked() )
        {
            for( vector<HexCoord>::iterator j = selected.begin(); 
                 j != selected.end(); ++j )
            {
                HexCoord h = *j;
                damage( h );
            }
            break;
        }
    }
}

