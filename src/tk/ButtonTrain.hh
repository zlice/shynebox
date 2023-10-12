// ButtonTrain.hh for Shynebox Window Manager

/*
  This holds buttons in a row (like a train). Used for tabs and iconbar.
  Toolbar also holds items but can hold iconbar which is of type ButtonTrain.
  Dynamically calculates positioning placement and sizes per item.
*/

#ifndef TK_BTNTRAIN_HH
#define TK_BTNTRAIN_HH

#include "SbWindow.hh"
#include "EventHandler.hh"
#include "NotCopyable.hh"
#include "Orientation.hh"
#include "Config.hh"

#include <list>
#include <functional>

namespace tk {

class Button;

class ButtonTrain: public SbWindow, public EventHandler, private NotCopyable {
public:
  // LEFT, CENTER, RIGHT => fixed button size
  // RELATIVE/SMART => relative/variable button size
  typedef Button * Item;
  typedef std::list<Item> ItemList;

  explicit ButtonTrain(const SbWindow &parent, bool auto_resize = true);
  virtual ~ButtonTrain();

  void resize(unsigned int width, unsigned int height);
  void moveResize(int x, int y,
                  unsigned int width, unsigned int height);

  void insertItem(Item item);
  bool removeItem(Item item); // return true if something was removed
  void removeAll();
  void moveItem(Item item, int movement);
  int find(const Button * item);
  void setMaxSizePerClient(unsigned int size);
  void setMaxTotalSize(unsigned int size);
  void setAlignment(tk::ButtonTrainAlignment_e a);
  void setOrientation(Orientation orient);

  Item back() { return m_item_list.back(); }

  void update() { repositionItems(); }
  // so we can add items without having an graphic update for each item
  void setUpdateLock(bool value) { m_update_lock = value; }

  // event handler
  void exposeEvent(XExposeEvent &event);
  // for use when embedded in something that may passthrough
  bool tryExposeEvent(XExposeEvent &event);

  void invalidateBackground();

  // accessors
  tk::ButtonTrainAlignment_e alignment() const { return m_align; }
  Orientation orientation() const { return m_orientation; }
  int size() const { return m_item_list.size(); }
  bool empty() const { return m_item_list.empty(); }
  unsigned int maxWidthPerClient() const;
  bool updateLock() const { return m_update_lock; }

  ItemList::iterator begin() { return m_item_list.begin(); }
  ItemList::iterator end() { return m_item_list.end(); }

  void clear(); // clear all windows
  void repositionItems();

private:
  Orientation m_orientation;

  tk::ButtonTrainAlignment_e m_align;
  unsigned int m_max_size_per_client;
  unsigned int m_max_total_size;
  ItemList m_item_list;
  bool m_update_lock, m_auto_resize;
};

} // end namespace tk

#endif // TK_BTNTRAIN_HH

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
