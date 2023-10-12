// SbCommands.hh for Shynebox Window Manager

/*
  Basic commands to restart, reconfigure, execute (shell), etc.
  Used in 'keys' file.
*/

#ifndef SBCOMMANDS_HH
#define SBCOMMANDS_HH

#include "tk/Command.hh"
#include "ClientPattern.hh"
#include "FocusableList.hh"

class ClientMenu;
class SbMenu;

namespace SbCommands {

/// executes a system command
class ExecuteCmd: public tk::Command<void> {
public:
  ExecuteCmd(const std::string &cmd, int screen_num = -1);
  void execute();
  int run(); // same as execute but returns pid
private:
  std::string m_cmd;
  const int m_screen_num;
};

/// exit shynebox
class ExitShyneboxCmd: public tk::Command<void> {
public:
  void execute();
};

/// saves resources
class SaveResources: public tk::Command<void> {
public:
  void execute();
};

/// restarts shynebox
class RestartShyneboxCmd: public tk::Command<void> {
public:
  RestartShyneboxCmd(const std::string &cmd);
  void execute();
  static tk::Command<void> *parse(const std::string &command,
                                    const std::string &args, bool trusted);
private:
  std::string m_cmd;
};

/// reconfigures shynebox
class ReconfigureShyneboxCmd: public tk::Command<void> {
public:
  void execute();
};

class ReloadStyleCmd: public tk::Command<void> {
public:
  void execute();
};

class SetStyleCmd: public tk::Command<void> {
public:
  explicit SetStyleCmd(const std::string &filename);
  void execute();
private:
  std::string m_filename;
};

class KeyModeCmd: public tk::Command<void> {
public:
  explicit KeyModeCmd(const std::string &arguments);
  void execute();
private:
  std::string m_keymode;
  std::string m_end_args;
};

class HideMenuCmd: public tk::Command<void> {
public:
  void execute();
};

class ShowClientMenuCmd: public tk::Command<void> {
public:
  ShowClientMenuCmd(int option, std::string &pat):
    m_option(option|FocusableList::LIST_GROUPS), m_pat(pat.c_str() ) { }

  void execute();
  static tk::Command<void> *parse(const std::string &command,
                              const std::string &args, bool trusted);
private:
  const int m_option;
  const ClientPattern m_pat;
  std::list<ShyneboxWindow *> m_list;
  ClientMenu *m_menu = 0; // this will get destroyed by menus
};

class ShowCustomMenuCmd: public tk::Command<void> {
public:
  explicit ShowCustomMenuCmd(const std::string &arguments);

  void execute();
  void reload();
private:
 std::string custom_menu_file;
 SbMenu *m_menu = 0; // this will get destroyed by menus
};

class ShowRootMenuCmd: public tk::Command<void> {
public:
  void execute();
};

class ShowWorkspaceMenuCmd: public tk::Command<void> {
public:
  void execute();
};

} // end namespace SbCommands

#endif // SBCOMMANDS_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
