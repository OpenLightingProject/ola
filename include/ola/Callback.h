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
 * Callback.h
 * Callback classes, these are similar to closures but can take arguments at
 * exec time.
 * Copyright (C) 2005-2010 Simon Newton
 */

#ifndef INCLUDE_OLA_CALLBACK_H_
#define INCLUDE_OLA_CALLBACK_H_

namespace ola {

// A single argument callback
template <typename ReturnType, typename Arg1>
class BaseCallback1 {
  public:
    virtual ~BaseCallback1() {}
    virtual ReturnType Run(Arg1 arg1) = 0;
    virtual ReturnType DoRun(Arg1 arg1) = 0;
};


/*
 * Callback, this is a closure that can be called multiple times
 */
template <typename ReturnType, typename Arg1>
class Callback1: public BaseCallback1<ReturnType, Arg1> {
  public:
    virtual ~Callback1() {}
    ReturnType Run(Arg1 arg1) { return this->DoRun(arg1); }
};


/*
 * A single use closure, this deletes itself after it's run.
 */
template <typename ReturnType, typename Arg1>
class SingleUseCallback1: public BaseCallback1<ReturnType, Arg1> {
  public:
    virtual ~SingleUseCallback1() {}
    ReturnType Run(Arg1 arg1) {
      ReturnType ret = this->DoRun(arg1);
      delete this;
      return ret;
    }
};


/*
 * A single use closure returning void, this deletes itself after it's run.
 */
template <typename Arg1>
class SingleUseCallback1<void, Arg1>: public BaseCallback1<void, Arg1> {
  public:
    virtual ~SingleUseCallback1() {}
    void Run(Arg1 arg1) {
      DoRun(arg1);
      delete this;
      return;
    }
};


/*
 * An method closure with no create-time arguments, and one exec time arg
 */
template <typename Class, typename Parent, typename ReturnType, typename Arg1>
class MethodCallback1: public Parent {
  public:
    typedef ReturnType (Class::*Method)(Arg1 arg);

    /*
     * @param object the object to use in the method call
     * @param callback the method to call
     */
    MethodCallback1(Class *object, Method callback):
      Parent(),
      m_object(object),
      m_callback(callback) {}
    ReturnType DoRun(Arg1 arg1) { return (m_object->*m_callback)(arg1); }

  private:
    Class *m_object;
    Method m_callback;
};


/*
 * Create a new single use method closure.
 */
template <typename Class, typename ReturnType, typename Arg1>
inline SingleUseCallback1<ReturnType, Arg1>* NewSingleCallback(
    Class* object,
    ReturnType (Class::*method)(Arg1 arg)) {
  return new MethodCallback1<Class,
                             SingleUseCallback1<ReturnType, Arg1>,
                             ReturnType,
                             Arg1>(
      object,
      method);
}


/*
 * Create a new method closure.
 */
template <typename Class, typename ReturnType, typename Arg1>
inline Callback1<ReturnType, Arg1>* NewCallback(
    Class* object,
    ReturnType (Class::*method)(Arg1 arg)) {
  return new MethodCallback1<Class,
                             Callback1<ReturnType, Arg1>,
                             ReturnType,
                             Arg1>(
      object,
      method);
}


/*
 * An method closure with one create-time arguments, and one exec time arg
 */
template <typename Class, typename Parent, typename ReturnType, typename A1,
          typename Arg1>
class MethodCallback1_1: public Parent {
  public:
    typedef ReturnType (Class::*Method)(A1, Arg1 arg);

    /*
     * @param object the object to use in the method call
     * @param callback the method to call
     */
    MethodCallback1_1(Class *object, Method callback, A1 a1):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_a1(a1) {}
    ReturnType DoRun(Arg1 arg1) { return (m_object->*m_callback)(m_a1, arg1); }

  private:
    Class *m_object;
    Method m_callback;
    A1 m_a1;
};


/*
 * Create a new single use method closure.
 */
template <typename Class, typename ReturnType, typename A1, typename Arg1>
inline SingleUseCallback1<ReturnType, Arg1>* NewSingleCallback(
    Class* object,
    ReturnType (Class::*method)(A1 a1, Arg1 arg),
    A1 a1) {
  return new MethodCallback1_1<Class,
                               SingleUseCallback1<ReturnType, Arg1>,
                               ReturnType,
                               A1,
                               Arg1>(
      object,
      method,
      a1);
}


/*
 * Create a new method closure.
 */
template <typename Class, typename ReturnType, typename A1, typename Arg1>
inline Callback1<ReturnType, Arg1>* NewCallback(
    Class* object,
    ReturnType (Class::*method)(A1 a1, Arg1 arg),
    A1 a1) {
  return new MethodCallback1_1<Class,
                              Callback1<ReturnType, Arg1>,
                              ReturnType,
                              A1,
                              Arg1>(
      object,
      method,
      a1);
}


// Two argument callbacks
template <typename ReturnType, typename Arg1, typename Arg2>
class BaseCallback2 {
  public:
    virtual ~BaseCallback2() {}
    virtual ReturnType Run(Arg1 arg1, Arg2 arg2) = 0;
    virtual ReturnType DoRun(Arg1 arg1, Arg2 arg2) = 0;
};


/*
 * Callback, this is a closure that can be called multiple times
 */
template <typename ReturnType, typename Arg1, typename Arg2>
class Callback2: public BaseCallback2<ReturnType, Arg1, Arg2> {
  public:
    virtual ~Callback2() {}
    ReturnType Run(Arg1 arg1, Arg2 arg2) { return this->DoRun(arg1, arg2); }
};


/*
 * A single use closure, this deletes itself after it's run.
 */
template <typename ReturnType, typename Arg1, typename Arg2>
class SingleUseCallback2: public BaseCallback2<ReturnType, Arg1, Arg2> {
  public:
    virtual ~SingleUseCallback2() {}
    ReturnType Run(Arg1 arg1, Arg2 arg2) {
      ReturnType ret = this->DoRun(arg1, arg2);
      delete this;
      return ret;
    }
};


/*
 * A single use closure returning void, this deletes itself after it's run.
 */
template <typename Arg1, typename Arg2>
class SingleUseCallback2<void, Arg1, Arg2>
    : public BaseCallback2<void, Arg1, Arg2> {
  public:
    virtual ~SingleUseCallback2() {}
    void Run(Arg1 arg1, Arg2 arg2) {
      DoRun(arg1, arg2);
      delete this;
      return;
    }
};


/*
 * An method closure with no create-time arguments, and two exec time arg
 */
template <typename Class, typename Parent, typename ReturnType,
          typename Arg1, typename Arg2>
class MethodCallback2: public Parent {
  public:
    typedef ReturnType (Class::*Method)(Arg1 arg, Arg2 arg2);

    /*
     * @param object the object to use in the method call
     * @param callback the method to call
     */
    MethodCallback2(Class *object, Method callback):
      Parent(),
      m_object(object),
      m_callback(callback) {}
    ReturnType DoRun(Arg1 arg1, Arg2 arg2) {
      return (m_object->*m_callback)(arg1, arg2);
    }

  private:
    Class *m_object;
    Method m_callback;
};


/*
 * Create a new single use method closure.
 */
template <typename Class, typename ReturnType, typename Arg1, typename Arg2>
inline SingleUseCallback2<ReturnType, Arg1, Arg2>* NewSingleCallback(
    Class* object,
    ReturnType (Class::*method)(Arg1 arg, Arg2 arg2)) {
  return new MethodCallback2<Class,
                             SingleUseCallback2<ReturnType, Arg1, Arg2>,
                             ReturnType,
                             Arg1,
                             Arg2>(
      object,
      method);
}


/*
 * Create a new method closure.
 */
template <typename Class, typename ReturnType, typename Arg1, typename Arg2>
inline Callback2<ReturnType, Arg1, Arg2>* NewCallback(
    Class* object,
    ReturnType (Class::*method)(Arg1 arg, Arg2 arg2)) {
  return new MethodCallback2<Class,
                             Callback2<ReturnType, Arg1, Arg2>,
                             ReturnType,
                             Arg1,
                             Arg2>(
      object,
      method);
}


/*
 * An method closure with one create-time argument, and two exec time arg
 */
template <typename Class, typename Parent, typename ReturnType,
          typename A1, typename Arg1, typename Arg2>
class MethodCallback2_1: public Parent {
  public:
    typedef ReturnType (Class::*Method)(A1 a1, Arg1 arg, Arg2 arg2);

    /*
     * @param object the object to use in the method call
     * @param callback the method to call
     */
    MethodCallback2_1(Class *object, Method callback, A1 a1):
      Parent(),
      m_object(object),
      m_callback(callback),
      m_a1(a1) {}
    ReturnType DoRun(Arg1 arg1, Arg2 arg2) {
      return (m_object->*m_callback)(m_a1, arg1, arg2);
    }

  private:
    Class *m_object;
    Method m_callback;
    A1 m_a1;
};


/*
 * Create a new single use method closure.
 */
template <typename Class, typename ReturnType, typename A1, typename Arg1,
          typename Arg2>
inline SingleUseCallback2<ReturnType, Arg1, Arg2>* NewSingleCallback(
    Class* object,
    ReturnType (Class::*method)(A1 a1, Arg1 arg, Arg2 arg2),
    A1 a1) {
  return new MethodCallback2_1<Class,
                               SingleUseCallback2<ReturnType, Arg1, Arg2>,
                               ReturnType,
                               A1,
                               Arg1,
                               Arg2>(
      object,
      method,
      a1);
}


/*
 * Create a new method closure.
 */
template <typename Class, typename ReturnType, typename A1, typename Arg1,
          typename Arg2>
inline Callback2<ReturnType, Arg1, Arg2>* NewCallback(
    Class* object,
    ReturnType (Class::*method)(A1 a1, Arg1 arg, Arg2 arg2),
    A1 a1) {
  return new MethodCallback2_1<Class,
                               Callback2<ReturnType, Arg1, Arg2>,
                               ReturnType,
                               A1,
                               Arg1,
                               Arg2>(
      object,
      method,
      a1);
}


// Four argument callbacks
template <typename ReturnType, typename Arg1, typename Arg2, typename Arg3,
          typename Arg4>
class BaseCallback4 {
  public:
    virtual ~BaseCallback4() {}
    virtual ReturnType Run(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) = 0;
    virtual ReturnType DoRun(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) = 0;
};


/*
 * Callback, this is a closure that can be called multiple times
 */
template <typename ReturnType, typename Arg1, typename Arg2, typename Arg3,
          typename Arg4>
class Callback4: public BaseCallback4<ReturnType, Arg1, Arg2, Arg3, Arg4> {
  public:
    virtual ~Callback4() {}
    ReturnType Run(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
      return this->DoRun(arg1, arg2, arg3, arg4);
    }
};


/*
 * A single use closure, this deletes itself after it's run.
 */
template <typename ReturnType, typename Arg1, typename Arg2, typename Arg3,
          typename Arg4>
class SingleUseCallback4: public
                          BaseCallback4<ReturnType, Arg1, Arg2, Arg3, Arg4> {
  public:
    virtual ~SingleUseCallback4() {}
    ReturnType Run(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
      ReturnType ret = this->DoRun(arg1, arg2, arg3, arg4);
      delete this;
      return ret;
    }
};


/*
 * A single use closure returning void, this deletes itself after it's run.
 */
template <typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class SingleUseCallback4<void, Arg1, Arg2, Arg3, Arg4>
    : public BaseCallback4<void, Arg1, Arg2, Arg3, Arg4> {
  public:
    virtual ~SingleUseCallback4() {}
    void Run(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
      DoRun(arg1, arg2, arg3, arg4);
      delete this;
      return;
    }
};


/*
 * An method closure with no create-time arguments, and four exec time arg
 */
template <typename Class, typename Parent, typename ReturnType,
          typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class MethodCallback4: public Parent {
  public:
    typedef ReturnType (Class::*Method)(Arg1 arg, Arg2 arg2, Arg3 arg3,
                                        Arg4 arg4);

    /*
     * @param object the object to use in the method call
     * @param callback the method to call
     */
    MethodCallback4(Class *object, Method callback):
      Parent(),
      m_object(object),
      m_callback(callback) {}
    ReturnType DoRun(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {
      return (m_object->*m_callback)(arg1, arg2, arg3, arg4);
    }

  private:
    Class *m_object;
    Method m_callback;
};


/*
 * Create a new single use method closure.
 */
template <typename Class, typename ReturnType, typename Arg1, typename Arg2,
          typename Arg3, typename Arg4>
inline SingleUseCallback4<ReturnType, Arg1, Arg2, Arg3, Arg4>*
  NewSingleCallback(
    Class* object,
    ReturnType (Class::*method)(Arg1 arg, Arg2 arg2, Arg3 arg3, Arg4 arg4)) {
  return new MethodCallback4<Class,
                             SingleUseCallback4<ReturnType, Arg1, Arg2, Arg3,
                                                Arg4>,
                             ReturnType,
                             Arg1,
                             Arg2,
                             Arg3,
                             Arg4>(
      object,
      method);
}


/*
 * Create a new method closure.
 */
template <typename Class, typename ReturnType, typename Arg1, typename Arg2,
          typename Arg3, typename Arg4>
inline Callback4<ReturnType, Arg1, Arg2, Arg3, Arg4>* NewCallback(
    Class* object,
    ReturnType (Class::*method)(Arg1 arg, Arg2 arg2, Arg3 arg3, Arg4 arg4)) {
  return new MethodCallback4<Class,
                             Callback4<ReturnType, Arg1, Arg2, Arg3, Arg4>,
                             ReturnType,
                             Arg1,
                             Arg2,
                             Arg3,
                             Arg4>(
      object,
      method);
}
}  // ola
#endif  // INCLUDE_OLA_CALLBACK_H_
