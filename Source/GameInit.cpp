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
#include "Sprites.h"

#include "Map.h"
#include "View.h"

#include "ViewWin.h"
#include "GameWin.h"
#include "MainWin.h"

#include "StatusBar.h"

//////////////////////////////////////////////////////////////////////
// Initialize the map terrain in a background thread

static struct InitializationProgress
{   
    vector<int> clocks;
    unsigned int i;

    void record_progress();
    int current_progress();
    void increment() { i++; }

    InitializationProgress(): i(0) {}
} *record_progress = NULL, *old_progress = NULL;

void InitializationProgress::record_progress()
{
    clock_t t = clock();
    clocks.push_back( t );
}

int InitializationProgress::current_progress() 
{
    // Interpolation!
    if( clocks.size() == 0 ) return 0;
    
    int begin = clocks[0];
    int end = clocks[clocks.size()-1];
    int where = clocks[ min(int(clocks.size()-1), int(i) ) ];

    if( where > end ) return 0;
    return 100*(where-begin)/(end-begin);
}

bool GameWindow::initialization_message( const char* text )
{
    // The map initialization system will give us some text to display
    extern Subject<string> status_info;
    
    if( record_progress )
        record_progress->record_progress();

    char s[256];
    strcpy( s, text );
    if( old_progress )
    {
        sprintf(s,"%d%%: %s", old_progress->current_progress(), text);
        old_progress->increment();
    }

    status_info = s;
    statusbar_->update();
    world_map_->update();
    DosSleep(32);
    return !SimBlobWindow::program_running;
}

int GameWindow::initialization_thread(int)
{
    HAB hab = WinInitialize( 0 );
    HMQ hmq = WinCreateMsgQueue( hab, 0 );

    // Lower the priority to idle time, but not too low
    DosSetPriority( PRTYS_THREAD, PRTYC_IDLETIME, +31, 0 );

    // Record initialization to a file
    // We maintain a map from initialization stages to times,
    // and the next time through we can use this information to
    // guess what percentage of the initialization is complete.
    // Adaptive progress meters are needed here because the initialization
    // steps are controlled by the user (in InitMap.txt), and it can
    // vary.  If the user changes the file, the progress meter will
    // be wrong once, but the next time it will be fairly accurate.
    {
        InitializationProgress progress1, progress2;
        FILE* f = fopen("Data/Progress.dat","rt");
        if( f )
        {
            for(;;)
            {
                int i;
                if( fscanf(f,"%d",&i) < 1 )
                    break;
                progress1.clocks.push_back(i);
            }
            fclose(f);
            old_progress = &progress1;
        }

        record_progress = &progress2;
        frame()->window_text( "Creating a world .." );
        map->initialize( closure(this,&GameWindow::initialization_message) );
        frame()->window_text( "BlobCity   -   "
#include "build.h"
                          );
        record_progress = NULL;
        old_progress = NULL;

        f = fopen("Data/Progress.dat","wt");
        if( f )
        {
            for( vector<int>::iterator i = progress2.clocks.begin();
                 i != progress2.clocks.end(); ++i )
            {
                fprintf(f,"%d ",*i);
            }
            fclose(f);
        }
    }
    
    bitmaps_initialized = true;
    
    simulate_thread_id =
        Figment::begin_thread( closure( map, &Map::simulation_thread ) );
    DosSleep( 100 );
    if( viewwin_ )
        viewwin_->painter_thread_id =
            Figment::begin_thread( closure( viewwin_,
                                            &ViewWindow::painter_thread ) );
    DosSleep( 100 );
    view_thread_id =
        Figment::begin_thread( closure( this, &GameWindow::update_thread ) );
    
    DosSleep( 200 );
    
    viewwin_->request_full_paint();

    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );

    init_thread_id = 0;
    return 0;
}

