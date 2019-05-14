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

#ifndef Closure_h
#define Closure_h

#ifndef __BORLANDC__

// This code is incredibly ugly and non-portable.  Sorry.  I wanted to
// have a *value* type that served as a closure, and the portable ways
// of doing this made it an *object* type (heap allocated), which is
// a pain to use because you have to free it.  I suppose the alternative
// is to have a value wrapper around the object type.

struct ClosureTemp
{
};

template<class T,class R,class A>
struct ClosureRep
{
    R (T::*_fun)(A);
};

template<class R,class A>
struct Closure
{
    void* _obj;
    R (ClosureTemp::*_fun)(A);
    R (*_run)(void*,R (ClosureTemp::*)(A),A);

    R operator () ( A arg ) { return (*_run)(_obj,_fun,arg); }
};

template<class T,class R,class A>
struct ClMemberFunction
{
    static R Run( void* _obj, R (ClosureTemp::*_fun)(A), A arg )
    {
        T* obj = static_cast<T*>(_obj);

        ClosureRep<ClosureTemp,R,A> temp;
        temp._fun = _fun;
        R (T::*fun)(A) = (reinterpret_cast<ClosureRep<T,R,A>&>(temp))._fun;
        return (obj->*fun)(arg);
    }
};

template<class T,class R,class A>
Closure<R,A> closure( T* obj, R (T::*fun)(A) )
{
    Closure<R,A> cl;
    cl._obj = static_cast<void*>(obj);
    ClosureRep<T,R,A> temp;
    temp._fun = fun;
    cl._fun = (reinterpret_cast<ClosureRep<ClosureTemp,R,A>&>(temp))._fun;
    cl._run = ClMemberFunction<T,R,A>::Run;
    return cl;
}

#else

template<class R,class A>
struct Closure
{
    void* _obj;
    R (Closure<R,A>::*_fun)(A);
    R (*_run)(void*,R (Closure<R,A>::*)(A),A);

    R operator () ( A arg ) { return (*_run)(_obj,_fun,arg); }
};

template<class T,class R,class A>
struct ClMemberFunction
{
    static R Run( void* _obj, R (Closure<R,A>::*_fun)(A), A arg )
    {
        T* obj = (T*) (_obj);
#ifdef __BORLANDC__
        R (T::*fun)(A) = reinterpret_cast< R (T::*)(A) >( _fun );
#else
        union { R (Closure<R,A>::*_fun)(A); R (T::*fun)(A); } temp;
        temp._fun = _fun;
        R (T::*fun)(A) = temp.fun;
#endif
        return (obj->*fun)(arg);
    }
};

template<class T,class R,class A>
Closure<R,A> closure( T* obj, R (T::*fun)(A) )
{
    Closure<R,A> cl;
    cl._obj = (void*)(obj);
#ifdef __BORLANDC__
    cl._fun = reinterpret_cast< R (Closure<R,A>::*)(A) >( fun );
#else
    union { R (Closure<R,A>::*_fun)(A); R (T::*fun)(A); } temp;
    temp.fun = fun;
    cl._fun = temp._fun;
#endif
    cl._run = ClMemberFunction<T,R,A>::Run;
    return cl;
}

#endif

#endif
