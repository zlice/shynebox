// LogicCommands.hh for Shynebox Window Manager

/*
  Executes a Command<bool> and uses the result to decide what to do
  Mostly for ClientPattern matches
*/

#ifndef TK_LOGICCOMMANDS_HH
#define TK_LOGICCOMMANDS_HH

#include "Command.hh"

#include <string>
#include <vector>

namespace tk {

class IfCommand: public Command<void> {
public:
  IfCommand(Command<bool> &cond,
            Command<void> &t, Command<void> &f):
      m_cond(&cond), m_t(&t), m_f(&f) { }
  void execute() {
    if (m_cond->execute() ) {
      if (m_t)
        m_t->execute();
    }
    else if (m_f)
      m_f->execute();
  }
  static Command<void> *parse(const std::string &cmd,
                              const std::string &args, bool trusted);
private:
  Command<bool> *m_cond = 0;
  Command<void> *m_t = 0, *m_f = 0;
};

// executes a list of Command<bool>s until one is true
class OrCommand: public Command<bool> {
public:
  void add(Command<bool> &com);
  size_t size() const;
  bool execute();
private:
  std::vector<Command<bool> *> m_commandlist;
};

// executes a list of Command<bool>s until one is false
class AndCommand: public Command<bool> {
public:
  void add(Command<bool> &com);
  size_t size() const;
  bool execute();
private:
  std::vector<Command<bool> *> m_commandlist;
};

// executes a list of Command<bool>s, returning the parity
class XorCommand: public Command<bool> {
public:
  void add(Command<bool> &com);
  size_t size() const;
  bool execute();
private:
  std::vector<Command<bool> *> m_commandlist;
};

// executes a Command<bool> and returns the negation
class NotCommand: public Command<bool> {
public:
  NotCommand(Command<bool> &com): m_command(&com) { }
  bool execute() { return !m_command->execute(); }
private:
  Command<bool> *m_command;
};

} // end namespace tk

#endif // TK_LOGICCOMMANDS_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2007 Fluxbox Team (fluxgen at fluxbox dot org)
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
