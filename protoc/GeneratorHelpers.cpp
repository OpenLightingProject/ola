// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// Author: kenton@google.com (Kenton Varda)
//  Based on original Protocol Buffers design by
//  Sanjay Ghemawat, Jeff Dean, and others.

#include <google/protobuf/io/printer.h>
#include <google/protobuf/stubs/common.h>

#include <limits>
#include <map>
#include <string>
#include <vector>

#include "protoc/GeneratorHelpers.h"
#include "protoc/StrUtil.h"

using std::string;
using google::protobuf::Descriptor;
using google::protobuf::EnumDescriptor;

namespace ola {

namespace {

string DotsToUnderscores(const string& name) {
  return StringReplace(name, ".", "_", true);
}

string DotsToColons(const string& name) {
  return StringReplace(name, ".", "::", true);
}

// Returns whether the provided descriptor has an extension. This includes its
// nested types.
bool HasExtension(const Descriptor* descriptor) {
  if (descriptor->extension_count() > 0) {
    return true;
  }
  for (int i = 0; i < descriptor->nested_type_count(); ++i) {
    if (HasExtension(descriptor->nested_type(i))) {
      return true;
    }
  }
  return false;
}

}  // namespace

const char kThickSeparator[] =
  "// ===================================================================\n";
const char kThinSeparator[] =
  "// -------------------------------------------------------------------\n";

string ClassName(const Descriptor* descriptor, bool qualified) {
  // Find "outer", the descriptor of the top-level message in which
  // "descriptor" is embedded.
  const Descriptor* outer = descriptor;
  while (outer->containing_type() != NULL) outer = outer->containing_type();

  const string& outer_name = outer->full_name();
  string inner_name = descriptor->full_name().substr(outer_name.size());

  if (qualified) {
    return "::" + DotsToColons(outer_name) + DotsToUnderscores(inner_name);
  } else {
    return outer->name() + DotsToUnderscores(inner_name);
  }
}

string ClassName(const EnumDescriptor* enum_descriptor, bool qualified) {
  if (enum_descriptor->containing_type() == NULL) {
    if (qualified) {
      return "::" + DotsToColons(enum_descriptor->full_name());
    } else {
      return enum_descriptor->name();
    }
  } else {
    string result = ClassName(enum_descriptor->containing_type(), qualified);
    result += '_';
    result += enum_descriptor->name();
    return result;
  }
}

string StripProto(const string& filename) {
  if (HasSuffixString(filename, ".protodevel")) {
    return StripSuffixString(filename, ".protodevel");
  } else {
    return StripSuffixString(filename, ".proto");
  }
}

// Convert a file name into a valid identifier.
string FilenameIdentifier(const string& filename) {
  string result;
  for (unsigned int i = 0; i < filename.size(); i++) {
    if (ascii_isalnum(filename[i])) {
      result.push_back(filename[i]);
    } else {
      // Not alphanumeric.  To avoid any possibility of name conflicts we
      // use the hex code for the character.
      result.push_back('_');
      char buffer[kFastToBufferSize];
      result.append(FastHexToBuffer(static_cast<uint8_t>(filename[i]), buffer));
    }
  }
  return result;
}

// Return the name of the AddDescriptors() function for a given file.
string GlobalAddDescriptorsName(const string& filename) {
  return "protobuf_AddDesc_" + FilenameIdentifier(filename);
}

// Return the name of the AssignDescriptors() function for a given file.
string GlobalAssignDescriptorsName(const string& filename) {
  return "protobuf_AssignDesc_" + FilenameIdentifier(filename);
}

bool StaticInitializersForced(const FileDescriptor* file) {
  if (HasDescriptorMethods(file) || file->extension_count() > 0) {
    return true;
  }
  for (int i = 0; i < file->message_type_count(); ++i) {
    if (HasExtension(file->message_type(i))) {
      return true;
    }
  }
  return false;
}

void PrintHandlingOptionalStaticInitializers(
    const FileDescriptor* file, Printer* printer,
    const char* with_static_init, const char* without_static_init,
    const char* var1, const string& val1,
    const char* var2, const string& val2) {
  map<string, string> vars;
  if (var1) {
    vars[var1] = val1;
  }
  if (var2) {
    vars[var2] = val2;
  }
  PrintHandlingOptionalStaticInitializers(
      vars, file, printer, with_static_init, without_static_init);
}

void PrintHandlingOptionalStaticInitializers(
    const map<string, string>& vars, const FileDescriptor* file,
    Printer* printer, const char* with_static_init,
    const char* without_static_init) {
  if (StaticInitializersForced(file)) {
    printer->Print(vars, with_static_init);
  } else {
    printer->Print(vars, (string(
      "#ifdef GOOGLE_PROTOBUF_NO_STATIC_INITIALIZER\n") +
      without_static_init +
      "#else\n" +
      with_static_init +
      "#endif\n").c_str());
  }
}
}  // namespace ola
