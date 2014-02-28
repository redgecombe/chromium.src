// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/input/synthetic_gesture_target_aura.h"

#include "content/browser/renderer_host/render_widget_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/browser/renderer_host/ui_events_helper.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/events/gestures/gesture_configuration.h"

using blink::WebTouchEvent;
using blink::WebMouseWheelEvent;

namespace content {

SyntheticGestureTargetAura::SyntheticGestureTargetAura(
    RenderWidgetHostImpl* host)
    : SyntheticGestureTargetBase(host) {
}

void SyntheticGestureTargetAura::DispatchWebTouchEventToPlatform(
    const WebTouchEvent& web_touch,
    const ui::LatencyInfo& latency_info) {
  TouchEventWithLatencyInfo touch_with_latency(web_touch, latency_info);
  ScopedVector<ui::TouchEvent> events;
  bool conversion_success = MakeUITouchEventsFromWebTouchEvents(
      touch_with_latency, &events, LOCAL_COORDINATES);
  DCHECK(conversion_success);

  aura::Window* window = GetWindow();
  aura::WindowEventDispatcher* dispatcher = window->GetDispatcher();
  for (ScopedVector<ui::TouchEvent>::iterator iter = events.begin(),
      end = events.end(); iter != end; ++iter) {
    (*iter)->ConvertLocationToTarget(window, dispatcher->window());
    ui::EventDispatchDetails details = dispatcher->OnEventFromSource(*iter);
    if (details.dispatcher_destroyed)
      break;
  }
}

void SyntheticGestureTargetAura::DispatchWebMouseWheelEventToPlatform(
      const blink::WebMouseWheelEvent& web_wheel,
      const ui::LatencyInfo&) {
  gfx::Point location(web_wheel.x, web_wheel.y);
  ui::MouseEvent mouse_event(
      ui::ET_MOUSEWHEEL, location, location, ui::EF_NONE, ui::EF_NONE);
  ui::MouseWheelEvent wheel_event(
      mouse_event, web_wheel.deltaX, web_wheel.deltaY);

  aura::Window* window = GetWindow();
  wheel_event.ConvertLocationToTarget(window, window->GetRootWindow());
  ui::EventDispatchDetails details =
      window->GetDispatcher()->OnEventFromSource(&wheel_event);
  if (details.dispatcher_destroyed)
    return;
}

namespace {

ui::EventType
WebMouseEventTypeToEventType(blink::WebInputEvent::Type web_type) {
  switch (web_type) {
    case blink::WebInputEvent::MouseDown:
      return ui::ET_MOUSE_PRESSED;

    case blink::WebInputEvent::MouseUp:
      return ui::ET_MOUSE_RELEASED;

    case blink::WebInputEvent::MouseMove:
      return ui::ET_MOUSE_MOVED;

    case blink::WebInputEvent::MouseEnter:
      return ui::ET_MOUSE_ENTERED;

    case blink::WebInputEvent::MouseLeave:
      return ui::ET_MOUSE_EXITED;

    case blink::WebInputEvent::ContextMenu:
      NOTREACHED() << "WebInputEvent::ContextMenu not supported by"
          "SyntheticGestureTargetAura";

    default:
      NOTREACHED();
  }

  return ui::ET_UNKNOWN;
}

int WebMouseEventButtonToFlags(blink::WebMouseEvent::Button button) {
  switch (button) {
    case blink::WebMouseEvent::ButtonLeft:
      return ui::EF_LEFT_MOUSE_BUTTON;

    case blink::WebMouseEvent::ButtonMiddle:
      return ui::EF_MIDDLE_MOUSE_BUTTON;

    case blink::WebMouseEvent::ButtonRight:
      return ui::EF_RIGHT_MOUSE_BUTTON;

    default:
      NOTREACHED();
  }

  return 0;
}

}  // namespace

void SyntheticGestureTargetAura::DispatchWebMouseEventToPlatform(
      const blink::WebMouseEvent& web_mouse,
      const ui::LatencyInfo& latency_info) {
  gfx::Point location(web_mouse.x, web_mouse.y);
  ui::EventType event_type = WebMouseEventTypeToEventType(web_mouse.type);
  int flags = WebMouseEventButtonToFlags(web_mouse.button);
  ui::MouseEvent mouse_event(event_type, location, location, flags, flags);

  aura::Window* window = GetWindow();
  mouse_event.ConvertLocationToTarget(window, window->GetRootWindow());
  ui::EventDispatchDetails details =
      window->GetDispatcher()->OnEventFromSource(&mouse_event);
  if (details.dispatcher_destroyed)
    return;
}

SyntheticGestureParams::GestureSourceType
SyntheticGestureTargetAura::GetDefaultSyntheticGestureSourceType() const {
  return SyntheticGestureParams::TOUCH_INPUT;
}

bool SyntheticGestureTargetAura::SupportsSyntheticGestureSourceType(
    SyntheticGestureParams::GestureSourceType gesture_source_type) const {
  return gesture_source_type == SyntheticGestureParams::TOUCH_INPUT ||
      gesture_source_type == SyntheticGestureParams::MOUSE_INPUT;
}

int SyntheticGestureTargetAura::GetTouchSlopInDips() const {
  // - 1 because Aura considers a pointer to be moving if it has moved at least
  // 'max_touch_move_in_pixels_for_click' pixels.
  return ui::GestureConfiguration::max_touch_move_in_pixels_for_click() - 1;
}

aura::Window* SyntheticGestureTargetAura::GetWindow() const {
  aura::Window* window = render_widget_host()->GetView()->GetNativeView();
  DCHECK(window);
  return window;
}

}  // namespace content
