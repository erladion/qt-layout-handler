#include "gstutils.h"

#include <gst/gst.h>

// gst-launch-1.0 -e videotestsrc num-buffers=150 ! videoconvert ! \
   cudaupload ! cudaconvert ! nvh264enc ! h264parse ! matroskamux ! \
   filesink location=/tmp/nv_full.mkv

// gst-launch-1.0 -e videotestsrc num-buffers=150 ! videoconvert ! cudaupload ! nvh264enc ! h264parse ! matroskamux ! filesink location=/tmp/nv_up.mkv

// gst-launch-1.0 -e videotestsrc num-buffers=150 ! videoconvert ! nvh264enc ! h264parse ! matroskamux ! filesink location=/tmp/nv_bare.mkv

// gst-inspect-1.0 nvh264enc | sed -n '/SINK template/,/PRESENT/p'

// True only if the element can actually be instantiated on this machine, not
// merely that its plugin is registered (factory_find would lie about NVENC on a
// box where the plugin is present but no usable encode device exists).
static bool encoderUsable(const char* factory) {
  if (GstElement* element = gst_element_factory_make(factory, nullptr)) {
    gst_object_unref(element);
    return true;
  }
  return false;
}

QString selectH264Encoder() {
  // Returns the encode chain that follows an upstream "videoconvert !". The
  // chain feeds system-memory (CPU) frames, so hardware encoders need explicit
  // upload/convert elements in front of them.

  // NVIDIA NVENC encodes from CUDA memory: a bare "videoconvert ! nvh264enc"
  // can't negotiate (empty file / "link has no sink"), so upload and convert on
  // the GPU first. Note nvh264enc has no "zerolatency" property.
  //
  // bitrate is in kbps and is the one quality knob present on every nvh264enc
  // version; the default (~2 Mbps) is far too low for 1080p screen content and
  // looks blocky, so raise it. (gst-inspect-1.0 nvh264enc shows version-specific
  // options like rc-mode/qp-const for finer constant-quality control.)
  if (encoderUsable("nvh264enc")) {
    const QString enc = QStringLiteral("nvh264enc bitrate=15000");
    if (encoderUsable("cudaupload") && encoderUsable("cudaconvert")) {
      return QStringLiteral("cudaupload ! cudaconvert ! ") + enc;
    }
    if (encoderUsable("cudaupload")) {
      return QStringLiteral("cudaupload ! ") + enc;
    }
    return enc;
  }

  // VA-API (Intel / AMD); vapostproc handles the upload/format when present.
  if (encoderUsable("vah264enc")) {
    if (encoderUsable("vapostproc")) {
      return QStringLiteral("vapostproc ! vah264enc");
    }
    return QStringLiteral("vah264enc");
  }

  // Software fallbacks: reliable, encode system memory directly, no GPU session.
  // Tuned for recording quality rather than live latency: veryfast (a big jump
  // over ultrafast yet still real-time at 1080p) with constant-quantizer rate
  // control (quantizer ~20) so the bitrate follows the content instead of being
  // capped at x264enc's ~2 Mbps CBR default. Lower the quantizer for higher
  // quality / larger files.
  if (encoderUsable("x264enc")) {
    return QStringLiteral("x264enc speed-preset=veryfast pass=quant quantizer=20");
  }
  return QStringLiteral("openh264enc");
}
