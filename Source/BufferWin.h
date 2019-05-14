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

#ifndef BufferWin_h
#define BufferWin_h

class Blitter;
class ColorPalette;

// This window contains a pixel buffer, and can draw it to the PM window
class BufferWindow : public CustomWindow
{
  public:
    BufferWindow();
    virtual ~BufferWindow();

    virtual void setup_window( Window* parent );

    ColorPalette* palette;
    void select_palette( ColorPalette* palette );
    void unselect_palette();

    PixelBuffer *pb;
    Blitter* blitter;
    void create_buffer();
    void paint_buffer( const Rect& rect );
    void destroy_buffer();

    CachedPS ps_screen;
    Mutex buffer_lock;

    virtual void evPaint( PS& ps, const Rect& rc );
    virtual void evSize();

    virtual void update() = 0;
    virtual void handle_mouse( int msg, int x, int y, unsigned short kbdflag );
    virtual MRESULT WndProc( HWND hWnd, ULONG iMessage, MPARAM, MPARAM );
};

#endif
