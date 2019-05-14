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

#ifndef Blitter_h
#define Blitter_h

class ColorPalette;

class Blitter
{
  protected:
	PixelBuffer* pb;
	virtual int draw_all( PS& ps, int x, int y );
	virtual int draw_rect( PS& ps, int x, int y, const Rect& rc );
	virtual int draw_rgn( PS& ps, int x, int y, const DamageArea& area );

  public:
	Blitter();
	virtual ~Blitter();

	// Draw to a PS
	int draw( PS& ps, int x, int y )
	{ return draw_all( ps, x, y ); }
	int draw( PS& ps, int x, int y, const Rect& rc )
	{ return draw_rect( ps, x, y, rc ); }
	int draw( PS& ps, int x, int y, const DamageArea& area )
	{ return draw_rgn( ps, x, y, area ); }

	// Visible region notification
	virtual void vrgn_enable( HWND client, HWND frame, const Region& rgn );
	virtual void vrgn_disable();

	// Palette
	virtual void palette_changed( ColorPalette* palette );

	// Prepare to use buffer as the image buffer
	virtual void acquire( PixelBuffer* pb );

	// Stop using buffer as the image buffer
	virtual void release();

	//////////////////////////////////////////////////
	// Creation
	enum BlitType { UseGpi, UseWin, UseDive, UseGpiHybrid, UseWinHybrid };
	static BlitType blit_controls_type, blit_map_type;
	static Blitter* create_controls();
	static Blitter* create_map();
};

#endif
