#ifndef GSTUTILS_H
#define GSTUTILS_H

#include <QString>

// Picks the best available H.264 encoder element (with its config string) so
// recording works on any GPU: NVIDIA (nvh264enc), VA-API (vah264enc), or
// software (x264enc), instead of assuming a specific encoder is installed.
QString selectH264Encoder();

#endif  // GSTUTILS_H
