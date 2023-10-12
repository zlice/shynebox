// HeadArea.cc for Shynebox Window Manager

#include "HeadArea.hh"
#include "Strut.hh"

#include <iostream>

HeadArea::HeadArea() : m_available_workspace_area(new Strut(0,0,0,0,0) )
{ } // HeadArea class init

HeadArea::~HeadArea() {
  delete m_available_workspace_area;
} // HeadArea class destroy

Strut *HeadArea::requestStrut(int head, int left, int right, int top, int bottom, Strut* next) {
  Strut *str = new Strut(head, left, right, top, bottom, next);
  m_strutlist.push_back(str);
  return str;
}

void HeadArea::clearStrut(Strut *str) {
  if (str == 0)
    return;

  // find strut and erase it
  std::list<Strut *>::iterator it = m_strutlist.begin();
  for (; it != m_strutlist.end() ; ++it) {
    if (*it == str) {
      m_strutlist.erase(it);
      delete str;
      return;
    }
  }

  std::cerr << "clearStrut() failed because the strut was not found\n";
}

bool HeadArea::updateAvailableWorkspaceArea() {
  // find max of left, right, top and bottom and set avaible workspace area

  // clear old area
  Strut oldarea = *(m_available_workspace_area);
  if (m_available_workspace_area)
    delete m_available_workspace_area;
  m_available_workspace_area = new Strut(0, 0, 0, 0, 0);

  // calculate max area
  for (auto it : m_strutlist) {
    int l = std::max(m_available_workspace_area->left(),   it->left() ),
        r = std::max(m_available_workspace_area->right(),  it->right() ),
        b = std::max(m_available_workspace_area->bottom(), it->bottom() ),
        t = std::max(m_available_workspace_area->top(),    it->top() );
    *m_available_workspace_area = Strut(0, l, r, t, b);
  }

  // only notify if the area changed
  return !(oldarea == *(m_available_workspace_area) );
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2004 - 2006 Mathieu De Zutter (mathieu at dezutter dot org)
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
