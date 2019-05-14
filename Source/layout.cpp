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

#include "Figment.h"
#include "Sprites.h"
#include "Layer.h"
#include "Glyph.h"
#include "Layout.h"

#undef VARYPARAM
#define VARYPARAM(W) W* g1,W* g2,W* g3,W* g4,W* g5,W* g6,W* g7,W* g8,W* g9,W* g10,W* g11,W* g12,W* g13,W* g14,W* g15,W* g16,W* g17,W* g18,W* g19

WindowLayout::~WindowLayout() { }

//////////////////////////////////////////////////////////////////////

class GroupLayout: public WindowLayout
{
  public:
    enum Policy { Horizontal, Vertical, Overlap };   

    GroupLayout( Policy policy, VARYPARAM(Glyph) );
    virtual ~GroupLayout();

    virtual void add( Glyph* w );

    virtual void request( Size& c, Size& s );
    virtual void allocate( const Rect& area );

    // Draw
    //   Draw each child
    virtual void draw_region( PixelBuffer& pb, Point origin, DamageArea& );

    // Mark damage
    virtual void mark_damaged( const Rect& area, DamageArea& damaged );

    // Pick
    //   Pick one child
    virtual Glyph* pick( Point p );

    struct Info
    {
        Glyph* w;
        Size c;
        Size s;
        int x, y;
    };
  protected:
    Policy policy;
    vector<Info> children;

    void sum_req( long& ct, long& st, long (Size::*d) );
    void max_req( long& ct, long& st, long (Size::*d) );
    void sum_alloc( int p0, int ct, long (Size::*d), int (Info::*p) );
    void max_alloc( int p0, int ct, long (Size::*d), int (Info::*p) );
};

GroupLayout::GroupLayout( Policy p, VARYPARAM(Glyph) )
    :policy(p)
{
    children.reserve(16);       // otherwise, STL makes the init vector huge
    add(g1);add(g2);add(g3);
    add(g4);add(g5);add(g6);
    add(g7);add(g8);add(g9);    
    add(g10);
    add(g11);add(g12);add(g13);
    add(g14);add(g15);add(g16);
    add(g17);add(g18);add(g19); 
}

GroupLayout::~GroupLayout()
{
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
        delete (*i).w;
}

void GroupLayout::add( Glyph* w )
{
    if( w != NULL )
    {
        Info i;
        i.w = w;
        children.push_back( i );
    }
}

void GroupLayout::sum_req( long& ct, long& st, long (Size::*d) )
{
    ct = 0;
    st = 0;
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
    {
        ct += (*i).c.*d;
        st += (*i).s.*d;
    }
}

void GroupLayout::max_req( long& ct, long& st, long (Size::*d) )
{
    ct = 0;
    st = MaxStretch;
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
    {
        int ci = (*i).c.*d;
        int cs = ci + (*i).s.*d;
        if( ci > ct ) ct = ci;
        if( cs < st ) st = cs;
    }
    st -= ct;
    if( st < 0 ) st = 0;
}

void GroupLayout::sum_alloc( int p0, int ct, long (Size::*d), 
                             int (GroupLayout::Info::*p) )
{
    // First find out how much the widgets really wanted (at minimum)
    int t = 0;
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
        t += (*i).c.*d;

    // While there are more pixels available than required, we
    // can try to expand each widget by its stretch amount
    while( ct > t )
    {
        long diff = ct-t;
        int count = 0;
        for( vector<Info>::iterator i=children.begin();
             i!=children.end(); ++i )
            if( (*i).s.*d > 0 )
                count++;

        // If there are no widgets that will accept the pixels, break out
        if( count == 0 )
            break;
        long diff_per_child = (diff+count-1)/count;
        for( vector<Info>::iterator i=children.begin();
             i!=children.end(); ++i )
            if( (*i).s.*d > 0 )
            {
                // If we can transfer something, do it and update counts
                int transfer = min(diff_per_child,min((*i).s.*d,diff));
                (*i).c.*d += transfer;
                (*i).s.*d -= transfer;
                // There is less left to transfer this round
                diff -= transfer;
                // The total space allocated has increased
                t += transfer;
                // There is one less widget that gets space this round
                count--;
            }
    }

    // Determine a position for each widget
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
    {
        (*i).*p = p0;
        p0 += (*i).c.*d; 
    }
}

void GroupLayout::max_alloc( int p0, int ct, long (Size::*d),
                             int (GroupLayout::Info::*p) )
{
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
    {
        // All of them are positioned at the same place
        (*i).*p = p0;

        // Can the widget be expanded?
#if 0
        // This doesn't seem to work...
        int dd = min( ct - (*i).c.*d, (*i).s.*d );
        if( dd > 0 )
            (*i).c.*d += dd;
#else
        (*i).c.*d = ct;
#endif
    }
}

void GroupLayout::request( Size& c, Size& s )
{
    // First ask each child to see what it wants
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
        (*i).w -> request( (*i).c, (*i).s );

    // Add up the numbers to determine how much this widget needs
    if( policy == Horizontal )
    {
        sum_req( c.cx, s.cx, &Size::cx );
        max_req( c.cy, s.cy, &Size::cy );
    }
    else if( policy == Vertical )
    {
        max_req( c.cx, s.cx, &Size::cx );
        sum_req( c.cy, s.cy, &Size::cy );
    }
    else
    {
        max_req( c.cx, s.cx, &Size::cx );
        max_req( c.cy, s.cy, &Size::cy );
    }
}

void GroupLayout::allocate( const Rect& area )
{
    // Record the area for this glyph
    WindowLayout::allocate( area );
    
    // First determine how to give the space we have to the subwidgets
    if( policy == Horizontal )
    {
        sum_alloc( area.xLeft, area.width(), &Size::cx, &Info::x );
        max_alloc( area.yBottom, area.height(), &Size::cy, &Info::y );
    }
    else if( policy == Vertical )
    {
        max_alloc( area.xLeft, area.width(), &Size::cx, &Info::x );
        sum_alloc( area.yBottom, area.height(), &Size::cy, &Info::y );
    }
    else
    {
        max_alloc( area.xLeft, area.width(), &Size::cx, &Info::x );
        max_alloc( area.yBottom, area.height(), &Size::cy, &Info::y );
    }

    // Tell each widget how much space it got
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
        (*i).w -> allocate( Rect( Point((*i).x,(*i).y), (*i).c ) );
}

void GroupLayout::draw_region( PixelBuffer& pb, Point origin, 
                               DamageArea& damage )
{
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
        (*i).w -> draw_region( pb, origin, damage );
}

void GroupLayout::mark_damaged( const Rect& area, DamageArea& damage )
{
    Glyph::mark_damaged( area, damage );
    for( vector<Info>::iterator i=children.begin(); i!=children.end(); ++i )
        (*i).w -> mark_damaged( area, damage );
}

Glyph* GroupLayout::pick( Point p )
{
    // Go through in reverse order and let every child pick
    for( vector<Info>::iterator i=children.end(); i!=children.begin(); --i )
    {       
        Glyph* g = (*(i-1)).w;
        if( g->area().contains(p) )
        {
            g = g->pick(p);
            if( g != NULL ) return g;
        }
    }
    return NULL;
}

//////////////////////////////////////////////////////////////////////
class Deck: public GroupLayout
{
  public:
    Deck( Subject<int>& which, VARYPARAM(Glyph) )
        :GroupLayout( Overlap,
                      g1,g2,g3,g4,g5,g6,g7,g8,g9,g10,
                      g11,g12,g13,g14,g15,g16,g17,g18,g19 ),
         index_( which, closure(this,&Deck::index_changed) )
    {
    }
    
    ~Deck()
    {
    }
       
    virtual void draw( PixelBuffer& pb, Point origin, const Rect& area );
    virtual void draw_region( PixelBuffer& pb, Point origin, DamageArea& );
    virtual Glyph* pick( Point p );

  protected:
    bool index_changed( const int& );
    Observer<int> index_;
};

bool Deck::index_changed( const int& )
{
    set_damaged();
    return true;
}

void Deck::draw( PixelBuffer& pb, Point origin, const Rect& area )
{
    int i = index_.subject();
    if( i >= 0 && i < children.size() )
        children[i].w->draw( pb, origin, area );
}

void Deck::draw_region( PixelBuffer& pb, Point origin, DamageArea& damaged )
{
    int i = index_.subject();
    if( i >= 0 && i < children.size() )
        children[i].w->draw_region( pb, origin, damaged );
}

Glyph* Deck::pick( Point p )
{
    int i = index_.subject();
    if( i >= 0 && i < children.size() ) return children[i].w->pick(p);
    else return NULL;
}

//////////////////////////////////////////////////////////////////////

class AtomLayout: public WindowLayout
{
  public:
    AtomLayout( int cx0, int cy0, int sx0, int sy0 )
        :cx(cx0), cy(cy0), sx(sx0), sy(sy0) {}
    virtual ~AtomLayout() {}

    virtual void request( Size& c, Size& s );

  protected:
    int cx, cy;
    int sx, sy;
};

void AtomLayout::request( Size& c, Size& s )
{
    c.cx = cx;
    c.cy = cy;
    s.cx = sx;
    s.cy = sy;
}

class AtomWindowLayout: public AtomLayout
{
  public:
    AtomWindowLayout( Window* w, int cx, int cy, int sx, int sy )
        :AtomLayout( cx, cy, sx, sy), window(w) {}
 
    virtual void allocate( const Rect& area );
  private:
    Window* window;
};

void AtomWindowLayout::allocate( const Rect& area )
{
    AtomLayout::allocate( area );
    if( window )
        window->position( area.bottom_left(), area.width(), area.height() );
}

class AtomGlyphLayout: public AtomLayout
{
  public:
    AtomGlyphLayout( Glyph* w, int cx, int cy, int sx, int sy )
        :AtomLayout( cx, cy, sx, sy), window(w) {}

    virtual void request( Size& c, Size& s );
    virtual void allocate( const Rect& area );
    virtual void draw( PixelBuffer& pb, Point origin, const Rect& area );
    virtual void draw_region( PixelBuffer& pb, Point origin, DamageArea& );
    virtual void mark_damaged( const Rect& area, DamageArea& damage );
    virtual Glyph* pick( Point p );
  private:
    Glyph* window;
};

void AtomGlyphLayout::request( Size& c, Size& s )
{
    if( window ) window->request( c, s );
    AtomLayout::request( c, s );
}

void AtomGlyphLayout::allocate( const Rect& area )
{
    AtomLayout::allocate( area );
    if( window ) window->allocate( area );
}

void AtomGlyphLayout::draw( PixelBuffer& pb, Point origin, const Rect& area )
{ 
    if( window ) window->draw( pb, origin, area ); 
}

void AtomGlyphLayout::draw_region( PixelBuffer& pb, Point or, DamageArea& dm )
{ 
    if( window ) window->draw_region( pb, or, dm ); 
}

void AtomGlyphLayout::mark_damaged( const Rect& area, DamageArea& damage )
{ 
    AtomLayout::mark_damaged( area, damage );
    if( window ) window->mark_damaged( area, damage ); 
}

Glyph* AtomGlyphLayout::pick( Point p )
{
    if( window ) return window->pick(p);
    else return NULL;
}
    
//////////////////////////////////////////////////////////////////////

WindowLayout* WindowLayout::horiz( VARYPARAM(Glyph) )
{
    return new GroupLayout( GroupLayout::Horizontal,
                            g1,g2,g3,g4,g5,g6,g7,g8,g9,g10,
                            g11,g12,g13,g14,g15,g16,g17,g18,g19 );
}

WindowLayout* WindowLayout::vert( VARYPARAM(Glyph) )
{
    return new GroupLayout( GroupLayout::Vertical,
                            g1,g2,g3,g4,g5,g6,g7,g8,g9,g10,
                            g11,g12,g13,g14,g15,g16,g17,g18,g19 );
}

WindowLayout* WindowLayout::overlay( VARYPARAM(Glyph) )
{
    return new GroupLayout( GroupLayout::Overlap,
                            g1,g2,g3,g4,g5,g6,g7,g8,g9,g10,
                            g11,g12,g13,g14,g15,g16,g17,g18,g19 );
}

WindowLayout* WindowLayout::deck( Subject<int>& index, VARYPARAM(Glyph) )
{
    return new Deck( index, 
                     g1,g2,g3,g4,g5,g6,g7,g8,g9,g10,
                     g11,g12,g13,g14,g15,g16,g17,g18,g19 );
}

WindowLayout* WindowLayout::hcenter( Glyph* wl )
{
    return horiz( hglue(0), wl, hglue(0) );
}

WindowLayout* WindowLayout::vcenter( Glyph* wl )
{
    return vert( vglue(0), wl, vglue(0) );
}

WindowLayout* WindowLayout::center( Glyph* wl )
{
    return hcenter(vcenter(wl));
}

WindowLayout* WindowLayout::fixed( Window* w, int cx, int cy )
{
    return new AtomWindowLayout( w, cx, cy, NoStretch, NoStretch );
}

WindowLayout* WindowLayout::stretchable( Window* w, int cx, int cy,
                                         int sx, int sy )
{
    return new AtomWindowLayout( w, cx, cy, sx, sy );
}

WindowLayout* WindowLayout::fixed( Glyph* w, int cx, int cy )
{
    return new AtomGlyphLayout( w, cx, cy, NoStretch, NoStretch );
}

WindowLayout* WindowLayout::stretchable( Glyph* w, int cx, int cy,
                                         int sx, int sy )
{
    return new AtomGlyphLayout( w, cx, cy, sx, sy );
}

WindowLayout* WindowLayout::space( int cx, int cy )
{
    return new AtomWindowLayout( NULL, cx, cy, NoStretch, NoStretch );
}

WindowLayout* WindowLayout::hspace( int cx )
{
    return new AtomWindowLayout( NULL, cx, 1, NoStretch, MaxStretch );
}

WindowLayout* WindowLayout::vspace( int cy )
{
    return new AtomWindowLayout( NULL, 1, cy, MaxStretch, NoStretch );
}

WindowLayout* WindowLayout::glue( int cx, int cy, int sx, int sy )
{
    return new AtomWindowLayout( NULL, cx, cy, sx, sy );
}

WindowLayout* WindowLayout::hglue( int cx )
{
    return new AtomWindowLayout( NULL, cx, 1, MaxStretch, MaxStretch );
}

WindowLayout* WindowLayout::vglue( int cy )
{
    return new AtomWindowLayout( NULL, 1, cy, MaxStretch, MaxStretch );
}

//////////////////////////////////////////////////////////////////////

