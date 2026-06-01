#include "rightcontrolpanel.h"
#include "gamemanager.h"
#include "unit.h"

#include <QFont>
#include <QStyle>

RightControlPanel::RightControlPanel(GameManager* gm, QWidget* parent)
    : QWidget(parent)
    , m_gameMgr(gm)
{
    setupUI();
    applyStyles();
}

void RightControlPanel::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    QFont labelFont("Microsoft YaHei", 9);
    QFont boldFont("Microsoft YaHei", 9, QFont::Bold);

    // ==================== 模块一：数据状态区 ====================
    QFrame* dataSeparator1 = new QFrame(this);
    dataSeparator1->setFrameShape(QFrame::HLine);
    dataSeparator1->setFrameShadow(QFrame::Sunken);

    m_roundLabel = new QLabel(this);
    m_roundLabel->setFont(boldFont);

    m_stateLabel = new QLabel(this);
    m_stateLabel->setFont(boldFont);
    m_stateLabel->setWordWrap(true);

    m_hpLabel = new QLabel(this);
    m_hpLabel->setFont(boldFont);

    m_goldLabel = new QLabel(this);
    m_goldLabel->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));

    m_populationLabel = new QLabel(this);
    m_populationLabel->setFont(labelFont);

    m_expLabel = new QLabel(this);
    m_expLabel->setFont(QFont("Microsoft YaHei", 7));

    m_expBar = new QProgressBar(this);
    m_expBar->setMinimum(0);
    m_expBar->setMaximum(100);
    m_expBar->setTextVisible(false);
    m_expBar->setFixedHeight(8);

    m_streakLabel = new QLabel(this);
    m_streakLabel->setFont(labelFont);
    m_streakLabel->setWordWrap(true);

    QFrame* dataSeparator2 = new QFrame(this);
    dataSeparator2->setFrameShape(QFrame::HLine);
    dataSeparator2->setFrameShadow(QFrame::Sunken);

    mainLayout->addWidget(dataSeparator1);
    mainLayout->addWidget(m_roundLabel);
    mainLayout->addWidget(m_stateLabel);
    mainLayout->addWidget(m_hpLabel);
    mainLayout->addWidget(m_goldLabel);
    mainLayout->addWidget(m_populationLabel);
    mainLayout->addWidget(m_expLabel);
    mainLayout->addWidget(m_expBar);
    mainLayout->addWidget(m_streakLabel);
    mainLayout->addWidget(dataSeparator2);

    // ==================== 模块二：游戏操作区 ====================
    m_refreshBtn = new QPushButton(QStringLiteral("刷新商店 (2G)"), this);
    m_refreshBtn->setFont(boldFont);
    m_refreshBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_refreshBtn->setMinimumHeight(38);
    m_refreshBtn->setCursor(Qt::PointingHandCursor);

    m_buyXPBtn = new QPushButton(QStringLiteral("购买经验 (4G)"), this);
    m_buyXPBtn->setFont(boldFont);
    m_buyXPBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_buyXPBtn->setMinimumHeight(38);
    m_buyXPBtn->setCursor(Qt::PointingHandCursor);

    mainLayout->addWidget(m_refreshBtn);
    mainLayout->addWidget(m_buyXPBtn);

    // ==================== 模块三：暂停按钮（操作区下方，上移至此） ====================
    m_pauseBtn = new QPushButton(QStringLiteral("暂停游戏"), this);
    m_pauseBtn->setFont(boldFont);
    m_pauseBtn->setMinimumHeight(34);
    m_pauseBtn->setCursor(Qt::PointingHandCursor);
    mainLayout->addWidget(m_pauseBtn);

    // ==================== 模块四：弹性留白 ====================
    mainLayout->addSpacerItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));

    // ==================== 聚焦单位详情面板 ====================
    m_focusedUnitSeparator = new QFrame(this);
    m_focusedUnitSeparator->setFrameShape(QFrame::HLine);
    m_focusedUnitSeparator->setFrameShadow(QFrame::Sunken);
    m_focusedUnitSeparator->setVisible(false);
    mainLayout->addWidget(m_focusedUnitSeparator);

    m_focusedUnitTitle = new QLabel(this);
    m_focusedUnitTitle->setFont(QFont("Microsoft YaHei", 10, QFont::Bold));
    m_focusedUnitTitle->setVisible(false);
    mainLayout->addWidget(m_focusedUnitTitle);

    m_focusedUnitDetail = new QLabel(this);
    m_focusedUnitDetail->setFont(QFont("Microsoft YaHei", 8));
    m_focusedUnitDetail->setWordWrap(true);
    m_focusedUnitDetail->setVisible(false);
    mainLayout->addWidget(m_focusedUnitDetail);

    setLayout(mainLayout);

    connect(m_refreshBtn, &QPushButton::clicked, this, &RightControlPanel::refreshShopRequested);
    connect(m_buyXPBtn, &QPushButton::clicked, this, &RightControlPanel::buyXPRequested);
    connect(m_pauseBtn, &QPushButton::clicked, this, &RightControlPanel::pauseRequested);
}

void RightControlPanel::applyStyles()
{
    setStyleSheet(QStringLiteral(
        "RightControlPanel {"
        "   background-color: #F5F5F0;"
        "   border-left: 1px solid #C8C8C8;"
        "}"
        "QLabel {"
        "   color: #333333;"
        "}"
        "QProgressBar {"
        "   border: none;"
        "   border-radius: 3px;"
        "   background-color: #DCDCDC;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #9370DB;"
        "   border-radius: 3px;"
        "}"
    ));

    m_goldLabel->setStyleSheet("color: #D2A00A; font-size: 12pt;");
    m_hpLabel->setStyleSheet("color: #DC2828;");
    m_stateLabel->setStyleSheet("color: #643296;");

    m_refreshBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "   background-color: #FFD700;"
        "   color: #333333;"
        "   border: 1px solid #C89600;"
        "   border-radius: 6px;"
        "   padding: 6px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #FFE44D;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #DAA520;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #C8C8C8;"
        "   color: #888888;"
        "   border-color: #A0A0A0;"
        "}"
    ));

    m_buyXPBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "   background-color: #9370DB;"
        "   color: #FFFFFF;"
        "   border: 1px solid #643296;"
        "   border-radius: 6px;"
        "   padding: 6px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #A885E8;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #7B5CB8;"
        "}"
        "QPushButton:disabled {"
        "   background-color: #C8C8C8;"
        "   color: #888888;"
        "   border-color: #A0A0A0;"
        "}"
    ));

    m_pauseBtn->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "   background-color: #5A6B7D;"
        "   color: #FFFFFF;"
        "   border: 1px solid #3D4F60;"
        "   border-radius: 5px;"
        "   padding: 4px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #6E8098;"
        "}"
        "QPushButton:pressed {"
        "   background-color: #485764;"
        "}"
    ));


}

void RightControlPanel::Refresh()
{
    GameManager* gm = m_gameMgr;
    if (!gm) return;

    m_roundLabel->setText(QStringLiteral("当前关卡: 第 ")
                          + QString::number(gm->getCurrentRound())
                          + QStringLiteral(" 轮"));

    GameState state = gm->getState();
    QString stateStr;
    if (state == GameState::Preparation) {
        stateStr = QStringLiteral("【准备阶段】\n操作：拖拽排兵布阵\n快捷键：[空格]开战");
    } else if (state == GameState::Battle) {
        stateStr = QStringLiteral("【战斗进行中...】");
    } else {
        stateStr = QStringLiteral("【回合结算中】");
    }
    m_stateLabel->setText(stateStr);

    m_hpLabel->setText(QStringLiteral("玩家血量: ")
                       + QString::number(gm->playerHp)
                       + QStringLiteral(" / 100"));

    m_goldLabel->setText(QStringLiteral("拥有金币: ")
                         + QString::number(gm->getPlayerGold())
                         + QStringLiteral(" G"));

    m_populationLabel->setText(QStringLiteral("当前人口: ")
                               + QString::number(gm->getpoplulation())
                               + QStringLiteral(" 级"));

    int expVal = gm->getPlayerExp();
    int expMax = gm->getPlayerExpToNextLevel();
    m_expLabel->setText("EXP: " + QString::number(expVal) + "/" + QString::number(expMax));
    m_expBar->setMaximum(expMax > 0 ? expMax : 100);
    m_expBar->setValue(expVal);

    QString streakText;
    if (gm->getInterestGold() > 0) {
        streakText += QStringLiteral("利息: +")
                      + QString::number(gm->getInterestGold())
                      + QStringLiteral(" G");
    }
    if (gm->getWinStreak() > 0) {
        if (!streakText.isEmpty()) streakText += QStringLiteral("  ");
        streakText += QStringLiteral("连胜: ")
                      + QString::number(gm->getWinStreak())
                      + QStringLiteral(" 场");
    }
    if (gm->getLoseStreak() > 0) {
        if (!streakText.isEmpty()) streakText += QStringLiteral("  ");
        streakText += QStringLiteral("连败: ")
                      + QString::number(gm->getLoseStreak())
                      + QStringLiteral(" 场");
    }
    m_streakLabel->setText(streakText);

    bool inPrep = (state == GameState::Preparation);
    m_refreshBtn->setEnabled(inPrep);
    m_buyXPBtn->setEnabled(inPrep);
}

void RightControlPanel::SetFocusedUnit(Unit* unit, const QString& skillDesc)
{
    bool hasUnit = (unit != nullptr);

    m_focusedUnitSeparator->setVisible(hasUnit);
    m_focusedUnitTitle->setVisible(hasUnit);
    m_focusedUnitDetail->setVisible(hasUnit);

    if (!hasUnit) return;

    m_focusedUnitTitle->setText(QStringLiteral("【 ")
                                + QString::fromStdString(unit->name)
                                + QStringLiteral(" 的面板 】"));

    QString starStr;
    for (int k = 0; k < unit->star; k++) starStr += QStringLiteral("*");
    QString slotStr = QStringLiteral("星级: ") + starStr
                      + QStringLiteral("  槽位: ")
                      + QString::number((int)unit->equippedItems.size())
                      + QStringLiteral("/") + QString::number(unit->getMaxItemSlots());

    int bonusAtkSum = 0;
    int bonusHpSum = 0;
    for (auto* item : unit->equippedItems) {
        bonusAtkSum += item->bonusAtk;
        bonusHpSum += item->bonusHp;
    }

    QString hpStr = QStringLiteral("生命: ")
                    + QString::number(unit->hp) + QStringLiteral("/") + QString::number(unit->maxHp);
    if (bonusHpSum > 0) hpStr += QStringLiteral(" (+") + QString::number(bonusHpSum) + QStringLiteral(")");

    QString atkStr = QStringLiteral("攻击: ") + QString::number(unit->atk);
    if (bonusAtkSum > 0) atkStr += QStringLiteral(" (+") + QString::number(bonusAtkSum) + QStringLiteral(")");

    QString rangeStr = QStringLiteral("射程: ") + QString::number(unit->range);

    QString traitStr = QStringLiteral("羁绊: ");
    if (!unit->traits.empty())
        traitStr += QString::fromStdString(unit->traits[0]);
    else
        traitStr += QStringLiteral("无");

    QString gearStr = QStringLiteral("已装: ");
    if (unit->equippedItems.empty()) {
        gearStr += QStringLiteral("暂无装备");
    } else {
        for (auto* item : unit->equippedItems)
            gearStr += QStringLiteral("[") + QString::fromStdString(item->name) + QStringLiteral("] ");
    }

    QString detail = QString("<p style='color:#FF8C00;'>%1</p>"
                              "<p>%2</p>"
                              "<p>%3</p>"
                              "<p>%4</p>"
                              "<p>%5</p>"
                              "<p style='color:#1E64B4; font-weight:bold;'>%6</p>"
                              "<p style='color:#8B4513;'>%7</p>")
                         .arg(slotStr, hpStr, atkStr, rangeStr, traitStr, skillDesc, gearStr);

    m_focusedUnitDetail->setText(detail);
}
