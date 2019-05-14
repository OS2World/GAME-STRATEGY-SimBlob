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
#include "Layer.h"

//////////////////////////////////////////////////////////////////////
Layer::Layer()
{
}

Layer::~Layer()
{
}

void Layer::mark_damaged( const Rect&, DamageArea& )
{
}

void Layer::draw_rect( PixelBuffer&, const Point&, const Rect& )
{
}

void Layer::draw( PixelBuffer& buffer, const Point& origin, 
                  DamageArea& damaged )
{
    for( int rect_i = 0; rect_i < damaged.num_rects(); rect_i++ )
    {
        // Draw everything inside damage_rect
        Rect damage_rect = damaged.rect(rect_i);
        Rect clipping_rect = damage_rect;
        clipping_rect.xLeft -= origin.x;
        clipping_rect.xRight -= origin.x;
        clipping_rect.yBottom -= origin.y;
        clipping_rect.yTop -= origin.y;
        buffer.clip_to( clipping_rect );
        
        draw_rect( buffer, origin, damage_rect );
        buffer.no_clip();
    }
}

//////////////////////////////////////////////////////////////////////
void BackgroundLayer::draw_rect( PixelBuffer& B, const Point&, const Rect& )
{
    const Rect& C = B.clipping_area;
    for( int y = C.yBottom; y < C.yTop; y++ )
    {
        byte* buf = B.row( y );
        for( int x = C.xLeft; x < C.xRight; x++ )
            buf[x] = color;
    }

#if 1
    // This version will paint the glyph with a sprite
    extern Sprite background_rle;
    int width = background_rle.image->width;
    int height = background_rle.image->height;
    for( int x = C.xLeft/width; x < (C.xRight+width-1)/width; x++ )
        for( int y = C.yBottom/height; y < (C.yTop+height-1)/height; y++ )
            B.DrawSprite( background_rle, x*width, y*height );
#endif

#if 0
    // This code will put a rounded bevel around the area... but
    // it really should go into a separate, reusable glyph
    const int topcol = 0xFF;
    const int botcol = 0x7F;
    for( int y = C.yBottom; y < C.yTop; y++ )
    {
        byte* buf = B.row( y );
        if( y == 1 )
        {
            for( int x = C.xLeft; x < C.xRight; x++ )
                buf[x] = TRANSPARENCY1(topcol,buf[x]);
        }
        else if( y == B.height-2 )
        {
            for( int x = C.xLeft; x < C.xRight; x++ )
                buf[x] = TRANSPARENCY1(botcol,buf[x]);
        }
        else if( y == 0 )
        {
            for( int x = C.xLeft; x < C.xRight; x++ )
                buf[x] = TRANSPARENCY1(TRANSPARENCY1(topcol,buf[x]),buf[x]);
        }
        else if( y == B.height-1 )
        {
            for( int x = C.xLeft; x < C.xRight; x++ )
                buf[x] = TRANSPARENCY1(TRANSPARENCY1(botcol,buf[x]),buf[x]);
        }

        int R = B.width-1;
        if( C.xLeft <= 1 && 1 < C.xRight )
            buf[1] = TRANSPARENCY1(botcol,buf[1]);
        if( C.xLeft <= R-1 && R-1 < C.xRight )
            buf[R-1] = TRANSPARENCY1(topcol,buf[R-1]);
        if( C.xLeft <= 0 && 0 < C.xRight )
            buf[0] = TRANSPARENCY1(TRANSPARENCY1(botcol,buf[0]),buf[0]);
        if( C.xLeft <= R && R < C.xRight )
            buf[R] = TRANSPARENCY1(TRANSPARENCY1(topcol,buf[R]),buf[R]);
    }
#endif
}

//////////////////////////////////////////////////////////////////////
