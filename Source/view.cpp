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
#include "Figment.h"

#include "Sprites.h"

#include "Map.h"
#include "Map_Const.h"
#include "View.h"
#include "Layer.h"
#include "TextGlyph.h"

#define DAMAGE_SELECTED 1
#define TRAIN 0

//////////////////////////////////////////////////////////////////////
// Useful functions:
//      WinQueryVisibleRegion -- determine what areas of the window
//      are actually visible; use to clip the area that's being redrawn?

// View options
extern struct { bool enabled; int id; } view_options[];
extern int NUM_VIEW_OPTIONS;
extern Subject<int> debug_view;

#include "SimBlob.h"
bool view_option( int id )
{
    for( int i = 0; i < NUM_VIEW_OPTIONS; i++ )
    {
        if( view_options[i].id == id )
            return view_options[i].enabled;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////
Rect View::view_area( const Rect& client_area )
{
    Rect v = client_area;

    v.xLeft += xoffset;
    v.xRight += xoffset;
    v.yBottom += yoffset;
    v.yTop += yoffset;

    return v;
}

Rect View::client_area( const Rect& view_area )
{
    Rect v = view_area;

    v.xLeft -= xoffset;
    v.xRight -= xoffset;
    v.yBottom -= yoffset;
    v.yTop -= yoffset;

    return v;
}

void View::mark_damaged()
{
    last_update_ = -1;
}

static int RoadIndex( Map* map, const HexCoord& h )
{
    int index = 0;
    for( unsigned d = 0; d < 6; d++ )
    {
        HexCoord h2 = Neighbor(h,HexDirection(d));
        if( map->valid(h2) && ( map->terrain(h2) == Road || map->terrain(h2) == Bridge ) )
            index |= (1<<d);
    }
    return index;
}

static int WallIndex( Map* map, const HexCoord& h )
{
    int index = 0;
    for( unsigned d = 0; d < 6; d++ )
    {
        HexCoord h2 = Neighbor(h,HexDirection(d));
        Terrain t2 = map->valid(h2)? map->terrain(h2) : Wall;
        if( t2 == Wall || t2 == Gate )
            index |= (1<<d);
    }
    return index;
}

//////////////////////////////////////////////////////////////////////
class MapView: public Layer
{
    Map* map;
    View* view;
    MapArray<value> water_cache;
    
  public:
    MapView( Map* m, View* v );
    ~MapView();

    virtual void mark_damaged( const Rect& view_area, DamageArea& damaged );
    virtual void draw_rect( PixelBuffer& buffer, const Point& origin, 
                            const Rect& damaged );
    virtual HitResult hit( Point p );
};

class CursorView: public Layer
{
    Map* map;
    View* view;
    int old_hex_style;
    HexCoord cursor_loc, old_cursor_loc;

  public:
    CursorView( Map* m, View* v, HexCoord c )
        :map(m), view(v), cursor_loc(c), old_cursor_loc(0,0), old_hex_style(0)
    {}

    void set_cursor( HexCoord c ) { cursor_loc = c; }

    virtual void mark_damaged( const Rect& view_area, DamageArea& damaged );
    virtual void draw_rect( PixelBuffer& buffer, const Point& origin, 
                            const Rect& damaged );
    virtual HitResult hit( Point p );
};

void CursorView::mark_damaged( const Rect&, DamageArea& damaged )
{
    extern int hex_style;
    if( old_cursor_loc != cursor_loc || hex_style != old_hex_style )
    {
        if( map->valid( old_cursor_loc ) )
            damaged += old_cursor_loc.rect();
        if( map->valid( cursor_loc ) )
            damaged += cursor_loc.rect();
        old_cursor_loc = cursor_loc;
        old_hex_style = hex_style;
    }

#if DAMAGE_SELECTED
    // Do we need to damage the selection?  It doesn't seem to harm us
    // if we don't..
    if( map->selected.size() != 0 )
    {
        Mutex::Lock lock( map->selection_mutex, 500 );
        if( lock.locked() )
        {
            for( vector<HexCoord>::iterator k = map->selected.begin();
                 k != map->selected.end(); ++k )
            {
                HexCoord h = *k;
                damaged += h.rect();
            }
        }
    }
#endif
}

void CursorView::draw_rect( PixelBuffer& buffer, const Point& origin, 
                            const Rect& damage_rect )
{
    int x = cursor_loc.x()-origin.x;
    int y = cursor_loc.y()-origin.y;
    buffer.DrawSprite( cursor_rle[old_hex_style%NUM_CURSOR_FRAMES], x, y );

    int left, bottom, right, top;
    View::rect_to_hexarea( damage_rect, left, bottom, right, top );

    // We need access to the selection and the  military units
    // They can't change or move while we're drawing them
    if( map->selected.size() != 0 )
    {
        Mutex::Lock lock( map->selection_mutex, 500 );
        if( lock.locked() )
        {
            extern Terrain current_tool;
            Sprite* overlay = NULL;
            switch( current_tool )
            {
              case Farm: overlay = &(farm_rle[0]); break;
              case Market: overlay = &market_rle; break;
              case Houses: overlay = &(house_rle[0]); break;
              case Fire: overlay = &fire1_rle; break;
              case Trees: overlay = &trees_rle[NUM_TREES/2]; break;
              case Gate: overlay = &tower_rle; break;
              case Canal: overlay = &canal_rle; break;
              case WatchFire: overlay = &fire2_rle; break;
              default: break;
            }

            for( vector<HexCoord>::iterator k = map->selected.begin(); k != map->selected.end(); ++k )
            {
                HexCoord h = *k;
                if( (left <= h.m && h.m <= right) && 
                    (bottom <= h.n && h.n <= top) )
                {
                    int x = h.x()-origin.x, y = h.y()-origin.y;
                    buffer.DrawSprite( marktile_rle, x, y );
                    if( !map->valid( cursor_loc ) )
                    {
                        buffer.DrawColoredSprite( sprinkles[5], x, y, 0xF9 );
                        buffer.DrawColoredSprite( sprinkles[2], x, y, 0xFF );
                    }
                    else if( map->terrain(h) != current_tool )
                    {
                        Sprite* S = NULL;
                        if( overlay )
                            S = overlay;
                        else
                        {
                            int index = 0;
                            switch( current_tool )
                            {
                              case Road:
                              case Bridge:
                                  index = RoadIndex( map, h ); break;
                              case Wall:
                                  index = WallIndex( map, h ); break;
                            }

                            for( unsigned d = 0; d < 6; d++ )
                            {
                                HexCoord u = Neighbor(h,HexDirection(d));
                                vector<HexCoord>::iterator back,next;
                                back = k-1;
                                next = k+1;
                                if( k != map->selected.begin() )
                                    if( u == *back )
                                        index |= (1<<d);
                                if( next != map->selected.end() )
                                    if( u == *next )
                                        index |= (1<<d);
                            }

                            switch( current_tool )
                            {
                              case Road:
                              case Bridge:
                                  S = &(road_rle[index]); break;
                              case Wall:
                                  S = &(wall_rle[index]); break;
                            }
                        }
                        if( S )
                            buffer.DrawTransparent2Sprite( *S, x, y );
                    }
                }
            }
        }
    }
}

HitResult CursorView::hit( Point )
{
    return HitResult();         // The cursor can never be 'hit'
}

//////////////////////////////////////////////////////////////////////
class UnitView: public Layer
{
    inline Rect unit_area( const Point& p )
    {
        return Rect( p.x-Unit_size_x/2-1, p.y-Unit_size_y/2-1,
                     p.x+Unit_size_x/2+1, p.y+Unit_size_y/2+1 );
    }
    
    struct UnitCache
    {
        Unit* unit;
        int id;
        Point loc;
    };
    
    int CACHE_SIZE;
    UnitCache *cache;
    int cache_length;
    void build_cache( int size );
    void destroy_cache();
    
    Map* map;
    View* view;

  public:
    UnitView( Map* m, View* v );
    ~UnitView();

    virtual void mark_damaged( const Rect& view_area, DamageArea& damaged );
    virtual void draw_rect( PixelBuffer& buffer, const Point& origin, 
                            const Rect& damaged );
    virtual void draw( PixelBuffer& buffer, const Point& origin, 
                       DamageArea& damaged );
    virtual HitResult hit( Point p );
};

UnitView::UnitView( Map* m, View* v )
    :map(m), view(v)
{
    build_cache( 128 );
}

UnitView::~UnitView()
{
    destroy_cache();
}

void UnitView::build_cache( int size )
{
    CACHE_SIZE = size;
    cache = new UnitCache[CACHE_SIZE];
    cache_length = 0;
}

void UnitView::destroy_cache()
{
    delete[] cache;
    cache = NULL;
    cache_length = 0;
}

void UnitView::mark_damaged( const Rect& view_area, DamageArea& damaged )
{
    static Rect prev_rect;
    damaged += prev_rect;
    prev_rect = Rect();
#if TRAIN
    // The train is an experiment... it doesn't do anything useful
    for( int i = 0; i < train.num_cars(); i++ )
    {
        Rect r = train.car_loc(i).rect();
        damaged += r;
        prev_rect = merge(prev_rect, r);
    }
#endif
    
    if( map->units.size() != 0 || cache_length != 0 )
    {
        Mutex::Lock lock( map->unit_mutex, 500 );
        if( lock.locked() )
        {
            int cache_index = 0;
            for( vector<Unit*>::iterator u = map->units.begin(); 
                 (cache_index!=cache_length) || (u!=map->units.end()) ; )
            {
                Unit* unit = (u == map->units.end())? NULL : (*u);
                int unit_id = (unit==NULL)? 0x7ffffff : unit->id;
                int cache_id = (cache_index>=cache_length)? 
                    0x7ffffff : (cache[cache_index].id);

                if( cache_id < unit_id )
                {
                    // Case 1:  Unit left the cache
                    // This should always be the case when u is at end
                    damaged += unit_area( cache[cache_index].loc );
                    ++cache_index;
                }
                else if( cache_id == unit_id )
                {
                    // Case 2:  Unit stays around
                    Point loc = unit->gridloc();
                    if( loc != cache[cache_index].loc )
                    {
                        // The unit has moved
                        damaged += unit_area( cache[cache_index].loc );
                    }
                    ++cache_index;
                    ++u;
                }
                else
                {
                    // Case 3:  Unit moved out of cache
                    // This should always be the case when cache iter at end
                    ++u;
                }
            }
            
            // Build the cache
            cache_length = 0;
            for( vector<Unit*>::iterator u = map->units.begin();
                 u != map->units.end(); ++u )
            {
                Unit& unit = *(*u);
                if( unit.dead() ) continue;
                
                Point p = unit.gridloc();
                if( p.x >= view_area.xLeft-Unit_size_x &&
                    p.x <= view_area.xRight+Unit_size_x &&
                    p.y >= view_area.yBottom-Unit_size_y &&
                    p.y <= view_area.yTop+Unit_size_y )
                {
                    damaged += unit_area( p );
                    if( cache_length < CACHE_SIZE )
                    {
                        UnitCache& C = cache[cache_length++];
                        C.unit = &unit;
                        C.id = unit.id;
                        C.loc = p;
                        if( unit.moving() )
                            damaged += unit_area( p );
                    }
                }
            }
        }
    }
}

#if TRAIN
// This function draws a train, but the train is not part of the simulation
// at this point.
static void draw_train( Map&, PixelBuffer& buffer, const Point& origin )
{
    int pos = int(train.location);
    int N = train.path.size();
    for( int car = 0; car < train.num_cars(); car++, pos+=CARDIST )
    {
        int L = (pos+5)/10, LL = (L==0)? L:L-1, LR = (L==N-1)? L:L+1;
        double x0 = train.path[ L].x(), y0 = train.path[ L].y();
        double x1 = train.path[LL].x(), y1 = train.path[LL].y();
        double x2 = train.path[LR].x(), y2 = train.path[LR].y();
        
        // Calculate a `fillet' point
        double cx = 0.5*(x1+x0), cy = 0.5*(y1+y0);
        double bx = x0-x1, by = y0-y1;
        double ax = 0.5*(x2+x1)-x0, ay = 0.5*(y2+y1)-y0;
        
        double t = 0.1*((pos+5)%10);
        double rx = ax*t*t + bx*t + cx, ry = ay*t*t + by*t + cy;
        double ru = 2*ax*t + bx, rv = 2*ay*t + by;
        
        // Find the angle
        double a = atan2(rv,ru);    // (y, x)
        int angle = (100*NUM_ANGLES + int(a/pi*NUM_ANGLES)) % NUM_ANGLES;
        buffer.DrawSprite( trains[angle], int(rx)-origin.x, int(ry)-origin.y );
    }
}
#endif

void UnitView::draw_rect( PixelBuffer& buffer, const Point& origin, 
                          const Rect& )
{
#if TRAIN
    draw_train( *map, buffer, origin );
#endif
    
    for( int cache_index = 0; cache_index < cache_length; cache_index++ )
    {
        Point p = cache[cache_index].loc;
        int x = p.x-origin.x;
        int y = p.y-origin.y;

        Sprite* blob = &soldier_rle;
        if( cache[cache_index].unit->type == Unit::Firefighter )
            blob = &firefighter_rle;
        if( cache[cache_index].unit->type == Unit::Builder )
            blob = &worker_rle;
        buffer.DrawSprite( *blob, x, y );
    }
}

void UnitView::draw( PixelBuffer& buffer, const Point& origin, 
                     DamageArea& damaged )
{
    Layer::draw( buffer, origin, damaged );

    // Resize the cache if necessary, for the next time through
    if( cache_length == CACHE_SIZE )
    {
        destroy_cache();
        build_cache( CACHE_SIZE * 2 );
    }
}

HitResult UnitView::hit( Point p )
{
    for( int cache_index = 0; cache_index < cache_length; cache_index++ )
    {
        Point up = cache[cache_index].loc;
        Sprite* blob = &soldier_rle;
        if( cache[cache_index].unit->type == Unit::Firefighter )
            blob = &firefighter_rle;
        if( cache[cache_index].unit->type == Unit::Builder )
            blob = &worker_rle;
        if( blob->pick( up, p ) )
            return HitResult( cache[cache_index].unit );
    }
    return HitResult();
}

//////////////////////////////////////////////////////////////////////
MapView::MapView( Map* m, View* v )
    :map(m), view(v), water_cache(0)
{
}

MapView::~MapView()
{
}

void MapView::mark_damaged( const Rect& view_area, DamageArea& damaged )
{
    int left, bottom, right, top;
    View::rect_to_hexarea( view_area, left, bottom, right, top );

    // It would be nice to return immediately if we have to draw
    // everything, but the water cache is updated in this function,
    // so we can't return immediately.
    if( debug_view != ID_DEBUG_NONE )
    {
        damaged += view_area;
    }

    bool view_water = view_option( ID_VIEW_WATER );

    // For each column, look for the damaged area
    for( int m = left; m <= right; m++ )
    {       
        // The column is the current area of damage
        Rect column( HexCoord(m,bottom).rect() );
        // The right and top should be one past the end of the rectangle
        column.xRight++;
        // Filling tells us whether we are in or out of the damaged area
        bool filling = false;
        // If it's a big column, there's water somewhere in there,
        // so it needs to be wider and taller than normal
        bool big_column = false;

        for( int n = bottom; n <= top; n++ )
        {
            HexCoord h(m,n);

            // Only valid areas might be damaged, unless the last
            // update is negative.  Let update store either the
            // last update, or 0 for invalid areas.
            int update = 0;
            if( map->valid(h) )
            {
                // First check the main map (no water)
                update = map->damage_[h];
                // Fire is animated, so it's always drawn
                if( map->terrain(h) == Fire || map->terrain(h) == Scorched )
                    update = 0x7fffffff;

                // Now check for water
                if( view_water )
                {
                    int w = (map->water(h)+5)/6;
                    if( w < 0 ) w = 0;
                    if( w != water_cache[h] )
                    {
                        water_cache[h] = w;
                        update = 0x7fffffff;
                        big_column = true;
                    }
                }
            }
            if( update > view->last_update_ )
            {
                if( !filling )
                {
                    column.yBottom = h.bottom();
                    filling = true;
                }
            }
            else
            {
                if( filling )
                {
                    column.yTop = h.bottom();
                    filling = false;
                    if( big_column )
                    {
                        // If there was water in the column, it should
                        // be made larger
                        column.xLeft -= HexWidth/2;
                        column.xRight += HexWidth/2;
                        column.yBottom -= HexHeight/2;
                        column.yTop += HexHeight/2;
                        big_column = false;
                    }
                    damaged += column;
                }
            }
        }
        
        // Now we're at the end of the column
        if( filling )
        {
            column.yTop = HexCoord(m,top).top() + 1;
            if( big_column )
            {
                column.xLeft -= HexWidth/2;
                column.xRight += HexWidth/2;
                column.yBottom -= HexHeight/2;
                column.yTop += HexHeight/2;
            }
            damaged += column;
        }
    }
}

void MapView::draw_rect( PixelBuffer& buffer, const Point& origin, 
                         const Rect& damage_rect )
{
    // Phase is for animation
    static int phase = 0;
    phase++;

    int left, bottom, right, top;
    View::rect_to_hexarea( damage_rect, left, bottom, right, top );

    bool draw_roads = view_option( ID_VIEW_ROADS );
    bool draw_fire = view_option( ID_VIEW_FIRE );
    bool draw_buildings = view_option( ID_VIEW_BUILDINGS );
    bool draw_structures = view_option( ID_VIEW_STRUCTURES );
    bool draw_water = view_option( ID_VIEW_WATER );

    // First stage: draw ground and most terrain types
    for( int m = left; m <= right; m++ )
    {
        for( int n = bottom; n <= top; n++ )
        {
            HexCoord h(m,n);
            if( map->valid(h) )
            {
                int x = h.x() - origin.x;
                int y = h.y() - origin.y;
                int a = map->altitude(h);
                if( a < 0 ) a = 0;
                if( a >= NUM_TERRAIN_TILES ) a = NUM_TERRAIN_TILES-1;

                // buffer.DrawSprite( terrain_rle[a], x, y );
                
                {
                    for( int d = 0; d < 6; d++ )
                    {
                        HexCoord h2 = Neighbor(h,HexDirection(d));
                        int a2 = ( map->valid(h2) )? map->altitude(h2):a;
                        int aa = ( 2*a + a2 ) / 3;
                        if( aa < 0 ) aa = 0;
                        if( aa >= NUM_TERRAIN_TILES ) aa = NUM_TERRAIN_TILES-1;
                        buffer.DrawSprite( terrain_rle[aa+d*NUM_TERRAIN_TILES], x, y );
                    }
                    buffer.DrawSprite( terrain_rle[a+6*NUM_TERRAIN_TILES], x, y );
                }
                
                if( debug_view != ID_DEBUG_NONE )
                {
                    int v = 0;
                    switch( debug_view )
                    {
                      case ID_DEBUG_PREFS: v = map->prefs_[h]; break;
                      case ID_DEBUG_HEAT: v = map->heat_[h]/15; break;
                      case ID_DEBUG_LABOR: v = map->labor_[h]/15; break;
                      case ID_DEBUG_FOOD: v = map->food_[h]/15; break;
                      case ID_DEBUG_MOISTURE: v = 2*map->moisture_[h]; break;
                    }
                    int i = 16 + v;
                    if( i < 0 ) i = 0;
                    if( i > 31 ) i = 31;
                    int j = abs(v);
                    if( j > 16 ) j = 16;
                    if( j != 0 ) 
                        buffer.DrawColoredTransparent2Sprite( sprinkles[j-1], x, y, GrayColors[i] );
                }
                    
                Terrain t = map->terrain(h);
                Sprite* spr = NULL;
                int index = 0;
                    
                if( draw_roads && t == Road )
                {
                    index = RoadIndex( map, h );
                    spr = &(road_rle[index]);
                }
                else if( draw_structures && t == Wall )
                {
                    index = WallIndex( map, h );
                    spr = &(wall_rle[index]);
                }
                else if( draw_buildings && t == Houses )
                {
                    int buildings = map->extra_[h];
                    if( buildings > 7 ) Log("View","Buildings>7");
                    spr = &(house_rle[buildings]);
                }
                else if( draw_buildings && t == Farm )
                {
                    int which_farm =
                        map->day_of_year() + (map->extra_[h] % 40) + h.m%4;
                    which_farm = which_farm * NUM_FARMS / map->days_in_year();
                    which_farm += NUM_FARMS/2;
                    spr = &(farm_rle[which_farm % NUM_FARMS]);
                }
                else if( draw_buildings && t == Market )
                    spr = &market_rle;
                else if( draw_water && t == Trees )
                {
                    int age = (map->extra_[h])/2;
                    if( age < 0 ) age = 0;
                    if( age >= NUM_TREES ) age = NUM_TREES-1;

                    int season = (map->day_of_year() + (map->extra_[h]+h.n)%20)
                        * NUM_TREE_SEASONS / map->days_in_year();
                    
                    season %= NUM_TREE_SEASONS;
                    
                    spr = &trees_rle[age+season*NUM_TREES];
                }
                else if( draw_structures && t == Gate )
                    spr = &tower_rle;
                else if( draw_structures && t == WatchFire )
                    spr = &tower_rle;
                else if( draw_fire && t == Lava )
                    spr = &lava_rle;
                else if( draw_fire && t == Fire )
                {
                    spr = ((h.m+phase+map->extra_[h])&1)?(&fire1_rle):(&fire2_rle);
                }
                                        
                // Draw something underneath canals
                if( !map->erosion(h) )
                    buffer.DrawTransparent2Sprite( canal_rle, x, y );
                
                if( spr != NULL )
                {
                    buffer.DrawSprite( *spr, x, y );
                }
                else if( draw_fire && t == Scorched )
                {
                    int i = 10-(map->extra_[h]/3);
                    if( i < 0 ) i = 0;
                    buffer.DrawColoredTransparent2Sprite( sprinkles[i], 
                                                          x, y, 0x00 );
                }

                // Draw something on top of canals
                if( !map->erosion(h) )
                    buffer.DrawColoredSprite( sprinkles[2], x, y, 0x04 );
                
                // Draw the rest of watchtowers
                if( draw_structures && t == WatchFire )
                    buffer.DrawTransparent2Sprite( fire1_rle, x, y );
            }
            else
                buffer.DrawSprite( tower_rle, h.x()-origin.x, h.y()-origin.y );
        }
    }

    // Second stage: draw water
    if( draw_water )
        for( int m = left; m <= right; m++ )
        {
            for( int n = bottom; n <= top; n++ )
            {
                HexCoord h(m,n);
                if( map->valid(h) )
                {
                    int x = h.x() - origin.x;
                    int y = h.y() - origin.y;
                    
                    int w0 = water_cache[h];
                    if( w0 < 0 )
                    {
                        DosBeep( 1000, 10 );
                        buffer.DrawSprite( lava_rle, x, y );
                    }

                    if( w0 > 0 )
                    {
                        buffer.DrawSprite( water_rle[w0>250?63:w0/4], x, y );
                        for( int d = 0; d < 3; d++ )
                        {
                            HexCoord h2 = Neighbor(h,HexDirection(d));
                            if( !map->valid(h2) )
                                continue;
                            int w1 = water_cache[h2];
                            if( w1 <= 0 )
                                continue;
                            
                            unsigned w = (w0 + w1 + 6) / 8;
                            if( w > 63 ) w = 63;
                            
                            int hx = (h.x()+h2.x())/2-origin.x;
                            int hy = (h.y()+h2.y())/2-origin.y;
                            buffer.DrawSprite( water_rle[w], hx, hy );
                        }
                    }
                }
            }
        }

    // Third stage:  draw bridges
    if( draw_roads )
        for( int m = left; m <= right; m++ )
        {
            for( int n = bottom; n <= top; n++ )
            {
                HexCoord h(m,n);
                if( map->valid(h) )
                {
                    int x = h.x() - origin.x;
                    int y = h.y() - origin.y;
                    
                    Terrain t = map->terrain(h);
                    if( t == Bridge )
                    {
                        int index = RoadIndex( map, h );
                        buffer.DrawSprite( road_rle[index], x, y );
                        buffer.DrawColoredSprite( overlay_rle[index], x, y, 0x00 );
                    }
                }
            }
        }

    // Draw an S on each water source
    for( int i = 0; i < NUM_WATER_SOURCES; i++ )
    {
        HexCoord h = map->water_sources_[i];
        if( (left <= h.m && h.m <= right)&&(bottom <= h.n && h.n <= top) )
        {
            int x = h.x()-5-origin.x;
            int y = h.y()-6-origin.y;
            buffer.DrawColoredSprite( text12.data['S'], x+1, y-1, 0x00 );
            buffer.DrawColoredSprite( text12.data['S'], x, y, 0xff );
        }
    }

    // Draw a B at the solder start position
    if( (left <= map->city_center_.m && map->city_center_.m <= right)&&
        (bottom <= map->city_center_.n && map->city_center_.n <= top) )
    {
        int x = map->city_center_.x()-5-origin.x;
        int y = map->city_center_.y()-6-origin.y;
        buffer.DrawColoredSprite( text12.data['*'], x+1, y-1, 0x00 );
        buffer.DrawColoredSprite( text12.data['*'], x, y, 0xff );
    }

    // Draw a gray hex around any places that are going to be built
    for( int i = 0; i < jobs.size(); ++i )
    {
        Job& jb = jobs[i];
        if( jb.build != -1 )
        {
            int col = 0x77;
            if( jb.blob != -1 ) col = 0x0D;
            int x = jb.location.x() - origin.x;
            int y = jb.location.y() - origin.y;
            buffer.DrawColoredSprite( cursor_rle[0], x, y, col );
        }
    }

    // Draw a blue hex around the drop point
    HexCoord highlight_hex = PointToHex( view->highlight.x, view->highlight.y );
    if( map->valid(highlight_hex) )
    {
        int x = highlight_hex.x() - origin.x;
        int y = highlight_hex.y() - origin.y;
        buffer.DrawColoredSprite( cursor_rle[0], x, y, 0x04 );
    }

}

HitResult MapView::hit( Point p )
{
    HexCoord h = PointToHex( p.x, p.y );
    if( map->valid(h) )
        return HitResult(h);
    return HitResult();
}

//////////////////////////////////////////////////////////////////////

static struct MapViewRecord
{
    // Layers
    MapView mapview;
    UnitView unitview;
    CursorView cursorview;
    TextLayer textview;

    // Individual glyphs
    TextGlyph* volcano_glyph;
    Subject<string> volcano_text;
    
    MapViewRecord( Map* map, View* view );
    ~MapViewRecord();
} * map_info = NULL;

MapViewRecord::MapViewRecord( Map* map, View* view )
    :mapview(map,view),
     unitview(map,view),
     cursorview(map,view,HexCoord()),
     volcano_text(string(""))
{
    TextStyle volcano_style( Font::MediumBG );
    volcano_style.text_color = 0xff;
    volcano_style.shadow_color = 0x01;
    volcano_glyph = new TextGlyph( volcano_text, volcano_style );

    textview.add( volcano_glyph );
}

void View::update( const Rect& view_area, DamageArea& damaged, 
                   PixelBuffer* pb, HexCoord cursor_loc )
{
    long new_time = map->time_tick_;

    if( pb == NULL )
        return;

    if( !map_info ) map_info = new MapViewRecord( map, this );
    
    static HexCoord volcano_loc;
    if( volcano_loc != map->volcano_ )
    {
        if( Map::valid(map->volcano_) )
        {
            set(map_info->volcano_text,"Volcano!");
            Size s, c;
            map_info->volcano_glyph->request( s, c );
            Point p( map->volcano_.x()-28, map->volcano_.y()-4 );
            map_info->volcano_glyph->allocate( Rect(p,s) );
        }
        else
            set(map_info->volcano_text,"");
        volcano_loc = map->volcano_;
    }
            
    PixelBuffer& B = (*pb);
    Point origin(xoffset,yoffset);
    map_info->cursorview.set_cursor( cursor_loc );

    // These views are in relative coordinates
    map_info->mapview.mark_damaged( view_area, damaged );
    map_info->unitview.mark_damaged( view_area, damaged );
    map_info->textview.mark_damaged( view_area, damaged );
    map_info->cursorview.mark_damaged( view_area, damaged );
    
    // At this point, damaged contains the damaged region

    //////////////////////////////////////////////////
    // Now draw the damaged region

    map_info->mapview.draw( B, origin, damaged );
    map_info->unitview.draw( B, origin, damaged );
    map_info->textview.draw( B, origin, damaged );
    map_info->cursorview.draw( B, origin, damaged );

    last_update_ = new_time;
}

HitResult View::hit( Point p )
{
    HitResult h;
    if( !map_info ) return HitResult();
    h = map_info->cursorview.hit(p);
    if( h.type != HitResult::tNone ) return h;
    h = map_info->unitview.hit(p);
    if( h.type != HitResult::tNone ) return h;
    h = map_info->mapview.hit(p);
    if( h.type != HitResult::tNone ) return h;
    return HitResult();
}
