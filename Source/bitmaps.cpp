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

#include "Figment.h"                    // Figment class library
#include "Bitmaps.h"

#include <memory.h>

Bitmap tile;

// The bitmap class should store the size of the bitmap
// (Otherwise, it has to get it from the query bitmap parameters API

Bitmap::Bitmap()
{
    hbm = NULLHANDLE;
}

Bitmap::Bitmap( int ID, int width, int height )
{
    hbm = NULLHANDLE;
    load( ID, width, height );
}

Bitmap::Bitmap( int ID )
{
    hbm = NULLHANDLE;
    load( ID );
}

Bitmap::Bitmap( int width, int height )
{
    hbm = NULLHANDLE;
    load( width, height );
}

void Bitmap::load( int width, int height )
{
    // This function is ugly!
    
    unload();

    SIZEL sizl = { 0, 0 };  /* use same page size as device         */
    /* context data structure */
    DEVOPENSTRUC dop = { 0L, PSZ("DISPLAY"), NULL, 0L, 0L, 0L, 0L, 0L, 0L };

    // create memory DC and presentation space, associating DC with the PS
    hdcMemory = DevOpenDC( Figment::hab, OD_MEMORY, PSZ("*"),
                           5L, (PDEVOPENDATA)&dop, NULLHANDLE );
    hpsMemory = PS( GpiCreatePS( Figment::hab, hdcMemory, &sizl,
                                 GPIA_ASSOC|PU_PELS|GPIT_MICRO|GPIF_DEFAULT ) );

    // Make a width x height bitmap with 8-bit depth
    BITMAPINFOHEADER2 info;
    memset( &info, 0, sizeof(BITMAPINFOHEADER2) );
    info.cbFix = 16;
    info.cx = width;
    info.cy = height;
    info.cPlanes = 1;
    info.cBitCount = 8;
    hbm = GpiCreateBitmap( hpsMemory, &info, 0, NULL, NULL );

    GpiSetBitmap( hpsMemory, hbm );
}

void Bitmap::load( int ID )
{
    load( ID, HexWidth, HexHeight );
}

void Bitmap::load( int ID, int width, int height )
{
    // Just like load(w,h) but instead of creating a bitmap, it
    // loads one from the resources linked into the executable
    unload();

    SIZEL sizl = { 0, 0 };  /* use same page size as device         */
    /* context data structure */
    DEVOPENSTRUC dop = { 0L, PSZ("DISPLAY"), NULL, 0L, 0L, 0L, 0L, 0L, 0L };

    // create memory DC and presentation space, associating DC with the PS
    hdcMemory = DevOpenDC( Figment::hab, OD_MEMORY, PSZ("*"), 5L,
                           reinterpret_cast<PDEVOPENDATA>(&dop), NULLHANDLE );
    if( hdcMemory == DEV_ERROR )
        Throw("Bitmap::load -- Could not OpenDC");
    if( Figment::hab == 0 )
        Throw("HAB not created yet");
    hpsMemory = PS( GpiCreatePS( Figment::hab, hdcMemory, &sizl,
                                 GPIA_ASSOC|PU_PELS|GPIT_MICRO|GPIF_DEFAULT ) );
    hbm = GpiLoadBitmap( hpsMemory, NULLHANDLE, ID, width, height );

    if( hbm == GPI_ERROR )
    {
        int errcode = WinGetLastError( Figment::hab );
        static char err[256];
        sprintf( err, "Bitmap::load -- Could not load bitmap %d -- error code is 0x%x", ID, errcode );
        Throw(err);
    }
    GpiSetBitmap( hpsMemory, hbm );
}

void Bitmap::unload()
{
    if( hbm != NULLHANDLE )
    {
        if( GpiSetBitmap( hpsMemory, NULLHANDLE ) == GPI_ERROR )
            Throw("Could not reset bitmap");
        if( GpiDeleteBitmap( hbm ) == GPI_ERROR )
            Throw("Could not delete bitmap");
        hbm = NULLHANDLE;

        if( GpiDestroyPS( hpsMemory ) == GPI_ERROR )
            Throw("Could not destroy PS");
        if( DevCloseDC( hdcMemory ) == GPI_ERROR )
            Throw("Could not close DC");
    }
}

void Bitmap::bitblt( PS& my_ps, int x, int y, int paste_type,
                     int xoffset, int yoffset, int xsize, int ysize )
{
    if( hbm != NULLHANDLE )
    {
        // display the bit map on the screen by copying it from the memory
        // device context into the screen device context
        my_ps.bitblt( PS(hpsMemory), 
                      Rect(x,y,x+xsize,y+ysize), 
                      Point(xoffset,yoffset), 
                      paste_type );
    }
}

Bitmap::~Bitmap()
{
    unload();
}

