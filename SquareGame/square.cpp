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
#include <QScrollArea>
#include <QPolygonF>

const int MIN_SPACING = 8;
const int PLAYER_SPAWN_CLEARANCE = 130;
const int PLAYER_DANGER_MARGIN = 24;
const int POWERUP_DROP_CHANCE = 32;
const int POWERUP_RADIUS = 11;
const int POWERUP_DROP_MIN_DISTANCE = 140;
const int POWERUP_DURATION_FRAMES = 360;
const int SHRINK_BOMB_FRAMES = 180;
const int MAGNET_RANGE = 190;
const int BLINKER_PERIOD_FRAMES = 96;
const double BLINKER_SCALE_RANGE = 0.22;
const double HORROR_MERGE_PENALTY = 0.80;
const int SPLIT_CHILD_MIN_SIZE = 10;

Square::Square(QWidget *parent) //构造函数
    : QWidget(parent), gameTimer(new QTimer(this)), difficulty(Difficulty::Medium), gameStarted(false)
    //把Square初始化成Qt窗口
    //创建游戏定时器，驱动游戏循环
    //默认难度为medium
    //游戏刚创建还没开始
{
    setFixedSize(800, 600);
    setWindowTitle("Square");
    setFocusPolicy(Qt::StrongFocus); //允许窗口获得键盘焦点

    connect(gameTimer, &QTimer::timeout, this, &Square::updateGame);
    resetGameState(); //初始化游戏状态

    QTimer::singleShot(0, this, &Square::showMainMenu); //窗口事件循环开始后弹出主菜单
}

Square::~Square()
{
}

Square::DifficultySettings Square::currentSettings() const
{   //根据当前难度，返回对应游戏参数
    switch (difficulty) {
    case Difficulty::Easy:
        return {"Easy", 14, 75, 3, 1, 4, 1, 5, 3};
    case Difficulty::Medium:
        return {"Medium", 20, 90, 4, 1, 3, 2, 5, 5};
    case Difficulty::Tough:
        return {"Tough", 26, 105, 3, 2, 3, 3, 6, 7};
    case Difficulty::HellNo:
        return {"Hell NO", 34, 120, 3, 2, 2, 4, 7, 9};
    }
    return {"Medium", 20, 90, 4, 1, 3, 2, 5, 5};
}

QString Square::difficultyName() const
{   //返回当前游戏难度名称
    return currentSettings().name;
}

QString Square::dialogStyleSheet() const
{   //统一返回所有弹窗的样式表
    return
        "QDialog {"
        "    background-color: rgba(28, 30, 34, 245);"
        "    border: 3px solid #22c55e;"
        "    border-radius: 12px;"
        "}"
        "QLabel#title {"
        "    color: #f8fafc;"
        "    font-size: 42px;"
        "    font-weight: bold;"
        "}"
        "QLabel#info {"
        "    color: #cbd5e1;"
        "    font-size: 16px;"
        "    line-height: 130%;"
        "}"
        "QPushButton {"
        "    background-color: #2563eb;"
        "    color: white;"
        "    font-size: 17px;"
        "    font-weight: bold;"
        "    padding: 9px 24px;"
        "    border-radius: 7px;"
        "    border: none;"
        "    min-width: 150px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #3b82f6;"
        "}"
        "QPushButton#danger {"
        "    background-color: #dc2626;"
        "}"
        "QPushButton#danger:hover {"
        "    background-color: #ef4444;"
        "}"
        "QPushButton#accent {"
        "    background-color: #16a34a;"
        "}"
        "QPushButton#accent:hover {"
        "    background-color: #22c55e;"
        "}"
        "QPushButton#selected {"
        "    background-color: #16a34a;"
        "}"
        "QPushButton[resultButton=\"true\"] {"
        "    min-width: 104px;"
        "    max-width: 104px;"
        "    padding: 9px 0px;"
        "}"
        "QScrollArea {"
        "    background-color: transparent;"
        "    border: none;"
        "}"
        "QScrollArea QWidget {"
        "    background-color: transparent;"
        "}";
}

void Square::resetGameState()
{   //重置游戏状态
    player = GameSquare(385, 285, 30, Qt::green);
    enemies.clear();             //敌人方块列表
    enemyTypes.clear();          //敌人类型
    enemyBaseSizes.clear();      //敌人基础大小，主要是闪烁方块
    enemyPhaseOffsets.clear();   //闪烁方块相对偏移
    shrinkRestoreSizes.clear();  //缩小炸弹结束回复敌人大小
    eatEffects.clear();          //吞噬特效
    powerUps.clear();            //道具
    pressedKeys.clear();         //当前按下的键
    //状态计数器归0
    playerPulseFrames = 0;   //脉冲特效帧数
    score = 0;               //分数
    dashFrames = 0;          //冲刺帧数
    dashCooldownFrames = 0;  //冲刺冷却
    shieldFrames = 0;        //护盾时间
    slowFieldFrames = 0;     //减速场时间
    magnetFrames = 0;        //磁铁时间
    shrinkBombFrames = 0;    //缩小炸弹时间
    powerUpsDropped = 0;     //掉落道具数量
    gameFrame = 0;           //游戏帧数
}

void Square::updateVisualEffects()
{   //更新视觉特效生命周期
    ++gameFrame;

    for (int i = 0; i < eatEffects.size(); ++i) {
        ++eatEffects[i].age;
        if (eatEffects[i].age > eatEffects[i].maxAge) {
            eatEffects.removeAt(i);
            --i;
        }
    }

    if (playerPulseFrames > 0) {
        --playerPulseFrames;
    }
}

void Square::startGame()
{   //开始游戏
    resetGameState();
    initEnemies();
    gameStarted = true;
    gameTimer->start(16);
    setFocus();
    update();
}

void Square::centerDialog(QDialog& dialog) const
{
    QPoint topLeft = mapToGlobal(QPoint((width() - dialog.width()) / 2,
                                        (height() - dialog.height()) / 2));
    dialog.move(topLeft);
    dialog.raise();
    dialog.activateWindow();
}

void Square::showMainMenu()
{   //显示主菜单
    gameTimer->stop();
    gameStarted = false;
    resetGameState();
    update();

    QDialog menu(this);
    menu.setWindowTitle("Square Menu");
    menu.setFixedSize(430, 390);
    menu.setModal(true);
    menu.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    menu.setStyleSheet(dialogStyleSheet());

    QVBoxLayout *layout = new QVBoxLayout(&menu);
    layout->setContentsMargins(34, 30, 34, 30);
    layout->setSpacing(16);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("Square", &menu);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel *difficultyLabel = new QLabel(QString("Difficulty: %1").arg(difficultyName()), &menu);
    difficultyLabel->setObjectName("info");
    difficultyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(difficultyLabel);

    QPushButton *startBtn = new QPushButton("Start", &menu);
    QPushButton *difficultyBtn = new QPushButton("Difficulty", &menu);
    QPushButton *helpBtn = new QPushButton("Help", &menu);
    QPushButton *quitBtn = new QPushButton("Quit", &menu);
    helpBtn->setObjectName("accent");
    quitBtn->setObjectName("danger");

    layout->addWidget(startBtn);
    layout->addWidget(difficultyBtn);
    layout->addWidget(helpBtn);
    layout->addWidget(quitBtn);

    connect(startBtn, &QPushButton::clicked, [&]() {
        menu.accept();
        startGame();
    });
    connect(difficultyBtn, &QPushButton::clicked, [&]() {
        showDifficultyDialog();
        difficultyLabel->setText(QString("Difficulty: %1").arg(difficultyName()));
    });
    connect(helpBtn, &QPushButton::clicked, this, &Square::showHelpDialog);
    connect(quitBtn, &QPushButton::clicked, [&]() {
        menu.reject();
        close();
    });

    centerDialog(menu);
    menu.exec();
}

void Square::showDifficultyDialog()
{   //难度选择栏
    QDialog dialog(this);
    dialog.setWindowTitle("Choose Difficulty");
    dialog.setFixedSize(470, 360);
    dialog.setModal(true);
    dialog.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dialog.setStyleSheet(dialogStyleSheet());

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(34, 30, 34, 30);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("Difficulty", &dialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    auto addDifficultyButton = [&](Difficulty value, const QString& text) {
        QPushButton *button = new QPushButton(text, &dialog);
        if (difficulty == value) {
            button->setObjectName("selected");
        }
        connect(button, &QPushButton::clicked, [&dialog, this, value]() {
            setDifficulty(value);
            dialog.accept();
        });
        layout->addWidget(button);
    };

    addDifficultyButton(Difficulty::Easy, "Easy");
    addDifficultyButton(Difficulty::Medium, "Medium");
    addDifficultyButton(Difficulty::Tough, "Tough");
    addDifficultyButton(Difficulty::HellNo, "Hell NO");

    QPushButton *backBtn = new QPushButton("Back", &dialog);
    backBtn->setObjectName("danger");
    connect(backBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    layout->addWidget(backBtn);

    centerDialog(dialog);
    dialog.exec();
}

void Square::showHelpDialog()
{    //规则帮助栏
    QDialog dialog(this);
    dialog.setWindowTitle("Help");
    dialog.setFixedSize(560, 360);
    dialog.setModal(true);
    dialog.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dialog.setStyleSheet(dialogStyleSheet());

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(34, 30, 34, 30);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("Help", &dialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel *infoLabel = new QLabel(
        "Game Rules\n"
        "Use the arrow keys to move the green square.\n"
        "Eat squares that are smaller than you to grow.\n"
        "Avoid larger squares, or the game is over.\n"
        "Eat every enemy square to win.\n\n"
        "Green enemy outline: safe to eat.\n"
        "Red enemy outline: dangerous.\n"
        "Plain square: moves in a straight line.\n"
        "Purple diamond: Hunter enemy that slowly tracks you.\n"
        "Cyan pulse: Blinker enemy that grows and shrinks over time.\n"
        "Orange plus: Splitter enemy that splits into two smaller squares when eaten.\n"
        "Black X: Horror enemy looks edible, but eating it cuts the merged size by 20%.\n"
        "Pink bolt: Speedster ignores Slow Field and hunts you while Slow Field is active.\n\n"
        "Press Space to dash for a short burst.\n"
        "Dash has a cooldown, shown in the HUD.\n\n"
        "Eating enemies may drop a yellow round power-up.\n"
        "Power-ups scatter to random places and keep moving.\n"
        "Touch it to reveal and activate one effect.\n"
        "Each difficulty has a total power-up limit.\n"
        "The game pauses for 3 seconds while the effect is shown.\n"
        "Power-ups: Shield, Slow Field, Magnet, Shrink Bomb.\n\n"
        "Press Esc during the game to pause.\n"
        "Pause menu options: Resume, Restart, Quit.\n\n"
        "Eating enemies gives score.\n"
        "Harder difficulties have higher score multipliers.\n"
        "Difficulty changes enemy count, enemy size, enemy speed, and player speed.",
        &dialog);
    infoLabel->setObjectName("info");
    infoLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    infoLabel->setWordWrap(true);

    QScrollArea *scrollArea = new QScrollArea(&dialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setFixedHeight(160);
    scrollArea->setWidget(infoLabel);
    layout->addWidget(scrollArea);

    QPushButton *backBtn = new QPushButton("Back", &dialog);
    connect(backBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addWidget(backBtn, 0, Qt::AlignCenter);

    centerDialog(dialog);
    dialog.exec();
}

void Square::setDifficulty(Difficulty newDifficulty)
{
    difficulty = newDifficulty;
}

void Square::initEnemies()
{   //根据当前难度初始化整局游戏的敌人方块
    //取得随机生成器，读取当前难度配置
    QRandomGenerator *rand = QRandomGenerator::global();
    DifficultySettings settings = currentSettings();

    for (int i = 0; i < settings.enemyCount; ++i) {
        int size = 0;
        int x = 0;
        int y = 0;
        int attempts = 0;
        bool ok = false;

        do {
            ok = true;
            if (i < settings.starterEnemyCount) {         //前几个开局敌人保证小于玩家
                size = 16 + i * 2 + rand->bounded(-1, 2);
                if (size > player.getSize() - 1) {
                    size = player.getSize() - 1;
                }
            } else {                                     //后面敌人越来越大
                int growIndex = i - settings.starterEnemyCount;
                int growCount = qMax(1, settings.enemyCount - settings.starterEnemyCount - 1);
                double progress = static_cast<double>(growIndex) / growCount;
                size = 32 + static_cast<int>((settings.enemyBaseSize - 32) * progress)
                       + rand->bounded(-6, 7);
                if (size < player.getSize() + 2) {
                    size = player.getSize() + 2;
                }
                if (size > settings.enemyBaseSize) {
                    size = settings.enemyBaseSize;
                }
            }
            //随机生成位置
            x = rand->bounded(0, width() - size);
            y = rand->bounded(0, height() - size);

            //出生位置检查
            QRect tempRect(x, y, size, size);
            tempRect.adjust(-MIN_SPACING, -MIN_SPACING, MIN_SPACING, MIN_SPACING);

            //敌人矩形扩一圈安全距离，避免敌人互相贴太近
            QRect playerSafeRect = player.getRect();
            playerSafeRect.adjust(-PLAYER_SPAWN_CLEARANCE, -PLAYER_SPAWN_CLEARANCE,
                                  PLAYER_SPAWN_CLEARANCE, PLAYER_SPAWN_CLEARANCE);

            if (tempRect.intersects(playerSafeRect)) {
                ok = false;
            } else {
                for (const GameSquare& existing : enemies) {
                    if (tempRect.intersects(existing.getRect())) {
                        ok = false;
                        break;
                    }
                }
            }

            ++attempts;
            //找不到合适位置跳过这个敌人
        } while (attempts < 1000000 && !ok);

        if (!ok) {
            continue;
        }

        //随即决定敌人类型
        EnemyType enemyType = randomEnemyType(i);
        if (enemyType == EnemyType::Horror) {
            size = qMax(12, player.getSize() - rand->bounded(3, 9));
        }

        //创建敌人
        //随机初始移动方向
        //加入敌人列表，记录类型、基础大小、闪烁动画相位等等
        GameSquare enemy(x, y, size, enemyColorForType(enemyType));
        setRandomDirection(enemy);
        appendEnemy(enemy, enemyType, size, rand->bounded(BLINKER_PERIOD_FRAMES));
    }
}

void Square::setRandomDirection(GameSquare& enemy)
{   //给敌人随机设置初始移动方向
    QRandomGenerator *rand = QRandomGenerator::global();
    int direction = rand->bounded(4);

    switch (direction) {
    case 0:
        enemy.setSpeedX(0);
        enemy.setSpeedY(-1);
        break;
    case 1:
        enemy.setSpeedX(0);
        enemy.setSpeedY(1);
        break;
    case 2:
        enemy.setSpeedX(-1);
        enemy.setSpeedY(0);
        break;
    case 3:
        enemy.setSpeedX(1);
        enemy.setSpeedY(0);
        break;
    }
}

bool Square::checkCollision(const GameSquare& enemy, int newX, int newY)
{   //判断敌人移动到新位置会不会撞到窗口边界，检查新位置是否合法
    if (newX < 0 || newX > width() - enemy.getSize() ||
        newY < 0 || newY > height() - enemy.getSize()) {
        return true;
    }
    return false;
}

bool Square::checkEnemyCollisions(int index, int newX, int newY)
{   //判断某个敌人移动到新位置之后会不会和其他敌人发生碰撞
    QRect currentRect(newX, newY, enemies[index].getSize(), enemies[index].getSize());
    currentRect.adjust(-MIN_SPACING, -MIN_SPACING, MIN_SPACING, MIN_SPACING);

    for (int i = 0; i < enemies.size(); ++i) {
        if (i == index) {
            continue;
        }

        if (currentRect.intersects(enemies[i].getRect())) {
            return true;
        }
    }
    return false;
}

bool Square::isNearPlayer(const GameSquare& enemy, int margin) const
{   //判断某个敌人是否靠近玩家
    QRect playerSafeRect = player.getRect();
    playerSafeRect.adjust(-margin, -margin, margin, margin);
    return playerSafeRect.intersects(enemy.getRect());
}

void Square::steerAwayFromPlayer(GameSquare& enemy)
{   //让某个敌人改变方向远离玩家，让敌人朝远离玩家的一边移动
    int enemyCenterX = enemy.getX() + enemy.getSize() / 2;
    int enemyCenterY = enemy.getY() + enemy.getSize() / 2;
    int playerCenterX = player.getX() + player.getSize() / 2;
    int playerCenterY = player.getY() + player.getSize() / 2;
    int dx = enemyCenterX - playerCenterX;
    int dy = enemyCenterY - playerCenterY;

    if (qAbs(dx) >= qAbs(dy)) {
        enemy.setSpeedX(dx >= 0 ? 1 : -1);
        enemy.setSpeedY(0);
    } else {
        enemy.setSpeedX(0);
        enemy.setSpeedY(dy >= 0 ? 1 : -1);
    }
}

void Square::steerHunterTowardPlayer(int index, int step)
{   //让Hunter敌人选择一个更靠近玩家、同时不会撞墙或撞其他敌人的方向
    if (index < 0 || index >= enemies.size()) {
        return;
    }

    GameSquare& enemy = enemies[index];
    int playerCenterX = player.getX() + player.getSize() / 2;
    int playerCenterY = player.getY() + player.getSize() / 2;

    const int directions[4][2] = {
        {1, 0},
        {-1, 0},
        {0, 1},
        {0, -1}
    };

    bool foundRoute = false;
    int bestSpeedX = enemy.getSpeedX();
    int bestSpeedY = enemy.getSpeedY();
    qint64 bestDistance = 0;

    for (const auto& direction : directions) {
        int candidateX = enemy.getX() + direction[0] * step;
        int candidateY = enemy.getY() + direction[1] * step;

        if (checkCollision(enemy, candidateX, candidateY)
            || checkEnemyCollisions(index, candidateX, candidateY)) {
            continue;
        }

        int candidateCenterX = candidateX + enemy.getSize() / 2;
        int candidateCenterY = candidateY + enemy.getSize() / 2;
        qint64 dx = candidateCenterX - playerCenterX;
        qint64 dy = candidateCenterY - playerCenterY;
        qint64 distance = dx * dx + dy * dy;

        if (!foundRoute || distance < bestDistance) {
            foundRoute = true;
            bestDistance = distance;
            bestSpeedX = direction[0];
            bestSpeedY = direction[1];
        }
    }

    if (foundRoute) {
        enemy.setSpeedX(bestSpeedX);
        enemy.setSpeedY(bestSpeedY);
    } else {
        enemy.setSpeedX(-enemy.getSpeedX());
        enemy.setSpeedY(-enemy.getSpeedY());
    }
}

Square::EnemyType Square::randomEnemyType(int index) const
{   //根据当前难度和敌人序号，随机决定这个敌人属于那种类型
    DifficultySettings settings = currentSettings();
    // 前几个初始敌人固定为普通小方块，保证开局有稳定成长空间。
    if (index < settings.starterEnemyCount) {
        return EnemyType::Normal;
    }

    // 难度越高，特殊敌人的出现概率越高；未命中特殊概率时回退为普通方块。
    int hunterChance = 0;
    int blinkerChance = 0;
    int splitterChance = 0;
    int horrorChance = 0;
    int speedsterChance = 0;

    switch (difficulty) {
    case Difficulty::Easy:
        hunterChance = 8;
        blinkerChance = 9;
        splitterChance = 7;
        horrorChance = 5;
        speedsterChance = 4;
        break;
    case Difficulty::Medium:
        hunterChance = 14;
        blinkerChance = 13;
        splitterChance = 11;
        horrorChance = 8;
        speedsterChance = 7;
        break;
    case Difficulty::Tough:
        hunterChance = 18;
        blinkerChance = 15;
        splitterChance = 14;
        horrorChance = 10;
        speedsterChance = 10;
        break;
    case Difficulty::HellNo:
        hunterChance = 22;
        blinkerChance = 17;
        splitterChance = 16;
        horrorChance = 12;
        speedsterChance = 13;
        break;
    }

    int roll = QRandomGenerator::global()->bounded(100);
    if (roll < hunterChance) {
        return EnemyType::Hunter;
    }
    roll -= hunterChance;
    if (roll < blinkerChance) {
        return EnemyType::Blinker;
    }
    roll -= blinkerChance;
    if (roll < splitterChance) {
        return EnemyType::Splitter;
    }
    roll -= splitterChance;
    if (roll < horrorChance) {
        return EnemyType::Horror;
    }
    roll -= horrorChance;
    if (roll < speedsterChance) {
        return EnemyType::Speedster;
    }
    return EnemyType::Normal;
}

QColor Square::enemyColorForType(EnemyType type) const
{   //决定哪个敌人方块的颜色
    QRandomGenerator *rand = QRandomGenerator::global();
    switch (type) {
    case EnemyType::Normal:
        // 普通方块使用随机颜色，用于维持原始游戏的多彩视觉。
        return QColor(rand->bounded(50, 256),
                      rand->bounded(50, 256),
                      rand->bounded(50, 256));
    case EnemyType::Hunter:
        // 追踪方块为紫色，并在绘制时叠加菱形标记。
        return QColor(168, 85, 247);
    case EnemyType::Blinker:
        // 闪烁方块为青色，并在绘制时叠加圆环标记。
        return QColor(34, 211, 238);
    case EnemyType::Splitter:
        // 分裂方块为橙色，并在绘制时叠加加号标记。
        return QColor(249, 115, 22);
    case EnemyType::Horror:
        // 恐怖方块为黑色，并在绘制时叠加白色 X 标记。
        return QColor(17, 24, 39);
    case EnemyType::Speedster:
        // 神速方块为粉色，并在绘制时叠加闪电标记。
        return QColor(236, 72, 153);
    }
    return QColor(148, 163, 184);
}

void Square::appendEnemy(const GameSquare& enemy, EnemyType type, int baseSize, int phaseOffset)
{   // 敌人的类型、基础尺寸、闪烁相位和缩小炸弹恢复尺寸都与 enemies 下标一一对应。
    enemies.append(enemy);
    enemyTypes.append(type);
    enemyBaseSizes.append(baseSize);
    enemyPhaseOffsets.append(phaseOffset);
    shrinkRestoreSizes.append(0);
}

void Square::updateBlinkerEnemies()
{
    for (int i = 0; i < enemies.size(); ++i) {
        // 闪烁方块被缩小炸弹影响时，暂时停止周期缩放，避免尺寸恢复逻辑互相覆盖。
        if (i >= enemyTypes.size() || enemyTypes[i] != EnemyType::Blinker
            || i >= enemyBaseSizes.size() || i >= shrinkRestoreSizes.size()
            || shrinkRestoreSizes[i] > 0) {
            continue;
        }

        int phaseOffset = i < enemyPhaseOffsets.size() ? enemyPhaseOffsets[i] : 0;
        double wave = qSin((gameFrame + phaseOffset) * 2.0 * M_PI / BLINKER_PERIOD_FRAMES);
        int oldSize = enemies[i].getSize();
        int newSize = qMax(SPLIT_CHILD_MIN_SIZE,
                           static_cast<int>(floor(enemyBaseSizes[i]
                                                  * (1.0 + BLINKER_SCALE_RANGE * wave))));
        if (newSize == oldSize) {
            continue;
        }

        int centerX = enemies[i].getX() + oldSize / 2;
        int centerY = enemies[i].getY() + oldSize / 2;
        enemies[i].setSize(newSize);
        enemies[i].setX(qBound(0, centerX - newSize / 2, width() - newSize));
        enemies[i].setY(qBound(0, centerY - newSize / 2, height() - newSize));
    }
}

void Square::spawnSplitEnemies(const GameSquare& enemy, EnemyType sourceType)
{
    // 只有分裂方块触发该逻辑；太小的分裂方块不再继续拆分，避免无限增殖。
    if (sourceType != EnemyType::Splitter || enemy.getSize() < SPLIT_CHILD_MIN_SIZE * 2) {
        return;
    }

    int childSize = qMax(SPLIT_CHILD_MIN_SIZE, enemy.getSize() / 2);
    const int directions[4][2] = {
        {-1, 0},
        {1, 0},
        {0, -1},
        {0, 1}
    };
    int spacing = qMax(childSize + MIN_SPACING,
                       player.getSize() / 2 + childSize + MIN_SPACING);
    int spawned = 0;

    // 按左右上下寻找生成位置，尽量避开玩家和现有敌人，避免刚分裂就立即重叠。
    for (const auto& direction : directions) {
        int childX = qBound(0,
                            enemy.getX() + enemy.getSize() / 2 - childSize / 2
                                + direction[0] * spacing,
                            width() - childSize);
        int childY = qBound(0,
                            enemy.getY() + enemy.getSize() / 2 - childSize / 2
                                + direction[1] * spacing,
                            height() - childSize);

        QRect childRect(childX, childY, childSize, childSize);
        QRect paddedChildRect = childRect.adjusted(-MIN_SPACING,
                                                   -MIN_SPACING,
                                                   MIN_SPACING,
                                                   MIN_SPACING);
        if (paddedChildRect.intersects(player.getRect())) {
            continue;
        }

        bool overlapsEnemy = false;
        for (const GameSquare& existing : enemies) {
            if (paddedChildRect.intersects(existing.getRect())) {
                overlapsEnemy = true;
                break;
            }
        }
        if (overlapsEnemy) {
            continue;
        }

        GameSquare child(childX, childY, childSize, enemyColorForType(EnemyType::Normal));
        setRandomDirection(child);
        appendEnemy(child,
                    EnemyType::Normal,
                    childSize,
                    QRandomGenerator::global()->bounded(BLINKER_PERIOD_FRAMES));
        ++spawned;
        if (spawned == 2) {
            break;
        }
    }
}

int Square::mergedSizeAfterEating(const GameSquare& enemy, EnemyType type) const
{   //恐怖方块效果
    int mergedSize = static_cast<int>(floor(sqrt(player.getSize() * player.getSize()
                                           + enemy.getSize() * enemy.getSize())));
    if (type == EnemyType::Horror) {
        // 恐怖方块的陷阱效果：虽然能被吃掉，但最终合并尺寸会减少 20%。
        mergedSize = qMax(1, static_cast<int>(floor(mergedSize * HORROR_MERGE_PENALTY)));
    }
    return mergedSize;
}

void Square::removeEnemyAt(int index)
{   // 删除敌人时同步删除所有并行属性，防止后续敌人类型和尺寸数据错位。
    enemies.removeAt(index);
    if (index < enemyTypes.size()) {
        enemyTypes.removeAt(index);
    }
    if (index < enemyBaseSizes.size()) {
        enemyBaseSizes.removeAt(index);
    }
    if (index < enemyPhaseOffsets.size()) {
        enemyPhaseOffsets.removeAt(index);
    }
    if (index < shrinkRestoreSizes.size()) {
        shrinkRestoreSizes.removeAt(index);
    }
}

void Square::spawnPowerUp(const QPointF& center)
{   //玩家吃掉敌人之后按概率生成一个道具
    //读取当前难度配置
    DifficultySettings settings = currentSettings();
    //限制道具掉落数量
    if (powerUpsDropped >= settings.maxPowerUps || powerUps.size() >= 5) {
        return;
    }

    QRandomGenerator *rand = QRandomGenerator::global();
    //随机生成 0~99 的随机数
    if (rand->bounded(100) >= POWERUP_DROP_CHANCE) {
        return;
    }

    //创建道具
    PowerUp powerUp;
    powerUp.radius = POWERUP_RADIUS;
    powerUp.type = static_cast<PowerUpType>(rand->bounded(4));

    //决定道具掉落位置
    QPointF dropCenter = center;
    for (int attempt = 0; attempt < 80; ++attempt) {
        double x = rand->bounded(powerUp.radius, width() - powerUp.radius);
        double y = rand->bounded(powerUp.radius, height() - powerUp.radius);
        QPointF candidate(x, y);
        double dx = candidate.x() - center.x();
        double dy = candidate.y() - center.y();
        if (dx * dx + dy * dy >= POWERUP_DROP_MIN_DISTANCE * POWERUP_DROP_MIN_DISTANCE) {
            dropCenter = candidate;
            break;
        }
    }

    //找不到合适位置就掉落在原来center附近
    powerUp.center = dropCenter;
    setRandomPowerUpDirection(powerUp);
    powerUps.append(powerUp);
    ++powerUpsDropped;
}

void Square::setRandomPowerUpDirection(PowerUp& powerUp)
{   //刚生成的道具随机设置一个移动方向
    QRandomGenerator *rand = QRandomGenerator::global();
    int direction = rand->bounded(4);

    switch (direction) {
    case 0:
        powerUp.speedX = 0;
        powerUp.speedY = -1;
        break;
    case 1:
        powerUp.speedX = 0;
        powerUp.speedY = 1;
        break;
    case 2:
        powerUp.speedX = -1;
        powerUp.speedY = 0;
        break;
    case 3:
        powerUp.speedX = 1;
        powerUp.speedY = 0;
        break;
    }
}

void Square::movePowerUps(int speed)
{   //让场上所有道具移动，碰到窗口边界时反弹
    for (PowerUp& powerUp : powerUps) {
        QPointF nextCenter(powerUp.center.x() + powerUp.speedX * speed,
                           powerUp.center.y() + powerUp.speedY * speed);

        if (nextCenter.x() < powerUp.radius || nextCenter.x() > width() - powerUp.radius) {
            powerUp.speedX = -powerUp.speedX;
            nextCenter.setX(powerUp.center.x() + powerUp.speedX * speed);
        }
        if (nextCenter.y() < powerUp.radius || nextCenter.y() > height() - powerUp.radius) {
            powerUp.speedY = -powerUp.speedY;
            nextCenter.setY(powerUp.center.y() + powerUp.speedY * speed);
        }

        powerUp.center.setX(qBound(static_cast<double>(powerUp.radius),
                                   nextCenter.x(),
                                   static_cast<double>(width() - powerUp.radius)));
        powerUp.center.setY(qBound(static_cast<double>(powerUp.radius),
                                   nextCenter.y(),
                                   static_cast<double>(height() - powerUp.radius)));
    }
}

void Square::checkPowerUpCollisions()
{   //检查玩家是否碰到道具
    QRect playerRect = player.getRect();
    for (int i = 0; i < powerUps.size(); ++i) {
        const PowerUp& powerUp = powerUps[i];
        //用外接矩形检测碰撞
        QRectF powerUpRect(powerUp.center.x() - powerUp.radius,
                           powerUp.center.y() - powerUp.radius,
                           powerUp.radius * 2,
                           powerUp.radius * 2);
        //判断道具矩形是否与玩家相交
        if (powerUpRect.intersects(QRectF(playerRect))) {
            PowerUpType type = powerUp.type;
            powerUps.removeAt(i);
            activatePowerUp(type);
            return;
        }
    }
}

QString Square::powerUpName(PowerUpType type) const
{   //玩家拿到道具之后显示道具名称
    switch (type) {
    case PowerUpType::Shield:
        return "Shield";
    case PowerUpType::SlowField:
        return "Slow Field";
    case PowerUpType::Magnet:
        return "Magnet";
    case PowerUpType::ShrinkBomb:
        return "Shrink Bomb";
    }
    return "Power-up";
}

QString Square::powerUpDescription(PowerUpType type) const
{   //拿到道具之后显示道具功能
    switch (type) {
    case PowerUpType::Shield:
        return "Blocks one hit from a bigger enemy square for 6 seconds.";
    case PowerUpType::SlowField:
        return "Slows most enemies for 6 seconds. Speedsters ignore it and hunt you while it is active.";
    case PowerUpType::Magnet:
        return "Pulls nearby edible squares toward you for 6 seconds.";
    case PowerUpType::ShrinkBomb:
        return "Shrinks bigger enemies for 3 seconds, then restores them to 160% of their original size.";
    }
    return "A mysterious square-side advantage.";
}

void Square::activatePowerUp(PowerUpType type)
{   //拿到道具之后使用道具功能，若为时效性道具则触发倒计时
    switch (type) {
    case PowerUpType::Shield:
        shieldFrames = POWERUP_DURATION_FRAMES;
        break;
    case PowerUpType::SlowField:
        slowFieldFrames = POWERUP_DURATION_FRAMES;
        break;
    case PowerUpType::Magnet:
        magnetFrames = POWERUP_DURATION_FRAMES;
        break;
    case PowerUpType::ShrinkBomb:
        applyShrinkBomb();
        break;
    }

    showPowerUpDialog(type);
}

void Square::showPowerUpDialog(PowerUpType type)
{   //玩家拿到道具之后暂停游戏并弹出说明窗口，3秒后游戏继续
    gameTimer->stop();
    pressedKeys.clear();

    QDialog dialog(this);
    dialog.setWindowTitle(powerUpName(type));
    dialog.setFixedSize(470, 260);
    dialog.setModal(true);
    dialog.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    dialog.setStyleSheet(dialogStyleSheet());

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(34, 30, 34, 30);
    layout->setSpacing(18);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel(powerUpName(type), &dialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel *infoLabel = new QLabel(powerUpDescription(type)
                                   + "\n\nGame resumes automatically in 3 seconds.",
                                   &dialog);
    infoLabel->setObjectName("info");
    infoLabel->setAlignment(Qt::AlignCenter);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    centerDialog(dialog);
    QTimer::singleShot(3000, &dialog, &QDialog::accept);
    dialog.exec();

    if (gameStarted) {
        gameTimer->start(16);
        setFocus();
    }
}

void Square::applyMagnetEffect()
{   //磁铁道具功能实现
    for (int i = 0; i < enemies.size(); ++i) {
        GameSquare& enemy = enemies[i];
        if (enemy.getSize() > player.getSize()) {
            continue;
        }

        int enemyCenterX = enemy.getX() + enemy.getSize() / 2;
        int enemyCenterY = enemy.getY() + enemy.getSize() / 2;
        int playerCenterX = player.getX() + player.getSize() / 2;
        int playerCenterY = player.getY() + player.getSize() / 2;
        int dx = playerCenterX - enemyCenterX;
        int dy = playerCenterY - enemyCenterY;
        if (dx * dx + dy * dy > MAGNET_RANGE * MAGNET_RANGE) {
            continue;
        }

        int moveX = 0;
        int moveY = 0;
        if (qAbs(dx) >= qAbs(dy)) {
            moveX = dx > 0 ? 1 : -1;
        } else {
            moveY = dy > 0 ? 1 : -1;
        }

        int newX = enemy.getX() + moveX;
        int newY = enemy.getY() + moveY;
        if (!checkCollision(enemy, newX, newY)
            && !checkEnemyCollisions(i, newX, newY)) {
            enemy.setX(newX);
            enemy.setY(newY);
        }
    }
}

void Square::applyShrinkBomb()
{   //缩小炸弹功能实现
    shrinkBombFrames = SHRINK_BOMB_FRAMES;

    for (int i = 0; i < enemies.size(); ++i) {
        GameSquare& enemy = enemies[i];
        if (enemy.getSize() <= player.getSize()) {
            continue;
        }

        if (i < shrinkRestoreSizes.size() && shrinkRestoreSizes[i] == 0) {
            shrinkRestoreSizes[i] = enemy.getSize();
        }

        int newSize = qMax(player.getSize() + 1,
                           static_cast<int>(floor(enemy.getSize() * 0.72)));
        enemy.setSize(newSize);
        if (i < enemyBaseSizes.size()) {
            enemyBaseSizes[i] = newSize;
        }
        if (enemy.getX() > width() - enemy.getSize()) {
            enemy.setX(width() - enemy.getSize());
        }
        if (enemy.getY() > height() - enemy.getSize()) {
            enemy.setY(height() - enemy.getSize());
        }
    }
}

void Square::restoreShrinkBombEnemies()
{   //缩小炸弹时间结束反效果实现，敌人大小回复160%
    int maxSize = qMin(width(), height()) - 1;

    for (int i = 0; i < enemies.size() && i < shrinkRestoreSizes.size(); ++i) {
        if (shrinkRestoreSizes[i] <= 0) {
            continue;
        }

        int restoredSize = qMin(maxSize,
                                static_cast<int>(floor(shrinkRestoreSizes[i] * 2.6)));
        enemies[i].setSize(restoredSize);
        if (i < enemyBaseSizes.size()) {
            enemyBaseSizes[i] = restoredSize;
        }
        if (enemies[i].getX() > width() - enemies[i].getSize()) {
            enemies[i].setX(width() - enemies[i].getSize());
        }
        if (enemies[i].getY() > height() - enemies[i].getSize()) {
            enemies[i].setY(height() - enemies[i].getSize());
        }
        shrinkRestoreSizes[i] = 0;
    }

    shrinkBombFrames = 0;
}

void Square::paintEvent(QPaintEvent *event)
{   //游戏的界面绘制函数
    //刷新窗口时调用此函数
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    //填充深色背景
    painter.fillRect(rect(), QColor(15, 23, 42));

    //画网格背景
    painter.setPen(QColor(30, 41, 59));
    for (int x = 0; x < width(); x += 40) {
        painter.drawLine(x, 0, x, height());
    }
    for (int y = 0; y < height(); y += 40) {
        painter.drawLine(0, y, width(), y);
    }

    if (!gameStarted) {
        painter.setPen(QColor(226, 232, 240));
        painter.setFont(QFont("Arial", 30, QFont::Bold));
        painter.drawText(rect(), Qt::AlignCenter, "Square");
        return;
    }

    //画玩家
    QRect playerDrawRect(player.getX(), player.getY(), player.getSize(), player.getSize());
    if (playerPulseFrames > 0) {
        QColor pulseColor(250, 204, 21, 80 + playerPulseFrames * 8);
        QRect pulseRect = playerDrawRect.adjusted(-5, -5, 5, 5);
        painter.setPen(QPen(pulseColor, 4));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(pulseRect);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(player.getColor());
    painter.drawRect(playerDrawRect);

    //画所有敌人
    for (int i = 0; i < enemies.size(); ++i) {
        const GameSquare& enemy = enemies[i];
        QColor outlineColor = enemy.getSize() <= player.getSize()
                                  ? QColor(34, 197, 94)
                                  : QColor(239, 68, 68);
        QRect enemyRect(enemy.getX(), enemy.getY(), enemy.getSize(), enemy.getSize());
        painter.setPen(QPen(outlineColor, 3));
        painter.setBrush(enemy.getColor());
        painter.drawRect(enemyRect);

        EnemyType type = i < enemyTypes.size() ? enemyTypes[i] : EnemyType::Normal;
        if (type == EnemyType::Hunter) {
            QPointF center(enemy.getX() + enemy.getSize() / 2.0,
                           enemy.getY() + enemy.getSize() / 2.0);
            double markSize = qMax(7.0, enemy.getSize() * 0.15);
            QPolygonF marker;
            marker << QPointF(center.x(), center.y() - markSize)
                   << QPointF(center.x() + markSize, center.y())
                   << QPointF(center.x(), center.y() + markSize)
                   << QPointF(center.x() - markSize, center.y());
            painter.setPen(QPen(QColor(245, 208, 254), 2));
            painter.setBrush(QColor(126, 34, 206, 210));
            painter.drawPolygon(marker);
        } else if (type == EnemyType::Blinker) {
            QRectF inner(enemyRect.adjusted(enemy.getSize() / 4,
                                            enemy.getSize() / 4,
                                            -enemy.getSize() / 4,
                                            -enemy.getSize() / 4));
            painter.setPen(QPen(QColor(165, 243, 252), 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(inner);
        } else if (type == EnemyType::Splitter) {
            painter.setPen(QPen(QColor(255, 237, 213), 3));
            int cx = enemy.getX() + enemy.getSize() / 2;
            int cy = enemy.getY() + enemy.getSize() / 2;
            int arm = qMax(5, enemy.getSize() / 5);
            painter.drawLine(cx - arm, cy, cx + arm, cy);
            painter.drawLine(cx, cy - arm, cx, cy + arm);
        } else if (type == EnemyType::Horror) {
            painter.setPen(QPen(QColor(248, 250, 252), 3));
            int pad = qMax(4, enemy.getSize() / 5);
            painter.drawLine(enemy.getX() + pad,
                             enemy.getY() + pad,
                             enemy.getX() + enemy.getSize() - pad,
                             enemy.getY() + enemy.getSize() - pad);
            painter.drawLine(enemy.getX() + enemy.getSize() - pad,
                             enemy.getY() + pad,
                             enemy.getX() + pad,
                             enemy.getY() + enemy.getSize() - pad);
        } else if (type == EnemyType::Speedster) {
            painter.setPen(QPen(QColor(252, 231, 243), 3));
            QPointF center(enemy.getX() + enemy.getSize() / 2.0,
                           enemy.getY() + enemy.getSize() / 2.0);
            double markSize = qMax(7.0, enemy.getSize() * 0.18);
            QPolygonF bolt;
            bolt << QPointF(center.x() - markSize * 0.2, center.y() - markSize)
                 << QPointF(center.x() + markSize * 0.55, center.y() - markSize * 0.1)
                 << QPointF(center.x() + markSize * 0.1, center.y() - markSize * 0.1)
                 << QPointF(center.x() + markSize * 0.35, center.y() + markSize)
                 << QPointF(center.x() - markSize * 0.55, center.y() + markSize * 0.05)
                 << QPointF(center.x() - markSize * 0.1, center.y() + markSize * 0.05);
            painter.setBrush(QColor(252, 231, 243, 180));
            painter.drawPolygon(bolt);
        }
    }

    //画道具
    for (const PowerUp& powerUp : powerUps) {
        painter.setPen(QPen(QColor(253, 224, 71), 3));
        painter.setBrush(QColor(250, 204, 21));
        painter.drawEllipse(powerUp.center, powerUp.radius, powerUp.radius);
        painter.setPen(QColor(113, 63, 18));
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        QRectF markRect(powerUp.center.x() - powerUp.radius,
                        powerUp.center.y() - powerUp.radius,
                        powerUp.radius * 2,
                        powerUp.radius * 2);
        painter.drawText(markRect, Qt::AlignCenter, "?");
    }

    //画吞噬特效
    for (const EatEffect& effect : eatEffects) {
        double progress = static_cast<double>(effect.age) / effect.maxAge;
        int alpha = qMax(0, static_cast<int>(210 * (1.0 - progress)));
        double radius = effect.startRadius + 48.0 * progress;
        QColor ringColor = effect.color;
        ringColor.setAlpha(alpha);
        painter.setPen(QPen(ringColor, 4));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(effect.center, radius, radius);
    }

    painter.setPen(QColor(226, 232, 240));
    painter.setFont(QFont("Arial", 13, QFont::Bold));
    QString dashText = dashCooldownFrames == 0 && dashFrames == 0
                           ? "Ready"
                           : QString::number((dashCooldownFrames + 59) / 60);
    if (dashFrames > 0) {
        dashText = "Boost";
    }
    //画HUD信息
    QString powerText = QString("Shield:%1  Slow:%2  Magnet:%3  Shrink:%4")
                            .arg(shieldFrames > 0 ? QString::number((shieldFrames + 59) / 60) : "Off")
                            .arg(slowFieldFrames > 0 ? QString::number((slowFieldFrames + 59) / 60) : "Off")
                            .arg(magnetFrames > 0 ? QString::number((magnetFrames + 59) / 60) : "Off")
                            .arg(shrinkBombFrames > 0 ? QString::number((shrinkBombFrames + 59) / 60) : "Off");
    painter.drawText(12, 24, QString("Difficulty: %1    Size: %2    Enemies: %3    Score: %4    Dash: %5")
                            .arg(difficultyName())
                            .arg(player.getSize())
                            .arg(enemies.size())
                            .arg(score)
                            .arg(dashText));
    painter.drawText(12, 46, powerText);
}

void Square::keyPressEvent(QKeyEvent *event)
{   //处理键盘按下事件
    //处理暂停 'Esc'
    if (event->key() == Qt::Key_Escape && gameStarted && gameTimer->isActive()) {
        pressedKeys.clear();
        gameTimer->stop();
        showPauseDialog();
        return;
    }

    //处理空格冲刺 'Space'
    if (event->key() == Qt::Key_Space && gameStarted && gameTimer->isActive()
        && dashFrames == 0 && dashCooldownFrames == 0 && !event->isAutoRepeat()) {
        dashFrames = 18;
        dashCooldownFrames = 105;
        return;
    }

    //处理方向键
    pressedKeys.insert(event->key());
}

void Square::keyReleaseEvent(QKeyEvent *event)
{   //处理键盘松开事件
    //保证玩家移动随按键松开而停止
    pressedKeys.remove(event->key());
}

void Square::updateGame()
{   //游戏每帧更新函数
    if (!gameStarted) {
        return;
    }

    updateVisualEffects();

    //读取当前难度配置
    DifficultySettings settings = currentSettings();
    int speed = settings.playerSpeed;
    //冲刺逻辑
    if (dashFrames > 0) {
        speed = settings.playerSpeed * 3;
        --dashFrames;
    }
    //道具倒计时
    if (dashCooldownFrames > 0) {
        --dashCooldownFrames;
    }
    if (shieldFrames > 0) {
        --shieldFrames;
    }
    if (shrinkBombFrames > 0) {
        --shrinkBombFrames;
        if (shrinkBombFrames == 0) {
            //恢复缩小的敌人
            restoreShrinkBombEnemies();
        }
    }

    //玩家移动
    if (pressedKeys.contains(Qt::Key_Left))  player.move(-speed, 0);
    if (pressedKeys.contains(Qt::Key_Right)) player.move(speed, 0);
    if (pressedKeys.contains(Qt::Key_Up))    player.move(0, -speed);
    if (pressedKeys.contains(Qt::Key_Down))  player.move(0, speed);

    updateBlinkerEnemies();  //更新闪烁放块的大小

    //限制玩家不能出界
    int maxX = width() - player.getSize();
    int maxY = height() - player.getSize();
    if (player.getX() < 0) player.setX(0);
    if (player.getX() > maxX) player.setX(maxX);
    if (player.getY() < 0) player.setY(0);
    if (player.getY() > maxY) player.setY(maxY);

    QRect playerRect = player.getRect();

    //胜负判断
    if (enemies.isEmpty()) {
        gameTimer->stop();
        gameStarted = false;
        showVictoryDialog();
        return;
    }

    //移动道具，再检测玩家是否拿到道具
    movePowerUps(settings.enemySpeed);
    checkPowerUpCollisions();

    //磁铁道具生效
    if (magnetFrames > 0) {
        applyMagnetEffect();
        --magnetFrames;
    }

    int baseSpeed = settings.enemySpeed;
    bool enemiesMoveThisFrame = true;
    if (slowFieldFrames > 0) {
        // 减速场会降低大多数敌人的移动频率或速度；神速方块在后续逻辑中单独豁免。
        --slowFieldFrames;
        if (baseSpeed > 1) {
            baseSpeed = qMax(1, baseSpeed - 1);
        } else {
            enemiesMoveThisFrame = slowFieldFrames % 2 == 0;
        }
    }

    //敌人移动循环
    for (int i = 0; i < enemies.size(); ++i) {
        GameSquare& enemy = enemies[i];
        //读取敌人类型
        EnemyType type = i < enemyTypes.size() ? enemyTypes[i] : EnemyType::Normal;
        bool isHunter = type == EnemyType::Hunter;
        bool isSpeedster = type == EnemyType::Speedster;
        bool speedsterHunts = isSpeedster && slowFieldFrames > 0;
        int enemyStep = (isSpeedster && slowFieldFrames > 0) ? settings.enemySpeed : baseSpeed;

        // 普通、闪烁、分裂、恐怖方块默认保持直线移动；减速场可能让它们隔帧暂停。
        if (!enemiesMoveThisFrame && !isSpeedster) {
            continue;
        }
        // 追踪方块隔帧行动，形成“缓慢朝玩家靠近”的效果。
        if (isHunter && gameFrame % 2 != 0) {
            continue;
        }

        // 追踪方块始终追玩家；神速方块只在减速场开启时追玩家，并且不受减速影响。
        if (isHunter || speedsterHunts) {
            steerHunterTowardPlayer(i, enemyStep);
        }

        //计算敌人新位置
        int newX = enemy.getX() + enemy.getSpeedX() * enemyStep;
        int newY = enemy.getY() + enemy.getSpeedY() * enemyStep;

        //撞墙效果
        if (checkCollision(enemy, newX, newY)) {
            if (newX < 0 || newX > width() - enemy.getSize()) {
                enemy.setSpeedX(-enemy.getSpeedX());
            }
            if (newY < 0 || newY > height() - enemy.getSize()) {
                enemy.setSpeedY(-enemy.getSpeedY());
            }
            newX = enemy.getX() + enemy.getSpeedX() * enemyStep;
            newY = enemy.getY() + enemy.getSpeedY() * enemyStep;
        }

        //没撞到其他敌人
        if (!checkEnemyCollisions(i, newX, newY)) {
            enemy.setX(newX);
            enemy.setY(newY);
        } else {
            enemy.setSpeedX(-enemy.getSpeedX());
            enemy.setSpeedY(-enemy.getSpeedY());
        }
    }

    bool gameOver = false;
    for (int i = 0; i < enemies.size(); ++i) {
        const GameSquare& enemy = enemies[i];
        //玩家和敌人碰撞判断
        if (playerRect.intersects(enemy.getRect())) {
            if (player.getSize() >= enemy.getSize()) {
                //玩家大于敌人，可吞噬
                EnemyType eatenType = i < enemyTypes.size() ? enemyTypes[i] : EnemyType::Normal;
                GameSquare eatenEnemy = enemy;
                EatEffect effect;
                effect.center = QPointF(enemy.getX() + enemy.getSize() / 2.0,
                                        enemy.getY() + enemy.getSize() / 2.0);
                effect.color = enemy.getColor();
                effect.age = 0;
                effect.maxAge = 22;
                effect.startRadius = enemy.getSize() / 2.0;
                eatEffects.append(effect);
                playerPulseFrames = 14;

                //计算玩家新大小
                int newSize = mergedSizeAfterEating(enemy, eatenType);
                //吃到恐怖方块额外减少20%
                score += enemy.getSize() * settings.scoreMultiplier;
                player.setSize(newSize);
                spawnPowerUp(effect.center);
                removeEnemyAt(i);
                // 如果吃掉的是分裂方块，原方块消失后会生成两个更小的普通方块。
                spawnSplitEnemies(eatenEnemy, eatenType);
                --i;
            } else //一般会失败
            {   //如果有护盾
                if (shieldFrames > 0) {
                    shieldFrames = 0;
                    steerAwayFromPlayer(enemies[i]);
                    int pushDistance = player.getSize() + 18;
                    enemies[i].setX(qBound(0,
                                           enemies[i].getX() + enemies[i].getSpeedX() * pushDistance,
                                           width() - enemies[i].getSize()));
                    enemies[i].setY(qBound(0,
                                           enemies[i].getY() + enemies[i].getSpeedY() * pushDistance,
                                           height() - enemies[i].getSize()));
                    playerPulseFrames = 14;
                } else { //不然就真失败
                    gameOver = true;
                }
            }
            break;
        }
    }

    //当游戏失败
    if (gameOver) {
        gameTimer->stop();
        gameStarted = false;
        showGGDialog();
        return;
    }

    //重新绘制窗口
    update();
}

void Square::showPauseDialog()
{   //显示暂停菜单
    QDialog pauseDialog(this);
    pauseDialog.setWindowTitle("Paused");
    pauseDialog.setFixedSize(430, 280);
    pauseDialog.setModal(true);
    pauseDialog.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    pauseDialog.setStyleSheet(dialogStyleSheet());

    QVBoxLayout *layout = new QVBoxLayout(&pauseDialog);
    layout->setSpacing(20);
    layout->setContentsMargins(34, 30, 34, 30);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("Paused", &pauseDialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel *infoLabel = new QLabel(
        //显示信息
        QString("Difficulty: %1\nSize: %2\nEnemies: %3\nScore: %4")
            .arg(difficultyName())
            .arg(player.getSize())
            .arg(enemies.size())
            .arg(score),
        &pauseDialog);
    infoLabel->setObjectName("info");
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);

    //按钮布局
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(12);

    //继续游戏按钮
    QPushButton *resumeBtn = new QPushButton("Resume", &pauseDialog);
    resumeBtn->setProperty("resultButton", true);
    connect(resumeBtn, &QPushButton::clicked, [&]() {
        pauseDialog.accept();
        if (gameStarted) {
            gameTimer->start(16);
            setFocus();
        }
    });

    //游戏重开按钮
    QPushButton *restartBtn = new QPushButton("Restart", &pauseDialog);
    restartBtn->setProperty("resultButton", true);
    connect(restartBtn, &QPushButton::clicked, [&]() {
        pauseDialog.accept();
        restartGame();
    });

    //退出按钮
    QPushButton *quitBtn = new QPushButton("Quit", &pauseDialog);
    quitBtn->setObjectName("danger");
    quitBtn->setProperty("resultButton", true);
    connect(quitBtn, &QPushButton::clicked, [&]() {
        pauseDialog.accept();
        close();
    });

    buttonLayout->addWidget(restartBtn);
    buttonLayout->addWidget(resumeBtn);
    buttonLayout->addWidget(quitBtn);
    layout->addLayout(buttonLayout);

    centerDialog(pauseDialog);
    pauseDialog.exec();
}

void Square::showGGDialog()
{   //显示失败窗口
    QDialog gameOverDialog(this);
    gameOverDialog.setWindowTitle("Game Over - GG");
    gameOverDialog.setFixedSize(420, 300);
    gameOverDialog.setModal(true);
    gameOverDialog.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    gameOverDialog.setStyleSheet(dialogStyleSheet().replace("#22c55e", "#ef4444"));

    QVBoxLayout *layout = new QVBoxLayout(&gameOverDialog);
    layout->setSpacing(20);
    layout->setContentsMargins(34, 30, 34, 30);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("GG", &gameOverDialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel *infoLabel = new QLabel(
        QString("You were eaten by a bigger square.\nCurrent Size: %1\nDifficulty: %2\nScore: %3")
            .arg(player.getSize())
            .arg(difficultyName())
            .arg(score),
        &gameOverDialog);
    infoLabel->setObjectName("info");
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(12);

    QPushButton *restartBtn = new QPushButton("Restart", &gameOverDialog);
    restartBtn->setProperty("resultButton", true);
    connect(restartBtn, &QPushButton::clicked, [&]() {
        gameOverDialog.accept();
        restartGame();
    });

    QPushButton *menuBtn = new QPushButton("Menu", &gameOverDialog);
    menuBtn->setProperty("resultButton", true);
    connect(menuBtn, &QPushButton::clicked, [&]() {
        gameOverDialog.accept();
        showMainMenu();
    });

    QPushButton *quitBtn = new QPushButton("Quit", &gameOverDialog);
    quitBtn->setObjectName("danger");
    quitBtn->setProperty("resultButton", true);
    connect(quitBtn, &QPushButton::clicked, this, &Square::close);

    buttonLayout->addWidget(restartBtn);
    buttonLayout->addWidget(menuBtn);
    buttonLayout->addWidget(quitBtn);
    layout->addLayout(buttonLayout);

    centerDialog(gameOverDialog);
    gameOverDialog.exec();
}

void Square::showVictoryDialog()
{   //显示游戏胜利窗口
    QDialog gameOverDialog(this);
    gameOverDialog.setWindowTitle("Game Victory");
    gameOverDialog.setFixedSize(440, 300);
    gameOverDialog.setModal(true);
    gameOverDialog.setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    gameOverDialog.setStyleSheet(dialogStyleSheet());

    QVBoxLayout *layout = new QVBoxLayout(&gameOverDialog);
    layout->setSpacing(20);
    layout->setContentsMargins(34, 30, 34, 30);
    layout->setAlignment(Qt::AlignCenter);

    QLabel *titleLabel = new QLabel("Victory", &gameOverDialog);
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel *infoLabel = new QLabel(
        QString("You ate all the enemies.\nCurrent Size: %1\nDifficulty: %2\nScore: %3")
            .arg(player.getSize())
            .arg(difficultyName())
            .arg(score),
        &gameOverDialog);
    infoLabel->setObjectName("info");
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(12);

    QPushButton *restartBtn = new QPushButton("Restart", &gameOverDialog);
    restartBtn->setProperty("resultButton", true);
    connect(restartBtn, &QPushButton::clicked, [&]() {
        gameOverDialog.accept();
        restartGame();
    });

    QPushButton *menuBtn = new QPushButton("Menu", &gameOverDialog);
    menuBtn->setProperty("resultButton", true);
    connect(menuBtn, &QPushButton::clicked, [&]() {
        gameOverDialog.accept();
        showMainMenu();
    });

    QPushButton *quitBtn = new QPushButton("Quit", &gameOverDialog);
    quitBtn->setObjectName("danger");
    quitBtn->setProperty("resultButton", true);
    connect(quitBtn, &QPushButton::clicked, this, &Square::close);

    buttonLayout->addWidget(restartBtn);
    buttonLayout->addWidget(menuBtn);
    buttonLayout->addWidget(quitBtn);
    layout->addLayout(buttonLayout);

    centerDialog(gameOverDialog);
    gameOverDialog.exec();
}

void Square::restartGame()
{
    startGame();
}
