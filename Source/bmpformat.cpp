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

#include <memory.h>

#if 0

// Ew, none of this code isn't being used anymore... I used to store all
// my sprites as OS/2-format bitmaps, but now I use a portable format that
// properly supports transparency.

// I left it here but #if-ed it out.

#include "BmpFormat.h"
#include "Notion.h"
#include "Figment.h"

#include "initcode.h"

#if INIT_BITMAPS

BitmapFile::BitmapFile()
{
	header = NULL;
	info = NULL;
	rowwidth = 0;
	pixels = NULL;
}

BitmapFile::~BitmapFile()
{
	free();
}

void BitmapFile::free()
{
	delete[] pixels;
	delete header;
	header = NULL;
	info = NULL;
	rowwidth = 0;
	pixels = NULL;
}

void BitmapFile::read( const char* filename )
{
	free();

	FILE* f = fopen( filename, "rb" );
	if( !f )
		Throw("Could not open bitmap file");

	header = new BITMAPFILEHEADER2;
	info = &(header->bmp2);
	rowwidth = 0;
	pixels = NULL;

	Try
	{
		fread( header, sizeof(BITMAPFILEHEADER2)-sizeof(BITMAPINFOHEADER2), 1, f );
		if( header->usType != BFT_BMAP )
			Throw("Not a Bitmap File");
		memset( info, 0, sizeof(BITMAPINFOHEADER2) );
		fread( info, sizeof(info->cbFix), 1, f );
		if( info->cbFix <= 12 )
			Throw("Not an OS/2 2.0 Format Bitmap");
		fread( (char *)(info)+sizeof(info->cbFix), info->cbFix-sizeof(info->cbFix), 1, f );
		if( info->cPlanes != 1 )
			Throw("Can't handle planar image bitmaps");
		if( info->ulCompression != BCA_UNCOMP )
			Throw("Can't handle compressed bitmaps");

		int cols = info->cx, rows = info->cy;
		// after word alignment, how many bytes is each row?
		rowwidth = ( sizeof(RGB)*cols + 3 ) / 4 * 4;
		pixels = new byte[rowwidth*rows];
		if( !pixels )
			Throw("Could not allocate bitmap");

		if( info->cBitCount == 24 )
		{
			if( fread( pixels, sizeof(RGB), rowwidth*rows, f ) < rowwidth*rows )
				Throw("Could not read data from 24-bit encoded file");
		}
		else if( info->cBitCount == 8 || info->cBitCount == 4 )
		{
			// Read the color table
			int ncolors = 1 << (info->cPlanes * info->cBitCount);
			if( info->cclrUsed != 0 )
				ncolors = info->cclrUsed;

			RGB2* ctable = new RGB2[ncolors];
			if( fread( ctable, sizeof(RGB2), ncolors, f ) < ncolors )
			{
				delete [] ctable;
				Throw("Could not read color table");
			}

			if( info->cBitCount == 8 )
			{
				int rawwidth = ( cols+3 ) / 4 * 4;
				byte* rawdata = new byte[rawwidth*rows];
				fread( rawdata, rawwidth*rows, 1, f );
				for( int j = 0; j < rows; j++ )
					for( int i = 0; i < cols; i++ )
						((RGB*)(pixels+rowwidth*j))[i] = (const RGB&)(ctable[(rawdata+rawwidth*j)[i]]);
				delete[] rawdata;
			}
			else if( info->cBitCount == 4 )
			{
				int rawwidth = ( cols+7 ) / 8 * 4;
				byte* rawdata = new byte[rawwidth*rows];
				fread( rawdata, rawwidth*rows, 1, f );
				for( int j = 0; j < rows; j++ )
					for( int i = 0; i < cols; i++ )
					{
						// even half is the high bits, odd half the low bits
						int e = (rawdata+rawwidth*j)[i/2];
						if( i % 2 == 0 )
							e = e >> 4;
						else
							e = e & 0x0f;
						((RGB*)(pixels+rowwidth*j))[i] = (const RGB&)(ctable[e]);
					}
				delete[] rawdata;
			}
			else
			{
				delete[] ctable;
				Throw("Unknown indexed bit depth");
			}

			delete[] ctable;
		}
		else
			Throw("Unknown bit depth");
	}
	Catch(...)
	{
		delete[] pixels;
		delete header;
		fclose( f );
		ThrowAgain();
	}

	fclose( f );

	info->cPlanes = 1;
	info->cBitCount = 24;
}

void BitmapFile::capture( HBITMAP bitmap, PS& ps )
{
	BITMAPINFOHEADER I;
	I.cbFix = sizeof(I);
	if( !GpiQueryBitmapParameters( bitmap, &I ) )
		Throw("Could not get bitmap parameters in conversion");
	capture( I.cx, I.cy, ps );
}

void BitmapFile::capture( int cx, int cy, PS& ps )
{
	free();

	header = new BITMAPFILEHEADER2;
	info = &(header->bmp2);
	pixels = NULL;

	header->usType = BFT_BMAP;
	header->cbSize = sizeof(BITMAPFILEHEADER2);
	header->xHotspot = 0;
	header->yHotspot = 0;

	info->cbFix = sizeof(BITMAPINFOHEADER2);
	info->cx = cx;
	info->cy = cy;
	info->cPlanes = 1;
	info->cBitCount = 24;
	info->ulCompression = BCA_UNCOMP;
	info->cbImage = 0;
	info->cxResolution = 0;
	info->cyResolution = 0;
	info->cclrUsed = 0;
	info->cclrImportant = 0;
	info->usUnits = BRU_METRIC;
	info->usReserved = 0;
	info->usRecording = BRA_BOTTOMUP;
	info->usRendering = BRH_NOTHALFTONED;
	info->cSize1 = 0;
	info->cSize2 = 0;
	info->ulColorEncoding = BCE_RGB;
	info->ulIdentifier = 0;
	rowwidth = ( info->cBitCount * info->cx + 31 ) / 32 * 4;

	int buffersize = rowwidth * info->cy;
	pixels = new byte[buffersize];
	if( GpiQueryBitmapBits( ps, 0, info->cy, PBYTE(pixels), (BITMAPINFO2*)info ) < 0 )
		Throw("Could not get bits in conversion");

	/* assume no color table */
	header->offBits = sizeof(*header)-sizeof(BITMAPINFOHEADER2)+info->cbFix;
}

void BitmapFile::convert_to_image( Color transparent )
{
	// Assume that the current bitmap has some pixels that are
	// transparent, and we want to convert those pixels to black
	if( !header ) return;

	RGB trans;
	trans.bRed   = ColorToRed  ( transparent );
	trans.bGreen = ColorToGreen( transparent );
	trans.bBlue  = ColorToBlue ( transparent );

	RGB black;
	black.bRed   = 0;
	black.bGreen = 0;
	black.bBlue  = 0;

	for( int j = 0; j < info->cy; j++ )
		for( int i = 0; i < info->cx; i++ )
		{
			RGB* c = (RGB *)(pixels+rowwidth*j);
			if( c[i].bBlue == trans.bBlue && c[i].bGreen == trans.bGreen && c[i].bRed == trans.bRed )
				c[i] = black;
		}
}

void BitmapFile::convert_to_mask( Color transparent )
{
	// Assume that the current bitmap has some pixels that are
	// transparent, and we want to convert those pixels to black
	if( !header ) return;

	RGB trans;
	trans.bRed   = ColorToRed  ( transparent );
	trans.bGreen = ColorToGreen( transparent );
	trans.bBlue  = ColorToBlue ( transparent );

	RGB black;				RGB white;
	black.bRed   = 0;		white.bRed   = 255;
	black.bGreen = 0;		white.bGreen = 255;
	black.bBlue  = 0;		white.bBlue  = 255;

	for( int j = 0; j < info->cy; j++ )
		for( int i = 0; i < info->cx; i++ )
		{
			RGB* c = (RGB *)(pixels+rowwidth*j);
			if( c[i].bBlue == trans.bBlue && c[i].bGreen == trans.bGreen && c[i].bRed == trans.bRed )
				c[i] = white;
			else
				c[i] = black;
		}
}

void BitmapFile::write( const char* filename )
{
	FILE* f = fopen( filename, "wb" );
	if( !f )
		Throw("Could not open bitmap file for writing");

	if( fwrite( header, sizeof(BITMAPFILEHEADER2)-sizeof(BITMAPINFOHEADER2), 1, f ) < 1 )
		Throw("Did not write header");
	if( fwrite( info, sizeof(info->cbFix), 1, f ) < 1 )
		Throw("Did not write size field");
	if( fwrite( (char *)(info)+sizeof(info->cbFix), info->cbFix-sizeof(info->cbFix), 1, f ) < 1 )
		Throw("Did not write information structure");
	if( fwrite( pixels, sizeof(RGB), rowwidth*info->cy, f ) < rowwidth*info->cy )
		Throw("Did not write pixels");

	fclose( f );
}

#endif

//////////////////////////////////////////////////////////////////////

#if 0

Indexed256Bitmap::Indexed256Bitmap()
{
	header = NULL;
	info = NULL;
	rowwidth = 0;
	pixels = NULL;
}

Indexed256Bitmap::~Indexed256Bitmap()
{
	free();
}

void Indexed256Bitmap::free()
{
	delete[] pixels;
	delete header;
	header = NULL;
	info = NULL;
	rowwidth = 0;
	pixels = NULL;
}

void Indexed256Bitmap::write( const char* filename )
{
	FILE* f = fopen( filename, "wb" );
	if( !f )
		Throw("Could not open bitmap file for writing");

	fwrite( header, sizeof(BITMAPFILEHEADER2)-sizeof(BITMAPINFOHEADER2), 1, f );
	fwrite( info, sizeof(info->cbFix), 1, f );
	fwrite( (char *)(info)+sizeof(info->cbFix), info->cbFix-sizeof(info->cbFix), 1, f );
	fwrite( (char *)(info)+sizeof(BITMAPINFOHEADER2), sizeof(RGB2), 256, f );
	fwrite( pixels, sizeof(RGB), rowwidth*info->cy, f );

	fclose( f );
}

void Indexed256Bitmap::convert( PS& ps, BitmapFile& rgb_bitmap )
{
	free();
	if( rgb_bitmap.header == NULL ) return;

	char* rgb = rgb_bitmap.pixels;
	int rgb_rowwidth = rgb_bitmap.rowwidth;

	// Allocate a bitmap header with color table
	header = (BITMAPFILEHEADER2*)(new char[sizeof(BITMAPFILEHEADER2)+256*sizeof(RGB2)]);
	(*header) = *(rgb_bitmap.header);
	delete rgb_bitmap.header;
	info = &(header->bmp2);

	{
		// GpiQueryRealColors
		LONG* table = (long *)((char *)header + sizeof(BITMAPFILEHEADER2));
		GpiQueryRealColors( ps, 0, 0, 256, table );
	}

	rgb_bitmap.header = NULL;
	rgb_bitmap.info   = NULL;
	rgb_bitmap.pixels = NULL;
	rgb_bitmap.rowwidth = 0;

	int cols = info->cx, rows = info->cy;
	rowwidth = ( cols + 3 ) / 4 * 4;
	pixels = new byte[rowwidth*rows];

	for( int j = 0; j < rows; j++ )
	{
		RGB* rgb_row = (RGB *)(rgb+rgb_rowwidth*j);
		byte* rowdata = (byte *)(pixels+rowwidth*j);
		for( int i = 0; i < cols; i++ )
		{
			RGB& x = rgb_row[i];
			rowdata[i] = solid( ps, iRGB(x.bRed,x.bGreen,x.bBlue) );
		}
	}

	delete [] rgb;
	info->cPlanes = 1;
	info->cBitCount = 8;
	info->cclrUsed = 1;
	info->cclrImportant = 0;
}

#endif

#endif
