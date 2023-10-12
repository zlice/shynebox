// FocusableList.cc for Shynebox Window Manager

#include "FocusableList.hh"

#include "Focusable.hh"
#include "FocusControl.hh"
#include "Screen.hh"
#include "WinClient.hh"
#include "Window.hh"

#include "tk/StringUtil.hh"

#include <vector>

#include "Debug.hh"

#ifdef HAVE_CSTRING
  #include <cstring>
#else
  #include <string.h>
#endif

using std::string;
using std::vector;

void FocusableList::parseArgs(const string &in, int &opts, string &pat) {
  string options;
  int err = tk::StringUtil::getStringBetween(options, in.c_str(),
                                             '{', '}', " \t\n");

  // the rest of the string is a ClientPattern
  pat = in.c_str() + (err > 0 ? err : 0);

  // now parse the options
  vector<string> args;
  tk::StringUtil::stringtok(args, options);

  opts = 0;
  for (auto &it : args) {
    if (strcasecmp(it.c_str(), "static") == 0)
      opts |= STATIC_ORDER;
    else if (strcasecmp(it.c_str(), "groups") == 0)
      opts |= LIST_GROUPS;
  }
}

const FocusableList *FocusableList::getListFromOptions(BScreen &scr, int opts) {
  if (opts & LIST_GROUPS)
    return (opts & STATIC_ORDER) ?
            &scr.focusControl().creationOrderWinList() :
            &scr.focusControl().focusedOrderWinList();
  return (opts & STATIC_ORDER) ?
          &scr.focusControl().creationOrderList() :
          &scr.focusControl().focusedOrderList();
}

FocusableList::FocusableList(BScreen &scr, const string & pat):
      m_parent(0), m_screen(scr) {
  int options = 0;
  string pattern;
  parseArgs(pat, options, pattern);
  m_parent = getListFromOptions(scr, options);
  if (m_pat)
    delete m_pat;
  m_pat = new ClientPattern(pattern.c_str() );

  addMatching();
}

FocusableList::FocusableList(BScreen &scr, const FocusableList &parent,
                                                    const string & pat):
      m_pat(new ClientPattern(pat.c_str() ) ),
      m_parent(&parent), m_screen(scr) {
  addMatching();
} // FocusableList class init

FocusableList::~FocusableList() {
  if (m_pat)
    delete m_pat;
} // FocusableList class destroy

void FocusableList::addMatching() {
  if (!m_parent)
    return;

  const Focusables &list = m_parent->clientList();

  for (auto &it : list) {
    if (m_pat->match(*it) ) {
      m_list.push_back(it);
      m_pat->addMatch();
    }
  }
}

void FocusableList::pushFront(Focusable &win) {
  m_list.push_front(&win);
}

void FocusableList::pushBack(Focusable &win) {
  m_list.push_back(&win);
}

void FocusableList::moveToFront(Focusable &win) {
  // if the window isn't already in this list, we could accidentally add it
  if (!contains(win) )
    return;

  m_list.remove(&win);
  m_list.push_front(&win);
}

void FocusableList::moveToBack(Focusable &win) {
  // if the window isn't already in this list, we could accidentally add it
  if (!contains(win) )
    return;

  m_list.remove(&win);
  m_list.push_back(&win);
}

void FocusableList::remove(Focusable &win) {
  // if the window isn't already in this list, we could send a bad signal
  bool contained = contains(win);

  if (!contained)
    return;

  m_list.remove(&win);
}

void FocusableList::reset() {
  m_list.clear();
  if (m_pat)
    m_pat->resetMatches();
  if (m_parent)
    addMatching();
}

bool FocusableList::contains(const Focusable &win) const {
  auto it = std::find(m_list.begin(), m_list.end(), &win);
  return it != m_list.end();
}

Focusable *FocusableList::find(const ClientPattern &pat) const {
  for (auto &it : m_list)
    if (pat.match(*it) )
      return it;
  return 0;
}

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
