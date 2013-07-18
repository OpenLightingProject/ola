/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Copyright (C) 2010 Simon Newton
 */
//! [Example] NOLINT(whitespace/comments)
#include <ola/base/Flags.h>
#include <iostream>

using std::cout;
using std::endl;

// These options are --foo and --nobar.
DEFINE_bool(foo, false, "Enable feature foo");
DEFINE_bool(bar, true, "Disable feature bar");

// FLAGS_name defaults to "simon" and can be changed with --name bob
DEFINE_string(name, "simon", "Specify the name");

// Short options can also be specified:
// FLAGS_baz can be set with --baz or -b
DEFINE_s_int8(baz, b, 0, "Sets the value of baz");

int main(int argc, char* argv[]) {
  ola::SetHelpString("<options>", "Description of binary");
  ola::ParseFlags(&argc, argv);

  cout << "--foo is " << FLAGS_foo << endl;
  cout << "--bar is " << FLAGS_bar << endl;
  cout << "--name is " << FLAGS_name.str() << endl;
  cout << "--baz (-b) is " << static_cast<int>(FLAGS_baz) << endl;
}
//! [Example] NOLINT(whitespace/comments)
