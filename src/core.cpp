#include "core.h"
#include "iso_reader.h"
#include <sys/stat.h>
#include <sys/resource.h>
#include <stdexcept>
#include <limits>
#include <cerrno>
#include <unistd.h>
#include <QtDBus>
namespace df {
    static void fail(const QString& s) {
        throw std::runtime_error(s.toUtf8().constData());
    }
    QString executable(const QString& name) {
        const QString found=QStandardPaths::findExecutable(name);
        if(found.isEmpty()) fail("缺少依赖："+name);
        return found;
    }
    CommandResult command(const QString& program,const QStringList& args,int timeout,Log log) {
        QProcess p;
        QProcessEnvironment env=QProcessEnvironment::systemEnvironment();
        env.insert("LC_ALL","C.UTF-8");
        env.insert("LANG","C.UTF-8");
        p.setProcessEnvironment(env);
        p.setProcessChannelMode(QProcess::SeparateChannels);
        if (QFileInfo(program).fileName() == "wodim") {
            // cdrkit can successfully enable MCL_FUTURE under a finite
            // memlock budget and then fail later allocations in libusal.
            // Disable this optional optimization for this child only.
            // Do not grant capabilities, raise limits, or modify the parent.
            p.setChildProcessModifier([&p] {
                const struct rlimit disabled {0, 0};
                if (::setrlimit(RLIMIT_MEMLOCK, &disabled) != 0) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
                    p.failChildProcessModifier("disable optional memory locking", errno);
#else
                    // Qt 6.2/6.4 have no failure-reporting API for the modifier.
                    // Stop before exec rather than run wodim without isolation.
                    ::_exit(125);
#endif
                }
            });
        }
        p.start(executable(program),args);
        if(!p.waitForStarted(5000)) fail("无法启动 "+program+"："+p.errorString());
        QElapsedTimer time;
        time.start();
        CommandResult result;
        QByteArray pendingOut, pendingErr;
        auto emitLines = [&](QByteArray& pending, const QByteArray& bytes, bool flush = false) {
            pending += bytes;
            while (true) {
                const int cr = pending.indexOf('\r'), lf = pending.indexOf('\n');
                const int end = cr < 0 ? lf : lf < 0 ? cr : qMin(cr, lf);
                if (end < 0) break;
                QString line = QString::fromUtf8(pending.left(end));
                line.remove(QRegularExpression("[\\x{0000}-\\x{0008}\\x{000b}\\x{000c}\\x{000e}-\\x{001f}]"));
                if (log && !line.trimmed().isEmpty()) log(line);
                pending.remove(0, end + 1);
            }
            if ((flush || pending.size() > 65536) && !pending.isEmpty()) {
                if (log) log(QString::fromUtf8(pending));
                pending.clear();
            }
        };
        while (p.state() != QProcess::NotRunning) {
            p.waitForReadyRead(100);
            auto a = p.readAllStandardOutput(), b = p.readAllStandardError();
            result.out += a;
            result.err += b;
            emitLines(pendingOut, a);
            emitLines(pendingErr, b);
            if (timeout > 0 && time.elapsed() > timeout) {
                p.kill();
                p.waitForFinished();
                result.timedOut = true;
                break;
            }
        }
        auto a = p.readAllStandardOutput(), b = p.readAllStandardError();
        result.out += a;
        result.err += b;
        emitLines(pendingOut, a, true);
        emitLines(pendingErr, b, true);
        result.code=result.timedOut?-1:(p.exitStatus()==QProcess::NormalExit?p.exitCode():-1);
        return result;
    }
    static CommandResult checked(const QString& p,const QStringList& args,int timeout=20000,Log log= {
    }) {
        auto r=command(p,args,timeout,log);
        if(r.code!=0)fail(p+" 执行失败："+r.text().right(1800));
        return r;
    }
    static qint64 number(const QString& s,const QString& pattern,int group=1) {
        auto m=QRegularExpression(pattern,QRegularExpression::CaseInsensitiveOption).match(s);
        return m.hasMatch()?m.captured(group).toLongLong():-1;
    }
    QString Disc::chineseType() const {
        if(family=="cd")return profile.contains("RW")?"可擦写光盘":"普通光盘";
        if(family=="dvd")return profile.contains("RW")||profile.contains("RAM")?"高密度可擦写光盘":"高密度光盘";
        return "未识别介质";
    }
    Disc parseCd(const QString& atip,const QString& toc,const QString& ms) {
        Disc d;
        d.family="cd";
        d.profile=atip.contains("CD-RW",Qt::CaseInsensitive)?"CD-RW":"CD-R";
        d.raw=atip+'\n'+toc+'\n'+ms;
        const auto lower=d.raw.toLower();
        if(lower.contains("no medium")||lower.contains("medium not present")||lower.contains("no disk / wrong disk")) {
            d.state="empty";
            d.message="未插入光盘";
            return d;
        }
        if(lower.contains("permission denied")||lower.contains("device or resource busy")||lower.contains("seems to be mounted")) {
            d.message="设备权限不足或光盘被占用";
            return d;
        }
        qint64 cap=number(atip,"ATIP start of lead out:\\s*(\\d+)");
        if(cap<0)cap=number(d.raw,"blocks total:\\s*(\\d+)");
        if(cap>0)d.total=cap*2048;
        const qint64 remain=number(d.raw,"blocks remaining:\\s*(\\d+)");
        if(remain>=0)d.free=remain*2048;
        const auto match=QRegularExpression("^\\s*(\\d+),(\\d+)\\s*$").match(ms.trimmed());
        if(match.hasMatch()) {
            d.msinfo=match.captured(1)+','+match.captured(2);
            d.nextSector=match.captured(2).toLongLong();
            d.state="appendable";
            if(d.free<0&&d.total>0)d.free=qMax<qint64>(0,d.total-d.nextSector*2048);
            QRegularExpression re("session\\s+(\\d+)",QRegularExpression::CaseInsensitiveOption);
            auto it=re.globalMatch(toc);
            while(it.hasNext())d.sessions=qMax(d.sessions,it.next().captured(1).toInt());
            d.sessions=qMax(1,d.sessions);
            d.message="可追加";
        } else {
            const bool dataToc=QRegularExpression("track:.*lba:",QRegularExpression::CaseInsensitiveOption).match(toc).hasMatch()||toc.contains("last session start",Qt::CaseInsensitive);
            const bool explicitBlank=lower.contains("disk is blank")||lower.contains("disc is blank")||lower.contains("empty disk")||(d.total>0&&d.free==d.total);
            const bool emptyToc=toc.contains("Cannot read TOC",Qt::CaseInsensitive)&&!dataToc&&atip.contains("ATIP info from disk",Qt::CaseInsensitive);
            if((explicitBlank||emptyToc)&&d.total>0) {
                d.state="blank";
                d.free=d.total;
                d.message="空白光盘";
            }
            else if(dataToc) {
                d.state="closed";
                d.free=0;
                d.message="已封盘或不可追加";
            }
            else d.message="介质状态未知，禁止写入";
        }
        if(d.total<0||d.free<0) {
            d.state="unknown";
            d.message="无法确定光盘剩余容量";
        }
        return d;
    }
    Disc parseDvd(const QString& text) {
        Disc d;
        d.family="dvd";
        d.raw=text;
        if(text.contains("no media mounted",Qt::CaseInsensitive)) {
            d.state="empty";
            d.message="未插入光盘";
            return d;
        }
        auto m=QRegularExpression("Mounted Media:\\s*\\S+,\\s*([^\\r\\n]+)").match(text);
        if(!m.hasMatch()) {
            d.message="未识别高密度介质";
            return d;
        }d.profile=m.captured(1).trimmed();
        if(!d.profile.startsWith("DVD")) {
            d.message="暂不支持此介质";
            return d;
        }
        const auto status=QRegularExpression("Disc status:\\s*(\\S+)",QRegularExpression::CaseInsensitiveOption).match(text).captured(1).toLower();
        qint64 cap=number(text,"READ CAPACITY:\\s*(\\d+)\\*2048");
        if(cap<0)cap=number(text,"Legacy lead-out at:?\\s*(\\d+)");
        qint64 free=number(text,"Free Blocks:\\s*(\\d+)");
        if(cap>0)d.total=cap*2048;
        if(free>=0)d.free=free*2048;
        if(status=="blank") {
            d.state="blank";
            if(d.total<0)d.total=d.free;
            if(d.free<0)d.free=d.total;
            d.message="空白光盘";
        }
        else if(status.startsWith("append")) {
            d.state="appendable";
            d.message="可追加";
            d.sessions=1;
        }
        else {
            d.state="closed";
            d.message="已封盘或不支持追加";
        }
        // Overwrite media may report complete while remaining growable. Calculate used space from ISO PVD during inspect.
        if(d.profile.contains("DVD+RW")||d.profile.contains("DVD-RAM")||text.contains("Restricted Overwrite",Qt::CaseInsensitive)) {
            if(status!="blank") {
                d.state="overwrite";
                d.message="正在读取可擦写介质容量";
            }
        }
        auto si=QRegularExpression("Session\\s+(\\d+)").globalMatch(text);
        while(si.hasNext())d.sessions=qMax(d.sessions,si.next().captured(1).toInt());
        if(d.total<0||d.free<0) {
            if(d.state!="overwrite") {
                d.state="unknown";
                d.message="无法确定光盘剩余容量";
            }
        }
        return d;
    }
    QStringList devices() {
        QStringList list;
        QDir dir("/sys/class/block");
        for(const auto& s:dir.entryList( {
            "sr*"
        },QDir::Dirs|QDir::NoDotAndDotDot))list<<"/dev/"+s;
        return list;
    }
    Disc inspect(const QString& device) {
        if(!QRegularExpression("^/dev/sr[0-9]+$").match(device).hasMatch())fail("设备必须为系统光驱");
        Disc d;
        d.device=device;
        if(!QFileInfo::exists(device)) {
            d.state="empty";
            d.message="未检测到光驱";
            return d;
        }
        const auto mediaStatus = opticalMediaStatus(device);
        if (!mediaStatus.isEmpty()) {
            d.state = "empty";
            d.message = mediaStatus;
            return d;
        }
        struct MountRestore {
            QString device;
            bool restore=false;
            ~MountRestore() {
                if(restore) {
                    try {
                        mountDisc(device);
                    }catch(...) {
                    }
                }
            }
        } guard {
            device,!mountPoint(device).isEmpty()
        };
        if(guard.restore)unmountDisc(device);
        auto r=command("dvd+rw-mediainfo", {
            device
        });
        if(r.text().contains("Mounted Media:")&&!r.text().contains("non-DVD media mounted",Qt::CaseInsensitive)) {
            d=parseDvd(r.text());
            if(d.state=="overwrite") {
                QFile f(device);
                if(f.open(QIODevice::ReadOnly)&&f.seek(16*2048)) {
                    auto pvd=f.read(2048);
                    if(pvd.size()==2048&&pvd.mid(1,5)=="CD001") {
                        quint32 blocks=qFromLittleEndian<quint32>(pvd.constData()+80);
                        if(d.total>0&&qint64(blocks)*2048<=d.total) {
                            d.free=d.total-qint64(blocks)*2048;
                            d.state="appendable";
                            d.message="可追加";
                            d.sessions=1;
                        }
                    }
                }
                if(d.state=="overwrite") {
                    d.state="unknown";
                    d.message="无法安全确定可擦写光盘状态";
                }
            }
        } else {
            auto a=command("wodim", {
                "dev="+device,"-atip"
            });
            auto t=command("wodim", {
                "dev="+device,"-toc"
            });
            auto m=command("wodim", {
                "dev="+device,"-msinfo"
            });
            d=parseCd(a.text(),t.text(),QString::fromUtf8(m.out));
        }
        d.device=device;
        return d;
    }
    QString formatBytes(qint64 b,const QString& setting) {
        if(b<0)return "—";
        QString u=setting;
        if(u=="auto") {
            u=b>=qint64(1024)*1024*1024?"GB":b>=1024*1024?"MB":"KB";
        }double factor=u=="GB"?1073741824.0:u=="MB"?1048576.0:1024.0;
        return QString::number(double(b)/factor,'f',u=="KB"?1:2)+" "+u;
    }
    static void validateName(const QString& name) {
        if(name.isEmpty()||name=="."||name==".."||name.contains('\n')||name.contains('\r')||name.contains('/')||name.contains('\\'))fail("不支持此文件名："+name);
        if(name.contains(';'))fail("文件名包含光盘目录不保留的分号，请重命名："+name);
        for (const auto c : name) if (c.isSurrogate()) fail("文件名包含中文目录不支持的字符，请重命名："+name);
        if(name.size()>100)fail("文件名超过兼容长度："+name);
        if(name.endsWith('.')||name.endsWith(' ')||name.contains(QRegularExpression("[<>:\"|?*\\x{0000}-\\x{001f}]")))fail("文件名不兼容 Windows："+name);
        const QString stem=name.section('.',0,0).toUpper();
        if(QRegularExpression("^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])$").match(stem).hasMatch())fail("文件名为系统保留名称："+name);
    }
    static qint64 treeSize(const QString& path) {
        QFileInfo fi(path);
        if(fi.isSymLink())fail("请移除符号链接："+path);
        if(fi.isFile()) {
            if(!fi.isReadable())fail("文件不可读："+path);
            return fi.size();
        }if(!fi.isDir())fail("不支持的文件类型："+path);
        qint64 n=0;
        QDirIterator it(path,QDir::Files|QDir::Dirs|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot,QDirIterator::Subdirectories);
        while(it.hasNext()) {
            it.next();
            auto f=it.fileInfo();
            validateName(f.fileName());
            if(f.isSymLink())fail("请移除符号链接："+f.filePath());
            if(f.isFile()) {
                if(!f.isReadable())fail("文件不可读："+f.filePath());
                n+=f.size();
            }else if(!f.isDir())fail("不支持的文件类型："+f.filePath());
        }return n;
    }
    Entry entry(const QString& p) {
        QFileInfo fi(p);
        validateName(fi.fileName());
        return {
            fi.absoluteFilePath(),fi.fileName(),treeSize(fi.absoluteFilePath())
        };
    }
    static void copyTree(const QString& source,const QString& dest) {
        QFileInfo fi(source);
        if(fi.isSymLink())fail("复制时检测到符号链接");
        if(fi.isDir()) {
            if(!QDir().mkpath(dest))fail("无法创建缓存目录");
            QSet<QString> names;
            for(const auto& item:QDir(source).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System)) {
                validateName(item.fileName());
                const auto key=item.fileName().toCaseFolded();
                if(names.contains(key))fail("目录内存在大小写重名："+item.fileName());
                names.insert(key);
                copyTree(item.absoluteFilePath(),dest+'/'+item.fileName());
            }
        }else if(fi.isFile()) {
            if(!QFile::copy(source,dest))fail("无法缓存文件："+source);
        }else fail("源文件已改变："+source);
    }
    void stageEntries(const QList<Entry>& entries,const QString& root) {
        QSet<QString> names;
        for(const auto& e:entries) {
            validateName(e.name);
            if(names.contains(e.name.toCaseFolded()))fail("存在重名文件或目录："+e.name);
            names.insert(e.name.toCaseFolded());
            entry(e.path);
            copyTree(e.path,root+'/'+e.name);
        }
    }
    Manifest manifest(const QString& root) {
        Manifest result;
        QDir dir(root);
        QDirIterator it(root,QDir::Files|QDir::Hidden|QDir::System,QDirIterator::Subdirectories);
        while(it.hasNext()) {
            auto path=it.next();
            if(it.fileInfo().isSymLink())fail("不支持光盘中的符号链接");
            QFile f(path);
            if(!f.open(QIODevice::ReadOnly))fail("读取失败："+path);
            QCryptographicHash h(QCryptographicHash::Sha256);
            if(!h.addData(&f))fail("计算校验失败："+path);
            result.insert(dir.relativeFilePath(path),h.result());
        }return result;
    }
    QString verifyManifest(const QString& root, const Manifest& expected, ProgressCallback progress) {
        qint64 total = 0, done = 0;
        for (auto it = expected.cbegin(); it != expected.cend(); ++it)
            total += qMax<qint64>(0, QFileInfo(root + '/' + it.key()).size());
        QElapsedTimer timer;
        timer.start();
        if (progress) progress({"校验文件", total > 0 ? 0 : -1});
        for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
            QFile f(root + '/' + it.key());
            if (!f.open(QIODevice::ReadOnly)) return "光盘缺失文件：" + it.key();
            QCryptographicHash hash(QCryptographicHash::Sha256);
            while (!f.atEnd()) {
                const auto bytes = f.read(1024 * 1024);
                if (bytes.isEmpty() && f.error() != QFile::NoError) return "光盘读取失败：" + it.key();
                hash.addData(bytes);
                done += bytes.size();
                if (progress && timer.elapsed() >= 100) {
                    progress({"校验文件", total > 0 ? int(qMin<qint64>(99, done * 100 / total)) : -1});
                    timer.restart();
                }
            }
            if (hash.result() != it.value()) return "文件校验失败：" + it.key();
        }
        if (progress) progress({"校验文件", 100});
        return {};
    }
    bool parseToolProgress(const QString& text, Progress& progress) {
        if (text.contains("Fixating", Qt::CaseInsensitive) || text.contains("writing lead-out", Qt::CaseInsensitive)) {
            progress = {"关闭区段", -1};
            return true;
        }
        if (text.contains("Performing OPC")) {
            progress = {"校准光驱", -1};
            return true;
        }
        auto match = QRegularExpression("Track\\s+\\d+:\\s*([0-9.]+)\\s+of\\s+([0-9.]+)\\s+MB\\s+written", QRegularExpression::CaseInsensitiveOption).match(text);
        if (match.hasMatch() && match.captured(2).toDouble() > 0) {
            progress = {"写入数据", qBound(0, int(match.captured(1).toDouble() * 100 / match.captured(2).toDouble()), 100)};
            return true;
        }
        match = QRegularExpression("([0-9]+(?:\\.[0-9]+)?)%\\s+done").match(text);
        if (match.hasMatch()) {
            progress = {"构建映像", qBound(0, int(match.captured(1).toDouble()), 100)};
            return true;
        }
        match = QRegularExpression("\\(\\s*([0-9]+(?:\\.[0-9]+)?)%\\)").match(text);
        if (match.hasMatch()) {
            progress = {"写入数据", qBound(0, int(match.captured(1).toDouble()), 100)};
            return true;
        }
        return false;
    }
    QStringList isoArguments(const QString& root,const QString& image,const QString& label,const Disc& d) {
        QStringList a= {
            "-R","-J","-joliet-long","-input-charset","utf-8","-V",label,"-o",image
        };
        if(!d.blank()&&d.family=="cd") {
            if(!QRegularExpression("^\\d+,\\d+$").match(d.msinfo).hasMatch())fail("缺少有效多区段偏移");
            a<<"-C"<<d.msinfo<<"-M"<<d.device;
        }a<<root;
        return a;
    }
    qint64 cdFifoBytes(quint64 memlockLimit) {
        // Keep at least half of the lock budget for wodim itself and its
        // transfer buffers. Below 2 MiB use the prebuilt file directly.
        constexpr quint64 mib = 1024 * 1024;
        if (memlockLimit < 2 * mib) return 0;
        return qint64(qMin<quint64>(4 * mib, memlockLimit / 2) / 65536 * 65536);
    }
    QStringList cdArguments(const QString& image,const Options& o) {
        struct rlimit lockLimit {};
        quint64 budget = 0;
        if (getrlimit(RLIMIT_MEMLOCK, &lockLimit) == 0) {
            budget = lockLimit.rlim_cur == RLIM_INFINITY
                ? std::numeric_limits<quint64>::max() : quint64(lockLimit.rlim_cur);
        }
        QStringList a= {
            "dev="+o.device,"fs="+QString::number(cdFifoBytes(budget)),"-v","-tao","-data","-pad"
        };
        if(!o.finalize)a<<"-multi";
        if(o.speed>0)a<<"speed="+QString::number(o.speed);
        a<<image;
        return a;
    }
    QStringList dvdArguments(const QString& root,const Options& o,const Disc& d) {
        QStringList a;
        if(o.finalize)a<<"-dvd-compat";
        if(o.speed>0)a<<"-speed="+QString::number(o.speed);
        a<<(d.blank()?"-Z":"-M")<<o.device<<"-R"<<"-J"<<"-joliet-long"<<"-input-charset"<<"utf-8"<<"-V"<<o.label<<root;
        return a;
    }
    QString mountPoint(const QString& device) {
        auto r=command("findmnt", {
            "-J","-S",device,"-o","TARGET"
        });
        auto a=QJsonDocument::fromJson(r.out).object().value("filesystems").toArray();
        return a.isEmpty()?QString():a.first().toObject().value("target").toString();
    }
    void rescanDisc(const QString& device, Log log) {
        if (!QRegularExpression("^/dev/sr[0-9]+$").match(device).hasMatch()) fail("设备必须为系统光驱");
        QDBusInterface block("org.freedesktop.UDisks2", "/org/freedesktop/UDisks2/block_devices/" + device.section('/', -1),
                             "org.freedesktop.UDisks2.Block", QDBusConnection::systemBus());
        block.setTimeout(5000);
        const QVariantMap options{{"auth.no_user_interaction", true}};
        const auto reply = block.call("Rescan", options);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            if (log) log("设备刷新未完成，继续等待介质就绪：" + reply.errorMessage());
        } else if (log) log("已请求重新识别光盘文件系统");
    }
    QString mountWithRetry(const MountOps& ops, Log log, int attempts) {
        QString lastError;
        for (int attempt = 0; attempt < qMax(1, attempts); ++attempt) {
            auto point = ops.current();
            if (!point.isEmpty()) return point;
            if (attempt == 0 && ops.rescan) ops.rescan();
            const auto result = ops.mount();
            point = ops.current(); // Desktop automount can win the race.
            if (!point.isEmpty()) return point;
            lastError = result.text().trimmed();
            const auto lower = lastError.toLower();
            const bool retryable = result.code == 0 || lower.contains("not a mountable filesystem") ||
                lower.contains("not ready") || lower.contains("no medium") || lower.contains("no media") ||
                lower.contains("not found") || lower.contains("already mounted");
            if (!retryable) fail("挂载光盘失败：" + lastError);
            if (attempt + 1 >= qMax(1, attempts)) break;
            if (log) log(QString("等待光盘文件系统就绪（%1/%2）").arg(attempt + 1).arg(attempts));
            if (ops.pause) ops.pause(2000);
        }
        fail("等待光盘文件系统超时，未完成校验。可重新插入光盘后检查文件。\n" + lastError);
        return {};
    }
    QString mountDisc(const QString& device, Log log) {
        MountOps ops;
        ops.current = [device] { return mountPoint(device); };
        ops.rescan = [device, log] { rescanDisc(device, log); };
        ops.mount = [device] { return command("udisksctl", {"mount", "-b", device, "-o", "ro", "--no-user-interaction"}, 10000); };
        ops.pause = [](int milliseconds) { QThread::msleep(milliseconds); };
        return mountWithRetry(ops, log);
    }
    void finishWrittenDisc(BurnResult& result, const Options& options, const PostWriteOps& ops, Log log, ProgressCallback progress) {
        if (!result.written) return;
        if (options.verify) {
            if (progress) progress({"等待光盘就绪", -1});
            try {
                ops.verify();
                result.verified = true;
                if (log) log("原有文件和新增文件校验通过");
            } catch (const std::exception& error) {
                result.verificationError = QString::fromUtf8(error.what());
                if (log) log("写入已完成，但校验未完成：" + result.verificationError);
            }
        }
        // Ejection is independent of verification. Never rewrite on a verification error.
        if (options.eject) {
            if (progress) progress({"弹出光盘", -1});
            try {
                ops.eject();
                result.ejected = true;
                if (log) log("光盘已弹出");
            } catch (const std::exception& error) {
                result.ejectError = QString::fromUtf8(error.what());
                if (log) log("弹出光盘失败：" + result.ejectError);
            }
        }
        result.success = (!options.verify || result.verified) && (!options.eject || result.ejected);
        QStringList summary{"数据写入完成"};
        summary << (options.verify ? (result.verified ? "文件校验通过" : "校验未完成：" + result.verificationError) : "未执行校验");
        if (options.eject) summary << (result.ejected ? "光盘已弹出" : "弹出失败：" + result.ejectError);
        result.message = summary.join('\n');
        if (log) log(result.message);
    }
    void unmountDisc(const QString& device) {
        if(!mountPoint(device).isEmpty())checked("udisksctl", {
            "unmount","-b",device
        },30000);
    }
    void createImage(const QString& root,const QString& image,const QString& label,Log log) {
        Disc d;
        d.state="blank";
        checked("genisoimage",isoArguments(root,image,label,d),0,log);
    }
    BurnResult burn(const QList<Entry>& entries,const Options& options,Log callback,ProgressCallback progress) {
        BurnResult result;
        auto log=[&](const QString& s) {
            result.log<<s;
            if(callback)callback(s);
            Progress parsed;
            if(progress && parseToolProgress(s, parsed)) progress(parsed);
        };
        auto phase = [&](const QString& stage) { if(progress) progress({stage, -1}); };
        bool restoreMount=false;
        bool unmounted=false;
        try {
            if(entries.isEmpty())fail("未添加文件");
            if(options.label.trimmed().isEmpty()||options.label.toUtf8().size()>32)fail("光盘名称不能为空且须在32字节以内");
            phase("检查光盘");
            auto d=inspect(options.device);
            if(!d.writable())fail(d.message.isEmpty()?"介质不可写入":d.message);
            // Fingerprint disc metadata again immediately before write to catch a tray swap.
            const QString before=d.raw;
            const QString priorMount=mountPoint(options.device);
            restoreMount=!priorMount.isEmpty();
            QTemporaryDir temp(QDir::tempPath()+"/discforge-XXXXXX");
            if(!temp.isValid())fail("无法建立刻录缓存");
            const QString root=temp.path()+"/data";
            QDir().mkpath(root);
            qint64 inputBytes=0;
            for(const auto& e:entries)inputBytes+=entry(e.path).bytes;
            QStorageInfo storage(temp.path());
            if(storage.bytesAvailable()<inputBytes*2+64*1024*1024)fail("临时目录空间不足");
            phase("缓存源文件");
            log("正在缓存源文件");
            stageEntries(entries,root);
            auto incoming=manifest(root);
            Manifest expected;
            if(!d.blank()) {
                phase("读取原有文件");
                log("正在校验原有文件");
                const QString oldRoot=mountDisc(options.device,log);
                expected=manifest(oldRoot);
                QSet<QString> oldNames;
                for(const auto& n:QDir(oldRoot).entryList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden))oldNames.insert(n.toCaseFolded());
                for(const auto& e:entries)if(oldNames.contains(e.name.toCaseFolded()))fail("光盘已存在同名文件或目录："+e.name);
            }
            for(auto it=incoming.cbegin();it!=incoming.cend();++it)expected.insert(it.key(),it.value());
            unmountDisc(options.device);
            unmounted=true;
            auto current=inspect(options.device);
            if(current.raw!=before)fail("光盘状态已改变，请刷新后重试");
            if(d.family=="cd") {
                const QString iso=temp.path()+"/session.iso";
                phase("构建映像");
                log("正在构建兼容映像");
                checked("genisoimage",isoArguments(root,iso,options.label,d),0,log);
                QFile image(iso);
                if(!image.open(QIODevice::ReadOnly))fail("映像读取失败");
                result.bytes=image.size();
                if(result.bytes+16*1024*1024>d.free)fail("映像与多区段开销超过剩余空间");
                // Output file keeps all diagnostic bytes out of ISO payload; no stdout pipeline.
                const auto args = cdArguments(iso,options);
                log("刻录缓冲区：" + args[1].mid(3) + " 字节");
                log("已仅对刻录子进程禁用可选的内存锁定");
                phase("写入数据");
                log("开始写入光盘");
                checked("wodim",args,0,log);
                result.written=true;
            }else {
                const auto estimate=checked("genisoimage", {
                    "-quiet","-print-size","-R","-J","-joliet-long","-input-charset","utf-8",root
                },0);
                bool ok=false;
                const auto blocks=QString::fromUtf8(estimate.out).trimmed().toLongLong(&ok);
                if(!ok||blocks<=0)fail("无法计算映像容量");
                result.bytes=blocks*2048;
                if(result.bytes+16*1024*1024>d.free)fail("映像与区段开销超过剩余空间");
                phase("写入数据");
                log("开始写入光盘");
                checked("growisofs",dvdArguments(root,options,d),0,log);
                result.written=true;
            }
            PostWriteOps post;
            post.verify = [&] {
                log("正在重新读取并校验全部文件");
                unmountDisc(options.device);
                QString error;
                if (d.family == "cd") {
                    log("直接读取最新区段，校验不依赖系统挂载");
                    log("校验区段起始扇区：" + QString::number(d.nextSector));
                    error = verifyCd(options.device, quint32(d.nextSector), expected, progress);
                } else {
                    const auto mounted = mountDisc(options.device, log);
                    error = verifyManifest(mounted, expected, progress);
                }
                if (!error.isEmpty()) fail(error);
            };
            post.eject = [&] {
                unmountDisc(options.device);
                checked("eject", {options.device}, 30000);
                unmounted = false;
            };
            finishWrittenDisc(result, options, post, log, progress);
            if(restoreMount&&!options.eject&&!options.verify) {
                mountDisc(options.device);
                unmounted=false;
            }
        }catch(const std::exception& e) {
            result.success=false;
            result.message=QString::fromUtf8(e.what());
            log(result.message);
            if(restoreMount&&unmounted) {
                try {
                    mountDisc(options.device);
                }catch(const std::exception& x) {
                    log(QString::fromUtf8(x.what()));
                }
            }
        }
        return result;
    }
}
