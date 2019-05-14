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

#include "Bitmaps.h"
#include "Sprites.h"
#include "Map.h"
#include "View.h"
#include "Menu.h"

#include "Layout.h"
#include "BufferWin.h"
#include "GlyphLib.h"

NeatBackground::NeatBackground( Subject<bool>* r, Subject<bool>* h )
    :reverse(r), highlight(h) 
{
    Closure<bool,const bool&> 
        cl( closure(this,&NeatBackground::notify_update) );
    if(r) r->add_dependent( cl );
    if(h) h->add_dependent( cl );
}

NeatBackground::~NeatBackground()
{
    Closure<bool,const bool&> 
        cl( closure(this,&NeatBackground::notify_update) );
    if(reverse) reverse->remove_dependent( cl );
    if(highlight) highlight->remove_dependent( cl );
}

void NeatBackground::draw( PixelBuffer& pb, Point, const Rect& clip_area )
{
    int c0 = 18;
    int c1 = 28;
    if( highlight && bool(*highlight) ) { c0 = 8; c1 = 31; }
    double m = 0.3;
    double t0 = m*area_.width();
    double t1 = t0 + area_.height();
    Rect A = intersect(area_,clip_area);
    for( int y = A.yBottom; y < A.yTop; y++ )
    {
        byte* row = pb.row(y);
        for( int x = A.xLeft; x < A.xRight; x++ )
        {
            bool light_edge = (x<area_.xLeft+2) || (y>=area_.yTop-2);
            bool dark_edge = (y<area_.yBottom+2) || (x>=area_.xRight-2);
            double t = ( ((y-area_.yBottom)-m*(x-area_.xLeft))+t0 ) / t1;
            if( x%3==0 ) t += 0.02;
            if( y%3==0 ) t -= 0.02;
            int c = int( c0 + (c1-c0)*t );
            if( reverse && bool(*reverse) && ( light_edge || dark_edge ) ) 
            {
                c = int( 4 + (31-4)*(1-t) );

                // HACK for a solid border
                c = 0xA0;
            }
            else if( light_edge ) c = min(c+4,31);
            else if( dark_edge ) c = max(c-4,0);
            row[x] = GrayColors[c&31];

            // HACK for a solid border
            if( c == 0xA0 ) row[x] = c;
        }
    }
}

//////////////////////////////////////////////////////////////////////

void BitmapGlyph::draw( PixelBuffer& pb, Point, const Rect& /* clip_area */ )
{
    int x = (area().xLeft+area().xRight-sprite.image->width)/2;
    int y = (area().yBottom+area().yTop-sprite.image->height)/2;
    pb.DrawSprite( sprite, x+sprite.hotspot.x, y+sprite.hotspot.y );
}

//////////////////////////////////////////////////////////////////////

RadioButtonGlyph::RadioButtonGlyph( Subject<int>& c, int v, BufferWindow* win,
                                    Subject<bool>& h, Subject<bool>& d )
    :value(v), highlight(h), down(d), window(win),
     control( c, closure(this,&RadioButtonGlyph::set_control) )
{
    set_control( control.subject() );
}

//////////////////////////////////////////////////////////////////////

NotebookTab::NotebookTab( Subject<bool>* r )
    :reverse(r) 
{
    Closure<bool,const bool&> 
        cl( closure(this,&NotebookTab::notify_update) );
    if(r) r->add_dependent( cl );
}

NotebookTab::~NotebookTab()
{
    Closure<bool,const bool&> 
        cl( closure(this,&NotebookTab::notify_update) );
    if(reverse) reverse->remove_dependent( cl );
}

void NotebookTab::draw( PixelBuffer& pb, Point, const Rect& clip_area )
{
    // Draw a border around the tab, with very faint lines inside
    // and stronger lines outside
    Rect
        top1( area_.xLeft+2, area_.yTop-2, area_.xRight-2, area_.yTop-1 ),
        top2( area_.xLeft+1, area_.yTop-1, area_.xRight-1, area_.yTop ),
        left1( area_.xLeft, area_.yBottom, area_.xLeft+1, area_.yTop-1 ),
        left2( area_.xLeft+1, area_.yBottom+1, area_.xLeft+2, area_.yTop-1 ),
        right1( area_.xRight-1, area_.yBottom, area_.xRight, area_.yTop-1 ),
        right2( area_.xRight-2, area_.yBottom+1, area_.xRight-1, area_.yTop-1 );
    
    Rect bott1( area_.xLeft, area_.yBottom+1, area_.xRight, area_.yBottom+2 );
    Rect bott2( area_.xLeft, area_.yBottom, area_.xRight, area_.yBottom+1 );
    
    pb.DrawTransparentRect( intersect(left1, clip_area), 0xff );
    pb.DrawTransparent2Rect( intersect(left2, clip_area), 0xff );
    pb.DrawTransparent2Rect( intersect(top1, clip_area), 0xff );
    pb.DrawTransparentRect( intersect(top2, clip_area), 0xff );
    pb.DrawTransparentRect( intersect(right1, clip_area), 0x7f );
    pb.DrawTransparent2Rect( intersect(right2, clip_area), 0x7f );
    if( !reverse || !bool(*reverse) )
    {
        pb.DrawTransparentRect( intersect(bott1, clip_area), 0xff );
        pb.DrawTransparent2Rect( intersect(bott2, clip_area), 0xff );
    }
}

//////////////////////////////////////////////////////////////////////

SolidArea::SolidArea( byte color_index, int transparency_type,
                      Subject<bool>* h = NULL )
    :color_index_(color_index),
     transparency_type_(transparency_type),
     highlight(h) 
{
    Closure<bool,const bool&> 
        cl( closure(this,&SolidArea::notify_update) );
    if(h) h->add_dependent( cl );
}

SolidArea::~SolidArea()
{
    Closure<bool,const bool&> 
        cl( closure(this,&SolidArea::notify_update) );
    if(highlight) highlight->remove_dependent( cl );
}

void SolidArea::draw( PixelBuffer& pb, Point, const Rect& clip_area )
{
    if( highlight && !bool(*highlight) )
        return;
    
    Rect A = intersect( area_, clip_area );
    if( transparency_type_ == 3 )
        pb.DrawTransparent3Rect( A, color_index_ );
    else if( transparency_type_ == 2 )
        pb.DrawTransparent2Rect( A, color_index_ );
    else if( transparency_type_ == 1 )
        pb.DrawTransparentRect( A, color_index_ );
    else
        pb.DrawRect( A, color_index_ );
}

//////////////////////////////////////////////////////////////////////

void DividerGlyph::draw( PixelBuffer& B, Point origin, const Rect& area )
{
    const Rect& C = B.clipping_area;
    const int topcol = 0xFF;
    const int botcol = 0x7F;
    for( int y = C.yBottom; y < C.yTop; y++ )
    {
        byte* buf = B.row( y );
        int L = area_.xLeft+1;
        int R = area_.xRight-2;
        if( y == area_.yBottom || y == area_.yBottom+1 ||
            y == area_.yTop-1 || y == area_.yTop-2 )
            for( int x = L; x <= R; x++ )
                if( C.xLeft <= x && x <= C.xRight )
                {
                    byte V = buf[x];
                    if( y == area_.yBottom )
                        buf[x] = TRANSPARENCY1(topcol,V);
                    else if( y == area_.yBottom+1 )
                        buf[x] = TRANSPARENCY1(TRANSPARENCY1(topcol,V),V);
                    else if( y == area_.yTop-1 )
                        buf[x] = TRANSPARENCY1(botcol,V);
                    else
                        buf[x] = TRANSPARENCY1(TRANSPARENCY1(botcol,V),V);
                }

        if( y >= area_.yBottom && y < area_.yTop )
        {
            if( C.xLeft <= L && L < C.xRight )
                buf[L] = TRANSPARENCY1(botcol,buf[L]);
            if( C.xLeft <= R && R < C.xRight )
                buf[R] = TRANSPARENCY1(topcol,buf[R]);
            if( C.xLeft <= L-1 && L-1 < C.xRight )
                buf[L-1] = TRANSPARENCY1(TRANSPARENCY1(botcol,buf[L-1]),buf[L-1]);
            if( C.xLeft <= R+1 && R+1 < C.xRight )
                buf[R+1] = TRANSPARENCY1(TRANSPARENCY1(topcol,buf[R+1]),buf[R+1]);
        }
    }
}

