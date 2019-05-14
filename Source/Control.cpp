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

#include "Map.h"
#include "Map_const.h"
#include "View.h"
#include "Menu.h"

#include "ViewWin.h"
#include "GameWin.h"
#include "MainWin.h"
#include "StatusBar.h"

//////////////////////////////////////////////////////////////////////
int hex_style = 0;
int money = 12000;

//////////////////////////////////////////////////////////////////////
EventSem sim_painted;
double tps = 0.0, fps = 0.0, pps = 0.0;
int pixcount = 0;
int drwcount = 0;

int Map::simulation_thread(int)
{
    int simcount = 0;
    clock_t time0 = clock();
    clock_t last_sim = clock();

    int phase = 0;
    for(;;)
    {
        if( !SimBlobWindow::program_running ) break;

        process_commands();
        
        last_sim = clock();
        simulate();

        // Keep a count of how many simulation ticks we do
        simcount++;
        if( clock() - time0 > CLK_TCK * 3 && simcount > 13 )
        {
            clock_t time1 = clock();
            double scale = double(CLK_TCK) / ( time1 - time0 );

            if( time1 > time0 && !isnan(scale) )
            {
                double new_tps = simcount * scale;
                tps = new_tps * 0.7 + tps * 0.3;
                simcount = 0;
                
                double new_fps = drwcount * scale;
                fps = new_fps * 0.7 + fps * 0.3;
                drwcount = 0;
                
                double new_pps = pixcount * scale;
                pps = new_pps * 0.7 + pps * 0.3;
                pixcount = 0;
                
                time0 = time1;
            }
        }

        if( !SimBlobWindow::program_running ) break;

        int phasing = 1 + (game_speed / 20);
        if( game_speed == 500 ) phasing = 1;
        if( game_speed == 1000 ) phasing = 1000;
        if( (phase++)%phasing == 0 )
        {
            sim_painted.post();
            if( phasing > 10 ) phasing = 10;
            int w = 1000*phasing/game_speed
                - int(1000*(clock()-last_sim)/CLK_TCK)
                - 20;     // Extra fudge for DosSleep
            if( w < 10 ) w = 10;
            if( w > 1000 ) w = 1000;
            DosSleep( w );
        }
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////
extern Subject<string> current_tool_help, status_info;
extern Subject<int> tb_index;

// This should go through the list of views and update each one
int GameWindow::update_thread(int)
{
    HAB hab = WinInitialize( 0 );
    HMQ hmq = WinCreateMsgQueue( hab, 0 );
    WinCancelShutdown( hmq, true ); // don't worry about shutting down thread
    
    while( !bitmaps_initialized && SimBlobWindow::program_running )
        DosSleep(100); // wait until the map is ready

    for(;;)
    {
        if( !SimBlobWindow::program_running ) break;

        if( map->valid( map->volcano_ ) && map->volcano_time_ % 4 == 0 )
            world_map_->update_now();
        else
            world_map_->update();

        if( tb_index == -2 )
        {
            extern Point last_identify;
            char s[256];
            HitResult hr = viewwin_->view->hit(last_identify);
            if( hr.type == HitResult::tUnit )
                sprintf( s, "%s #%d %s",
                         hr.unit->type==Unit::Builder?"Builder":"Firefighter",
                         hr.unit->id,
                         hr.unit->moving()?" (moving)":"" );
            else if( hr.type == HitResult::tHexagon )
            {
                HexCoord h(hr.location);
                Terrain t = map->terrain(h);
                if( t == Clear )
                    sprintf( s, "[Clear]" );
                else if( t == Farm )
                    sprintf( s, "[Farm] prod:%d", map->food_[h] );
                else if( t == Houses )
                    sprintf( s, "[Houses] pop:%d", map->residents(h) );
                else if( t == Market )
                    sprintf( s, "[Market] labor:%d food:%d",
                             map->labor_[h], map->food_[h] );
                else if( t == Road || t == Bridge )
                    sprintf( s, "[%s] blobs:%d cargo:%d",
                             (t==Road)?"Road":"Bridge",
                             map->labor_[h], map->food_[h] );
                else if( t == Trees )
                    sprintf( s, "[Forest] age:%dyr",
                             map->extra_[h]*500/TICKS_PER_DAY/DAYS_PER_YEAR );
                else if( t == Wall )
                    sprintf( s, "[Wall]" );
                else if( t == Gate )
                    sprintf( s, "[Gate]" );
                else if( t == Lava || t == Scorched || t == Fire )
                    sprintf( s, "[Heat:%d]", map->heat_[h] );
                else if( t == WatchFire )
                    sprintf( s, "[Watchtower]" );
                else
                    sprintf( s, "[Map]" );

                if( map->water(h) > 0 )
                {
                    char* s_append = s + strlen(s);
                    double w = 2.5 * map->water(h) / WATER_MULT;
                    sprintf( s_append, " water:%1.2fm", w );
                }
                // Always append the elevation?
                {
                    char* s_append = s + strlen(s);
                    double a = 2.5 * map->altitude(h);
                    sprintf( s_append, " elev:%1.1fm", a );
                }
            }
            else
                sprintf( s, "None" );
            set( status_info, s );
            infoarea_->update();
        }
        else if( tb_index == -1 )
            set( status_info, "RMB Drag = Move" );
        else
            set( status_info, "LMB Click/Drag = Build" );

        // Display the money
        static int _money = 0;
        {
            char s[256];
            _money = money;
            
            extern Subject<string> money_info;
            sprintf( s, "%d c", _money );

            set(money_info,s);
            statusbar_->update();
        }

        // Display the speed in ticks per second
        static double _tps = 0.0;
        static double _fps = 0.0;
        static double _pps = 0.0;
        if( _tps != tps || _fps != fps || _pps != pps )
        {
            char s[256];
            _tps = tps;
            _fps = fps;
            _pps = pps;

            extern Subject<string> date_info, 
                info5, info6, info7, info8, info9, info10,
                speed_info_ticks, speed_info_frames, speed_info_pixels;
            
            sprintf( s, "Date: %d %s %d",
                     map->day(), map->monthname(), map->year() );
            set(date_info,s);
            
            sprintf( s, "%3.2f ticks/sec", tps );
            set(speed_info_ticks,s);
            sprintf( s, "%3.2f", fps );
            set(speed_info_frames,s);
            sprintf( s, "%4.3f", pps*1e-6 );
            set(speed_info_pixels,s);

            sprintf( s, "%d", map->total_labor );
            set(info5,s);
            int w = map->total_working;
            if( w > map->total_labor ) w = map->total_labor;
            if( w > map->total_jobs ) w = map->total_jobs;
            int t1 = 100*w/(1+map->total_labor);
            if( t1 > 100 ) t1 = 100;
            sprintf( s, "%d%%", t1 );
            set(info6,s);
            
            sprintf( s, "%d", map->total_jobs );
            set(info7,s);
            int t2 = 100*w/(1+map->total_jobs);
            if( t2 > 100 ) t2 = 100;
            sprintf( s, "%d%%", t2 );
            set(info8,s);

            sprintf( s, "%d", map->total_working );
            set(info9,s);
            sprintf( s, "%d", map->total_fed );
            set(info10,s);

            infoarea_->update();
            statusbar_->update();
            buttonbar_->update();
        }

        viewwin_->request_full_update();

        if( !SimBlobWindow::program_running ) break;
        sim_painted.reset();
        sim_painted.wait( 200 );
    }

    WinDestroyMsgQueue( hmq );
    WinTerminate( hab );
    view_thread_id = 0;
    return 0;
}
