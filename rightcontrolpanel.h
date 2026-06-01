#ifndef RIGHTCONTROLPANEL_H
#define RIGHTCONTROLPANEL_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>

class GameManager;
class Unit;

class RightControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit RightControlPanel(GameManager* gm, QWidget* parent = nullptr);
    void Refresh();
    void SetFocusedUnit(Unit* unit, const QString& skillDesc);

signals:
    void pauseRequested();
    void refreshShopRequested();
    void buyXPRequested();

private:
    void setupUI();
    void applyStyles();

    GameManager* m_gameMgr;

    // Module 1: Data area
    QLabel* m_roundLabel;
    QLabel* m_stateLabel;
    QLabel* m_hpLabel;
    QLabel* m_goldLabel;
    QLabel* m_populationLabel;
    QLabel* m_expLabel;
    QProgressBar* m_expBar;
    QLabel* m_streakLabel;

    // Module 2: Operations
    QPushButton* m_refreshBtn;
    QPushButton* m_buyXPBtn;

    // Module 3: Spacer (in layout, no member)

    // Module 4: System control
    QPushButton* m_pauseBtn;

    // Focused unit detail
    QLabel* m_focusedUnitTitle;
    QLabel* m_focusedUnitDetail;
    QFrame* m_focusedUnitSeparator;
};

#endif // RIGHTCONTROLPANEL_H
