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

#ifndef Layout_h
#define Layout_h

#include "Glyph.h"
#include "Subject.h"

#define VARYPARAM(W) W* g1=NULL,W* g2=NULL,W* g3=NULL,W* g4=NULL,W* g5=NULL,W* g6=NULL,W* g7=NULL,W* g8=NULL,W* g9=NULL,W* g10=NULL,W* g11=NULL,W* g12=NULL,W* g13=NULL,W* g14=NULL,W* g15=NULL,W* g16=NULL,W* g17=NULL,W* g18=NULL,W* g19=NULL

// This is a factory that also serves as a base class.  These techniques
// were inspired by the InterViews UI library and the _Design Patterns_
// book (see the Composite pattern).
class WindowLayout: public Glyph
{
  public:
    enum { NoStretch = 0, MaxStretch = 32767 };

    virtual ~WindowLayout();

    //////////////////////////////////////////////////
    // Layout creation

    // Horizontal column of windows
    static WindowLayout* horiz( VARYPARAM(Glyph) );

    // Vertical column of windows
    static WindowLayout* vert( VARYPARAM(Glyph) );

    // Overlapping windows
    static WindowLayout* overlay( VARYPARAM(Glyph) );

    // Deck of windows - index chooses which one
    static WindowLayout* deck( Subject<int>& which, VARYPARAM(Glyph) );

    // Centering
    static WindowLayout* hcenter( Glyph* wl );
    static WindowLayout* vcenter( Glyph* wl );
    static WindowLayout* center( Glyph* wl );

    // Unstretchable window
    static WindowLayout* fixed( Window* w, int cx, int cy );
    static WindowLayout* fixed( Glyph* w, int cx, int cy );

    // Stretchable window, with (sx,sy) = stretch amounts
    static WindowLayout* stretchable( Window* w, int cx, int cy, 
                                      int sx=MaxStretch, int sy=MaxStretch );
    static WindowLayout* stretchable( Glyph* w, int cx, int cy, 
                                      int sx=MaxStretch, int sy=MaxStretch );

    static WindowLayout* hstretchable( Window* w, int cx, int cy )
    { return WindowLayout::stretchable( w, cx, cy, MaxStretch, 0 ); }
    static WindowLayout* vstretchable( Window* w, int cx, int cy )
    { return WindowLayout::stretchable( w, cx, cy, 0, MaxStretch ); }
    
    static WindowLayout* hstretchable( Glyph* w, int cx, int cy )
    { return WindowLayout::stretchable( w, cx, cy, MaxStretch, 0 ); }
    static WindowLayout* vstretchable( Glyph* w, int cx, int cy )
    { return WindowLayout::stretchable( w, cx, cy, 0, MaxStretch ); }
    
    // Empty space
    static WindowLayout* space( int cx, int cy );
    static WindowLayout* hspace( int cx );
    static WindowLayout* vspace( int cy );

    // Glue
    static WindowLayout* glue( int cx, int cy, 
                               int sx=MaxStretch, int sy=MaxStretch );
    static WindowLayout* hglue( int cx );
    static WindowLayout* vglue( int cy );

    //
    //////////////////////////////////////////////////
};

#endif
