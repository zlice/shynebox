// LayerManager.hh for Shynebox Window Manager

/*
  Essentially just an interface between the other layer classes.
  Screen + Toolbar(LayerMenu 'LayerObject') are the main users.
*/

#ifndef TK_LAYERMANGER_HH
#define TK_LAYERMANGER_HH

#include <vector>
#include <cstdlib> // size_t

namespace tk {

class LayerItem;
class Layer;

class LayerManager {
public:
  explicit LayerManager(int numlayers);
  ~LayerManager();
  LayerItem *getLowestItemAboveLayer(int layernum);

  void remove(LayerItem &item);

  void moveToLayer(LayerItem &item, int layernum);

  Layer *getLayer(size_t num);
  const Layer *getLayer(size_t num) const;

  bool isUpdatable() const { return m_lock == 0; }
  void lock() { ++m_lock; }
  void unlock() { if (--m_lock == 0) restack(); }

private:
  void restack();

  std::vector<Layer *> m_layers;
  int m_lock;
};

}
#endif // TK_LAYERMANGER_HH

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
