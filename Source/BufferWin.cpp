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

#include "Figment.h"
#include "Sprites.h"
#include "Blitter.h"
#include "Palette.h"
#include "BufferWin.h"

#include "os2_extra.h"

BufferWindow::BufferWindow()
    :palette(NULL), pb(NULL), ps_screen(NULLHANDLE)
{
    blitter = Blitter::create_controls();
}

BufferWindow::~BufferWindow()
{
    blitter->release();
    delete blitter;

    unselect_palette();
    delete pb;
}

void BufferWindow::setup_window( Window* parent_win )
{
    create_window( parent_win->handle(), ClassName, 0, 0, NULLHANDLE, this );
    ps_screen.retarget( handle() );
}

void BufferWindow::evSize()
{
    WinPostMsg( handle(), WM_VRNENABLED, 0, 0 );
    // update() ??
}

void BufferWindow::create_buffer()
{
    destroy_buffer();

    Mutex::Lock lock( buffer_lock );
    Rect area( this->area() );
    if( !pb && area.width() > 0 && area.height() > 0 )
    {
        // Create a new buffer
        pb = new PixelBuffer( area.width(), area.height() );
        for( int i = 0; i < pb->linewidth*pb->height; i++ )
            pb->data[i] = 0;
        blitter->acquire( pb );
        WinPostMsg( handle(), WM_VRNENABLED, 0, 0 );
    }
}

void BufferWindow::destroy_buffer()
{
    Mutex::Lock lock( buffer_lock );
    Rect area( this->area() );  
    if( pb && ( pb->width != area.width() || pb->height != area.height() ) )
    {
        blitter->release();
        delete pb;
        pb = NULL;
    }
}

void BufferWindow::paint_buffer( const Rect& rect )
{
    Mutex::Lock lock( buffer_lock );
    if( pb )
    {
        blitter->draw( ps_screen, 0, 0, rect );
    }   
}

void BufferWindow::evPaint( PS& ps, const Rect& rc )
{
    Mutex::Lock lock( buffer_lock );
    if( pb )
    {
        if( palette != NULL )
            palette->select( ps );
        blitter->draw( ps, 0, 0, rc );
        if( palette != NULL )
            palette->unselect( ps );
    }
    else
        ps.fill_rect( rc, CLR_PALEGRAY );
}

void BufferWindow::select_palette( ColorPalette* p )
{
    palette = p;
    if( palette != NULL )
        palette->select( ps_screen );
}

void BufferWindow::unselect_palette()
{
    if( palette != NULL )
        palette->unselect( ps_screen );
    palette = NULL;
}

void BufferWindow::handle_mouse( int, int, int, unsigned short )
{
    // do nothing
}

MRESULT BufferWindow::WndProc( HWND hWnd, ULONG iMessage, 
                               MPARAM mParam1, MPARAM mParam2 )
{
    switch( iMessage )
    {       
      case WM_CREATE:
      {
          WinSetVisibleRegionNotify( hWnd, true );
          break;
      }
      case WM_DESTROY:
      {
          WinSetVisibleRegionNotify( hWnd, false );
          break;
      }
      case WM_MOUSEMOVE:
      case WM_SINGLESELECT:
      case WM_BEGINSELECT:
      case WM_ENDSELECT:
      case WM_BEGINDRAG:
      case WM_ENDDRAG:
      case WM_CONTEXTMENU:
      case WM_BUTTON1DOWN:
      case WM_BUTTON1UP:
      case WM_MOUSELEAVE:
      {
          handle_mouse( iMessage, SHORT1FROMMP(mParam1), SHORT2FROMMP(mParam1),
                        SHORT2FROMMP(mParam2) );
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
          Rect rect[1];
          int count = rgn.query_rects( 1, rect );
          if( count > 0 )
          {
              blitter->vrgn_enable( handle(), parent()->handle(), rgn );
              invalidate();
          }
          else
              blitter->vrgn_disable();

          break;
      }
    }
    return CustomWindow::WndProc( hWnd, iMessage, mParam1, mParam2 );
}

