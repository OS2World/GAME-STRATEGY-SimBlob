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

#ifndef View_h
#define View_h

#include "Bitmaps.h"
#include "Sprites.h"

class Unit;
struct HitResult
{
    enum HitType { tNone, tUnknown, tUnit, tHexagon } type;
    Unit* unit;
    HexCoord location;

    HitResult(): type(tNone), unit(NULL) {}
    HitResult( bool flag ): type(flag?tUnknown:tNone), unit(NULL) {}
    HitResult( Unit* u ): type(tUnit), unit(u), location(u->hexloc()) {}
    HitResult( HexCoord h ): type(tHexagon), unit(NULL), location(h) {}
};

class View
{
    Map* map;
    
  public:
    Mutex view_mutex;
    long last_update_;
    int xoffset, yoffset;

    Point highlight;
    
    View( Map* m ): map(m), last_update_(0), 
        xoffset(0), yoffset(0) {}

    static void rect_to_hexarea( const Rect& r, int& left, int& bottom,
                                 int& right, int& top )
    {
        // Warning!  No range checking!
        left = r.xLeft / HexXSpacing - 1;
        right = r.xRight / HexXSpacing + 2;
        bottom = r.yBottom / HexYSpacing - 1;
        top = r.yTop / HexYSpacing + 2;     
    }
    
    void mark_damaged();
    void update( const Rect& view_area, DamageArea& damaged, 
                 PixelBuffer* pb, HexCoord cursor_loc );
    HitResult hit( Point p );
    Rect view_area( const Rect& client_area );  // offset by xoffset,yoffset
    Rect client_area( const Rect& view_area ); // offset the other way
    bool visible( const HexCoord& h, const Rect& view_area );
};

inline bool View::visible( const HexCoord& h, const Rect& view_area )
{
    return( h.right() >= view_area.xLeft && h.left() < view_area.xRight &&
            h.top() >= view_area.yBottom && h.bottom() < view_area.yTop );
}

#endif

