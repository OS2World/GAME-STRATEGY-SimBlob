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

#include "Map.h"
#include "View.h"

#include "ViewWin.h"
#include "GameWin.h"
#include "MainWin.h"

#include "Blitter.h"
#include "StatusBar.h"

//////////////////////////////////////////////////////////////////////

extern int hex_style;

ViewWindow::ViewWindow( GameWindow* gw )
    :gamewin(gw), pb(NULL), blitter(NULL),
     view(new View(gw->map)), map(gw->map), ps_screen(NULLHANDLE),
     painter_scroll_x(-1), painter_scroll_y(-1), painter_full(true),
     painter_update(false), painter_rect(), painter_thread_id(0),
     cursor_loc( 0, 0 ), old_cursor_loc( 0, 0 ), cursor_valid( false )
{
    blitter = Blitter::create_map();
}

void ViewWindow::setup_window( Window* parent )
{
    create_window( parent->handle(), ClassName, 0, 0, NULLHANDLE, this );
    ps_screen.retarget( handle() );
}

ViewWindow::~ViewWindow()
{
    blitter->release();
    delete blitter;

    // Wake up the painter thread and kill it
    painter_event.post();
    Figment::kill_thread( painter_thread_id );
    delete view;
}

void ViewWindow::highlight_point( Point p )
{
    view->highlight = p;
}

void ViewWindow::update_cursor_validity()
{
    static const char* firemsg = "Can't erase fires";
    static const char* emptymsg = "";
    extern Subject<string> toolmsg;
    if( map->valid( cursor_loc ) )
    {
        // Check if it's okay to use the current tool here
        extern Subject<int> tb_index;
        int i = tb_index;
        if( i != 6 && i >= 0 && map->terrain( cursor_loc ) == Fire ) 
        {
            set( toolmsg, firemsg );
            cursor_valid = false;
        }
        else
        {
            cursor_valid = true;
            if( i != 6 && i >= 0 )
                for( vector<HexCoord>::iterator j = map->selected.begin();
                     j != map->selected.end(); ++j )
                    if( map->terrain( *j ) == Fire )
                    {
                        set( toolmsg, firemsg );
                        cursor_valid = false;
                        break;
                    }
            if( cursor_valid ) set( toolmsg, emptymsg );
        }
    }
    else
    {
        cursor_valid = false;
        set( toolmsg, emptymsg );
    }
}

void ViewWindow::set_cursor_loc( const HexCoord& h0 )
{
    HexCoord h(h0);
    if( !map->valid( h ) )
        h = HexCoord( 0, 0 );   
    if( cursor_loc == h )
        return;

    if( map->valid( cursor_loc ) )
        request_full_update();
    cursor_loc = h;
    update_cursor_validity();
    if( map->valid( cursor_loc ) )
        request_paint( view->client_area(cursor_loc.rect()) );
}

void ViewWindow::activate()
{
    // increase the priority of the painter thread
    if( painter_thread_id != 0 )
        DosSetPriority( PRTYS_THREAD, PRTYC_REGULAR, +1, painter_thread_id );
}

void ViewWindow::deactivate()
{
    // decrease the painter thread priority to IDLETIME
    if( painter_thread_id != 0 )
        DosSetPriority( PRTYS_THREAD, PRTYC_IDLETIME, +6, painter_thread_id );
    // make the cursor invisible
    set_cursor_loc( HexCoord(0,0) );
}

void ViewWindow::select_palette( ColorPalette* palette )
{
    if( palette )
        palette->select( ps_screen );
    else
        Log("ViewWindow::select_palette","palette==NULL");
    if( blitter )
        blitter->palette_changed( palette );
    else
        Log("ViewWindow::select_palette","blitter==NULL");
}

void ViewWindow::unselect_palette( ColorPalette* palette )
{
    palette->unselect( ps_screen );
    blitter->palette_changed( NULL );
}

Terrain current_tool = Clear;
extern Subject<int> tb_index;
#include "Tools.h"

void ViewWindow::MouseMove( int iMessage, int mousex, int mousey, 
                            unsigned short kbdflags )
{
    static Tool* tools[] = {
    new SelectTool(map, view),
    new BlobTool(map),
    new BuildTool(gamewin, Clear),
    new BuildTool(gamewin, Road),
    new BuildTool(gamewin, Bridge),
    new BuildTool(gamewin, Canal),
    new BuildTool(gamewin, Wall),
    new BuildTool(gamewin, Gate),
    new BuildTool(gamewin, WatchFire),
    new BuildTool(gamewin, Fire),
    new BuildTool(gamewin, Trees)
    };
                  
    current_tool = gamewin->current_terrain_draw();
    Tool& TOOL = (kbdflags & KC_CTRL)? *(tools[1]) : *(tools[tb_index+2]);
    Tool::kbdflags = kbdflags;
    
    static int state = 0;
    static bool inside = true;
    Point p(mousex + view->xoffset, mousey + view->yoffset);
    if( area().contains( Point(mousex,mousey) ) )
    {
        if( !inside ) TOOL.enter();
        inside = true;
    }
    else
    {
        if( inside ) TOOL.leave();
        p = Point(HexXOrigin,HexYOrigin);
        inside = false;
    }
    set_cursor_loc( PointToHex(p.x,p.y) );

    cursor_valid = TOOL.validity(p);
    
    if( state == 'S' )
    {
        // Currently Swiping
        switch( iMessage )
        {
          case WM_MOUSEMOVE:
              TOOL.swipe( p );
              return;
          case WM_BEGINSELECT:
              // Ignore repeated message
              return;
          case WM_ENDSELECT:
              TOOL.end_swipe( p );
              WinSetCapture( HWND_DESKTOP, NULLHANDLE );
              state = 0;
              return;
          default:
              // Cancel the swipe because we found a message we couldn't handle
              TOOL.cancel_swipe();
              WinSetCapture( HWND_DESKTOP, NULLHANDLE );
              state = 0;
              break;
        }
    }
    else if( state == 'D' )
    {
        // Currently Dragging
        switch( iMessage )
        {
          case WM_MOUSEMOVE:
              TOOL.drag( p );
              return;
          case WM_BEGINDRAG:
              // Ignore repeated message
              return;
          case WM_ENDDRAG:
              TOOL.end_drag( p );
              WinSetCapture( HWND_DESKTOP, NULLHANDLE );
              state = 0;
              return;
          default:
              // Cancel the drag
              TOOL.cancel_drag();
              WinSetCapture( HWND_DESKTOP, NULLHANDLE );
              state = 0;
              break;
        }
    }

    switch( iMessage )
    {
      case WM_MOUSEMOVE:
          // Ignore
          return;
      case WM_BEGINSELECT:
          WinSetCapture( HWND_DESKTOP, handle() );
          TOOL.begin_swipe( p );
          state = 'S';
          return;
      case WM_BEGINDRAG:
          WinSetCapture( HWND_DESKTOP, handle() );
          TOOL.begin_drag( p );
          state = 'D';
          return;
      case WM_ENDDRAG:
      case WM_ENDSELECT:
          // These shouldn't occur, but if they do ..
          WinSetCapture( HWND_DESKTOP, NULLHANDLE );
          return;
      case WM_SINGLESELECT:
          TOOL.select( p );
          return;
      case WM_CONTEXTMENU:
      {
          TOOL.contextmenu( p );
          view->highlight = p;
          // train.begin_moving( map, HexCoord(1,1), HexCoord(Map::MSize-2,Map::NSize-2) );
          static HWND menu = WinLoadMenu( handle(), NULL, ID_CONTEXT_MAP );
          WinPopupMenu( handle(), gamewin->handle(), menu, p.x, p.y,
                        ID_CONTEXT_MAP,
                        PU_HCONSTRAIN | PU_VCONSTRAIN |
                        PU_MOUSEBUTTON2 | PU_MOUSEBUTTON1 );
          return;
      }
      case WM_OPEN:
          TOOL.open( p );
          return;
    }
}

Point ViewWindow::future_offset() const
{
    int xoffset = view->xoffset;
    int xoffset_paint = painter_scroll_x;
    if( xoffset_paint >= 0 )
        xoffset = xoffset_paint;
    int yoffset = view->yoffset;
    int yoffset_paint = painter_scroll_y;
    if( yoffset_paint >= 0 )
        yoffset = yoffset_paint;
    return Point(xoffset,yoffset);
}

void ViewWindow::set_scrollbars()
{
    if( !gamewin || !gamewin || !gamewin->vert || !gamewin->horiz )
        return;

    HexCoord minhex_h( 1, 1 );
    HexCoord minhex_v( 2, 1 );
    HexCoord maxhex_h( Map::MSize, 1 );
    HexCoord maxhex_v( 1, Map::NSize );

    int windowheight = area().yTop;
    int windowwidth = area().xRight;

    Point offset = future_offset();

    gamewin->vert->set_minmax( minhex_v.bottom(), maxhex_v.top() );
    int y2 = maxhex_v.top()-offset.y+minhex_v.bottom();
    gamewin->vert->set_position( y2 - windowheight, y2 );
    WinPostMsg( handle(), WM_VSCROLL, (void *)FID_VERTSCROLL, MPFROM2SHORT(gamewin->vert->pos1(),SB_SLIDERPOSITION) );

    gamewin->horiz->set_minmax( minhex_h.left(), maxhex_h.right() );
    gamewin->horiz->set_position( offset.x, windowwidth + offset.x );
    WinPostMsg( handle(), WM_HSCROLL, (void *)FID_HORZSCROLL, MPFROM2SHORT(gamewin->horiz->pos1(),SB_SLIDERPOSITION) );
}

void ViewWindow::evSize()
{
    set_scrollbars();
    request_full_paint();
}

#include "os2_extra.h"

// ViewWindow object window procedure
MRESULT ViewWindow::WndProc( HWND hWnd, ULONG iMessage, MPARAM mParam1, MPARAM mParam2 )
{
    switch( iMessage )
    {       
      case WM_CREATE:
      {
          // Do window initialization stuff here
          WinStartTimer( Figment::hab, hWnd, 1, 175 );
          set_scrollbars();
          WinSetVisibleRegionNotify( hWnd, TRUE );

          break;
      }
      
      case WM_VRNDISABLED:
      {
          blitter->vrgn_disable();
          break;
      }

      case WM_VRNENABLED:
      {
          Region rgn( ps_screen );
          WinQueryVisibleRegion( handle(), rgn );
          Rect rect[50];
          int count = rgn.query_rects( 50, rect );
          if( count > 0 )
              blitter->vrgn_enable( handle(), gamewin->handle(), rgn );
          else
              blitter->vrgn_disable();

          break;
      }

      case DM_DROP:
      {
          DosBeep(1000,100);
          DRAGINFO* drag = (DRAGINFO*)mParam1;
          // DrgAccessDragInfo( drag );
          highlight_point( Point( drag->xDrop, drag->yDrop ) );
          // DrgFreeDragInfo( drag );
          break;
      }
      
      // WinCalcFrameRect might be useful somewhere...

      case WM_CHAR:
      {
        // Use virtual keystrokes and not normal ASCII for now
        CHRMSG* msg = CHARMSG(&iMessage);

        if( msg->fs & KC_KEYUP )
            break;

        // control == msg->fs & INP_CTRL
        int vmsg = 0;
        int hmsg = 0;
        if( msg->fs & INP_CTRL )
        {
            // Handle Ctrl+Left and Ctrl+Right
            if( msg->vkey == VK_LEFT ) msg->vkey = VK_HOME;
            if( msg->vkey == VK_RIGHT ) msg->vkey = VK_END;
        }
        switch( msg->vkey )
        {
          case VK_UP:           vmsg = SB_LINEUP;   break;
          case VK_DOWN:         vmsg = SB_LINEDOWN; break;
          case VK_PAGEUP:       vmsg = SB_PAGEUP;   break;
          case VK_PAGEDOWN:     vmsg = SB_PAGEDOWN; break;

          case VK_LEFT:         hmsg = SB_LINEUP;   break;
          case VK_RIGHT:        hmsg = SB_LINEDOWN; break;
          case VK_HOME:         hmsg = SB_PAGEUP;   break;
          case VK_END:          hmsg = SB_PAGEDOWN; break;
        }

        if( vmsg != 0 )
            WinPostMsg( handle(), WM_VSCROLL, MPARAM(gamewin->vert->handle()), MPFROM2SHORT(0,vmsg) );
        if( hmsg != 0 )
            WinPostMsg( handle(), WM_HSCROLL, MPARAM(gamewin->horiz->handle()), MPFROM2SHORT(0,hmsg) );

        break;
      }

      case WM_VSCROLL:
      {
        int scrollpos = SHORT1FROMMP(mParam2);
        int command = SHORT2FROMMP(mParam2);

        if( !gamewin->vert->enabled() )
            break;

        HexCoord maxhex( 1, Map::NSize );
        HexCoord minhex( 2, 1 );

        if( command == SB_PAGEUP   )    gamewin->vert->set_position( gamewin->vert->pos1() - (gamewin->vert->range()-HexYSpacing) );
        if( command == SB_PAGEDOWN )    gamewin->vert->set_position( gamewin->vert->pos1() + (gamewin->vert->range()-HexYSpacing) );
        if( command == SB_LINEUP   )    gamewin->vert->set_position( gamewin->vert->pos1() - HexYSpacing );
        if( command == SB_LINEDOWN )    gamewin->vert->set_position( gamewin->vert->pos1() + HexYSpacing );

        int where = gamewin->vert->pos2();
        if( command == SB_SLIDERTRACK )
            where += (scrollpos-gamewin->vert->pos1());
        int newoffset = ( maxhex.top()-where ) + minhex.bottom();

        if( command != SB_ENDSCROLL )
            request_scroll_vert( newoffset );
        break;
      }

      case WM_HSCROLL:
      {
        int scrollpos = SHORT1FROMMP(mParam2);
        int command = SHORT2FROMMP(mParam2);

        if( command == SB_PAGEUP   )    gamewin->horiz->set_position( gamewin->horiz->pos1() - (gamewin->horiz->range()-HexXSpacing) );
        if( command == SB_PAGEDOWN )    gamewin->horiz->set_position( gamewin->horiz->pos1() + (gamewin->horiz->range()-HexXSpacing) );
        if( command == SB_LINEUP   )    gamewin->horiz->set_position( gamewin->horiz->pos1() - HexXSpacing );
        if( command == SB_LINEDOWN )    gamewin->horiz->set_position( gamewin->horiz->pos1() + HexXSpacing );

        int pos = ( command == SB_SLIDERTRACK ) ? scrollpos : gamewin->horiz->pos1();

        request_scroll_horiz( pos );
        break;
      }

      case WM_MOUSEMOVE:
      case WM_BEGINDRAG:
      case WM_ENDDRAG:
      case WM_BEGINSELECT:
      case WM_ENDSELECT:
      case WM_SINGLESELECT:
      case WM_CONTEXTMENU:
      {
        if( !GameWindow::bitmaps_initialized )
            break;

        // Check if we have focus
        if( WinQueryActiveWindow( HWND_DESKTOP ) != gamewin->frame()->handle() )
            break;

        static bool pointers_init = false;
        static HPOINTER handpointer = NULL;
        static HPOINTER buildpointer = NULL;
        if( !pointers_init )
        {
            pointers_init = true;
            handpointer = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, IDP_HAND );
            buildpointer = WinLoadPointer( HWND_DESKTOP, NULLHANDLE, IDP_BUILD );
        }

        if( tb_index == -2 )
        { if( handpointer ) WinSetPointer( HWND_DESKTOP, handpointer ); }
        else
        { if( buildpointer ) WinSetPointer( HWND_DESKTOP, buildpointer ); }

        MouseMove( iMessage, SHORT1FROMMP(mParam1), SHORT2FROMMP(mParam1),
                   SHORT2FROMMP(mParam2) );
        return 0;
      }

      case WM_TIMER:
      {
        if( map->valid( cursor_loc ) );
        {
            // I'm trying to detect if the pointer left the window
            Point point;
            Rect rect = area();
            WinQueryPointerPos( HWND_DESKTOP, &point );
            // That point needs to be translated to make it relative to the window
            WinMapWindowPoints( HWND_DESKTOP, handle(), &point, 1 );

            if( !WinPtInRect( Figment::hab, &rect, &point ) )
            {
                set_cursor_loc( HexCoord(0,0) );
                MouseMove( WM_MOUSEMOVE, -1, -1, 0 );
            }
        }

        // Update the cursor style
        if( GameWindow::bitmaps_initialized )
        {
            hex_style++;
            if( map->valid( cursor_loc ) )
                request_paint( view->client_area(cursor_loc.rect()) );
        }

        return 0;
      }

      case WM_MOUSELEAVE:
      {
          MouseMove( WM_MOUSEMOVE, -1, -1, 0 );
          break;
      }
      
      case WM_DESTROY:
      {
          WinSetVisibleRegionNotify( hWnd, FALSE );
          WinStopTimer( Figment::hab, hWnd, 1 );
          break;
      }
    }
    return CustomWindow::WndProc( hWnd, iMessage, mParam1, mParam2 );
}

