#ifndef GSTUTILS_H
#define GSTUTILS_H

#include <QString>

// Picks the best available H.264 encoder element (with its config string) so
// recording works on any GPU: NVIDIA (nvh264enc), VA-API (vah264enc), or
// software (x264enc), instead of assuming a specific encoder is installed.
QString selectH264Encoder();

// Picks the best available audio encoder chain for muxing into Matroska (Opus,
// then AAC, then MP3). Returns an empty string if none can be instantiated.
QString selectAudioEncoder();

// Picks the audio capture source, preferring system/app audio (the default
// sink's monitor on PulseAudio/PipeWire) and falling back to the default input
// (mic). Returns an empty string if no source is available.
QString selectAudioSource();

// True only if the element can actually be instantiated on this machine (not
// merely that its plugin is registered).
bool gstElementAvailable(const QString& factory);

#endif  // GSTUTILS_H
