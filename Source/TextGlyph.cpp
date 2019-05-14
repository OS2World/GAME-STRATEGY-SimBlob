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
#include "TextGlyph.h"
#include "Images.h"

// HACK!  Char widths shrunk by EXTRA at ############ points
int EXTRA = 5;

//////////////////////////////////////////////////////////////////////
LabelGlyph::LabelGlyph( string s )
    :style_(Font::Normal), width_(0), height_(0), label_(s)
{
    update_size();
}

LabelGlyph::LabelGlyph( string s, TextStyle sty )
    :style_(sty), width_(0), height_(0), label_(s)
{
    update_size();
}

LabelGlyph::~LabelGlyph()
{
}

string LabelGlyph::get_string() const
{
    return label_;
}

void LabelGlyph::update_size()
{
    int x = EXTRA;
    unsigned short y = 0;
    const string& s( get_string() );
    for( const char* c = s.c_str(); *c; c++ )
    {
        x += style_.font->data[*c].image->width - EXTRA; // ################
        y = max( y, style_.font->data[*c].image->height );
    }

    // Save the measurements in the object
    width_ = x;
    height_ = y;
}

void LabelGlyph::request( Size& c, Size& s )
{
    // Calculate the size of the string
    int x = EXTRA;
    unsigned short y = 0;
    string strtemp( get_string() ); 
    for( const char* ch = strtemp.c_str(); *ch; ch++ )
    {
        x += style_.font->data[*ch].image->width - EXTRA; // #################
        y = max( y, style_.font->data[*ch].image->height );
    }

    // Use those measurements to determine the glyph size
    c.cx = x;
    c.cy = y;
    if( style_.style == Font::Shadow && x > 0 )
    {
        c.cx += 2;
        c.cy += 2;      
    }
    s.cx = 0;
    s.cy = 0;   
}

void LabelGlyph::draw( PixelBuffer& buffer, Point origin, const Rect& area )
{
    int xL = area_.xLeft - origin.x;
    int xR = area_.xRight - origin.x - width_;
    int x = xL;
    if( style_.justify == TextStyle::Right ) x = xR;
    if( style_.justify == TextStyle::Center ) x = (xL+xR)/2;
    int y = area_.yBottom - origin.y + style_.font->data[32].hotspot.y;
    string s( get_string() );

    //////////////////////////////////////////////////////////////////////
    // ***************** HOW DO WE CLIP ?????  ***************
    Rect old_clip = buffer.clipping_area;
    buffer.clip_to( intersect( old_clip, area_-origin ) );
    Font& font = *(style_.font);

    for( const char* c = s.c_str(); *c; c++ )
    {
        if( style_.style == Font::Shadow )
            buffer.DrawColoredTransparentSprite( font.data[128|*c], 
                                                 x+1, y-1,
                                                 style_.shadow_color );
        buffer.DrawColoredSprite( font.data[*c],
                                  x, y, style_.text_color );
        x += font.data[*c].image->width - EXTRA; // #################
    }
    
    buffer.clip_to( old_clip );
}

Glyph* LabelGlyph::pick( Point p )
{
    int x = area_.xLeft, y = area_.yBottom;
    string s( get_string() );
    for( const char* c = s.c_str(); *c; c++ )
    {
        if( style_.style == Font::Shadow )
            if( style_.font->data[128|*c].pick( Point(x+1,y-1), p ) )
                return this;
        if( style_.font->data[*c].pick( Point(x,y), p ) )
            return this;
        x += style_.font->data[*c].image->width - EXTRA; // ##################
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////
TextGlyph::TextGlyph( Subject<string>& s )
    :LabelGlyph(""), str( s, closure( this, &TextGlyph::notify ) )
{
    update_size();
}

TextGlyph::TextGlyph( Subject<string>& s, TextStyle sty )
    :LabelGlyph("", sty), str( s, closure( this, &TextGlyph::notify ) )
{
    update_size();
}

TextGlyph::~TextGlyph()
{
}

string TextGlyph::get_string() const
{
    return str.subject();
}

bool TextGlyph::notify( const string& )
{
    update_size();
    set_damaged();
    return true;
}

//////////////////////////////////////////////////////////////////////
TextLayer::TextLayer()
{
}

TextLayer::~TextLayer()
{
}

void TextLayer::add( Glyph* g )
{
    Info I;
    I.glyph = g;
    info.push_back( I );
}

void TextLayer::remove( Glyph* g )
{
    Throw("OOPS");
}

typedef vector<TextLayer::Info>::iterator Iterator;
// #define Iterator vector<TextLayer::Info>::iterator

void TextLayer::mark_damaged( const Rect& view_area, DamageArea& damaged )
{
    for( Iterator i = info.begin(); i != info.end(); ++i )
        (*i).glyph->mark_damaged( view_area, damaged );
}

void TextLayer::draw( PixelBuffer& buffer, const Point& origin, 
                      DamageArea& damaged )
{
    /*
    Rect old_clip = buffer.clipping_area;
    for( Iterator i = info.begin(); i != info.end(); ++i )
    {
        TextInfo& I = *i;

        buffer.clip_to( intersect( old_clip, I.area-origin ) );
        I.glyph->draw_region( buffer, origin, I.area );
    }
    buffer.clip_to( old_clip );
    */

    for( Iterator i = info.begin(); i != info.end(); ++i )
        (*i).glyph->draw_region( buffer, origin, damaged );
}

static Glyph* picked = NULL;

bool TextLayer::pick( Point p, MouseInfo& mouse )
{
    // IGNORING THE ORIGIN
    bool some_glyph_contains_pointer = false;
    for( Iterator i = info.end(); i != info.begin(); --i )
    {
        // Go through the glyphs in reverse order
        Info& I = *(i-1);
        if( I.glyph->area().contains(p) )
        {
            Glyph* g = I.glyph->pick(p);
            some_glyph_contains_pointer = true;
            if( g != picked )
            {
                if( picked != NULL ) picked->mouse_leave();
                picked = g;
                if( picked != NULL ) picked->mouse_enter();
            }
            
            if( g != NULL )
            {
                if( mouse.type == MouseSelect )
                    g->mouse_select(p);
                else if( mouse.type == MouseContextMenu )
                    g->mouse_contextmenu(p);
                else if( mouse.type == MouseClickBegin )
                    g->mouse_downL(p);
                else if( mouse.type == MouseClickEnd )
                    g->mouse_upL(p);
                else
                    g->mouse_move(p);
            }
        }
    }

    if( picked != NULL && !some_glyph_contains_pointer )
    {
        picked->mouse_leave();
        picked = NULL;
    }

    return (picked != NULL);
}

