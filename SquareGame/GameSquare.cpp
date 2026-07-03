#include "GameSquare.h"
#include <QRect>

GameSquare::GameSquare() 
    : x(0), y(0), size(50), color(Qt::red), speedX(0), speedY(0)
{
}

GameSquare::GameSquare(int x, int y, int size, QColor color)
    : x(x), y(y), size(size), color(color), speedX(0), speedY(0)
{
}
