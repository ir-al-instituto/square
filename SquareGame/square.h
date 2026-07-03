#ifndef SQUARE_H
#define SQUARE_H

#include <QWidget>
#include <QTimer>
#include <QSet>
#include <QList>
#include "GameSquare.h"

class Square : public QWidget
{
    Q_OBJECT

public:
    explicit Square(QWidget *parent = nullptr);
    ~Square();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void updateGame();
    void restartGame();
    void showGGDialog();
    void showVictoryDialog();

private:
    void initEnemies();
    void setRandomDirection(GameSquare& enemy);
    bool checkCollision(const GameSquare& enemy, int newX, int newY);
    bool checkEnemyCollisions(int index, int newX, int newY);
    
    QTimer *gameTimer;
    QSet<int> pressedKeys;
    
    // 游戏对象
    GameSquare player;              // 玩家方块
    QList<GameSquare> enemies;      // 敌人方块
};

#endif // SQUARE_H
