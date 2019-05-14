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
#include "SimBlob.h"                    // Resource file symbolics

#include "Map.h"
#include "View.h"
#include "MsgWin.h"

#include "ViewWin.h"
#include "GameWin.h"
#include "MainWin.h"

//////////////////////////////////////////////////////////////////////

bool SimBlobWindow::program_running = true;

SimBlobWindow::SimBlobWindow()
    :client_(NULL)
{
}

SimBlobWindow::~SimBlobWindow()
{
    if( client_ )
    {
        delete client_;
        client_ = NULL;
    }
}

void SimBlobWindow::setup_window( Window* )
{
    FRAMECDATA fcdata;

    fcdata.cb            = sizeof( FRAMECDATA );
    fcdata.flCreateFlags = FCF_TASKLIST | FCF_MENU | FCF_AUTOICON 
        | FCF_SIZEBORDER | FCF_ICON | FCF_ACCELTABLE;
    fcdata.hmodResources = 0; 
    fcdata.idResources   = ID_WINDOW;

    create_window( HWND_DESKTOP, WC_FRAME, 0, ID_WINDOW, NULLHANDLE, &fcdata );

    fcdata.flCreateFlags = FCF_SYSMENU | FCF_TITLEBAR | FCF_MINMAX;
    // | FCF_VERTSCROLL | FCF_HORZSCROLL;
    WinCreateFrameControls( window_, &fcdata, NULL );

    DosSleep(32);
    client_ = new GameWindow;
    client() -> setup_window( this );

    WinSendMsg( window_, WM_UPDATEFRAME, 
                MPFROMLONG( fcdata.flCreateFlags ), NULL );

    WinSendMsg( WinWindowFromID( window_, FID_TITLEBAR ),
                 TBM_SETHILITE, MPFROMSHORT( TRUE ), NULL );

    // Set title bar of window
    titlebar_text( "SimBlob" );

    if( !WinRestoreWindowPos( PSZ(AppName), PSZ(AppWinPos), handle() ) )
    {
        int size_x = 600;
        int size_y = 440;
        Rect boundary( 10, 20, 10+size_x, 20+size_y );
        
        WinCalcFrameRect( handle(), &boundary, false );
        
        WinSetWindowPos( handle(), HWND_TOP,
                         boundary.xLeft, boundary.yBottom,
                         boundary.width(), boundary.height(),
                         SWP_SIZE | SWP_MOVE | SWP_ACTIVATE | SWP_SHOW
                         );
    }
    else
        WinSetWindowPos( handle(), HWND_TOP, 0, 0, 0, 0,
                         SWP_SHOW | SWP_ACTIVATE );

    DosSleep(32);
    client_->init_thread_id = Figment::begin_thread( closure( client(), &GameWindow::initialization_thread ) );
}

GameWindow* SimBlobWindow::client() { return client_; }

