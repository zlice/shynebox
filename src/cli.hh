// cli.hh for Shynebox Window Manager

/*
  Split over cli_files, cli_info and cli_options and used in 'main'.

  cli_files
  Handles config file detection and setup if it is empty or needs updated.

  cli_info
  Prints program info '-i' and build options used.

  cli_options
  Handles command line arg parsing and variable setup. Prints usage message.

*/

#ifndef CLI_HH
#define CLI_HH

#include <fstream>
#include <string>

namespace ShyneboxCli {

struct Options {
  Options();
  int parse(int argc, char** argv);

  std::string session_display;
  std::string rc_path;
  std::string rc_file;
  std::string log_filename;
  bool xsync;
};

void showInfo(std::ostream&);

void setupConfigFiles(const std::string& dirname, const std::string& rc);
void updateConfigFilesIfNeeded(const std::string& rc_file);

}

#endif // CLI_HH

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
