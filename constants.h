#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QColor>
#include <QString>

#include <QGraphicsItem>

namespace Constants {
// Timings (ms)
constexpr int StatusMessageDuration = 3000;
constexpr int StartupTimerDelay = 100;
constexpr int SelectionDebounceDelay = 50;

// Layout Defaults
constexpr int DefaultGridSize = 50;
constexpr double SnapDistance = 15.0;

// Dimensions
constexpr int RulerThickness = 25;
constexpr int RibbonHeight = 100;
constexpr int PropertiesDialogWidth = 200;
constexpr int PropertiesDialogHeight = 220;
constexpr qreal GuideHandleSize = 20.0;

namespace Item {
enum Types {
  AppItem = QGraphicsItem::UserType + 1,
  ZoneItem = QGraphicsItem::UserType + 2,
  GroupItem = QGraphicsItem::UserType + 3,
  GuideItem = QGraphicsItem::UserType + 4,
  Artboard = QGraphicsItem::UserType + 5,
  MirroredAppItem = QGraphicsItem::UserType + 6
};
}

// Colors (POD types)
namespace Color {
// Main Window / View
constexpr QRgb ViewBackgroundEmpty = qRgb(32, 32, 32);  // #202020
constexpr QRgb CornerWidget = qRgb(43, 43, 43);         // #2b2b2b

// App Items
constexpr QRgb AppItemFill = qRgba(60, 60, 60, 100);
constexpr QRgb AppItemBorder = qRgb(0, 0, 0);
constexpr QRgb AppItemText = qRgb(211, 211, 211);  // LightGray
constexpr QRgb AppItemStatus = qRgb(200, 200, 200);

// Zone Items
constexpr QRgb ZoneItemFill = qRgba(60, 20, 80, 100);
constexpr QRgb ZoneItemBorder = qRgb(171, 71, 188);
constexpr QRgb ZoneItemText = qRgb(255, 255, 255);

// Selection / Snapping
constexpr QRgb SelectionHighlight = qRgb(0, 120, 215);
constexpr QRgb SelectionLocked = qRgb(255, 100, 100);
constexpr QRgb GroupBorderPassive = qRgba(120, 120, 120, 180);
constexpr QRgb SmartSnapGuide = qRgb(255, 0, 255);  // Magenta
constexpr QRgb PermanentGuide = qRgb(0, 255, 255);  // Cyan

// Workspace
constexpr QRgb WorkspaceFill = qRgb(80, 80, 80);
constexpr QRgb GridLines = qRgb(110, 110, 110);
constexpr QRgb SystemBars = qRgb(0, 0, 0);
constexpr QRgb SystemBarText = qRgb(255, 255, 255);

// Rulers
constexpr QRgb RulerBackground = qRgb(43, 43, 43);
constexpr QRgb RulerText = qRgb(160, 160, 160);
constexpr QRgb RulerMarker = qRgb(255, 0, 0);

// Properties Dialog Colors
constexpr QRgb PropBackground = qRgb(53, 53, 53);    // #353535
constexpr QRgb PropBorder = qRgb(80, 80, 80);        // #505050
constexpr QRgb PropText = qRgb(224, 224, 224);       // #e0e0e0
constexpr QRgb PropInputBase = qRgb(43, 43, 43);     // #2b2b2b
constexpr QRgb PropInputText = qRgb(255, 255, 255);  // #ffffff
constexpr QRgb PropHeaderBg = qRgb(37, 37, 37);      // #252525

// Ribbon Colors
constexpr QRgb RibbonBg = qRgb(255, 255, 255);             // #ffffff
constexpr QRgb RibbonSeparator = qRgb(224, 224, 224);      // #e0e0e0
constexpr QRgb RibbonBorder = qRgb(192, 192, 192);         // #c0c0c0
constexpr QRgb RibbonText = qRgb(51, 51, 51);              // #333333
constexpr QRgb RibbonHover = qRgb(224, 224, 224);          // #e0e0e0
constexpr QRgb RibbonPressed = qRgb(192, 192, 192);        // #c0c0c0
constexpr QRgb RibbonHighlight = qRgb(51, 153, 255);       // #3399ff
constexpr QRgb RibbonHighlightText = qRgb(255, 255, 255);  // #ffffff

// SpinBox Light Theme
constexpr QRgb SpinBoxLightBg = qRgb(255, 255, 255);            // #ffffff
constexpr QRgb SpinBoxLightText = qRgb(51, 51, 51);             // #333333
constexpr QRgb SpinBoxLightBorder = qRgb(204, 204, 204);        // #cccccc
constexpr QRgb SpinBoxLightSelection = qRgb(51, 153, 255);      // #3399ff
constexpr QRgb SpinBoxLightButton = qRgb(245, 245, 245);        // #f5f5f5
constexpr QRgb SpinBoxLightHover = qRgb(224, 224, 224);         // #e0e0e0
constexpr QRgb SpinBoxLightPressed = qRgb(208, 208, 208);       // #d0d0d0
constexpr QRgb SpinBoxLightDisabledText = qRgb(170, 170, 170);  // #aaaaaa
constexpr QRgb SpinBoxLightDisabledBg = qRgb(240, 240, 240);    // #f0f0f0

// SpinBox Dark Theme Extras
constexpr QRgb SpinBoxDarkSelection = qRgb(0, 120, 215);       // #0078d7
constexpr QRgb SpinBoxDarkHover = qRgb(69, 69, 69);            // #454545
constexpr QRgb SpinBoxDarkDisabledText = qRgb(128, 128, 128);  // #808080

// Icons
constexpr const char* IconSpinUpDark = ":/resources/spin-up-dark.svg";
constexpr const char* IconSpinDownDark = ":/resources/spin-down-dark.svg";
constexpr const char* IconSpinUpLight = ":/resources/spin-up.svg";
constexpr const char* IconSpinDownLight = ":/resources/spin-down.svg";
}  // namespace Color

// Stylesheets
namespace Style {
constexpr const char* RibbonButton =
    "QToolButton { border: none; border-radius: 3px; padding: 4px; color: %1; }"
    "QToolButton:hover { background-color: %2; }"
    "QToolButton:pressed { background-color: %3; }";

constexpr const char* RibbonPopup = "QWidget { border: 1px solid %1; background-color: %2; }";

constexpr const char* Menu =
    "QMenu { background-color: %1; border: 1px solid %2; color: %3; padding: 2px; }"
    "QMenu::item { padding: 5px 25px 5px 25px; background: transparent; color: %3; }"
    "QMenu::item:selected { background-color: %4; color: %5; }";

constexpr const char* SpinBox =
    "QSpinBox { background-color: %1; color: %2; border: 1px solid %3; padding: 2px; selection-background-color: %4; }"
    "QSpinBox::up-button { subcontrol-origin: border; subcontrol-position: top right; width: 16px; border-left: 1px solid %3; border-bottom: 1px "
    "solid %3; background: %5; }"
    "QSpinBox::down-button { subcontrol-origin: border; subcontrol-position: bottom right; width: 16px; border-left: 1px solid %3; border-top: 0px "
    "solid %3; background: %5; }"
    "QSpinBox::up-button:hover, QSpinBox::down-button:hover { background: %6; }"
    "QSpinBox::up-button:pressed, QSpinBox::down-button:pressed { background: %7; }"
    "QSpinBox::up-arrow { width: 10px; height: 10px; image: url(%8); }"
    "QSpinBox::down-arrow { width: 10px; height: 10px; image: url(%9); }"
    "QSpinBox:disabled { color: %10; background-color: %11; }";

constexpr const char* PropLabel = "QLabel { color: %1; }";
}  // namespace Style
}  // namespace Constants

#endif  // CONSTANTS_H
