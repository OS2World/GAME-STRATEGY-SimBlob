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
#include "deque.h"

#include "Figment.h"                    // Figment class library
#include "SimBlob.h"                    // Resource file symbolics

#include "Bitmaps.h"
#include "Sprites.h"
#include "Blitter.h"

#include "Map.h"
#include "View.h"

#include "ViewWin.h"
#include "GameWin.h"
#include "MainWin.h"

#include "os2_extra.h"

void UpdateDamage( Rect& rect, const HexCoord& h )
{
    rect = merge( rect, h.rect() );
}

static void ScrollUp( PixelBuffer& buffer, int dy )
{
    if( dy < buffer.height )
    {
        memmove( buffer.data + buffer.linewidth*dy, buffer.data, 
                 buffer.linewidth * (buffer.height-dy) );
    }
    else
        dy = buffer.height;
    memset( buffer.data, 0, buffer.linewidth * dy );
}

static void ScrollDown( PixelBuffer& buffer, int dy )
{
    if( dy < buffer.height )
    {
        memmove( buffer.data, buffer.data + buffer.linewidth*dy,
                 buffer.linewidth * (buffer.height-dy) );
    }
    else
        dy = buffer.height;
    memset( buffer.data + buffer.linewidth * (buffer.height-dy), 
            0, buffer.linewidth * dy );
}

static void ScrollRight( PixelBuffer& buffer, int dx )
{
    if( dx >= buffer.width )
        dx = buffer.width;
    for( int y = 0; y < buffer.height; y++ )
    {
        byte* row = buffer.data + y*buffer.linewidth;
        if( dx < buffer.width )
            memmove( row + dx, row, buffer.width-dx );
        memset( row, 0, dx );
    }
}

static void ScrollLeft( PixelBuffer& buffer, int dx )
{
    if( dx >= buffer.width )
        dx = buffer.width;
    for( int y = 0; y < buffer.height; y++ )
    {
        byte* row = buffer.data + y*buffer.linewidth;
        if( dx < buffer.width) 
            memmove( row, row + dx, buffer.width-dx );
        memset( row + buffer.width - dx, 0, dx );
    }
}

int ViewWindow::painter_thread(int)
{
    HAB hab = WinInitialize( 0 );
    HMQ hmq = WinCreateMsgQueue( hab, 0 );
    WinCancelShutdown( hmq, true );

    // Increase the paint priority for better response times
    DosSetPriority( PRTYS_THREAD, PRTYC_REGULAR, +3, 0 );

    const HexCoord maxhex_h( Map::MSize, 1 );
    const HexCoord maxhex_v( 1, Map::NSize );

    while( SimBlobWindow::program_running )
    {
        if( painter_event.wait( 200 ) )
        {
            Rect area = this->area();
            
            if( pb && ( pb->width != area.width()
                         || pb->height != area.height() ) )
            {
                blitter->release();
                delete pb;
                pb = NULL;
            }

            Rect clipping;
            int dx = 0;
            int dy = 0;
            bool paint_full = false;
            bool update_full = false;
            {
                Mutex::Lock lock( painter_access );

                if( !pb )
                {
                    // First create a new buffer
                    pb = new PixelBuffer(area.width(),area.height());
                    blitter->acquire( pb );
                    WinPostMsg( handle(), WM_VRNENABLED, 0, 0 );
                    for( int i = 0; i < pb->linewidth*pb->height; i++ )
                        pb->data[i] = 0;

                    // Now set everything up to repaint it completely
                    clipping = area;
                    paint_full = true;

                    if( painter_scroll_x >= 0 ) view->xoffset = painter_scroll_x;
                    if( painter_scroll_y >= 0 ) view->yoffset = painter_scroll_y;
                    if( view->xoffset <= 0 )
                        view->xoffset = 0;
                    else if( view->xoffset + area.width() > maxhex_h.right() )
                        view->xoffset = maxhex_h.right() - area.width();
                    if( view->yoffset <= 0 )
                        view->yoffset = 0;
                    else if( view->yoffset + area.height() > maxhex_v.top() )
                        view->yoffset = maxhex_v.top() - area.height();

                    painter_scroll_x = -1;
                    painter_scroll_y = -1;
                    painter_rect = Rect();
                    painter_update = false;
                    painter_full = false;
                }
                else if( !WinIsWindowShowing( handle() ) )
                {
                    // Don't bother doing anything if the window can't be seen
                    painter_event.reset();
                    continue;
                }
                else if( painter_scroll_x >= 0 )
                {
                    dx = view->xoffset - painter_scroll_x;
                    painter_scroll_x = -1;

                    // Is there also a y scroll needed?
                    if( painter_scroll_y >= 0 )
                    {
                        // We might want to scroll y at the same time
                        int new_dy = view->yoffset - painter_scroll_y;
                        int xs = max( 0, (area.width()-abs(dx)) );
                        int ys = max( 0, (area.height()-abs(new_dy)) );
                        int a0 = xs*ys, a1 = area.width()*area.height();
                        bool bigjump = ( a0 > -a1*2/5 && a0 < a1*2/5 );
                        bool usedive =
                            Blitter::blit_map_type==Blitter::UseDive;
                        
                        if( bigjump || usedive )
                        {
                            // It's a big scroll, so let's scroll y as well
                            dy = new_dy;
                            painter_scroll_y = -1;
                            paint_full = true;
                        }
                    }
                    painter_event.reset();
                }
                else if( painter_scroll_y >= 0 )
                {
                    dy = view->yoffset - painter_scroll_y;
                    painter_scroll_y = -1;
                    painter_event.reset();
                }
                else if( painter_full )
                {
                    paint_full = true;
                    painter_full = false;
                    painter_update = false;
                    painter_rect = Rect();
                    clipping = area;
                }
                else if( painter_update )
                {
                    update_full = true;
                    painter_update = false;
                    clipping = area;
                }
                else if( painter_rect.width() > 0 )
                {
                    clipping = intersect( view->client_area(painter_rect), 
                                          area );
                    painter_rect = Rect();
                }
                else
                {
                    painter_event.reset();
                    continue;
                }
            }

            if( dy != 0 || dx != 0 )
            {
                view->xoffset -= dx;
                view->yoffset -= dy;
                // let's go on with the painting procedure to get 
                // the rest painted
                paint_full = true;
                clipping = area;

                if( dx > 0 && dy == 0 )
                {
                    clipping.xRight = clipping.xLeft + dx;
                    ScrollRight( *pb, dx );
                }
                else if( dx < 0 && dy == 0 )
                {
                    clipping.xLeft = clipping.xRight + dx;
                    ScrollLeft( *pb, -dx );
                }
                else if( dy > 0 && dx == 0 )
                {
                    clipping.yTop = clipping.yBottom + dy;
                    ScrollUp( *pb, dy );
                }
                else if( dy < 0 && dx == 0 )
                {
                    clipping.yBottom = clipping.yTop + dy;
                    ScrollDown( *pb, -dy );
                }
                else
                    // Both axes -- redraw everything
                    // later optimization -- use DamageArea to 
                    // draw only some areas
                    ;
            }

            DamageArea to_paint( clipping );

            if( paint_full || update_full )
            {
                if( paint_full )
                {
                    // If it's a full paint, we might need 
                    // to paint areas that are not mapped
                    view->mark_damaged();
                }
                // damaged holds the area of the map that changed
                DamageArea damaged( view->view_area( clipping ) );
                HexCoord c;
                if( cursor_valid ) c = cursor_loc;
                view->update( view->view_area( clipping ), damaged, pb, c );
                // Update cursor location
                old_cursor_loc = cursor_loc;
                
                if( !paint_full )
                {
                    // If it's not a full paint, we can just 
                    // draw things that are updated, so compute
                    // the bounding rectangle (later, change
                    // this to use the region)
                    for( int i = 0; i < damaged.num_rects(); i++ )
                    {
                        Rect damage_rect = damaged.rect(i);
                        to_paint += view->client_area(damage_rect);
                    }
                }
                else
                    to_paint += clipping;
            }
            else
                to_paint += clipping;

            // No more clipping
            to_paint.clip_to( area );

            if( to_paint.num_rects() > 0 )
            {
                if( pb )
                {
                    bool scrolling_needed = ( dy != 0 || dx != 0 );
                    
                    // Special case:  DIVE can selectively blit lines
                    // but not columns, so if we have to blit all lines
                    // anyway (horiz scrolling), don't bother with the
                    // WinScrollWindow call.  Actually, it doesn't seem
                    // to work right anyway (mixing PM & DIVE) so right
                    // now it blits everything for any scrolling op
                    if( scrolling_needed &&
                        Blitter::blit_map_type == Blitter::UseDive )
                    {
                        to_paint += area;
                        scrolling_needed = false;
                    }
                    if( scrolling_needed )
                    {
                        Region rgn( ps_screen );
                        WinScrollWindow( handle(), dx, dy, 
                                         NULL, NULL, rgn, NULL, 0 );
                        Rect rc[50];
                        int num = rgn.query_rects( 50, rc );
                        for( int i = 0; i < num; i++ )
                            to_paint += rc[i];
                    }

                    extern int drwcount;
                    drwcount++;
                    extern int pixcount;
                    int pixels = blitter->draw( ps_screen, 0, 0, to_paint );
                    pixcount += pixels;
                }
                else
                    DosBeep( 100, 100 );
            }
        }
    }

    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );

    painter_thread_id = 0;
    return 0;
}

void ViewWindow::evPaint( PS& ps_screen, const Rect& rc )
{
    if( !GameWindow::bitmaps_initialized || map == NULL || view == NULL )
    {
        // This is only used before the map is initialized
        Rect cl = area();

        ps_screen.fill_rect( rc, CLR_WHITE );
        ps_screen.font( "Tms Rmn", 64, false );
        ps_screen.color( CLR_BLACK );
        ps_screen.text( Point((cl.xLeft+cl.xRight-ps_screen.text_width("BlobCity"))/2,cl.yTop-70), "BlobCity" );

        return;
    }

    request_paint( rc );
}

