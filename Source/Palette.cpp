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
#include "stl.h"

#include "Figment.h"
#include "Palette.h"

static bool PaletteManagerExists()
{
    CachedPS desktop_ps( HWND_DESKTOP );
    HDC dc = GpiQueryDevice( desktop_ps );
    long flags;
    DevQueryCaps( dc, CAPS_ADDITIONAL_GRAPHICS, 1, &flags );
    return ( CAPS_PALETTE_MANAGER & flags ) != 0;
}

static bool palette_manager_exists = PaletteManagerExists();

ColorPalette::ColorPalette( PS& ps_ )
    :ps(ps_), pal(NULLHANDLE)
{
}

ColorPalette::~ColorPalette()
{
    destroy();
}

void ColorPalette::create()
{
    if( palette_manager_exists )
    {
        extern unsigned long DesktopRGB[256];
        pal = GpiCreatePalette( ps, 0, LCOLF_CONSECRGB, 256, DesktopRGB );
        GpiSelectPalette( ps, pal );
    }
}

void ColorPalette::select( PS& ps )
{
    if( palette_manager_exists )
    {
        GpiSelectPalette( ps, pal );
    }
}

void ColorPalette::unselect( PS& ps )
{
    if( palette_manager_exists )
    {
        GpiSelectPalette( ps, NULLHANDLE );
    }
}

bool ColorPalette::realize( HWND wnd )
{
    if( palette_manager_exists && pal != NULLHANDLE )
    {
        unsigned long pccir = 256;
        return ( WinRealizePalette( wnd, ps, &pccir ) != 0 );
    }
    else
        return false;
}

void ColorPalette::destroy()
{
    if( palette_manager_exists && pal != NULLHANDLE )
    {
        GpiSelectPalette( ps, NULLHANDLE );
        GpiDeletePalette( pal );
        pal = NULLHANDLE;
    }
}
