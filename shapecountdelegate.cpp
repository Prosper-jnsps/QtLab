
#include "shapecountdelegate.h"

#include <QPainter>
#include <QApplication>
#include <QStyle>
#include <QWidget>
#include <QPolygonF>
#include <QColor>

void ShapeCountDelegate::paint(
    QPainter *painter,
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    int quantity = index.data(Qt::DisplayRole).toInt();

    QString type = index.siblingAtColumn(1)
                       .data(Qt::DisplayRole).toString();

    // Проверка количества и типа
    if (quantity <= 0 ||
        (type != "Прямоугольник" &&
         type != "Эллипс" &&
         type != "Многоугольник")) {

        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // Количество отображаемых иконок
    int iconCount = 1;

    if (quantity >= 4 && quantity <= 10)
        iconCount = 2;
    else if (quantity > 10)
        iconCount = 3;

    painter->save();

    // Стандартный фон ячейки
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);
    opt.text.clear();

    QStyle *style = opt.widget
                        ? opt.widget->style()
                        : QApplication::style();

    style->drawControl(
        QStyle::CE_ItemViewItem,
        &opt,
        painter,
        opt.widget
        );

    painter->setClipRect(option.rect);
    painter->setRenderHint(QPainter::Antialiasing);

    // Размеры и расположение иконок
    const int iconSize = 16;
    const int spacing = 4;

    int startX = option.rect.left() + 6;
    int centerY = option.rect.center().y();

    QColor color;

    if (type == "Прямоугольник")
        color = QColor(135, 206, 250);
    else if (type == "Эллипс")
        color = QColor(144, 238, 144);
    else
        color = QColor(255, 215, 0);

    painter->setPen(QPen(QColor(55, 65, 75), 1));
    painter->setBrush(color);

    for (int i = 0; i < iconCount; ++i) {

        QRectF r(
            startX + i * (iconSize + spacing),
            centerY - iconSize / 2,
            iconSize,
            iconSize
            );

        // Прямоугольник
        if (type == "Прямоугольник") {
            painter->drawRect(r.adjusted(1, 2, -1, -2));
        }

        // Эллипс
        else if (type == "Эллипс") {
            painter->drawEllipse(r.adjusted(1, 2, -1, -2));
        }

        // Многоугольник
        else {
            QPolygonF polygon;

            polygon << QPointF(r.center().x(), r.top() + 1)
                    << QPointF(r.right() - 1, r.bottom() - 1)
                    << QPointF(r.left() + 1, r.bottom() - 1);

            painter->drawPolygon(polygon);
        }
    }

    // Точное количество фигур справа
    int occupied = 6 + iconCount * iconSize
                   + (iconCount - 1) * spacing + 4;

    QRect textRect = option.rect.adjusted(
        occupied, 0, -5, 0
        );

    QColor textColor = (option.state & QStyle::State_Selected)
                           ? option.palette.color(QPalette::HighlightedText)
                           : option.palette.color(QPalette::Text);

    painter->setPen(textColor);

    painter->drawText(
        textRect,
        Qt::AlignRight | Qt::AlignVCenter,
        QString::number(quantity)
        );

    painter->restore();
}

QSize ShapeCountDelegate::sizeHint(
    const QStyleOptionViewItem &option,
    const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);

    size.setWidth(qMax(size.width(), 110));
    size.setHeight(qMax(size.height(), 28));

    return size;
}
