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

#ifndef ViewWin_h
#define ViewWin_h

class Map;
class GameWindow;
class Blitter;
class ColorPalette;
class ViewWindow : public CustomWindow
{
  public:
    ViewWindow( GameWindow* gamewin );
    virtual ~ViewWindow();

    virtual void setup_window( Window* parent );

    TID painter_thread_id;
    int painter_thread( int );

    void select_palette( ColorPalette* palette );
    void unselect_palette( ColorPalette* palette );

    void highlight_point( Point p );
    
    View* view;
    Map* map;
    GameWindow* gamewin;

    PixelBuffer* pb;
    Blitter* blitter;

    CachedPS ps_screen;

    bool cursor_valid;
    HexCoord cursor_loc, old_cursor_loc;
    void update_cursor_validity();
    void set_cursor_loc( const HexCoord& h );

    void activate();
    void deactivate();
    void set_scrollbars();

    void MouseMove( int iMessage, int mousex, int mousey, 
                    unsigned short kbdflags );

    virtual void evPaint( PS& ps, const Rect& rc );
    virtual void evSize();

    virtual MRESULT WndProc( HWND hWnd, ULONG iMessage, MPARAM, MPARAM );

    Point future_offset() const;

  protected:
    Mutex painter_access;
    EventSem painter_event;
    Rect painter_rect;
    int painter_scroll_x;
    int painter_scroll_y;
    bool painter_full;
    bool painter_update;
  public:
#define L Mutex::Lock lock( painter_access )
#define P painter_event.post()
    void request_paint( Rect r )
        { L; painter_rect = merge( painter_rect, view->view_area(r) ); P; }
    void request_full_paint()
        { L; painter_rect = view->view_area(area()); painter_full = true; P; }
    void request_full_update()
	    { L; painter_update = true; P; }
    void request_scroll_vert( int newy )
        { L; painter_scroll_y = newy; P; }
    void request_scroll_horiz( int newx )
        { L; painter_scroll_x = newx; P; }
    void request_scroll( int newx, int newy )
        { L; painter_scroll_x = newx; painter_scroll_y = newy; P; }
#undef P
#undef L
};

#endif
