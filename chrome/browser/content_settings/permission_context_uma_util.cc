// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/metrics/histogram.h"
#include "chrome/browser/content_settings/permission_context_uma_util.h"
#include "url/gurl.h"


// UMA keys need to be statically initialized so plain function would not
// work. Use a Macro instead.
#define PERMISSION_ACTION_UMA(secure_origin,                                   \
            permission, permission_secure, permission_insecure, action)        \
    UMA_HISTOGRAM_ENUMERATION(                                                 \
        permission,                                                            \
        action,                                                                \
        PERMISSION_ACTION_NUM);                                                \
    if (secure_origin) {                                                       \
        UMA_HISTOGRAM_ENUMERATION(permission_secure,                           \
                                  action,                                      \
                                  PERMISSION_ACTION_NUM);                      \
    } else {                                                                   \
        UMA_HISTOGRAM_ENUMERATION(permission_insecure,                         \
                                  action,                                      \
                                  PERMISSION_ACTION_NUM);                      \
    }


namespace {

// Enum for UMA purposes, make sure you update histograms.xml if you add new
// permission actions. Never delete or reorder an entry; only add new entries
// immediately before PERMISSION_NUM
enum PermissionAction {
  GRANTED = 0,
  DENIED = 1,
  DISMISSED = 2,
  IGNORED = 3,

  // Always keep this at the end.
  PERMISSION_ACTION_NUM,
};

// Enum for UMA purposes, make sure you update histograms.xml if you add new
// permission actions. Never delete or reorder an entry; only add new entries
// immediately before PERMISSION_NUM
enum PermissionType {
  PERMISSION_UNKNOWN = 0,
  PERMISSION_MIDI_SYSEX = 1,
  PERMISSION_PUSH_MESSAGING = 2,
  PERMISSION_NOTIFICATIONS = 3,
  PERMISSION_GEOLOCATION = 4,

  // Always keep this at the end.
  PERMISSION_NUM,
};

void RecordPermissionAction(
      ContentSettingsType permission,
      PermissionAction action,
      bool secure_origin) {
  switch (permission) {
      case CONTENT_SETTINGS_TYPE_GEOLOCATION:
        PERMISSION_ACTION_UMA(
            secure_origin,
            "ContentSettings.PermissionActions_Geolocation",
            "ContentSettings.PermissionActionsSecureOrigin_Geolocation",
            "ContentSettings.PermissionActionsInsecureOrigin_Geolocation",
            action);
        break;
      case CONTENT_SETTINGS_TYPE_NOTIFICATIONS:
        PERMISSION_ACTION_UMA(
            secure_origin,
            "ContentSettings.PermissionActions_Notifications",
            "ContentSettings.PermissionActionsSecureOrigin_Notifications",
            "ContentSettings.PermissionActionsInsecureOrigin_Notifications",
            action);
        break;
      case CONTENT_SETTINGS_TYPE_MIDI_SYSEX:
        PERMISSION_ACTION_UMA(
            secure_origin,
            "ContentSettings.PermissionActions_MidiSysEx",
            "ContentSettings.PermissionActionsSecureOrigin_MidiSysEx",
            "ContentSettings.PermissionActionsInsecureOrigin_MidiSysEx",
            action);
        break;
      case CONTENT_SETTINGS_TYPE_PUSH_MESSAGING:
        PERMISSION_ACTION_UMA(
            secure_origin,
            "ContentSettings.PermissionActions_PushMessaging",
            "ContentSettings.PermissionActionsSecureOrigin_PushMessaging",
            "ContentSettings.PermissionActionsInsecureOrigin_PushMessaging",
            action);
        break;
#if defined(OS_ANDROID)
      case CONTENT_SETTINGS_TYPE_PROTECTED_MEDIA_IDENTIFIER:
        // TODO(miguelg): support protected media through
        // the generic permission class.
        break;
#endif
      default:
        NOTREACHED() << "PERMISSION " << permission << " not accounted for";
    }
}

void RecordPermissionRequest(
    ContentSettingsType permission, bool secure_origin) {
  PermissionType type;
  switch (permission) {
    case CONTENT_SETTINGS_TYPE_GEOLOCATION:
      type = PERMISSION_GEOLOCATION;
      break;
    case CONTENT_SETTINGS_TYPE_NOTIFICATIONS:
      type = PERMISSION_NOTIFICATIONS;
      break;
    case CONTENT_SETTINGS_TYPE_MIDI_SYSEX:
      type = PERMISSION_MIDI_SYSEX;
      break;
    case CONTENT_SETTINGS_TYPE_PUSH_MESSAGING:
      type = PERMISSION_PUSH_MESSAGING;
      break;
    default:
      NOTREACHED() << "PERMISSION " << permission << " not accounted for";
      type = PERMISSION_UNKNOWN;
  }
  UMA_HISTOGRAM_ENUMERATION(
      "ContentSettings.PermissionRequested",
      type,
      PERMISSION_NUM);
  if (secure_origin) {
    UMA_HISTOGRAM_ENUMERATION(
        "ContentSettings.PermissionRequested_SecureOrigin",
        type,
        PERMISSION_NUM);
  } else {
    UMA_HISTOGRAM_ENUMERATION(
        "ContentSettings.PermissionRequested_InsecureOrigin",
        type,
        PERMISSION_NUM);
  }
}

} // namespace

// Make sure you update histograms.xml permission histogram_suffix if you
// add new permission
void PermissionContextUmaUtil::PermissionRequested(
    ContentSettingsType permission, const GURL& requesting_origin) {
  RecordPermissionRequest(permission, requesting_origin.SchemeIsSecure());
}

void PermissionContextUmaUtil::PermissionGranted(
    ContentSettingsType permission, const GURL& requesting_origin) {
  RecordPermissionAction(permission, GRANTED,
                         requesting_origin.SchemeIsSecure());
}

void PermissionContextUmaUtil::PermissionDenied(
    ContentSettingsType permission, const GURL& requesting_origin) {
  RecordPermissionAction(permission, DENIED,
                         requesting_origin.SchemeIsSecure());
}

void PermissionContextUmaUtil::PermissionDismissed(
    ContentSettingsType permission, const GURL& requesting_origin) {
  RecordPermissionAction(permission, DISMISSED,
                         requesting_origin.SchemeIsSecure());
}

void PermissionContextUmaUtil::PermissionIgnored(
    ContentSettingsType permission, const GURL& requesting_origin) {
  RecordPermissionAction(permission, IGNORED,
                         requesting_origin.SchemeIsSecure());
}
