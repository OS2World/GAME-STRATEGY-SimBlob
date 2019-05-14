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

#ifndef Sprites_h
#define Sprites_h

const byte RLE_MASK = 253;

// An RLE has rows[], each an index into the data[] array.  Each row
// is a sequence of (begin, end, byte, byte, byte, ...) subsequences.  To
// draw the RLE, you get the begin and end values, and you read end-begin
// bytes in the array as pixel values.  (Note:  like STL and Python, I
// am using half-open intervals.  Begin *IS* in the range and End is *NOT*
// in the range.  The loop should be for(x=begin; x<end; x++).  )
struct RLE
{
    byte* data;
    short* rows;
    unsigned short width, height;
    unsigned short size;

    RLE();
    ~RLE();
};

// These sprites support transparency and use a fairly compact format
// (RLE) that allows for efficient blitting
struct Sprite
{
    static Subject<bool> ShowTransparent;
    
    struct
    {
        int x;
        int y;
    } hotspot;

    RLE* image;                 // Image
    RLE* trans1;                // Transparent parts (33%, 67%)
    RLE* trans2;

    Sprite();
    ~Sprite();

    // Picking
    bool pick( Point origin, Point p );
    bool pick_transparent( Point origin, Point p );
};

struct PixelBuffer
{
    byte* data;
    int width;
    int height;
    int linewidth;

    PixelBuffer( int w, int h );
    ~PixelBuffer();

    byte* row( int y ) { return data+y*linewidth; }
    byte pixel( int x, int y ) { return data[x+y*linewidth]; }

    // Clipping
    Rect clipping_area;
    void clip_to( const Rect& r );
    void no_clip();

    // Blitting
    void DrawSprite( Sprite& sprite, int x, int y );
    void DrawTransparentSprite( Sprite& sprite, int x, int y );
    void DrawTransparent2Sprite( Sprite& sprite, int x, int y );
    void DrawColoredSprite( Sprite& sprite, int x, int y, byte c );
    void DrawColoredTransparentSprite( Sprite& sprite, int x, int y, byte c );
    void DrawColoredTransparent2Sprite( Sprite& sprite, int x, int y, byte c );

    // Rectangles
    void DrawRect( Rect r, byte c );
    void DrawTransparentRect( Rect r, byte c );
    void DrawTransparent2Rect( Rect r, byte c );
    void DrawTransparent3Rect( Rect r, byte c );
};

struct Font
{
    const char* name;
    Sprite* data;         // First 128 are normal; second 128 are bold

  public:
    enum Style { Normal, Shadow, LightBG, MediumBG, HeavyBG };

    Font( const char* n ): name(n) { data = new Sprite[256]; }
    ~Font() { delete[] data; }

    int draw( PixelBuffer& buffer, int x, int y, byte fg, byte bg, Style sty );
    int width( const char* str, Style sty );
    int height( const char* str, Style sty );
};

// Conversions
void PSToBuffer( PS& ps, Color rgbmask, int mask, PixelBuffer& buffer, 
                 int x0 = 0, int y0 = 0 );
class Bitmap;
void BitmapToBuffer( Bitmap& bmp, Color rgbmask,
                     int mask, PixelBuffer& buffer );

// Creation
RLE* MakeRLE( PixelBuffer& buffer, int mask );

// Picking
//    bx,by = beginning point for the sprite
//    px,py = pick point
bool PickRLE( RLE* rle, int bx, int by, int px, int py );

// I/O
void SaveRLE( RLE* rle, const char* filename );
RLE* LoadRLE( const char* filename );
#if defined(__STDIO_H) || defined(__stdio_h) || defined(_STDIO_H)
void SaveRLE( RLE* rle, FILE* f );
RLE* LoadRLE( FILE* f );
#endif

// Standard OS/2 Palette
extern unsigned long* DesktopRGB; // [256];
// Transparency table:  first color is weaker
#define TRANSPARENCY1(i,j) Transparency1[((i)<<8)|(j)]
extern byte* Transparency1; // [65536];
// Gray Table
extern byte GrayColors[32];

#endif
