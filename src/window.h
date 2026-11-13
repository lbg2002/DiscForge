#pragma once
#include "core.h"
#include <QtWidgets>
#include <QtConcurrent>

class CapacityGauge : public QWidget {
public:
    explicit CapacityGauge(QWidget* p = nullptr) : QWidget(p) {
        setFixedSize(114, 114);
        setAccessibleName("容量仪表");
    }
    qint64 total = 0, used = 0, planned = 0;
    QColor accentColor{"#5eb8ff"};
protected:
    void paintEvent(QPaintEvent*) override;
};

class CapacityChart : public QWidget {
public:
    explicit CapacityChart(QWidget* p = nullptr) : QWidget(p) {
        setMinimumHeight(96);
        setAccessibleName("光盘容量区段图");
    }
    qint64 total = 0, used = 0, planned = 0;
    QString unit = "auto";
    QColor accentColor{"#5eb8ff"};
protected:
    void paintEvent(QPaintEvent*) override;
};

class Window : public QMainWindow {
    Q_OBJECT
public:
    Window();
    bool selfTest(const QString& renderPath = {});
protected:
    void closeEvent(QCloseEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    QSettings prefs;
    QTemporaryDir uiAssetsDir;
    QStackedWidget* stack = nullptr;
    QButtonGroup* navigation = nullptr;
    QComboBox *drive = nullptr, *speed = nullptr, *unit = nullptr, *theme = nullptr, *density = nullptr;
    QLineEdit* discLabel = nullptr;
    QTableWidget *fileTable = nullptr, *sessionTable = nullptr, *historyTable = nullptr;
    QLabel *typeLabel = nullptr, *mediaState = nullptr, *capacityLabel = nullptr, *sessionLabel = nullptr;
    QLabel *filesSummary = nullptr, *plannedLabel = nullptr, *readyLabel = nullptr, *nextLabel = nullptr, *statusLabel = nullptr;
    QLabel *topStatusPill = nullptr, *driveSubLabel = nullptr, *profileBadge = nullptr, *freeSubLabel = nullptr;
    QLabel *engineModeLabel = nullptr, *engineVerifyLabel = nullptr;
    QLabel *labelByteCount = nullptr, *modeHintLabel = nullptr, *actionHintLabel = nullptr;
    QLabel *sideDriveLabel = nullptr, *sideMediaLabel = nullptr, *sideFreeLabel = nullptr;
    QLabel *sessDeviceVal = nullptr, *sessDeviceSub = nullptr, *sessStateVal = nullptr, *sessStateSub = nullptr;
    QLabel *sessCountVal = nullptr, *sessCountSub = nullptr, *sessFreeVal = nullptr, *sessFreeSub = nullptr;
    QLabel *historyStatsLabel = nullptr;
    QWidget *emptyFileOverlay = nullptr, *emptyHistoryOverlay = nullptr;
    QPlainTextEdit* historyDetailView = nullptr;
    QPushButton *burnButton = nullptr, *refreshButton = nullptr, *ejectDiscButton = nullptr;
    QPushButton *addButton = nullptr, *folderButton = nullptr, *removeSelectedButton = nullptr, *clearButton = nullptr;
    QPushButton *appendButton = nullptr, *finalButton = nullptr;
    QPushButton *refreshSessionButton = nullptr, *clearHistoryButton = nullptr;
    QCheckBox *verify = nullptr, *eject = nullptr, *defaultVerify = nullptr, *defaultEject = nullptr;
    QPlainTextEdit* output = nullptr;
    QDockWidget* outputDock = nullptr;
    CapacityGauge* gauge = nullptr;
    CapacityChart *chart = nullptr, *sessionChart = nullptr;
    QProgressBar* taskProgress = nullptr;
    QLabel *phaseLabel = nullptr, *elapsedLabel = nullptr;
    QWidget* taskPanel = nullptr;
    QElapsedTimer taskClock;
    QTimer* taskTimer = nullptr;
    QList<df::Entry> entries;
    df::Disc disc;
    bool busy = false, finalize = false, burning = false;
    QJsonArray history;

    QWidget* burnPage();
    QWidget* settingsPage();
    QWidget* listPage(bool sessions);
    void updateView();
    void scan();
    void ejectCurrentDisc();
    void removeSelectedEntries();
    void importPaths(const QStringList&);
    void startBurn();
    void setBusy(bool);
    void appendLog(const QString&);
    void updateProgress(const df::Progress&);
    void loadHistory();
    void saveHistory(const df::BurnResult&, const df::Options&);
    void refreshHistory();
    void showHistoryDetail(int row);
    void applyAppearance();
    QColor currentAccent() const;
    QString unitSetting() const {
        return prefs.value("units", "auto").toString();
    }
    QString formatted(qint64 n) const {
        return df::formatBytes(n, unitSetting());
    }
    qint64 plannedBytes() const;
    void selectPage(int);
};
