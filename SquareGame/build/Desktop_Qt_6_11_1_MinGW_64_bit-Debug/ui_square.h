/********************************************************************************
** Form generated from reading UI file 'square.ui'
**
** Created by: Qt User Interface Compiler version 6.11.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SQUARE_H
#define UI_SQUARE_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Square
{
public:

    void setupUi(QWidget *Square)
    {
        if (Square->objectName().isEmpty())
            Square->setObjectName("Square");
        Square->resize(800, 600);

        retranslateUi(Square);

        QMetaObject::connectSlotsByName(Square);
    } // setupUi

    void retranslateUi(QWidget *Square)
    {
        Square->setWindowTitle(QCoreApplication::translate("Square", "Square", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Square: public Ui_Square {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SQUARE_H
