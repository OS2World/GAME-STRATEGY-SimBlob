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
#include "SimBlob.h"

#include "Sprites.h"
#include "StatusBar.h"

#include "GlyphLib.h"
#include "TextGlyph.h"
#include "Layout.h"

#include "Images.h"

Subject<string> date_info( string("Date:               ") );
Subject<string> status_info( string("BlobCity") );

Subject<string> speed_info_ticks( string("Speed") );
Subject<string> speed_info_frames( string("") );
Subject<string> speed_info_pixels( string("") );

Subject<string> money_info( string("0") );

Subject<string> info4( string("BlobCity") );
Subject<string> info5( string("0") );
Subject<string> info6( string("0") );
Subject<string> info7( string("0") );
Subject<string> info8( string("0") );
Subject<string> info9( string("0") );
Subject<string> info10( string("0") );

Subject<string> current_tool_help( string("Clear") );
Subject<int> tb_index( -2 );

// The top information window has modes for
// finance (0), census (1), and speed (2)
Subject<int> info_page( 1 );
Subject<bool> finance_highlight( false );
Subject<bool> finance_down( false );
Subject<bool> census_highlight( false );
Subject<bool> census_down( false );
Subject<bool> speed_highlight( false );
Subject<bool> speed_down( false );

Subject<int> debug_view( ID_DEBUG_NONE );

Subject<string> toolmsg("");

Glyph* infoarea;
Glyph* btarea;
Glyph* sbarea;

struct ToolToHelp
{
    Observer<int> observer;
    bool notify( const int& newval );

    ToolToHelp(): observer( tb_index, closure(this, &ToolToHelp::notify) ) {}
} tool_to_help;

bool ToolToHelp::notify( const int& newval )
{
    const char* s = "";
    switch( newval )
    {
      case 0: s = "Clear - 5s"; break;
      case 1: s = "Road - 30s"; break;
      case 2: s = "Bridge - 80s"; break;
      case 3: s = "Trench - 50s"; break;
      case 4: s = "Wall - 100s"; break;
      case 5: s = "Gate - 200s"; break;
      case 6: s = "Watchtower - 500s"; break;
      case 7: s = "Fire - 2s"; break;
      case 8: s = "Forest - 40s"; break;
      case -2: s = "Information"; break;
      case -1: s = "Blob"; break;
    }
    current_tool_help = string(s);
    return true;
}

Glyph* neat_button( StatusBar* owner, Subject<int>& index,
                    Subject<bool>& down, Subject<bool>& highlight,
                    int cycle_count, Glyph* a0=NULL, Glyph* a1=NULL,
                    Glyph* a2=NULL, Glyph* a3=NULL, Glyph* a4=NULL )
{
    WindowLayout& WL = *(static_cast<WindowLayout*>(NULL));
    return 
        WL.overlay
        (
         new NeatBackground( &down, &highlight ),
         WL.deck( index, a0, a1, a2, a3, a4 ),
         new CustomButtonGlyph( index, owner, highlight, down, cycle_count )
         );
}

void build_glyphs( StatusBar* infoarea_,
                   StatusBar* buttonbar_,
                   StatusBar* statusbar_ )
{
    WindowLayout& WL = *(static_cast<WindowLayout*>(NULL));

    TextStyle statusbar_text( Font::Shadow );
    TextStyle statusbar_right_text( TextStyle::Right, Font::Shadow );
    TextStyle date_text( TextStyle::Center, Font::Shadow );
    TextStyle title_text( Font::Shadow, &text24 );
    TextStyle info_text( TextStyle::Right, Font::Normal );
    title_text.text_color = 0x01;
    title_text.shadow_color = 0xff;
    info_text.text_color = 0xA0;
    
    TextGlyph* date = new TextGlyph( date_info, date_text );
    TextGlyph* money = new TextGlyph( money_info, statusbar_right_text );
    TextGlyph* status = new TextGlyph( status_info, statusbar_text );
    TextGlyph* sp_ticks = new TextGlyph( speed_info_ticks, statusbar_text );
    TextGlyph* sp_frames = new TextGlyph( speed_info_frames, info_text );
    TextGlyph* sp_pixels = new TextGlyph( speed_info_pixels, info_text );

    TextGlyph* glyph4 = new TextGlyph( info4, title_text );
    TextGlyph* glyph5 = new TextGlyph( info5, statusbar_right_text );
    
    TextGlyph* glyph6 = new TextGlyph( info6, info_text );
    TextGlyph* glyph7 = new TextGlyph( info7, info_text );
    TextGlyph* glyph8 = new TextGlyph( info8, info_text );
    TextGlyph* glyph9 = new TextGlyph( info9, info_text );
    TextGlyph* glyph10 = new TextGlyph( info10, info_text );

    TextStyle header_text( TextStyle::Left, Font::Normal, &text12i );
    header_text.text_color = GrayColors[6];

    struct Radio
    {
        BufferWindow* window;
        Radio( BufferWindow* win ): window(win) {}
        
        Glyph* make( int value, Sprite& rle )
        {
            Subject<bool>* h = new Subject<bool>(false);
            Subject<bool>* d = new Subject<bool>(false);
            WindowLayout& WL = *(static_cast<WindowLayout*>(NULL));
            return 
                WL.fixed( WL.overlay(
                                     new NeatBackground( d, h ),
                                     new BitmapGlyph( rle ), 
                                     new RadioButtonGlyph( tb_index, value, 
                                                           window, *h, *d )
                                     ), 32, 32 );
        }
    };

    // The info area has its own layout
    Radio radio( infoarea_ );
    infoarea = 
        WL.vert
        (
         WL.vert(
                 WL.vspace(8),
                 WL.horiz(
                          WL.hglue(0),
                          radio.make( 8, lone_tree_rle ),
                          WL.overlay(
                                     radio.make( 6, tower_rle ),
                                     new BitmapGlyph( fire1_rle )
                                     ),
                          radio.make( 2, bridge_rle ),
                          radio.make( 3, canal_rle ),
                          WL.hglue(0)
                          ),
                 WL.horiz(
                          WL.hglue(0),
                          radio.make( 4, wallbmp_rle ),
                          radio.make( 7, fire2_rle ),
                          radio.make( 1, road_rle[9] ),
                          radio.make( 5, gate_rle ),
                          WL.hglue(0)
                          ),
                 WL.horiz(
                          WL.hglue(0),
                          radio.make( 0, clear_rle ),
                          radio.make( -2, hand_rle ),
                          radio.make( -1, soldier_rle ),
                          WL.hglue(0)
                          ),
                 WL.vglue(0),
                 WL.vspace( 2 ),
                 WL.stretchable( new TextGlyph( toolmsg ), 10, 16 )
                 ),
         WL.vspace( 4 ),
         WL.hstretchable( new DividerGlyph, 1, 4 ),
         WL.vert(
                 WL.hstretchable( date, 1, 16 ),
                 WL.vspace( 4 ),
                 WL.hcenter( glyph4 )
                 )
         );

    // Lay out the button bar
    Glyph* finance_button =
        WL.overlay(
                   new NotebookTab( &finance_down ),
                   new SolidArea( 0xff, 3, &finance_highlight ),
                   WL.horiz(
                            WL.hspace(2),
                            new LabelGlyph( "Silver", header_text ),
                            WL.hspace(4),
                            WL.hstretchable(money,10,16),
                            WL.hspace(4)
                            ),
                   new RadioButtonGlyph( info_page, 0, 
                                         buttonbar_,
                                         finance_highlight,
                                         finance_down )
                   );

    Glyph* census_button =
        WL.overlay(
                   new NotebookTab( &census_down ),
                   new SolidArea( 0xff, 3, &census_highlight ),
                   WL.horiz(
                            WL.hspace(2),
                            new LabelGlyph( "Population", header_text ),
                            WL.hspace(4),
                            WL.hstretchable(glyph5,10,16),
                            WL.hspace(4)
                            ),
                   new RadioButtonGlyph( info_page, 1, 
                                         buttonbar_,
                                         census_highlight,
                                         census_down )
                   );
                   
    Glyph* speed_button =
        WL.overlay(
                   new NotebookTab( &speed_down ),
                   new SolidArea( 0xff, 3, &speed_highlight ),
                   WL.horiz( WL.hspace(4), WL.hstretchable(sp_ticks,1,18) ),
                   new RadioButtonGlyph( info_page, 2, 
                                         buttonbar_,
                                         speed_highlight,
                                         speed_down )
                   );
                   
    Glyph* finance_display =
        WL.overlay
        (
         new SolidArea( 0xff, 3, &finance_highlight ),
         WL.vert( WL.vglue(1),
                  new LabelGlyph("will go here"),
                  new LabelGlyph("Financial Page", header_text),
                  WL.vspace(2) )
         );

    struct L // convenience function
    {
        static Glyph* pair( Glyph* a, Glyph* b )
        {
            return WindowLayout::horiz
                ( a, WindowLayout::hstretchable( b, 1, 18 ) );
        }
    };
    
    Glyph* census_display =
        WL.overlay
        (
         new SolidArea( 0xff, 3, &census_highlight ),
         WL.horiz
         (
          WL.hspace(2),
          WL.hstretchable
          (
           WL.vert( L::pair( new LabelGlyph("Employment"), glyph6 ),
                    L::pair( new LabelGlyph("Census Page", header_text),
                             new BitmapGlyph( soldier_rle ) ) ),     
           1, 16 ),
          WL.vstretchable( new DividerGlyph, 4, 1 ),
          WL.hstretchable
          (
           WL.vert( L::pair( new LabelGlyph("Jobs Filled"), glyph8 ),
                    L::pair( new LabelGlyph("Jobs"), glyph7 ) ),
           1, 16 ),
          WL.vstretchable( new DividerGlyph, 4, 1 ),
          WL.hstretchable
          (
           WL.vert( L::pair( new LabelGlyph("Food Eaten"), glyph10 ),
                    L::pair( new LabelGlyph("Food Prod."), glyph9 ) ),
           1, 16 ),
          WL.hspace(2)
          )
         );

    Glyph* speed_display =
        WL.overlay
        (
         new SolidArea( 0xff, 3, &speed_highlight ),         
         WL.horiz( WL.hspace(10),
                   WL.vert( WL.vglue(1),
                            new LabelGlyph("Speed Page", header_text),
                            WL.vspace(2) ),
                   WL.vstretchable( new DividerGlyph, 4, 1 ),
                   WL.vert
                   ( L::pair( new LabelGlyph("Millions of pixels per second:"),
                              sp_pixels ),
                     L::pair( new LabelGlyph("Frames per second:"),
                              sp_frames ) ) )
         );
    
    // Position overlay area elements
    Glyph* overlayarea =
        WL.deck( info_page,
                 finance_display,
                 census_display,
                 speed_display );
    
    btarea =
        WL.vert
        (
         WL.hstretchable( new SolidArea(0x7f,1), 1, 1 ),
         WL.horiz
         (
          WL.hstretchable( new SolidArea(0x7f,2), 1, 1 ),
          WL.fixed( new SolidArea(0x7f,1), 1, 1 )
          ),
         WL.horiz
         (
          WL.vstretchable( new SolidArea(0xff,1), 1, 1 ),
          WL.vstretchable( new SolidArea(0xff,2), 1, 1 ),
          WL.stretchable( overlayarea, 2, 2 ),
          WL.vstretchable( new SolidArea(0x7f,2), 1, 1 ),
          WL.vstretchable( new SolidArea(0x7f,1), 1, 1 )
          ),
         WL.horiz
         (
          WL.hstretchable( finance_button, 2, 18 ),
          WL.hstretchable( census_button, 2, 18 ),
          WL.hstretchable( speed_button, 2, 18 )
          )
         );

    // Position status bar elements
    sbarea = 
        WL.horiz
        (
         WL.vstretchable( new TextGlyph( current_tool_help, TextStyle(TextStyle::Center) ), 150, 16 ),
         WL.stretchable( new DividerGlyph, 4, 1, 0 ),
         WL.hspace( 2 ),
         WL.stretchable( status, 100, 1 )
         );
}
