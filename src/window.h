#pragma once
#include "core.h"
#include <QtWidgets>
#include <QtConcurrent>
class CapacityGauge:public QWidget {
    public: explicit CapacityGauge(QWidget* p=nullptr):QWidget(p) {
        setFixedSize(106,106);
        setAccessibleName("容量仪表");
    }
    qint64 total=0,used=0,planned=0;
    protected:void paintEvent(QPaintEvent*) override;
};
class CapacityChart:public QWidget {
    public:explicit CapacityChart(QWidget* p=nullptr):QWidget(p) {
        setMinimumHeight(88);
        setAccessibleName("光盘容量区段图");
    }
    qint64 total=0,used=0,planned=0;
    QString unit="auto";
    protected:void paintEvent(QPaintEvent*) override;
};
class Window:public QMainWindow {
    Q_OBJECT
    public:Window();
    bool selfTest(const QString& renderPath= {
    });
    protected:void closeEvent(QCloseEvent*) override;
    void dragEnterEvent(QDragEnterEvent*) override;
    void dropEvent(QDropEvent*) override;
    private:
    QSettings prefs;
    QStackedWidget* stack;
    QButtonGroup* navigation;
    QComboBox *drive,*speed,*unit,*theme,*density;
    QLineEdit* discLabel;
    QTableWidget *fileTable,*sessionTable,*historyTable;
    QLabel *typeLabel,*mediaState,*capacityLabel,*sessionLabel,*filesSummary,*plannedLabel,*readyLabel,*nextLabel,*statusLabel;
    QPushButton *burnButton,*refreshButton,*addButton,*folderButton,*clearButton,*appendButton,*finalButton;
    QCheckBox *verify,*eject,*defaultVerify,*defaultEject;
    QPlainTextEdit* output;
    QDockWidget* outputDock;
    CapacityGauge* gauge;
    CapacityChart* chart;
    QProgressBar* taskProgress;
    QLabel *phaseLabel, *elapsedLabel;
    QWidget* taskPanel;
    QElapsedTimer taskClock;
    QTimer* taskTimer;
    QList<df::Entry> entries;
    df::Disc disc;
    bool busy=false,finalize=false,burning=false;
    QJsonArray history;
    QWidget* burnPage();
    QWidget* settingsPage();
    QWidget* listPage(bool sessions);
    void updateView();
    void scan();
    void importPaths(const QStringList&);
    void startBurn();
    void setBusy(bool);
    void appendLog(const QString&);
    void updateProgress(const df::Progress&);
    void loadHistory();
    void saveHistory(const df::BurnResult&,const df::Options&);
    void refreshHistory();
    void applyAppearance();
    QString unitSetting()const {
        return prefs.value("units","auto").toString();
    }QString formatted(qint64 n)const {
        return df::formatBytes(n,unitSetting());
    }qint64 plannedBytes()const;
    void selectPage(int);
};
