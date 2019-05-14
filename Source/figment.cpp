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
#include <process.h>
#include <memory.h>
#include <map.h>

// Names
//  Imagine
//  Flight
//  Create
//  Inspire
//  Invent
//  Image
//  Castle
//  Dream
//  Vision
//  Notion
//  Figment
//  Illusion
//  Picture
//  Globe

// Declare and initialize all globals
HAB Figment::hab = 0;
HMQ Figment::hmq = 0;
bool CustomWindow::initialized = false;

static map<HWND,Window*> window_map;

// Standard PM Initialization sequence
bool Figment::Initialize()
{
    if( (hab = WinInitialize(NULL)) == (HAB)NULL ) return false;
    if( (hmq = WinCreateMsgQueue(hab,0)) == (HMQ)NULL ) return false;
    return true;
}

// Standard PM Termination sequence
void Figment::Terminate()
{
    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );
}

// Begin a thread in an action
static Mutex begin_thread_mutex;
EventSem begin_thread_event;
void _Optlink begin_thread_closure( void* data )
{
    Closure<int,int> action = *( reinterpret_cast<Closure<int,int>*>(data) );
    begin_thread_event.post();
    action( 0 );
}

TID Figment::begin_thread( Closure<int,int> action )
{
    const int K = 1024;
    const int buffer = 6*K;
    const int stacksize = 1024*K - buffer;

    Mutex::Lock lock( begin_thread_mutex );
    begin_thread_event.reset();
    TID tid = _beginthread( begin_thread_closure, NULL, stacksize,
                            reinterpret_cast<void*>(&action) );
    begin_thread_event.wait();
    return tid;
}

void Figment::kill_thread( TID& id )
{
    DosSetPriority( PRTYS_THREAD, PRTYC_REGULAR, +31, id ); 
    for( int i = 0; i < 5; i++ )
    {
        if( id != 0 )
            DosWaitThread( &id, DCWW_NOWAIT );
        if( id != 0 )
            DosSleep( 100 );
    }

    if( id != 0 )
    {
        DosKillThread( id );
        id = 0;
    }
}
    
Window* Figment::lookup( HWND wnd )
{
    map<HWND,Window*>::iterator i = window_map.find(wnd);
    if( i != window_map.end() )
        return i->second;
    else
        return NULL;
}

void Figment::add_association( HWND hwnd, Window* window )
{
    window_map[hwnd] = window;
}

void Figment::remove_association( HWND hwnd, Window* )
{
    window_map.erase(hwnd);
}

// Standard PM Message Loop
bool Figment::MessageLoop()
{
    bool rc;
    QMSG qmsg;
    for(;;)
    {
        rc = bool( WinGetMsg( hab, &qmsg, (HWND)NULL, 0, 0 ) );
        if( !rc ) break;
        WinDispatchMsg( hab, &qmsg );
        
        // Assert( _heapset(0xF00FBABE) == _HEAPOK );
        // Assert( _heapchk() == _HEAPOK );
    }
    return rc;
}


//////////////////////////////////////////////////////////////////////

Window::Window() : window_(NULLHANDLE)
{
}

Window::~Window()
{
    if( window_ != NULLHANDLE )
    {
        WinDestroyWindow( window_ );
        Figment::remove_association( window_, this );
    }
}

bool Window::show( int nCmdShow )
{
    return WinShowWindow( handle(), nCmdShow );
}

bool Window::update()
{
    return WinUpdateWindow( handle() );
}

void Window::create_window( HWND parent, PSZ winclass,
                            int style, int id, HWND owner, void* extra )
{
    window_ = WinCreateWindow( parent, winclass, NULL, style, 0, 0, 0, 0,
                               owner, HWND_TOP, id, extra, NULL );
    Figment::add_association( window_, this );
}

void Window::window_text( const char* text )
{
    WinSetWindowText( window_, text );
}

void Window::invalidate()
{
    WinInvalidateRect( window_, NULL, FALSE );
}

void Window::invalidate( Rect rect )
{
    if( rect.xLeft < rect.xRight && rect.yBottom < rect.yTop )
        WinInvalidateRect( window_, &rect, FALSE );
}

void Window::position( Point origin, int width, int height )
{
    WinSetWindowPos( handle(), HWND_TOP, 
                     origin.x, origin.y, width, height,
                     SWP_SIZE | SWP_MOVE | SWP_SHOW );
}

Rect Window::area()
{
    Rect rect;
    if( window_ != NULLHANDLE )
        WinQueryWindowRect( window_, &rect );
    return rect;
}

Window* Window::parent()
{
    HWND p = WinQueryWindow( handle(), QW_PARENT );
    return Figment::lookup( p );
}

#if 0
// This is just some code I found to make windows move around... at
// some point in time I had thought about making little floating
// message windows that would be draggable.
switch( msg )
 {
    case WM_BEGINDRAG:
    {
        WinSendMsg(WinQueryWindow(hwnd, QW_PARENT),
                   WM_TRACKFRAME, MPFROMSHORT(TF_MOVE | TF_STANDARD), 
                   0);
        return (MRESULT)TRUE;
    }
 }

void Window::interactive_move()
{
    LONG cxScreen, cyScreen;
    TRACKINFO ti;
    RECTL rc;
    SWP swp;

    // Get screen boundaries
    cxScreen = WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
    cyScreen = WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );

    ti.cxBorder = 3;        // set border width
    ti.cyBorder = 3;
    ti.cxGrid = 0;          // set tracking grid
    ti.cyGrid = 0;
    ti.cxKeyboard = 4;      // set keyboard increment
    ti.cyKeyboard = 4;

    // Set screen boundaries for tracking
    ti.rclBoundary.xLeft  = 0;
    ti.rclBoundary.xRight = cxScreen;
    ti.rclBoundary.yTop   = cyScreen;
    ti.rclBoundary.yBottom = 0;

    WinQueryWindowRect( handle(), &rc );         // get window bounds

    // Set minimum/maximum size for window
    // (here same as the window size)
    ti.ptlMinTrackSize.x = rc.xRight -  rc.xLeft ;
    ti.ptlMinTrackSize.y = rc.yTop   -  rc.yBottom ;
    ti.ptlMaxTrackSize.x = rc.xRight -  rc.xLeft ;
    ti.ptlMaxTrackSize.y = rc.yTop   -  rc.yBottom ;

    // Get initial window position and set
    // initial tracking location
    swp.hwnd = handle();
    WinQueryWindowPos( handle(), &swp );
    ti.rclTrack.xLeft   = swp.x;
    ti.rclTrack.yBottom = swp.y;
    ti.rclTrack.xRight  = swp.x + swp.cx;
    ti.rclTrack.yTop    = swp.y + swp.cy;

    // Track the move, return when mouse is lifted
    ti.fs = TF_MOVE ;
    if( !WinTrackRect( HWND_DESKTOP, NULL, &ti ) ) return;

    // Do the move.
    swp.fl = SWP_MOVE;
    WinSetWindowPos( handle(), HWND_DESKTOP,
        ti.rclTrack.xLeft,
        ti.rclTrack.yBottom,
        ti.rclTrack.xRight - ti.rclTrack.xLeft,
        ti.rclTrack.yTop   - ti.rclTrack.yBottom,
        swp.fl );
}
#endif

MRESULT Window::WndProc( HWND hWnd, ULONG iMessage, MPARAM m1, MPARAM m2 )
{
    return WinDefWindowProc( hWnd, iMessage, m1, m2 );
}

//////////////////////////////////////////////////////////////////////

PSZ CustomWindow::ClassName = "Figment";

CustomWindow::CustomWindow()
{
    if( !initialized ) Register();
}

CustomWindow::~CustomWindow()
{
}

void CustomWindow::Register()
{
    if( !WinRegisterClass( Figment::hab, CustomWindow::ClassName, 
                           ::WndProcFigment,
                           CS_SIZEREDRAW,// |CS_CLIPCHILDREN|CS_CLIPSIBLINGS, 
                           sizeof(CustomWindow *) ) )
        Throw("Can't register class");
    initialized = true;
}

void CustomWindow::repaint( Rect rect )
{
    if( window_ != NULLHANDLE )
    {
        CachedPS ps( window_ );
        evPaint( ps, rect );
    }
}

MRESULT CustomWindow::WndProc( HWND hWnd, ULONG iMessage, MPARAM m1, MPARAM m2 )
{
    switch( iMessage )
    {
      case WM_SIZE:
        evSize();
        break;

        // WinMapWindowPoints turns desktop coords -> window coords
        // GpiQueryColorIndex
        // GpiQueryRGBColor

      case WM_PAINT:
        // Window draw stuff here
        {
            PainterPS ps( hWnd );
            Rect rc( ps.paint_area() );

            // HPOINTER old_pointer = WinQueryPointer( HWND_DESKTOP );
            // WinSetPointer( HWND_DESKTOP, WinQuerySysPointer( HWND_DESKTOP, SPTR_WAIT, FALSE ) );

            ps.color( CLR_NEUTRAL );
            ps.back_color( CLR_BACKGROUND );  // set text background
            ps.text_mode( PS::TransparentFont );
            evPaint( ps, rc );

            // WinSetPointer( HWND_DESKTOP, old_pointer );

            return 0;
        }

      case WM_COMMAND:
        if( evCommand( SHORT1FROMMP(m1), hWnd ) )
            return 0;
        break;

      case WM_CLOSE:
        break;
    }
    return Window::WndProc( hWnd, iMessage, m1, m2 );
}

bool CustomWindow::evCommand( USHORT /*command*/, HWND /*hWnd*/ )
{
    return false;   // we couldn't handle it
}

void CustomWindow::evSize()
{
}

void CustomWindow::evPaint( PS& /*ps*/, const Rect& /*rc*/ )
{
}

MRESULT EXPENTRY _export 
WndProcFigment( HWND hWnd, ULONG iMessage, MPARAM mParam1, MPARAM mParam2 )
{
    // Pointer to the c++ window object
    Window *pWindow =
        reinterpret_cast<Window *>( WinQueryWindowULong( hWnd, 0 ) );
    if( pWindow == 0 )
    {
        if( iMessage == WM_CREATE )
        {
            pWindow = reinterpret_cast<Window *>( mParam1 );
            WinSetWindowULong( hWnd, 0, (ULONG)pWindow );
            pWindow->window_ = hWnd;
            return pWindow->WndProc( hWnd, iMessage, mParam1, mParam2 );
        }
        else
        {
            // This should never happen, unless someone's using
            // WinStdCreateWindow, in which case other things will
            // break!
            Throw("Invalid Message");           
        }
    }
    return pWindow->WndProc( hWnd, iMessage, mParam1, mParam2 );
}


//////////////////////////////////////////////////////////////////////

static Mutex created_window_mutex;
PMWindow *PMWindow::created_window = NULL;

MRESULT PMWindow::WndProc( HWND a, ULONG b, MPARAM c, MPARAM d )
{
    if( old_proc_ )
        return old_proc_(a,b,c,d);
    else
        return WinDefWindowProc( a, b, c, d );
}

void PMWindow::create_window( HWND parent, PSZ winclass,
                              int style, int id, HWND owner, void* extra )
{
    created_window_mutex.lock();
    created_window = this;
    Window::create_window( parent, winclass, style, id, owner, extra );
    old_proc_ = WinSubclassWindow( handle(), WndProcNonFigment );
}

MRESULT EXPENTRY _export 
WndProcNonFigment( HWND hWnd, ULONG iMessage, MPARAM mParam1, MPARAM mParam2 )
{
    PMWindow *pWindow = reinterpret_cast<PMWindow *>(Figment::lookup(hWnd));
    if( pWindow == 0 )
    {
        if( iMessage == WM_CREATE && PMWindow::created_window != NULL )
        {
            // It must be in created_window
            pWindow = PMWindow::created_window;
            PMWindow::created_window = NULL;
            created_window_mutex.unlock();

            pWindow->window_ = hWnd;
            return pWindow->WndProc( hWnd, iMessage, mParam1, mParam2 );
        }
        else
        {
            char s[250];
            sprintf(s,"hWnd=%ux, msg=%4x, mp1=%4x, mp2=%4x",hWnd,iMessage,int(mParam1),int(mParam2));
            Log("[Msg Handler]",s);
            return WinDefWindowProc( hWnd, iMessage, mParam1, mParam2 );
        }
    }
    return pWindow->WndProc( hWnd, iMessage, mParam1, mParam2 );
}

#if 0
// These are functions for setting presentation parameters.
// I experimented with them some, and got some nice screen
// shots, but I stopped using them.

void PMWindow::pp_foreground( Color c )
{
    RGB rgb = { ColorToBlue(c), ColorToGreen(c), ColorToRed(c) };
    WinSetPresParam( handle(), PP_FOREGROUNDCOLOR, sizeof(RGB), &rgb );
}
    
void PMWindow::pp_background( Color c )
{
    RGB rgb = { ColorToBlue(c), ColorToGreen(c), ColorToRed(c) };
    WinSetPresParam( handle(), PP_BACKGROUNDCOLOR, sizeof(RGB), &rgb );
}

void PMWindow::pp_font( const char* f )
{
    WinSetPresParam( handle(), PP_FONTNAMESIZE, strlen(f)+1, (void *)f );
}
#endif    
    
//////////////////////////////////////////////////////////////////////

void SimpleFrame::setup_window( Window* )
{
    FRAMECDATA fcdata;

    fcdata.cb            = sizeof( FRAMECDATA );
    fcdata.flCreateFlags = FCF_TITLEBAR;
    fcdata.hmodResources = 0;
    fcdata.idResources   = 0;

    create_window( HWND_DESKTOP, WC_FRAME, 0, id_, owner_, &fcdata );
    client() -> setup_window( this );
    WinSetWindowPos( handle(), HWND_TOP, 0, 0, 0, 0, SWP_ACTIVATE | SWP_SHOW );
    position( Point(1,250), 44, 250 );
}


//////////////////////////////////////////////////////////////////////

void PS::font( const char* face, int size, bool bold )
{
    FATTRS fat;

    fat.usRecordLength = sizeof(FATTRS);
    fat.fsSelection = bold? FATTR_SEL_BOLD : 0;
    fat.lMatch = 0L;
    fat.idRegistry = 0;
    fat.usCodePage = 850;
    fat.lMaxBaselineExt = 0;
    fat.lAveCharWidth = 0;
    fat.fsType = 0; // FATTR_TYPE_ANTIALIASED;
    fat.fsFontUse = FATTR_FONTUSE_NOMIX | FATTR_FONTUSE_OUTLINE;
    strcpy( fat.szFacename, face );

    GpiCreateLogFont( hps,        /* presentation space               */
                      NULL,       /* does not use logical font name   */
                      1L,         /* local identifier                 */
                      &fat );     /* structure with font attributes   */
    GpiSetCharSet( hps, 1L );     /* sets font for presentation space */
    SIZEF sz;
    sz.cx = MAKEFIXED( size,0 );
    sz.cy = MAKEFIXED( size,0 );
    GpiSetCharBox( hps, &sz );
}

int PS::text_width( const char* s )
{
    int len = strlen(s);
    POINTL ptl;
    POINTL *aptl = new POINTL[len+1];
    int width;

    ptl.x = ptl.y = 0;
    if( !GpiQueryCharStringPosAt( hps, &ptl, 0L, len, PCH(s), 0, aptl ) )
        Throw("Could not get text_width");
    width = aptl[len].x;
    delete [] aptl;
    return width;
}

//////////////////////////////////////////////////////////////////////
CachedPS::CachedPS( HWND window )
    :PS( window == NULLHANDLE? NULLHANDLE : WinGetPS( window ) )
{
}

CachedPS::~CachedPS()
{
    if( hps != NULLHANDLE )
        WinReleasePS( hps );
}

void CachedPS::retarget( HWND window )
{
    if( hps != NULLHANDLE )
        WinReleasePS( hps );
    hps = window == NULLHANDLE? NULLHANDLE : WinGetPS( window );
}

//////////////////////////////////////////////////////////////////////

// AbortProgram -- report an error returned from an API service.
VOID AbortProgram( HWND hwndFrame, HWND hwndClient )
{
    PERRINFO  pErrInfoBlk;
    PSZ       pszOffSet;
    PSZ  pszErrMsg;

    DosBeep( 100, 10 );
    DosBeep( 440, 110 );
    pErrInfoBlk = WinGetErrorInfo( Figment::hab );
    if( pErrInfoBlk != NULL )
    {
        pszOffSet = ((PSZ)pErrInfoBlk) + pErrInfoBlk->offaoffszMsg;
        pszErrMsg = ((PSZ)pErrInfoBlk) + *((PSHORT)pszOffSet);
        if( hwndFrame && hwndClient )
            WinMessageBox( HWND_DESKTOP,     // Parent window is desk top
                       hwndFrame,            // Owner window is our frame
                       (PSZ)pszErrMsg,       // PMWIN Error message
                       (PSZ)"Error Msg",     // Title bar message
                       0,                    // Message identifier
                       MB_MOVEABLE | MB_CUACRITICAL | MB_CANCEL ); // Flags
        else
            WinMessageBox( HWND_DESKTOP,     // Parent window is desk top
                       HWND_DESKTOP,         // Owner window is desktop
                       (PSZ)pszErrMsg,       // PMWIN Error message
                       (PSZ)"Error Msg",     // Title bar message
                       0,                    // Message identifier
                       MB_MOVEABLE | MB_CUACRITICAL | MB_CANCEL ); // Flags
        WinFreeErrorInfo( pErrInfoBlk );
    }
    WinPostMsg( hwndClient, WM_QUIT, (ULONG)0, (ULONG)0 );
}

//////////////////////////////////////////////////////////////////////

ScrollBar::ScrollBar( int id )
    : id_(id), low_(0), high_(100), pos1_(0), pos2_(0)
{
}

ScrollBar::~ScrollBar()
{
}

void ScrollBar::setup_window( Window* parent_window )
{
    int style = SBS_AUTOTRACK | window_style();
    create_window( parent_window->handle(), WC_SCROLLBAR, style, 
                   id_, parent_window->handle() );
    update();
}

void ScrollBar::update()
{
    if( handle() != NULLHANDLE )
    {
        WinSendMsg( handle(), SBM_SETTHUMBSIZE, 
                    MPFROM2SHORT(pos2_-pos1_,high_-low_), MPARAM(0) );
        WinSendMsg( handle(), SBM_SETSCROLLBAR, 
                    MPARAM(pos1_), MPFROM2SHORT(low_,high_-(pos2_-pos1_)) );
    }
}

void ScrollBar::set_minmax( int low, int high )
{
    low_ = low;
    high_ = high;
    update();
}

void ScrollBar::set_position( int pos )
{
    pos2_ = pos + (pos2_-pos1_);
    pos1_ = pos;
    update();
}

void ScrollBar::set_position( int new_pos1, int new_pos2 )
{
    pos1_ = new_pos1;
    pos2_ = new_pos2;
    update();
}

int ScrollBar::pos1()
{
    if( handle() == NULLHANDLE )
        return 0;
    int p = int( WinSendMsg( handle(), SBM_QUERYPOS, MPARAM(NULL), MPARAM(NULL) ) );
    if( p != pos1_ )
        set_position( p, p + (pos2_-pos1_) );
    return p;
}

int ScrollBar::pos2()
{
    return pos1() - pos1_ + pos2_;
}

int ScrollBar::range()
{
    return pos2_ - pos1_;
}

bool ScrollBar::enabled()
{
    return ( pos1_ > low_ ) || ( pos2_ < high_ );
}

HorizScrollBar::HorizScrollBar( int id ): ScrollBar(id) {}
int HorizScrollBar::window_style() { return SBS_HORZ; }

VertScrollBar::VertScrollBar( int id ): ScrollBar(id) {}
int VertScrollBar::window_style() { return SBS_VERT; }

//////////////////////////////////////////////////////////////////////

PushButton::PushButton( int id, const char* text )
    :id_(id)
{
    text_ = new char[strlen(text)+1];
    strcpy( text_, text );
}

PushButton::~PushButton()
{
    delete[] text_;
}

void PushButton::setup_window( Window* parent )
{
    create_window( parent->handle(), WC_BUTTON, 
                   BS_PUSHBUTTON
                   // | BS_USEBUTTON
                   |BS_NOPOINTERFOCUS|BS_NOCURSORSELECT
                   , 
                   id_, parent->handle() );
    window_text( text_ );
}

//////////////////////////////////////////////////////////////////////

Label::Label( const char* text )
    :alignment_(DT_CENTER|DT_VCENTER)
{
    text_ = new char[strlen(text)+1];
    strcpy( text_, text );
}

Label::Label( const char* text, int alignment )
    :alignment_(alignment)
{
    text_ = new char[strlen(text)+1];
    strcpy( text_, text );
}

Label::~Label()
{
    delete[] text_;
}

void Label::setup_window( Window* parent )
{
    create_window( parent->handle(), WC_STATIC, 
                   SS_TEXT|alignment_, -1, parent->handle() );
    window_text( text_ );
}

//////////////////////////////////////////////////////////////////////

void ErrorBox( const char* text1, const char* text2 )
{
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, 
                   text2, text1, 0, MB_OK | MB_NOICON );
}
