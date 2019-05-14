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

#ifndef MsgWin_h
#define MsgWin_h

// This was going to be a pop up message window but I never finished
class MessageWindow : public FrameWindow
{
  protected:
	virtual void create_client_window();
	
  public:
	MessageWindow( FrameWindow* parent_window, const char* text );
	virtual ~MessageWindow();
	virtual void setup_window( Window* parent );

	virtual void Paint( PS& ps, const Rect& rc );
	virtual MRESULT WndProc( HWND hWnd, ULONG iMessage, MPARAM mParam1, MPARAM mParam2 );
};

#endif

