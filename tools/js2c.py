#!/usr/bin/env python
#
# Copyright 2012 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This is a utility for converting JavaScript source code into C-style
# char arrays. It is used for embedded JavaScript code in the V8
# library.

import os, re, sys, string
try:
  from slimit import minify
except ImportError:
  print 'You have to install slimit from https://pypi.python.org/pypi/slimit'

def ToCAsciiArray(lines):
  result = []
  for chr in lines:
    value = ord(chr)
    assert value < 128
    result.append(str(value))
  return ", ".join(result)


def RemoveCommentsAndTrailingWhitespace(lines):
  lines = re.sub(r'//.*\n', '\n', lines) # end-of-line comments
  lines = re.sub(re.compile(r'/\*.*?\*/', re.DOTALL), '', lines) # comments.
  lines = re.sub(r'\s+\n+', '\n', lines) # trailing whitespace
  return lines


def ReadFile(filename):
  file = open(filename, "rt")
  try:
    lines = file.read()
  finally:
    file.close()
  return lines


def ReadLines(filename):
  result = []
  for line in open(filename, "rt"):
    if '#' in line:
      line = line[:line.index('#')]
    line = line.strip()
    if len(line) > 0:
      result.append(line)
  return result


EVAL_PATTERN = re.compile(r'\beval\s*\(')
WITH_PATTERN = re.compile(r'\bwith\s*\(')


def Validate(lines, file):
  lines = RemoveCommentsAndTrailingWhitespace(lines)
  # Because of simplified context setup, eval and with is not
  # allowed in the natives files.
  eval_match = EVAL_PATTERN.search(lines)
  if eval_match:
    raise ("Eval disallowed in natives: %s" % file)
  with_match = WITH_PATTERN.search(lines)
  if with_match:
    raise ("With statements disallowed in natives: %s" % file)


def ExpandConstants(lines, constants):
  for key, value in constants:
    lines = key.sub(str(value), lines)
  return lines


def ExpandMacros(lines, macros):
  # We allow macros to depend on the previously declared macros, but
  # we don't allow self-dependecies or recursion.
  for name_pattern, macro in reversed(macros):
    pattern_match = name_pattern.search(lines, 0)
    while pattern_match is not None:
      # Scan over the arguments
      height = 1
      start = pattern_match.start()
      end = pattern_match.end()
      assert lines[end - 1] == '('
      last_match = end
      arg_index = [0]  # Wrap state into array, to work around Python "scoping"
      mapping = { }
      def add_arg(str):
        # Remember to expand recursively in the arguments
        replacement = ExpandMacros(str.strip(), macros)
        mapping[macro.args[arg_index[0]]] = replacement
        arg_index[0] += 1
      while end < len(lines) and height > 0:
        # We don't count commas at higher nesting levels.
        if lines[end] == ',' and height == 1:
          add_arg(lines[last_match:end])
          last_match = end + 1
        elif lines[end] in ['(', '{', '[']:
          height = height + 1
        elif lines[end] in [')', '}', ']']:
          height = height - 1
        end = end + 1
      # Remember to add the last match.
      add_arg(lines[last_match:end-1])
      result = macro.expand(mapping)
      # Replace the occurrence of the macro with the expansion
      lines = lines[:start] + result + lines[end:]
      pattern_match = name_pattern.search(lines, start + len(result))
  return lines

class TextMacro:
  def __init__(self, args, body):
    self.args = args
    self.body = body
  def expand(self, mapping):
    result = self.body
    for key, value in mapping.items():
        result = result.replace(key, value)
    return result

class PythonMacro:
  def __init__(self, args, fun):
    self.args = args
    self.fun = fun
  def expand(self, mapping):
    args = []
    for arg in self.args:
      args.append(mapping[arg])
    return str(self.fun(*args))

CONST_PATTERN = re.compile(r'^const\s+([a-zA-Z0-9_]+)\s*=\s*([^;]*);$')
MACRO_PATTERN = re.compile(r'^macro\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*=\s*([^;]*);$')
PYTHON_MACRO_PATTERN = re.compile(r'^python\s+macro\s+([a-zA-Z0-9_]+)\s*\(([^)]*)\)\s*=\s*([^;]*);$')


def ReadMacros(lines):
  constants = []
  macros = []
  for line in lines:
    hash = line.find('#')
    if hash != -1: line = line[:hash]
    line = line.strip()
    if len(line) is 0: continue
    const_match = CONST_PATTERN.match(line)
    if const_match:
      name = const_match.group(1)
      value = const_match.group(2).strip()
      constants.append((re.compile("\\b%s\\b" % name), value))
    else:
      macro_match = MACRO_PATTERN.match(line)
      if macro_match:
        name = macro_match.group(1)
        args = [match.strip() for match in macro_match.group(2).split(',')]
        body = macro_match.group(3).strip()
        macros.append((re.compile("\\b%s\\(" % name), TextMacro(args, body)))
      else:
        python_match = PYTHON_MACRO_PATTERN.match(line)
        if python_match:
          name = python_match.group(1)
          args = [match.strip() for match in python_match.group(2).split(',')]
          body = python_match.group(3).strip()
          fun = eval("lambda " + ",".join(args) + ': ' + body)
          macros.append((re.compile("\\b%s\\(" % name), PythonMacro(args, fun)))
        else:
          raise ("Illegal line: " + line)
  return (constants, macros)


HEADER_TEMPLATE = """\
// This file was generated from .js source files by js2c.py. If you
// want to make changes to this file you should either change the
// javascript source files or `js2c.py` script.

#include <string>

namespace adblock {

static const char sources[] = { %(sources_data)s };
std::string js_sources[] = { %(source_lines)sstd::string() };

}  // namespace adblock
"""


SOURCE_DECLARATION = 'std::string("%(name)s"), std::string(sources + %(offset)i, %(raw_length)i), '


def JS2C(source, debug, target):
  ids = []
  modules = []
  # Locate the macros file name.
  consts = []
  macros = []
  for s in source:
    if 'macros.py' == (os.path.split(str(s))[1]):
      (consts, macros) = ReadMacros(ReadLines(str(s)))
    else:
      modules.append(s)

  debug_js_dir = ""
  if debug:
    debug_js_dir = os.path.join(os.path.split(str(target))[0], 'js');
    if not os.path.isdir(debug_js_dir):
      os.makedirs(debug_js_dir)

  module_offset = 0
  all_sources = []
  for module in modules:
    filename = str(module)
    lines = ReadFile(filename)
    lines = ExpandConstants(lines, consts)
    lines = ExpandMacros(lines, macros)
    Validate(lines, filename)
    lines = minify(lines, mangle=True)
    id = os.path.split(filename)[1]
    raw_length = len(lines)
    ids.append((id, raw_length, module_offset))
    all_sources.append(lines)
    module_offset += raw_length
    if debug:
      debug_js = open(os.path.join(debug_js_dir, id), 'w')
      debug_js.write(lines)
      debug_js.close()

  sources_data = ToCAsciiArray("".join(all_sources))

  # Build source code lines
  source_lines = [ ]
  for (id, raw_length, module_offset) in ids:
    source_lines.append(SOURCE_DECLARATION % {
      'name': id,
      'offset': module_offset,
      'raw_length': raw_length,
      })

  # Emit result
  output = open(str(target), "w")
  output.write(HEADER_TEMPLATE % {
    'sources_data': sources_data,
    'source_lines': "".join(source_lines)
  })
  output.close()

def main():
  natives = sys.argv[1]
  debug = sys.argv[2]
  source_files = sys.argv[3:]
  JS2C(source_files, debug, natives)

if __name__ == "__main__":
  main()
