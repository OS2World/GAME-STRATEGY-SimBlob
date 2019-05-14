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

#ifndef Glyph_h
#define Glyph_h

enum MouseType { MouseMove, MouseSelect, MouseContextMenu,
                 MouseClickBegin, MouseClickEnd };
struct MouseInfo
{
    MouseType type;
    unsigned short kbd_flags;
};

// The Glyph is the base class for a hierarchy of drawable objects,
// all of which are independent of OS/2 and instead draw to a pixel
// array (PixelBuffer).
class Glyph
{
  protected:
    // This function is called whenever the size of the glyph changes
    virtual void update_size();

    // The area of the pixelbuffer which the glyph is assigned
    Rect area_;

    // Is this glyph damaged?  (i.e., does it need to be redrawn?)
    bool damaged_;

  public:
    Glyph();
    virtual ~Glyph();

    ////////// Basic functionality

    // Ask the glyph how much space it wants (c) and how much it can take (s)
    virtual void request( Size& c, Size& s );

    // Tell the glyph what area it actually got
    virtual void allocate( const Rect& area );

    // Tell the glyph to draw itself, clipping to `area'
    virtual void draw( PixelBuffer& pb, Point origin, const Rect& area );

    // Tell the glyph to draw itself, clipping to the damage area
    virtual void draw_region( PixelBuffer& pb, Point origin, DamageArea& );

    // Ask the glyph what areas it wants redrawn, it should add to `damaged'
    virtual void mark_damaged( const Rect& area, DamageArea& damaged );

    // Ask the glyph whether it can handle a mouse click at point p
    virtual Glyph* pick( Point p );

    ///////// Mouse events

    // The mouse pointer entered this glyph
    virtual void mouse_enter() {}

    // The mouse pointer left this glyph
    virtual void mouse_leave() {}

    // The mouse pointer moved inside the glyph
    virtual void mouse_move( Point ) {}

    // The mouse 'select' button was clicked (usually left button)
    virtual void mouse_select( Point ) {}

    // The mouse 'context menu' button was clicked (usually right button)
    virtual void mouse_contextmenu( Point ) {}

    // The mouse 'open' action (usually left double click)
    virtual void mouse_open( Point ) {}

    // The left button went UP
    virtual void mouse_upL( Point ) {}

    // The left button went DOWN
    virtual void mouse_downL( Point ) {}

    ////////// Information about the glyph, inlined & nonvirtual

    // Tell the glyph that it needs to be redrawn
    void set_damaged() { damaged_ = true; }

    // Ask the glyph what area it covers
    Rect area() const { return area_; }
};

#endif
