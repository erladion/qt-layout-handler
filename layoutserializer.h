#ifndef LAYOUTSERIALIZER_H
#define LAYOUTSERIALIZER_H

#include <QString>

class LayoutScene;

class LayoutSerializer {
public:
  // Save the current state of the scene to an XML file
  static bool save(LayoutScene* scene, const QString& filePath);

  // Load a layout from an XML file (clears existing items first)
  static bool load(LayoutScene* scene, const QString& filePath);

  // Load a layout from an XML string (useful for Templates/Presets)
  static bool loadFromXml(LayoutScene* scene, const QString& xmlContent);
};

#endif  // LAYOUTSERIALIZER_H
