// ButtonTrain.cc for Shynebox Window Manager

#include "ButtonTrain.hh"

#include "Button.hh"
#include "TextButton.hh"
#include "TextUtils.hh"
#include "EventManager.hh"

#include <vector>

#define ContAlignEnum ButtonTrainAlignment_e

namespace tk {

ButtonTrain::ButtonTrain(const SbWindow &parent, bool auto_resize):
      SbWindow(parent, 0, 0, 1, 1, ExposureMask),
      m_orientation(ROT0),
      m_align(ContAlignEnum::RELATIVE),
      m_max_size_per_client(60),
      m_max_total_size(0),
      m_update_lock(false),
      m_auto_resize(auto_resize) {
  EventManager::instance()->add(*this, *this);
} // ButtonTrain class init

ButtonTrain::~ButtonTrain() { } // ButtonTrain class destroy
// ^ ~SbWindow cleans event manager

void ButtonTrain::resize(unsigned int width, unsigned int height) {
  if (SbWindow::width() == width && SbWindow::height() == height)
    return;

  SbWindow::resize(width, height);
  repositionItems();
}

void ButtonTrain::moveResize(int x, int y,
                           unsigned int width, unsigned int height) {
  SbWindow::moveResize(x, y, width, height);
  repositionItems();
}

void ButtonTrain::insertItem(Item item) {
  if (find(item) != -1 || item->parent() != this)
    return;

  item->setOrientation(m_orientation);
  m_item_list.push_back(item);

  // make sure we dont have duplicate items
  m_item_list.unique();

  repositionItems();
}

// currently only used for frame tab moves
void ButtonTrain::moveItem(Item item, int movement) {
  if (m_item_list.empty() )
    return;

  int index = find(item);
  const size_t size = m_item_list.size();

  if (index < 0 || (movement % static_cast<signed>(size) ) == 0)
    return;

  int newindex = (index + movement) % static_cast<signed>(size);
  if (newindex < 0) // neg wrap
    newindex += size;

  ItemList::iterator it = begin();
  for (; it != end() ; ++it)
    if (*it == item)
      break;
  m_item_list.erase(it);

  for (it = begin(); newindex >= 0; ++it, --newindex)
    if (newindex == 0)
      break;

  m_item_list.insert(it, item);
  repositionItems();
}

// frame tabs use ret value
bool ButtonTrain::removeItem(Item item) {
  ItemList::iterator it = begin();
  for (; it != end() ; ++it) {
    if (*it == item) {
      m_item_list.erase(it);
      repositionItems();
      return true;
    }
  }
  return false;
}

void ButtonTrain::removeAll() {
  m_item_list.clear();
  if (!m_update_lock)
    clear();
}

int ButtonTrain::find(const Button * item) {
  ItemList::iterator it = begin();
  int index = 0;
  for (; it != end(); ++it, ++index)
    if ((*it) == item)
      return index;
  return -1;
}

void ButtonTrain::setMaxSizePerClient(unsigned int size) {
  if (size != m_max_size_per_client) {
    m_max_size_per_client = size;
    repositionItems();
  }
}

void ButtonTrain::setMaxTotalSize(unsigned int size) {
  if (m_max_total_size == size)
    return;

  m_max_total_size = size;

  repositionItems();
  return;
}

void ButtonTrain::setAlignment(ContAlignEnum a) {
  if (m_align != a) {
    m_align = a;
    repositionItems();
  }
}

void ButtonTrain::exposeEvent(XExposeEvent &event) {
  if (!m_update_lock)
    clearArea(event.x, event.y, event.width, event.height);
}

// currently only used by frame
bool ButtonTrain::tryExposeEvent(XExposeEvent &event) {
  if (event.window == window() ) {
    exposeEvent(event);
    return true;
  }

  for (auto it : m_item_list) {
    if (it->window() == event.window) {
      it->exposeEvent(event);
      return true;
    }
  }
  return false;
}

void ButtonTrain::repositionItems() {
  if (empty() || m_update_lock)
    return;

  // NOTE: all calculations here are done in non-rotated space

  const size_t num_items = m_item_list.size();
  unsigned int max_width_per_client = maxWidthPerClient(),
               borderW = m_item_list.front()->borderWidth(),
               total_width, cur_width, height;

  // unrotate
  if (m_orientation <= ROT180) { // horz
    total_width = cur_width = width();
    height = this->height();
  } else {
    total_width = cur_width = this->height();
    height = width();
  }

  // if we have a max total size, then we must also resize ourself
  // within that bound
  ContAlignEnum align = alignment();

  // if LEFT || CENTER || RIGHT
  if (m_max_total_size && align <= ContAlignEnum::RIGHT) {
    total_width = (max_width_per_client + borderW) * num_items - borderW;
    if (total_width > m_max_total_size) {
      total_width = m_max_total_size;
      if (m_max_total_size > ((num_items - 1)*borderW) ) // don't go negative with unsigned nums
        max_width_per_client = (m_max_total_size - (num_items - 1)*borderW ) / num_items;
      else
        max_width_per_client = 1;
    }
    // auto-resize in case item was removed
    if (m_auto_resize && total_width != cur_width) {
      // calling ButtonTrain::resize here risks infinite loops
      unsigned int neww = total_width, newh = height;
      translateSize(m_orientation, neww, newh);
      if (!(align == ContAlignEnum::LEFT
            && (m_orientation == ROT0 || m_orientation == ROT90) )
          &&
          !(align == ContAlignEnum::RIGHT
            && (m_orientation == ROT180 || m_orientation == ROT270) ) ) {
        int deltax = 0;
        int deltay = 0;
        if (m_orientation <= ROT180) // horz
          deltax = - (total_width - cur_width);
        else // vert
          deltay = - (total_width - cur_width);
        // NOTE: rounding errors could accumulate in this process
        //       but it should only be an off by 1 center pixel
        //       nothing can really be done about odd sized windows here
        if (align == ContAlignEnum::CENTER) {
          deltax = deltax/2;
          deltay = deltay/2;
        }
        SbWindow::moveResize(x() + deltax, y() + deltay, neww, newh);
        cur_width = width();
      } else
        SbWindow::resize(neww, newh);
      // if rotate check
    } // if auto_resize and total_width != cur_width
  } // if max_total_size and (LEFT || CENTER || RIGHT)

  int rounding_error = 0;
  if (align == ContAlignEnum::RELATIVE || total_width == m_max_total_size)
    rounding_error = total_width -
           ((max_width_per_client + borderW)* num_items - borderW);

  int next_x = -borderW; // zero so the border of the first shows
  // just push LEFT to the right basically
  if (align == ContAlignEnum::RIGHT)
    next_x += cur_width - total_width;
  else if (align == ContAlignEnum::CENTER)
    next_x += std::max(0, (signed)((cur_width/2) - (total_width/2) ) );
    // stay in left bounds of center - (num_items * max_width_per_client) / 2;

  int tmpx, tmpy;
  unsigned int tmpw, tmph;
  unsigned int totalDemands = 0;
  std::vector<unsigned int> buttonDemands;

  if (align == ContAlignEnum::RELATIVE_SMART && total_width == m_max_total_size) {
    buttonDemands.reserve(num_items);
    for (auto it : m_item_list) {
      buttonDemands.push_back(it->preferredWidth() );
      totalDemands += buttonDemands.back();
    }
    if (totalDemands) {
      int overhead = totalDemands - total_width;
      if (overhead > int(buttonDemands.size() ) ) {
        // try to be fair. If we're short on space and some items
        // take > 150% of the average, we preferably shrink them, so
        // "a" and "a very long item with useless information" won't
        // become "a very long item with" and ""
        overhead += buttonDemands.size(); // compensate for rounding errors
        const unsigned int mean = totalDemands / buttonDemands.size();
        const unsigned int thresh = 3 * mean / 2;
        int greed = 0;
        for (size_t i = 0; i < buttonDemands.size(); ++i)
          if (buttonDemands.at(i) > thresh)
            greed += buttonDemands.at(i);

        if (greed) {
          for (size_t i = 0; i < buttonDemands.size(); ++i) {
            if (buttonDemands.at(i) > thresh) {
              int d = buttonDemands.at(i)*overhead/greed;
              if (buttonDemands.at(i) > mean + d) {
                buttonDemands.at(i) -= d;
              } else { // do not shrink below mean or a huge item number would super-punish larger ones
                d = buttonDemands.at(i) - mean;
                buttonDemands.at(i) = mean;
              }
              totalDemands -= d;
            }
          }
        } // greed
      } // overhead > buttonDemands.size()
      rounding_error = total_width;
      for (size_t i = 0; i < buttonDemands.size(); ++i)
        rounding_error -= buttonDemands.at(i)*total_width/totalDemands;
    } // if totalDemands
  } // if align RELATIVE_SMART and total_width == max_total_size

  int i = 0;
  for (auto &it : m_item_list) {
    // we only need to do rounding adds with alignment RELATIVE
    // OR with max_total_size triggered
    tmpw = 0;
    if (rounding_error) {
      --rounding_error;
      ++tmpw;
    }
    // rotate the x and y coords
    tmpx = next_x;
    tmpy = -borderW;
    if ((align == ContAlignEnum::RELATIVE || align == ContAlignEnum::RELATIVE_SMART)
         && totalDemands)
      tmpw += buttonDemands.at(i)*total_width/totalDemands;
    else
      tmpw += max_width_per_client;
    tmph = height;
    next_x += tmpw + borderW;

    translateCoords(m_orientation, tmpx, tmpy, total_width, height);
    translatePosition(m_orientation, tmpx, tmpy, tmpw, tmph, borderW);
    translateSize(m_orientation, tmpw, tmph);

    // resize each clients including border in size (does a clear)
    it->moveResize(tmpx, tmpy, tmpw, tmph);
    i++;
  } // for itemlist
} // repositionItems

unsigned int ButtonTrain::maxWidthPerClient() const {
  switch (alignment() ) {
  default:
  case ContAlignEnum::RIGHT:
  case ContAlignEnum::CENTER:
  case ContAlignEnum::LEFT:
    return m_max_size_per_client;
    break;
  case ContAlignEnum::RELATIVE_SMART:
  case ContAlignEnum::RELATIVE:
    if (size() == 0)
      return width();
    else {
      unsigned int tot_bw = m_item_list.front()->borderWidth()
                             * (size() - 1);
      unsigned int w = width(), h = height();
      translateSize(m_orientation, w, h);
      if (w < tot_bw)
        return 1;
      else
        return (w - tot_bw) / size();
    }
    break;
  }

  // this will never happen anyway
  return 1;
}

void ButtonTrain::invalidateBackground() {
  SbWindow::invalidateBackground();
  for (auto it : m_item_list)
    it->invalidateBackground();
}

void ButtonTrain::clear() {
  for (auto it : m_item_list)
    it->clear();
}

void ButtonTrain::setOrientation(Orientation orient) {
  if (m_orientation == orient)
    return;

  SbWindow::invalidateBackground();
  for (auto it : m_item_list)
    it->setOrientation(orient);

  bool sz_chgd = (m_orientation <= ROT180 && orient > ROT180)
              || (m_orientation > ROT180 && orient <= ROT180);

  m_orientation = orient;
  if (sz_chgd)
    resize(height(), width() ); // calls repositionItems()
  else
    repositionItems();
}

} // end namespace tk

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
