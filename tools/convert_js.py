#!/usr/bin/env python
# coding: utf-8

import os, codecs, argparse

class CStringArray:
  def __init__(self):
    self._buffer = []
    self._strings = []

  def add(self, string):
    string = string.encode('utf-8').replace('\r', '')
    self._strings.append('std::string(buffer + %i, %i)' % (len(self._buffer), len(string)))
    self._buffer.extend(map(lambda c: str(ord(c)), string))

  def write(self, outHandle, arrayName):
    print >>outHandle, '#include <string>'
    print >>outHandle, 'namespace adblock {'
    print >>outHandle, '  const char buffer[] = {%s};' % ', '.join(self._buffer)
    print >>outHandle, '  std::string %s[] = {%s, std::string()};' % (arrayName, ', '.join(self._strings))
    print >>outHandle, '}  // namespace adblock'

def convert(files, outFile):
  array = CStringArray()

  for file in files:
    fileHandle = codecs.open(os.path.join('../lib', file), 'rb', encoding='utf-8')
    array.add(os.path.basename(file))
    array.add(fileHandle.read())
    fileHandle.close()

  outHandle = open(outFile, 'wb')
  array.write(outHandle, 'js_sources')
  outHandle.close()
  print 'Finish converting js files'

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Convert JavaScript files')
  parser.add_argument('--convert', metavar='file_to_convert', nargs='+',
      help='JavaScript files to convert')
  parser.add_argument('output_file', help='output from the conversion')
  args = parser.parse_args()
  convert(args.convert, args.output_file)
