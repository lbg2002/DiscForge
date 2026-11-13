#include "window.h"
#include "version.h"
#include <cmath>

namespace {

const QColor kMuted("#8ea7bf");
const QColor kUsedBar("#4f7899");
const QColor kWarnColor("#f29d6b");

class BrandEmblem : public QWidget {
public:
    explicit BrandEmblem(QWidget* p = nullptr) : QWidget(p) {
        setFixedSize(30, 30);
    }
    QColor accent{"#5eb8ff"};
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QPointF c(width() / 2.0, height() / 2.0);
        QRadialGradient bg(c, 14.0);
        bg.setColorAt(0.0, QColor(accent.red(), accent.green(), accent.blue(), 55));
        bg.setColorAt(1.0, QColor(18, 32, 48, 220));
        p.setPen(QPen(accent, 1.8));
        p.setBrush(bg);
        p.drawEllipse(c, 12.5, 12.5);

        p.setPen(QPen(QColor("#8ce8df"), 1.2, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, 8.2, 8.2);

        p.setPen(QPen(accent.lighter(125), 2.0));
        p.drawArc(QRectF(c.x() - 10.0, c.y() - 10.0, 20.0, 20.0), 35 * 16, 65 * 16);
        p.drawArc(QRectF(c.x() - 10.0, c.y() - 10.0, 20.0, 20.0), 215 * 16, 65 * 16);

        p.setPen(QPen(QColor("#d9f2ff"), 1.6));
        p.setBrush(QColor("#0d1620"));
        p.drawEllipse(c, 3.4, 3.4);
    }
};

QIcon makeNavIcon(int id, const QColor& color) {
    QPixmap px(36, 36);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    QPen pen(color, 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    if (id == 0) {
        // Optical disc icon
        p.drawEllipse(QPointF(18, 18), 12.5, 12.5);
        p.drawEllipse(QPointF(18, 18), 3.8, 3.8);
        p.drawArc(QRectF(9.5, 9.5, 17, 17), 40 * 16, 55 * 16);
        p.drawArc(QRectF(9.5, 9.5, 17, 17), 220 * 16, 55 * 16);
    } else if (id == 1) {
        // Layered sessions icon
        QPolygonF top;
        top << QPointF(18, 6.5) << QPointF(30.5, 12.5) << QPointF(18, 18.5) << QPointF(5.5, 12.5);
        p.drawPolygon(top);
        QPainterPath mid;
        mid.moveTo(5.5, 18.5);
        mid.lineTo(18, 24.5);
        mid.lineTo(30.5, 18.5);
        p.drawPath(mid);
        QPainterPath bot;
        bot.moveTo(5.5, 24.0);
        bot.lineTo(18, 30.0);
        bot.lineTo(30.5, 24.0);
        p.drawPath(bot);
    } else if (id == 2) {
        // History clock icon
        p.drawEllipse(QPointF(18, 18), 12.0, 12.0);
        QPainterPath hands;
        hands.moveTo(18, 11.0);
        hands.lineTo(18, 18.5);
        hands.lineTo(23.5, 21.5);
        p.drawPath(hands);
    } else {
        // Settings sliders icon
        p.drawLine(QPointF(6.5, 10.5), QPointF(29.5, 10.5));
        p.drawLine(QPointF(6.5, 18.0), QPointF(29.5, 18.0));
        p.drawLine(QPointF(6.5, 25.5), QPointF(29.5, 25.5));
        p.setBrush(QColor("#131c26"));
        p.drawEllipse(QPointF(13.5, 10.5), 3.2, 3.2);
        p.drawEllipse(QPointF(23.0, 18.0), 3.2, 3.2);
        p.drawEllipse(QPointF(16.0, 25.5), 3.2, 3.2);
    }
    px.setDevicePixelRatio(2.0);
    return QIcon(px);
}

QIcon makeEntryIcon(bool isFolder, const QColor& accent) {
    QPixmap px(36, 36);
    px.fill(Qt::transparent);
    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    if (isFolder) {
        QColor fill = accent.lighter(110);
        fill.setAlpha(45);
        p.setPen(QPen(accent.lighter(115), 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(fill);
        QPainterPath folder;
        folder.moveTo(5, 11);
        folder.lineTo(14, 11);
        folder.lineTo(17, 14.5);
        folder.lineTo(31, 14.5);
        folder.lineTo(31, 28.5);
        folder.lineTo(5, 28.5);
        folder.closeSubpath();
        p.drawPath(folder);
    } else {
        QColor stroke("#8ec8f6");
        QColor fill("#8ec8f6");
        fill.setAlpha(32);
        p.setPen(QPen(stroke, 2.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(fill);
        QPainterPath doc;
        doc.moveTo(8, 6);
        doc.lineTo(22, 6);
        doc.lineTo(28, 12);
        doc.lineTo(28, 30);
        doc.lineTo(8, 30);
        doc.closeSubpath();
        p.drawPath(doc);
        p.drawLine(QPointF(21.5, 6.5), QPointF(21.5, 12.5));
        p.drawLine(QPointF(21.5, 12.5), QPointF(27.5, 12.5));
        p.drawLine(QPointF(12.5, 18.5), QPointF(23.5, 18.5));
        p.drawLine(QPointF(12.5, 23.5), QPointF(20.5, 23.5));
    }
    px.setDevicePixelRatio(2.0);
    return QIcon(px);
}

void writeUiIndicatorImages(const QString& dirPath) {
    {
        QImage check(32, 32, QImage::Format_ARGB32_Premultiplied);
        check.fill(Qt::transparent);
        QPainter p(&check);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor("#081523"), 4.2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath path;
        path.moveTo(7.5, 16.5);
        path.lineTo(13.8, 22.8);
        path.lineTo(24.8, 10.2);
        p.drawPath(path);
        check.save(dirPath + "/check.png");
    }
    {
        QImage chevron(28, 28, QImage::Format_ARGB32_Premultiplied);
        chevron.fill(Qt::transparent);
        QPainter p(&chevron);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor("#96b2cc"), 3.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QPainterPath path;
        path.moveTo(7.5, 11.0);
        path.lineTo(14.0, 17.5);
        path.lineTo(20.5, 11.0);
        p.drawPath(path);
        chevron.save(dirPath + "/chevron.png");
    }
}

bool isDirectoryEntry(const df::Entry& e) {
    QFileInfo fi(e.path);
    if (fi.exists()) return fi.isDir();
    return !e.name.contains('.');
}

} // namespace

void CapacityGauge::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPointF center(width() / 2.0, height() / 2.0);
    p.translate(center);

    // Subtle radial instrument glow
    QRadialGradient glow(QPointF(0, 0), 52.0);
    glow.setColorAt(0.0, QColor(accentColor.red(), accentColor.green(), accentColor.blue(), 26));
    glow.setColorAt(0.75, QColor(20, 36, 52, 40));
    glow.setColorAt(1.0, Qt::transparent);
    p.setPen(Qt::NoPen);
    p.setBrush(glow);
    p.drawEllipse(QPointF(0, 0), 52.0, 52.0);

    // Outer precision scale ticks
    for (int i = 0; i < 48; ++i) {
        p.save();
        p.rotate(i * 7.5);
        const bool major = (i % 4 == 0);
        p.setPen(QPen(major ? QColor("#5881a3") : QColor("#365269"), major ? 1.4 : 1.0));
        p.drawLine(QPointF(0, -50), QPointF(0, major ? -45.0 : -47.0));
        p.restore();
    }

    auto arc = [&](const QColor& c, int start, int span, double penWidth = 6.2) {
        QPen pen(c, penWidth, Qt::SolidLine, Qt::FlatCap);
        p.setPen(pen);
        p.drawArc(QRectF(-38, -38, 76, 76), start, span);
    };

    arc(QColor("#223649"), 0, 360 * 16);

    const bool over = total > 0 && (used + planned > total);
    if (total > 0) {
        const double a = qBound(0.0, double(used) / total, 1.0);
        const double b = qBound(0.0, double(planned) / total, 1.0 - a);
        if (a > 0.001) {
            arc(kUsedBar, 90 * 16, -int(a * 360 * 16));
        }
        if (b > 0.001) {
            arc(over ? kWarnColor : accentColor, int((90 - a * 360) * 16), -int(b * 360 * 16));
        }
    }

    p.setPen(QPen(QColor("#395873"), 1.0, Qt::DashLine));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QRectF(-29, -29, 58, 58));

    p.setPen(over ? kWarnColor : QColor("#e3f3ff"));
    QFont f = font();
    f.setPointSize(14);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    const QString pctText = total > 0
        ? QString::number(double(used + planned) / total * 100.0, 'f', 0) + "%"
        : "—";
    p.drawText(QRectF(-34, -17, 68, 24), Qt::AlignCenter, pctText);

    f.setPointSize(8);
    f.setWeight(QFont::Normal);
    p.setFont(f);
    p.setPen(kMuted);
    p.drawText(QRectF(-38, 7, 76, 16), Qt::AlignCenter, "写入后占用");
}

void CapacityChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const double left = 18.0, right = width() - 18.0, w = qMax(60.0, right - left);

    QFont f = font();
    f.setPointSize(9);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    p.setPen(QColor("#d9ecfc"));
    p.drawText(QRectF(left, 8, w * 0.45, 18), Qt::AlignLeft | Qt::AlignVCenter, "容量分配预览");

    f.setPointSize(8);
    f.setWeight(QFont::Normal);
    p.setFont(f);
    const qint64 remain = qMax<qint64>(0, total - used - planned);
    const bool over = total > 0 && (used + planned > total);
    p.setPen(over ? kWarnColor : QColor("#9cb6ce"));
    const QString rightSummary = total > 0
        ? QString("总容量 %1  ·  写入后可用 %2").arg(df::formatBytes(total, unit), df::formatBytes(remain, unit))
        : QString("等待识别光盘介质容量");
    p.drawText(QRectF(left + w * 0.35, 8, w * 0.65, 18), Qt::AlignRight | Qt::AlignVCenter, rightSummary);

    // Track bar
    const QRectF barRect(left, 33, w, 14);
    QPainterPath trackPath;
    trackPath.addRoundedRect(barRect, 5, 5);
    p.fillPath(trackPath, QColor("#1f3142"));

    const double a = total > 0 ? qBound(0.0, double(used) / total, 1.0) : 0.0;
    const double b = total > 0 ? qBound(0.0, double(planned) / total, 1.0 - a) : 0.0;

    p.save();
    p.setClipPath(trackPath);
    if (a > 0.0) {
        p.fillRect(QRectF(left, 33, w * a, 14), kUsedBar);
    }
    if (b > 0.0) {
        p.fillRect(QRectF(left + w * a, 33, w * b, 14), over ? kWarnColor : accentColor);
    }
    if (a > 0.005 && b > 0.005) {
        p.setPen(QPen(QColor("#0f1923"), 1.6));
        p.drawLine(QPointF(left + w * a, 33), QPointF(left + w * a, 47));
    }
    p.restore();

    p.setPen(QPen(QColor("#3d5a75"), 1.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(barRect, 5, 5);

    // Percentage ticks
    for (int i = 0; i <= 4; ++i) {
        const double x = left + w * i / 4.0;
        p.setPen(QPen(QColor("#334c63"), 1.0, Qt::DashLine));
        p.drawLine(QPointF(x, 29), QPointF(x, 51));
        p.setPen(QColor("#7d98b0"));
        const Qt::Alignment align = i == 0 ? Qt::AlignLeft : i == 4 ? Qt::AlignRight : Qt::AlignHCenter;
        const double labelX = i == 0 ? x : i == 4 ? x - 44 : x - 22;
        p.drawText(QRectF(labelX, 52, 44, 15), align, QString::number(i * 25) + "%");
    }

    // Legend row with colored dots
    const double legendY = 72.0;
    auto drawLegendItem = [&](double x, const QColor& dotColor, const QString& text) -> double {
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(QPointF(x + 4.5, legendY + 7.5), 4.0, 4.0);
        p.setPen(QColor("#a5bfd6"));
        const QRectF tr(x + 13.0, legendY, 220, 16);
        QRectF br;
        p.drawText(tr, Qt::AlignLeft | Qt::AlignVCenter, text, &br);
        return x + 13.0 + br.width() + 24.0;
    };

    double lx = left;
    lx = drawLegendItem(lx, kUsedBar, "已有数据 " + df::formatBytes(used, unit));
    lx = drawLegendItem(lx, over ? kWarnColor : accentColor, "本次待写入 " + df::formatBytes(planned, unit));
    drawLegendItem(lx, QColor("#354e66"), "剩余可用 " + df::formatBytes(remain, unit) + "（含 16 MB 区段预留）");
}

static QFrame* panel(const char* name = "panel") {
    auto f = new QFrame;
    f->setObjectName(name);
    return f;
}

static QHBoxLayout* hbox(QWidget* w, int m = 0, int gap = 8) {
    auto l = new QHBoxLayout(w);
    l->setContentsMargins(m, m, m, m);
    l->setSpacing(gap);
    return l;
}

static QVBoxLayout* vbox(QWidget* w, int m = 0, int gap = 8) {
    auto l = new QVBoxLayout(w);
    l->setContentsMargins(m, m, m, m);
    l->setSpacing(gap);
    return l;
}

static QLabel* label(const QString& s, const char* name = nullptr) {
    auto l = new QLabel(s);
    if (name) l->setObjectName(name);
    return l;
}

static QFrame* vline() {
    auto f = new QFrame;
    f->setObjectName("vdivider");
    f->setFixedWidth(1);
    return f;
}

static QFrame* hline() {
    auto f = new QFrame;
    f->setObjectName("hdivider");
    f->setFixedHeight(1);
    return f;
}

static void tableStyle(QTableWidget* t, const QStringList& heads) {
    t->setColumnCount(heads.size());
    t->setHorizontalHeaderLabels(heads);
    t->verticalHeader()->hide();
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setShowGrid(false);
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    t->setAlternatingRowColors(false);
    t->setWordWrap(false);
    t->setFocusPolicy(Qt::StrongFocus);
    t->setIconSize(QSize(18, 18));
    t->setMinimumHeight(175);
}

Window::Window() : prefs("DiscForge", "DiscForge") {
    setWindowTitle("光铸 · 光盘刻录");
    resize(1160, 820);
    setMinimumSize(880, 640);
    setAcceptDrops(true);

    if (uiAssetsDir.isValid()) {
        writeUiIndicatorImages(uiAssetsDir.path());
    }

    auto root = new QWidget;
    auto vertical = vbox(root, 0, 0);
    setCentralWidget(root);

    // Top Application Header
    auto top = new QFrame;
    top->setObjectName("topbar");
    top->setFixedHeight(52);
    auto topLayout = hbox(top, 0, 12);
    topLayout->setContentsMargins(18, 0, 18, 0);
    auto emblem = new BrandEmblem;
    emblem->setObjectName("brandEmblem");
    topLayout->addWidget(emblem, 0, Qt::AlignVCenter);
    topLayout->addWidget(label("光铸", "brand"), 0, Qt::AlignVCenter);
    topLayout->addWidget(label("DiscForge Optical Studio", "brandSub"), 0, Qt::AlignVCenter);
    topLayout->addStretch();
    topStatusPill = label("未检测到光驱", "topPill");
    topStatusPill->setFixedHeight(26);
    topStatusPill->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(topStatusPill, 0, Qt::AlignVCenter);
    const QString verText = QString("v%1").arg(DISCFORGE_VERSION);
    auto* verBadge = label(verText, "versionBadge");
    verBadge->setFixedHeight(26);
    verBadge->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(verBadge, 0, Qt::AlignVCenter);
    vertical->addWidget(top);

    // Active Burn Progress Banner
    taskPanel = new QFrame;
    taskPanel->setObjectName("taskPanel");
    auto taskLayout = vbox(taskPanel, 12, 6);
    auto taskLabels = new QWidget;
    auto taskRow = hbox(taskLabels, 0, 10);
    phaseLabel = label("", "taskPhase");
    elapsedLabel = label("", "subtle");
    taskRow->addWidget(phaseLabel);
    taskRow->addStretch();
    taskRow->addWidget(elapsedLabel);
    taskLayout->addWidget(taskLabels);
    taskProgress = new QProgressBar;
    taskProgress->setTextVisible(false);
    taskProgress->setFixedHeight(8);
    taskLayout->addWidget(taskProgress);
    vertical->addWidget(taskPanel);
    taskPanel->hide();

    taskTimer = new QTimer(this);
    taskTimer->setInterval(1000);
    connect(taskTimer, &QTimer::timeout, this, [this] {
        const qint64 seconds = taskClock.elapsed() / 1000;
        elapsedLabel->setText(
            QString("已用 %1:%2").arg(seconds / 60, 2, 10, QChar('0')).arg(seconds % 60, 2, 10, QChar('0')));
    });

    // Main Shell (Sidebar + Page Stack)
    auto body = new QWidget;
    auto bodyLayout = hbox(body, 0, 0);
    vertical->addWidget(body, 1);

    auto side = new QFrame;
    side->setObjectName("sidebar");
    side->setFixedWidth(174);
    auto navLayout = vbox(side, 12, 6);
    auto navHeader = label("功能工作区", "navSection");
    navHeader->setContentsMargins(6, 2, 6, 4);
    navLayout->addWidget(navHeader);

    navigation = new QButtonGroup(this);
    const QStringList titles = {"数据刻录", "光盘区段", "任务记录", "设置"};
    for (int i = 0; i < 4; ++i) {
        if (i == 3) {
            navLayout->addStretch();
            // Sidebar Quick Media Card
            auto quickCard = panel("sideCard");
            auto ql = vbox(quickCard, 10, 4);
            ql->addWidget(label("当前光驱概览", "sideCardTitle"));
            sideDriveLabel = label("未检测到光驱", "sideCardMain");
            sideMediaLabel = label("无可用介质", "subtle");
            sideFreeLabel = label("剩余 —", "subtle");
            ql->addWidget(sideDriveLabel);
            ql->addWidget(sideMediaLabel);
            ql->addWidget(sideFreeLabel);
            navLayout->addWidget(quickCard);
            navLayout->addSpacing(4);
        }
        auto b = new QPushButton(titles[i]);
        b->setCheckable(true);
        b->setObjectName("nav");
        b->setIconSize(QSize(18, 18));
        b->setCursor(Qt::PointingHandCursor);
        navigation->addButton(b, i);
        navLayout->addWidget(b);
    }
    navigation->button(0)->setChecked(true);
    connect(navigation, &QButtonGroup::idClicked, this, &Window::selectPage);
    bodyLayout->addWidget(side);

    stack = new QStackedWidget;
    bodyLayout->addWidget(stack, 1);
    stack->addWidget(burnPage());
    stack->addWidget(listPage(true));
    stack->addWidget(listPage(false));
    stack->addWidget(settingsPage());

    // Bottom Status Bar
    statusLabel = new QLabel;
    statusBar()->addWidget(statusLabel, 1);
    auto outputButton = new QPushButton("刻录日志");
    outputButton->setObjectName("statusLogBtn");
    outputButton->setCursor(Qt::PointingHandCursor);
    statusBar()->addPermanentWidget(outputButton);

    // Enhanced Log Dock
    outputDock = new QDockWidget("刻录与工具诊断日志", this);
    auto dockContainer = new QWidget;
    auto dockLayout = vbox(dockContainer, 8, 6);
    auto dockTools = new QWidget;
    auto dtl = hbox(dockTools, 0, 8);
    dtl->addWidget(label("实时记录 genisoimage / wodim / growisofs / SG_IO 校验输出", "subtle"));
    dtl->addStretch();
    auto copyLogBtn = new QPushButton("复制日志");
    auto clearLogBtn = new QPushButton("清空日志");
    dtl->addWidget(copyLogBtn);
    dtl->addWidget(clearLogBtn);
    dockLayout->addWidget(dockTools);
    output = new QPlainTextEdit;
    output->setReadOnly(true);
    output->setMaximumBlockCount(5000);
    output->setPlaceholderText("尚无诊断日志输出。执行光盘刷新或开始刻录后，此处将显示详细进度与底层返回信息。");
    dockLayout->addWidget(output, 1);
    outputDock->setWidget(dockContainer);
    addDockWidget(Qt::BottomDockWidgetArea, outputDock);
    outputDock->hide();

    connect(outputButton, &QPushButton::clicked, this, [this] {
        outputDock->setVisible(!outputDock->isVisible());
    });
    connect(copyLogBtn, &QPushButton::clicked, this, [this] {
        QApplication::clipboard()->setText(output->toPlainText());
        statusLabel->setText("已复制刻录日志到剪贴板");
    });
    connect(clearLogBtn, &QPushButton::clicked, this, [this] {
        output->clear();
    });

    loadHistory();
    applyAppearance();
    updateView();
    if (!QCoreApplication::arguments().contains("--self-test")) {
        QTimer::singleShot(0, this, &Window::scan);
    }
}

QWidget* Window::burnPage() {
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto page = new QWidget;
    auto l = vbox(page, 18, 12);
    scroll->setWidget(page);

    // Page Header
    auto heading = new QWidget;
    auto head = hbox(heading, 0, 10);
    auto titleWrap = new QWidget;
    auto titleCol = vbox(titleWrap, 0, 2);
    titleCol->addWidget(label("数据刻录", "title"));
    titleCol->addWidget(label("支持 CD / DVD 首次刻录、多区段追加与写后逐文件 SHA-256 完整性校验", "subtle"));
    head->addWidget(titleWrap);
    head->addStretch();
    ejectDiscButton = new QPushButton("弹出光盘");
    ejectDiscButton->setCursor(Qt::PointingHandCursor);
    refreshButton = new QPushButton("刷新光盘");
    refreshButton->setCursor(Qt::PointingHandCursor);
    head->addWidget(ejectDiscButton);
    head->addWidget(refreshButton);
    connect(ejectDiscButton, &QPushButton::clicked, this, &Window::ejectCurrentDisc);
    connect(refreshButton, &QPushButton::clicked, this, &Window::scan);
    l->addWidget(heading);

    // Device & Optical Media Overview Banner
    auto device = panel("device");
    auto d = hbox(device, 13, 14);
    gauge = new CapacityGauge;
    d->addWidget(gauge);

    auto choose = new QWidget;
    auto chooseLayout = vbox(choose, 0, 6);
    chooseLayout->addWidget(label("刻录设备", "sectionCap"));
    drive = new QComboBox;
    drive->setMinimumWidth(185);
    drive->setMaximumWidth(240);
    chooseLayout->addWidget(drive);
    driveSubLabel = label("等待扫描系统光驱设备", "subtle");
    chooseLayout->addWidget(driveSubLabel);
    d->addWidget(choose);

    d->addWidget(vline());

    auto type = new QWidget;
    auto typeLayout = vbox(type, 0, 6);
    typeLayout->addWidget(label("介质与状态", "sectionCap"));
    auto typeRow = new QWidget;
    auto trl = hbox(typeRow, 0, 8);
    typeLabel = label("未识别介质", "mediaTitle");
    profileBadge = label("—", "badge");
    profileBadge->setFixedHeight(22);
    profileBadge->setAlignment(Qt::AlignCenter);
    trl->addWidget(typeLabel, 0, Qt::AlignVCenter);
    trl->addWidget(profileBadge, 0, Qt::AlignVCenter);
    trl->addStretch();
    typeLayout->addWidget(typeRow);
    mediaState = label("未检测到光驱", "statePill");
    typeLayout->addWidget(mediaState);
    d->addWidget(type, 1);

    d->addWidget(vline());

    auto engine = new QWidget;
    auto engineLayout = vbox(engine, 0, 6);
    engineLayout->addWidget(label("文件系统与校验引擎", "sectionCap"));
    engineModeLabel = label("ISO 9660 + Joliet + Rock Ridge", "settingTitle");
    engineVerifyLabel = label("写后 SHA-256 完整性核对", "subtle");
    engineLayout->addWidget(engineModeLabel);
    engineLayout->addWidget(engineVerifyLabel);
    d->addWidget(engine, 1);

    d->addWidget(vline());

    auto cap = new QWidget;
    cap->setMinimumWidth(160);
    auto capLayout = vbox(cap, 0, 6);
    capLayout->addWidget(label("光盘容量与区段", "sectionCap"));
    capacityLabel = label("—", "capacity");
    sessionLabel = label("已有 0 个区段", "subtle");
    freeSubLabel = label("可用空间 —", "subtleAccent");
    capLayout->addWidget(capacityLabel);
    capLayout->addWidget(sessionLabel);
    capLayout->addWidget(freeSubLabel);
    d->addWidget(cap);
    l->addWidget(device);

    connect(drive, &QComboBox::activated, this, [this] {
        scan();
    });

    // Main Workspace Grid (Files Table + Write Configuration)
    auto grid = new QWidget;
    auto gl = hbox(grid, 0, 14);

    // Left: File Staging Panel
    auto filePanel = panel();
    auto fl = vbox(filePanel, 0, 0);
    auto tools = new QWidget;
    tools->setObjectName("panelhead");
    auto tl = hbox(tools, 10, 6);
    tl->setContentsMargins(14, 9, 12, 9);
    tl->addWidget(label("待写入文件", "panelTitle"));
    tl->addStretch();
    addButton = new QPushButton("添加文件");
    folderButton = new QPushButton("添加文件夹");
    removeSelectedButton = new QPushButton("移除选中");
    clearButton = new QPushButton("清空");
    for (auto* b : {addButton, folderButton, removeSelectedButton, clearButton}) {
        b->setCursor(Qt::PointingHandCursor);
        tl->addWidget(b);
    }
    fl->addWidget(tools);

    fileTable = new QTableWidget;
    tableStyle(fileTable, {"名称", "大小", "类型与来源路径", ""});
    fileTable->horizontalHeader()->setStretchLastSection(false);
    fileTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    fileTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fileTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    fileTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    fileTable->setColumnWidth(3, 42);
    fileTable->setMinimumHeight(245);
    fileTable->installEventFilter(this);

    // Empty state overlay inside fileTable
    auto emptyLayout = vbox(fileTable, 24, 0);
    emptyLayout->addStretch();
    emptyFileOverlay = new QFrame;
    emptyFileOverlay->setObjectName("emptyCard");
    emptyFileOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    emptyFileOverlay->setMaximumWidth(460);
    auto el = vbox(emptyFileOverlay, 22, 8);
    auto emptyTitle = label("拖拽文件或文件夹到此处，或点击上方按钮导入", "emptyTitle");
    emptyTitle->setAlignment(Qt::AlignCenter);
    auto emptySub = label("导入时自动检查剩余空间、重名冲突及 Windows / Joliet 中文目录名称兼容性", "subtle");
    emptySub->setAlignment(Qt::AlignCenter);
    emptySub->setWordWrap(true);
    el->addWidget(emptyTitle);
    el->addWidget(emptySub);
    auto emptyCenter = new QWidget;
    emptyCenter->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    auto ecl = hbox(emptyCenter, 0, 0);
    ecl->addStretch();
    ecl->addWidget(emptyFileOverlay);
    ecl->addStretch();
    emptyLayout->addWidget(emptyCenter);
    emptyLayout->addStretch();

    fl->addWidget(fileTable, 1);

    auto summary = new QWidget;
    summary->setObjectName("panelFoot");
    auto sl = hbox(summary, 10, 10);
    sl->setContentsMargins(14, 9, 14, 9);
    filesSummary = label("共 0 项", "subtle");
    plannedLabel = label("合计 0 KB", "subtleStrong");
    sl->addWidget(filesSummary);
    sl->addStretch();
    sl->addWidget(plannedLabel);
    fl->addWidget(summary);
    gl->addWidget(filePanel, 1);

    // Right: Write Settings Panel
    auto configPanel = panel();
    configPanel->setFixedWidth(286);
    auto cl = vbox(configPanel, 0, 0);
    auto configHeadWrap = new QWidget;
    configHeadWrap->setObjectName("panelhead");
    auto chl = hbox(configHeadWrap, 0, 6);
    chl->setContentsMargins(14, 11, 14, 11);
    chl->addWidget(label("写入设置", "panelTitle"));
    chl->addStretch();
    cl->addWidget(configHeadWrap);

    auto config = new QWidget;
    auto conf = vbox(config, 14, 8);

    auto labelHeader = new QWidget;
    auto lhl = hbox(labelHeader, 0, 4);
    lhl->addWidget(label("光盘名称 (卷标)", "fieldLabel"));
    lhl->addStretch();
    labelByteCount = label("0 / 32 字节", "subtle");
    lhl->addWidget(labelByteCount);
    conf->addWidget(labelHeader);

    discLabel = new QLineEdit(prefs.value("label", "研究资料").toString());
    discLabel->setMaxLength(32);
    discLabel->setPlaceholderText("输入光盘名称（最多 32 字节）");
    conf->addWidget(discLabel);

    connect(discLabel, &QLineEdit::textChanged, this, [this](const QString& text) {
        const int bytes = text.trimmed().toUtf8().size();
        if (labelByteCount) {
            labelByteCount->setText(QString("%1 / 32 字节").arg(bytes));
            labelByteCount->setStyleSheet(bytes > 32 || bytes == 0 ? "color:#f29d6b;" : "");
        }
    });
    {
        const int initBytes = discLabel->text().trimmed().toUtf8().size();
        labelByteCount->setText(QString("%1 / 32 字节").arg(initBytes));
    }

    conf->addSpacing(2);
    conf->addWidget(label("写入速度", "fieldLabel"));
    speed = new QComboBox;
    speed->addItem("自动（按介质最佳速度）", 0);
    for (int s : {4, 8, 16}) {
        speed->addItem(QString::number(s) + " 倍速", s);
    }
    conf->addWidget(speed);

    conf->addSpacing(2);
    conf->addWidget(label("写入模式", "fieldLabel"));
    auto modes = new QWidget;
    auto ml = hbox(modes, 0, 6);
    appendButton = new QPushButton("追加模式");
    finalButton = new QPushButton("写后封盘");
    for (auto* b : {appendButton, finalButton}) {
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        ml->addWidget(b);
    }
    conf->addWidget(modes);

    modeHintLabel = label("", "modeHint");
    modeHintLabel->setWordWrap(true);
    conf->addWidget(modeHintLabel);

    conf->addSpacing(2);
    conf->addWidget(hline());
    conf->addSpacing(2);
    conf->addWidget(label("完整性与后续动作", "fieldLabel"));

    verify = new QCheckBox("写后校验全部文件 (SHA-256)");
    verify->setChecked(prefs.value("verify", true).toBool());
    verify->setToolTip("刻录完成后从最新区段回读原有及新增文件并逐一比对 SHA-256");
    eject = new QCheckBox("完成后自动弹出光盘");
    eject->setChecked(prefs.value("eject", true).toBool());
    eject->setToolTip("刻录及校验流程结束后自动弹出光驱托盘");
    conf->addWidget(verify);
    conf->addWidget(eject);
    conf->addStretch();
    cl->addWidget(config, 1);
    gl->addWidget(configPanel);
    l->addWidget(grid, 1);

    // Capacity Chart Card
    chart = new CapacityChart;
    chart->setObjectName("panel");
    l->addWidget(chart);

    // Bottom Action Footer Card
    auto actions = panel("actionBar");
    auto al = hbox(actions, 12, 12);
    al->setContentsMargins(16, 11, 16, 11);
    readyLabel = label("未检测到光驱", "readyBadge");
    nextLabel = label("", "modeBadge");
    nextLabel->setFixedHeight(24);
    nextLabel->setAlignment(Qt::AlignCenter);
    actionHintLabel = label("将文件拖入上方列表即可自动核算剩余容量", "subtle");
    al->addWidget(readyLabel, 0, Qt::AlignVCenter);
    al->addWidget(nextLabel, 0, Qt::AlignVCenter);
    al->addWidget(actionHintLabel, 0, Qt::AlignVCenter);
    al->addStretch();
    burnButton = new QPushButton("开始刻录");
    burnButton->setObjectName("primary");
    burnButton->setMinimumWidth(164);
    burnButton->setCursor(Qt::PointingHandCursor);
    al->addWidget(burnButton, 0, Qt::AlignVCenter);
    l->addWidget(actions);

    connect(addButton, &QPushButton::clicked, this, [this] {
        auto paths = QFileDialog::getOpenFileNames(this, "添加文件");
        if (!paths.isEmpty()) importPaths(paths);
    });
    connect(folderButton, &QPushButton::clicked, this, [this] {
        auto path = QFileDialog::getExistingDirectory(this, "添加文件夹");
        if (!path.isEmpty()) importPaths({path});
    });
    connect(removeSelectedButton, &QPushButton::clicked, this, &Window::removeSelectedEntries);
    connect(clearButton, &QPushButton::clicked, this, [this] {
        entries.clear();
        updateView();
    });
    connect(appendButton, &QPushButton::clicked, this, [this] {
        finalize = false;
        updateView();
    });
    connect(finalButton, &QPushButton::clicked, this, [this] {
        finalize = true;
        updateView();
    });
    connect(burnButton, &QPushButton::clicked, this, &Window::startBurn);
    connect(fileTable, &QTableWidget::itemSelectionChanged, this, [this] {
        if (removeSelectedButton) {
            removeSelectedButton->setEnabled(!busy && !fileTable->selectedItems().isEmpty());
        }
    });

    return scroll;
}

QWidget* Window::listPage(bool sessions) {
    auto page = new QWidget;
    auto l = vbox(page, 20, 14);

    if (sessions) {
        // Sessions Page Header
        auto headWrap = new QWidget;
        auto hl = hbox(headWrap, 0, 10);
        auto titleCol = new QWidget;
        auto tcl = vbox(titleCol, 0, 2);
        tcl->addWidget(label("光盘区段", "title"));
        tcl->addWidget(label("展示光驱返回的物理介质状态、区段数量、扇区地址及容量分布", "subtle"));
        hl->addWidget(titleCol);
        hl->addStretch();
        refreshSessionButton = new QPushButton("刷新介质信息");
        refreshSessionButton->setCursor(Qt::PointingHandCursor);
        connect(refreshSessionButton, &QPushButton::clicked, this, &Window::scan);
        hl->addWidget(refreshSessionButton);
        l->addWidget(headWrap);

        // 4 Metric Cards Row
        auto cardsWidget = new QWidget;
        auto cardsRow = hbox(cardsWidget, 0, 12);
        auto makeMetricCard = [&](const QString& title, QLabel*& valOut, QLabel*& subOut) {
            auto c = panel("metricCard");
            auto cl = vbox(c, 14, 5);
            cl->addWidget(label(title, "sectionCap"));
            valOut = label("—", "metricValue");
            subOut = label("—", "subtle");
            cl->addWidget(valOut);
            cl->addWidget(subOut);
            cardsRow->addWidget(c, 1);
        };
        makeMetricCard("当前设备", sessDeviceVal, sessDeviceSub);
        makeMetricCard("介质状态", sessStateVal, sessStateSub);
        makeMetricCard("已识别区段", sessCountVal, sessCountSub);
        makeMetricCard("剩余可用空间", sessFreeVal, sessFreeSub);
        l->addWidget(cardsWidget);

        // Visual Capacity Chart on Sessions Page
        sessionChart = new CapacityChart;
        sessionChart->setObjectName("panel");
        l->addWidget(sessionChart);

        // Detailed Parameter Table inside Panel
        auto tableCard = panel();
        auto tcl2 = vbox(tableCard, 0, 0);
        auto th = new QWidget;
        th->setObjectName("panelhead");
        auto thl = hbox(th, 0, 8);
        thl->setContentsMargins(14, 10, 14, 10);
        thl->addWidget(label("介质与多区段技术参数明细", "panelTitle"));
        thl->addStretch();
        tcl2->addWidget(th);

        sessionTable = new QTableWidget;
        tableStyle(sessionTable, {"项目", "内容"});
        sessionTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        sessionTable->setColumnWidth(0, 240);
        sessionTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        sessionTable->setMinimumWidth(360);
        tcl2->addWidget(sessionTable, 1);
        l->addWidget(tableCard, 1);
    } else {
        // History Page Header
        auto headWrap = new QWidget;
        auto hl = hbox(headWrap, 0, 10);
        auto titleCol = new QWidget;
        auto tcl = vbox(titleCol, 0, 2);
        tcl->addWidget(label("任务记录", "title"));
        tcl->addWidget(label("保留最近 200 次刻录、写后 SHA-256 校验与弹出任务的执行档案", "subtle"));
        hl->addWidget(titleCol);
        hl->addStretch();
        historyStatsLabel = label("共 0 条记录", "badge");
        historyStatsLabel->setFixedHeight(26);
        historyStatsLabel->setAlignment(Qt::AlignCenter);
        clearHistoryButton = new QPushButton("清空记录");
        clearHistoryButton->setCursor(Qt::PointingHandCursor);
        hl->addWidget(historyStatsLabel, 0, Qt::AlignVCenter);
        hl->addWidget(clearHistoryButton, 0, Qt::AlignVCenter);
        l->addWidget(headWrap);

        auto historyCard = panel();
        auto hcl = vbox(historyCard, 0, 0);
        auto hh = new QWidget;
        hh->setObjectName("panelhead");
        auto hhl = hbox(hh, 0, 8);
        hhl->setContentsMargins(14, 10, 14, 10);
        hhl->addWidget(label("历史刻录任务列表", "panelTitle"));
        hhl->addStretch();
        hhl->addWidget(label("点击任意记录可查看下方执行摘要与诊断日志", "subtle"));
        hcl->addWidget(hh);

        historyTable = new QTableWidget;
        tableStyle(historyTable, {"时间", "写入方式", "大小", "状态"});
        historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        auto emptyLayout = vbox(historyTable, 20, 0);
        emptyLayout->addStretch();
        emptyHistoryOverlay = new QFrame;
        emptyHistoryOverlay->setObjectName("emptyCard");
        emptyHistoryOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        emptyHistoryOverlay->setMaximumWidth(420);
        auto el = vbox(emptyHistoryOverlay, 18, 6);
        auto et = label("暂无历史刻录记录", "emptyTitle");
        et->setAlignment(Qt::AlignCenter);
        auto es = label("完成首次或追加刻录后，此处将自动记录时间、写入字节数、校验结果与详细日志", "subtle");
        es->setAlignment(Qt::AlignCenter);
        es->setWordWrap(true);
        el->addWidget(et);
        el->addWidget(es);
        auto ec = new QWidget;
        ec->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        auto ecl = hbox(ec, 0, 0);
        ecl->addStretch();
        ecl->addWidget(emptyHistoryOverlay);
        ecl->addStretch();
        emptyLayout->addWidget(ec);
        emptyLayout->addStretch();

        hcl->addWidget(historyTable, 1);
        l->addWidget(historyCard, 3);

        // Task Detail & Log Viewer Panel
        auto detailCard = panel();
        auto dcl = vbox(detailCard, 0, 0);
        auto dh = new QWidget;
        dh->setObjectName("panelhead");
        auto dhl = hbox(dh, 0, 8);
        dhl->setContentsMargins(14, 9, 14, 9);
        dhl->addWidget(label("选中任务详情与执行日志", "panelTitle"));
        dhl->addStretch();
        dcl->addWidget(dh);
        historyDetailView = new QPlainTextEdit;
        historyDetailView->setReadOnly(true);
        historyDetailView->setPlaceholderText("在上方列表中选择一条任务记录，即可查看对应的写入、校验、弹出结果及工具输出日志。");
        historyDetailView->setMinimumHeight(130);
        dcl->addWidget(historyDetailView, 1);
        l->addWidget(detailCard, 2);

        connect(historyTable, &QTableWidget::currentCellChanged, this,
                [this](int currentRow, int, int, int) { showHistoryDetail(currentRow); });
        connect(clearHistoryButton, &QPushButton::clicked, this, [this] {
            if (history.isEmpty()) return;
            if (QMessageBox::question(this, "清空任务记录", "确定要清空全部历史任务记录吗？",
                                      QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes) {
                return;
            }
            history = QJsonArray();
            const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
            QDir().mkpath(dir);
            QSaveFile f(dir + "/history.json");
            if (f.open(QIODevice::WriteOnly)) {
                f.write(QJsonDocument(history).toJson());
                f.commit();
            }
            refreshHistory();
        });
    }
    return page;
}

QWidget* Window::settingsPage() {
    auto scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto page = new QWidget;
    auto l = vbox(page, 20, 14);
    scroll->setWidget(page);

    auto titleCol = new QWidget;
    auto tcl = vbox(titleCol, 0, 2);
    tcl->addWidget(label("设置", "title"));
    tcl->addWidget(label("配置容量换算单位、界面强调色、表格密度及默认写后校验行为", "subtle"));
    l->addWidget(titleCol);

    auto columns = new QWidget;
    auto colLayout = hbox(columns, 0, 16);

    // Left Card: Preferences
    auto box = panel();
    auto boxLayout = vbox(box, 0, 0);
    auto boxHead = new QWidget;
    boxHead->setObjectName("panelhead");
    auto bhl = hbox(boxHead, 0, 8);
    bhl->setContentsMargins(16, 11, 16, 11);
    bhl->addWidget(label("界面外观与默认刻录行为", "panelTitle"));
    bhl->addStretch();
    boxLayout->addWidget(boxHead);

    auto rowsWrap = new QWidget;
    auto rowsLayout = vbox(rowsWrap, 18, 14);

    auto addSettingRow = [&](const QString& title, const QString& desc, QWidget* control) {
        auto rowWidget = new QWidget;
        auto rl = hbox(rowWidget, 0, 16);
        auto textWrap = new QWidget;
        auto tl = vbox(textWrap, 0, 3);
        tl->addWidget(label(title, "settingTitle"));
        auto sub = label(desc, "subtle");
        sub->setWordWrap(true);
        tl->addWidget(sub);
        rl->addWidget(textWrap, 1);
        rl->addWidget(control, 0, Qt::AlignRight | Qt::AlignVCenter);
        rowsLayout->addWidget(rowWidget);
    };

    unit = new QComboBox;
    unit->setFixedWidth(210);
    unit->addItem("自动 (KB / MB / GB)", "auto");
    for (auto u : {"KB", "MB", "GB"}) unit->addItem(u, u);
    unit->setCurrentIndex(qMax(0, unit->findData(unitSetting())));
    addSettingRow("容量显示单位", "按 1024 进制换算，统一应用于文件列表、容量仪表与任务记录", unit);
    rowsLayout->addWidget(hline());

    theme = new QComboBox;
    theme->setFixedWidth(210);
    theme->addItem("冰蓝 (默认)", "blue");
    theme->addItem("青绿 (极光)", "cyan");
    theme->addItem("琥珀 (暖金)", "amber");
    theme->setCurrentIndex(qMax(0, theme->findData(prefs.value("theme", "blue"))));
    addSettingRow("主题强调色", "同步切换主操作按钮、侧边栏高亮、容量圆环与分配图表配色", theme);
    rowsLayout->addWidget(hline());

    density = new QComboBox;
    density->setFixedWidth(210);
    density->addItem("标准间距", false);
    density->addItem("紧凑视图", true);
    density->setCurrentIndex(prefs.value("compact", false).toBool() ? 1 : 0);
    addSettingRow("界面表格密度", "调整待写入文件、光盘区段与历史记录表格的行高间距", density);
    rowsLayout->addWidget(hline());

    defaultVerify = new QCheckBox("启用");
    defaultVerify->setChecked(prefs.value("verify", true).toBool());
    addSettingRow("默认写后校验", "启动时默认开启 SHA-256 逐文件回读校验（CD 采用 SG_IO 扇区直读）", defaultVerify);
    rowsLayout->addWidget(hline());

    defaultEject = new QCheckBox("启用");
    defaultEject->setChecked(prefs.value("eject", true).toBool());
    addSettingRow("默认完成后弹出", "启动时默认开启写入及校验完成后自动弹出光驱托盘", defaultEject);
    rowsLayout->addWidget(hline());

    auto storageBadge = label("本地存储", "badge");
    storageBadge->setFixedHeight(24);
    storageBadge->setAlignment(Qt::AlignCenter);
    addSettingRow("配置与记录存储", "设置项保存于 ~/.config/DiscForge，最近 200 次任务保存于 ~/.local/share/DiscForge", storageBadge);

    boxLayout->addWidget(rowsWrap);
    boxLayout->addStretch();
    colLayout->addWidget(box, 3);

    // Right Card: System Dependencies & Environment Info
    auto envBox = panel();
    auto envLayout = vbox(envBox, 0, 0);
    auto envHead = new QWidget;
    envHead->setObjectName("panelhead");
    auto ehl = hbox(envHead, 0, 8);
    ehl->setContentsMargins(16, 11, 16, 11);
    ehl->addWidget(label("系统刻录组件与环境自检", "panelTitle"));
    ehl->addStretch();
    envLayout->addWidget(envHead);

    auto envBody = new QWidget;
    auto ebl = vbox(envBody, 16, 10);

    const QList<QPair<QString, QString>> toolsList = {
        {"genisoimage", "ISO 9660 / Joliet / Rock Ridge 映像生成"},
        {"wodim", "CD-R / CD-RW 多区段 TAO 写入引擎"},
        {"growisofs", "DVD±R / DVD±RW 首次与追加写入引擎"},
        {"udisksctl", "UDisks2 非特权光盘挂载与重扫接口"},
        {"findmnt", "系统文件挂载点查询工具"},
        {"eject", "物理光驱托盘弹出控制工具"}
    };

    for (const auto& item : toolsList) {
        auto row = new QWidget;
        auto rl = hbox(row, 0, 10);
        auto info = new QWidget;
        auto il = vbox(info, 0, 2);
        il->addWidget(label(item.first, "settingTitle"));
        il->addWidget(label(item.second, "subtle"));
        rl->addWidget(info, 1);
        const QString found = QStandardPaths::findExecutable(item.first);
        auto statusBadge = label(found.isEmpty() ? "未安装" : "● 已就绪", found.isEmpty() ? "warnBadge" : "okBadge");
        statusBadge->setFixedHeight(24);
        statusBadge->setAlignment(Qt::AlignCenter);
        if (!found.isEmpty()) statusBadge->setToolTip(found);
        rl->addWidget(statusBadge, 0, Qt::AlignVCenter);
        ebl->addWidget(row);
    }

    ebl->addWidget(hline());
    auto aboutTitle = label(QString("光铸 · DiscForge v%1").arg(DISCFORGE_VERSION), "settingTitle");
    ebl->addWidget(aboutTitle);
    auto aboutDesc = label(
        "轻量级原生 Linux 数据光盘刻录工具。使用系统 Qt6 动态库与标准刻录后端，无后台驻留与联网行为。",
        "subtle");
    aboutDesc->setWordWrap(true);
    ebl->addWidget(aboutDesc);
    ebl->addStretch();
    envLayout->addWidget(envBody, 1);
    colLayout->addWidget(envBox, 2);

    l->addWidget(columns);
    l->addStretch();

    connect(unit, &QComboBox::currentIndexChanged, this, [this] {
        prefs.setValue("units", unit->currentData());
        updateView();
        refreshHistory();
    });
    connect(theme, &QComboBox::currentIndexChanged, this, [this] {
        prefs.setValue("theme", theme->currentData());
        applyAppearance();
    });
    connect(density, &QComboBox::currentIndexChanged, this, [this] {
        prefs.setValue("compact", density->currentData());
        applyAppearance();
    });
    connect(defaultVerify, &QCheckBox::toggled, this, [this](bool on) {
        prefs.setValue("verify", on);
        verify->setChecked(on);
    });
    connect(defaultEject, &QCheckBox::toggled, this, [this](bool on) {
        prefs.setValue("eject", on);
        eject->setChecked(on);
    });
    return scroll;
}

bool Window::eventFilter(QObject* watched, QEvent* event) {
    if (watched == fileTable && event->type() == QEvent::KeyPress && !busy) {
        auto* ke = static_cast<QKeyEvent*>(event);
        if (ke->key() == Qt::Key_Delete || ke->key() == Qt::Key_Backspace) {
            removeSelectedEntries();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void Window::removeSelectedEntries() {
    if (busy || !fileTable) return;
    QSet<int> rows;
    for (const auto* item : fileTable->selectedItems()) {
        rows.insert(item->row());
    }
    if (rows.isEmpty()) return;
    QList<int> sorted = rows.values();
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int r : sorted) {
        if (r >= 0 && r < entries.size()) {
            entries.removeAt(r);
        }
    }
    updateView();
}

void Window::ejectCurrentDisc() {
    if (busy) return;
    const QString target = !disc.device.isEmpty() ? disc.device : drive->currentData().toString();
    if (target.isEmpty()) return;
    setBusy(true);
    appendLog("正在请求弹出光盘：" + target);
    auto watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher] {
        const QString err = watcher->result();
        watcher->deleteLater();
        setBusy(false);
        if (!err.isEmpty()) {
            appendLog("弹出光盘失败：" + err);
            QMessageBox::warning(this, "弹出光盘", err);
        } else {
            appendLog("光盘已弹出");
            scan();
        }
    });
    watcher->setFuture(QtConcurrent::run([target] {
        try {
            df::unmountDisc(target);
            const auto r = df::command("eject", {target}, 30000);
            if (r.code != 0) return QString("弹出失败：") + r.text().trimmed();
            return QString();
        } catch (const std::exception& e) {
            return QString::fromUtf8(e.what());
        }
    }));
}

qint64 Window::plannedBytes() const {
    qint64 n = 0;
    for (const auto& e : entries) n += e.bytes;
    return n;
}

void Window::updateView() {
    const QColor accent = currentAccent();
    int folderCount = 0, fileCount = 0;
    fileTable->setRowCount(entries.size());
    for (int i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        const bool isDir = isDirectoryEntry(e);
        if (isDir) ++folderCount;
        else ++fileCount;

        auto* name = new QTableWidgetItem(makeEntryIcon(isDir, accent), e.name);
        name->setToolTip(e.path);
        fileTable->setItem(i, 0, name);

        auto* size = new QTableWidgetItem(formatted(e.bytes));
        size->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        fileTable->setItem(i, 1, size);

        const QString typePath = QString("%1 · %2").arg(isDir ? "文件夹" : "文件", e.path);
        auto* sourceItem = new QTableWidgetItem(typePath);
        sourceItem->setForeground(QColor("#829db6"));
        sourceItem->setToolTip(e.path);
        fileTable->setItem(i, 2, sourceItem);

        auto* remove = new QPushButton("×");
        remove->setObjectName("rowRemove");
        remove->setToolTip("移除 " + e.name);
        remove->setCursor(Qt::PointingHandCursor);
        remove->setEnabled(!busy);
        connect(remove, &QPushButton::clicked, this, [this, i] {
            if (i >= 0 && i < entries.size()) {
                entries.removeAt(i);
                updateView();
            }
        });
        fileTable->setCellWidget(i, 3, remove);
    }

    if (emptyFileOverlay) {
        emptyFileOverlay->setVisible(entries.isEmpty());
    }
    if (removeSelectedButton) {
        removeSelectedButton->setEnabled(!busy && !fileTable->selectedItems().isEmpty());
    }
    if (clearButton) {
        clearButton->setEnabled(!busy && !entries.isEmpty());
    }

    if (entries.isEmpty()) {
        filesSummary->setText("共 0 项 · 支持拖入文件与文件夹");
    } else {
        filesSummary->setText(
            QString("共 %1 项（文件夹 %2 · 文件 %3）").arg(entries.size()).arg(folderCount).arg(fileCount));
    }
    plannedLabel->setText("待写入合计 " + formatted(plannedBytes()));

    const QString cType = disc.chineseType();
    typeLabel->setText(cType);
    if (profileBadge) {
        profileBadge->setText(disc.profile.isEmpty() ? "未识别" : disc.profile);
    }
    const QString stateMsg = disc.message.isEmpty() ? "未检测到光驱" : disc.message;
    mediaState->setText("● " + stateMsg);
    if (disc.writable()) {
        mediaState->setStyleSheet("color:#5fe0b7;font-weight:600;");
    } else if (disc.state == "closed") {
        mediaState->setStyleSheet("color:#eebc7e;font-weight:600;");
    } else {
        mediaState->setStyleSheet("color:#90a7be;");
    }

    capacityLabel->setText(formatted(disc.total));
    sessionLabel->setText(QString("已有 %1 个区段").arg(disc.sessions));
    if (freeSubLabel) {
        freeSubLabel->setText("当前可用 " + formatted(disc.free));
    }
    if (driveSubLabel) {
        driveSubLabel->setText(disc.device.isEmpty() ? "未检测到系统光驱 (/dev/sr*)" : "设备节点：" + disc.device);
    }
    if (engineModeLabel && engineVerifyLabel) {
        if (disc.family == "dvd") {
            engineModeLabel->setText("growisofs · Joliet + Rock Ridge");
            engineVerifyLabel->setText("写后挂载回读 · SHA-256 逐文件校验");
        } else {
            engineModeLabel->setText("genisoimage + wodim (TAO)");
            engineVerifyLabel->setText("SG_IO 扇区直读 · SHA-256 逐文件校验");
        }
    }
    if (ejectDiscButton) {
        ejectDiscButton->setEnabled(!busy && (!disc.device.isEmpty() || drive->count() > 0));
    }

    appendButton->setChecked(!finalize);
    finalButton->setChecked(finalize);
    if (modeHintLabel) {
        modeHintLabel->setText(finalize
            ? "写后封盘：刻录完成后关闭光盘（Fixate），跨系统只读兼容性最佳，但无法再次追加文件。"
            : "追加模式：保留多区段引导信息，本次刻录后光盘仍可继续追加写入新文件。");
    }

    const qint64 used = (disc.total >= 0 && disc.free >= 0) ? qMax<qint64>(0, disc.total - disc.free) : 0;
    gauge->total = qMax<qint64>(0, disc.total);
    gauge->used = used;
    gauge->planned = plannedBytes();
    gauge->accentColor = accent;
    gauge->update();

    chart->total = qMax<qint64>(0, disc.total);
    chart->used = used;
    chart->planned = plannedBytes();
    chart->unit = unitSetting();
    chart->accentColor = accent;
    chart->update();

    if (sessionChart) {
        sessionChart->total = qMax<qint64>(0, disc.total);
        sessionChart->used = used;
        sessionChart->planned = plannedBytes();
        sessionChart->unit = unitSetting();
        sessionChart->accentColor = accent;
        sessionChart->update();
    }

    const bool space = disc.free >= plannedBytes() + 16 * 1024 * 1024;
    const bool ready = !busy && !entries.isEmpty() && disc.writable() && space;
    burnButton->setEnabled(ready);

    const QString readyText = busy ? "正在处理"
        : entries.isEmpty()        ? "未添加文件"
        : !disc.writable()         ? "介质不可写入"
        : !space                   ? "剩余空间不足"
                                   : "准备就绪";
    readyLabel->setText("● " + readyText);
    readyLabel->setStyleSheet(ready ? "color:#5fe0b7;font-weight:600;"
                              : (!space && !entries.isEmpty() && disc.writable()) ? "color:#f29d6b;font-weight:600;"
                                                                                  : "color:#adc4da;font-weight:600;");
    nextLabel->setText(finalize ? "写入后封盘" : disc.blank() ? "首次写入" : "追加区段");

    if (actionHintLabel) {
        if (ready) {
            const qint64 afterFree = qMax<qint64>(0, disc.free - plannedBytes());
            actionHintLabel->setText("预计写入后剩余 " + formatted(afterFree));
        } else if (!entries.isEmpty() && disc.writable() && !space) {
            actionHintLabel->setText("需预留至少 16 MB 多区段开销，请精简待写入文件");
        } else {
            actionHintLabel->setText("写入前将自动缓存源文件并校验已有目录");
        }
    }

    statusLabel->setText(busy ? "正在处理" : stateMsg);
    if (topStatusPill) {
        topStatusPill->setText(disc.device.isEmpty() ? stateMsg : QString("%1 · %2").arg(disc.device, stateMsg));
    }
    if (sideDriveLabel) {
        sideDriveLabel->setText(disc.device.isEmpty() ? "未检测到光驱" : disc.device);
        sideMediaLabel->setText(QString("%1 · %2").arg(cType, stateMsg));
        sideFreeLabel->setText("可用 " + formatted(disc.free));
    }

    // Update Sessions Page Metric Cards & Table
    if (sessDeviceVal) {
        sessDeviceVal->setText(disc.device.isEmpty() ? "—" : disc.device);
        sessDeviceSub->setText(disc.profile.isEmpty() ? cType : QString("%1 (%2)").arg(cType, disc.profile));
        sessStateVal->setText(stateMsg);
        sessStateSub->setText(disc.writable() ? "支持写入与追加" : "当前不可写入");
        sessCountVal->setText(QString("%1 个区段").arg(disc.sessions));
        sessCountSub->setText(disc.nextSector > 0 ? QString("下一扇区 LBA %1").arg(disc.nextSector) : "起始扇区 LBA 0");
        sessFreeVal->setText(formatted(disc.free));
        sessFreeSub->setText("总容量 " + formatted(disc.total));
    }

    sessionTable->setRowCount(0);
    auto add = [this](const QString& k, const QString& v) {
        const int r = sessionTable->rowCount();
        sessionTable->insertRow(r);
        auto* kItem = new QTableWidgetItem(k);
        kItem->setForeground(QColor("#96b0c8"));
        sessionTable->setItem(r, 0, kItem);
        sessionTable->setItem(r, 1, new QTableWidgetItem(v));
    };
    add("设备", disc.device.isEmpty() ? "—" : disc.device);
    add("介质", cType);
    add("介质规格", disc.profile.isEmpty() ? "—" : disc.profile);
    add("状态", stateMsg);
    add("已识别区段数", QString::number(disc.sessions));
    add("总容量", formatted(disc.total));
    add("已用空间", disc.total >= 0 ? formatted(used) : "—");
    add("剩余空间", formatted(disc.free));
    add("计划写入后可用", disc.free >= 0 ? formatted(qMax<qint64>(0, disc.free - plannedBytes())) : "—");
    add("多区段偏移 / 下一扇区", !disc.msinfo.isEmpty() ? QString("msinfo %1 (LBA %2)").arg(disc.msinfo).arg(disc.nextSector)
                                                      : (disc.blank() ? "LBA 0（空白光盘首次刻录）" : "—"));
}

void Window::setBusy(bool on) {
    busy = on;
    for (QWidget* w : QList<QWidget*>{drive, refreshButton, ejectDiscButton, refreshSessionButton,
                                      addButton, folderButton, removeSelectedButton, clearButton,
                                      appendButton, finalButton, discLabel, speed, verify, eject,
                                      clearHistoryButton}) {
        if (w) w->setEnabled(!on);
    }
    updateView();
}

void Window::scan() {
    if (busy) return;
    const QString selected = drive->currentData().toString();
    const auto list = df::devices();
    {
        QSignalBlocker block(drive);
        drive->clear();
        for (const auto& d : list) {
            drive->addItem(QString("光驱 %1 (%2)").arg(d.section('/', -1).mid(2).toInt() + 1).arg(d), d);
        }
        if (!selected.isEmpty() && drive->findData(selected) >= 0) {
            drive->setCurrentIndex(drive->findData(selected));
        }
    }
    if (list.isEmpty()) {
        disc = {};
        disc.message = "未检测到光驱";
        updateView();
        return;
    }
    const QString target = drive->currentData().toString();
    setBusy(true);
    auto watcher = new QFutureWatcher<df::Disc>(this);
    connect(watcher, &QFutureWatcher<df::Disc>::finished, this, [this, watcher] {
        disc = watcher->result();
        watcher->deleteLater();
        setBusy(false);
        appendLog(disc.message);
        if (disc.state == "unknown" && !disc.raw.isEmpty()) appendLog(disc.raw);
    });
    watcher->setFuture(QtConcurrent::run([target] {
        try {
            return df::inspect(target);
        } catch (const std::exception& e) {
            df::Disc d;
            d.device = target;
            d.message = QString::fromUtf8(e.what());
            return d;
        }
    }));
}

void Window::importPaths(const QStringList& paths) {
    if (busy) return;
    setBusy(true);
    auto watcher = new QFutureWatcher<QPair<QList<df::Entry>, QString>>(this);
    connect(watcher, &QFutureWatcher<QPair<QList<df::Entry>, QString>>::finished, this, [this, watcher] {
        auto r = watcher->result();
        watcher->deleteLater();
        for (const auto& e : r.first) {
            bool exists = false;
            for (const auto& old : entries) {
                if (old.name.toCaseFolded() == e.name.toCaseFolded()) exists = true;
            }
            if (!exists) entries << e;
            else r.second += "\n重名项目未添加：" + e.name;
        }
        setBusy(false);
        if (!r.second.isEmpty()) QMessageBox::warning(this, "导入结果", r.second.trimmed());
    });
    watcher->setFuture(QtConcurrent::run([paths] {
        QPair<QList<df::Entry>, QString> result;
        for (const auto& p : paths) {
            try {
                result.first << df::entry(p);
            } catch (const std::exception& e) {
                result.second += '\n' + QString::fromUtf8(e.what());
            }
        }
        return result;
    }));
}

void Window::startBurn() {
    if (busy || !burnButton->isEnabled()) return;
    df::Options options{
        disc.device,
        discLabel->text().trimmed(),
        speed->currentData().toInt(),
        finalize,
        verify->isChecked(),
        eject->isChecked()
    };
    if (options.label.isEmpty() || options.label.toUtf8().size() > 32) {
        QMessageBox::warning(this, "光盘名称", "请输入32字节以内的光盘名称。");
        return;
    }
    const auto message = QString("设备：%1\n卷标：%2\n写入：%3（共 %4 项）\n方式：%5%6")
                             .arg(drive->currentText(),
                                  options.label,
                                  formatted(plannedBytes()),
                                  QString::number(entries.size()),
                                  finalize ? "写入后封盘" : "保留追加能力",
                                  finalize ? "\n封盘后无法继续追加。" : "");
    if (QMessageBox::question(this, "确认刻录", message, QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) !=
        QMessageBox::Yes) {
        return;
    }
    prefs.setValue("label", options.label);
    burning = true;
    taskClock.start();
    elapsedLabel->setText("已用 00:00");
    taskTimer->start();
    taskProgress->setStyleSheet("");
    taskPanel->show();
    updateProgress({"检查光盘", -1});
    setBusy(true);
    outputDock->show();
    auto work = entries;
    auto watcher = new QFutureWatcher<df::BurnResult>(this);
    connect(watcher, &QFutureWatcher<df::BurnResult>::finished, this, [this, watcher, options] {
        const auto result = watcher->result();
        watcher->deleteLater();
        burning = false;
        taskTimer->stop();
        setBusy(false);
        saveHistory(result, options);
        QStringList states{result.written ? "数据写入完成" : "写入未完成"};
        if (result.written && options.verify) states << (result.verified ? "校验通过" : "校验未完成");
        if (result.written && options.eject) states << (result.ejected ? "已弹出" : "弹出失败");
        phaseLabel->setText(states.join(" · "));
        taskProgress->setRange(0, 100);
        taskProgress->setValue(result.written ? 100 : 0);
        if (!result.success) taskProgress->setStyleSheet("QProgressBar::chunk{background:#eebc7e;}");
        if (result.success) QMessageBox::information(this, "刻录完成", result.message);
        else if (result.written) QMessageBox::warning(this, "写入完成，后续处理有警告", result.message);
        else QMessageBox::critical(this, "写入未完成", result.message);
        scan();
    });
    watcher->setFuture(QtConcurrent::run([this, work, options] {
        return df::burn(
            work,
            options,
            [this](const QString& text) {
                QMetaObject::invokeMethod(this, [this, text] { appendLog(text); }, Qt::QueuedConnection);
            },
            [this](const df::Progress& progress) {
                QMetaObject::invokeMethod(
                    this, [this, progress] { if (burning) updateProgress(progress); }, Qt::QueuedConnection);
            });
    }));
}

void Window::updateProgress(const df::Progress& progress) {
    phaseLabel->setText(progress.stage +
                        (progress.percent >= 0 ? QString(" · %1%").arg(progress.percent) : QString()));
    if (progress.percent < 0) {
        taskProgress->setRange(0, 0);
    } else {
        taskProgress->setRange(0, 100);
        taskProgress->setValue(qBound(0, progress.percent, 100));
    }
}

void Window::appendLog(const QString& text) {
    output->appendPlainText(QTime::currentTime().toString("HH:mm:ss") + "  " + text);
}

void Window::selectPage(int id) {
    stack->setCurrentIndex(id);
    navigation->button(id)->setChecked(true);
}

void Window::loadHistory() {
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(dir + "/history.json");
    if (file.open(QIODevice::ReadOnly)) {
        history = QJsonDocument::fromJson(file.readAll()).array();
    }
    refreshHistory();
}

void Window::saveHistory(const df::BurnResult& r, const df::Options& o) {
    QJsonObject item{
        {"time", QDateTime::currentDateTime().toString(Qt::ISODate)},
        {"mode", o.finalize ? "封盘" : "追加"},
        {"bytes", double(r.bytes)},
        {"status",
         !r.written ? "写入未完成"
         : o.verify && !r.verified ? "写入完成，校验未完成"
         : o.eject && !r.ejected ? (r.verified ? "校验通过，弹出失败" : "写入完成，弹出失败")
         : r.verified ? "校验通过"
                      : "写入完成，未校验"},
        {"message", r.message},
        {"log", QJsonArray::fromStringList(r.log)}
    };
    item.insert("written", r.written);
    item.insert("verified", r.verified);
    item.insert("ejected", r.ejected);
    item.insert("verificationError", r.verificationError);
    item.insert("ejectError", r.ejectError);
    history.prepend(item);
    while (history.size() > 200) history.removeLast();
    const auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QSaveFile f(dir + "/history.json");
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(history).toJson());
        if (!f.commit()) appendLog("任务记录保存失败");
    } else {
        appendLog("任务记录保存失败");
    }
    refreshHistory();
}

void Window::refreshHistory() {
    if (!historyTable) return;
    historyTable->setRowCount(history.size());
    int passedCount = 0;
    for (int i = 0; i < history.size(); ++i) {
        const auto o = history[i].toObject();
        const QString statusStr = o["status"].toString();
        if (statusStr.contains("校验通过") || statusStr == "写入完成，未校验") {
            ++passedCount;
        }
        const QStringList cells{
            QDateTime::fromString(o["time"].toString(), Qt::ISODate).toString("yyyy-MM-dd HH:mm"),
            o["mode"].toString(),
            formatted(qint64(o["bytes"].toDouble())),
            statusStr
        };
        for (int c = 0; c < 4; ++c) {
            auto* item = new QTableWidgetItem(cells[c]);
            item->setToolTip(o["message"].toString());
            if (c == 3) {
                if (statusStr.contains("未完成") || statusStr.contains("失败")) {
                    item->setForeground(kWarnColor);
                } else if (statusStr.contains("校验通过")) {
                    item->setForeground(QColor("#5fe0b7"));
                }
            }
            historyTable->setItem(i, c, item);
        }
    }
    if (emptyHistoryOverlay) {
        emptyHistoryOverlay->setVisible(history.isEmpty());
    }
    if (clearHistoryButton) {
        clearHistoryButton->setEnabled(!busy && !history.isEmpty());
    }
    if (historyStatsLabel) {
        historyStatsLabel->setText(
            history.isEmpty() ? "共 0 条记录"
                              : QString("共 %1 条记录 · 正常完成 %2 次").arg(history.size()).arg(passedCount));
    }
    if (!history.isEmpty()) {
        historyTable->selectRow(0);
        showHistoryDetail(0);
    } else if (historyDetailView) {
        historyDetailView->clear();
    }
}

void Window::showHistoryDetail(int row) {
    if (!historyDetailView) return;
    if (row < 0 || row >= history.size()) {
        historyDetailView->clear();
        return;
    }
    const auto o = history[row].toObject();
    QStringList lines;
    lines << QString("【任务时间】 %1")
                 .arg(QDateTime::fromString(o["time"].toString(), Qt::ISODate).toString("yyyy-MM-dd HH:mm:ss"));
    lines << QString("【写入方式】 %1    【数据大小】 %2    【状态】 %3")
                 .arg(o["mode"].toString(), formatted(qint64(o["bytes"].toDouble())), o["status"].toString());
    if (!o["message"].toString().isEmpty()) {
        lines << QString("【结果摘要】 %1").arg(o["message"].toString().replace('\n', " | "));
    }
    if (!o["verificationError"].toString().isEmpty()) {
        lines << QString("【校验诊断】 %1").arg(o["verificationError"].toString());
    }
    if (!o["ejectError"].toString().isEmpty()) {
        lines << QString("【弹出诊断】 %1").arg(o["ejectError"].toString());
    }
    const auto logArr = o["log"].toArray();
    if (!logArr.isEmpty()) {
        lines << "──────────────────── 执行日志 ────────────────────";
        for (const auto& entry : logArr) {
            lines << entry.toString();
        }
    }
    historyDetailView->setPlainText(lines.join('\n'));
}

QColor Window::currentAccent() const {
    if (theme && theme->currentData() == "cyan") return QColor("#4ee0c8");
    if (theme && theme->currentData() == "amber") return QColor("#f2b866");
    return QColor("#5eb8ff");
}

void Window::applyAppearance() {
    const QColor accentCol = currentAccent();
    const QString accent = accentCol.name();
    const QString accentHover = accentCol.lighter(115).name();
    const QString assetPath = uiAssetsDir.isValid() ? uiAssetsDir.path() : QString();

    if (auto* w = findChild<QWidget*>("brandEmblem")) {
        auto* emblem = static_cast<BrandEmblem*>(w);
        emblem->accent = accentCol;
        emblem->update();
    }
    if (navigation) {
        for (int i = 0; i < 4; ++i) {
            if (auto* b = navigation->button(i)) {
                b->setIcon(makeNavIcon(i, accentCol));
            }
        }
    }

    setStyleSheet(QString(R"(
        QWidget {
            background: #0d141d;
            color: #e4eef8;
            font-family: "Noto Sans CJK SC", "Source Han Sans SC", "Microsoft YaHei", sans-serif;
            font-size: 12px;
        }
        QMainWindow {
            background: #0d141d;
        }
        QLabel {
            background: transparent;
        }
        QFrame#topbar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #16283b, stop:0.6 #121e2c, stop:1 #0f1823);
            border-bottom: 1px solid #283b50;
        }
        QLabel#brand {
            font-size: 19px;
            font-weight: 700;
            color: #e8f4ff;
            letter-spacing: 1px;
        }
        QLabel#brandSub {
            font-size: 11px;
            color: #7794b0;
            padding-left: 4px;
        }
        QLabel#topPill {
            background: #142536;
            border: 1px solid #2c4762;
            border-radius: 11px;
            padding: 3px 11px;
            font-size: 11px;
            color: #b4d2ec;
        }
        QLabel#versionBadge {
            background: #172a3e;
            border: 1px solid #355472;
            border-radius: 4px;
            padding: 3px 8px;
            font-size: 11px;
            color: %1;
            font-weight: 600;
        }
        QLabel#title {
            font-size: 20px;
            font-weight: 600;
            color: #ecf5ff;
        }
        QLabel#panelTitle {
            font-size: 13px;
            font-weight: 600;
            color: #dcebfa;
        }
        QLabel#sectionCap {
            font-size: 11px;
            color: #829db6;
            font-weight: 500;
        }
        QLabel#fieldLabel {
            font-size: 11px;
            color: #96b0c9;
            font-weight: 600;
        }
        QLabel#settingTitle {
            font-size: 13px;
            font-weight: 600;
            color: #deecfa;
        }
        QLabel#mediaTitle {
            font-size: 15px;
            font-weight: 600;
            color: #eaf5ff;
        }
        QLabel#metricValue {
            font-size: 16px;
            font-weight: 600;
            color: #e6f3ff;
        }
        QLabel#subtle {
            color: #86a0b9;
            font-size: 11px;
        }
        QLabel#subtleStrong {
            color: #cce3f8;
            font-size: 12px;
            font-weight: 600;
        }
        QLabel#subtleAccent {
            color: %1;
            font-size: 11px;
            font-weight: 500;
        }
        QLabel#capacity {
            font-size: 19px;
            font-weight: 600;
            color: #e2f2ff;
        }
        QLabel#badge {
            background: #1a2f45;
            border: 1px solid #355575;
            border-radius: 4px;
            padding: 2px 8px;
            font-size: 11px;
            color: #c2dff8;
        }
        QLabel#okBadge {
            background: #12312a;
            border: 1px solid #286655;
            border-radius: 4px;
            padding: 2px 8px;
            font-size: 11px;
            color: #63e2b7;
        }
        QLabel#warnBadge {
            background: #352718;
            border: 1px solid #73512a;
            border-radius: 4px;
            padding: 2px 8px;
            font-size: 11px;
            color: #f2b866;
        }
        QLabel#modeBadge {
            background: #193149;
            border: 1px solid #35587a;
            border-radius: 4px;
            padding: 3px 9px;
            font-size: 11px;
            color: #cbe5fc;
        }
        QLabel#modeHint {
            background: #101a25;
            border: 1px solid #24384d;
            border-radius: 5px;
            padding: 8px 10px;
            color: #8da8c2;
            font-size: 11px;
        }
        QFrame#vdivider {
            background: #283c52;
        }
        QFrame#hdivider {
            background: #223346;
        }
        QFrame#sidebar {
            background: #101924;
            border-right: 1px solid #243548;
        }
        QLabel#navSection {
            font-size: 10px;
            font-weight: 600;
            color: #69839c;
            letter-spacing: 1px;
        }
        QFrame#sideCard {
            background: #142130;
            border: 1px solid #263b52;
            border-radius: 6px;
        }
        QLabel#sideCardTitle {
            font-size: 10px;
            color: #7996b2;
            font-weight: 600;
        }
        QLabel#sideCardMain {
            font-size: 12px;
            color: #dcecf9;
            font-weight: 600;
        }
        QPushButton {
            border: 1px solid #334a62;
            background: #192838;
            color: #dceaf7;
            padding: 6px 12px;
            border-radius: 5px;
            font-weight: 500;
        }
        QPushButton:hover {
            background: #243a50;
            border-color: %1;
        }
        QPushButton:disabled {
            color: #566b80;
            border-color: #22303f;
            background: #131d28;
        }
        QPushButton:checked {
            background: #1f415e;
            color: #e6f4ff;
            border: 1px solid %1;
            font-weight: 600;
        }
        QPushButton#nav {
            text-align: left;
            background: transparent;
            color: #9bb1c7;
            border: 1px solid transparent;
            border-radius: 6px;
            padding: 10px 12px;
            font-size: 13px;
        }
        QPushButton#nav:hover {
            background: #172636;
            color: #dcecf9;
        }
        QPushButton#nav:checked {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #1f3d5a, stop:1 #172c42);
            color: #eef7ff;
            border: 1px solid #35597b;
            border-left: 3px solid %1;
            font-weight: 600;
        }
        QPushButton#primary {
            background: %1;
            color: #071624;
            border: 1px solid %1;
            border-radius: 6px;
            font-size: 13px;
            font-weight: 700;
            padding: 9px 24px;
        }
        QPushButton#primary:hover {
            background: %2;
            border-color: %2;
        }
        QPushButton#primary:disabled {
            background: #1c2e40;
            color: #637a91;
            border: 1px solid #2b4055;
        }
        QPushButton#rowRemove {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 4px;
            color: #7f9ab3;
            padding: 0px;
            font-size: 15px;
            font-weight: 700;
            margin: 3px 5px;
        }
        QPushButton#rowRemove:hover {
            background: #3d1d28;
            border-color: #8f394e;
            color: #ff94a6;
        }
        QPushButton#statusLogBtn {
            background: #182738;
            border: 1px solid #2f455c;
            border-radius: 4px;
            padding: 2px 10px;
            font-size: 11px;
            color: #b7d2eb;
        }
        QPushButton#statusLogBtn:hover {
            border-color: %1;
            color: #ffffff;
        }
        QFrame#panel, CapacityChart#panel, QFrame#metricCard {
            background: #131e2b;
            border: 1px solid #273a4f;
            border-radius: 7px;
        }
        QFrame#actionBar {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #152536, stop:1 #131e2b);
            border: 1px solid #2e4760;
            border-radius: 7px;
        }
        QFrame#device {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #183149, stop:0.55 #152435, stop:1 #131e2b);
            border: 1px solid #365777;
            border-radius: 7px;
        }
        QFrame#device QWidget {
            background: transparent;
        }
        QFrame#device QComboBox {
            background: #0e1823;
        }
        QFrame#emptyCard {
            background: #101a25;
            border: 1px dashed #314b66;
            border-radius: 8px;
        }
        QLabel#emptyTitle {
            font-size: 13px;
            font-weight: 600;
            color: #bdd6ee;
        }
        QWidget#panelhead, QLabel#panelhead {
            background: #172636;
            border-bottom: 1px solid #273a4f;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
        }
        QWidget#panelFoot {
            background: #111b27;
            border-top: 1px solid #243649;
            border-bottom-left-radius: 6px;
            border-bottom-right-radius: 6px;
        }
        QComboBox, QLineEdit {
            border: 1px solid #354c63;
            border-radius: 5px;
            background: #0e1722;
            padding: 6px 10px;
            color: #e5eff9;
            selection-background-color: #2f5475;
        }
        QComboBox:hover, QLineEdit:hover, QLineEdit:focus {
            border-color: %1;
        }
        QComboBox::drop-down {
            subcontrol-origin: padding;
            subcontrol-position: top right;
            width: 26px;
            border: 0px;
            background: transparent;
        }
        QComboBox::down-arrow {
            image: url(%3/chevron.png);
            width: 12px;
            height: 12px;
        }
        QComboBox QAbstractItemView {
            background: #162535;
            border: 1px solid #385572;
            selection-background-color: #264c6e;
            padding: 4px;
        }
        QTableWidget {
            background: #131e2b;
            border: 0;
            selection-background-color: #1f3d58;
            selection-color: #eef7ff;
            gridline-color: #243649;
        }
        QTableWidget::item {
            padding: 6px 10px;
            border-bottom: 1px solid #1e2e3e;
        }
        QHeaderView::section {
            background: #101924;
            border: 0;
            border-bottom: 1px solid #26394d;
            padding: 8px 10px;
            color: #89a4be;
            font-size: 11px;
            font-weight: 600;
            text-align: left;
        }
        QTableCornerButton::section {
            background: #101924;
            border: 0;
        }
        QCheckBox {
            spacing: 9px;
            background: transparent;
            color: #dceaf8;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #44607c;
            border-radius: 4px;
            background: #0e1722;
        }
        QCheckBox::indicator:hover {
            border-color: %1;
        }
        QCheckBox::indicator:checked {
            background: %1;
            border-color: %1;
            image: url(%3/check.png);
        }
        QStatusBar {
            background: #101924;
            border-top: 1px solid #243548;
            color: #8fa9c2;
        }
        QStatusBar::item {
            border: 0;
        }
        QScrollArea {
            border: 0;
            background: transparent;
        }
        QScrollBar:vertical {
            background: #0e1620;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #2f475e;
            min-height: 24px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QPlainTextEdit {
            background: #0b121a;
            border: 1px solid #25374a;
            border-radius: 5px;
            padding: 6px;
            color: #a9c4dc;
            font-family: "JetBrains Mono", "Noto Sans Mono CJK SC", monospace;
            font-size: 11px;
        }
        QDockWidget {
            color: #c2d9ee;
            font-weight: 600;
        }
        QFrame#taskPanel {
            background: #15283b;
            border-bottom: 1px solid #335474;
        }
        QLabel#taskPhase {
            font-weight: 600;
            color: #e4f2ff;
        }
        QProgressBar {
            background: #21364a;
            border: 0;
            border-radius: 4px;
        }
        QProgressBar::chunk {
            background: %1;
            border-radius: 4px;
        }
        QToolTip {
            color: #e8f2fc;
            background: #1b2f44;
            border: 1px solid #4d7396;
            padding: 5px 8px;
        }
    )").arg(accent, accentHover, assetPath));

    const int rowH = prefs.value("compact", false).toBool() ? 32 : 42;
    if (fileTable) fileTable->verticalHeader()->setDefaultSectionSize(rowH);
    if (sessionTable) sessionTable->verticalHeader()->setDefaultSectionSize(rowH);
    if (historyTable) historyTable->verticalHeader()->setDefaultSectionSize(rowH);
    updateView();
}

void Window::closeEvent(QCloseEvent* e) {
    if (busy) {
        QMessageBox::information(this, "正在处理", "当前任务结束后才能关闭程序。");
        e->ignore();
        return;
    }
    e->accept();
}

void Window::dragEnterEvent(QDragEnterEvent* e) {
    if (!busy && e->mimeData()->hasUrls()) e->acceptProposedAction();
}

void Window::dropEvent(QDropEvent* e) {
    if (busy) return;
    QStringList paths;
    for (const auto& url : e->mimeData()->urls()) {
        if (url.isLocalFile()) paths << url.toLocalFile();
    }
    if (!paths.isEmpty()) {
        selectPage(0);
        importPaths(paths);
        e->acceptProposedAction();
    }
}

bool Window::selfTest(const QString& path) {
    if (!path.isEmpty()) resize(1180, 920);
    disc.device = "/dev/sr0";
    disc.family = "cd";
    disc.profile = "CD-R";
    disc.state = "appendable";
    disc.message = "可追加";
    disc.sessions = 2;
    disc.nextSector = 104448;
    disc.msinfo = "0,104448";
    disc.total = 700LL * 1024 * 1024;
    disc.free = 496LL * 1024 * 1024;
    entries = {
        {"/tmp/实验数据", "实验数据", 182LL * 1024 * 1024},
        {"/tmp/项目技术报告.pdf", "项目技术报告.pdf", 26LL * 1024 * 1024}
    };
    drive->addItem("光驱一 (/dev/sr0)", "/dev/sr0");
    updateView();

    bool ok = burnButton->isEnabled() && fileTable->rowCount() == 2;
    const auto before = prefs.value("units", "auto");
    for (const auto& setting : QStringList{"KB", "MB", "GB"}) {
        unit->setCurrentIndex(unit->findData(setting));
        ok = ok && fileTable->item(0, 1)->text().endsWith(setting);
    }
    unit->setCurrentIndex(unit->findData(before));
    selectPage(3);
    ok = ok && stack->currentIndex() == 3;
    selectPage(0);
    taskPanel->show();
    updateProgress({"写入数据", 42});
    elapsedLabel->setText("已用 00:18");
    ok = ok && taskProgress->value() == 42 && phaseLabel->text().contains("42%");
    updateProgress({"关闭区段", -1});
    ok = ok && taskProgress->minimum() == 0 && taskProgress->maximum() == 0;
    updateProgress({"写入数据", 42});
    show();
    QApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    if (!path.isEmpty()) {
        taskPanel->hide();
        QApplication::processEvents();
        QPixmap image(size());
        render(&image);
        ok = image.save(path) && ok;
        const QFileInfo location(path);
        const auto directory = location.absolutePath();
        // Self-test only: capture real widgets with synthetic, non-private data.
        taskPanel->hide();
        selectPage(1);
        QApplication::processEvents();
        QPixmap sessions(size());
        render(&sessions);
        ok = sessions.save(directory + "/光盘区段.png") && ok;
        selectPage(3);
        QApplication::processEvents();
        QPixmap settings(size());
        render(&settings);
        ok = settings.save(directory + "/设置.png") && ok;
    }
    return ok;
}
