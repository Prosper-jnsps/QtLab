
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsLineItem;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    enum class ShapeType {
        Rectangle,
        Ellipse,
        Polygon
    };

    struct Connection {
        QGraphicsItem *first;
        QGraphicsItem *second;
        QGraphicsLineItem *line;
    };

    Ui::MainWindow *ui;
    QGraphicsScene *m_scene = nullptr;

    int m_nextId = 1;
    QVector<Connection> m_connections;

    void addShape(ShapeType type);
    void connectSelectedShapes();
    void updateConnections();
};

#endif // MAINWINDOW_H
