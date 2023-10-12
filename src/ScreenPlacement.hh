// ScreenPlacement.hh for Shynebox Window Manager

/*
  Main class for placement handling
  This is a bridge between screen and the real placement strategy.
  It holds a pointer to the real placement strategy which is
  wrapped around its placeWindow.
  The actual strategy's placeWindow function will modify x,y coords
  for, which are normally sent to a move() for windows.
*/

#ifndef SCREENPLACEMENT_HH
#define SCREENPLACEMENT_HH

#include "PlacementStrategy.hh"
#include "tk/Config.hh"

namespace tk {
class Menu;
}
class BScreen;

class ScreenPlacement: public PlacementStrategy {
public:
  explicit ScreenPlacement(BScreen &screen);

  virtual ~ScreenPlacement() {
    if (m_strategy)
      delete m_strategy;
  }
  void placeWindow(const ShyneboxWindow &window, int head,
                   int &place_x, int &place_y);
  void placeAndShowMenu(tk::Menu& menu, int x, int y);

  #define SPPEnum tk::ScreenPlacementPolicy_e
  SPPEnum placementPolicy() const { return m_placement_policy; }
  tk::RowDirection_e rowDirection() const { return m_row_direction; }
  tk::ColDirection_e colDirection() const { return m_col_direction; }

private:
  // config items
  tk::RowDirection_e &m_row_direction;
  tk::ColDirection_e &m_col_direction;
  tk::ScreenPlacementPolicy_e &m_placement_policy;

  SPPEnum m_old_policy;              // holds old policy, used to determine if config has changed
  PlacementStrategy *m_strategy = 0; // main strategy
  BScreen& m_screen;
  #undef SPPEnum
};

#endif // SCREENPLACEMENT_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2006 Fluxbox Team (fluxgen at fluxbox dot org)
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
