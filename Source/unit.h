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

#ifndef Unit_h
#define Unit_h

class Map;

const int Unit_size_x = 16;
const int Unit_size_y = 16;

const int DO_NOTHING = -2;
struct Job
{
    HexCoord location;
    int build;                  // -1 if there's no job, DO_NOTHING for no-op
    int blob;                   // -1 if none
    Job() : location(0,0), build(-1), blob(-1) {}
};

extern vector<Job> jobs;

const int DEAD_ID = -1;

// Represent a blob in the game world
struct Unit
{
    enum Type { Idle, Builder, Firefighter } type;
                
    int id;                     // -1 if dead
    bool dead() { return id == DEAD_ID; }
    short index;                // index in the units vector

    void reincarnate( Map*, Type t, HexCoord h );   // bring this unit to life
    void die( Map* );           // kill this unit
    
    HexCoord final_dest;
    short num_attempts;         // how many times we tried to get to dest
    short agitated;             // how agitated this unit is (0 == normal)
    vector<HexCoord> path;      // where to walk, in reverse order
    enum { MaxJobs = 6 };
    short jobs[MaxJobs];        // jobs assigned to this blob

    HexCoord home;  // where it was born, if it came from a watchtower
    HexCoord loc;   // hex location
    HexCoord prev_loc;

    int wait_steps; // number of steps we need to wait for obstacles to move
    Point source;   // starting point
    Point dest;     // next point to visit
    int j;          // where we are along the current line segment
    int nsteps;     // how many steps are there in this line segment?

    bool moving()   // is this unit moving?
    { return path.size() > 0; }

    bool busy();     // is this unit doing anything?
    bool too_busy(); // is this unit too busy to take a new job?
    void accept_job( int i ); // take the job at position i
    void perform_jobs( Map* map );    // perform any appropriate jobs
    void perform_movement( Map* map );
    
    int pos( Map* map );    // position in the units array
    Point gridloc();        // the current x,y position
    HexCoord hexloc();      // the current hex position
    HexCoord hexloc_left(); // the two hexes that this unit is between
    HexCoord hexloc_right();

    void handle_blocked_movement( Map* map );  // what to do if we can't move
    void step( Map* map );  // move one step
    void stop();    // stop movement
    void update_path( Map* map, int count );
    void set_dest( Map* map, HexCoord new_dest );   // start moving towards a new dest

  public:
    Unit( int index_ );
    ~Unit() {}

    static Unit* make( Map* map, Type t, const HexCoord& location );
    static void destroy( Unit* unit, Map* map );
};

struct WatchtowerFire
{
    HexCoord location;

  public:
    WatchtowerFire( HexCoord h = HexCoord() );
    ~WatchtowerFire() {}

    friend bool operator == ( const WatchtowerFire& a,
                              const WatchtowerFire& b )
    { return a.location == b.location; }
};

inline void destroy( Unit** ) {}
inline void destroy( WatchtowerFire** ) {}

#if 0
const int NUMCARS = 6;
const int CARDIST = 8;

struct Train
{
    vector<HexCoord> path;      // in reverse order

    double speed, location;
    
    Train();
    void move( Map* map );
    void begin_moving( Map* map, const HexCoord& A, const HexCoord& B );

    int num_cars() { return path.size()==0? 0 : NUMCARS; }
    HexCoord car_loc(int i);
};

extern Train train;
#endif

#endif

