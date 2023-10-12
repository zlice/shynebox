// LayerItem.cc for Shynebox Window Manager

#include "LayerItem.hh"

using namespace tk;

LayerItem::LayerItem(SbWindow &win, Layer &layer) :
      m_layer(&layer) {
  m_windows.push_back(&win);
  m_layer->insert(*this);
} // LayerItem class init


LayerItem::~LayerItem() {
  m_layer->remove(*this);
} // LayerItem class destroy

void LayerItem::setLayer(Layer &layer) {
  // make sure we don't try to set the same layer
  if (m_layer == &layer)
    return;

  m_layer->remove(*this);
  m_layer = &layer;
  m_layer->insert(*this);
}

bool LayerItem::raise() {
  return m_layer->raise(*this);
}

void LayerItem::lower() {
  m_layer->lower(*this);
}

void LayerItem::tempRaise() {
  m_layer->tempRaise(*this);
}

void LayerItem::moveToLayer(int layernum) {
  m_layer->moveToLayer(*this, layernum);
}

void LayerItem::addWindow(SbWindow &win) {
  // I'd like to think we can trust ourselves that it won't be added twice...
  // Otherwise we're always scanning through the list.
  m_windows.push_back(&win);
  m_layer->alignItem(*this); // only use
}

void LayerItem::removeWindow(SbWindow &win) {
  // I'd like to think we can trust ourselves that it won't be added twice...
  // Otherwise we're always scanning through the list.
  // reverse for small vec back=top efficiency
  reverse_iterator it = m_windows.rbegin();
  for (; it != m_windows.rend() ; ++it) {
    if (*it == &win) {
      m_windows.erase(std::next(it).base() );
      return;
    }
  }
}

void LayerItem::bringToTop(SbWindow &win) {
  removeWindow(win);
  addWindow(win);
}

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 - 2006 Henrik Kinnunen (fluxgen at fluxbox dot org)
//                and Simon Bowden    (rathnor at users.sourceforge.net)
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
