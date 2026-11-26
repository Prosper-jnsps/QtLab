#ifndef SHAPECOUNTDELEGATE_H
#define SHAPECOUNTDELEGATE_H

#include <QStyledItemDelegate>

class ShapeCountDelegate : public QStyledItemDelegate
{
public:
    explicit ShapeCountDelegate(QObject *parent = nullptr)
        : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;
};


#endif // SHAPECOUNTDELEGATE_H
