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
#include "Glyph.h"

//////////////////////////////////////////////////////////////////////
Glyph::Glyph()
    :damaged_(true), area_()
{
}

Glyph::~Glyph()
{
}

void Glyph::request( Size& c, Size& s )
{
    // Empty glyph
    c.cx = 0;
    c.cy = 0;
    s.cx = 10000;
    s.cy = 10000;
}

void Glyph::allocate( const Rect& new_area )
{
    Rect old_area = area_;
    area_ = new_area;
    if( new_area.width() != old_area.width()
        || new_area.height() != old_area.height() )
        update_size();
    set_damaged();
}

void Glyph::update_size()
{
    // Subclasses will override this if they need to handle resizes
}

void Glyph::draw( PixelBuffer&, Point, const Rect& )
{
    // Subclasses will override this if they are visible
}

void Glyph::draw_region( PixelBuffer& pb, Point origin, DamageArea& damaged )
{
    for( int rect_i = 0; rect_i < damaged.num_rects(); rect_i++ )
    {
        // Draw everything inside damage_rect
        Rect damage_rect = damaged.rect(rect_i);
        pb.clip_to( damage_rect-origin );
        
        draw( pb, origin, damage_rect );
        pb.no_clip();
    }
}

void Glyph::mark_damaged( const Rect& area, DamageArea& damaged )
{
    if( damaged_ )
    {
        if( area_.width() > 0 && area_.height() > 0 )
            damaged += area_;
        damaged_ = false;
    }
}

Glyph* Glyph::pick( Point )
{
    return NULL;
}

