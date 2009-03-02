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

/*
 * Base Closure
 */
class LlaClosure {
  public:
    LlaClosure(bool single_use): m_single_use(single_use) {}
    virtual int Run() = 0;
    virtual ~LlaClosure() {}
    bool SingleUse() { return m_single_use; }
  protected:
    bool m_single_use;
};


/*
 * A templatized Closure, with no arguments
 * @param object the object to use in the method call
 * @param method the method to call
 */
template <typename Class>
class ObjectClosure: public LlaClosure {
  public:
    typedef int (Class::*RequestHandler)();

    ObjectClosure(Class *object,
                  RequestHandler handler,
                  bool single_use=false):
      LlaClosure(single_use),
      m_object(object),
      m_handler(handler) {}
    int Run() {
      int ret = (m_object->*m_handler)();
      if (m_single_use)
        delete this;
      return ret;
    }

  private:
    Class *m_object;
    RequestHandler m_handler;
};


/*
 * Create a new single use Closure. See above.
 */
template <typename Class>
inline LlaClosure* NewSingleClosure(Class* object,
                                    int (Class::*method)()) {
  return new ObjectClosure<Class>(object, method, true);
}


/*
 * Create a new Closure. See above.
 */
template <typename Class>
inline LlaClosure* NewClosure(Class* object,
                              int (Class::*method)()) {
  return new ObjectClosure<Class>(object, method);
}


/*
 * A templatized Closure, that takes one argument
 * @param object the object to use in the method call
 * @param method the method to call
 * @param arg the argument to pass to the method
 */
template <typename Class, typename Arg>
class ObjectArgClosure: public LlaClosure {
  public:
    typedef int (Class::*RequestHandler)(Arg arg);

    ObjectArgClosure(Class *object,
                     RequestHandler handler,
                     Arg arg,
                     bool single_use=false):
      LlaClosure(single_use),
      m_object(object),
      m_handler(handler),
      m_arg(arg) {}
    int Run() {
      int ret = (m_object->*m_handler)(m_arg);
      if (m_single_use)
        delete this;
      return ret;
    }

  private:
    Class *m_object;
    RequestHandler m_handler;
    Arg m_arg;
};


/*
 * Create a new single use Closure
 */
template <typename Class, typename Arg>
inline LlaClosure* NewSingleClosure(Class* object,
                                    int (Class::*method)(Arg arg),
                                    Arg arg) {
  return new ObjectArgClosure<Class, Arg>(object, method, arg, true);
}


/*
 * Create a new Closure
 */
template <typename Class, typename Arg>
inline LlaClosure* NewClosure(Class* object,
                              int (Class::*method)(Arg arg),
                              Arg arg) {
  return new ObjectArgClosure<Class, Arg>(object, method, arg);
}


} //lla

#endif
