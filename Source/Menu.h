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

#ifndef Menu_h
#define Menu_h

// OS/2 and Windows have a message model, where the appliation receives
// events when something occurs.  I instead want a callback model, where
// an object's method is called when something occurred.  This module
// handles the translation from the message model to the callback model
// (for menu items only).

class MenuItem
{
  protected:
    Window* window_;
    int id_;
  public:
    MenuItem( Window* w, int i ): window_(w), id_(i) {}
    virtual ~MenuItem() {}
    
    inline int id() { return id_; }
    virtual void activate() = 0;
};

// inline void destroy( MenuItem** ) {}
class Menu
{
  private:
    vector<MenuItem*> items_;
    Window* window;
  public:
    Menu( Window* w = NULL ): window(w) {}
    ~Menu();

    void set_window( Window* w ) { window = w; }
    
    void toggle( int id, Subject<bool>& val );
    void value( int id, Subject<int>& val, int set_to );
    void command( int id, Closure<bool,int> cmd );
    
    bool handle( int id );
};

class ToggleMenuItem: public MenuItem
{
  protected:
    Observer<bool> value_;
  public:
    ToggleMenuItem( Window* w, int i, Subject<bool>& v )
        :MenuItem(w,i), value_( v, closure( this, &ToggleMenuItem::notify ) )
    {
        notify(value_);
    }

    ~ToggleMenuItem() {}

    bool notify( const bool& new_value );
    virtual void activate();
};

class ValueMenuItem: public MenuItem
{
  protected:
    Observer<int> value_;
    int known_value_;
    
  public:
    ValueMenuItem( Window* w, int i, Subject<int>& v, int kv )
        :MenuItem(w,i), value_( v, closure( this, &ValueMenuItem::notify ) ),
         known_value_(kv)
    {
        notify(value_);
    }

    ~ValueMenuItem() {}

    bool notify( const int& new_value );
    virtual void activate();
};

class CommandMenuItem: public MenuItem
{
  protected:
    Closure<bool,int> command_;

  public:
    CommandMenuItem( Window* w, int i, Closure<bool,int> c )
        :MenuItem(w,i), command_(c)
    {
    }

    virtual void activate();
};

#endif
