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

#ifndef Subject_h
#define Subject_h

#define LOCKING 1

#include "Notion.h"

// This class implements the Subject half of the Subject-Observer pattern.
// (See _Design Patterns_ by Gamma, et. al.)  It is parameterized on the
// type of value being watched.
template<class T>
class Subject
{
    vector< Closure<bool,const T&> > dependents_;
  public:
    mutable Mutex mutex;
    T data_;

    Subject( const T& init_value )
        :data_(init_value)
    {}

    ~Subject()
    {}
    
    operator T () const
    {
        mutex.lock();
        T tmp = data_;
        mutex.unlock();
        return tmp;
    }
    const T& operator = ( const T& new_value )
    {
        mutex.lock();
        if( data_ != new_value )
        {
            data_ = new_value;
            mutex.unlock();
            update();
        }
        else
            mutex.unlock();
        return data_;
    }

    void add_dependent( Closure<bool,const T&> cl )
    { dependents_.push_back( cl ); }
    void remove_dependent( Closure<bool,const T&> cl );

    void update();
  private:
    Subject( const Subject<T>& from ); // unimplemented
};

inline void set( Subject<string>& subject, const char* new_value )
{
    subject.mutex.lock();
    if( subject.data_ != new_value ) 
    { 
        subject.data_ = new_value;
        subject.mutex.unlock();
        subject.update(); 
    }
    else
        subject.mutex.unlock();
}

template<class T>
void Subject<T>::remove_dependent( Closure<bool,const T&> cl )
{
    vector< Closure<bool,const T&> >::iterator i;
    for( i = dependents_.begin(); i != dependents_.end(); ++i )
        if( (*i)._obj == cl._obj )
            break;

    if( i != dependents_.end() )
        dependents_.erase( i );
}

template<class T>
void Subject<T>::update()
{
    for( vector< Closure<bool,const T&> >::iterator i = 
             dependents_.begin(); i != dependents_.end(); ++i )
    {
        (*i)( data_ );
    }
}

// Observer class
//   Implements the Observer half of Subject-Observer.
//   Usage:  declare a variable of type Observer<T>
//           initialize it with the subject and the closure to call
//              when something changes
//   When the variable is constructed, it registers with the subject.
//   When the variable is destructed, it deregisters with the subject.
template <class T>
class Observer
{
    Subject<T>& subject_;
    Closure<bool,const T&> closure_;
  public:
    Observer( Subject<T>& sb, Closure<bool,const T&> cl )
        :subject_(sb), closure_(cl)
    {
        subject_.add_dependent(closure_);
    }

    ~Observer()
    {
        subject_.remove_dependent(closure_);
    }

    Subject<T>& subject() { return subject_; }
    const Subject<T>& subject() const { return subject_; }
    
    operator const T& () const { return subject_.data_; }
    const T& operator = ( const T& new_value ) { return subject_ = new_value; }
};

#endif
