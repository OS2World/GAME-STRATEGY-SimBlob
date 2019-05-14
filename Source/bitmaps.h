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

#ifndef Bitmaps_h
#define Bitmaps_h

#include "HexCoord.h"
// #include "BmpFormat.h"

class Bitmap
{
  public:
	Bitmap();
	Bitmap( int ID );
	Bitmap( int width, int height );
	Bitmap( int ID, int width, int height );
	Bitmap( const char* filename );
    // Bitmap( BitmapFile& bf );
	virtual ~Bitmap();

	void load( int ID );
	void load( int ID, int width, int height );
	void load( int width, int height );
	void load( const char* filename );
    // void load( BitmapFile& bf );
	void unload();

	void bitblt( PS& ps, int x, int y, int paste_type,
				 int xoffset = 0, int yoffset = 0,
				 int xsize = HexWidth, int ysize = HexHeight );
	PS& ps() { return hpsMemory; }
	HBITMAP handle() { return hbm; }

  protected:
	HDC hdcMemory;
	PS hpsMemory;
	HBITMAP hbm;
};

extern Bitmap tile;
#endif

