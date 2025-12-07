#include "layoutserializer.h"

#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <QTextStream>

#include "layoutscene.h"
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
    }
  }

  QTextStream stream(&file);
  stream << doc.toString();
  file.close();
  return true;
}

bool LayoutSerializer::load(LayoutScene* scene, const QString& filePath) {
  QFile file(filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    return false;
  }

  QString content = file.readAll();
  file.close();

  return loadFromXml(scene, content);
}

bool LayoutSerializer::loadFromXml(LayoutScene* scene, const QString& xmlContent) {
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
    }
    node = node.nextSibling();
  }
  return true;
}
