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
#include "StatusBar.h"

#include "os2_extra.h"

StatusBar::StatusBar()
    :layer0(0x08)
{
}

StatusBar::~StatusBar()
{
}

void StatusBar::handle_mouse( int msg, int x, int y, unsigned short kbdflag )
{
    MouseInfo mouse;
    mouse.kbd_flags = kbdflag;
    mouse.type = MouseMove;
    if( msg == WM_MOUSELEAVE )
    {
        // Special case
        layer1.pick( Point(-1,-1), mouse );
        return;
    }
    if( msg == WM_SINGLESELECT ) mouse.type = MouseSelect;
    if( msg == WM_CONTEXTMENU ) mouse.type = MouseContextMenu;
    if( msg == WM_BUTTON1DOWN ) mouse.type = MouseClickBegin; 
    if( msg == WM_BUTTON1UP ) mouse.type = MouseClickEnd;
    layer1.pick( Point(x,y), mouse );
}

void StatusBar::update()
{
    destroy_buffer();
    bool full_redraw = (pb==NULL);
    create_buffer();

    Mutex::Lock lock( buffer_lock );
    Rect area( this->area() );
    DamageArea damaged( area );

    if( full_redraw )
        damaged += area;
    layer0.mark_damaged( area, damaged );
    layer1.mark_damaged( area, damaged );

    PixelBuffer& B = *pb;
    layer0.draw( B, Point(0,0), damaged );
    layer1.draw( B, Point(0,0), damaged );

    for( int i = 0; i < damaged.num_rects(); i++ )
    {
        Rect damage_rect = damaged.rect(i);
        invalidate( damage_rect );
    }
}
