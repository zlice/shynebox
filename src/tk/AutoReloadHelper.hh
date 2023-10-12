// AutoreloadHelper.hh for Shynebox Window Manager

/*
  Help auto-reload menu files
  E.g. when you edit files but don't restart/reconfigure
*/

#ifndef AUTORELOADHELPER_HH
#define AUTORELOADHELPER_HH

#include <map>
#include <string>
#include <sys/types.h>

#include "Command.hh"

namespace tk {

class AutoReloadHelper {
public:
  ~AutoReloadHelper() {
    if (m_reload_cmd)
      delete m_reload_cmd;
  }

  void setMainFile(const std::string& filename);
  void addFile(const std::string& filename);
  void setReloadCmd(Command<void> &cmd) { m_reload_cmd = &cmd; }

  void checkReload();
  void reload();

private:
  Command<void> *m_reload_cmd = 0;
  std::string m_main_file;

  typedef std::map<std::string, time_t> TimestampMap;
  TimestampMap m_timestamps;
};

} // end namespace tk

#endif // AUTORELOADHELPER_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2008 Fluxbox Team (fluxgen at fluxbox dot org)
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
