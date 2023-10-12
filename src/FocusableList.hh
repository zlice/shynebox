// FocusableList.hh for Shynebox Window Manager

/*
  Creates list of focusables based on ClientPatterns.
  Mostly used for Iconbar, but also key commands, and misc loops.
*/

#ifndef FOCUSABLELIST_HH
#define FOCUSABLELIST_HH

#include "tk/NotCopyable.hh"

#include "ClientPattern.hh"

#include <list>

class BScreen;
class Focusable;
class WinClient;
class ShyneboxWindow;

class FocusableList: private tk::NotCopyable {
public:
  ~FocusableList();
  typedef std::list<Focusable *> Focusables;

  // list option bits
  enum {
    LIST_GROUPS = 0x01,  // list groups(tabs) instead of clients
    STATIC_ORDER = 0x02  // use creation order instead of focused order
  };

  FocusableList(BScreen &scr): m_parent(0), m_screen(scr) { }
  FocusableList(BScreen &scr, const std::string & pat);
  FocusableList(BScreen &scr, const FocusableList &parent,
                const std::string & pat);

  static void parseArgs(const std::string &in, int &opts, std::string &out);
  static const FocusableList *getListFromOptions(BScreen &scr, int opts);

  // functions for modifying the list contents
  void pushFront(Focusable &win);
  void pushBack(Focusable &win);
  void moveToFront(Focusable &win);
  void moveToBack(Focusable &win);
  void remove(Focusable &win);

  Focusables &clientList() { return m_list; }
  const Focusables &clientList() const { return m_list; }

  bool empty() const { return m_list.empty(); }
  bool contains(const Focusable &win) const;
  Focusable *find(const ClientPattern &pattern) const;

  void windowUpdated(ShyneboxWindow &sbwin);
  void reset();

private:
  void addMatching();

  ClientPattern *m_pat = 0;
  const FocusableList *m_parent;
  BScreen &m_screen;
  std::list<Focusable *> m_list;
};

#endif // FOCUSABLELIST_HH

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
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
