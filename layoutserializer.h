#ifndef LAYOUTSERIALIZER_H
#define LAYOUTSERIALIZER_H

#include <QString>

class LayoutScene;
class CaptureController;

class LayoutSerializer {
public:
  // Save the current state of the scene to an XML file
  static bool save(LayoutScene* scene, const QString& filePath);

  // Load a layout from an XML file (clears existing items first). A
  // CaptureController is needed to restore mirrored captures (it re-matches them
  // to open windows); pass nullptr to skip captures (e.g. for templates).
  static bool load(LayoutScene* scene, const QString& filePath, CaptureController* capture = nullptr);

  // Load a layout from an XML string (useful for Templates/Presets)
  static bool loadFromXml(LayoutScene* scene, const QString& xmlContent, CaptureController* capture = nullptr);
};

#endif  // LAYOUTSERIALIZER_H
