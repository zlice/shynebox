// Layer.cc for Shynebox Window Manager

#include "Layer.hh"
#include "LayerItem.hh"
#include "App.hh"
#include "SbWindow.hh"
#include "LayerManager.hh"

#include <iostream>
#include <numeric>

using namespace tk;

#ifdef DEBUG
using std::cerr;
#endif

namespace {

void extract_windows_to_stack(const LayerItem::Windows& windows, std::vector<Window>& stack) {
  for (auto it : windows)
    stack.push_back(it->window() );
}

void extract_windows_to_stack(const tk::Layer::ItemList& items,
            LayerItem* temp_raised, std::vector<Window>& stack) {
  // add windows that go on top
  if (temp_raised)
    extract_windows_to_stack(temp_raised->getWindows(), stack);

  // add all the windows from each other item
  for (int i=items.size()-1 ; i >= 0 ; i--) {
    if (items[i] == temp_raised)
      continue;
    extract_windows_to_stack(items[i]->getWindows(), stack);
  }
}

void restack(const tk::Layer::ItemList& items, LayerItem* temp_raised) {
  std::vector<Window> stack;
  extract_windows_to_stack(items, temp_raised, stack);

  if (!stack.empty() )
    XRestackWindows(tk::App::instance()->display(), &stack[0], stack.size() );
}

} // end of anonymous namespace


void Layer::restack(const std::vector<Layer*>& layers) {
  std::vector<Window> stack;

  for (auto l : layers)
    extract_windows_to_stack(l->itemList(), 0, stack);

  if (!stack.empty() )
    XRestackWindows(tk::App::instance()->display(), &stack[0], stack.size() );
}

Layer::Layer(LayerManager &manager, int layernum):
  m_manager(manager), m_layernum(layernum), m_needs_restack(false) { }
// Layer class init

Layer::~Layer() { } // Layer class destroy

void Layer::restack() {
  if (m_manager.isUpdatable() ) {
    ::restack(itemList(), 0);
    m_needs_restack = false;
  }
}

void Layer::restackAndTempRaise(LayerItem &item) {
  ::restack(itemList(), &item);
}

// Stack all windows associated with 'item' below the 'above' item
void Layer::stackBelowItem(LayerItem &item, LayerItem *above) {
  if (!m_manager.isUpdatable() )
    return;

  // if there are no windows provided for above us,
  // then we must restack the entire layer
  // we can't do XRaiseWindow because a restack then causes OverrideRedirect
  // windows to get pushed to the bottom
  if (!above || m_needs_restack) { // must need to go right to top
    restack();
    return;
  }

  std::vector<Window> stack;

  // We do have a window to stack below
  // so we put it on top, and fill the rest of the array with the ones to go below it.
  // assume that above's window exists
  stack.push_back(above->getWindows().back()->window() );

  // fill the rest of the array
  extract_windows_to_stack(item.getWindows(), stack);

  XRestackWindows(tk::App::instance()->display(), &stack[0], stack.size() );
}

// only used once in LayerItem
// keep order of X windows
// external tabs or ShowDesktop toggles behave wildly without this
void Layer::alignItem(LayerItem &item) {
  if (m_items.back() == &item) {
    // already top of layer, normal new/focused windows
    stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum) );
    return;
  }

  // search from top (which is back of ItemList)
  reverse_iterator it = m_items.rbegin();
  for (; it != m_items.rend() ; ++it)
    if (*it == &item)
      break;

  --it; // go one item above (back is top, reverse-iter, so decrement)

  if (it == m_items.rbegin() ) // reached top-most item, therefore it was already raised
    stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum) );
  else // stack below item
    stackBelowItem(item, *it);
}

void Layer::insert(LayerItem &item) {
  m_items.push_back(&item); // reverse bot<>top for our vec-list for efficiency
  // restack below next window up
  stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum) );
}

void Layer::remove(LayerItem &item) {
  // reverse is a smidge faster (20ns) for a few more bytes
  // likely you're removing from top
  reverse_iterator it = m_items.rbegin();
  for (; it != m_items.rend() ; ++it) {
    if (*it == &item) {
      m_items.erase(std::next(it).base() );
      return;
    }
  }
}

bool Layer::raise(LayerItem &item) {
  // assume it is already in this layer
  if (&item == m_items.back() ) {
    if (m_needs_restack) {
      restack();
      return true;
    }
    return false; // nothing to do
  }

  size_t items_sz = m_items.size();
  remove(item);
  if (items_sz == m_items.size() ) { // not found
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): WARNING: raise on item not in layer["<<m_layernum<<"]\n";
#endif // DEBUG
    return false;
  }

  m_items.push_back(&item);
  stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum) );
  return true;
}

void Layer::tempRaise(LayerItem &item) {
  // assume it is already in this layer
  if (!m_needs_restack && &item == m_items.back() )
    return; // nothing to do

  iterator it = m_items.begin();
  for (; it != m_items.end() ; ++it)
    if (*it == &item)
      break;
  if (it == m_items.end() ) { // not found
#ifdef DEBUG
    cerr<<__FILE__<<"("<<__LINE__<<"): WARNING: raise on item not in layer["<<m_layernum<<"]\n";
#endif // DEBUG
    return;
  }

  if (m_needs_restack)
    restackAndTempRaise(item);
  else
    stackBelowItem(item, m_manager.getLowestItemAboveLayer(m_layernum) );

  m_needs_restack = true;
}

void Layer::lower(LayerItem &item) {
  // assume already in this layer
  // is it already the lowest?
  if (&item == m_items.front() ) {
    if (m_needs_restack)
      restack();
    return; // nothing to do
  }

#ifdef DEBUG
  size_t items_sz = m_items.size();
#endif // DEBUG

  remove(item);

#ifdef DEBUG
  if (items_sz == m_items.size() ) { // not found
    cerr<<__FILE__<<"("<<__LINE__<<"): WARNING: lower on item not in layer\n";
    return;
  }
#endif // DEBUG

  // add it to the bottom
  m_items.insert(m_items.begin(), &item); // push_front for vector, inefficient

  // find the item we need to stack below
  // start at the end
  iterator it = m_items.begin();

  // go up one so we have an object
  it++; // (which must exist, since at least this item is in the layer)
  // go up another one
  // must exist, otherwise our item == m_items.front()
  it++;

  // and restack our window below that one.
  stackBelowItem(item, *it);
}

void Layer::moveToLayer(LayerItem &item, int layernum) {
  m_manager.moveToLayer(item, layernum);
}

LayerItem *Layer::getLowestItem() {
//  if (m_items.empty() )
//    return 0;
//  else
//    return m_items.front();
  return m_items.size() ? m_items.front() : 0;
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
