// LayerItem.hh for Shynebox Window Manager

/*
  Represents a window (and its transients)
  It can move to another layer and re-order
  Though this mainly applies to special windows like the toolbar
*/

#ifndef TK_LAYERITEM_HH
#define TK_LAYERITEM_HH

#include "Layer.hh"
#include "NotCopyable.hh"
#include <vector>
#include <cstdlib> // size_t

namespace tk {

class SbWindow;

class LayerItem : private NotCopyable {
public:
  typedef std::vector<SbWindow *> Windows;
  typedef std::vector<SbWindow *>::reverse_iterator reverse_iterator;

  LayerItem(SbWindow &win, Layer &layer);
  ~LayerItem();

  void setLayer(Layer &layer);

  bool raise();
  void lower();
  void tempRaise(); // this raise gets reverted by a restack()

  void moveToLayer(int layernum);

  const Layer &getLayer() const { return *m_layer; }
  Layer &getLayer() { return *m_layer; }
  int getLayerNum() { return m_layer->getLayerNum(); }

  // a LayerItem holds several windows that are equivalent in a layer
  // (i.e. if one is raised, then they should all be).
  void addWindow(SbWindow &win);
  void removeWindow(SbWindow &win);

  // using this you can bring one window to the top of this item (equivalent to add then remove)
  void bringToTop(SbWindow &win);

  Windows &getWindows() { return m_windows; }

private:
  Layer *m_layer;
  Windows m_windows;
};

} // namespace tk

#endif // TK_LAYERITEM_HH

// Copyright (c) 2023 Shynebox - zlice
//
// Copyright (c) 2003 Henrik Kinnunen (fluxgen at fluxbox dot org)
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
