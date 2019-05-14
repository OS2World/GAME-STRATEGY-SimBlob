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

#ifndef Layer_h
#define Layer_h

// The layer class hierarchy is somewhere between the Figment libraries
// and the Glyph libraries.  Layers are simple - not resizable, they
// take up the entire pixel buffer.  They could be considered containers
// of a glyph hierarchy, but glyphs can also contain glyphs so there may
// not be a need for layers anymore.  However, I have not removed them
// from the program.
class Layer
{
  public:
    Layer();
    virtual ~Layer();

    virtual void mark_damaged( const Rect& view_area, DamageArea& damaged );
    virtual void draw_rect( PixelBuffer& buffer, const Point& origin, 
                            const Rect& damaged );
    virtual void draw( PixelBuffer& buffer, const Point& origin, 
                       DamageArea& damaged );
	// virtual HitResult hit( Point p ) { return PickType(false); }
};

// BackgroundLayer is just the layer that goes underneath other layers.
// The implementation has several combinations -- flat/bitmap and
// plain/bevelled -- but these combinations are controlled with #ifdefs
// and not in the interface.
class BackgroundLayer: public Layer
{
    byte color;
  public:
    BackgroundLayer( byte c ): color(c) {}
    ~BackgroundLayer() {}

    virtual void draw_rect( PixelBuffer& buffer, const Point& origin, 
                            const Rect& damaged );
	// virtual PickType hit( Point p ) { return PickType(true); }
};

#endif
