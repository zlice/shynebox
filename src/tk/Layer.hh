// Layer.hh for Shynebox Window Manager

/*
  Holds a list of 'LayerItems' in ~this~ layer
  (items may be multiple items/windows themselves)
  This is where actual X restacking is done
*/

#ifndef TK_LAYER_HH
#define TK_LAYER_HH

#include <vector>
#include <list>

namespace tk {

class LayerManager;
class LayerItem;

class Layer {
public:
  Layer(LayerManager &manager, int layernum);
  ~Layer();

  typedef std::vector<LayerItem *> ItemList;
  typedef std::vector<LayerItem *>::iterator iterator;
  typedef std::vector<LayerItem *>::reverse_iterator reverse_iterator;
  // reverse is a bit better performance for reverse order vector list
  // front = bottom, back = top

  void setLayerNum(int layernum) { m_layernum = layernum; };
  int  getLayerNum() const { return m_layernum; };
  // Put all items on the same layer (called when layer item added to)
  void alignItem(LayerItem &item);
  void stackBelowItem(LayerItem &item, LayerItem *above);
  LayerItem *getLowestItem();
  const ItemList &itemList() const { return m_items; }
  ItemList &itemList() { return m_items; }

  // we redefine these as Layer has special optimisations, and X restacking needs
  void insert(LayerItem &item);
  void remove(LayerItem &item);

  // bring to top of layer
  bool raise(LayerItem &item);
  void lower(LayerItem &item);

  // raise it, but don't make it permanent (i.e. restack will revert)
  void tempRaise(LayerItem &item);

  // send to next layer up
  void raiseLayer(LayerItem &item);
  void lowerLayer(LayerItem &item);
  void moveToLayer(LayerItem &item, int layernum);

  static void restack(const std::vector<Layer*>& layers);

private:
  void restack();
  void restackAndTempRaise(LayerItem &item);

  LayerManager &m_manager;
  int m_layernum;
  bool m_needs_restack;
  ItemList m_items;
};

} // namespace tk

#endif // TK_LAYER_HH

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
