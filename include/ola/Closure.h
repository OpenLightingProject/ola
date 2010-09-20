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

#ifndef INCLUDE_OLA_CLOSURE_H_
#define INCLUDE_OLA_CLOSURE_H_

#include <ola/Callback.h>

namespace ola {


template <typename ReturnType>
class BaseClosure {
  public:
    virtual ~BaseClosure() {}
    virtual ReturnType Run() = 0;
    virtual ReturnType DoRun() = 0;
};


/*
 * Closure, this is a closure that can be called multiple times
 */
template <typename ReturnType>
class Closure: public BaseClosure<ReturnType> {
  public:
    virtual ~Closure() {}
    ReturnType Run() { return this->DoRun(); }
};


/*
 * A single use closure, this deletes itself after it's run.
 */
template <typename ReturnType>
class SingleUseClosure: public BaseClosure<ReturnType> {
  public:
    virtual ~SingleUseClosure() {}
    ReturnType Run() {
      ReturnType ret = this->DoRun();
      delete this;
      return ret;
    }
};


/*
 * A single use closure returning void, this deletes itself after it's run.
 */
template <>
class SingleUseClosure<void>: public BaseClosure<void> {
  public:
    virtual ~SingleUseClosure() {}
    void Run() {
      DoRun();
      delete this;
      return;
    }
};


/*
 * A function closure, with no arguments
 */
template <typename Parent, typename ReturnType>
class FunctionClosure: public Parent {
  public:
    typedef ReturnType (*Function)();

    explicit FunctionClosure(Function callback):
      Parent(),
      m_callback(callback) {}
    ReturnType DoRun() { return m_callback(); }

  private:
    Function m_callback;
};


/*
 * Create a new single use function closure.
 */
template <typename ReturnType>
inline SingleUseClosure<ReturnType>* NewSingleClosure(
    ReturnType (*callback)()) {
  return new FunctionClosure<SingleUseClosure<ReturnType>, ReturnType>(
      callback);
}


/*
 * Create a new function closure.
 */
template <typename ReturnType>
inline Closure<ReturnType>* NewClosure(ReturnType (*callback)()) {
  return new FunctionClosure<Closure<ReturnType>, ReturnType>(callback);
}


/*
 * A function closure, with one argument
 */
template <typename Parent, typename ReturnType, typename Arg>
class FunctionArgClosure: public Parent {
  public:
    typedef ReturnType (*Function)(Arg arg);

    /*
     * @param callback the function to call
     */
    FunctionArgClosure(Function callback, Arg arg):
      Parent(),
      m_callback(callback),
      m_arg(arg) {}
    ReturnType DoRun() { return m_callback(m_arg); }

  private:
    Function m_callback;
    Arg m_arg;
};


/*
 * Create a new single use function closure.
 */
template <typename ReturnType, typename Arg>
inline SingleUseClosure<ReturnType>* NewSingleClosure(
    ReturnType (*callback)(Arg arg),
    Arg arg) {
  return new FunctionArgClosure<SingleUseClosure<ReturnType>, ReturnType, Arg>(
      callback,
      arg);
}


/*
 * Create a new function closure.
 */
template <typename ReturnType, typename Arg>
inline Closure<ReturnType>* NewClosure(ReturnType (*callback)(Arg arg),
                                       Arg arg) {
  return new FunctionArgClosure<Closure<ReturnType>, ReturnType, Arg>(
      callback,
      arg);
}

/*
 * An method closure with no arguments
 */
template <typename Class, typename Parent, typename ReturnType>
class MethodClosure: public Parent {
  public:
    typedef ReturnType (Class::*Method)();

    /*
     * @param object the object to use in the method call
     * @param callback the method to call
     */
    MethodClosure(Class *object,
                  Method callback):
      Parent(),
      m_object(object),
      m_callback(callback) {}
    ReturnType DoRun() { return (m_object->*m_callback)(); }

  private:
    Class *m_object;
    Method m_callback;
};


/*
 * Create a new single use method closure.
 */
template <typename Class, typename ReturnType>
inline SingleUseClosure<ReturnType>* NewSingleClosure(
    Class* object,
    ReturnType (Class::*method)()) {
  return new MethodClosure<Class, SingleUseClosure<ReturnType>, ReturnType>(
      object,
      method);
}


/*
 * Create a new method closure.
 */
template <typename Class, typename ReturnType>
inline Closure<ReturnType>* NewClosure(Class* object,
                                       ReturnType (Class::*method)()) {
  return new MethodClosure<Class, Closure<ReturnType>, ReturnType>(object,
                                                                   method);
}


/*
 * A method closure that takes one argument
 */
template <typename Class, typename Parent, typename ReturnType, typename Arg>
class MethodArgClosure: public Parent {
  public:
    typedef ReturnType (Class::*Method)(Arg arg);

    /*
     * @param object the object to use in the method call
     * @param handle the method to call
     * @param arg the argument to pass to the method
     */
    MethodArgClosure(Class *object,
                     Method callback,
                     Arg arg):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_arg(arg) {}
    ReturnType DoRun() { return (m_object->*m_callback)(m_arg); }

  private:
    Class *m_object;
    Method m_callback;
    Arg m_arg;
};


/*
 * Create a new single use one-arg method closure
 */
template <typename Class, typename ReturnType, typename Arg>
inline SingleUseClosure<ReturnType>* NewSingleClosure(
    Class* object,
    ReturnType (Class::*method)(Arg arg),
    Arg arg) {
  return new MethodArgClosure<Class,
                              SingleUseClosure<ReturnType>,
                              ReturnType,
                              Arg>(
      object,
      method,
      arg);
}


/*
 * Create a new one-arg method closure
 */
template <typename Class, typename ReturnType, typename Arg>
inline Closure<ReturnType>* NewClosure(Class* object,
                                       ReturnType (Class::*method)(Arg arg),
                                       Arg arg) {
  return new MethodArgClosure<Class, Closure<ReturnType>, ReturnType, Arg>(
      object,
      method,
      arg);
}


/*
 * A method closure that takes two arguments
 */
template <typename Class,
          typename Parent,
          typename ReturnType,
          typename Arg,
          typename Arg2>
class MethodTwoArgClosure: public Parent {
  public:
    typedef ReturnType (Class::*Method)(Arg arg, Arg2 arg2);

    /*
     * @param object the object to use in the method call
     * @param method the method to call
     * @param arg the argument to pass to the method
     * @param arg2 the second argument to pass to the method
     */
    MethodTwoArgClosure(Class *object,
                        Method callback,
                        Arg arg,
                        Arg2 arg2):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_arg(arg),
      m_arg2(arg2) {}
    ReturnType DoRun() { return (m_object->*m_callback)(m_arg, m_arg2); }

  private:
    Class *m_object;
    Method m_callback;
    Arg m_arg;
    Arg2 m_arg2;
};


/*
 * Create a new single use two-arg method closure
 */
template <typename Class, typename Arg, typename ReturnType, typename Arg2>
inline SingleUseClosure<ReturnType>* NewSingleClosure(
    Class* object,
    ReturnType (Class::*method)(Arg arg, Arg2 arg2),
    Arg arg,
    Arg2 arg2) {
  return new MethodTwoArgClosure<Class,
                                 SingleUseClosure<ReturnType>,
                                 ReturnType,
                                 Arg,
                                 Arg2>(
      object,
      method,
      arg,
      arg2);
}


/*
 * Create a new two-arg method closure
 */
template <typename Class, typename ReturnType, typename Arg, typename Arg2>
inline Closure<ReturnType>* NewClosure(
    Class* object,
    ReturnType (Class::*method)(Arg arg, Arg2 arg2),
    Arg arg, Arg2 arg2) {
  return new MethodTwoArgClosure<Class,
                                 Closure<ReturnType>,
                                 ReturnType,
                                 Arg,
                                 Arg2>(
      object,
      method,
      arg,
      arg2);
}


/*
 * A method closure that takes three arguments
 */
template <typename Class,
          typename Parent,
          typename ReturnType,
          typename Arg,
          typename Arg2,
          typename Arg3>
class MethodThreeArgClosure: public Parent {
  public:
    typedef ReturnType (Class::*Method)(Arg arg, Arg2 arg2, Arg3 arg3);

    /*
     * @param object the object to use in the method call
     * @param method the method to call
     * @param arg the argument to pass to the method
     * @param arg2 the second argument to pass to the method
     */
    MethodThreeArgClosure(Class *object,
                          Method callback,
                          Arg arg,
                          Arg2 arg2,
                          Arg3 arg3):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_arg(arg),
      m_arg2(arg2),
      m_arg3(arg3) {}
    ReturnType DoRun() {
      return (m_object->*m_callback)(m_arg, m_arg2, m_arg3);
    }

  private:
    Class *m_object;
    Method m_callback;
    Arg m_arg;
    Arg2 m_arg2;
    Arg3 m_arg3;
};


/*
 * Create a new single use three-arg method closure
 */
template <typename Class, typename ReturnType, typename Arg, typename Arg2,
          typename Arg3>
inline SingleUseClosure<ReturnType>* NewSingleClosure(
    Class* object,
    ReturnType (Class::*method)(Arg arg, Arg2 arg2, Arg3 arg3),
    Arg arg,
    Arg2 arg2,
    Arg3 arg3) {
  return new MethodThreeArgClosure<Class,
                                   SingleUseClosure<ReturnType>,
                                   ReturnType,
                                   Arg,
                                   Arg2,
                                   Arg3>(
      object,
      method,
      arg,
      arg2,
      arg3);
}


/*
 * Create a new three-arg method closure
 */
template <typename Class, typename ReturnType, typename Arg, typename Arg2,
          typename Arg3>
inline Closure<ReturnType>* NewClosure(
    Class* object,
    ReturnType (Class::*method)(Arg arg, Arg2 arg2, Arg3 arg3),
    Arg arg, Arg2 arg2, Arg3 arg3) {
  return new MethodThreeArgClosure<Class,
                                   Closure<ReturnType>,
                                   ReturnType,
                                   Arg,
                                   Arg2,
                                   Arg3>(
      object,
      method,
      arg,
      arg2,
      arg3);
}


/*
 * A method closure that takes four arguments
 */
template <typename Class,
          typename Parent,
          typename ReturnType,
          typename Arg,
          typename Arg2,
          typename Arg3,
          typename Arg4>
class MethodFourArgClosure: public Parent {
  public:
    typedef ReturnType (Class::*Method)(Arg arg, Arg2 arg2, Arg3 arg3,
                                        Arg4 arg4);

    /*
     * @param object the object to use in the method call
     * @param method the method to call
     * @param arg the argument to pass to the method
     * @param arg2 the second argument to pass to the method
     */
    MethodFourArgClosure(Class *object,
                          Method callback,
                          Arg arg,
                          Arg2 arg2,
                          Arg3 arg3,
                          Arg4 arg4):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_arg(arg),
      m_arg2(arg2),
      m_arg3(arg3),
      m_arg4(arg4) {}
    ReturnType DoRun() {
      return (m_object->*m_callback)(m_arg, m_arg2, m_arg3, m_arg4);
    }

  private:
    Class *m_object;
    Method m_callback;
    Arg m_arg;
    Arg2 m_arg2;
    Arg3 m_arg3;
    Arg4 m_arg4;
};


/*
 * Create a new single use four-arg method closure
 */
template <typename Class, typename ReturnType, typename Arg, typename Arg2,
          typename Arg3, typename Arg4>
inline SingleUseClosure<ReturnType>* NewSingleClosure(
    Class* object,
    ReturnType (Class::*method)(Arg arg, Arg2 arg2, Arg3 arg3, Arg4 arg4),
    Arg arg,
    Arg2 arg2,
    Arg3 arg3,
    Arg4 arg4) {
  return new MethodFourArgClosure<Class,
                                   SingleUseClosure<ReturnType>,
                                   ReturnType,
                                   Arg,
                                   Arg2,
                                   Arg3,
                                   Arg4>(
      object,
      method,
      arg,
      arg2,
      arg3,
      arg4);
}


/*
 * Create a new three-arg method closure
 */
template <typename Class, typename ReturnType, typename Arg, typename Arg2,
          typename Arg3, typename Arg4>
inline Closure<ReturnType>* NewClosure(
    Class* object,
    ReturnType (Class::*method)(Arg arg, Arg2 arg2, Arg3 arg3, Arg4 arg4),
    Arg arg, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
  return new MethodFourArgClosure<Class,
                                   Closure<ReturnType>,
                                   ReturnType,
                                   Arg,
                                   Arg2,
                                   Arg3,
                                   Arg4>(
      object,
      method,
      arg,
      arg2,
      arg3,
      arg4);
}
}  // ola
#endif  // INCLUDE_OLA_CLOSURE_H_
