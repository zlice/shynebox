// cli_cfiles.cc for Shynebox Window Manager

#include "cli.hh"
#include "defaults.hh"

#include "Debug.hh"
#include "tk/FileUtil.hh"
#include "tk/I18n.hh"
#include "tk/Config.hh"
#include "tk/StringUtil.hh"

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/types.h>
#include <sys/stat.h>
#endif // HAVE_SYS_STAT_H

#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif

#ifdef HAVE_CSTDLIB
  #include <cstdlib>
#else
  #include <stdlib.h>
#endif

using std::string;
using std::cerr;

/**
 setup the configutation files in
 home directory
*/
void ShyneboxCli::setupConfigFiles(const std::string& dirname, const std::string& rc) {
  _SB_USES_NLS;

  const bool has_dir = tk::FileUtil::isDirectory(dirname.c_str() );

  struct CFInfo {
    bool create_file;
    const char* default_name;
    const std::string filename;
  } cfiles[] = {
      //{ !has_dir, DEFAULT_INITFILE, rc }, // handled by ConfigManager
      { !has_dir, DEFAULTKEYSFILE, dirname + "/keys" },
      { !has_dir, DEFAULTMENU, dirname + "/menu" },
      { !has_dir, DEFAULT_APPSFILE, dirname + "/apps" },
      { !has_dir, DEFAULT_OVERLAY, dirname + "/overlay" },
      { !has_dir, DEFAULT_WINDOWMENU, dirname + "/windowmenu" }
  };
  const size_t nr_of_cfiles = sizeof(cfiles)/sizeof(CFInfo);

  if (!has_dir) { // create directory
    sbdbg << "Creating dir: " << dirname << "\n";
    if (mkdir(dirname.c_str(), 0700) ) {
      cerr << _SB_CONSOLETEXT(Shynebox, ErrorCreatingDirectory,
                              "Can't create directory: ",
                              "Can't create a directory, for directory name").c_str()
           << dirname.c_str() << "\n";
      return; // this just returns to main, which may bomb out
    }
  } else // check what files exist
    for (size_t i = 0; i < nr_of_cfiles; ++i)
      cfiles[i].create_file = access(cfiles[i].filename.c_str(), F_OK);

  bool sync_fs = false;

  // copy defaults for files that don't exist
  for (size_t i = 0; i < nr_of_cfiles; ++i) {
    if (cfiles[i].create_file) {
      tk::FileUtil::copyFile(tk::StringUtil::expandFilename(cfiles[i].default_name).c_str(),
                               cfiles[i].filename.c_str() );
      sync_fs = true;
    }
  }
#ifdef HAVE_SYNC
  if (sync_fs)
    sync();
#endif
} // setupConfigFiles

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2014 - Mathias Gumz <akira at fluxbox.org>
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
