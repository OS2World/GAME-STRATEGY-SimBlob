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

#include "Figment.h"
#include "Menu.h"

bool ToggleMenuItem::notify( const bool& new_value )
{
    if( !window_ ) return false;
    HWND menu = WinWindowFromID( window_->handle(), FID_MENU );
    WinCheckMenuItem( menu, id_, new_value );

    return true;
}

void ToggleMenuItem::activate()
{
    bool old_value = value_;
    value_ = !old_value;
}

bool ValueMenuItem::notify( const int& new_value )
{
    if( !window_ ) return false;
    HWND menu = WinWindowFromID( window_->handle(), FID_MENU );
    bool val = (new_value == known_value_);
    WinCheckMenuItem( menu, id_, val );

    return true;
}

void ValueMenuItem::activate()
{
    value_ = known_value_;
}

void CommandMenuItem::activate()
{
    command_(id_);
}

void Menu::toggle( int id, Subject<bool>& val )
{
    items_.push_back( new ToggleMenuItem( window, id, val ) );
}

void Menu::value( int id, Subject<int>& val, int set_to )
{
    items_.push_back( new ValueMenuItem( window, id, val, set_to ) );
}

void Menu::command( int id, Closure<bool,int> cmd )
{
    items_.push_back( new CommandMenuItem( window, id, cmd ) );
}

bool Menu::handle( int id )
{
    bool handled = false;
    for( vector<MenuItem*>::iterator i = items_.begin();
         i != items_.end(); i++ )
    {
        MenuItem* m = *i;
        if( m->id() == id )
        {
            m->activate();
            handled = true;
        }
    }
    return handled;
}

Menu::~Menu()
{
    for( vector<MenuItem*>::iterator i = items_.begin();
         i != items_.end(); i++ )
        delete (*i);
}

