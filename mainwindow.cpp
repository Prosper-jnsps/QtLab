
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "shapecountdelegate.h"
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsRectItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsLineItem>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QPolygonF>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QAbstractItemView>
#include <QHash>
#include <QComboBox>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Подключение базы данных
    if (!initializeDatabase()) {
        QMessageBox::critical(
            this,
            "Ошибка SQLite",
            "Не удалось создать или открыть базу данных!"
            );
    }

    // Создание графической сцены
    m_scene = new QGraphicsScene(this);
    m_scene->setSceneRect(0, 0, 800, 500);

    ui->graphicsView->setScene(m_scene);
    // Восстановление сохранённых фигур

    // Восстановление фигур и связей из базы данных
    if (m_database.isOpen()) {
        loadShapesFromDatabase();
        loadConnectionsFromDatabase();
    }

    // Выделение нескольких фигур
    ui->graphicsView->setDragMode(
        QGraphicsView::RubberBandDrag
        );

    // Панель фильтрации фигур
    QHBoxLayout *filterLayout = new QHBoxLayout();

    // Фильтр по ID
    QSpinBox *idFilter = new QSpinBox(ui->centralwidget);
    idFilter->setRange(0, 1000000000);
    idFilter->setSpecialValueText("Все");

    filterLayout->addWidget(new QLabel("ID:", this));
    filterLayout->addWidget(idFilter);

    // Фильтр по типу фигуры
    QComboBox *typeFilter = new QComboBox(ui->centralwidget);

    typeFilter->addItem("Все типы", "");
    typeFilter->addItem("Прямоугольник", "Прямоугольник");
    typeFilter->addItem("Эллипс", "Эллипс");
    typeFilter->addItem("Многоугольник", "Многоугольник");

    filterLayout->addWidget(new QLabel("Тип:", this));
    filterLayout->addWidget(typeFilter);

    // Фильтр по количеству фигур данного типа
    QSpinBox *countFilter = new QSpinBox(ui->centralwidget);
    countFilter->setRange(0, 1000000);
    countFilter->setSpecialValueText("Все");

    filterLayout->addWidget(
        new QLabel("Количество:", this)
        );
    filterLayout->addWidget(countFilter);

    // Кнопка сброса фильтров
    QPushButton *resetButton =
        new QPushButton("Сбросить", ui->centralwidget);

    filterLayout->addWidget(resetButton);

    // Добавляем панель под основными кнопками
    ui->verticalLayout->insertLayout(1, filterLayout);

    // Применение фильтров к модели
    auto applyFilters = [this, idFilter, typeFilter, countFilter]() {

        if (!m_tableModel)
            return;

        QStringList conditions;

        if (idFilter->value() > 0) {
            conditions << QString("id = %1")
            .arg(idFilter->value());
        }

        QString selectedType =
            typeFilter->currentData().toString();

        if (!selectedType.isEmpty()) {
            conditions << QString("type = '%1'")
            .arg(selectedType);
        }

        if (countFilter->value() > 0) {
            conditions << QString("type_count = %1")
            .arg(countFilter->value());
        }

        m_tableModel->setFilter(
            conditions.join(" AND ")
            );

        if (!m_tableModel->select()) {
            QMessageBox::warning(
                this,
                "Ошибка фильтрации",
                m_tableModel->lastError().text()
                );
        }
    };

    // Автоматическая фильтрация
    connect(idFilter, &QSpinBox::valueChanged,
            this, applyFilters);

    connect(typeFilter, &QComboBox::currentIndexChanged,
            this, applyFilters);

    connect(countFilter, &QSpinBox::valueChanged,
            this, applyFilters);

    // Сброс фильтров
    connect(resetButton, &QPushButton::clicked,
            this, [=]() {
                idFilter->setValue(0);
                typeFilter->setCurrentIndex(0);
                countFilter->setValue(0);
                applyFilters();
            });

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

    // Кнопка удаления выбранной фигуры
    connect(ui->btnDelete, &QPushButton::clicked,
            this, [this]() {
                deleteSelectedShape();
            });

    // Таймер сохранения координат фигур
    m_positionSaveTimer = new QTimer(this);
    m_positionSaveTimer->setSingleShot(true);
    m_positionSaveTimer->setInterval(300);

    connect(m_positionSaveTimer, &QTimer::timeout,
            this, &MainWindow::savePositions);

    // Обновление линий и сохранение координат
    connect(m_scene, &QGraphicsScene::changed,
            this, [this](const QList<QRectF> &) {

                updateConnections();

                // Сохраняем координаты после перемещения
                m_positionSaveTimer->start();
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

    // Ввод ширины
    int width = QInputDialog::getInt(
        this, title, "Введите ширину:",
        120, 10, 1000, 1, &ok
        );

    if (!ok) return;

    // Ввод высоты
    int height = QInputDialog::getInt(
        this, title, "Введите высоту:",
        90, 10, 1000, 1, &ok
        );

    if (!ok) return;

    // Начальные координаты
    int index = m_nextId - 1;

    qreal x = 20 + (index % 8) * 35;
    qreal y = 20 + (index % 6) * 25;

    // Сохранение фигуры в SQLite
    QSqlQuery query(m_database);

    query.prepare(
        "INSERT INTO figures "
        "(type, width, height, pos_x, pos_y) "
        "VALUES (:type, :width, :height, :x, :y)"
        );

    query.bindValue(":type", title);
    query.bindValue(":width", width);
    query.bindValue(":height", height);
    query.bindValue(":x", x);
    query.bindValue(":y", y);

    if (!query.exec()) {
        QMessageBox::critical(
            this,
            "Ошибка SQLite",
            query.lastError().text()
            );
        return;
    }

    // ID назначается базой данных
    int id = query.lastInsertId().toInt();
    m_nextId = id + 1;

    // Создание графической фигуры
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
    item->setData(0, id);
    item->setPos(x, y);

    // Перемещение и выделение
    item->setFlag(QGraphicsItem::ItemIsMovable);
    item->setFlag(QGraphicsItem::ItemIsSelectable);
    item->setZValue(1);

    // Отображение ID на фигуре
    auto *label = new QGraphicsSimpleTextItem(
        QString("ID %1").arg(id), item
        );

    label->setPos(
        item->boundingRect().center()
        - label->boundingRect().center()
        );

    label->setAcceptedMouseButtons(Qt::NoButton);

    m_scene->clearSelection();

    // Обновление количества фигур каждого типа
    QSqlQuery countQuery(m_database);

    if (!countQuery.exec(
            "UPDATE figures SET type_count = "
            "(SELECT COUNT(*) FROM figures AS f "
            "WHERE f.type = figures.type)"
            )) {
        QMessageBox::warning(
            this, "Ошибка SQLite",
            countQuery.lastError().text()
            );
    }

    // Обновление таблицы
    if (!m_tableModel->select()) {
        QMessageBox::warning(
            this, "Ошибка таблицы",
            m_tableModel->lastError().text()
            );
    }
}


// Создание связи между двумя фигурами

void MainWindow::connectSelectedShapes()
{
    QList<QGraphicsItem *> selected;

    // Получаем выбранные фигуры
    for (QGraphicsItem *item : m_scene->selectedItems()) {
        if (item->data(0).isValid()) {
            selected.append(item);
        }
    }

    if (selected.size() != 2) {
        QMessageBox::warning(
            this, "Ошибка",
            "Выберите ровно две фигуры!"
            );
        return;
    }

    QGraphicsItem *first = selected[0];
    QGraphicsItem *second = selected[1];

    int id1 = first->data(0).toInt();
    int id2 = second->data(0).toInt();

    // Проверка существующих графических связей
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

    // Сохраняем ID в одном порядке
    int smallerId = qMin(id1, id2);
    int largerId = qMax(id1, id2);

    // Начинаем транзакцию
    if (!m_database.transaction()) {
        QMessageBox::critical(
            this, "Ошибка SQLite",
            m_database.lastError().text()
            );
        return;
    }

    // Сохранение связи в базе данных
    QSqlQuery query(m_database);

    query.prepare(
        "INSERT INTO figure_connections "
        "(figure1_id, figure2_id) "
        "VALUES (:first, :second)"
        );

    query.bindValue(":first", smallerId);
    query.bindValue(":second", largerId);

    if (!query.exec()) {
        QString error = query.lastError().text();
        m_database.rollback();

        QMessageBox::warning(
            this, "Ошибка SQLite",
            error
            );
        return;
    }

    // Обновляем идентификаторы связанных фигур
    QSqlQuery updateQuery(m_database);

    if (!updateQuery.exec(R"(
        UPDATE figures
        SET connected_ids = COALESCE(
            (
                SELECT GROUP_CONCAT(
                    CASE
                        WHEN c.figure1_id = figures.id
                        THEN c.figure2_id
                        ELSE c.figure1_id
                    END, ', '
                )
                FROM figure_connections AS c
                WHERE c.figure1_id = figures.id
                   OR c.figure2_id = figures.id
            ),
            ''
        )
    )")) {
        QString error = updateQuery.lastError().text();
        m_database.rollback();

        QMessageBox::critical(
            this, "Ошибка SQLite", error
            );
        return;
    }

    if (!m_database.commit()) {
        QString error = m_database.lastError().text();
        m_database.rollback();

        QMessageBox::critical(
            this, "Ошибка SQLite", error
            );
        return;
    }

    // Создаём графическую линию
    QPointF p1 = first->sceneBoundingRect().center();
    QPointF p2 = second->sceneBoundingRect().center();

    QGraphicsLineItem *line = m_scene->addLine(
        QLineF(p1, p2),
        QPen(QColor(70, 70, 70), 2)
        );

    line->setZValue(0);

    m_connections.append({first, second, line});

    m_scene->clearSelection();

    // Обновляем таблицу
    if (!m_tableModel->select()) {
        QMessageBox::warning(
            this, "Ошибка таблицы",
            m_tableModel->lastError().text()
            );
    }
}




// Удаление выбранной фигуры и её связей


void MainWindow::deleteSelectedShape()
{
    QList<QGraphicsItem *> selected;

    for (QGraphicsItem *item : m_scene->selectedItems()) {
        if (item->data(0).isValid()) {
            selected.append(item);
        }
    }

    if (selected.size() != 1) {
        QMessageBox::warning(
            this, "Ошибка",
            "Выберите одну фигуру для удаления!"
            );
        return;
    }

    QGraphicsItem *item = selected.first();
    int id = item->data(0).toInt();

    if (!m_database.transaction()) {
        QMessageBox::critical(
            this, "Ошибка SQLite",
            m_database.lastError().text()
            );
        return;
    }

    // Удаляем связи из базы данных
    QSqlQuery deleteLinks(m_database);

    deleteLinks.prepare(
        "DELETE FROM figure_connections "
        "WHERE figure1_id = :id1 OR figure2_id = :id2"
        );

    deleteLinks.bindValue(":id1", id);
    deleteLinks.bindValue(":id2", id);

    if (!deleteLinks.exec()) {
        QString error = deleteLinks.lastError().text();
        m_database.rollback();
        QMessageBox::critical(this, "Ошибка SQLite", error);
        return;
    }

    // Удаляем фигуру из базы данных
    QSqlQuery deleteFigure(m_database);

    deleteFigure.prepare(
        "DELETE FROM figures WHERE id = :id"
        );
    deleteFigure.bindValue(":id", id);

    if (!deleteFigure.exec()) {
        QString error = deleteFigure.lastError().text();
        m_database.rollback();
        QMessageBox::critical(this, "Ошибка SQLite", error);
        return;
    }

    if (deleteFigure.numRowsAffected() != 1) {
        m_database.rollback();
        QMessageBox::warning(
            this, "Ошибка",
            "Фигура не найдена в базе данных!"
            );
        return;
    }

    // Обновляем список связей оставшихся фигур
    QSqlQuery updateLinks(m_database);

    if (!updateLinks.exec(R"(
        UPDATE figures
        SET connected_ids = COALESCE(
            (
                SELECT GROUP_CONCAT(
                    CASE
                        WHEN c.figure1_id = figures.id
                        THEN c.figure2_id
                        ELSE c.figure1_id
                    END, ', '
                )
                FROM figure_connections AS c
                WHERE c.figure1_id = figures.id
                   OR c.figure2_id = figures.id
            ),
            ''
        )
    )")) {
        QString error = updateLinks.lastError().text();
        m_database.rollback();
        QMessageBox::critical(this, "Ошибка SQLite", error);
        return;
    }

    // Обновляем количество фигур каждого типа
    QSqlQuery updateCount(m_database);

    if (!updateCount.exec(R"(
        UPDATE figures
        SET type_count = (
            SELECT COUNT(*)
            FROM figures AS f
            WHERE f.type = figures.type
        )
    )")) {
        QString error = updateCount.lastError().text();
        m_database.rollback();
        QMessageBox::critical(this, "Ошибка SQLite", error);
        return;
    }

    if (!m_database.commit()) {
        QString error = m_database.lastError().text();
        m_database.rollback();
        QMessageBox::critical(this, "Ошибка SQLite", error);
        return;
    }

    // Удаляем графические линии
    for (int i = m_connections.size() - 1; i >= 0; --i) {
        const Connection &connection = m_connections[i];

        if (connection.first == item ||
            connection.second == item) {

            QGraphicsLineItem *line = connection.line;
            m_connections.removeAt(i);
            delete line;
        }
    }

    // Удаляем графическую фигуру
    delete item;

    // Обновляем отображение таблицы
    if (!m_tableModel->select()) {
        QMessageBox::warning(
            this, "Ошибка таблицы",
            m_tableModel->lastError().text()
            );
    }

    qDebug() << "Удалена фигура ID:" << id;
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

// Инициализация базы данных SQLite
bool MainWindow::initializeDatabase()
{
    // Постоянная папка для базы данных
    QString dataPath = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation
        );

    if (!QDir().mkpath(dataPath)) {
        qWarning() << "Не удалось создать папку БД";
        return false;
    }

    QString databasePath =
        QDir(dataPath).filePath("shapelink.db");

    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(databasePath);

    if (!m_database.open()) {
        qWarning() << m_database.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);

    // Включаем каскадное удаление связей
    if (!query.exec("PRAGMA foreign_keys = ON")) {
        qWarning() << query.lastError().text();
        return false;
    }

    // Таблица фигур
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS figures (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            type TEXT NOT NULL,
            width INTEGER NOT NULL,
            height INTEGER NOT NULL,
            pos_x REAL NOT NULL DEFAULT 0,
            pos_y REAL NOT NULL DEFAULT 0,
            connected_ids TEXT NOT NULL DEFAULT '',
            type_count INTEGER NOT NULL DEFAULT 1
        )
    )")) {
        qWarning() << query.lastError().text();
        return false;
    }

    // Таблица связей
    if (!query.exec(R"(
        CREATE TABLE IF NOT EXISTS figure_connections (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            figure1_id INTEGER NOT NULL,
            figure2_id INTEGER NOT NULL,
            FOREIGN KEY (figure1_id)
                REFERENCES figures(id) ON DELETE CASCADE,
            FOREIGN KEY (figure2_id)
                REFERENCES figures(id) ON DELETE CASCADE,
            UNIQUE (figure1_id, figure2_id),
            CHECK (figure1_id != figure2_id)
        )
    )")) {
        qWarning() << query.lastError().text();
        return false;
    }

    // Модель данных для таблицы
    m_tableModel = new QSqlTableModel(this, m_database);
    m_tableModel->setTable("figures");
    m_tableModel->setEditStrategy(
        QSqlTableModel::OnManualSubmit
        );

    m_tableModel->setHeaderData(
        0, Qt::Horizontal, "ID");
    m_tableModel->setHeaderData(
        1, Qt::Horizontal, "Тип");
    m_tableModel->setHeaderData(
        2, Qt::Horizontal, "Ширина");
    m_tableModel->setHeaderData(
        3, Qt::Horizontal, "Высота");
    m_tableModel->setHeaderData(
        4, Qt::Horizontal, "X");
    m_tableModel->setHeaderData(
        5, Qt::Horizontal, "Y");
    m_tableModel->setHeaderData(
        6, Qt::Horizontal, "Связи");
    m_tableModel->setHeaderData(
        7, Qt::Horizontal, "Количество");

    ui->tableView->setModel(m_tableModel);

    // Делегат отображения количества фигур
    ui->tableView->setItemDelegateForColumn(
        7,
        new ShapeCountDelegate(ui->tableView)
        );

    // Ширина и высота ячеек
    ui->tableView->setColumnWidth(7, 120);
    ui->tableView->verticalHeader()->setDefaultSectionSize(32);

    ui->tableView->setEditTriggers(
        QAbstractItemView::NoEditTriggers
        );

    if (!m_tableModel->select()) {
        qWarning() << m_tableModel->lastError().text();
        return false;
    }

    qDebug() << "SQLite:" << databasePath;
    return true;
}

void MainWindow::loadShapesFromDatabase()
{
    // Загрузка фигур из базы данных
    QSqlQuery query(m_database);

    if (!query.exec(
            "SELECT id, type, width, height, "
            "pos_x, pos_y FROM figures ORDER BY id"
            )) {
        QMessageBox::warning(
            this,
            "Ошибка SQLite",
            query.lastError().text()
            );
        return;
    }

    while (query.next()) {

        int id = query.value(0).toInt();
        QString type = query.value(1).toString();
        int width = query.value(2).toInt();
        int height = query.value(3).toInt();
        qreal x = query.value(4).toDouble();
        qreal y = query.value(5).toDouble();

        QGraphicsItem *item = nullptr;

        // Восстановление прямоугольника
        if (type == "Прямоугольник") {

            item = m_scene->addRect(
                0, 0, width, height,
                QPen(Qt::black),
                QBrush(QColor(135, 206, 250))
                );

            // Восстановление эллипса
        } else if (type == "Эллипс") {

            item = m_scene->addEllipse(
                0, 0, width, height,
                QPen(Qt::black),
                QBrush(QColor(144, 238, 144))
                );

            // Восстановление многоугольника
        } else if (type == "Многоугольник") {

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

        if (!item) {
            qWarning() << "Неизвестный тип фигуры:" << type;
            continue;
        }

        // Восстановление идентификатора
        item->setData(0, id);

        // Восстановление координат
        item->setPos(x, y);

        // Перемещение и выделение
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setFlag(QGraphicsItem::ItemIsSelectable);
        item->setZValue(1);

        // Отображение ID
        auto *label = new QGraphicsSimpleTextItem(
            QString("ID %1").arg(id), item
            );

        label->setPos(
            item->boundingRect().center()
            - label->boundingRect().center()
            );

        label->setAcceptedMouseButtons(Qt::NoButton);

        // Следующий уникальный идентификатор
        if (id >= m_nextId) {
            m_nextId = id + 1;
        }
    }
}

void MainWindow::loadConnectionsFromDatabase()
{
    // Поиск фигур по идентификаторам
    QHash<int, QGraphicsItem *> figures;

    for (QGraphicsItem *item : m_scene->items()) {
        if (item->data(0).isValid()) {
            int id = item->data(0).toInt();
            figures.insert(id, item);
        }
    }

    // Получение связей из базы данных
    QSqlQuery query(m_database);

    if (!query.exec(
            "SELECT figure1_id, figure2_id "
            "FROM figure_connections"
            )) {
        QMessageBox::warning(
            this,
            "Ошибка SQLite",
            query.lastError().text()
            );
        return;
    }

    while (query.next()) {

        int id1 = query.value(0).toInt();
        int id2 = query.value(1).toInt();

        // Проверка существования фигур
        if (!figures.contains(id1) ||
            !figures.contains(id2)) {
            qWarning() << "Связь с неизвестной фигурой:"
                       << id1 << id2;
            continue;
        }

        QGraphicsItem *first = figures.value(id1);
        QGraphicsItem *second = figures.value(id2);

        // Координаты центров фигур
        QPointF p1 =
            first->sceneBoundingRect().center();

        QPointF p2 =
            second->sceneBoundingRect().center();

        // Восстановление графической линии
        QGraphicsLineItem *line = m_scene->addLine(
            QLineF(p1, p2),
            QPen(QColor(70, 70, 70), 2)
            );

        line->setZValue(0);

        // Сохранение связи в памяти
        m_connections.append({
            first,
            second,
            line
        });
    }
}

void MainWindow::savePositions()
{
    if (!m_database.isOpen() || !m_scene)
        return;

    QSqlQuery query(m_database);

    // Обновляем координаты только при изменении
    if (!query.prepare(R"(
        UPDATE figures
        SET pos_x = :x, pos_y = :y
        WHERE id = :id
          AND (pos_x <> :check_x
               OR pos_y <> :check_y)
    )")) {
        qWarning() << query.lastError().text();
        return;
    }

    bool updated = false;

    for (QGraphicsItem *item : m_scene->items()) {

        // Пропускаем линии и подписи
        if (!item->data(0).isValid())
            continue;

        int id = item->data(0).toInt();
        QPointF pos = item->pos();

        query.bindValue(":id", id);
        query.bindValue(":x", pos.x());
        query.bindValue(":y", pos.y());
        query.bindValue(":check_x", pos.x());
        query.bindValue(":check_y", pos.y());

        if (!query.exec()) {
            qWarning() << "Ошибка сохранения ID:"
                       << id
                       << query.lastError().text();
            continue;
        }

        if (query.numRowsAffected() > 0)
            updated = true;
    }

    // Обновляем таблицу при изменении координат
    if (updated && m_tableModel) {
        if (!m_tableModel->select()) {
            qWarning() << m_tableModel->lastError().text();
        }
    }
}

MainWindow::~MainWindow()
{
    // Сохранение последних координат перед закрытием
    if (m_positionSaveTimer)
        m_positionSaveTimer->stop();

    savePositions();

    delete ui;
}



