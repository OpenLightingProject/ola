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
 * foo.cpp
 * Copyright (C) 2011 Simon Newton
 */

#include <ola/Logging.h>

// prototype of bison-generated parser function
int yyparse();

int main(int argc, char **argv) {
  ola::InitLogging(ola::OLA_LOG_INFO, ola::OLA_LOG_STDERR);

  if ((argc > 1) && (freopen(argv[1], "r", stdin) == NULL)) {
    OLA_WARN << argv[0] << ": File " << argv[1] << " cannot be opened.\n";
    exit(1);
  }

  yyparse();
  return 0;
}
