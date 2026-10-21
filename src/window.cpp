#include "window.h"
#include <cmath>
static const QColor blue("#72bdff"),cyan("#6ce3da"),muted("#92aabd");
void CapacityGauge::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const QPointF center(width()/2.,height()/2.);
    p.translate(center);
    p.setPen(QPen(QColor("#496a84"),1));
    for(int i=0;i<48;i++) {
        p.save();
        p.rotate(i*7.5);
        p.drawLine(QPointF(0,-47),QPointF(0,i%4?-44:-42));
        p.restore();
    }
    auto arc=[&](QColor c,int start,int span) {
        p.setPen(QPen(c,5));
        p.drawArc(QRectF(-35,-35,70,70),start,span);
    };
    arc(QColor("#2b4156"),0,360*16);
    if(total>0) {
        double a=qBound(0.,double(used)/total,1.),b=qBound(0.,double(planned)/total,1.-a);
        arc(QColor("#6389a5"),90*16,-int(a*360*16));
        arc(cyan,int((90-a*360)*16),-int(b*360*16));
    }
    p.setPen(QPen(QColor("#45637a"),1,Qt::DashLine));
    p.drawEllipse(QRectF(-27,-27,54,54));
    p.setPen(total>0&&used+planned>total?QColor("#f3b17f"):QColor("#d5efff"));
    QFont f=font();
    f.setPointSize(14);
    p.setFont(f);
    p.drawText(QRectF(-32,-15,64,25),Qt::AlignCenter,total>0?QString::number(double(used+planned)/total*100,'f',0)+"%":"—");
    f.setPointSize(7);
    p.setFont(f);
    p.setPen(muted);
    p.drawText(QRectF(-35,9,70,16),Qt::AlignCenter,"写入后占用");
}
void CapacityChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const double left=15,right=width()-15,w=right-left;
    QFont f=font();
    f.setPointSize(8);
    p.setFont(f);
    p.setPen(QColor("#c8deef"));
    p.drawText(QRectF(left,7,w,18),Qt::AlignLeft,"容量分配");
    p.setPen(muted);
    p.drawText(QRectF(left,7,w,18),Qt::AlignRight,"写入后可用 "+df::formatBytes(qMax<qint64>(0,total-used-planned),unit));
    double a=total>0?qBound(0.,double(used)/total,1.):0,b=total>0?qBound(0.,double(planned)/total,1.-a):0;
    p.fillRect(QRectF(left,34,w,12),QColor("#273b4d"));
    p.fillRect(QRectF(left,34,w*a,12),QColor("#5b7f9a"));
    p.fillRect(QRectF(left+w*a,34,w*b,12),cyan);
    p.setPen(QColor("#597b96"));
    p.drawRect(QRectF(left,34,w,12));
    for(int i=0;i<=4;i++) {
        double x=left+w*i/4;
        p.setPen(QPen(QColor("#3c576b"),1,Qt::DashLine));
        p.drawLine(QPointF(x,30),QPointF(x,52));
        p.setPen(muted);
        Qt::Alignment align=i==0?Qt::AlignLeft:i==4?Qt::AlignRight:Qt::AlignHCenter;
        double labelX=i==0?x:i==4?x-42:x-21;
        p.drawText(QRectF(labelX,54,42,17),align,QString::number(i*25)+"%");
    }p.setPen(QColor("#8fa9bd"));
    p.drawText(QRectF(left,73,w,15),Qt::AlignLeft,"已有 "+df::formatBytes(used,unit)+"    待写入 "+df::formatBytes(planned,unit));
}
static QFrame* panel() {
    auto f=new QFrame;
    f->setObjectName("panel");
    return f;
}
static QHBoxLayout* hbox(QWidget* w,int m=0,int gap=8) {
    auto l=new QHBoxLayout(w);
    l->setContentsMargins(m,m,m,m);
    l->setSpacing(gap);
    return l;
}
static QVBoxLayout* vbox(QWidget* w,int m=0,int gap=8) {
    auto l=new QVBoxLayout(w);
    l->setContentsMargins(m,m,m,m);
    l->setSpacing(gap);
    return l;
}
static QLabel* label(const QString& s,const char* name=nullptr) {
    auto l=new QLabel(s);
    if(name)l->setObjectName(name);
    return l;
}
static void tableStyle(QTableWidget* t,const QStringList& heads) {
    t->setColumnCount(heads.size());
    t->setHorizontalHeaderLabels(heads);
    t->verticalHeader()->hide();
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::ExtendedSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setShowGrid(false);
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    t->setAlternatingRowColors(false);
    t->setWordWrap(false);
    t->setMinimumHeight(170);
}
Window::Window():prefs("DiscForge","DiscForge") {
    setWindowTitle("光铸 · 光盘刻录");
    resize(1110,760);
    setMinimumSize(850,610);
    setAcceptDrops(true);
    auto root=new QWidget;
    auto vertical=vbox(root);
    setCentralWidget(root);
    auto top=new QFrame;
    top->setObjectName("topbar");
    top->setFixedHeight(49);
    auto topLayout=hbox(top,14);
    topLayout->addWidget(label("◎  光铸","brand"));
    topLayout->addStretch();
    topLayout->addWidget(label("2.0 · 候选版","subtle"));
    vertical->addWidget(top);
    taskPanel = new QFrame;
    taskPanel->setObjectName("taskPanel");
    auto taskLayout = vbox(taskPanel, 10, 5);
    auto taskLabels = new QWidget;
    auto taskRow = hbox(taskLabels);
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
        elapsedLabel->setText(QString("已用 %1:%2").arg(seconds/60, 2, 10, QChar('0')).arg(seconds%60, 2, 10, QChar('0')));
    });

    auto body=new QWidget;
    auto bodyLayout=hbox(body,0,0);
    vertical->addWidget(body,1);
    auto side=new QFrame;
    side->setObjectName("sidebar");
    side->setFixedWidth(150);
    auto navLayout=vbox(side,10,7);
    navigation=new QButtonGroup(this);
    QStringList titles= {
        "数据刻录","光盘区段","任务记录","设置"
    };
    for(int i=0;i<4;i++) {
        if(i==3)navLayout->addStretch();
        auto b=new QPushButton(titles[i]);
        b->setCheckable(true);
        b->setObjectName("nav");
        navigation->addButton(b,i);
        navLayout->addWidget(b);
    }navigation->button(0)->setChecked(true);
    connect(navigation,&QButtonGroup::idClicked,this,&Window::selectPage);
    bodyLayout->addWidget(side);
    stack=new QStackedWidget;
    bodyLayout->addWidget(stack,1);
    stack->addWidget(burnPage());
    stack->addWidget(listPage(true));
    stack->addWidget(listPage(false));
    stack->addWidget(settingsPage());
    statusLabel=new QLabel;
    statusBar()->addWidget(statusLabel,1);
    auto outputButton=new QPushButton("刻录日志");
    outputButton->setFlat(true);
    statusBar()->addPermanentWidget(outputButton);
    outputDock=new QDockWidget("刻录日志",this);
    output=new QPlainTextEdit;
    output->setReadOnly(true);
    output->setMaximumBlockCount(5000);
    outputDock->setWidget(output);
    addDockWidget(Qt::BottomDockWidgetArea,outputDock);
    outputDock->hide();
    connect(outputButton,&QPushButton::clicked,this,[this] {
        outputDock->setVisible(!outputDock->isVisible());
    });
    loadHistory();
    applyAppearance();
    updateView();
    if(!QCoreApplication::arguments().contains("--self-test"))QTimer::singleShot(0,this,&Window::scan);
}
QWidget* Window::burnPage() {
    auto scroll=new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto page=new QWidget;
    auto l=vbox(page,14,10);
    scroll->setWidget(page);
    auto heading=new QWidget;
    auto head=hbox(heading);
    head->addWidget(label("数据刻录","title"));
    head->addStretch();
    refreshButton=new QPushButton("刷新光盘");
    head->addWidget(refreshButton);
    connect(refreshButton,&QPushButton::clicked,this,&Window::scan);
    l->addWidget(heading);
    auto device=panel();
    device->setObjectName("device");
    auto d=hbox(device,12,18);
    gauge=new CapacityGauge;
    d->addWidget(gauge);
    auto choose=new QWidget;
    auto chooseLayout=vbox(choose,0,7);
    chooseLayout->addWidget(label("刻录设备","subtle"));
    drive=new QComboBox;
    drive->setMinimumWidth(190);
    chooseLayout->addWidget(drive);
    d->addWidget(choose,1);
    auto type=new QWidget;
    auto typeLayout=vbox(type,0,8);
    typeLabel=label("未识别介质");
    mediaState=label("未检测到光驱","subtle");
    typeLayout->addWidget(typeLabel);
    typeLayout->addWidget(mediaState);
    d->addWidget(type);
    auto cap=new QWidget;
    auto capLayout=vbox(cap,0,8);
    capacityLabel=label("—","capacity");
    sessionLabel=label("已有 0 个区段","subtle");
    capLayout->addWidget(capacityLabel);
    capLayout->addWidget(sessionLabel);
    d->addWidget(cap);
    l->addWidget(device);
    connect(drive,&QComboBox::activated,this,[this] {
        scan();
    });
    auto grid=new QWidget;
    auto gl=hbox(grid,0,14);
    auto filePanel=panel();
    auto fl=vbox(filePanel,0,0);
    auto tools=new QWidget;
    tools->setObjectName("panelhead");
    auto tl=hbox(tools,9,3);
    tl->addWidget(label("待写入文件"));
    tl->addStretch();
    addButton=new QPushButton("添加文件");
    folderButton=new QPushButton("添加文件夹");
    clearButton=new QPushButton("清空");
    tl->addWidget(addButton);
    tl->addWidget(folderButton);
    tl->addWidget(clearButton);
    fl->addWidget(tools);
    fileTable=new QTableWidget;
    tableStyle(fileTable, {
        "名称","大小",""
    });
    fileTable->horizontalHeader()->setStretchLastSection(false);
    fileTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    fileTable->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
    fileTable->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Fixed);
    fileTable->setColumnWidth(2,38);
    fileTable->setMinimumHeight(235);
    fl->addWidget(fileTable,1);
    auto summary=new QWidget;
    summary->setObjectName("panelhead");
    auto sl=hbox(summary,10);
    filesSummary=label("共 0 项","subtle");
    plannedLabel=label("合计 0 KB","subtle");
    sl->addWidget(filesSummary);
    sl->addStretch();
    sl->addWidget(plannedLabel);
    fl->addWidget(summary);
    gl->addWidget(filePanel,1);
    auto configPanel=panel();
    configPanel->setFixedWidth(250);
    auto cl=vbox(configPanel,0,0);
    auto configHead=label("写入设置");
    configHead->setObjectName("panelhead");
    configHead->setContentsMargins(14,13,14,13);
    cl->addWidget(configHead);
    auto config=new QWidget;
    auto conf=vbox(config,12,4);
    conf->addWidget(label("光盘名称","subtle"));
    discLabel=new QLineEdit(prefs.value("label","研究资料").toString());
    discLabel->setMaxLength(32);
    conf->addWidget(discLabel);
    conf->addSpacing(2);
    conf->addWidget(label("写入速度","subtle"));
    speed=new QComboBox;
    speed->addItem("自动",0);
    for(int s: {
        4,8,16
    })speed->addItem(QString::number(s)+" 倍速",s);
    conf->addWidget(speed);
    conf->addSpacing(2);
    conf->addWidget(label("写入模式","subtle"));
    auto modes=new QWidget;
    auto ml=hbox(modes,0,0);
    appendButton=new QPushButton("追加");
    finalButton=new QPushButton("封盘");
    for(auto b: {
        appendButton,finalButton
    }) {
        b->setCheckable(true);
        ml->addWidget(b);
    }conf->addWidget(modes);
    verify=new QCheckBox("写后校验");
    verify->setChecked(prefs.value("verify",true).toBool());
    eject=new QCheckBox("完成后弹出");
    eject->setChecked(prefs.value("eject",true).toBool());
    conf->addSpacing(2);
    conf->addWidget(verify);
    conf->addWidget(eject);
    conf->addStretch();
    cl->addWidget(config,1);
    gl->addWidget(configPanel);
    l->addWidget(grid,1);
    chart=new CapacityChart;
    chart->setObjectName("panel");
    l->addWidget(chart);
    auto actions=new QWidget;
    auto al=hbox(actions);
    readyLabel=label("未检测到光驱");
    nextLabel=label("","subtle");
    al->addWidget(readyLabel);
    al->addWidget(nextLabel);
    al->addStretch();
    burnButton=new QPushButton("开始刻录");
    burnButton->setObjectName("primary");
    burnButton->setMinimumWidth(155);
    al->addWidget(burnButton);
    l->addWidget(actions);
    connect(addButton,&QPushButton::clicked,this,[this] {
        auto paths=QFileDialog::getOpenFileNames(this,"添加文件");if(!paths.isEmpty())importPaths(paths);
    });
    connect(folderButton,&QPushButton::clicked,this,[this] {
        auto path=QFileDialog::getExistingDirectory(this,"添加文件夹");if(!path.isEmpty())importPaths( {
            path
        });
    });
    connect(clearButton,&QPushButton::clicked,this,[this] {
        entries.clear();updateView();
    });
    connect(appendButton,&QPushButton::clicked,this,[this] {
        finalize=false;updateView();
    });
    connect(finalButton,&QPushButton::clicked,this,[this] {
        finalize=true;updateView();
    });
    connect(burnButton,&QPushButton::clicked,this,&Window::startBurn);
    return scroll;
}
QWidget* Window::listPage(bool sessions) {
    auto page=new QWidget;
    auto l=vbox(page,20,14);
    l->addWidget(label(sessions?"光盘区段":"任务记录","title"));
    auto table=new QTableWidget;
    tableStyle(table,sessions?QStringList {
        "项目","内容"
    }:QStringList {
        "时间","写入方式","大小","状态"
    });
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    if(sessions)sessionTable=table;
    else historyTable=table;
    l->addWidget(table);
    return page;
}
QWidget* Window::settingsPage() {
    auto page=new QWidget;
    auto l=vbox(page,20,14);
    l->addWidget(label("设置","title"));
    auto box=panel();
    auto form=new QFormLayout(box);
    form->setContentsMargins(20,18,20,18);
    form->setVerticalSpacing(20);
    unit=new QComboBox;
    unit->addItem("自动","auto");
    for(auto u: {
        "KB","MB","GB"
    })unit->addItem(u,u);
    unit->setCurrentIndex(qMax(0,unit->findData(unitSetting())));
    form->addRow("显示单位",unit);
    theme=new QComboBox;
    theme->addItem("冰蓝","blue");
    theme->addItem("青绿","cyan");
    theme->addItem("琥珀","amber");
    theme->setCurrentIndex(qMax(0,theme->findData(prefs.value("theme","blue"))));
    form->addRow("强调色",theme);
    density=new QComboBox;
    density->addItem("标准",false);
    density->addItem("紧凑",true);
    density->setCurrentIndex(prefs.value("compact",false).toBool()?1:0);
    form->addRow("界面密度",density);
    defaultVerify=new QCheckBox;
    defaultVerify->setChecked(prefs.value("verify",true).toBool());
    form->addRow("默认写后校验",defaultVerify);
    defaultEject=new QCheckBox;
    defaultEject->setChecked(prefs.value("eject",true).toBool());
    form->addRow("默认完成后弹出",defaultEject);
    l->addWidget(box);
    l->addStretch();
    connect(unit,&QComboBox::currentIndexChanged,this,[this] {
        prefs.setValue("units",unit->currentData());updateView();refreshHistory();
    });
    connect(theme,&QComboBox::currentIndexChanged,this,[this] {
        prefs.setValue("theme",theme->currentData());applyAppearance();
    });
    connect(density,&QComboBox::currentIndexChanged,this,[this] {
        prefs.setValue("compact",density->currentData());applyAppearance();
    });
    connect(defaultVerify,&QCheckBox::toggled,this,[this](bool on) {
        prefs.setValue("verify",on);verify->setChecked(on);
    });
    connect(defaultEject,&QCheckBox::toggled,this,[this](bool on) {
        prefs.setValue("eject",on);eject->setChecked(on);
    });
    return page;
}
qint64 Window::plannedBytes()const {
    qint64 n=0;
    for(auto& e:entries)n+=e.bytes;
    return n;
}
void Window::updateView() {
    fileTable->setRowCount(entries.size());
    for(int i=0;i<entries.size();i++) {
        const auto& e=entries[i];
        auto name=new QTableWidgetItem(e.name);
        name->setToolTip(e.path);
        fileTable->setItem(i,0,name);
        auto size=new QTableWidgetItem(formatted(e.bytes));
        size->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        fileTable->setItem(i,1,size);
        auto remove=new QPushButton("×");
        remove->setToolTip("移除");
        remove->setEnabled(!busy);
        connect(remove,&QPushButton::clicked,this,[this,i] {
            entries.removeAt(i);updateView();
        });
        fileTable->setCellWidget(i,2,remove);
    }filesSummary->setText(QString("共 %1 项").arg(entries.size()));
    plannedLabel->setText("合计 "+formatted(plannedBytes()));
    typeLabel->setText(disc.chineseType());
    mediaState->setText(disc.message.isEmpty()?"未检测到光驱":disc.message);
    capacityLabel->setText(formatted(disc.total));
    sessionLabel->setText(QString("已有 %1 个区段").arg(disc.sessions));
    appendButton->setChecked(!finalize);
    finalButton->setChecked(finalize);
    qint64 used=disc.total>=0&&disc.free>=0?qMax<qint64>(0,disc.total-disc.free):0;
    gauge->total=qMax<qint64>(0,disc.total);
    gauge->used=used;
    gauge->planned=plannedBytes();
    gauge->update();
    chart->total=qMax<qint64>(0,disc.total);
    chart->used=used;
    chart->planned=plannedBytes();
    chart->unit=unitSetting();
    chart->update();
    const bool space=disc.free>=plannedBytes()+16*1024*1024;
    burnButton->setEnabled(!busy&&!entries.isEmpty()&&disc.writable()&&space);
    readyLabel->setText(busy?"正在处理":entries.isEmpty()?"未添加文件":!disc.writable()?"介质不可写入":!space?"剩余空间不足":"准备就绪");
    nextLabel->setText(finalize?"写入后封盘":disc.blank()?"首次写入":"追加区段");
    statusLabel->setText(busy?"正在处理":disc.message.isEmpty()?"未检测到光驱":disc.message);
    sessionTable->setRowCount(0);
    auto add=[this](const QString& k,const QString& v) {
        int r=sessionTable->rowCount();
        sessionTable->insertRow(r);
        sessionTable->setItem(r,0,new QTableWidgetItem(k));
        sessionTable->setItem(r,1,new QTableWidgetItem(v));
    };
    add("设备",disc.device.isEmpty()?"—":disc.device);
    add("介质",disc.chineseType());
    add("状态",disc.message);
    add("已识别区段数",QString::number(disc.sessions));
    add("剩余空间",formatted(disc.free));
}
void Window::setBusy(bool on) {
    busy=on;
    for(QWidget* w:QList<QWidget*> {
        drive,refreshButton,addButton,folderButton,clearButton,appendButton,finalButton,discLabel,speed,verify,eject
    })w->setEnabled(!on);
    updateView();
}
void Window::scan() {
    if(busy)return;
    const QString selected=drive->currentData().toString();
    const auto list=df::devices();
    {
        QSignalBlocker block(drive);
        drive->clear();
        for(const auto& d:list)drive->addItem(QString("光驱 %1").arg(d.section('/',-1).mid(2).toInt()+1),d);
        if(!selected.isEmpty()&&drive->findData(selected)>=0)drive->setCurrentIndex(drive->findData(selected));
    }if(list.isEmpty()) {
        disc= {
        };
        disc.message="未检测到光驱";
        updateView();
        return;
    }const QString target=drive->currentData().toString();
    setBusy(true);
    auto watcher=new QFutureWatcher<df::Disc>(this);
    connect(watcher,&QFutureWatcher<df::Disc>::finished,this,[this,watcher] {
        disc=watcher->result();watcher->deleteLater();setBusy(false);appendLog(disc.message);
        if (disc.state == "unknown" && !disc.raw.isEmpty()) appendLog(disc.raw);
    });
    watcher->setFuture(QtConcurrent::run([target] {
        try {
            return df::inspect(target);
        }catch(const std::exception& e) {
            df::Disc d;d.device=target;d.message=QString::fromUtf8(e.what());return d;
        }
    }));
}
void Window::importPaths(const QStringList& paths) {
    if(busy)return;
    setBusy(true);
    auto watcher=new QFutureWatcher<QPair<QList<df::Entry>,QString>>(this);
    connect(watcher,&QFutureWatcher<QPair<QList<df::Entry>,QString>>::finished,this,[this,watcher] {
        auto r=watcher->result();watcher->deleteLater();for(const auto& e:r.first) {
            bool exists=false;for(const auto& old:entries)if(old.name.toCaseFolded()==e.name.toCaseFolded())exists=true;if(!exists)entries<<e;else r.second+="\n重名项目未添加："+e.name;
        }setBusy(false);if(!r.second.isEmpty())QMessageBox::warning(this,"导入结果",r.second.trimmed());
    });
    watcher->setFuture(QtConcurrent::run([paths] {
        QPair<QList<df::Entry>,QString> result;for(const auto& p:paths) {
            try {
                result.first<<df::entry(p);
            }catch(const std::exception& e) {
                result.second+='\n'+QString::fromUtf8(e.what());
            }
        }return result;
    }));
}
void Window::startBurn() {
    if(busy||!burnButton->isEnabled())return;
    df::Options options {
        disc.device,discLabel->text().trimmed(),speed->currentData().toInt(),finalize,verify->isChecked(),eject->isChecked()
    };
    if(options.label.isEmpty()||options.label.toUtf8().size()>32) {
        QMessageBox::warning(this,"光盘名称","请输入32字节以内的光盘名称。");
        return;
    }const auto message=QString("设备：%1\n写入：%2\n方式：%3%4").arg(drive->currentText(),formatted(plannedBytes()),finalize?"写入后封盘":"保留追加能力",finalize?"\n封盘后无法继续追加。":"");
    if(QMessageBox::question(this,"确认刻录",message,QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)!=QMessageBox::Yes)return;
    prefs.setValue("label",options.label);
    burning = true;
    taskClock.start();
    elapsedLabel->setText("已用 00:00");
    taskTimer->start();
    taskProgress->setStyleSheet("");
    taskPanel->show();
    updateProgress({"检查光盘", -1});
    setBusy(true);
    outputDock->show();
    auto work=entries;
    auto watcher=new QFutureWatcher<df::BurnResult>(this);
    connect(watcher,&QFutureWatcher<df::BurnResult>::finished,this,[this,watcher,options] {
        const auto result = watcher->result();
        watcher->deleteLater();
        burning = false;
        taskTimer->stop();
        setBusy(false);
        saveHistory(result,options);
        QStringList states{result.written ? "数据写入完成" : "写入未完成"};
        if (result.written && options.verify) states << (result.verified ? "校验通过" : "校验未完成");
        if (result.written && options.eject) states << (result.ejected ? "已弹出" : "弹出失败");
        phaseLabel->setText(states.join(" · "));
        taskProgress->setRange(0, 100);
        taskProgress->setValue(result.written ? 100 : 0);
        if (!result.success) taskProgress->setStyleSheet("QProgressBar::chunk{background:#eebc7e;}");
        if (result.success) QMessageBox::information(this,"刻录完成",result.message);
        else if (result.written) QMessageBox::warning(this,"写入完成，后续处理有警告",result.message);
        else QMessageBox::critical(this,"写入未完成",result.message);
        scan();
    });
    watcher->setFuture(QtConcurrent::run([this,work,options] {
        return df::burn(work,options,[this](const QString& text) {
            QMetaObject::invokeMethod(this,[this,text] { appendLog(text); },Qt::QueuedConnection);
        },[this](const df::Progress& progress) {
            QMetaObject::invokeMethod(this,[this,progress] { if (burning) updateProgress(progress); },Qt::QueuedConnection);
        });
    }));
}
void Window::updateProgress(const df::Progress& progress) {
    phaseLabel->setText(progress.stage + (progress.percent >= 0 ? QString(" · %1%").arg(progress.percent) : QString()));
    if (progress.percent < 0) taskProgress->setRange(0, 0);
    else {
        taskProgress->setRange(0, 100);
        taskProgress->setValue(qBound(0, progress.percent, 100));
    }
}
void Window::appendLog(const QString& text) {
    output->appendPlainText(QTime::currentTime().toString("HH:mm:ss")+"  "+text);
}
void Window::selectPage(int id) {
    stack->setCurrentIndex(id);
    navigation->button(id)->setChecked(true);
}
void Window::loadHistory() {
    const auto dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QFile file(dir+"/history.json");
    if(file.open(QIODevice::ReadOnly))history=QJsonDocument::fromJson(file.readAll()).array();
    refreshHistory();
}
void Window::saveHistory(const df::BurnResult& r,const df::Options& o) {
    QJsonObject item {
        {
            "time",QDateTime::currentDateTime().toString(Qt::ISODate)
        }, {
            "mode",o.finalize?"封盘":"追加"
        }, {
            "bytes",double(r.bytes)
        }, {
            "status",!r.written?"写入未完成":o.verify&&!r.verified?"写入完成，校验未完成":o.eject&&!r.ejected?(r.verified?"校验通过，弹出失败":"写入完成，弹出失败"):r.verified?"校验通过":"写入完成，未校验"
        }, {
            "message",r.message
        }, {
            "log",QJsonArray::fromStringList(r.log)
        }
    };
    item.insert("written", r.written);
    item.insert("verified", r.verified);
    item.insert("ejected", r.ejected);
    item.insert("verificationError", r.verificationError);
    item.insert("ejectError", r.ejectError);
    history.prepend(item);
    while(history.size()>200)history.removeLast();
    const auto dir=QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    QSaveFile f(dir+"/history.json");
    if(f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(history).toJson());
        if(!f.commit())appendLog("任务记录保存失败");
    }else appendLog("任务记录保存失败");
    refreshHistory();
}
void Window::refreshHistory() {
    if(!historyTable)return;
    historyTable->setRowCount(history.size());
    for(int i=0;i<history.size();i++) {
        auto o=history[i].toObject();
        QStringList cells {
            QDateTime::fromString(o["time"].toString(),Qt::ISODate).toString("MM-dd HH:mm"),o["mode"].toString(),formatted(qint64(o["bytes"].toDouble())),o["status"].toString()
        };
        for(int c=0;c<4;c++) {
            auto item=new QTableWidgetItem(cells[c]);
            item->setToolTip(o["message"].toString());
            historyTable->setItem(i,c,item);
        }
    }
}
void Window::applyAppearance() {
    QString accent=theme&&theme->currentData()=="cyan"?"#78d5c7":theme&&theme->currentData()=="amber"?"#eebc7e":"#72bdff";
    setStyleSheet(QString(R"(
 QLabel{background:transparent;}QFrame#device QWidget{background:transparent;}QFrame#device QComboBox{background:#101923;}QWidget{background:#10161e;color:#e2eaf3;font-family:"Noto Sans CJK SC","sans-serif";font-size:12px;} QMainWindow{background:#10161e;} QFrame#topbar{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #1d3045,stop:1 #17212d);border-bottom:1px solid #354458;}QLabel#brand{font-size:20px;font-weight:600;color:#d6ecff;background:transparent;}QLabel#title{font-size:20px;}QLabel#subtle{color:#98aabe;font-size:11px;background:transparent;}QLabel#capacity{font-size:18px;color:#d9edff;background:transparent;}QFrame#sidebar{background:#131c26;border-right:1px solid #2c3949;}QPushButton{border:1px solid #35495e;background:#1c2b3b;padding:6px 10px;border-radius:4px;}QPushButton:hover{background:#2b4359;border-color:%1;}QPushButton:disabled{color:#5f7285;border-color:#293542;background:#17212c;}QPushButton:checked{background:#224763;color:#d2ebff;border-bottom:2px solid %1;}QPushButton#nav{text-align:left;background:transparent;color:#a2b4c7;border:1px solid transparent;padding:12px;}QPushButton#nav:checked{background:#203c55;color:#d7edff;border:1px solid #365775;border-left:2px solid %1;}QPushButton#primary{background:%1;color:#0b1c2c;font-weight:600;padding:10px 22px;}QPushButton#primary:disabled{background:#22374b;color:#71889d;border:1px solid #34495d;}QFrame#panel,CapacityChart#panel{background:#151e29;border:1px solid #2f4153;border-radius:4px;}QFrame#device{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,stop:0 #1b344c,stop:1 #151e29);border:1px solid #3c5d78;border-radius:4px;}QWidget#panelhead,QLabel#panelhead{background:#182a3c;border-bottom:1px solid #2c3949;}QComboBox,QLineEdit{border:1px solid #3b4d61;border-radius:3px;background:#101923;padding:7px;selection-background-color:#365977;}QComboBox QAbstractItemView{background:#1b2b3c;selection-background-color:#2b5576;}QTableWidget{background:#151e29;border:0;selection-background-color:#23425d;gridline-color:#2c3949;}QTableWidget::item{padding:5px;border-bottom:1px solid #263646;}QHeaderView::section{background:#111b27;border:0;border-bottom:1px solid #2c3949;padding:8px;color:#8fa7bf;text-align:left;}QTableCornerButton::section{background:#111b27;border:0;}QCheckBox{spacing:8px;background:transparent;}QCheckBox::indicator{width:14px;height:14px;}QStatusBar{background:#141e2a;border-top:1px solid #2c3949;}QStatusBar::item{border:0;}QScrollArea{border:0;}QScrollBar:vertical{background:#111a24;width:9px;}QScrollBar::handle:vertical{background:#364f66;min-height:20px;}QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}QPlainTextEdit{background:#0d151e;border:1px solid #2c3949;color:#a4bfd5;}QDockWidget{color:#b2c9dd;}QFrame#taskPanel{background:#162a3d;border-bottom:1px solid #365573;}QProgressBar{background:#263c50;border:0;border-radius:2px;}QProgressBar::chunk{background:%1;border-radius:2px;}QToolTip{color:#e5eff8;background:#243b51;border:1px solid #6089a8;padding:5px;}QLabel{background:transparent;}
 )").arg(accent));
    fileTable->verticalHeader()->setDefaultSectionSize(prefs.value("compact",false).toBool()?30:40);
}
void Window::closeEvent(QCloseEvent* e) {
    if(busy) {
        QMessageBox::information(this,"正在处理","当前任务结束后才能关闭程序。");
        e->ignore();
        return;
    }e->accept();
}
void Window::dragEnterEvent(QDragEnterEvent* e) {
    if(!busy&&e->mimeData()->hasUrls())e->acceptProposedAction();
}
void Window::dropEvent(QDropEvent* e) {
    if(busy)return;
    QStringList paths;
    for(const auto& url:e->mimeData()->urls())if(url.isLocalFile())paths<<url.toLocalFile();
    if(!paths.isEmpty()) {
        selectPage(0);
        importPaths(paths);
        e->acceptProposedAction();
    }
}
bool Window::selfTest(const QString& path) {
    if (!path.isEmpty()) resize(1180, 920);
    disc.device="/dev/sr0";
    disc.family="cd";
    disc.profile="CD-R";
    disc.state="appendable";
    disc.message="可追加";
    disc.sessions=2;
    disc.total=700LL*1024*1024;
    disc.free=496LL*1024*1024;
    entries= {
        {
            "/tmp/实验数据","实验数据",182LL*1024*1024
        }, {
            "/tmp/项目技术报告","项目技术报告",26LL*1024*1024
        }
    };
    drive->addItem("光驱一","/dev/sr0");
    updateView();
    bool ok=burnButton->isEnabled()&&fileTable->rowCount()==2;
    const auto before=prefs.value("units","auto");
    for(const auto& setting:QStringList {
        "KB","MB","GB"
    }) {
        unit->setCurrentIndex(unit->findData(setting));
        ok=ok&&fileTable->item(0,1)->text().endsWith(setting);
    }unit->setCurrentIndex(unit->findData(before));
    selectPage(3);
    ok=ok&&stack->currentIndex()==3;
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
    QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);
    if(!path.isEmpty()) {
        taskPanel->hide();
        QApplication::processEvents();
        QPixmap image(size());
        render(&image);
        ok=image.save(path)&&ok;
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
    }return ok;
}
