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
#include "Bitmaps.h"
#include "Sprites.h"

#include "initcode.h"

Subject<bool> Sprite::ShowTransparent( true );

RLE::RLE()
    :data(NULL), rows(NULL), width(0), height(0), size(0)
{
}

RLE::~RLE()
{
    if( rows ) delete[] rows;
    if( data ) delete[] data;
}

byte GrayColors[32] = {0x00,0x70,0x71,0x72,0x73,0x74,0x75,0x76,
                       0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,
                       0xF8,0x81,0x82,0x83,0xF7,0x85,0x86,0x87,
                       0x07,0x08,0x8A,0x8B,0x8C,0x8D,0x8E,0xFF};

Sprite::Sprite()
    :image(NULL), trans1(NULL), trans2(NULL)
{
    hotspot.x = 0;
    hotspot.y = 0;
}

Sprite::~Sprite()
{
    if( image ) delete image;
    if( trans1 ) delete trans1;
    if( trans2 ) delete trans2;
}

bool Sprite::pick( Point origin, Point p )
{
    Point q(origin);
    q.x -= hotspot.x;
    q.y -= hotspot.y;
    return pick_transparent( origin, p )
        || PickRLE( trans2, q.x, q.y, p.x, p.y );
}

bool Sprite::pick_transparent( Point origin, Point p )
{
    Point q(origin);
    q.x -= hotspot.x;
    q.y -= hotspot.y;
    return PickRLE( image, q.x, q.y, p.x, p.y );
}

PixelBuffer::PixelBuffer( int w, int h )
    :width(w), height(h), clipping_area( 0, 0, w, h )

{
    linewidth = (3+width)/4 * 4;
    data = new byte[linewidth*height];
}

PixelBuffer::~PixelBuffer()
{
    delete[] data;
}

void PixelBuffer::clip_to( const Rect& r )
{
    clipping_area = intersect( r, Rect( 0, 0, width, height ) );
}

void PixelBuffer::no_clip()
{
    clipping_area = Rect( 0, 0, width, height );
}

//////////////////////////////////////////////////////////////////////
// Conversions

Color BestColorMatch( long palette[], int r1, int g1, int b1 )
{
    // V is the brightness value, which we take into account when comparing
    // http://mars.adv-boeblingen.de/~steinke/howto/semitrans.html
    // suggests using the brightness value
    double v1 = 0.3*r1 + 0.59*g1 + 0.11*b1;
    int best = 0;
    double best_rank = 1e10;
    for( int i = 0; i < 256; i++ )
    {
        int r2 = ColorToRed( palette[i] );
        int g2 = ColorToGreen( palette[i] );
        int b2 = ColorToBlue( palette[i] );
        double v2 = 0.3*r2 + 0.59*g2 + 0.11*b2;
        
        double rank = (r2-r1)*(r2-r1) + (g2-g1)*(g2-g1) + 
            (b2-b1)*(b2-b1) + (v2-v1)*(v2-v1);

        if( rank < best_rank )
        {
            best = i;
            best_rank = rank;
        }
    }

    return best;
}

void PSToBuffer( PS& ps, Color rgbmask, int mask, PixelBuffer& buffer,
                 int x0, int y0 )
{
    ps.rgb_mode();
    CachedPS dps( HWND_DESKTOP );

    for( int y = 0; y < buffer.height; y++ )
    {
        for( int x = 0; x < buffer.width; x++ )
        {
            Color c = ps.pixel(Point(x0+x,y0+y));
            int color_index = mask;
            if( c != rgbmask )
            {
                color_index = BestColorMatch( (long *)DesktopRGB, 
                                              ColorToRed(c),
                                              ColorToGreen(c),
                                              ColorToBlue(c) );
            }
            buffer.data[buffer.linewidth*y+x] = color_index;
        }
    }
}

void BitmapToBuffer( Bitmap& bmp, Color rgbmask, 
                     int mask, PixelBuffer& buffer )
{
    PSToBuffer( bmp.ps(), rgbmask, mask, buffer );
}

//////////////////////////////////////////////////////////////////////
// Blitting

// I use templates here to combine various types of options.  I don't
// want to check them at run time but I do want to have all the options
// available to me, so I use templates to produce each version of the
// blitting function, which will be optimized separately by the compiler.

struct NoColor_Trans
{
    // No color translation
    static inline byte op( byte i ) { return i; }
};

struct OneColor_Trans
{
    // Translate all pixels into color 'c'
    static byte c;
    static inline byte op( byte ) { return c; }
};

byte OneColor_Trans::c = 0;

template<class Trans>
struct NoColor_Op
{
    // No transparency
    static inline byte op( byte i, byte j ) { return Trans::op(i); }
};

template<class Trans>
struct Transparent16_Op
{
    // 16% transparency
    static inline byte op( byte i, byte j )
    { return TRANSPARENCY1(TRANSPARENCY1(Trans::op(i),j),j); }
};

template<class Trans>
struct Transparent33_Op
{
    // 33% transparency
    static inline byte op( byte i, byte j ) { return TRANSPARENCY1
                                                  (Trans::op(i),j); }
};

template<class Trans>
struct Transparent66_Op
{
    // 66% transparency
    static inline byte op( byte i, byte j ) { return TRANSPARENCY1
                                                  (j,Trans::op(i)); }
};

struct Clip_Op
{
    // Perform clipping
    static inline void clipX( const Rect& clipping, int& xL, int& xR, int& p )
    {
        if( xL < clipping.xLeft )
        {
            p -= (xL-clipping.xLeft);
            xL = clipping.xLeft;
        }
        if( xR > clipping.xRight )
            xR = clipping.xRight;
    }
    
    static inline int clipY( const Rect& clipping, int by )
    {
        return ( by >= clipping.yBottom && by < clipping.yTop );
    }
};

struct NoClip_Op
{
    // Don't perform clipping---this version is used if we know the
    // entire sprite lies within the bounding rectangle.  Since that
    // check is made outside the loop, we get a nice tight inner
    // loop with no clipping checks.
    static inline void clipX( const Rect&, int&, int&, int&  )
    {
    }

    static inline int clipY( const Rect&, int )
    {
        return 1;
    }
};

#pragma option -O2
template <class Op, class Clip>
void DrawRLE_OpClip( RLE* rle, PixelBuffer& buffer, int bx, int by, 
                     Op*, Clip* )
{
    // This function draws an RLE and is parameterized over the
    // pixel translation function (Op) and the clipping tests (Clip)
    for( int y = 0; y < rle->height; y++, by++ )
    {
        if( Clip::clipY( buffer.clipping_area, by ) )
        {
            int p = rle->rows[y];
            int q = y+1==rle->height ? rle->size : rle->rows[y+1];

            byte* buf = buffer.data + by*buffer.linewidth;
            while( p < q )
            {
                int xL = bx + rle->data[p++];
                int xR = xL + rle->data[p++];
                int np = p + (xR-xL);
                Clip::clipX( buffer.clipping_area, xL, xR, p );
                for( int x = xL; x < xR; x++ )
                {
                    buf[x] = Op::op( rle->data[p++], buf[x] );
                }
                p = np;
            }
        }
    }
}

template <class Op, class Clip>
void DrawRect_OpClip( PixelBuffer& buffer, Rect area, Op*, Clip* )
{
    for( int y = area.yBottom; y < area.yTop; y++ )
    {
        if( Clip::clipY( buffer.clipping_area, y ) )
        {
            int xL = area.xLeft;
            int xR = area.xRight;
            int p = 0;          // dummy
            byte* buf = buffer.data + y*buffer.linewidth;
            Clip::clipX( buffer.clipping_area, xL, xR, p );
            for( int x = xL; x < xR; x++ )
                buf[x] = Op::op( 0, buf[x] );
        }
    }
}
#pragma option -O.

template <class Op>
void DrawRLE( RLE* rle, PixelBuffer& buffer, int bx, int by, Op* op )
{
    if( !rle ) return;

    const Rect& C = buffer.clipping_area;

    // Can we clip the entire image?
    if( by+rle->height <= C.yBottom || by >= C.yTop ||
        bx+rle->width <= C.xLeft || bx >= C.xRight )
        return;

    // Can the entire image be put inside the clipping area?
    if( by >= C.yBottom && by+rle->height <= C.yTop &&
        bx >= C.xLeft && bx+rle->width <= C.xRight )
    {
        // Yes, we don't need fine grained clipping
        DrawRLE_OpClip( rle, buffer, bx, by, op, (NoClip_Op*)0 );
    }
    else
    {
        // No, partial clipping may be needed
        DrawRLE_OpClip( rle, buffer, bx, by, op, (Clip_Op*)0 );
    }
}

template <class Op>
void DrawRect( PixelBuffer& buffer, Rect area, Op* op )
{
    const Rect& C = buffer.clipping_area;

    // Can we clip the entire image?
    if( !intersect( C, area ) )
        return;

    // Can the entire image be put inside the clipping area?
    if( C.contains( area ) )
    {
        // Yes, we don't need fine grained clipping
        DrawRect_OpClip( buffer, area, op, (NoClip_Op*)0 );
    }
    else
    {
        // No, partial clipping may be needed
        DrawRect_OpClip( buffer, area, op, (Clip_Op*)0 );
    }
}

template <class ColorTrans>
void draw_sprite( PixelBuffer& buffer, int x, int y, RLE* a, RLE* b, RLE* c,
                  ColorTrans* )
{
    DrawRLE( a, buffer, x, y, (NoColor_Op<ColorTrans>*)0 );
    if( a != NULL && !(bool(Sprite::ShowTransparent)) )
        return;
    DrawRLE( b, buffer, x, y, (Transparent66_Op<ColorTrans>*)0 );
    DrawRLE( c, buffer, x, y, (Transparent33_Op<ColorTrans>*)0 );
}

void PixelBuffer::DrawSprite( Sprite& sprite, int x, int y )
{
    x = x - sprite.hotspot.x;
    y = y - sprite.hotspot.y;
    draw_sprite( *this, x, y, sprite.image, sprite.trans2, sprite.trans1,
                 (NoColor_Trans*)0 );
}

void PixelBuffer::DrawTransparentSprite( Sprite& sprite, int x, int y )
{
    x = x - sprite.hotspot.x;
    y = y - sprite.hotspot.y;
    draw_sprite( *this, x, y, (RLE*)0, sprite.image, sprite.trans2,
                 (NoColor_Trans*)0 );
}

void PixelBuffer::DrawTransparent2Sprite( Sprite& sprite, int x, int y )
{
    x = x - sprite.hotspot.x;
    y = y - sprite.hotspot.y;
    draw_sprite( *this, x, y, (RLE*)0, (RLE*)0, sprite.image,
                 (NoColor_Trans*)0 );
}

void PixelBuffer::DrawColoredSprite( Sprite& sprite, int x, int y, byte c )
{
    x = x - sprite.hotspot.x;
    y = y - sprite.hotspot.y;
    // This really should have a mutex around it
    OneColor_Trans::c = c;
    draw_sprite( *this, x, y, sprite.image, sprite.trans2, sprite.trans1,
                 (OneColor_Trans*)0 );
}

void PixelBuffer::DrawColoredTransparentSprite( Sprite& sprite, 
                                                int x, int y, byte c )
{
    x = x - sprite.hotspot.x;
    y = y - sprite.hotspot.y;
    // This really should have a mutex around it
    OneColor_Trans::c = c;
    draw_sprite( *this, x, y, (RLE*)0, sprite.image, sprite.trans2, 
                 (OneColor_Trans*)0 );
}

void PixelBuffer::DrawColoredTransparent2Sprite( Sprite& sprite, 
                                                 int x, int y, byte c )
{
    x = x - sprite.hotspot.x;
    y = y - sprite.hotspot.y;
    // This really should have a mutex around it
    OneColor_Trans::c = c;
    draw_sprite( *this, x, y, (RLE*)0, (RLE*)0, sprite.image, 
                 (OneColor_Trans*)0 );
}

void PixelBuffer::DrawRect( Rect r, byte c )
{
    OneColor_Trans::c = c;
    ::DrawRect( *this, r, (NoColor_Op<OneColor_Trans>*)0 );
}

void PixelBuffer::DrawTransparentRect( Rect r, byte c )
{
    OneColor_Trans::c = c;
    ::DrawRect( *this, r, (Transparent66_Op<OneColor_Trans>*)0 );
}

void PixelBuffer::DrawTransparent2Rect( Rect r, byte c )
{
    OneColor_Trans::c = c;
    ::DrawRect( *this, r, (Transparent33_Op<OneColor_Trans>*)0 );
}

void PixelBuffer::DrawTransparent3Rect( Rect r, byte c )
{
    OneColor_Trans::c = c;
    ::DrawRect( *this, r, (Transparent16_Op<OneColor_Trans>*)0 );
}

bool PickRLE( RLE* rle, int bx, int by, int px, int py )
{
    // Some sprites have blank components
    if( rle == NULL )
        return false;
    
    // First check if it's even possible that (px,py) hits
    if( py < by || py >= by+rle->height ||
        px < bx || px >= bx+rle->width )
        return false;

    for( int y = 0; y < rle->height; y++, by++ )
        if( by == py )
        {
            int p = rle->rows[y];
            int q = y+1==rle->height ? rle->size : rle->rows[y+1];
            
            while( p < q )
            {
                int xL = bx + rle->data[p++];
                int xR = xL + rle->data[p++];
                int np = p + (xR-xL);
                if( xL <= px && px < xR )
                    return true;
                p = np;
            }
        }
    return false;
}

//////////////////////////////////////////////////////////////////////
// Creation

RLE* MakeRLE( PixelBuffer& buffer, int mask )
{
    RLE& rle = *(new RLE);
    rle.width = buffer.width;
    rle.height = buffer.height;
    rle.rows = new short[rle.height];

    byte* tmpbuf = new byte[buffer.width*buffer.height*3];
    int pos = 0;

    for( int y = 0; y < buffer.height; y++ )
    {
        rle.rows[y] = pos;
        for( int x = 0; x < buffer.width; x++ )
        {          
            if( buffer.pixel(x,y) != mask )
            {
                int xL = x;
                int xR = xL + 1;
                for( ; xR < buffer.width; xR++ )
                    if( buffer.pixel(xR,y) == mask )
                        break;
                // xL:xR is a half-open range of non-mask pixel indices
                tmpbuf[pos++] = xL;
                tmpbuf[pos++] = xR-xL;
                // This loop increments the main X loop position
                // It copies the pixels and it skips it for the main loop
                for( ; x < xR; x++ )
                    tmpbuf[pos++] = buffer.pixel(x,y);
                // x must be xR at this point, so we resume scanning here
            }
        }
    }
    rle.size = pos;
    rle.data = new byte[rle.size];
    memcpy( rle.data, tmpbuf, rle.size );

    return &rle;
}

//////////////////////////////////////////////////////////////////////
// I/O
//   Format:  { width,height,size,rows,data }

#if INIT_BITMAPS != 0
void SaveRLE( RLE* rle, FILE* f )
{
    if( !rle )
    {
        unsigned short zero = 0;
        fwrite( &zero, sizeof(zero), 1, f );
    }
    else
    {
        fwrite( &(rle->width), sizeof(rle->width), 1, f );
        fwrite( &(rle->height), sizeof(rle->height), 1, f );
        fwrite( &(rle->size), sizeof(rle->size), 1, f );
        fwrite( rle->rows, sizeof(rle->rows[0]), rle->height, f );
        fwrite( rle->data, sizeof(rle->data[0]), rle->size, f );
    }
}

void SaveRLE( RLE* rle, const char* filename )
{
    if( rle && rle->data )
    {
        FILE* f = fopen( filename, "wb" );
        SaveRLE( rle, f );
        fclose(f);
    }
}
#endif

#define Read(p,s,n,f) if( fread(p,s,n,f) < n ) Throw("Could not read RLE file");

RLE* LoadRLE( FILE* f )
{
    unsigned short width = 0;
    Read( &width, sizeof(width), 1, f );

    if( width != 0 )
    {
        RLE* rle = new RLE;
        rle->width = width;
        if( rle->width > 500 ) Throw("Invalid Width");
        Read( &(rle->height), sizeof(rle->height), 1, f );
        if( rle->height > 500 ) Throw("Invalid Height");
        Read( &(rle->size), sizeof(rle->size), 1, f );
        rle->rows = new short[rle->height];
        rle->data = new byte[rle->size];
        Read( rle->rows, sizeof(rle->rows[0]), rle->height, f );
        Read( rle->data, sizeof(rle->data[0]), rle->size, f );
        return rle;
    }
    else
        return NULL;
}

RLE* LoadRLE( const char* filename )
{
    FILE* f = fopen( filename, "rb" );
    RLE* rle = LoadRLE( f );
    fclose(f);

    return rle;
}

