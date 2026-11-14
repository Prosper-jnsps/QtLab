#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsItem>
#include <QPushButton>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QPolygonF>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Создание графической сцены
    QGraphicsScene *scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    scene->setSceneRect(0, 0, 800, 500);

    // Кнопка добавления прямоугольника
    connect(ui->btnRectangle, &QPushButton::clicked,
            this, [scene]() {

                static int count = 0;

                QGraphicsRectItem *rect = scene->addRect(
                    count * 20,
                    count * 20,
                    120,
                    70,
                    QPen(Qt::black),
                    QBrush(QColor(135, 206, 250))
                    );

                // Разрешить перемещение и выделение
                rect->setFlag(QGraphicsItem::ItemIsMovable);
                rect->setFlag(QGraphicsItem::ItemIsSelectable);

                count++;
            });


    // Кнопка добавления эллипса
    connect(ui->btnEllipse, &QPushButton::clicked,
            this, [scene]() {

                static int count = 0;

                QGraphicsEllipseItem *ellipse = scene->addEllipse(
                    200 + count * 20,
                    50 + count * 20,
                    120,
                    80,
                    QPen(Qt::black),
                    QBrush(QColor(144, 238, 144))
                    );

                ellipse->setFlag(QGraphicsItem::ItemIsMovable);
                ellipse->setFlag(QGraphicsItem::ItemIsSelectable);

                count++;
            });


    // Кнопка добавления многоугольника
    connect(ui->btnPolygon, &QPushButton::clicked,
            this, [scene]() {

                static int count = 0;

                QPolygonF polygon;
                polygon << QPointF(0, 90)
                        << QPointF(60, 0)
                        << QPointF(120, 90);

                QGraphicsPolygonItem *item = scene->addPolygon(
                    polygon,
                    QPen(Qt::black),
                    QBrush(QColor(255, 215, 0))
                    );

                item->setPos(250 + count * 20,
                             100 + count * 20);

                item->setFlag(QGraphicsItem::ItemIsMovable);
                item->setFlag(QGraphicsItem::ItemIsSelectable);

                count++;
            });

}


MainWindow::~MainWindow()
{
    delete ui;
}
