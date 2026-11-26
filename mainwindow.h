
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QSqlDatabase>

class QGraphicsScene;
class QGraphicsItem;
class QGraphicsLineItem;
class QSqlTableModel;
class QTimer;
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

    QSqlDatabase m_database;
    QSqlTableModel *m_tableModel = nullptr;
    QTimer *m_positionSaveTimer = nullptr;
    void loadShapesFromDatabase();
    bool initializeDatabase();
    void addShape(ShapeType type);
    void connectSelectedShapes();
    void updateConnections();
    void deleteSelectedShape();
    void loadConnectionsFromDatabase();
    void savePositions();
};

#endif // MAINWINDOW_H
