#ifndef SQUARE_H
#define SQUARE_H

#include <QWidget>
#include <QTimer>
#include <QSet>
#include <QList>
#include <QString>
#include <QColor>
#include <QPointF>
#include "GameSquare.h"

class QDialog;

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
    enum class Difficulty {
        Easy,
        Medium,
        Tough,
        HellNo
    };

    enum class EnemyType {
        // 普通方块：沿当前方向直线移动，撞到边界或其他敌人后反弹。
        Normal,
        // 追踪方块：移动速度较慢，会持续选择更接近玩家的方向。
        Hunter,
        // 闪烁方块：以自身基础大小为中心，周期性变大和变小。
        Blinker,
        // 分裂方块：被玩家吃掉后，会尝试生成两个更小的普通方块。
        Splitter,
        // 恐怖方块：看起来比玩家小，但吃掉后合并结果会额外缩小 20%。
        Horror,
        // 神速方块：平时直线移动；减速场生效时不被减速，并转为追踪玩家。
        Speedster
    };

    enum class PowerUpType {
        Shield,
        SlowField,
        Magnet,
        ShrinkBomb
    };

    struct DifficultySettings {
        QString name;             //难度名称
        int enemyCount;           //敌人总数
        int enemyBaseSize;        // 后期敌人的最大基础尺寸
        int enemySizeStep;       //敌人尺寸增长步进，目前影响较弱
        int enemySpeed;         //敌人速度
        int playerSpeed;        //玩家速度
        int scoreMultiplier;    //得分倍率
        int starterEnemyCount;  //开局较小敌人的数量
        int maxPowerUps;        //当前难度最多掉落几个道具
    };

    struct EatEffect {
        QPointF center;
        QColor color;
        int age;
        int maxAge;
        double startRadius;
    };

    struct PowerUp {
        QPointF center;
        PowerUpType type;
        int radius;
        int speedX;
        int speedY;
    };

    void initEnemies();
    void setRandomDirection(GameSquare& enemy);
    bool checkCollision(const GameSquare& enemy, int newX, int newY);
    bool checkEnemyCollisions(int index, int newX, int newY);
    bool isNearPlayer(const GameSquare& enemy, int margin) const;
    void steerAwayFromPlayer(GameSquare& enemy);
    void steerHunterTowardPlayer(int index, int step);
    EnemyType randomEnemyType(int index) const;
    QColor enemyColorForType(EnemyType type) const;
    void appendEnemy(const GameSquare& enemy, EnemyType type, int baseSize, int phaseOffset);
    void updateBlinkerEnemies();
    void spawnSplitEnemies(const GameSquare& enemy, EnemyType sourceType);
    int mergedSizeAfterEating(const GameSquare& enemy, EnemyType type) const;
    void removeEnemyAt(int index);
    void spawnPowerUp(const QPointF& center);
    void setRandomPowerUpDirection(PowerUp& powerUp);
    void movePowerUps(int speed);
    void checkPowerUpCollisions();
    void activatePowerUp(PowerUpType type);
    void showPowerUpDialog(PowerUpType type);
    QString powerUpName(PowerUpType type) const;
    QString powerUpDescription(PowerUpType type) const;
    void applyMagnetEffect();
    void applyShrinkBomb();
    void restoreShrinkBombEnemies();
    void updateVisualEffects();
    void resetGameState();
    void startGame();
    void showMainMenu();
    void showDifficultyDialog();
    void showHelpDialog();
    void showPauseDialog();
    void centerDialog(QDialog& dialog) const;
    void setDifficulty(Difficulty newDifficulty);
    DifficultySettings currentSettings() const;
    QString difficultyName() const;
    QString dialogStyleSheet() const;

    QTimer *gameTimer;
    QSet<int> pressedKeys;
    Difficulty difficulty;
    bool gameStarted;

    GameSquare player;
    QList<GameSquare> enemies;
    QList<EnemyType> enemyTypes;
    QList<int> enemyBaseSizes;
    QList<int> enemyPhaseOffsets;
    QList<int> shrinkRestoreSizes;
    QList<EatEffect> eatEffects;
    QList<PowerUp> powerUps;
    int playerPulseFrames;
    int score;
    int dashFrames;
    int dashCooldownFrames;
    int shieldFrames;
    int slowFieldFrames;
    int magnetFrames;
    int shrinkBombFrames;
    int powerUpsDropped;
    int gameFrame;
};

#endif // SQUARE_H
