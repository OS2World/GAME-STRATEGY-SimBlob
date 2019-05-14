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

#ifndef GlyphLib_h
#define GlyphLib_h

// GlyphLib is a library of occasionally useful glyphs.

// NeatBackground is a diagonal gradient with a border.  There are two
// flags that are controlled with Subjects, so you can hook this glyph
// up with other glyphs (for example, flyover or a toggle button).
class NeatBackground: public Glyph
{
    Subject<bool>* reverse;
    Subject<bool>* highlight;

    bool notify_update( const bool& ) { set_damaged(); return true; }

  public:
    NeatBackground( Subject<bool>* r = NULL, Subject<bool>* h = NULL );
    virtual ~NeatBackground();

    virtual void draw( PixelBuffer& pb, Point, const Rect& clip_area );
};

// BitmapGlyph draws a sprite
class BitmapGlyph: public Glyph
{
    Sprite& sprite;
  public:
    BitmapGlyph( Sprite& s ): sprite(s) {}

    virtual void draw( PixelBuffer& pb, Point, const Rect& clip_area );
};

class BufferWindow;
// RadioButtonGlyph takes a subject c and a value v, and whenever it
// is clicked, it will set c to v.  A radio "group" (in traditional
// systems) is built from many radio buttons sharing the same subject c.
class RadioButtonGlyph: public Glyph
{
    int value;
    Subject<bool>& highlight;
    Subject<bool>& down;
    BufferWindow* window;
    Observer<int> control;

    inline bool set_control( const int& new_v )
    {
        down = bool( new_v == value );
        window->update();
        return true;
    }

  public:
    RadioButtonGlyph( Subject<int>& c, int v, BufferWindow* win,
                      Subject<bool>& h, Subject<bool>& d );
    
    ~RadioButtonGlyph()
    {
    }
    
    virtual Glyph* pick( Point ) { return this; }
    virtual void mouse_enter()
    {
        highlight = true;
        window->update();
    }
    virtual void mouse_leave()
    {
        highlight = false;
        window->update();
    }
    virtual void mouse_select( Point )
    {
        control.subject() = value;
        window->update();
    }
};

// The custom button cycles through the values in [0..cp-1],
// with left button incrementing and right button decrementing
class CustomButtonGlyph: public Glyph
{
    Subject<int>& index;
    BufferWindow* window;
    Subject<bool>& highlight;
    Subject<bool>& down;
    int cycle_period;
  public:
    CustomButtonGlyph( Subject<int>& i, BufferWindow* win, 
                       Subject<bool>& h, Subject<bool>& d, int cp )
        : index(i), window(win), highlight(h), down(d),
          cycle_period(cp) {}

    virtual Glyph* pick( Point ) { return this; }
    virtual void mouse_enter()
    {
        highlight = true;
        window->update();
    }
    virtual void mouse_leave()
    {
        highlight = false;
        window->update();
    }
    virtual void mouse_downL( Point )
    {
        down = true;
        window->update();
    }
    virtual void mouse_upL( Point )
    {
        down = false;
        window->update();
    }
    virtual void mouse_select( Point )
    {
        index = (1+index)%cycle_period;
        window->update();
    }
    virtual void mouse_contextmenu( Point )
    {
        index = (cycle_period-1+index)%cycle_period;
        window->update();
    }
};

// NotebookTab is purely visual
class NotebookTab: public Glyph
{
    Subject<bool>* reverse;
    bool notify_update( const bool& ) { set_damaged(); return true; }

  public:
    NotebookTab( Subject<bool>* r = NULL );
    virtual ~NotebookTab();

    virtual void draw( PixelBuffer& pb, Point, const Rect& clip_area );
};

// SolidArea just draws a rectangle in the desired color; it might
// be transparent.  This glyph can be hooked up to a boolean subject
// to turn on and off the drawing.  It's useful for providing a highlight
// color underneath some other glyph (hooked up to a flyover glyph), or
// lightening or darkening an area (by using transparent white/black).
class SolidArea: public Glyph
{
    byte color_index_;
    int transparency_type_;
    
    Subject<bool>* highlight;
    bool notify_update( const bool& ) { set_damaged(); return true; }

  public:
    SolidArea( byte color_index, int transparency_type,
               Subject<bool>* h = NULL );
    virtual ~SolidArea();
    
    virtual void draw( PixelBuffer& pb, Point, const Rect& clip_area );
};

// DividerGlyph just draws a shaded border
class DividerGlyph: public Glyph
{
  public:
    DividerGlyph() {}
    virtual void draw( PixelBuffer& buffer, Point origin, const Rect& area );
};
    
#endif
