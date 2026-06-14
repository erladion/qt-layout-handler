#include "presentercontroller.h"

#include <QButtonGroup>
#include <QColorDialog>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QWidget>

#include "draghandlewidget.h"
#include "layoutscene.h"

PresenterController::PresenterController(QGraphicsView* view, QObject* parent) : QObject(parent), m_pView(view) {
  createFloatingToolbar();
}

void PresenterController::setScene(LayoutScene* scene) {
  // Destroy the old drawing manager while its scene is still alive so it can
  // remove and delete its own items instead of dangling.
  if (m_pDrawingManager) {
    delete m_pDrawingManager;
    m_pDrawingManager = nullptr;
  }

  m_pScene = scene;

  if (m_pScene) {
    m_pDrawingManager = new DrawingManager(m_pScene, this);
    m_pDrawingManager->setColor(m_drawColor);
    m_pDrawingManager->setSize(m_drawSize);
    m_pDrawingManager->setShape(m_currentShape);
  }
}

bool PresenterController::handleViewportEvent(QObject* watched, QEvent* event) {
  if (watched != m_pView->viewport() || m_pScene == nullptr) {
    return false;
  }

  if (m_currentMode == Mode::Laser && event->type() == QEvent::MouseMove) {
    m_pScene->updateLaserPosition(m_pView->mapToScene(static_cast<QMouseEvent*>(event)->pos()));
    return true;
  }

  if (m_currentMode == Mode::Draw && m_pDrawingManager) {
    if (m_pDrawingManager->handleViewportEvent(event, m_pView)) {
      return true;
    }
  }

  return false;
}

void PresenterController::resetToEditMode() {
  // Checking the Edit button drives its toggle handler, which switches the mode
  // and hides the draw/laser popouts.
  if (m_pBtnEdit) {
    m_pBtnEdit->setChecked(true);
  }
}

bool PresenterController::removeDrawnItem(QGraphicsItem* item) {
  return m_pDrawingManager && m_pDrawingManager->removeItem(item);
}

bool PresenterController::eventFilter(QObject* watched, QEvent* event) {
  if (watched == m_floatingToolbar) {
    if (event->type() == QEvent::MouseButtonPress && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
      m_isDraggingToolbar = true;
      m_dragOffset = static_cast<QMouseEvent*>(event)->pos();
      return true;
    } else if (event->type() == QEvent::MouseMove && m_isDraggingToolbar) {
      QPoint globalMousePos = static_cast<QMouseEvent*>(event)->globalPosition().toPoint();
      QPoint parentPos = m_pView->viewport()->mapFromGlobal(globalMousePos);

      QPoint newPos = parentPos - m_dragOffset;

      // Clamp so the toolbar stays inside the viewport.
      const int maxX = m_pView->viewport()->width() - m_floatingToolbar->width();
      const int maxY = m_pView->viewport()->height() - m_floatingToolbar->height();
      newPos.setX(qBound(0, newPos.x(), maxX));
      newPos.setY(qBound(0, newPos.y(), maxY));

      m_floatingToolbar->move(newPos);
      updatePopoutPositions();  // Keep popouts glued to the side
      return true;
    } else if (event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton) {
      m_isDraggingToolbar = false;
      return true;
    }
  }
  return QObject::eventFilter(watched, event);
}

void PresenterController::createFloatingToolbar() {
  // Bound strictly to the viewport's parent view so it floats over the canvas.
  m_floatingToolbar = new QWidget(m_pView);
  m_floatingToolbar->setObjectName("FloatingToolbar");

  m_floatingToolbar->setStyleSheet(
      "#FloatingToolbar { background-color: rgba(30, 30, 30, 210); border-radius: 2px; border: 1px solid #444; }"
      "QPushButton { color: white; background: transparent; border: 1px solid #555; border-radius: 2px; padding: 6px 12px; }"
      "QPushButton:hover { background: rgba(255, 255, 255, 20); }"
      "QPushButton:checked { background: #E53935; border: 1px solid #ff5252; }");

  QVBoxLayout* mainLayout = new QVBoxLayout(m_floatingToolbar);
  mainLayout->setContentsMargins(8, 4, 8, 8);
  mainLayout->setSpacing(6);

  DragHandleWidget* dragHandle = new DragHandleWidget(m_floatingToolbar);
  mainLayout->addWidget(dragHandle);

  m_pBtnEdit = new QPushButton(QIcon(":/icons/move.svg"), "");
  m_pBtnDraw = new QPushButton(QIcon(":/icons/draw.svg"), "");
  m_pBtnLaser = new QPushButton(QIcon(":/icons/laser.svg"), "");
  m_pBtnClear = new QPushButton(QIcon(":/icons/clear.svg"), "");

  m_pBtnEdit->setFixedSize(24, 24);
  m_pBtnDraw->setFixedSize(24, 24);
  m_pBtnLaser->setFixedSize(24, 24);
  m_pBtnClear->setFixedSize(24, 24);

  m_pBtnEdit->setCheckable(true);
  m_pBtnDraw->setCheckable(true);
  m_pBtnLaser->setCheckable(true);
  m_pBtnEdit->setChecked(true);  // Default state

  mainLayout->addWidget(m_pBtnEdit);
  mainLayout->addWidget(m_pBtnDraw);
  mainLayout->addWidget(m_pBtnLaser);
  mainLayout->addWidget(m_pBtnClear);

  // The drawing manager is recreated per layout, so guard against it being absent.
  connect(m_pBtnClear, &QPushButton::clicked, this, [this]() {
    if (m_pDrawingManager) {
      m_pDrawingManager->clearDrawings();
    }
  });

  m_floatingToolbar->move(20, 20);
  m_floatingToolbar->show();
  m_floatingToolbar->raise();

  m_floatingToolbar->installEventFilter(this);

  m_laserSettingsWidget = new QWidget(m_pView->viewport());
  m_laserSettingsWidget->setStyleSheet("background-color: rgba(30, 30, 30, 220); border-radius: 8px; color: white;");
  m_laserSettingsWidget->hide();

  QHBoxLayout* laserLayout = new QHBoxLayout(m_laserSettingsWidget);
  laserLayout->setContentsMargins(10, 5, 10, 5);

  QLabel* laserLbl = new QLabel("Size:");
  QSlider* laserSlider = new QSlider(Qt::Horizontal);
  laserSlider->setRange(5, 50);
  laserSlider->setValue(m_laserSize);

  QPushButton* laserColorBtn = new QPushButton();
  laserColorBtn->setFixedSize(20, 20);
  laserColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #777; border-radius: 3px;").arg(m_laserColor.name()));

  laserLayout->addWidget(laserLbl);
  laserLayout->addWidget(laserSlider);
  laserLayout->addWidget(laserColorBtn);

  connect(laserSlider, &QSlider::valueChanged, this, [this](int val) {
    m_laserSize = val;
    if (m_pScene) {
      m_pScene->setLaserSize(val);
    }
  });

  connect(laserColorBtn, &QPushButton::clicked, this, [this, laserColorBtn]() {
    QColor color = QColorDialog::getColor(m_laserColor, m_floatingToolbar, "Laser Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
      m_laserColor = color;
      laserColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #777; border-radius: 3px;").arg(color.name()));
      if (m_pScene) {
        m_pScene->setLaserColor(color);
      }
    }
  });

  connect(m_pBtnLaser, &QPushButton::toggled, this, [this](bool checked) {
    if (checked) {
      m_currentMode = Mode::Laser;
      m_pBtnEdit->setChecked(false);
      m_pBtnDraw->setChecked(false);
      if (m_laserSettingsWidget) {
        m_laserSettingsWidget->show();
        updatePopoutPositions();
        m_laserSettingsWidget->raise();
      }
      if (m_pScene) {
        m_pScene->setLaserActive(true);
      }
    } else {
      if (m_laserSettingsWidget) {
        m_laserSettingsWidget->hide();
      }
      if (m_pScene) {
        m_pScene->setLaserActive(false);
      }
      if (!m_pBtnDraw->isChecked()) {
        m_pBtnEdit->setChecked(true);
      }
    }
  });

  // --- Draw settings popout ---
  m_drawSettingsWidget = new QWidget(m_pView->viewport());
  m_drawSettingsWidget->setStyleSheet("background-color: rgba(30, 30, 30, 220); border-radius: 8px; color: white;");
  m_drawSettingsWidget->hide();

  QVBoxLayout* drawLayout = new QVBoxLayout(m_drawSettingsWidget);
  drawLayout->setContentsMargins(10, 5, 10, 5);
  drawLayout->setSpacing(8);

  // Shape tools — a vertical list of one-click buttons (replaces the dropdown).
  struct ShapeTool {
    const char* label;
    const char* icon;
    DrawingManager::Shape shape;
  };
  const ShapeTool shapeTools[] = {
      {"Freehand", ":/icons/shape-freehand.svg", DrawingManager::Shape::Freehand},
      {"Marker", ":/icons/shape-marker.svg", DrawingManager::Shape::Marker},
      {"Arrow", ":/icons/shape-arrow.svg", DrawingManager::Shape::Arrow},
      {"Rectangle", ":/icons/shape-rectangle.svg", DrawingManager::Shape::Rectangle},
      {"Circle", ":/icons/shape-circle.svg", DrawingManager::Shape::Ellipse},
  };

  const QString shapeBtnStyle =
      "QPushButton { background: transparent; border: 1px solid #555; border-radius: 3px; padding: 3px; }"
      "QPushButton:hover { background: rgba(255, 255, 255, 20); }"
      "QPushButton:checked { background: #E53935; border: 1px solid #ff5252; }";

  QButtonGroup* shapeGroup = new QButtonGroup(m_drawSettingsWidget);  // exclusive by default
  QVBoxLayout* shapeLayout = new QVBoxLayout();
  shapeLayout->setSpacing(2);

  for (const ShapeTool& tool : shapeTools) {
    QPushButton* shapeBtn = new QPushButton(QIcon(tool.icon), "");
    shapeBtn->setToolTip(tool.label);
    shapeBtn->setIconSize(QSize(18, 18));
    shapeBtn->setFixedSize(30, 30);
    shapeBtn->setCheckable(true);
    shapeBtn->setStyleSheet(shapeBtnStyle);
    if (tool.shape == m_currentShape) {
      shapeBtn->setChecked(true);
    }
    shapeGroup->addButton(shapeBtn);
    shapeLayout->addWidget(shapeBtn);

    const DrawingManager::Shape shape = tool.shape;
    connect(shapeBtn, &QPushButton::clicked, this, [this, shape]() {
      m_currentShape = shape;
      if (m_pDrawingManager) {
        m_pDrawingManager->setShape(shape);
      }
    });
  }

  QSlider* drawSlider = new QSlider(Qt::Horizontal);
  drawSlider->setRange(1, 30);
  drawSlider->setValue(m_drawSize);
  drawSlider->setMinimumWidth(90);

  QPushButton* drawColorBtn = new QPushButton();
  drawColorBtn->setFixedSize(20, 20);
  drawColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #777; border-radius: 3px;").arg(m_drawColor.name()));

  QPushButton* undoBtn = new QPushButton(QIcon(":/icons/undo.svg"), "");
  QPushButton* redoBtn = new QPushButton(QIcon(":/icons/redo.svg"), "");

  undoBtn->setFixedSize(24, 24);
  redoBtn->setFixedSize(24, 24);

  QString btnStyle =
      "QPushButton { background-color: #444; border: 1px solid #666; border-radius: 3px; padding: 2px 6px; font-size: 11px; }"
      "QPushButton:hover { background-color: #555; }"
      "QPushButton:pressed { background-color: #333; }";

  undoBtn->setStyleSheet(btnStyle);
  redoBtn->setStyleSheet(btnStyle);

  QVBoxLayout* drawSettingsLayout = new QVBoxLayout();
  drawSettingsLayout->addWidget(drawSlider);
  drawSettingsLayout->addWidget(drawColorBtn, 0, Qt::AlignLeft);
  drawSettingsLayout->addStretch();

  QHBoxLayout* undoRedoLayout = new QHBoxLayout();
  undoRedoLayout->addWidget(undoBtn);
  undoRedoLayout->addWidget(redoBtn);
  undoRedoLayout->addStretch();

  QHBoxLayout* topRow = new QHBoxLayout();
  topRow->addLayout(shapeLayout);
  topRow->addLayout(drawSettingsLayout);

  drawLayout->addLayout(topRow);
  drawLayout->addLayout(undoRedoLayout);

  connect(undoBtn, &QPushButton::clicked, this, [this]() {
    if (m_pDrawingManager) {
      m_pDrawingManager->undo();
    }
  });

  connect(redoBtn, &QPushButton::clicked, this, [this]() {
    if (m_pDrawingManager) {
      m_pDrawingManager->redo();
    }
  });

  connect(drawSlider, &QSlider::valueChanged, this, [this](int val) {
    m_drawSize = val;
    if (m_pDrawingManager) {
      m_pDrawingManager->setSize(val);
    }
  });

  connect(drawColorBtn, &QPushButton::clicked, this, [this, drawColorBtn]() {
    QColor color = QColorDialog::getColor(m_drawColor, m_floatingToolbar, "Draw Color", QColorDialog::DontUseNativeDialog);
    if (color.isValid()) {
      m_drawColor = color;
      drawColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #777; border-radius: 3px;").arg(color.name()));
      if (m_pDrawingManager) {
        m_pDrawingManager->setColor(m_drawColor);
      }
    }
  });

  connect(m_pBtnEdit, &QPushButton::toggled, this, [this](bool checked) {
    if (checked) {
      m_currentMode = Mode::EditLayout;
      m_pBtnDraw->setChecked(false);
      m_pBtnLaser->setChecked(false);
    }
  });

  connect(m_pBtnDraw, &QPushButton::toggled, this, [this](bool checked) {
    if (checked) {
      m_currentMode = Mode::Draw;
      m_pBtnEdit->setChecked(false);
      m_pBtnLaser->setChecked(false);

      if (m_drawSettingsWidget) {
        m_drawSettingsWidget->show();
        updatePopoutPositions();
        m_drawSettingsWidget->raise();
      }
    } else {
      if (m_drawSettingsWidget) {
        m_drawSettingsWidget->hide();
      }
      // If the user manually turned the draw tool off, fall back to Edit mode.
      if (!m_pBtnLaser->isChecked()) {
        m_pBtnEdit->setChecked(true);
      }
    }
  });
}

void PresenterController::updatePopoutPositions() {
  if (!m_floatingToolbar) {
    return;
  }

  // Right edge of the floating toolbar + a 10px gap, aligned vertically.
  QPoint targetPos = m_floatingToolbar->pos() + QPoint(m_floatingToolbar->width() + 10, 0);

  if (m_laserSettingsWidget && m_laserSettingsWidget->isVisible()) {
    m_laserSettingsWidget->move(targetPos);
  }
  if (m_drawSettingsWidget && m_drawSettingsWidget->isVisible()) {
    m_drawSettingsWidget->move(targetPos);
  }
}
