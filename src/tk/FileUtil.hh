// FileUtil.hh for Shynebox Window Manager

/*
  Handles checking of directory and file status.
  Used to copy default files on first runs, read
  menus/styles(themes) and AutoReload helpers.
*/

#ifndef TK_FILEUTIL_HH
#define TK_FILEUTIL_HH

#ifdef HAVE_CTIME
  #include <ctime>
#else
  #include <time.h>
#endif
#include <sys/types.h>
#include <dirent.h>

#include <string>

#include "NotCopyable.hh"

namespace tk {

namespace FileUtil {
  bool isDirectory(const char* filename);
  bool isRegularFile(const char* filename);
  bool isExecutable(const char* filename);

  // -1 for failure
  time_t getLastStatusChangeTimestamp(const char* filename);

  bool copyFile(const char* from, const char* to);

} // end of File namespace

//  Wrapper class for DIR * routines
class Directory : private tk::NotCopyable {
public:
  explicit Directory(const char *dir = 0);
  ~Directory();
  const std::string &name() const { return m_name; }
  // go to start of filelist
  void rewind();
  // gets next dirent info struct in directory and
  // jumps to next directory entry
  struct dirent * read();
  // reads next filename in directory
  std::string readFilename();
  void close();
  bool open(const char *dir);
  // @return number of entries in the directory
  size_t entries() const { return m_num_entries; }

private:
  std::string m_name;
  DIR *m_dir;
  size_t m_num_entries; // number of file entries in directory
};

} // end namespace tk

#endif // TK_FILEUTIL_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2002 - 2004 Henrik Kinnunen (fluxgen at fluxbox dot org)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
