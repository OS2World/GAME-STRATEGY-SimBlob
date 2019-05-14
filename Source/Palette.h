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

#ifndef Palette_h
#define Palette_h

class ColorPalette
{
  private:
	PS& ps;
	HPAL pal;

  public:
	ColorPalette( PS& ps_ );
	~ColorPalette();

	// For the main window only
	void create();
	bool realize( HWND hwnd );
	void destroy();

	// For all other PSes
	void select( PS& ps );
	void unselect( PS& ps );
};

#endif
