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

#ifndef OLA_CLOSURE_H
#define OLA_CLOSURE_H

namespace ola {


class BaseClosure {
  public:
    virtual ~BaseClosure() {}
    virtual int Run() = 0;
    virtual int DoRun() = 0;
};


/*
 * Closure, this is a closure that can be called multiple times
 */
class Closure: public BaseClosure {
  public:
    virtual ~Closure() {}
    int Run() { return DoRun(); }
};


/*
 * A single use closure, this deletes itself after it's run.
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
 * A function closure, with no arguments
 */
template <typename Parent>
class FunctionClosure: public Parent {
  public:
    typedef int (*Callback)();

    /*
     * @param callback the function to call
     */
    FunctionClosure(Callback callback):
      Parent(),
      m_callback(callback) {}
    int DoRun() { return m_callback(); }

  private:
    Callback m_callback;
};


/*
 * Create a new single use function closure.
 */
inline SingleUseClosure* NewSingleClosure(int (*callback)()) {
  return new FunctionClosure<SingleUseClosure>(callback);
}


/*
 * Create a new function closure.
 */
inline Closure* NewClosure(int (*callback)()) {
  return new FunctionClosure<Closure>(callback);
}


/*
 * An method closure with no arguments
 */
template <typename Class, typename Parent>
class MethodClosure: public Parent {
  public:
    typedef int (Class::*Callback)();

    /*
     * @param object the object to use in the method call
     * @param callback the method to call
     */
    MethodClosure(Class *object,
                  Callback callback):
      Parent(),
      m_object(object),
      m_callback(callback) {}
    int DoRun() { return (m_object->*m_callback)(); }

  private:
    Class *m_object;
    Callback m_callback;
};


/*
 * Create a new single use method closure.
 */
template <typename Class>
inline SingleUseClosure* NewSingleClosure(Class* object,
                                          int (Class::*method)()) {
  return new MethodClosure<Class, SingleUseClosure>(object, method);
}


/*
 * Create a new method closure.
 */
template <typename Class>
inline Closure* NewClosure(Class* object,
                           int (Class::*method)()) {
  return new MethodClosure<Class, Closure>(object, method);
}


/*
 * A method closure that takes one argument
 */
template <typename Class, typename Parent, typename Arg>
class MethodArgClosure: public Parent {
  public:
    typedef int (Class::*Callback)(Arg arg);

    /*
     * @param object the object to use in the method call
     * @param handle the method to call
     * @param arg the argument to pass to the method
     */
    MethodArgClosure(Class *object,
                     Callback callback,
                     Arg arg):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_arg(arg) {}
    int DoRun() { return (m_object->*m_callback)(m_arg); }

  private:
    Class *m_object;
    Callback m_callback;
    Arg m_arg;
};


/*
 * Create a new single use one-arg method closure
 */
template <typename Class, typename Arg>
inline SingleUseClosure* NewSingleClosure(Class* object,
                                          int (Class::*method)(Arg arg),
                                          Arg arg) {
  return new MethodArgClosure<Class, SingleUseClosure, Arg>(object, method,
                                                            arg);
}


/*
 * Create a new one-arg method closure
 */
template <typename Class, typename Arg>
inline Closure* NewClosure(Class* object,
                           int (Class::*method)(Arg arg),
                           Arg arg) {
  return new MethodArgClosure<Class, Closure, Arg>(object, method, arg);
}


/*
 * A method closure that takes two arguments
 */
template <typename Class, typename Parent, typename Arg, typename Arg2>
class MethodTwoArgClosure: public Parent {
  public:
    typedef int (Class::*Callback)(Arg arg, Arg2 arg2);

    /*
     * @param object the object to use in the method call
     * @param method the method to call
     * @param arg the argument to pass to the method
     * @param arg2 the second argument to pass to the method
     */
    MethodTwoArgClosure(Class *object,
                        Callback callback,
                        Arg arg,
                        Arg2 arg2):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_arg(arg),
      m_arg2(arg2) {}
    int DoRun() { return (m_object->*m_callback)(m_arg, m_arg2); }

  private:
    Class *m_object;
    Callback m_callback;
    Arg m_arg;
    Arg2 m_arg2;
};


/*
 * Create a new single use two-arg method closure
 */
template <typename Class, typename Arg, typename Arg2>
inline SingleUseClosure* NewSingleClosure(
    Class* object,
    int (Class::*method)(Arg arg, Arg2 arg2),
    Arg arg,
    Arg2 arg2) {
  return new MethodTwoArgClosure<Class, SingleUseClosure, Arg, Arg2>
                 (object, method, arg, arg2);
}


/*
 * Create a new two-arg method closure
 */
template <typename Class, typename Arg, typename Arg2>
inline Closure* NewClosure(Class* object,
                           int (Class::*method)(Arg arg, Arg2 arg2),
                           Arg arg, Arg2 arg2) {
  return new MethodTwoArgClosure<Class, Closure, Arg, Arg2>
                 (object, method, arg, arg2);
}


} //ola
#endif
