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

#ifndef Map_h
#define Map_h

// DEVELOPMENT can be 0, 1, 2 depending on how many range checks to make
#define DEVELOPMENT 2

#if DEVELOPMENT
  #define CHECK_VALIDITY(h) if(!Map::valid(h)) Throw("Invalid Map ref");
#else
  #define CHECK_VALIDITY(h)
#endif

// Pick a horizontal map size, then calculate the vertical one based on
// the size of a hexagon.  The intent is to make the map square.
const int MAP_SIZE_X = 144;
const int MAP_SIZE_Y = (MAP_SIZE_X*4*21/(3*24));

#define FLAG_EROSION 0x01
typedef int value;

#include "HexCoord.h"
#include "Unit.h"
#include "Images.h"

// Player's money
extern int money;

enum Terrain { Clear,
               Road, Bridge, Farm, Wall, Houses,
               Gate, Trees, Fire, Lava, Scorched, Canal,
               WatchFire, Market,
               maxTerrain };

// Random hex traversal:
//     Loop from i = 0, i < NUM_HEXES
//     Fill in a HexCoord with hex_position(i,h)
#define NUM_HEXES (MAP_SIZE_X*MAP_SIZE_Y)
inline void hex_position( int i, HexCoord& h )
{
    extern unsigned short iterator_order[NUM_HEXES];
    unsigned short s = iterator_order[i];
    h.m = (s & 0xff);
    h.n = (s >> 8);
}

// This is an array that covers the size of the map, plus an extra one
// hexagon border.  In addition, it overloads operator [] so that I can
// put range checking in during development.
template <class T>
struct MapArray
{
    enum { MSize = MAP_SIZE_X };
    enum { NSize = MAP_SIZE_Y };
  private:
    T data[MSize+2][NSize+2];
    
  public:
    MapArray( T init_ );
    const T& operator [] ( const HexCoord& h ) const;
    T& operator [] ( const HexCoord& h );
};

template<class T> MapArray<T>::MapArray( T init_ )
{
    for( int i = 0; i < MSize+2; i++ )
        for( int j = 0; j < NSize+2; j++ )
            data[i][j] = init_;
}

template<class T>
inline const T& MapArray<T>::operator [] ( const HexCoord& h ) const
{
#if DEVELOPMENT >= 2
    CHECK_VALIDITY(h);
    if( !Map::valid(h) )
    {
        char s[256];
        sprintf(s,"Ptr = %p H = %d,%d", &(data[0][0]), h.m, h.n);
        void Log( const char* text1, const char* text2 );
        Log("MapArray<T>::operator []", s);
    }
#endif
    return data[h.m][h.n];
}

template<class T>
inline T& MapArray<T>::operator [] ( const HexCoord& h )
{
#if DEVELOPMENT >= 2
    CHECK_VALIDITY(h);
    if( !Map::valid(h) )
    {
        char s[256];
        sprintf(s,"Ptr = %p, H = %d,%d", &(data[0][0]), h.m, h.n);
        void Log( const char* text1, const char* text2 );
        Log("MapArray<T>::operator [] (non-const)", s);
    }
#endif
    return data[h.m][h.n];
}

//////////////////////////////////////////////////////////////////////
// Sectors are rectangular portions of the map
const int NUM_SECTORS_X = 8;
const int NUM_SECTORS_Y = 8;
const int NUM_SECTORS = NUM_SECTORS_X*NUM_SECTORS_Y;
const int SECTOR_X_SIZE = (MAP_SIZE_X+NUM_SECTORS_X-1)/NUM_SECTORS_X;
const int SECTOR_Y_SIZE = (MAP_SIZE_Y+NUM_SECTORS_Y-1)/NUM_SECTORS_Y;
const int HEXES_IN_SECTOR = SECTOR_X_SIZE*SECTOR_Y_SIZE;

template <class T>
struct SectorArray
{
  private:
    T data[NUM_SECTORS];
    
  public:
    SectorArray( T init_ )
    {
        for( int s = 0; s < NUM_SECTORS; ++s )
            data[s] = init_;
    }
    
    T& operator [] ( int sector )
    {
#if DEVELOPMENT >= 2
        unsigned s = unsigned(sector);
        if( s >= NUM_SECTORS ) Throw("Invalid sector");
#endif
        return data[sector];
    }
};

inline int sector( const HexCoord& h )
{
    return ((h.m-1)/SECTOR_X_SIZE)*NUM_SECTORS_X + (h.n-1)/SECTOR_Y_SIZE;
}

inline HexCoord sector_origin( int sector )
{
    return HexCoord( 1+(sector/NUM_SECTORS_X)*SECTOR_X_SIZE,
                     1+(sector%NUM_SECTORS_X)*SECTOR_Y_SIZE );
}

inline HexCoord sector_center( int sector )
{
    HexCoord h( sector_origin(sector) );
    return HexCoord( h.m + SECTOR_X_SIZE/2, h.n + SECTOR_Y_SIZE/2 );
}

struct SectorIterator
{
    HexCoord origin;
    int i;

    SectorIterator( int sector, int begin=0 )
        :origin( sector_origin(sector) ), i(begin)
    {}

    HexCoord operator * () const
    {
        return HexCoord( origin.m + i%SECTOR_X_SIZE,
                         origin.n + i/SECTOR_X_SIZE );
    }

    SectorIterator& operator ++ ()
    {
        ++i;
        return *this;
    }
};

inline bool operator == ( const SectorIterator& a, const SectorIterator& b )
{
    return a.origin == b.origin && a.i == b.i;
}

//////////////////////////////////////////////////////////////////////
// This is the main map structure
// At first I thought I would support multiple maps, but I think it
// would have been better to put these into a namespace as globals,
// and support just one map.
struct Map
{
  public:
    enum { MSize = MAP_SIZE_X };
    enum { NSize = MAP_SIZE_Y };
    Mutex mutex;
    Mutex unit_mutex;
    Mutex selection_mutex;

    Map();
    ~Map();

    static bool valid( const HexCoord& h );

    Terrain terrain( const HexCoord& h ) const { return terrain_[h]; }
    void set_terrain( const HexCoord& h, Terrain terrain );
    value water( const HexCoord& h ) const { return water_[h]; }
    void set_water( const HexCoord& h, value water );
    value labor( const HexCoord& h ) const { return labor_[h]; }
    void set_labor( const HexCoord& h, value labor );
    value altitude( const HexCoord& h ) const { return altitude_[h]; }
    void set_altitude( const HexCoord& h, value altitude );
    value moisture( const HexCoord& h ) const { return moisture_[h]; }
    void set_moisture( const HexCoord& h, value moisture );
    bool erosion( const HexCoord& h ) const { return (flags_[h] & FLAG_EROSION) != 0; }
    void set_erosion( const HexCoord& h, bool erosion );

    void damage( const HexCoord& h );
    void damage_neighboring( const HexCoord& h, Terrain t );
    void damage_neighboring_roads( const HexCoord& h );
    void damage_neighboring_walls( const HexCoord& h );

    void initialize( Closure<bool,const char *> action );
    void super_smooth_terrain();

    void calculate_moisture();
    void smooth_terrain( int num_hexes = NUM_HEXES/15 );
    void water_flow();
    void water_from_springs();
    void water_evaporation();
    void water_destruction();
    void add_farms();
    void add_trees();

    int alt[NUM_TERRAIN_TILES], out[NUM_TERRAIN_TILES];
    int histogram_disturbed;
    void recalculate_histogram();
    void redistribute_terrain( int percentage );

    HexCoord volcano_;
    int volcano_time_;
    void create_volcano( const HexCoord& h );
    void lava_flow();

    int total_labor;
    int total_working;
    int total_jobs;

    int total_food;
    int total_fed;
    void calculate_labor();

    HexCoord city_center_;
    void calculate_prefs();
    void calculate_center();

    Subject<bool> drought;
    int year() const;
    int month() const;
    int day() const;
    int day_of_year() const;
    int days_in_year() const;
    const char* monthname() const;

    Subject<int> game_speed;
    void simulate();
    void simulate_environment();
    void simulate_military();
    int simulation_thread(int);
    void process_commands();
    
    // Units
    MapArray<short> occupied_;  // Does not need to be saved, can be
    static vector<Unit*> units; // regenerated from units list

    // Watchtowers
    static vector<WatchtowerFire> watchtowers_;
    
    // Selected region (dragging)
    static vector<HexCoord> selected;
    static HexCoord select_begin, select_end;
    void cancel_selection();
    void damage_selection();
    void set_begin( HexCoord h );
    void set_end( HexCoord h );
    // void selection_to_terrain( Terrain t );

    // Sector iterators
    static inline SectorIterator sector_begin(int s)
    { return SectorIterator(s); }
    static inline SectorIterator sector_end(int s)
    { return SectorIterator(s,HEXES_IN_SECTOR); }
    
    // Location of the nearest market; this _could_ be a byte.. hm
    MapArray<unsigned short> nearest_market_; // LObyte = +/- M, HIbyte = +/- N

    HexCoord nearest_market( const HexCoord& h )
    {
        unsigned short d = nearest_market_[h];
        signed char dM = d & 0xff;
        signed char dN = d >> 8;
        return HexCoord( h.m + dM, h.n + dN );
    }

    void set_nearest_market( const HexCoord& h, const HexCoord& market )
    {
        unsigned char dM = market.m - h.m;
        unsigned char dN = market.n - h.n;
        nearest_market_[h] = dM | (dN << 8);
    }

    int residents( const HexCoord& h ) const;
    
    // private:
    MapArray<Terrain> terrain_;
    MapArray<value> altitude_;
    MapArray<value> water_;
    MapArray<value> moisture_;
    MapArray<value> labor_;
    MapArray<value> food_;
    MapArray<value> temp_;
    MapArray<value> heat_;
    MapArray<byte> extra_;
    MapArray<byte> flags_;
    MapArray<short int> prefs_;
    MapArray<long> damage_;
    MapArray<long> C_land_value_, R_land_value_, A_land_value_;
    HexCoord *water_sources_; // array
    long time_tick_;
    friend class View;

    SectorArray<byte> num_fires_;
    SectorArray<byte> num_trees_; // only those old enough to be cut down
    SectorArray<byte> num_jobs_; // for builders
    void collect_sector_statistics();
};

inline bool Map::valid( const HexCoord& h )
{
    return h.m >= 1 && h.m <= MSize && h.n >= 1 && h.n <= NSize;
}

inline void Map::damage( const HexCoord& h )
{
    CHECK_VALIDITY(h);
    damage_[h] = time_tick_+1;
}

inline int Map::residents( const HexCoord& h ) const
{
    CHECK_VALIDITY(h);
    return ( valid(h) && terrain(h) == Houses )? (15 + 10*(extra_[h])) : 0;
}

inline void Map::set_water( const HexCoord& h, value water )
{
    CHECK_VALIDITY(h);
    int old_water = water_[h];
    if( water != old_water )
    {
        water_[h] = water;
        damage_[h] = time_tick_+1;
    }
}

inline void Map::set_labor( const HexCoord& h, value labor )
{
    CHECK_VALIDITY(h);
    labor_[h] = labor;
}

inline void Map::set_altitude( const HexCoord& h, value altitude )
{
    CHECK_VALIDITY(h);
    int old_altitude = altitude_[h];
    if( altitude != old_altitude )
    {
        altitude_[h] = altitude;
        damage_[h] = time_tick_+1;
    }
}

inline void Map::set_moisture( const HexCoord& h, value moisture )
{
    CHECK_VALIDITY(h);
    moisture_[h] = moisture;
}

inline void Map::set_erosion( const HexCoord& h, bool erosion )
{
    CHECK_VALIDITY(h);
    if( erosion )
        flags_[h] |= FLAG_EROSION;
    else
        flags_[h] &= ~FLAG_EROSION;
}

#endif

