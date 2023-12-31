// CascadePlacement.hh for Shynebox Window Manager

/*
  Place by increasing x,y each window
  wrap if new window bounds goes over screen/head
*/

#ifndef CASCADEPLACEMENT_HH
#define CASCADEPLACEMENT_HH

#include "PlacementStrategy.hh"
#include "tk/NotCopyable.hh"

class BScreen;

class CascadePlacement: public PlacementStrategy,
                        private tk::NotCopyable {
public:
  explicit CascadePlacement(const BScreen &screen);
  ~CascadePlacement();
  void placeWindow(const ShyneboxWindow &window, int head,
                   int &place_x, int &place_y);
private:
  int *m_cascade_x;
  int *m_cascade_y;
};

#endif // CASCADEPLACEMENT_HH

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
