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

#ifndef TextGlyph_h
#define TextGlyph_h

#include <cstring.h>
#include "Subject.h"
#include "Glyph.h"

extern Font text12;
class TextStyle
{
  public:
    enum Justification { Left, Center, Right };
    TextStyle( Justification j, Font::Style s = Font::Normal,
               Font* f = &text12 )
        :justify(j), style(s), font(f),
         text_color(0x00), shadow_color(0xff) {}
    TextStyle( Font::Style s = Font::Normal, Font* f = &text12 )
        :justify(Left), style(s), font(f),
         text_color(0x00), shadow_color(0xff) {}

    Justification justify;
    Font::Style style;
    Font* font;
    byte text_color, shadow_color;
};

class LabelGlyph: public Glyph
{
  protected:
    TextStyle style_;
    string label_;
    
    int width_, height_;
    virtual void update_size();

  public:
    LabelGlyph( string s );
    LabelGlyph( string s, TextStyle sty );
    ~LabelGlyph();

    virtual void request( Size& c, Size& s );
    virtual void draw( PixelBuffer& buffer, Point origin, const Rect& area );
    virtual Glyph* pick( Point p );

    virtual string get_string() const;
};

class TextGlyph: public LabelGlyph
{
    // This class ignores the parent's 'label_' and uses a Subject instead
  protected:
    Observer<string> str;

  public:
    TextGlyph( Subject<string>& s );
    TextGlyph( Subject<string>& s, TextStyle sty );
    ~TextGlyph();

    bool notify( const string& );  // In case of an update to the string
    virtual string get_string() const;
};

// This layer is badly named -- it really can contain ANY glyphs
class TextLayer: public Layer
{
    struct Info
    {
        Glyph* glyph;
    };
    vector<Info> info;

  public:
    TextLayer();
    ~TextLayer();

    void add( Glyph* glyph );
    void remove( Glyph* glyph );

    bool pick( Point p, MouseInfo& mouse );

    virtual void mark_damaged( const Rect& view_area, DamageArea& damaged );
    virtual void draw( PixelBuffer& buffer, const Point& origin, 
                       DamageArea& damaged );
};

#endif
