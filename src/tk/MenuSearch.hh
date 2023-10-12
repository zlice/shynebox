// MenuSearch.hh for Shynebox Window Manager

/*
  A small helper which applies search operations on a list of MenuItems*.
  Search text is accessed through ITypeAheadable on Focusables and MenuItems.

  MenuSearch is case insensitive and can be one of options found in Config:

  NOWHERE (disabled)
  ITEMSTART
  SOMEWHERE
*/

#ifndef _MENU_SEARCH_HH_
#define _MENU_SEARCH_HH_

#include "Config.hh" // string

#include <vector>
#include <cstddef>

namespace tk {

class MenuItem;

class MenuSearch {
public:
  static void setMode(tk::MenuMode_e m);

  MenuSearch(const std::vector<tk::MenuItem*>& items);

  size_t size() const;
  void clear();
  void add(char c);
  void backspace();

  // is 'pattern' matching something?
  bool has_match();

  // would 'the_pattern' match something?
  bool would_match(const std::string& the_pattern);

  size_t num_matches();

  // returns true if m_text matches against m_items[i] and stores
  // the position where it matches in the string
  bool get_match(size_t i, size_t& idx);

  std::string pattern;

private:
  const std::vector<tk::MenuItem*>& m_items;
};

}
#endif // _MENU_SEARCH_HH

// Copyright (c) 2023 Shynebox - zlice
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
