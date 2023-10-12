// cli_options.cc for Shynebox Window Manager

#include "cli.hh"
#include "version.h"
#include "defaults.hh"
#include "Debug.hh"

#include "tk/App.hh"
#include "tk/FileUtil.hh"
#include "tk/StringUtil.hh"
#include "tk/Theme.hh"
#include "tk/I18n.hh"
#include "tk/Command.hh"
#include "tk/CommandParser.hh"

#include <cstdlib>
#include <cstring>
#include <iostream>

using std::cerr;
using std::cout;
using std::string;

ShyneboxCli::Options::Options() : xsync(false) {
  const char *env = getenv("DISPLAY");
  if (env && strlen(env) > 0)
    session_display.assign(env);

  // try   ~/.config/shynebox   then   ~/.shynebox
  const char *xdg = getenv("XDG_CONFIG_HOME"); // /home/you/.config

  if (xdg != 0) // var may not be set by session manager or shell
    rc_path = xdg + string("/") + realProgramName("shynebox");

  if (!tk::FileUtil::isDirectory(rc_path.c_str() ) ) {
    const char *home_env = getenv("HOME"); // /home/you/
    // /home/you/.
    const string home = home_env + string("/.");
    rc_path = home + realProgramName("shynebox");
  }

  rc_file = rc_path + "/init";
}

int ShyneboxCli::Options::parse(int argc, char** argv) {
    _SB_USES_NLS;

    int i;
    for (i = 1; i < argc; ++i) {
      string arg(argv[i]);
      if (arg == "-rc" || arg == "--rc") {
        // look for alternative rc file to use

        if ((++i) >= argc) {
          cerr<<_SB_CONSOLETEXT(main, RCRequiresArg,
                        "error: '-rc' requires an argument",
                        "the -rc option requires a file argument")<<"\n";
          return EXIT_FAILURE;
        }
        this->rc_file = argv[i];
      } else if (arg == "-display" || arg == "--display") {
        // check for -display option... to run on a display other than the one
        // set by the environment variable DISPLAY

        if ((++i) >= argc) {
          cerr<<_SB_CONSOLETEXT(main, DISPLAYRequiresArg,
                                "error: '-display' requires an argument",
                                "")<<"\n";
          return EXIT_FAILURE;
        }

        this->session_display = argv[i];
        if (!tk::App::setenv("DISPLAY", argv[i]) ) {
          cerr<<_SB_CONSOLETEXT(main, WarnDisplayEnv,
                                "warning: couldn't set environment variable 'DISPLAY'",
                                "")<<"\n";
          perror("putenv()");
        }
      } else if (arg == "-version" || arg == "-v" || arg == "--version") {
        // print current version string
        cout << "Shynebox " << __shynebox_version << " : (c) 2023 zlice\n\n";
        return EXIT_SUCCESS;
      } else if (arg == "-log" || arg == "--log") {
        if (++i >= argc) {
          cerr<<_SB_CONSOLETEXT(main, LOGRequiresArg,
                                "error: '-log' needs an argument", "")<<"\n";
          return EXIT_FAILURE;
        }
        this->log_filename = argv[i];
      } else if (arg == "-sync" || arg == "--sync") {
        this->xsync = true;
      } else if (arg == "-help" || arg == "-h" || arg == "--help") {
        // print program usage and command line options
        cout << "Shynebox " << __shynebox_version << " : (c) 2023 zlice\n"
             << "Website: https://github.com/zlice/shynebox/\n\n"
             << "-display <string>\t\tuse display connection.\n"
             << "-screen <all|int,int,int>\trun on specified screens only.\n"
             << "-rc <string>\t\t\tuse alternate resource file.\n"
             << "-no-toolbar\t\t\tdo not provide a toolbar.\n"
             << "-version\t\t\tdisplay version and exit.\n"
             << "-info\t\t\t\tdisplay some useful information.\n"
             << "-list-commands\t\t\tlist all valid key commands.\n"
             << "-sync\t\t\t\tsynchronize with X server for debugging.\n"
             << "-log <filename>\t\t\tlog output to file.\n"
             << "-help\t\t\t\tdisplay this help text and exit.\n\n";
        // NLS
        //printf(_SB_CONSOLETEXT(main, Usage,
        //               "Fluxbox %s : (c) %s Fluxbox Team\n"
        //               "Website: http://www.fluxbox.org/\n\n"
        //               "-display <string>\t\tuse display connection.\n"
        //               "-screen <all|int,int,int>\trun on specified screens only.\n"
        //               "-rc <string>\t\t\tuse alternate resource file.\n"
        //               "-no-toolbar\t\t\tdo not provide a toolbar.\n"
        //               "-version\t\t\tdisplay version and exit.\n"
        //               "-info\t\t\t\tdisplay some useful information.\n"
        //               "-list-commands\t\t\tlist all valid key commands.\n"
        //               "-sync\t\t\t\tsynchronize with X server for debugging.\n"
        //               "-log <filename>\t\t\tlog output to file.\n"
        //               "-help\t\t\t\tdisplay this help text and exit.\n\n",

        //               "Main usage string. Please lay it out nicely. One %%s gives the version, ther other gives the year").c_str(),
        //       __fluxbox_version, "2001-2015");
        return EXIT_SUCCESS;
      } else if (arg == "-info" || arg == "-i" || arg == "--info") {
        ShyneboxCli::showInfo(cout);
        return EXIT_SUCCESS;
      } else if (arg == "-list-commands" || arg == "--list-commands") {
        tk::CommandParser<void>::CreatorMap cmap = tk::CommandParser<void>::instance().creatorMap();
        for (auto it : cmap)
          cout << it.first << "\n";
        return EXIT_SUCCESS;
      } else if (arg == "-verbose" || arg == "--verbose") {
        tk::ThemeManager::instance().setVerbose(true);
      }
    } // for argc
    return -1;
} // parse

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
