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

#ifndef BmpFormat_h
#define BmpFormat_h

#include "Types.h"
#include "Notion.h"

class PS;

struct BitmapFile
{
	BITMAPFILEHEADER2* header;
	BITMAPINFOHEADER2* info;
	byte* pixels;		// bitmap info, with each row word-aligned
	int rowwidth;		// helps us to find the start of each row in pixels

	BitmapFile();
	~BitmapFile();

	void read( const char* filename );
	void capture( int cx, int cy, PS& ps );
	void capture( HBITMAP bitmap, PS& ps );
	void write( const char* filename );

	void convert_to_image( Color transparent );
	void convert_to_mask ( Color transparent );

	void free();

	private:
	  BitmapFile( const BitmapFile& );
};

struct Indexed256Bitmap
{
	BITMAPFILEHEADER2* header;
	BITMAPINFOHEADER2* info;
	byte* pixels;		// bitmap info, with each row word-aligned
	int rowwidth;		// helps us to find the start of each row in pixels

	Indexed256Bitmap();
	~Indexed256Bitmap();

	void convert( PS& ps, BitmapFile& rgb_bitmap );
	void free();
	void write( const char* filename );

	private:
	  Indexed256Bitmap( const Indexed256Bitmap& );
};

#endif
