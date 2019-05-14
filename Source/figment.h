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

#ifndef Figment_h
#define Figment_h

#pragma hdrstop

#include "Closure.h"
#include "Notion.h"

#ifndef __BORLANDC__
#define _export
#else
#define _Optlink
#endif

// The figment class library deals with OS/2 Presentation Manager windows.
// The Figment class keeps a map of window handles to window objects,
// and also contains static functions for working with OS/2 PM (including
// thread creation and destruction).
class Window;
class PS;
class Figment
{
  public:
    static HAB      hab;
    static HMQ      hmq;

    static void add_association( HWND hwnd, Window* window );
    static void remove_association( HWND hwnd, Window* window );
    static Window* lookup( HWND hwnd );

    static bool Initialize();
    static bool MessageLoop();
    static void Terminate();

    static TID begin_thread( Closure<int,int> thread_function );
    static void kill_thread( TID& id );
};

// Class for all windows
class Window
{
  protected:
    HWND window_;

    virtual void create_window( HWND parent, PSZ winclass,
                                int style, int id, HWND owner = NULLHANDLE, 
                                void* extra = NULL );

    friend MRESULT EXPENTRY _export WndProcNonFigment
    ( HWND, ULONG, MPARAM, MPARAM );
    friend MRESULT EXPENTRY _export WndProcFigment
    ( HWND, ULONG, MPARAM, MPARAM );

  public:
    Window();
    virtual ~Window();

    virtual void setup_window( Window* parent ) = 0;
    HWND handle() { return window_; }
    Window* parent();

    bool show( int nCmdShow );
    bool update();

    // Window text
    void window_text( const char* text );

    // Move and size the window
    void position( Point origin, int width, int height );

    // The area occupied by the window
    Rect area();

    // Let the user move the window around
    void interactive_move();

    // Invalidate a rectangle or the whole (client) window
    void invalidate();
    void invalidate( Rect area );

    virtual MRESULT WndProc( HWND hWnd, ULONG iMessage, 
                             MPARAM mParam1, MPARAM mParam2 );
};

// Class for windows customized by our window procedure
class CustomWindow: public Window
{
  protected:
    static PSZ ClassName;
  private:
    // Window registration for our custom window class
    static bool initialized;
    static void Register();

  public:
    CustomWindow();
    virtual ~CustomWindow();

    // Immediately repaint an area
    void repaint( Rect area );

    // Handle WM_COMMAND messages
    virtual bool evCommand( USHORT command, HWND hWnd );
    // Respond to resizing
    virtual void evSize();
    // Paint the window
    virtual void evPaint( PS& ps, const Rect& rc );

    // Message handler
    virtual MRESULT WndProc( HWND hWnd, ULONG iMessage, MPARAM, MPARAM );
};

class PMWindow: public Window
{
  protected:
    PFNWP old_proc_;
    virtual void create_window( HWND parent, PSZ winclass,
                                int style, int id, HWND owner = NULLHANDLE, 
                                void* extra = NULL );

  public:
    PMWindow() {}
    virtual ~PMWindow() {}

    void pp_foreground( Color c );
    void pp_background( Color c );
    void pp_font( const char* f );

    virtual MRESULT WndProc( HWND hWnd, ULONG iMessage, MPARAM, MPARAM );
    static PMWindow* created_window; 
};

// Abstract class for all app windows
class FrameWindow: public PMWindow
{
  public:
    FrameWindow() {}
    virtual ~FrameWindow() {}

    virtual Window* client() = 0;

    // Change the titlebar text
    void titlebar_text( const char* text ) { window_text( text ); }

    // Close the window
    inline void close() { WinPostMsg( client()->handle(), WM_CLOSE, 0, 0 ); }
    // Quit the program
    inline void quit_application()
    { WinPostMsg( client()->handle(), WM_QUIT, 0, 0 ); }
};

class PushButton: public PMWindow
{
    int id_;
    char* text_;
  public:
    PushButton( int id, const char *text );
    virtual ~PushButton();

    virtual void setup_window( Window* parent );
};

class Label: public PMWindow
{
    char* text_;
    int alignment_;
  public:
    Label( const char *text );
    Label( const char *text, int alignment );
    virtual ~Label();

    virtual void setup_window( Window* parent );
};

class ScrollBar: public PMWindow
{
    int id_;
    int low_, high_, pos1_, pos2_;
  public:
    ScrollBar( int id );
    virtual ~ScrollBar();

    virtual int window_style() = 0;
    virtual void setup_window( Window* parent );

    void update();
    void set_minmax( int low, int high );
    void set_position( int pos );
    void set_position( int pos1, int pos2 );
    int pos1();
    int pos2();
    int range();
    int id() { return id_; }
    bool enabled();
};

class HorizScrollBar: public ScrollBar
{
  public:
    HorizScrollBar( int id );
    virtual int window_style();
};

class VertScrollBar: public ScrollBar
{
  public:
    VertScrollBar( int id );
    virtual int window_style();
};

class SimpleFrame: public FrameWindow
{
    Window* client_;
    int id_;
    HWND owner_;

  public:
    SimpleFrame( Window* client, int id, HWND owner = NULLHANDLE )
        :client_(client), id_(id), owner_(owner) {}
    virtual ~SimpleFrame() {}

    virtual void setup_window( Window* parent );
    virtual Window* client() { return client_; }    
};

class PS
{
  protected:
    HPS hps;
    
    // Much of this class is inlined so that unused functions don't use space
  public:
    PS( HPS _hps = NULLHANDLE ): hps(_hps) {}
    inline PS( const PS& ps ): hps(ps.hps) {}
    inline ~PS() {}
    inline PS& operator = ( const PS& ps ) { hps = ps.hps; return *this; }

    inline operator HPS() { return hps; }

    inline void rgb_mode()
    { GpiCreateLogColorTable( hps, 0, LCOLF_RGB, 0, 0, NULL ); }
    inline void cmap_mode()
    { GpiCreateLogColorTable( hps, LCOL_RESET, LCOLF_INDRGB, 0, 0, NULL ); }

    inline void color( Color c ) { GpiSetColor( hps, c ); }
    inline void back_color( Color c ) { GpiSetBackColor( hps, c ); }

    Color nearest_color( Color c );

    enum Transparency { OpaqueFont, TransparentFont };
    inline void text_mode( Transparency t )
    {
        GpiSetBackMix( hps, t == OpaqueFont? BM_OVERPAINT : BM_LEAVEALONE );
    }
    inline void text( Point pt, const char* text )
    {
        GpiCharStringAt( hps, &pt, strlen(text), PSZ(text) );
    }

    void font( const char* face, int size, bool bold = 0 );
    int text_width( const char* s );

    inline void moveto( Point pt ) { GpiMove( hps, &pt ); }
    inline void moveto( int x, int y ) { moveto(Point(x,y)); }
    inline void lineto( Point pt ) { GpiLine( hps, &pt ); }
    inline void lineto( int x, int y ) { lineto(Point(x,y)); }
    inline void line( Point p1, Point p2 ) { moveto(p1); lineto(p2); }

    inline void pixel( Point p, Color c )
    {
        GpiSetColor( hps, c );
        GpiSetPel( hps, &p );
    }

    inline Color pixel( Point p ) { return GpiQueryPel( hps, &p ); }

    inline void fill_rect( const Rect& rect, Color c )
    {
        WinFillRect( hps, const_cast<Rect*>(&rect), c );
    }
    
    inline void fill_rect( Point p1, Point p2, Color c )
    {
        fill_rect( Rect(p1.x,p1.y,p2.x,p2.y), c );
    }
    
    inline void bitblt( const PS& ps, const Rect& target, Point source, 
                        int rop_mode = ROP_SRCCOPY )
    {
        Point points[] = { target.bottom_left(), target.top_right(), source };
        GpiBitBlt( hps, ps.hps, 3, points, rop_mode, BBO_IGNORE );
    }
};

class CachedPS: public PS
{
  public:
    CachedPS( HWND window );
    ~CachedPS();

    void retarget( HWND window );
};

class PainterPS: public PS
{
    RECTL rc;       // Not a Rect because of Rect's constructor
  public:
    PainterPS( HWND window ): PS( WinBeginPaint( window, HPS(NULL), &rc ) ) {}
    ~PainterPS() { WinEndPaint( hps ); }
    Rect paint_area() { return Rect(rc.xLeft,rc.yBottom,rc.xRight,rc.yTop); }
};

void ErrorBox( const char* text1, const char* text2 );

#endif
