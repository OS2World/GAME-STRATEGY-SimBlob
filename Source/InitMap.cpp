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
#include "Path.h"

#define SIMPLE_MAP 1

#include "uplift.h"

// My attempt to create a fractal terrain wasn't entirely successful.
static void fractal_terrain( Map& map, int m0, int n0, int d, int h )
{
    if( d <= 4 )
        return;

    // Pick a random slope
    double ds = h * 0.01 / (d*d) * ( ShortRandom(1000)-ShortRandom(1000) );
    int mcenter = m0 + d/2;
    int ncenter = n0 + d/2;
    for( int m = m0; m <= m0+d; ++m )
        for( int n = n0; n <= n0+d; ++n )
        {
            HexCoord h(m,n);
            if( map.valid(h) )
            {
                int da = int((d/2-abs(m-mcenter))*(d/2-abs(n-ncenter))*ds);
                map.set_altitude( h, map.altitude(h) + da );
            }
        }

    // Now do the same for smaller areas
    int dd = d/2;
    int hh = h*2/3;
    fractal_terrain( map, m0, n0, dd, hh );
    fractal_terrain( map, m0+dd, n0, dd, hh );
    fractal_terrain( map, m0, n0+dd, dd, hh );
    fractal_terrain( map, m0+dd, n0+dd, dd, hh );
}

// Create a circular bump or a dip at (m,n).  height isn't really the height.
void make_mountain( Map& map, int m, int n, int height )
{
    int r = abs(height)*2;
    for( int dm = -r; dm <= r; ++dm )
        for( int dn = -r; dn <= r; ++dn )
        {
            HexCoord h(m+dm,n+dn);
            if( map.valid(h) )
            {
                int p = ( 200 * height / ( 100 + dm*dm + dn*dn ) );
                if( p != 0 )
                    map.set_altitude( h, map.altitude( h ) + p );
            }
        }
}

#define Pause(s) if( pause(s) ) return;

struct MapInitializer
{
    Map& map;
    Closure<bool,const char *> pause;
    HexCoord water_sink;
    
    MapInitializer( Map& m, Closure<bool,const char*> p )
        :map(m), pause(p), water_sink(Map::MSize/2,Map::NSize/2)
    {
        // Assert( _heapset(0xABCD5678) == _HEAPOK );
        Assert( _heapchk() == _HEAPOK );
    }

    void flat_map()
    {
        for( int n = 1; n <= Map::NSize; ++n )
        {
            for( int m = 1; m <= Map::MSize; ++m )
                map.set_altitude( HexCoord(m,n), NUM_TERRAIN_TILES/3 );
        }
    }
    
    void random_mountains()
    {
        // Make round ups and downs
        Pause( "Adding Mountains and Valleys" );

        int H = NUM_TERRAIN_TILES/10;
        make_mountain( map, 1, Map::NSize/3, H );
        make_mountain( map, 1, Map::NSize*2/3, H );
        make_mountain( map, Map::MSize, Map::NSize/3, H );
        make_mountain( map, Map::MSize, Map::NSize*2/3, H );
        make_mountain( map, Map::MSize/3, 1, H );
        make_mountain( map, Map::MSize*2/3, 1, H );
        make_mountain( map, Map::MSize/3, Map::NSize, H );
        make_mountain( map, Map::MSize*2/3, Map::NSize, H );
        for( int size = NUM_TERRAIN_TILES/3; size > 16; size /= 3 )
        {
            Pause( "Adding Mountains and Valleys" );
            for( int n = size; n <= NUM_TERRAIN_TILES; n += 3 )
            {
                int x = ShortRandom(Map::MSize);
                int y = ShortRandom(Map::NSize);
                int height = (ShortRandom(size)-ShortRandom(size))/3;

                make_mountain( map, x, y, height );
            }
        }
    }

    void random_ridges()
    {
        // Make ridges or canyons

        Pause( "Adding Ridges and Canyons" );
        int a = ByteRandom(128)-64;
        int b = ByteRandom(128)-64;
        int c = (ByteRandom(128)-64) * (Map::MSize+Map::NSize);
        int dz = (ByteRandom(NUM_TERRAIN_TILES)
                  -ByteRandom(NUM_TERRAIN_TILES))/2;
        int halfwidth = ByteRandom(16) + 3;
        int halfwidth2 = halfwidth*halfwidth;
        int e = (a*a+b*b);
        // The line is ax + by + c = 0
        // Everything within distance halfwidth gets up to dz added to it
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                // Shake it a bit by adding randomness
                int x = m - Map::MSize/2 - ByteRandom(10) + ByteRandom(10);
                int y = n - Map::NSize/2 - ByteRandom(10) + ByteRandom(10);
                int d1 = a*x + b*y + c;
                int d2 = d1*d1 / e;
                if( d2 < halfwidth2 )
                    map.set_altitude( h, map.altitude(h) +
                                      dz*(halfwidth2-d2)/halfwidth2 );
            }
    }

    void soil_erosion()
    {
        for( int k = 0; k < 20; ++k )
        {
            Pause( "Soil Erosion" );
            map.smooth_terrain( NUM_HEXES/10 );
        }
    }

    void increase_mountains()
    {
        Pause( "Increasing Mountains" );

        int max_alt = 0;
        for( int w = 0; w < NUM_WATER_SOURCES; ++w )
        {
            int h = map.altitude( map.water_sources_[w] );
            if( h > max_alt )
                max_alt = h;
        }

        for( int w = 0; w < NUM_WATER_SOURCES; ++w )
        {
            HexCoord loc = map.water_sources_[w];
            int RW = 3; // number of random walk steps
            int diff = ( max_alt - map.altitude(loc) ) / RW;
            for( int rw = 0; rw < RW; ++rw )
            {
                make_mountain( map, loc.m, loc.n, diff/RW );
                // Move towards the edge of the map, to make the
                // water more likely to flow towards the center of the map
                if( loc.m > Map::MSize/2 )
                    loc.m += ByteRandom(3);
                else
                    loc.m -= ByteRandom(3);
                if( loc.n > Map::MSize/2 )
                    loc.n += ByteRandom(3);
                else
                    loc.n -= ByteRandom(3);
            }
        }
    }

    void water_flow()
    {
        Pause( "Water Flow" );
        for( int k = 0; k < 256; ++k )
        {
            HexCoord h( map.water_sources_[k%NUM_WATER_SOURCES] );
            map.set_water( h, map.water(h)+5 );
            map.water_flow();
            map.water_evaporation();
        }
    }
    
    void rescale_map()
    {
        Pause( "Rescaling" );
        int min_alt =  10000;
        int max_alt = -10000;
        int total_alt = 0;
        int count = 0;
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                int a = map.altitude(h);
                min_alt = min( min_alt, a );
                max_alt = max( max_alt, a );
                total_alt += a;
                ++count;
            }
        int avg = total_alt / (1+count);
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                int a = map.altitude(h);
                if( a < avg )
                    a = (a-min_alt) * NUM_TERRAIN_TILES/2 / (avg-min_alt);
                else
                    a = NUM_TERRAIN_TILES - 
                        (max_alt-a) * NUM_TERRAIN_TILES/2 / (max_alt-avg);
                map.set_altitude( h, a );
            }
    }

    void redistribute_and_smooth()
    {
        Pause( "Soil Erosion" );
        map.recalculate_histogram();
        map.redistribute_terrain( 500 );
        map.super_smooth_terrain();
    }

    void redistribute_only()
    {
        Pause( "Soil Erosion" );
        map.recalculate_histogram();
        map.redistribute_terrain( 500 );
    }

    void volcanic_activity()
    {
        for( int k = NUM_WATER_SOURCES-1; k >= 0; k-- )
        {
            Pause( "Volcanic Activity" );
            HexCoord h( 5+ShortRandom(Map::MSize-10),
                        5+ShortRandom(Map::NSize-10) );
            if( Map::valid(map.water_sources_[k]) )
                h = map.water_sources_[k];
            map.create_volcano( h );
            map.lava_flow();
            map.lava_flow();
        }

        for( int j = 0; j < 125; ++j )
        {
            if( j == 70 )
                Pause( "Volcanic Activity" );
            map.lava_flow();
        }
    
        // Erase the remains of the volcanoes
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                if( map.terrain( h ) == Fire || map.terrain( h ) == Scorched )
                {
                    map.set_terrain( h, Trees );
                    map.extra_[h] = ByteRandom(32);
                }
                else if( map.terrain(h) == Lava )
                    map.set_terrain( h, Clear );
            }
    }

    void great_flood()
    {
        Pause( "The Great Flood" );
        // First put water everywhere on the map
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
                map.set_water( HexCoord(m,n), 5 );
        Pause( "The Great Flood" );
        // Let the water flow around naturally
        for( int k = 60; k > 0; k-- )
        {
            if( k % 10 == 0 )
                Pause( "The Great Flood" );

            if( k % 40 == 0 )
            {
                Pause( "Lake Formation" );
                for( int m = 1; m <= Map::MSize; ++m )
                    for( int n = 1; n <= Map::NSize; ++n )
                        map.set_water( HexCoord(m,n),
                                       map.water(HexCoord(m,n)) + 3 );
            }
            map.water_flow();
            map.water_evaporation();
        }
        Pause( "Lake Removal" );
        // Determine how much water is around
        int k = 0;
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
                k += map.water(HexCoord(m,n));
        // How much is the average increase in water?
        k = (k+Map::MSize*Map::NSize/2) / (Map::MSize*Map::NSize);
        // Now turn water into land, to fill in lakes and flatten basins
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
        {
            HexCoord h(m,n);
            if( map.water(h) > k )
                map.set_altitude( h, map.altitude(h) +
                                  (map.water(h)-k+WATER_MULT/2)/WATER_MULT );
            map.set_water( h, 0 );
        }

        for( k = 0; k < 10; ++k )
            redistribute_and_smooth();
    }

    void uplifting()
    {
        uplift_terrain( map, 4, pause );
        map.water_flow();
        map.water_flow();
        map.water_flow();
    }

    void find_water_sources()
    {
        // Find the water sources and sink
        Pause( "Making Springs" );

        double field[2+Map::MSize][2+Map::NSize];
        for( int x = 0; x < 2+Map::MSize; ++x )
            for( int y = 0; y < 2+Map::NSize; ++y )
            {
                bool loc_valid = map.valid(HexCoord(x,y));
                double f = loc_valid? map.altitude_[HexCoord(x,y)] : 0.0;
                if( loc_valid && f < map.altitude(water_sink) )
                    water_sink = HexCoord(x,y);
                if( x < 6 || x > Map::MSize-6 )
                    f -= 40;
                if( y < 6 || y > Map::NSize-6 )
                    f -= 40;
                if( x < 4 || x > Map::MSize-4 )
                    f -= 40;
                if( y < 4 || y > Map::NSize-4 )
                    f -= 40;
                field[x][y] = f;
            }

        const int barrier = 5;
        for( int k = 0; k < NUM_WATER_SOURCES; ++k )
            field[map.water_sources_[k].m][map.water_sources_[k].n] -= barrier;

        for( int i = 0; i < 60; ++i )
        {
            int dist = i/2;

            // Move each water source upwards, leaving a trail of obstacles
            for( int k = 0; k < NUM_WATER_SOURCES; ++k )
            {
                int wx = map.water_sources_[k].m;
                int wy = map.water_sources_[k].n;
                int nx = wx, ny = wy;
                for( int x = max(wx-dist,1);
                     x <= min(wx+dist,int(Map::MSize)); x+=2 )
                    for( int y = max(wy-dist,1);
                         y <= min(wy+dist,int(Map::NSize)); y+=2 )
                        if( field[x][y] > field[nx][ny] + barrier )
                        {
                            nx = x;
                            ny = y;
                        }

                // Mark the old spot and neighbors as unsuitable
                if( wx != nx || wy != ny )
                {
                    for( int x = max(wx-3,1);
                         x <= min(wx+3,int(Map::MSize)); ++x )
                        for( int y = max(wy-3,1);
                             y <= min(wy+3,int(Map::NSize)); ++y )
                            if( x != nx || y != ny )
                                field[x][y] -= 20;
                    map.water_sources_[k] = HexCoord( nx, ny );
                }
            }
        }
    }

    void add_forests()
    {
        // Add forests depending on altitude
        Pause( "Adding Forests" );
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                int a = map.altitude( h );
                if( a >= NUM_TERRAIN_TILES / 5 && a <= 3 * NUM_TERRAIN_TILES / 5 
                    && ByteRandom(17) == 0 )
                    map.set_terrain( h, Trees );
                if( ByteRandom(7) == 0 && a < NUM_TERRAIN_TILES / 5 )
                    map.set_terrain( h, Trees );
                if( a > 3 * NUM_TERRAIN_TILES / 5 && ByteRandom(5) == 0 )
                    map.set_terrain( h, Trees );
            }
    }

    void water_flow_16()
    {
        for( int i = 0; i < 32; ++i )
            map.water_flow();
    }

    void verify_terrain()
    {
        for( int m = 1; m <= Map::MSize; ++m )
            for( int n = 1; n <= Map::NSize; ++n )
            {
                HexCoord h(m,n);
                int a = map.altitude(h);
                if( a < 0 ) a = 0;
                if( a >= NUM_TERRAIN_TILES ) a = NUM_TERRAIN_TILES-1;
                map.set_altitude( h, a );
            }
    }
    
    void river_channels()
    {
        // cut a channel for the rivers
        for( int k = 0; k < NUM_WATER_SOURCES; ++k )
        {
            Pause( "Making River Basins" );
            HexCoord h = map.water_sources_[k];
            map.set_water( h, 1 );

            for( int height = 4*map.altitude( h ); height > 0; height-- )
            {
                HexCoord bh = h;
                int ba = 10000;
                for( int dir = 5; dir >= 0; dir-- )
                {
                    HexCoord h2 = Neighbor( h, HexDirection(dir) );
                    if( !Map::valid(h2) || map.water(h2) > 0 )
                        continue;
                    int a = map.altitude( h2 );
                    // Let's try to steer towards the water sink:
                    int dx = abs(h2.m-water_sink.m);
                    int dy = abs(h2.n-water_sink.n);
                    a += (dx*dx + dy*dy) / (dx+dy+1);
                    if( a < ba || (a==ba && ByteRandom(2)==0) )
                    {
                        ba = a;
                        bh = h2;
                    }
                }

                if( bh != h )
                {
                    if( map.altitude( bh ) < height/4 )
                        height = 4*map.altitude( bh );
                    map.set_altitude( bh, height/4 );
                    map.set_water( bh, 5 );
                    h = bh;
                }
                else
                    break;
            }

            for( int m = 1; m <= Map::MSize; ++m )
                for( int n = 1; n <= Map::NSize; ++n )
                {
                    HexCoord h3(m,n);
                    if( map.water( h3 ) > 0 )
                        map.set_altitude( h3, map.altitude(h3) - 4 );
                }
            map.super_smooth_terrain();
            water_flow_16();
            for( int m = 1; m <= Map::MSize; ++m )
                for( int n = 1; n <= Map::NSize; ++n )
                {
                    HexCoord h3(m,n);
                    if( map.water( h3 ) > 0 )
                        map.set_altitude( h3, map.altitude(h3) - 3 );
                }
            map.super_smooth_terrain();
            water_flow_16();
            for( int m = 1; m <= Map::MSize; ++m )
                for( int n = 1; n <= Map::NSize; ++n )
                {
                    HexCoord h3(m,n);
                    if( map.water( h3 ) > 0 )
                        map.set_altitude( h3, map.altitude(h3) - 2 );
                }
            map.super_smooth_terrain();
        }
    }
};

void Map::initialize( Closure<bool,const char *> pause )
{
    randomize();

    // Fractal random terrain
    Pause( "Generating Terrain" );
    fractal_terrain( *this, 1, 1, max(int(Map::MSize),int(Map::NSize)), 
                     NUM_TERRAIN_TILES );
    
    MapInitializer init(*this,pause);

    init.rescale_map();
    make_mountain( *this, Map::MSize/2, Map::NSize/2, -NUM_TERRAIN_TILES/2 );

    FILE* f = fopen("InitMap.txt", "rt");
    if( !f ) f = fopen("Data/InitMap.txt", "rt");
    while( f )
    {
        char s[256];
        if( !fgets(s,256,f) )
        {
            fclose(f);
            break;
        }

        // Strip whitespace
        char c;
        while( *s && ((c = s[strlen(s)-1]) == ' ' || c == '\r' || c == '\n' ))
            s[strlen(s)-1] = '\0';

        if( !*s || s[0]=='!' || s[0]=='#' )
            ;
        else if( !stricmp(s,"flat-map") )
            init.flat_map();
        else if( !stricmp(s,"random-mountains") )
            init.random_mountains();
        else if( !stricmp(s,"random-ridges") )
            init.random_ridges();
        else if( !stricmp(s,"increase-mountains") )
            init.increase_mountains();
        else if( !stricmp(s,"rescale-map") )
            init.rescale_map();
        else if( !stricmp(s,"soil-erosion") )
            init.soil_erosion();
        else if( !stricmp(s,"super-soil-erosion") )
            init.redistribute_and_smooth();
        else if( !stricmp(s,"volcanic-activity") )
            init.volcanic_activity();
        else if( !stricmp(s,"great-flood") )
            init.great_flood();
        else if( !stricmp(s,"uplifting") )
            init.uplifting();
        else if( !stricmp(s,"water-flow") )
            init.water_flow();
        else if( !stricmp(s,"find-water-sources") )
            init.find_water_sources();
        else if( !stricmp(s,"dig-rivers") )
            init.river_channels();
        else if( !stricmp(s,"renormalize-terrain") )
            init.redistribute_only();
        else if( !stricmp(s,"add-forests") )
            init.add_forests();
    }

    init.verify_terrain();
}

void Map::super_smooth_terrain()
{
    for( int i = 0; i < NUM_HEXES; ++i )
    {
        HexCoord h; hex_position( i, h );

        if( !( flags_[h] & FLAG_EROSION ) )
            continue;

        int alt0 = altitude(h);
        int alt1 = 0;
        int count = 0;
        for( int d = 0; d < 6; ++d )
        {
            HexCoord h2 = Neighbor( h, HexDirection(d) );
            if( !Map::valid(h2) || !(flags_[h2] & FLAG_EROSION) )
                continue;
            alt1 += altitude( h2 );
            ++count;
        }
        if( count == 0 ) continue;
        alt1 += (count/2);
        alt1 /= count;

        int new_alt = (alt0*3 + alt1 + 2) / 4;
        if( new_alt < 0 ) new_alt = 0;
        set_altitude( h, new_alt );
    }
}

