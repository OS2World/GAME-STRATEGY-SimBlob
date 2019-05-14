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
#include "Blitter.h"

#define DEBUG_DIVE 0

#if DEBUG_DIVE
#define D(MSG) Log("Dive",MSG)
#else
#define D(MSG)
#endif

//////////////////////////////////////////////////////////////////////

Blitter::Blitter()
    :pb(NULL)
{
}

Blitter::~Blitter()
{
}

int Blitter::draw_all( PS& ps, int x, int y )
{
    if( pb != NULL )
        return draw( ps, x, y, Rect(0,0,pb->width,pb->height) );
    else
        return 0;
}

int Blitter::draw_rect( PS&, int /* x */, int /* y */, const Rect& )
{
    return 0;
}

int Blitter::draw_rgn( PS& ps, int x, int y, const DamageArea& area )
{
    int total = 0;
    for( int i = 0; i < area.num_rects(); i++ )
        total += draw( ps, x, y, area.rect(i) );
    return total;
}

void Blitter::palette_changed( ColorPalette* )
{
}

void Blitter::vrgn_disable()
{
}

void Blitter::vrgn_enable( HWND, HWND, const Region& )
{
}

void Blitter::acquire( PixelBuffer* buffer )
{
    if( pb != NULL )
        release();
    pb = buffer;
}

void Blitter::release()
{
    pb = NULL;
}

//////////////////////////////////////////////////////////////////////
class BlitPM: public Blitter
{
  protected:
    byte* bitmapinfo;
    void setup_info();
    void reset_info();

  public:
    BlitPM();
    ~BlitPM();

    virtual void acquire( PixelBuffer* buffer );
};

BlitPM::BlitPM()
    :bitmapinfo(NULL) 
{
}

BlitPM::~BlitPM()
{
    reset_info();
}

void BlitPM::acquire( PixelBuffer* buffer )
{
    Blitter::acquire( buffer );
    setup_info();
}

void BlitPM::setup_info()
{
    if( pb != NULL )
    {
        if( bitmapinfo == NULL )
        {
            bitmapinfo = new byte[16+256*sizeof(RGB2)];
            BITMAPINFO2* bmp = reinterpret_cast<BITMAPINFO2*>(bitmapinfo);
            bmp->cbFix = 16;
            bmp->cx = pb->width;
            bmp->cy = pb->height;
            bmp->cPlanes = 1;
            bmp->cBitCount = 8;
            RGB2* rgbtable = reinterpret_cast<RGB2*>(bitmapinfo+16);
            memcpy( rgbtable, DesktopRGB, sizeof(RGB2)*256 );
        }
        else
        {
            BITMAPINFO2* bmp = (BITMAPINFO2*)(bitmapinfo);
            bmp->cx = pb->width;
            bmp->cy = pb->height;
        }
    }
    else
        Log("BlitPM::setup_info()","pb==NULL");
}

void BlitPM::reset_info()
{
    if( bitmapinfo != NULL )
    {
        delete[] bitmapinfo;
        bitmapinfo = NULL;
    }
}

//////////////////////////////////////////////////////////////////////
class BlitGpi: public BlitPM
{
  protected:
    virtual int draw_rect( PS& ps, int x, int y, const Rect& rc );

  public:
    BlitGpi();
    ~BlitGpi();
};

BlitGpi::BlitGpi()
{
}

BlitGpi::~BlitGpi()
{
}

int BlitGpi::draw_rect( PS& ps, int x, int y, const Rect& rc )
{
    if( !pb || !bitmapinfo ) return 0;

    // Target, Source point array
    // Target points are INCLUSIVE!
    // Source points are EXCLUSIVE!
    Point points[4] = { 
      Point(x+rc.xLeft,y+rc.yBottom), 
      Point(x+rc.xLeft+rc.width()-1,y+rc.yBottom+rc.height()-1),
      rc.bottom_left(), rc.top_right()
    };
    GpiDrawBits( ps, pb->data, 
                 reinterpret_cast<BITMAPINFO2*>(bitmapinfo),
                 4L, points, ROP_SRCCOPY, BBO_IGNORE );
    return (rc.width()*rc.height());
}

class BlitWin: public BlitPM
{
  protected:
    virtual int draw_rect( PS& ps, int x, int y, const Rect& rc );

  public:
    BlitWin();
    ~BlitWin();
};

BlitWin::BlitWin()
{
}

BlitWin::~BlitWin()
{
}

int BlitWin::draw_rect( PS& ps, int x, int y, const Rect& rc )
{
    if( !pb || !bitmapinfo ) return 0;
    if( !pb->data )
        Log("BlitWin::draw_rect()","??");

    HBITMAP bmp =
        GpiCreateBitmap( ps, 
                         reinterpret_cast<BITMAPINFOHEADER2*>(bitmapinfo),
                         CBM_INIT, pb->data,
                         reinterpret_cast<BITMAPINFO2*>(bitmapinfo) );
    if( bmp == NULLHANDLE )
        Log("BlitWin::draw_rect()","nullhandle");
    Rect r(
           Point(x+rc.xLeft,y+rc.yBottom), 
           Point(x+rc.xLeft+rc.width(),y+rc.yBottom+rc.height()) );
    ps.pixel( Point(0,0), 0 );
    WinDrawBitmap( ps, bmp, const_cast<Rect *>(&rc),
                   reinterpret_cast<POINTL*>(&r),
                   0, 0, DBM_NORMAL );
    GpiDeleteBitmap( bmp );
    return (rc.width()*rc.height());
}

//////////////////////////////////////////////////////////////////////

#define INCL_MM_OS2
#include <mmioos2.h>
#include <fourcc.h>
#include <dive.h>

class BlitDive: public Blitter
{
    HDIVE dive;
    unsigned long bufnum;
    bool ready;

  public:
    BlitDive();
    ~BlitDive();

    virtual void vrgn_enable( HWND client, HWND frame, const Region& rgn );
    virtual void vrgn_disable();
    virtual void palette_changed( ColorPalette* pal );

    virtual int draw_all( PS& ps, int x, int y );
    virtual int draw_rect( PS& ps, int x, int y, const Rect& );
    virtual int draw_rgn( PS& ps, int x, int y, const DamageArea& );

    virtual void acquire( PixelBuffer* buffer );
    virtual void release();

    static bool DiveExists();
};

bool BlitDive::DiveExists()
{
    DIVE_CAPS DiveCaps = {0};
    FOURCC fccFormats[100];
    
    DiveCaps.pFormatData    = fccFormats;
    DiveCaps.ulFormatLength = 100;
    DiveCaps.ulStructLen    = sizeof(DIVE_CAPS);
    
    if( DiveQueryCaps( &DiveCaps, DIVE_BUFFER_SCREEN ) != 0 )
        return false;

    if( DiveCaps.ulDepth < 8 )
        return false;
    
    return true;
}

BlitDive::BlitDive()
    :dive(NULLHANDLE), bufnum(0), ready(false)
{
    static bool Enabled = DiveExists();
    
    D("----------------------------------------");
    if( !Enabled || ( DiveOpen( &dive, false, 0 ) != DIVE_SUCCESS ) )
    {
        // We couldn't access DIVE
        dive = NULLHANDLE;
        if( Enabled )
            D("*** Couldn't open");
        else
            D("*** Not enabled");
    }
}

BlitDive::~BlitDive()
{
    release();

    if( dive != NULLHANDLE ) 
    {
        DiveClose( dive );
        dive = NULLHANDLE;
    }
}

static void IDENTIFY( const char* t, PixelBuffer* pb )
{
    D(t);
    if( pb != 0 )
    {
        if( pb->width > 200 && pb->height > 100 )
            D("  [Main Window]");
        else if( pb->width > 200 )
            D("  [StatusBar]");
        else if( pb->height > 128 )
            D("  [Info Area]");
        else
            D("  [WorldMap]");
    }
    else
        D("  [* Null pixelbuffer]");
}

bool old_dive = true;

int BlitDive::draw_all( PS&, int /* x */, int /* y */ )
{
    if( ready && dive != NULLHANDLE && bufnum != 0 )
    {
        DiveBlitImage( dive, bufnum, DIVE_BUFFER_SCREEN );
        return pb->width * pb->height;
    }
    else 
    {
        if( pb->width > 0 || pb->height > 0 )
        {
            IDENTIFY("BlitDive::draw_all()",pb);
            if( !ready )             D("*** not ready");
            if( dive == NULLHANDLE ) D("*** dive == NULLHANDLE");
            if( bufnum == 0 )        D("*** bufnum == 0");
        }
        return 0;
    }
}

int BlitDive::draw_rect( PS& ps, int x, int y, const Rect& rect )
{
    if( rect.width() > 0 && rect.height() > 0 )
    {
        if( old_dive )
            return draw_all( ps, x, y );
        
        if( ready && dive != NULLHANDLE && bufnum != 0 )
        {
            byte linemask[2048];
            int count = 0;
            for( int y = 0; y < pb->height; y++ )
            {
                bool do_draw = ( rect.yBottom <= y ) && ( y < rect.yTop );
                linemask[pb->height-y-1] = do_draw? 0xff : 0x00;
                count += do_draw;
            }
            DiveBlitImageLines( dive, bufnum, DIVE_BUFFER_SCREEN, linemask );
            return pb->width * count; 
        }
    }
    return 0;
}

int BlitDive::draw_rgn( PS& ps, int x, int y, const DamageArea& area )
{
    if( area.num_rects() > 0 )
    {
        if( old_dive )
            return draw_all( ps, x, y );

        if( ready && dive != NULLHANDLE && bufnum != 0 )
        {
            byte linemask[2048];
            for( int y = 0; y < pb->height; y++ )
                linemask[y] = 0x00;
            for( int i = 0; i < area.num_rects(); i++ )
            {
                const Rect& rect = area.rect(i);
                for( int y = rect.yBottom; y < rect.yTop; y++ )
                    linemask[pb->height-y-1] = 0xff;
            }

            int count = 0;
            for( y = 0; y < pb->height; y++ )
                if( linemask[y] )
                    count++;
            DiveBlitImageLines( dive, bufnum, DIVE_BUFFER_SCREEN, linemask );
            return pb->width * count;
        }
    }
    return 0;
}

void BlitDive::acquire( PixelBuffer* buffer )
{
    Blitter::acquire( buffer );

    IDENTIFY("acquire():",pb);

    if( bufnum != 0 )
        D("    bufnum != 0" );

    int rc = DiveAllocImageBuffer( dive, &bufnum, FOURCC_LUT8,
                                   buffer->width, buffer->height,
                                   buffer->linewidth, buffer->data );
    if( rc != DIVE_SUCCESS )
    {
        // We couldn't get the buffer
        bufnum = 0;
        if( rc == DIVE_ERR_TOO_MANY_INSTANCES )
            D("*** Too many instances");
        else if( rc == DIVE_ERR_ALLOCATION_ERROR )
            D("*** Allocation error");
        else if( rc == DIVE_ERR_INVALID_BUFFER_NUMBER )
            D("*** Invalid Buffer Number");
        else if( rc == DIVE_ERR_INVALID_LINESIZE )
            D("*** Invalid line size");
        D("*** Couldn't get buffer");
    }
    else
    {
        D("Got buffer");
        if( bufnum == 0 )
            D("*** but it is 0");
        DiveSetSourcePalette( bufnum, 0, 256, (char *)DesktopRGB );
        DiveSetDestinationPalette( dive, 0, 256, 0 );
    }
}

void BlitDive::release()
{
    IDENTIFY("release():",pb);

    if( pb != NULL )
    {
        D("Releasing");
        if( bufnum != 0 )
        {
            D("  Bufnum != 0");
            DiveFreeImageBuffer( dive, bufnum );
            bufnum = 0;
        }
        Blitter::release();
    }
}

void BlitDive::vrgn_enable( HWND client, HWND frame, const Region& rgn )
{
    // [copied from Michael Duffy's DIVE article]

    IDENTIFY("vrgn_enable():",pb);
    if( dive != NULLHANDLE && bufnum != 0 )
    {
        D("proceeding");

        // Now find the window position and size, relative to parent.
        SWP swp;
        WinQueryWindowPos( client, &swp );
    
        // Convert the point to offset from desktop lower left.
        Point pt( swp.x, swp.y );
        WinMapWindowPoints( frame, HWND_DESKTOP, &pt, 1 );

        Rect rects[50];
        int count = rgn.query_rects( 50, rects );

        // Tell DIVE about the new settings.
        SETUP_BLITTER sb;

        sb.ulStructLen = sizeof( SETUP_BLITTER );
        sb.fccSrcColorFormat = FOURCC_LUT8;
        sb.ulSrcWidth   = pb->width;
        sb.ulSrcHeight  = pb->height;
        sb.ulSrcPosX    = 0;
        sb.ulSrcPosY    = 0;
        sb.fInvert      = TRUE;
        sb.ulDitherType = 1;

        sb.fccDstColorFormat = FOURCC_SCRN;
        sb.ulDstWidth        = swp.cx;
        sb.ulDstHeight       = swp.cy;
        sb.lDstPosX          = 0;
        sb.lDstPosY          = 0;
        sb.lScreenPosX       = pt.x;
        sb.lScreenPosY       = pt.y;
        sb.ulNumDstRects     = count;
        sb.pVisDstRects      = rects;

        if( DiveSetupBlitter( dive, &sb ) != DIVE_SUCCESS )
            D("*** Setup Blitter failed");
        ready = true;
    }
    
    if( dive == NULLHANDLE )
        D("*** dive == NULLHANDLE");
    if( bufnum == 0 ) 
        D("*** bufnum == 0");
}

void BlitDive::vrgn_disable()
{
    IDENTIFY("vrgn_disable():",pb);
    DiveSetupBlitter( dive, 0 );
    ready = false;
}

void BlitDive::palette_changed( ColorPalette* pal )
{
    // CHANGE THIS??
    if( pal != NULL )
    {
        DiveSetDestinationPalette( dive, 0, 256, 0 );
    }
}

// DiveBlitImageLines -- takes a line mask that can be used to blit only
// dirty lines; array of bytes, where 0xff=changed, 0x00=unchanged

//////////////////////////////////////////////////////////////////////

template <class BlitPM>
class BlitHybrid: public Blitter
{
    BlitPM gpi_blitter;
    BlitDive dive_blitter;
  public:
    BlitHybrid() {}
    ~BlitHybrid() {}

    virtual void vrgn_enable( HWND client, HWND frame, const Region& rgn )
    {
        Blitter::vrgn_enable( client, frame, rgn );
        gpi_blitter.vrgn_enable( client, frame, rgn );
        dive_blitter.vrgn_enable( client, frame, rgn );
    }
    
    virtual void vrgn_disable()
    {
        Blitter::vrgn_disable();
        gpi_blitter.vrgn_disable();
        dive_blitter.vrgn_disable();
    }
        
    virtual void palette_changed( ColorPalette* pal )
    {
        Blitter::palette_changed( pal );
        gpi_blitter.palette_changed( pal );
        dive_blitter.palette_changed( pal );
    }
    
    virtual void acquire( PixelBuffer* buffer )
    {
        Blitter::acquire(buffer);
        gpi_blitter.acquire(buffer);
        dive_blitter.acquire(buffer);
    }

    virtual void release()
    {
        Blitter::release();
        gpi_blitter.release();
        dive_blitter.release();
    }
    
    int draw_rgn( PS& ps, int x, int y, const DamageArea& area );
};

template <class BlitPM>
int BlitHybrid<BlitPM>::draw_rgn( PS& ps, int x, int y,
                                  const DamageArea& area )
{
    // First, figure out how many pixels would be drawn using DIVE
    int dive_pixels = 0;
    int gpi_pixels = 0;
    if( area.num_rects() > 0 )
    {
        bool linemask[2048];
        for( int y = 0; y < pb->height; y++ )
            linemask[y] = false;
        for( int i = 0; i < area.num_rects(); i++ )
        {
            const Rect& rect = area.rect(i);
            for( int y = rect.yBottom; y < rect.yTop; y++ )
            {
                if( y < 0 ) Throw("NEGATIVE");
                linemask[y] = true;
            }
            gpi_pixels += rect.width() * rect.height();
        }
        for( y = 0; y < pb->height; y++ )
            if( linemask[y] )
                dive_pixels += pb->linewidth;
    }

    // Now see if it's better to use DIVE or to use Gpi
    int dive_time = dive_pixels + 1000;
    int gpi_time = gpi_pixels * 3;

    // Use whatever is faster
    if( dive_time < gpi_time )
        return dive_blitter.draw( ps, x, y, area );
    else
        return gpi_blitter.draw( ps, x, y, area );
}


//////////////////////////////////////////////////////////////////////
// Global variable for now
Blitter::BlitType Blitter::blit_controls_type = Blitter::UseGpi;
Blitter::BlitType Blitter::blit_map_type = Blitter::UseGpi;

static Blitter* create_blitter( Blitter::BlitType type )
{
    switch( type )
    {
      case Blitter::UseGpi: return new BlitGpi;
      case Blitter::UseWin: return new BlitWin;
      case Blitter::UseDive: return new BlitDive;
      case Blitter::UseGpiHybrid: return new BlitHybrid<BlitGpi>;
      case Blitter::UseWinHybrid: return new BlitHybrid<BlitWin>;
      default: return NULL;
    }
}

Blitter* Blitter::create_controls()
{
    return create_blitter( blit_controls_type );
}

Blitter* Blitter::create_map()
{
    return create_blitter( blit_map_type );
}
