// MacroCommand.hh for Shynebox Window Manager

/*
  MacroCommand is a list of multiple Commands to run.

  ToggleCommand is a list of commands to roll to the next each execution.
*/

#ifndef TK_MACROCOMMAND_HH
#define TK_MACROCOMMAND_HH

#include "Command.hh"

#include <vector>
#include <cstdlib> // size_t

namespace tk {

// executes a list of commands
class MacroCommand:public Command<void> {
public:
  ~MacroCommand();
  void add(Command<void> &com);
  size_t size() const;
  virtual void execute();
private:
  std::vector<Command<void> *> m_commandlist;
};

// executes one command, next execute will do the other
class ToggleCommand:public Command<void> {
public:
  ToggleCommand();
  ~ToggleCommand();
  void add(Command<void> &com);
  size_t size() const;
  virtual void execute();
private:
  std::vector<Command<void> *> m_commandlist;
  size_t m_state;
};

} // end namespace tk

#endif // TK_MACROCOMMAND_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
