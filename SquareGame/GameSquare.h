#ifndef GAMESQUARE_H
#define GAMESQUARE_H

#include <QColor>
#include <QRect>

class GameSquare
{
public:
    // 构造函数
    GameSquare();
    GameSquare(int x, int y, int size, QColor color);

    int getX() const { return x; }
    int getY() const { return y; }
    int getSize() const { return size; }
    QColor getColor() const { return color; }
    int getSpeedX() const { return speedX; }
    int getSpeedY() const { return speedY; }

    void setX(int newX) { x = newX; }
    void setY(int newY) { y = newY; }
    void setSize(int newSize) { size = newSize; }
    void setColor(QColor newColor) { color = newColor; }
    void setSpeedX(int newSpeedX) { speedX = newSpeedX; }
    void setSpeedY(int newSpeedY) { speedY = newSpeedY; }
    
    void move(int dx, int dy) { x += dx; y += dy; }
    
    QRect getRect() const { return QRect(x, y, size, size); }

private:
    int x;          // X坐标
    int y;          // Y坐标
    int size;       // 边长
    QColor color;   // 颜色
    int speedX;     // X方向速度（-1, 0, 1）
    int speedY;     // Y方向速度（-1, 0, 1）
};

#endif // GAMESQUARE_H
