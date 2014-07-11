/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Macro.h
 */

/**
 * @file Macro.h
 * @brief Helper macros.
 */

#ifndef INCLUDE_OLA_BASE_MACRO_H_
#define INCLUDE_OLA_BASE_MACRO_H_

/**
 * @def DISALLOW_COPY_AND_ASSIGN(Foo)
 * @brief Creates dummy copy constructor and assignment operator declarations.
 *
 * Use this in the private: section of a class to prevent copying / assignment.
 *
 * @examplepara
 *   @code
 *   class Foo {
 *     public:
 *       Foo() { ... }
 *
 *     private:
 *       DISALLOW_COPY_AND_ASSIGN(Foo);
 *     };
 *   @endcode
 */
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

/*
 * This code was adapted from:
 * http://blogs.msdn.com/b/abhinaba/archive/
 *   2008/10/27/c-c-compile-time-asserts.aspx
 */

#ifdef __cplusplus

#define JOIN(X, Y) JOIN2(X, Y)
#define JOIN2(X, Y) X##Y

namespace internal {
  template <bool> struct STATIC_ASSERT_FAILURE;
  template <> struct STATIC_ASSERT_FAILURE<true> { enum { value = 1 }; };

  template<int x> struct static_assert_test{};
}

#define STATIC_ASSERT(x) \
  typedef ::internal::static_assert_test<\
    sizeof(::internal::STATIC_ASSERT_FAILURE< static_cast<bool>( x ) >)>\
      JOIN(_static_assert_typedef, __LINE__)

#else  // __cplusplus

#define STATIC_ASSERT(x) extern int __dummy[static_cast<int>x]

#endif  // __cplusplus

/*
 * End of adapted code.
 */
 
 /*
  * In order to account for platform differences with regard to packing, we
  * need to use the following macro while declaring types that need to have a
  * specific binary layout.
  * Taken from:
  * http://stackoverflow.com/questions/1537964/
  *   visual-c-equivalent-of-gccs-attribute-packed
  */
#ifdef _WIN32
#ifdef _MSC_VER
#define PACK(__Declaration__) \
    __pragma(pack(push, 1)) \
    __Declaration__; \
    __pragma(pack(pop))
#else
#define PACK(__Declaration__) \
    _Pragma("pack(push, 1)") \
    __Declaration__; \
    _Pragma("pack(pop)")
#endif
#else
#define PACK( __Declaration__ ) __Declaration__; __attribute__((__packed__))
#endif

#endif  // INCLUDE_OLA_BASE_MACRO_H_
