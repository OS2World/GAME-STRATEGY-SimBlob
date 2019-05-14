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

#include "Figment.h"                    // Figment class library
#include "SimBlob.h"                    // Resource file symbolics

#include "Bitmaps.h"
#include "Sprites.h"
#include "Map.h"
#include "View.h"

#include "ViewWin.h"
#include "GameWin.h"

//////////////////////////////////////////////////////////////////////

WorldMapWindow::WorldMapWindow( Map* m, ViewWindow* v )
    :map(m), view(v), mousedown(false), pb_terrain(NULL),
     view_type(VIEW_NORMAL), fire_on_map(false)
{
}

WorldMapWindow::~WorldMapWindow()
{
    delete pb_terrain;
}

bool WorldMapWindow::evCommand( USHORT command, HWND hWnd )
{
    switch( command )
    {
      case ID_WORLD_VIEW_NORMAL:
          view_type = VIEW_NORMAL;
          update_now();
          return true;
        
      case ID_WORLD_VIEW_TERRAIN:
          view_type = VIEW_TERRAIN;
          update_now();
          return true;

      case ID_WORLD_VIEW_CIV:
          view_type = VIEW_CIV;
          update_now();
          return true;

      default:
          return false;
    }
}

MRESULT WorldMapWindow::WndProc( HWND hWnd, ULONG iMessage,
                                 MPARAM mParam1, MPARAM mParam2 )
{
    if( iMessage == WM_SINGLESELECT )
        mousedown = false;
    if( iMessage == WM_MOUSEMOVE && mousedown )
        iMessage = WM_SINGLESELECT;

    switch( iMessage )
    {
      case WM_ENDSELECT:
      {
          mousedown = false;
          return 0;
      }

      case WM_BEGINSELECT:
      {
          mousedown = true;
          // Drop on through to SINGLESELECT
      }

      case WM_SINGLESELECT:
      {
          int mousex = SHORT1FROMMP(mParam1);
          int mousey = SHORT2FROMMP(mParam1);
          Rect area = view->area();
          int x = mousex*HexXSpacing - area.width()/2;
          int y = mousey*HexYSpacing - area.height()/2;

          // Check to see if it's in range
          if( x < 0 ) x = 0;
          if( y < 0 ) y = 0;
          if( x + area.width() > HexCoord(Map::MSize,1).right() )
              x = HexCoord(Map::MSize,1).right() - area.width();
          if( y + area.height() > HexCoord(1,Map::NSize).top() )
              y = HexCoord(1,Map::NSize).top() - area.height();

          view->request_scroll( x, y );
          view->set_scrollbars();
          update();
          return 0;
      }

      case WM_CONTEXTMENU:
      {
          Point p( SHORT1FROMMP(mParam1), SHORT2FROMMP(mParam1) );
          static HWND menu = WinLoadMenu( handle(), NULL, ID_CONTEXT_WORLD );
          WinPopupMenu( handle(), handle(), menu, p.x, p.y,
                        ID_CONTEXT_WORLD,
                        PU_HCONSTRAIN | PU_VCONSTRAIN |
                        PU_MOUSEBUTTON2 | PU_MOUSEBUTTON1 );
          return 0;
      }

    }
    return BufferWindow::WndProc( hWnd, iMessage, mParam1, mParam2 );
}

int text_width( Font& font, const char* text )
{
    int len = 0;
    extern int EXTRA;
    for( const char* c = text; *c; c++ )
        len += font.data[*c].image->width - EXTRA;
    return len;
}

void draw_text( PixelBuffer& buffer, int x0, int y0, const char* text,
                Font& font, int col1, int col2,
                int dx = +1, int dy = -1 )
{
    int descender = font.data['g'].hotspot.y;
    int leader = font.data['g'].hotspot.x;
    int height = font.data['M'].image->height;
    buffer.DrawTransparent2Rect( Rect( x0-leader, y0-descender,
                                       x0+leader+text_width(font,text),
                                       y0-descender+height ),
                                 col2 );
    
    for( int phase = 0; phase < 2; phase++ )
    {
        int x = x0;
        int y = y0;
        for( const char* c = text; *c; c++ )
        {
            if( phase == 0 )
                buffer.DrawColoredTransparentSprite( font.data[128+*c], x+dx, y+dy, col2 );
            else
                buffer.DrawColoredSprite( font.data[*c], x, y, col1 );
            extern int EXTRA;
            x += font.data[*c].image->width - EXTRA;
        }
    }
}

static inline byte water_color( int x, int y, int w )
{
    byte b = 0x00;
    if( (x&1) == 1 && (((x>>1) + y)&1) == 1 )
        w += 6;
    if( w < 32 )
    {
        if( w < 8 )                     b = 0x37;
        else if( w < 12 )               b = 0x17;
        else if( w < 20 )               b = 0x33;
        else                            b = 0x13;
    }
    else 
    {
        if( w > 54 )                    b = 0x2e;
        else if( w > 48 )               b = 0x32;
        else if( w > 40 )               b = 0x0f;
        else                            b = 0x2f;
    }
    return b;
}

bool WorldMapWindow::create_terrain_map()
{
    if( !pb ) Throw("NULL BUFFER");

    if( !pb_terrain ||
        pb->width != pb_terrain->width ||
        pb->height != pb_terrain->height )
    {
        // We need to recreate the buffer
        delete pb_terrain;
        pb_terrain = new PixelBuffer( pb->width, pb->height );
        for( int i = 0; i < pb->linewidth*pb->height; i++ )
            pb->data[i] = 0;
        return true;
    }

    // The buffer is just fine
    return false;
}

static int last_tick = 0;

void WorldMapWindow::update_now()
{
    // Forget when we last redrew
    last_tick = -1000;
    update();
}

extern bool view_option( int );
    
void WorldMapWindow::draw_normal( int left, int bottom, int right, int top )
{
    bool draw_roads = view_option( ID_VIEW_ROADS );
    bool draw_fire = view_option( ID_VIEW_FIRE );
    bool draw_buildings = view_option( ID_VIEW_BUILDINGS );
    bool draw_structures = view_option( ID_VIEW_STRUCTURES );
    bool draw_water = view_option( ID_VIEW_WATER );

    for( int y = bottom; y < top; y++ )
    {
        byte* row = pb_terrain->row(y);
        for( int x = left; x < right; x++ )
        {
            HexCoord h(x+1,y+1);
            byte b = 0x00;

            int w = (draw_water)? (map->water(h)) : 0;
            if( w > 0 )
            {
                if( map->terrain(h) == Bridge && draw_roads )
                    b = 0x00;
                else
                    b = water_color(x,y,w);
            }
            else
            {
                Terrain t = map->terrain(h);

                if( !draw_buildings && ( t==Farm || t==Houses || t==Market ) )
                    t = Clear;
                if( !draw_water && t == Trees )
                    t = Clear;
                if( !draw_fire && ( t==Scorched || t==Fire || t==Lava ) )
                    t = Clear;
                if( !draw_structures &&
                    ( t==Wall || t==Canal || t==Gate || t == WatchFire ) )
                    t = Clear;
                if( !draw_roads && ( t==Road || t==Bridge ) )
                    t = Clear;

                if( t != Clear && t != Trees && t != Scorched && 
                    t != Houses && t != Farm && t != Market )
                {
                    if( t == Road || t == Bridge )              b = 0x00;
                    else if( t == Wall || t == Gate )           b = 0xFF;
                    else if( t == Canal )                       b = 0x62;
                    else if( t == Fire )
                    { fire_on_map = true; b = 0xE8; }
                    else if( t == Lava )                        b = 0xA0;
                }
                else
                {
                    int a = map->altitude(h);
                    if( (x&1) == 1 && (((x>>1) + y)&1) == 1 )
                    {
                        if( ((y>>1) & 1) == 1 )
                            a -= 10;
                        else 
                            a += 10;
                    }

#define SCALE(x) ((x)*NUM_TERRAIN_TILES/100)

                    if( a < SCALE(65) )
                    {
                        if( a < SCALE(-5) )                     b = 0x02;
                        else if( a < SCALE(2) )                 b = 0x0C;
                        else if( a < SCALE(6) )                 b = 0x0D;
                        else if( a < SCALE(10) )                b = 0x4D;
                        else if( a < SCALE(20) )                b = 0x30;
                        else if( a < SCALE(33) )                b = 0x51;
                        else if( a < SCALE(50) )                b = 0x91;
                        else                                    b = 0x03;
                    }
                    else if( a < SCALE(96) )
                    {
                        if( a < SCALE(87) )                     b = 0xB1;
                        else if( a < SCALE(93) )                b = 0xB5;
                        else                                    b = 0xD5;
                    }
                    else if( a < SCALE(98) )                    b = 0xDA;
                    else if( a < SCALE(101) )                   b = 0xDE;
                    else if( a < SCALE(103) )                   b = 0xDE;
                    else                                        b = 0xFF;
#undef SCALE

                    // Blend colors together
                    if( t != Clear && (x&1) == 0 && (((x>>1) + y)&1) == 1 )
                    {
                        if( t == Scorched )
                            b = TRANSPARENCY1(b,0x78);
                        else if( t == Trees )
                            b = TRANSPARENCY1(b,0x5D);
                        else if( t == Farm || t == Houses || t == Market )
                            b = TRANSPARENCY1(b,0x85);
                    }
                }
            }

            row[x] = b;
        }
    }
}

void WorldMapWindow::draw_shaded( int left, int bottom, int right, int top )
{
    bool draw_water = view_option( ID_VIEW_WATER );
    bool draw_fire = view_option( ID_VIEW_FIRE );
    
    for( int y = bottom; y < top; y++ )
    {
        byte* row = pb_terrain->row(y);
        for( int x = left; x < right; x++ )
        {
            HexCoord h(x+1,y+1);
            byte b = (x%2==y%2)? 0x1F : 0x00;
            Terrain t = map->terrain(h);

            int w = (draw_water)? (map->water(h)) : 0;
            if( w > 0 )
                b = water_color(x,y,w);
            else if( draw_fire && t == Fire )
                b = 0xE8;
            else if( draw_fire && t == Lava )
                b = 0xA0;
            else if( b == 0x00 )
            {
                // The shading will be determined by the east/west slope
                HexCoord hL1(h.m-2,h.n+1);
                HexCoord hR1(h.m+1,h.n-2);
                HexCoord hL2(h.m-1,h.n+2);
                HexCoord hR2(h.m+2,h.n-1);

                int aC = map->altitude(h);
                int aL1 = map->valid(hL1)? map->altitude(hL1) : aC/2;
                int aL2 = map->valid(hL2)? map->altitude(hL2) : aC/2;
                int aR1 = map->valid(hR1)? map->altitude(hR1) : aC/2;
                int aR2 = map->valid(hR2)? map->altitude(hR2) : aC/2;
                int d = 16 + (
                              (aR1 - aC) +
                              (aC - aL1) +
                              (aR2 - aC) +
                              (aC - aL2)
                              ) / 5;
                // In addition, make it a little darker in the valleys
                d += ((aC-NUM_TERRAIN_TILES/2) * 6 / NUM_TERRAIN_TILES);
                if( d >= 32 ) d = 31;
                if( d < 0 ) d = 0;
                b = GrayColors[d];
            }
            else if( draw_fire && t == Scorched )
            {
                int age = map->extra_[h] / 2;
                if( age < 0 ) age = 0;
                if( age > 20 ) age = 20;
                b = GrayColors[age];
            }
            else if( draw_water && t == Trees )
                b = 0x5D;
            
            row[x] = b;
        }
    }
}

void WorldMapWindow::draw_civilization( int left, int bottom,
                                        int right, int top )
{
    bool draw_roads = view_option( ID_VIEW_ROADS );
    bool draw_buildings = view_option( ID_VIEW_BUILDINGS );
    bool draw_structures = view_option( ID_VIEW_STRUCTURES );
    bool draw_water = view_option( ID_VIEW_WATER );
    
    for( int y = bottom; y < top; y++ )
    {
        byte* row = pb_terrain->row(y);
        for( int x = left; x < right; x++ )
        {
            HexCoord h(x+1,y+1);
            Terrain t = map->terrain(h);
            byte b = (x%2==y%2)? GrayColors[18] : GrayColors[22];

            int w = (draw_water)? (map->water(h)) : 0;
            if( w > 0 && t != Bridge )
                b = TRANSPARENCY1( 0x00, water_color(x,y,w) );
            else if( draw_buildings && t == Farm )
                b = 0xC3;
            else if( draw_buildings && t == Houses )
                b = 0xA9;
            else if( draw_buildings && t == Market )
                b = 0x5F;
            else if( draw_roads && ( t == Road || t == Bridge ) )
                b = 0x24;
            else if( draw_structures &&
                     ( t == Wall || t == Gate || t == WatchFire ) )
                b = 0xFF;       // canals are structures too
            
            row[x] = b;
        }
    }
}

void WorldMapWindow::update()
{       
    // Create only resizes if the size has changed
    create_buffer();
    if( !pb )
        return;

    static int skipped_redraws = 0;
    // Don't redo the entire main map too often
    // Instead, just redraw the area that's being viewed
    int left = 0, bottom = 0, right = 0, top = 0;

    // If we do want to redraw everything, just change the limits
    if( create_terrain_map() ||
        map->time_tick_ - last_tick > 10 ||
        map->time_tick_ < 10 )
    {
        // We want to draw something, but how much?
        if( skipped_redraws++ > 50 ||
            map->time_tick_ - last_tick > 100 ||
            map->time_tick_ < 10 )
        {
            // Let's draw the entire map, so erase old info
            fire_on_map = false;            
            skipped_redraws = 0;
            last_tick = map->time_tick_;
            left = 0;
            bottom = 0;
            right = MAP_SIZE_X;
            top = MAP_SIZE_Y;
        }
        else
        {
            // Let's just draw our local area
            View::rect_to_hexarea( view->view->view_area( view->area() ),
                                   left, bottom, right, top );
        }
    }

    // Do some range checks for the world map size [left, right) [bottom, top)
    if( left < 0 ) left = 0;
    if( bottom < 0 ) bottom = 0;
    if( right > pb_terrain->width ) right = pb_terrain->width;
    if( top > pb_terrain->height ) top = pb_terrain->height;

    // Do some range checks for the actual map
    if( right > Map::MSize ) right = Map::MSize;
    if( top > Map::NSize ) top = Map::NSize;

    switch( view_type )
    {
      case VIEW_NORMAL:
          draw_normal( left, bottom, right, top );
          break;
      case VIEW_CIV:
          draw_civilization( left, bottom, right, top );
          break;
      case VIEW_TERRAIN:
          draw_shaded( left, bottom, right, top );
          break;
    }
    
    // Now copy pb_terrain (terrain data only) into pb (incl. blobs & msgs)
    // -> At this point, pb_terrain shouldn't be NULL, and it
    //    should be the same size as pb
    for( int y = 0; y < pb->height; ++y )
    {
        byte* row1 = pb_terrain->row(y);
        byte* row2 = pb->row(y);
        for( int x = 0; x < pb->width; ++x )
            row2[x] = row1[x];
    }

    // Put extra stuff, like where the view is and other messages
    if( GameWindow::bitmaps_initialized )
    {
        Rect area = view->area();
        Point offset = view->future_offset();
        int x0 = offset.x/HexXSpacing;
        int y0 = offset.y/HexYSpacing;

        static int phase = 0;
        phase = (phase+1)%3;

        phase = int(3*clock()/CLK_TCK) % 3;
        
        int yDistance = area.height()/HexYSpacing - 1;
        int xDistance = area.width()/HexXSpacing - 1;
        for( int y = y0+yDistance; y >= y0; y-- )
        {
            if( y >= y0+2 && y <= y0+yDistance-2 )
                continue;
            if( y >= 0 && y < pb->height )
            {
                byte* row = pb->row(y);
                for( int x = x0+xDistance; x >= x0; x-- )
                {
                    if( (y==y0||y==y0+yDistance) &&
                        (x==x0||x==x0+xDistance) )
                        continue;
                    
                    if( x >= 0 && x < pb->width )
                    {
                        int pos = (y<=y0+1)?x:(x0+xDistance-x);
                        pos = (pos+phase+(y==y0)+(y==y0+yDistance)) % 3;
                        if( pos == 0 )
                            row[x] = 0xF9;
                        else if( pos == 1 )
                            row[x] = 0x01;
                        else
                            row[x] = 0xFF;
                    }
                }
            }
        }

        for( int y = y0+yDistance; y >= y0; y-- )
        {
            if( y >= 0 && y < pb->height )
            {
                byte* row = pb->row(y);
                for( int x = x0+xDistance; x >= x0; x-- )
                {
                    if( x >= x0+2 && x <= x0+xDistance-2 )
                        continue;
                    if( (y==y0||y==y0+yDistance) &&
                        (x==x0||x==x0+xDistance) )
                        continue;
                    if( x >= 0 && x < pb->width )
                    {
                        int pos = (x<=x0+1)?(y0+yDistance-y):y;
                        pos = (pos+phase+2*((x==x0)+(x==x0+xDistance))) % 3;
                        if( pos == 0 )
                            row[x] = 0xF9;
                        else if( pos == 1 )
                            row[x] = 0x01;
                        else
                            row[x] = 0xFF;
                    }
                }
            }
        }
        
        extern bool flooding;
        if( flooding )
        {
            const char* s = "Flooding";
            draw_text( *pb, (MAP_SIZE_X-text_width(text12,s))/2, 60, 
                       s, text12, 0xff, 0x00 );
        }

        if( fire_on_map )
        {
            pb->DrawTransparentSprite( fire2_rle, 64, 46 );
            const char* s = "Fire!";
            draw_text( *pb, (MAP_SIZE_X-text_width(text12,s))/2, 40, 
                       s, text12, 0xff, 0x00 );
        }

        if( Map::valid( map->volcano_ ) )
        {
            const char* s = "Volcano!";
            draw_text( *pb, (MAP_SIZE_X-text_width(text12,s))/2, 6, 
                       s, text12, 0xff, 0x00 );
        }
    }
    else
    {
        const char* s = "World Creation";
        draw_text( *pb, (MAP_SIZE_X-text_width(text12i,s))/2, 6, s,
                   text12i, 0x00, 0xff );
    }

    // Now draw all the blobs
    for( vector<Unit*>::iterator u = map->units.begin();
         u != map->units.end(); ++u )
    {
        Unit* unit = *u;
        if( unit->dead() ) continue;

        // Draw a little red rectangle at each blob location
        HexCoord h(unit->hexloc());
        for( int dm = -1; dm <= 1; ++dm )
            for( int dn = -1; dn <= 1; ++dn )
            {
                int m = h.m + dm;
                int n = h.n + dn;
                if( m >= 0 && m < MAP_SIZE_X && n >= 0 && n < MAP_SIZE_Y )
                    pb->row(n)[m] = 0xf9; // 0xfd ?
            }
    }
    
    //  invalidate();
    paint_buffer( area() );
}
