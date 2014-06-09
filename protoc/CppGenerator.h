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
 * CppGenerator.h
 * Copyright (C) 2013 Simon Newton
 */

#ifndef PROTOC_CPPGENERATOR_H_
#define PROTOC_CPPGENERATOR_H_

#include <google/protobuf/compiler/code_generator.h>
#include <string>

namespace ola {

class CppGenerator : public google::protobuf::compiler::CodeGenerator {
 public:
  CppGenerator() {}
  ~CppGenerator() {}

  // implements CodeGenerator ----------------------------------------
  bool Generate(const google::protobuf::FileDescriptor *file,
                const std::string &parameter,
                google::protobuf::compiler::OutputDirectory *generator_context,
                std::string *error) const;

 private:
  CppGenerator(const CppGenerator&);
  CppGenerator& operator=(const CppGenerator&);
};

}  // namespace ola
#endif  // PROTOC_CPPGENERATOR_H_
