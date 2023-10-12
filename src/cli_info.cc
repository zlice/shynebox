// cli_info.cc for Shynebox Window Manager

#include "cli.hh"
#include "config.h"
#include "defaults.hh"
#include "version.h"

#include "tk/I18n.hh"
#include "tk/StringUtil.hh"


#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::ostream;

void ShyneboxCli::showInfo(ostream &ostr) {
  _SB_USES_NLS;
  ostr << _SB_CONSOLETEXT(Common, ShyneboxVersion, "Shynebox version", "Shynebox version heading")
       << ": " << __shynebox_version << "\n";

  if (strlen(gitrevision() ) > 0)
    ostr << _SB_CONSOLETEXT(Common, SvnRevision, "GIT Revision",
            "Revision number in GIT repositary")
         << ": " << gitrevision() << "\n";
#if defined(__DATE__) && defined(__TIME__)
  ostr << _SB_CONSOLETEXT(Common, Compiled, "Compiled",
          "Time shynebox was compiled")
       << ": " << __DATE__ << " " << __TIME__ << "\n";
#endif
#ifdef __shynebox_compiler
  ostr << _SB_CONSOLETEXT(Common, Compiler, "Compiler",
          "Compiler used to build shynebox")
       << ": " << __shynebox_compiler << "\n";
#endif
#ifdef __shynebox_compiler_version
  ostr << _SB_CONSOLETEXT(Common, CompilerVersion, "Compiler version",
          "Compiler version used to build shynebox")
       << ": " << __shynebox_compiler_version << "\n";
#endif

  ostr << "\n"
       << _SB_CONSOLETEXT(Common, Defaults, "Defaults",
               "Default values compiled in")
       << ": " << "\n";

  ostr << _SB_CONSOLETEXT(Common, DefaultMenuFile, "       menu",
          "default menu file (right aligned - make sure same width as other default values)")
      << ": " << tk::StringUtil::expandFilename(DEFAULTMENU) << "\n";
  ostr << _SB_CONSOLETEXT(Common, DefaultWindowMenuFile, " windowmenu",
          "default windowmenu file (right aligned - make sure same width as other default values)")
       << ": "
       << tk::StringUtil::expandFilename(DEFAULT_WINDOWMENU) << "\n";
  ostr << _SB_CONSOLETEXT(Common, DefaultStyle, "      style",
          "default style (right aligned - make sure same width as other default values)")
       << ": " << tk::StringUtil::expandFilename(DEFAULTSTYLE) << "\n";

  ostr << _SB_CONSOLETEXT(Common, DefaultKeyFile, "       keys",
          "default key file (right aligned - make sure same width as other default values)")
       << ": " << tk::StringUtil::expandFilename(DEFAULTKEYSFILE) << "\n";
  ostr << _SB_CONSOLETEXT(Common, DefaultInitFile, "       init",
          "default init file (right aligned - make sure same width as other default values)")
       << ": " << tk::StringUtil::expandFilename(DEFAULT_INITFILE) << "\n";

#ifdef NLS
  ostr << _SB_CONSOLETEXT(Common, DefaultLocalePath, "         nls",
          "location for localization files (right aligned - make sure same width as other default values)")
       << ": " << tk::StringUtil::expandFilename(LOCALEPATH) << "\n";
#endif

  const char NOT[] = "-";
  ostr << "\n"
       << _SB_CONSOLETEXT(Common, CompiledOptions, "Compiled options", "Options used when compiled")
       << " (" << NOT << " => "
       << _SB_CONSOLETEXT(Common, Disabled, "disabled", "option is turned off") << "): \n"
       <<

/**** NOTE: This list is in alphabetical order! ****/

#ifndef HAVE_FRIBIDI
      NOT <<
#endif
      "BIDI\n" <<

#ifndef DEBUG
      NOT <<
#endif // DEBUG
      "DEBUG\n" <<

#ifndef HAVE_IMLIB2
      NOT<<
#endif // HAVE_IMLIB2
      "IMLIB2\n" <<

#ifndef NLS
      NOT<<
#endif // NLS
      "NLS\n" <<

#ifndef SHAPE
      NOT <<
#endif // SHAPE
      "SHAPE\n" <<

#ifndef USE_TOOLBAR
      NOT <<
#endif // USE_TOOLBAR
      "TOOLBAR\n" <<

#ifndef USE_XFT
      NOT <<
#endif // USE_XFT
      "XFT\n" <<

#ifndef USE_XMB
      NOT <<
#endif // USE_XMB
      "XMB\n" <<

#ifndef HAVE_XPM
      NOT <<
#endif // HAVE_XPM
      "XPM\n"

      << "\n";
} // showInfo

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
