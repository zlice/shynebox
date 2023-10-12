// MinOverlapPlacement.hh for Shynebox Window Manager

/*
  Places windows based on area covered by un-minimized windows.
  This is probably the most math heavy placement to avoid the most overlap.
*/

#ifndef MINOVERLAPPLACEMENT_HH
#define MINOVERLAPPLACEMENT_HH

#include "ScreenPlacement.hh"

class MinOverlapPlacement: public PlacementStrategy {
public:
  MinOverlapPlacement() { };
  void placeWindow(const ShyneboxWindow &win, int head,
                   int &place_x, int &place_y);
};

#endif // MINOVERLAPPLACEMENT_HH

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
