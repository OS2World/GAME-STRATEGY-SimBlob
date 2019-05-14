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

#ifndef MapCmd_h
#define MapCmd_h

#define MAX_CMDS 1000

struct Command
{
    enum Type { None, SetTerrain, CreateBlob, MakeRoads, EraseAll } type;
    HexCoord location;
    Terrain terrain;

    Command():type(None) {}

    static Command build( Terrain t, const HexCoord& h )
    {
        Command c;
        c.type = SetTerrain;
        c.terrain = t;
        c.location = h;
        return c;
    }

    static Command create_blob( const HexCoord& h )
    {
        Command c;
        c.type = CreateBlob;
        c.location = h;
        return c;
    }
    
    static Command make_roads()
    {
        Command c;
        c.type = MakeRoads;
        return c;
    }
    
    static Command erase_all()
    {
        Command c;
        c.type = EraseAll;
        return c;
    }
};

class CommandQueue
{
  protected:
    Mutex mutex;

    int count;
    deque<Command> c;

  public:
    CommandQueue(): count(0) {}
    
    bool empty() const { return count == 0; }
    int count_unprotected() const { return count; }
    void push( const Command& x );
    Command pop();

};

extern CommandQueue map_commands;

#endif
