/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Closure.h
 * Closure classes.
 * Copyright (C) 2005-2009 Simon Newton
 */

#ifndef LLA_CLOSURE_H
#define LLA_CLOSURE_H

namespace lla {


class BaseClosure {
  public:
    virtual ~BaseClosure() {}
    virtual int Run() = 0;
    virtual int DoRun() = 0;
};


/*
 * Base Closure
 */
class Closure: public BaseClosure {
  public:
    virtual ~Closure() {}
    int Run() { return DoRun(); }
};


/*
 * A single use closure
 */
class SingleUseClosure: public BaseClosure {
  public:
    virtual ~SingleUseClosure() {}
    int Run() {
      int ret = DoRun();
      delete this;
      return ret;
    }
};

/*
 * A templatized Closure, with no arguments
 */
template <typename Class, typename Parent>
class ObjectClosure: public Parent {
  public:
    typedef int (Class::*RequestHandler)();

    /*
     * @param object the object to use in the method call
     * @param handler the method to call
     */
    ObjectClosure(Class *object,
                  RequestHandler handler):
      Parent(),
      m_object(object),
      m_handler(handler) {}
    int DoRun() { return (m_object->*m_handler)(); }

  private:
    Class *m_object;
    RequestHandler m_handler;
};


/*
 * Create a new single use Closure. See above.
 */
template <typename Class>
inline SingleUseClosure* NewSingleClosure(Class* object,
                                          int (Class::*method)()) {
  return new ObjectClosure<Class, SingleUseClosure>(object, method);
}


/*
 * Create a new Closure. See above.
 */
template <typename Class>
inline Closure* NewClosure(Class* object,
                           int (Class::*method)()) {
  return new ObjectClosure<Class, Closure>(object, method);
}


/*
 * A templatized Closure, that takes one argument
 */
template <typename Class, typename Parent, typename Arg>
class ObjectArgClosure: public Parent {
  public:
    typedef int (Class::*RequestHandler)(Arg arg);

    /*
     * @param object the object to use in the method call
     * @param handle the method to call
     * @param arg the argument to pass to the method
     */
    ObjectArgClosure(Class *object,
                     RequestHandler handler,
                     Arg arg):
      Parent(),
      m_object(object),
      m_handler(handler),
      m_arg(arg) {}
    int DoRun() { return (m_object->*m_handler)(m_arg); }

  private:
    Class *m_object;
    RequestHandler m_handler;
    Arg m_arg;
};


/*
 * Create a new single use Closure
 */
template <typename Class, typename Arg>
inline SingleUseClosure* NewSingleClosure(Class* object,
                                          int (Class::*method)(Arg arg),
                                          Arg arg) {
  return new ObjectArgClosure<Class, SingleUseClosure, Arg>(object, method,
                                                            arg);
}


/*
 * Create a new Closure
 */
template <typename Class, typename Arg>
inline Closure* NewClosure(Class* object,
                           int (Class::*method)(Arg arg),
                           Arg arg) {
  return new ObjectArgClosure<Class, Closure, Arg>(object, method, arg);
}


/*
 * A templatized Closure, that takes two arguments
 */
template <typename Class, typename Parent, typename Arg, typename Arg2>
class ObjectTwoArgClosure: public Parent {
  public:
    typedef int (Class::*RequestHandler)(Arg arg, Arg2 arg2);

    /*
     * @param object the object to use in the method call
     * @param method the method to call
     * @param arg the argument to pass to the method
     * @param arg2 the second argument to pass to the method
     */
    ObjectTwoArgClosure(Class *object,
                        RequestHandler handler,
                        Arg arg,
                        Arg2 arg2):
      Parent(),
      m_object(object),
      m_handler(handler),
      m_arg(arg),
      m_arg2(arg2) {}
    int DoRun() { return (m_object->*m_handler)(m_arg, m_arg2); }

  private:
    Class *m_object;
    RequestHandler m_handler;
    Arg m_arg;
    Arg2 m_arg2;
};


/*
 * Create a new single use Closure
 */
template <typename Class, typename Arg, typename Arg2>
inline SingleUseClosure* NewSingleClosure(
    Class* object,
    int (Class::*method)(Arg arg, Arg2 arg2),
    Arg arg,
    Arg2 arg2) {
  return new ObjectTwoArgClosure<Class, SingleUseClosure, Arg, Arg2>
                 (object, method, arg, arg2);
}


/*
 * Create a new Closure
 */
template <typename Class, typename Arg, typename Arg2>
inline Closure* NewClosure(Class* object,
                           int (Class::*method)(Arg arg, Arg2 arg2),
                           Arg arg, Arg2 arg2) {
  return new ObjectTwoArgClosure<Class, Closure, Arg, Arg2>
                 (object, method, arg, arg2);
}


} //lla
#endif
