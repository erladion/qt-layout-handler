#include "gstutils.h"

#include <gst/gst.h>

QString selectH264Encoder() {
  struct EncoderChoice {
    const char* factory;
    const char* pipeline;
  };
  static const EncoderChoice choices[] = {
      {"nvh264enc", "nvh264enc zerolatency=true"},
      {"vah264enc", "vah264enc"},
      {"x264enc", "x264enc tune=zerolatency speed-preset=ultrafast"},
  };
  for (const auto& choice : choices) {
    if (GstElementFactory* factory = gst_element_factory_find(choice.factory)) {
      gst_object_unref(factory);
      return QString::fromLatin1(choice.pipeline);
    }
  }
  return QStringLiteral("x264enc tune=zerolatency speed-preset=ultrafast");
}
