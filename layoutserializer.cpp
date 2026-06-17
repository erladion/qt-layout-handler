#include "layoutserializer.h"

#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>

#include "capturecontroller.h"
#include "layoutscene.h"
#include "mirroredappitem.h"
#include "resizableappitem.h"

bool LayoutSerializer::save(LayoutScene* scene, const QString& filePath) {
  if (!scene) {
    return false;
  }

  QFile file(filePath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return false;
  }

  QDomDocument doc;
  QDomElement root = doc.createElement("Layout");
  doc.appendChild(root);

  for (auto item : scene->items()) {
    if (item->type() == Constants::Item::AppItem) {
      auto appItem = static_cast<ResizableAppItem*>(item);
      QDomElement appEl = doc.createElement("App");
      appEl.setAttribute("name", appItem->name());
      appEl.setAttribute("x", appItem->scenePos().x());
      appEl.setAttribute("y", appItem->scenePos().y());
      appEl.setAttribute("width", appItem->rect().width());
      appEl.setAttribute("height", appItem->rect().height());
      appEl.setAttribute("z", appItem->zValue());
      root.appendChild(appEl);
    } else if (item->type() == Constants::Item::MirroredAppItem) {
      auto mirror = static_cast<MirroredAppItem*>(item);
      const CaptureSettings s = mirror->captureSettings();
      QDomElement el = doc.createElement("Mirror");
      // Stable identity for re-matching to a live window on load.
      el.setAttribute("class", mirror->appClass());
      el.setAttribute("title", mirror->appTitle());
      el.setAttribute("x", mirror->scenePos().x());
      el.setAttribute("y", mirror->scenePos().y());
      el.setAttribute("width", mirror->rect().width());
      el.setAttribute("height", mirror->rect().height());
      el.setAttribute("z", mirror->zValue());
      el.setAttribute("cropTop", s.cropTop);
      el.setAttribute("cropBottom", s.cropBottom);
      el.setAttribute("cropLeft", s.cropLeft);
      el.setAttribute("cropRight", s.cropRight);
      el.setAttribute("fps", s.framerate);
      el.setAttribute("useDamage", s.useDamage ? 1 : 0);
      root.appendChild(el);
    }
  }

  QTextStream stream(&file);
  stream << doc.toString();
  file.close();
  return true;
}

bool LayoutSerializer::load(LayoutScene* scene, const QString& filePath, CaptureController* capture) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QString content = file.readAll();
  file.close();

  return loadFromXml(scene, content, capture);
}

bool LayoutSerializer::loadFromXml(LayoutScene* scene, const QString& xmlContent, CaptureController* capture) {
  if (!scene) {
    return false;
  }

  QDomDocument doc;
  if (!doc.setContent(xmlContent)) {
    return false;
  }

  scene->clearLayout();

  QDomElement root = doc.documentElement();
  QDomNode node = root.firstChild();
  while (!node.isNull()) {
    QDomElement el = node.toElement();
    if (!el.isNull() && el.tagName() == "App") {
      ResizableAppItem* item =
          scene->addAppItem(el.attribute("name"), QRectF(0, 0, el.attribute("width").toDouble(), el.attribute("height").toDouble()));

      item->setPos(el.attribute("x").toDouble(), el.attribute("y").toDouble());
      if (el.hasAttribute("z")) {
        item->setZValue(el.attribute("z").toDouble());
      }
    } else if (!el.isNull() && el.tagName() == "Mirror" && capture) {
      CaptureSettings s;
      s.cropTop = el.attribute("cropTop").toInt();
      s.cropBottom = el.attribute("cropBottom").toInt();
      s.cropLeft = el.attribute("cropLeft").toInt();
      s.cropRight = el.attribute("cropRight").toInt();
      s.framerate = el.attribute("fps", "30").toInt();
      s.useDamage = el.attribute("useDamage").toInt() != 0;

      // The controller re-matches the saved identity to an open window (or
      // returns a disconnected placeholder).
      MirroredAppItem* item = capture->createSavedMirror(el.attribute("class"), el.attribute("title"), s);
      scene->addItem(item);
      item->setRect(0, 0, el.attribute("width").toDouble(), el.attribute("height").toDouble());
      item->setPos(el.attribute("x").toDouble(), el.attribute("y").toDouble());
      if (el.hasAttribute("z")) {
        item->setZValue(el.attribute("z").toDouble());
      }
    }
    node = node.nextSibling();
  }
  return true;
}
