
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>

#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QPolygonF>
#include <QPen>
#include <QBrush>
#include <QColor>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Создание графической сцены
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 800, 500);

    ui->graphicsView->setScene(m_scene);

    // Выделение нескольких фигур
    ui->graphicsView->setDragMode(
        QGraphicsView::RubberBandDrag
        );

    // Кнопки создания фигур
    connect(ui->btnRectangle, &QPushButton::clicked,
            this, [this]() {
                addShape(ShapeType::Rectangle);
            });

    connect(ui->btnEllipse, &QPushButton::clicked,
            this, [this]() {
                addShape(ShapeType::Ellipse);
            });

    connect(ui->btnPolygon, &QPushButton::clicked,
            this, [this]() {
                addShape(ShapeType::Polygon);
            });

    // Кнопка создания связи
    connect(ui->btnConnect, &QPushButton::clicked,
            this, [this]() {
                connectSelectedShapes();
            });

    // Обновление линий при перемещении фигур
    connect(m_scene, &QGraphicsScene::changed,
            this, [this](const QList<QRectF> &) {
                updateConnections();
            });
}


// Создание фигуры
void MainWindow::addShape(ShapeType type)
{
    QString title;

    if (type == ShapeType::Rectangle)
        title = "Прямоугольник";
    else if (type == ShapeType::Ellipse)
        title = "Эллипс";
    else
        title = "Многоугольник";

    bool ok = false;

    int width = QInputDialog::getInt(
        this, title, "Введите ширину:",
        120, 10, 1000, 1, &ok
        );

    if (!ok) return;

    int height = QInputDialog::getInt(
        this, title, "Введите высоту:",
        90, 10, 1000, 1, &ok
        );

    if (!ok) return;

    QGraphicsItem *item = nullptr;

    if (type == ShapeType::Rectangle) {

        item = m_scene->addRect(
            0, 0, width, height,
            QPen(Qt::black),
            QBrush(QColor(135, 206, 250))
            );

    } else if (type == ShapeType::Ellipse) {

        item = m_scene->addEllipse(
            0, 0, width, height,
            QPen(Qt::black),
            QBrush(QColor(144, 238, 144))
            );

    } else {

        QPolygonF polygon;
        polygon << QPointF(0, height)
                << QPointF(width / 2.0, 0)
                << QPointF(width, height);

        item = m_scene->addPolygon(
            polygon,
            QPen(Qt::black),
            QBrush(QColor(255, 215, 0))
            );
    }

    // Уникальный идентификатор
    int id = m_nextId++;
    item->setData(0, id);

    // Начальное положение
    item->setPos(
        20 + ((id - 1) % 8) * 35,
        20 + ((id - 1) % 6) * 25
        );

    // Перемещение и выделение
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setFlag(QGraphicsItem::ItemIsSelectable);
    item->setZValue(1);

    // Отображение идентификатора
    auto *label = new QGraphicsSimpleTextItem(
        QString("ID %1").arg(id), item
        );

    label->setPos(
        item->boundingRect().center()
        - label->boundingRect().center()
        );

    label->setAcceptedMouseButtons(Qt::NoButton);

    m_scene->clearSelection();
}


// Создание связи между двумя фигурами
void MainWindow::connectSelectedShapes()
{
    QList<QGraphicsItem *> selected;

    for (QGraphicsItem *item : m_scene->selectedItems()) {
        if (item->data(0).isValid()) {
            selected.append(item);
        }
    }

    if (selected.size() != 2) {
        QMessageBox::warning(
            this,
            "Ошибка",
            "Выберите ровно две фигуры!"
            );
        return;
    }

    QGraphicsItem *first = selected[0];
    QGraphicsItem *second = selected[1];

    // Проверка существующей связи
    for (const Connection &c : m_connections) {
        if ((c.first == first && c.second == second) ||
            (c.first == second && c.second == first)) {

            QMessageBox::information(
                this, "Информация",
                "Эти фигуры уже связаны!"
                );
            return;
        }
    }

    QPointF p1 = first->sceneBoundingRect().center();
    QPointF p2 = second->sceneBoundingRect().center();

    // Создание линии
    QGraphicsLineItem *line = m_scene->addLine(
        QLineF(p1, p2),
        QPen(QColor(70, 70, 70), 2)
        );

    line->setZValue(0);

    m_connections.append({first, second, line});

    m_scene->clearSelection();
}


// Обновление связей
void MainWindow::updateConnections()
{
    for (const Connection &c : m_connections) {

        QPointF p1 =
            c.first->sceneBoundingRect().center();

        QPointF p2 =
            c.second->sceneBoundingRect().center();

        QLineF current = c.line->line();

        // Обновляем только при изменении координат
        if (current.p1() != p1 ||
            current.p2() != p2) {

            c.line->setLine(QLineF(p1, p2));
        }
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}
