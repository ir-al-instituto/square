#include "square.h"
#include <QPainter>
#include <QKeyEvent>
#include <QApplication>
#include <QRandomGenerator>
#include <QtMath>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

const int MIN_SPACING = 8;

Square::Square(QWidget *parent) : QWidget(parent)
{
    // 设置窗口大小
    setFixedSize(800, 600);
    setFocusPolicy(Qt::StrongFocus);
    
    // 初始化玩家方块
    player = GameSquare(385, 285, 30, Qt::green);
    
    // 初始化敌人
    initEnemies();
    
    // 创建游戏定时器
    gameTimer = new QTimer(this);
    connect(gameTimer, &QTimer::timeout, this, &Square::updateGame);
    gameTimer->start(16);
}

Square::~Square()
{
}

void Square::initEnemies()
{
    QRandomGenerator *rand = QRandomGenerator::global();
    // 创建20个敌人
    for (int i = 0; i < 20; ++i) {
        int size, x, y;
        int attemps=0;
        bool ok=true;
        do{
            size = 90 - 3.5 * i + rand->bounded(-5, 6);
            x = rand->bounded(0, width() - size);
            y = rand->bounded(0, height() - size);
            QRect tempRect = QRect(x, y, size, size);
            tempRect.adjust(-MIN_SPACING, -MIN_SPACING,
                            MIN_SPACING, MIN_SPACING);
            if (tempRect.intersects(player.getRect())) {
                ok = false;
            }
            else {
                for (const GameSquare& existing : enemies) {
                    if (tempRect.intersects(existing.getRect())) {
                        ok = false;
                        break;
                    }
                }
            }
            ++attemps;
        }while(attemps<1e6 && !ok);
        QColor color = QColor(
            rand->bounded(50, 256),
            rand->bounded(50, 256),
            rand->bounded(50, 256)
            );

        GameSquare enemy(x, y, size, color);
        setRandomDirection(enemy);
        enemies.append(enemy);
    }
}

void Square::setRandomDirection(GameSquare& enemy)
{
    QRandomGenerator *rand = QRandomGenerator::global();
    int direction = rand->bounded(4);  // 0-3
    
    switch (direction) {
    case 0:  // 上
        enemy.setSpeedX(0);
        enemy.setSpeedY(-1);
        break;
    case 1:  // 下
        enemy.setSpeedX(0);
        enemy.setSpeedY(1);
        break;
    case 2:  // 左
        enemy.setSpeedX(-1);
        enemy.setSpeedY(0);
        break;
    case 3:  // 右
        enemy.setSpeedX(1);
        enemy.setSpeedY(0);
        break;
    }
}

bool Square::checkCollision(const GameSquare& enemy, int newX, int newY)
{
    // 检查是否碰到窗口边界
    if (newX < 0 || newX > width() - enemy.getSize() ||
        newY < 0 || newY > height() - enemy.getSize()) {
        return true;
    }
    return false;
}

bool Square::checkEnemyCollisions(int index, int newX, int newY)
{
    // 获取当前敌人的矩形
    QRect currentRect(newX, newY, enemies[index].getSize(), enemies[index].getSize());
    currentRect.adjust(-MIN_SPACING, -MIN_SPACING,
                    MIN_SPACING, MIN_SPACING);
    // 检查与其他所有敌人的碰撞
    for (int i = 0; i < enemies.size(); ++i) {
        if (i == index) continue;  // 跳过自己
        
        QRect otherRect = enemies[i].getRect();
        if (currentRect.intersects(otherRect)) {
            return true;  // 检测到碰撞
        }
    }
    return false;
}

void Square::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制玩家方块
    painter.setBrush(player.getColor());
    painter.drawRect(player.getX(), player.getY(), 
                     player.getSize(), player.getSize());
    
    // 绘制所有敌人方块
    for (const GameSquare& enemy : enemies) {
        painter.setBrush(enemy.getColor());
        painter.drawRect(enemy.getX(), enemy.getY(), 
                         enemy.getSize(), enemy.getSize());
    }
}

void Square::keyPressEvent(QKeyEvent *event)
{
    pressedKeys.insert(event->key());
}

void Square::keyReleaseEvent(QKeyEvent *event)
{
    pressedKeys.remove(event->key());
}

void Square::updateGame()
{
    // 移动玩家
    int speed = 3;
    if (pressedKeys.contains(Qt::Key_Left))  player.move(-speed, 0);
    if (pressedKeys.contains(Qt::Key_Right)) player.move(speed, 0);
    if (pressedKeys.contains(Qt::Key_Up))    player.move(0, -speed);
    if (pressedKeys.contains(Qt::Key_Down))  player.move(0, speed);
    
    // 玩家边界检测
    int maxX = width() - player.getSize();
    int maxY = height() - player.getSize();
    if (player.getX() < 0) player.setX(0);
    if (player.getX() > maxX) player.setX(maxX);
    if (player.getY() < 0) player.setY(0);
    if (player.getY() > maxY) player.setY(maxY);

    QRect playerRect = player.getRect();

    // 更新所有敌人
    // 清空所有敌人：结算胜利
    if (!enemies.size()) {
        gameTimer->stop();  // 停止游戏循环
        Square::showVictoryDialog();
        return;
    }
    int baseSpeed = 1;  // 基础移动速度
    for (int i = 0; i < enemies.size(); ++i) {
        GameSquare& enemy = enemies[i];
        
        // 计算新位置
        int newX = enemy.getX() + enemy.getSpeedX() * baseSpeed;
        int newY = enemy.getY() + enemy.getSpeedY() * baseSpeed;
        
        // 检查边界碰撞
        if (checkCollision(enemy, newX, newY)) {
            // 触壁反弹：反转方向
            if (newX < 0 || newX > width() - enemy.getSize()) {
                enemy.setSpeedX(-enemy.getSpeedX());
            }
            if (newY < 0 || newY > height() - enemy.getSize()) {
                enemy.setSpeedY(-enemy.getSpeedY());
            }
            newX = enemy.getX() + enemy.getSpeedX() * baseSpeed;
            newY = enemy.getY() + enemy.getSpeedY() * baseSpeed;
        }
        // 检查与玩家碰撞（不主动冲撞玩家）
        QRect tmpRect=enemy.getRect();
        tmpRect.adjust(-MIN_SPACING, -MIN_SPACING,
                       MIN_SPACING, MIN_SPACING);
        if (!playerRect.intersects(tmpRect)) {
            // 检查与其他敌人的碰撞
           if (!checkEnemyCollisions(i, newX, newY)) {
                // 没有碰撞，执行移动
                enemy.setX(newX);
                enemy.setY(newY);
            }
            else {
                enemy.setSpeedX(-enemy.getSpeedX());
                enemy.setSpeedY(-enemy.getSpeedY());
            }
        }
        else {
            enemy.setSpeedX(-enemy.getSpeedX());
            enemy.setSpeedY(-enemy.getSpeedY());
        }
    }
    bool GG = false;
    // 玩家与敌人的碰撞检测
    for (int i = 0; i < enemies.size(); ++i) {
        const GameSquare& enemy = enemies[i];
        if (playerRect.intersects(enemy.getRect())) {
            if (player.getSize()>=enemy.getSize()) {
                int newSize = floor(sqrt(player.getSize() * player.getSize()
                                   + enemy.getSize() * enemy.getSize()));
                player.setSize(newSize);
                // 移除被吃掉的敌人
                enemies.removeAt(i);
                i--;
            }
            else {
                GG = true;
            }
            break;
        }
    }
    if (GG) {
        gameTimer->stop();
        Square::showGGDialog();
        return;  // 结束updateGame，不再继续
    }
    update();
}

// 失败窗口
void Square::showGGDialog()
{
    QDialog *gameOverDialog = new QDialog(this);
    gameOverDialog->setWindowTitle("Game Over - GG");
    gameOverDialog->setFixedSize(400, 300);
    gameOverDialog->setModal(true);
    gameOverDialog->move(
        this->mapToGlobal(
            QPoint(
                (this->width() - gameOverDialog->width()) / 2 - 200,
                (this->height() - gameOverDialog->height()) / 2 - 80
                )
            )
        );
    gameOverDialog->setWindowFlags(Qt::FramelessWindowHint);
    gameOverDialog->setStyleSheet(
        "QDialog { "
        "    background-color: rgba(30, 30, 30, 230); "
        "    border: 3px solid #ff3333; "
        "    border-radius: 15px; "
        "}"
        "QLabel#title { "
        "    color: #ff3333; "
        "    font-size: 48px; "
        "    font-weight: bold; "
        "}"
        "QLabel#info { "
        "    color: #cccccc; "
        "    font-size: 18px; "
        "}"
        "QPushButton { "
        "    background-color: #ff3333; "
        "    color: white; "
        "    font-size: 18px; "
        "    font-weight: bold; "
        "    padding: 10px 30px; "
        "    border-radius: 8px; "
        "    border: none; "
        "}"
        "QPushButton:hover { "
        "    background-color: #ff5555; "
        "}"
        );

    QVBoxLayout *layout = new QVBoxLayout(gameOverDialog);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);
    QLabel *titleLabel = new QLabel("GG", gameOverDialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    QLabel *infoLabel = new QLabel(
        QString("You are eaten by the enemy!\nCurrent Size: %1").arg(player.getSize()),
        gameOverDialog
        );
    infoLabel->setObjectName("info");
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    QPushButton *restartBtn = new QPushButton("Restart", gameOverDialog);
    connect(restartBtn, &QPushButton::clicked, [=]() {
        gameOverDialog->close();
        restartGame();
    });

    QPushButton *quitBtn = new QPushButton("Quit", gameOverDialog);
    connect(quitBtn, &QPushButton::clicked, [=]() {
        gameOverDialog->close();
        close();
    });

    buttonLayout->addWidget(restartBtn);
    buttonLayout->addWidget(quitBtn);
    layout->addLayout(buttonLayout);

    gameOverDialog->exec();
}
//胜利窗口 
void Square::showVictoryDialog()
{
    QDialog *gameOverDialog = new QDialog(this);
    gameOverDialog->setWindowTitle("Game Victory");
    gameOverDialog->setFixedSize(400, 300);
    gameOverDialog->setModal(true);
    gameOverDialog->move(
        this->mapToGlobal(
            QPoint(
                (this->width() - gameOverDialog->width()) / 2 - 200,
                (this->height() - gameOverDialog->height()) / 2 - 80
                )
            )
        );
    gameOverDialog->setWindowFlags(Qt::FramelessWindowHint);
    gameOverDialog->setStyleSheet(
        "QDialog { "
        "    background-color: rgba(30, 30, 30, 230); "
        "    border: 3px solid #ff3333; "
        "    border-radius: 15px; "
        "}"
        "QLabel#title { "
        "    color: #ff3333; "
        "    font-size: 48px; "
        "    font-weight: bold; "
        "}"
        "QLabel#info { "
        "    color: #cccccc; "
        "    font-size: 18px; "
        "}"
        "QPushButton { "
        "    background-color: #ff3333; "
        "    color: white; "
        "    font-size: 18px; "
        "    font-weight: bold; "
        "    padding: 10px 30px; "
        "    border-radius: 8px; "
        "    border: none; "
        "}"
        "QPushButton:hover { "
        "    background-color: #ff5555; "
        "}"
        );

    QVBoxLayout *layout = new QVBoxLayout(gameOverDialog);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);
    QLabel *titleLabel = new QLabel("Victory", gameOverDialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    QLabel *infoLabel = new QLabel(
        QString("You ate all the enemies!\nCurrent Size: %1").arg(player.getSize()),
        gameOverDialog
        );
    infoLabel->setObjectName("info");
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);

    QPushButton *restartBtn = new QPushButton("Restart", gameOverDialog);
    connect(restartBtn, &QPushButton::clicked, [=]() {
        gameOverDialog->close();
        restartGame();
    });

    QPushButton *quitBtn = new QPushButton("Quit", gameOverDialog);
    connect(quitBtn, &QPushButton::clicked, [=]() {
        gameOverDialog->close();
        close();
    });

    buttonLayout->addWidget(restartBtn);
    buttonLayout->addWidget(quitBtn);
    layout->addLayout(buttonLayout);

    gameOverDialog->exec();
}

void Square::restartGame()
{
    player = GameSquare(385, 285, 30, Qt::green);
    enemies.clear();
    initEnemies();
    gameTimer->start(16);
    update();
}
