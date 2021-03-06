// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "athena/wm/bezel_controller.h"

#include "ui/aura/window.h"
#include "ui/events/event_handler.h"
#include "ui/gfx/display.h"
#include "ui/gfx/geometry/point_conversions.h"
#include "ui/gfx/screen.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace athena {
namespace {

// Using bezel swipes, the first touch that is registered is usually within
// 5-10 pixels from the edge, but sometimes as far as 29 pixels away.
// So setting this width fairly high for now.
const float kBezelWidth = 20.0f;

const float kScrollDeltaNone = 0;

bool ShouldProcessGesture(ui::EventType event_type) {
  return event_type == ui::ET_GESTURE_SCROLL_UPDATE ||
         event_type == ui::ET_GESTURE_SCROLL_BEGIN ||
         event_type == ui::ET_GESTURE_BEGIN ||
         event_type == ui::ET_GESTURE_END;
}

gfx::Display GetDisplay(aura::Window* window) {
  gfx::Screen* screen = gfx::Screen::GetScreenFor(window);
  return screen->GetDisplayNearestWindow(window);
}

float GetDistance(const gfx::PointF& location,
                  aura::Window* window,
                  BezelController::Bezel bezel) {
  DCHECK(bezel == BezelController::BEZEL_LEFT ||
         bezel == BezelController::BEZEL_RIGHT);
  // Convert location from window coordinates to screen coordinates.
  gfx::Point point_in_screen(gfx::ToRoundedPoint(location));
  wm::ConvertPointToScreen(window, &point_in_screen);
  return bezel == BezelController::BEZEL_LEFT
             ? point_in_screen.x()
             : point_in_screen.x() - GetDisplay(window).bounds().width();
}

}  // namespace

BezelController::BezelController(aura::Window* container)
    : container_(container),
      state_(NONE),
      scroll_bezel_(BEZEL_NONE),
      scroll_target_(NULL),
      left_right_delegate_(NULL) {
}

void BezelController::SetState(BezelController::State state) {
  // Use SetState(State, float) if |state| is one of the BEZEL_SCROLLING states.
  DCHECK_NE(state, BEZEL_SCROLLING_TWO_FINGERS);
  DCHECK_NE(state, BEZEL_SCROLLING_ONE_FINGER);
  SetState(state, kScrollDeltaNone);
}

void BezelController::SetState(BezelController::State state,
                               float scroll_delta) {
  if (!left_right_delegate_ || state == state_)
    return;

  if (state == BEZEL_SCROLLING_TWO_FINGERS)
    left_right_delegate_->ScrollBegin(scroll_bezel_, scroll_delta);
  else if (state_ == BEZEL_SCROLLING_TWO_FINGERS)
    left_right_delegate_->ScrollEnd();
  state_ = state;
  if (state == NONE) {
    scroll_bezel_ = BEZEL_NONE;
    scroll_target_ = NULL;
  }
}

// Only implemented for LEFT and RIGHT bezels ATM.
BezelController::Bezel BezelController::GetBezel(const gfx::PointF& location) {
  int screen_width = GetDisplay(container_).bounds().width();
  if (location.x() < kBezelWidth) {
    return BEZEL_LEFT;
  } else if (location.x() > screen_width - kBezelWidth) {
    return BEZEL_RIGHT;
  } else {
    return BEZEL_NONE;
  }
}

void BezelController::OnGestureEvent(ui::GestureEvent* event) {
  // TODO(mfomitchev): Currently we aren't retargetting or consuming any of the
  // touch events. This means that content can prevent the generation of gesture
  // events and two-finger scroll won't work. Possible solution to this problem
  // is hosting our own gesture recognizer or retargetting touch events at the
  // bezel.

  if (!left_right_delegate_)
    return;

  ui::EventType type = event->type();
  if (!ShouldProcessGesture(type))
    return;

  if (scroll_target_ && event->target() != scroll_target_)
    return;

  const gfx::PointF& event_location = event->location_f();
  const ui::GestureEventDetails& event_details = event->details();
  int num_touch_points = event_details.touch_points();
  float scroll_delta = kScrollDeltaNone;
  if (scroll_bezel_ != BEZEL_NONE) {
    aura::Window* target_window = static_cast<aura::Window*>(event->target());
    scroll_delta = GetDistance(event_location, target_window, scroll_bezel_);
  }

  if (type == ui::ET_GESTURE_BEGIN) {
    if (num_touch_points > 2) {
      SetState(IGNORE_CURRENT_SCROLL);
      return;
    }
    BezelController::Bezel event_bezel = GetBezel(event->location_f());
    switch (state_) {
      case NONE:
        scroll_bezel_ = event_bezel;
        scroll_target_ = event->target();
        if (event_bezel != BEZEL_LEFT && event_bezel != BEZEL_RIGHT)
          SetState(IGNORE_CURRENT_SCROLL);
        else
          SetState(BEZEL_GESTURE_STARTED);
        break;
      case IGNORE_CURRENT_SCROLL:
        break;
      case BEZEL_GESTURE_STARTED:
      case BEZEL_SCROLLING_ONE_FINGER:
        DCHECK_EQ(num_touch_points, 2);
        DCHECK(scroll_target_);
        DCHECK_NE(scroll_bezel_, BEZEL_NONE);

        if (event_bezel != scroll_bezel_) {
          SetState(IGNORE_CURRENT_SCROLL);
          return;
        }
        if (state_ == BEZEL_SCROLLING_ONE_FINGER)
          SetState(BEZEL_SCROLLING_TWO_FINGERS);
        break;
      case BEZEL_SCROLLING_TWO_FINGERS:
        // Should've exited above
        NOTREACHED();
        break;
    }
  } else if (type == ui::ET_GESTURE_END) {
    if (state_ == NONE)
      return;

    CHECK(scroll_target_);
    if (num_touch_points == 1) {
      SetState(NONE);
    } else {
      SetState(IGNORE_CURRENT_SCROLL);
    }
  } else if (type == ui::ET_GESTURE_SCROLL_BEGIN) {
    DCHECK(state_ == IGNORE_CURRENT_SCROLL || state_ == BEZEL_GESTURE_STARTED);
    if (state_ != BEZEL_GESTURE_STARTED)
      return;

    if (num_touch_points == 1) {
      SetState(BEZEL_SCROLLING_ONE_FINGER, scroll_delta);
      return;
    }

    DCHECK_EQ(num_touch_points, 2);
    SetState(BEZEL_SCROLLING_TWO_FINGERS, scroll_delta);
    if (left_right_delegate_->CanScroll())
      event->SetHandled();
  } else if (type == ui::ET_GESTURE_SCROLL_UPDATE) {
    if (state_ != BEZEL_SCROLLING_TWO_FINGERS)
      return;

    left_right_delegate_->ScrollUpdate(scroll_delta);
    if (left_right_delegate_->CanScroll())
      event->SetHandled();
  }
}

}  // namespace athena
