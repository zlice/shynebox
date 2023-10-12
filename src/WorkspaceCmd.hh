// WorkspaceCmd.hh for Shynebox Window Manager

/*
  Commands realted to Workspaces.
  This includes some non-obvious commands.
  Conditional ClientPattern logic matches like ForEach, Some and Every.
  Arrange/Unclutter (tiling and re-place based on Screen Strategy)
  Toggle toolbar visibilty and auto-hide.
*/

#ifndef WORKSPACECMD_HH
#define WORKSPACECMD_HH
#include "tk/Command.hh"

#include "ClientPattern.hh"
#include "FocusControl.hh"

class WindowListCmd: public tk::Command<void> {
public:
  WindowListCmd(int opts,
                tk::Command<void> &cmd,
                tk::Command<bool> &filter):
          m_opts(opts), m_cmd(&cmd), m_filter(&filter) { }
  ~WindowListCmd() {
    if (m_cmd)
      delete m_cmd;
    if (m_filter)
      delete m_filter;
  }
  void execute();
  static tk::Command<void> *parse(const std::string &command,
                                  const std::string &args, bool trusted);
private:
  int m_opts;
  tk::Command<void> *m_cmd = 0;
  tk::Command<bool> *m_filter = 0;
};

class SomeCmd: public tk::Command<bool> {
public:
  SomeCmd(tk::Command<bool> &cmd): m_cmd(&cmd) { }
  ~SomeCmd() {
    if (m_cmd)
      delete m_cmd;
  }

  bool execute();
  static tk::Command<bool> *parse(const std::string &command,
                                  const std::string &args, bool trusted);
private:
  tk::Command<bool> *m_cmd = 0;
};

class EveryCmd: public tk::Command<bool> {
public:
  EveryCmd(tk::Command<bool> &cmd): m_cmd(&cmd) { }
  ~EveryCmd() {
    if (m_cmd)
      delete m_cmd;
  }
  bool execute();
private:
  tk::Command<bool> *m_cmd = 0;
};

class AttachCmd: public tk::Command<void> {
public:
  explicit AttachCmd(const std::string &pat): m_pat(pat.c_str() ) { }
  void execute();
private:
  const ClientPattern m_pat;
};

class NextWindowCmd: public tk::Command<void> {
public:
  explicit NextWindowCmd(int option, std::string &pat):
          m_option(option), m_pat(pat.c_str() ) { }
  void execute();
private:
  const int m_option;
  const ClientPattern m_pat;
};

class PrevWindowCmd: public tk::Command<void> {
public:
  explicit PrevWindowCmd(int option, std::string &pat):
              m_option(option), m_pat(pat.c_str() ) { }
  void execute();
private:
  const int m_option;
  const ClientPattern m_pat;
};

class GoToWindowCmd: public tk::Command<void> {
public:
  GoToWindowCmd(int num, int option, std::string &pat):
          m_num(num), m_option(option), m_pat(pat.c_str() ) { }
  void execute();
  static tk::Command<void> *parse(const std::string &command,
                              const std::string &args, bool trusted);
private:
  const int m_num;
  const int m_option;
  const ClientPattern m_pat;
};

class AddWorkspaceCmd: public tk::Command<void> {
public:
  void execute();
};

class RemoveLastWorkspaceCmd: public tk::Command<void> {
public:
  void execute();
};

class NextWorkspaceCmd: public tk::Command<void> {
public:
  explicit NextWorkspaceCmd(int option):m_option(option) { }
  void execute();
private:
  const int m_option;
};

class PrevWorkspaceCmd: public tk::Command<void> {
public:
  explicit PrevWorkspaceCmd(int option):m_option(option) { }
  void execute();
private:
  const int m_option;
};

class LeftWorkspaceCmd: public tk::Command<void> {
public:
  explicit LeftWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
  void execute();
private:
  const int m_param;
};

class RightWorkspaceCmd: public tk::Command<void> {
public:
  explicit RightWorkspaceCmd(int num=1):m_param(num == 0 ? 1 : num) { }
  void execute();
private:
  const int m_param;
};

class JumpToWorkspaceCmd: public tk::Command<void> {
public:
  explicit JumpToWorkspaceCmd(int workspace_num);
  void execute();
private:
  const int m_workspace_num;
};

/// arranges windows in current workspace to rows and columns
class ArrangeWindowsCmd: public tk::Command<void> {
public:
  enum {
    UNSPECIFIED,
    VERTICAL,
    HORIZONTAL,
    STACKLEFT,
    STACKRIGHT,
    STACKTOP,
    STACKBOTTOM
  };
  explicit ArrangeWindowsCmd(int tile_method, std::string &pat):
          m_tile_method( tile_method ), m_pat(pat.c_str() ) { }
  void execute();
private:
  const int m_tile_method;
  const ClientPattern m_pat;
};

class UnclutterCmd: public tk::Command<void> {
public:
  explicit UnclutterCmd(std::string &pat): m_pat(pat.c_str() ) { }
  void execute();
private:
  const ClientPattern m_pat;
};

class ShowDesktopCmd: public tk::Command<void> {
public:
  void execute();
};

class ToggleToolbarAboveCmd: public tk::Command<void> {
public:
  void execute();
};

class ToggleToolbarHiddenCmd: public tk::Command<void> {
public:
  void execute();
};

class CloseAllWindowsCmd: public tk::Command<void> {
public:
  void execute();
};

class WorkspaceNameDialogCmd: public tk::Command<void> {
public:
  void execute();
};

#endif // WORKSPACECMD_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden (rathnor at users.sourceforge.net)
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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
