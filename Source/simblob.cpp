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

#include "InitCode.h"
#include "ViewWin.h"
#include "GameWin.h"
#include "MainWin.h"

#include "Blitter.h"

// main entry point
int main( int argc, const char* argv[] )
{
    // Initialize PM first
    if( Figment::Initialize() == FALSE )
        AbortProgram( (HWND)NULL, (HWND)NULL );

    // Determine what the args are
    if( argc > 1 )
    {
        if( argc > 3 )
        {
            WinMessageBox( HWND_DESKTOP, HWND_DESKTOP,
                           PSZ("BlobCity"), PSZ("Too Many Arguments"),
                           0, MB_OK | MB_NOICON );
            return 1;
        }
        
        for( int i = 1; i < argc; i++ )
        {
            Blitter::BlitType& blit_type = (i==1?Blitter::blit_map_type:
                                            Blitter::blit_controls_type);
            if( !stricmp( argv[i], "dive" ) )
                blit_type = Blitter::UseDive;
            else if( !stricmp( argv[i], "fastgpi" ) )
                blit_type = Blitter::UseGpi;
            else if( !stricmp( argv[i], "slowgpi" ) )
                blit_type = Blitter::UseWin;
            else if( !stricmp( argv[i], "fasthybrid" ) )
                blit_type = Blitter::UseGpiHybrid;
            else if( !stricmp( argv[i], "slowhybrid" ) )
                blit_type = Blitter::UseWinHybrid;
            else
            {
                WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, 
                               PSZ(argv[i]), PSZ("Invalid Argument"), 
                               0, MB_OK | MB_NOICON );
                return 1;
            }
        }
    }

#if INIT_BITMAPS != 0
    void alt_main();
    DosSetPriority( PRTYS_THREAD, PRTYC_IDLETIME, +31, 0 );
    alt_main();
    WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, 
                   PSZ("Bitmaps Generated"), PSZ("Notification"),
                   0, MB_OK | MB_NOICON );
#else
    LoadBitmaps();
    SimBlobWindow *W = new SimBlobWindow;
    W->setup_window( NULL );
    
    Figment::MessageLoop();
    SimBlobWindow::program_running = false;
    DosSleep(300);
    
    delete W;

    SimBlobWindow::program_running = false;
    Figment::Terminate();
#endif

    return 0;
}

void throw_error( const char* text, const char* where )
{
    static bool thrown = false;
    FILE* f = fopen("simblob.trp","at");
    fprintf(f,"Error %s @ %s\n",text,where);
    fclose(f);
    
    if( !thrown )
    {
        thrown = true;
        
        WinMessageBox( HWND_DESKTOP, HWND_DESKTOP, 
                       PSZ(where), PSZ(text), 0, MB_OK | MB_NOICON );
    }
    // exit(-1);
}

#include <stdio.h>
void Log( const char* t1, const char* t2 )
{
    FILE* f = fopen("simblob.log","at");
    fprintf(f,"[%s] %s\n",t1,t2);
    fclose(f);
}

