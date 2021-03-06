// This file is part of Desktop App Toolkit,
// a set of libraries for developing nice desktop applications.
//
// For license and copyright information please follow this link:
// https://github.com/desktop-app/legal/blob/master/LEGAL
//
#pragma once

#include "webrtc/webrtc_media_devices.h"

namespace Webrtc {

[[nodiscard]] std::vector<VideoInput> MacGetVideoInputList();

} // namespace Webrtc
