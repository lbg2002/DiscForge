#include "core.h"
#include "iso_reader.h"
#include <QtTest>
#include <stdexcept>
#include <limits>
#include <sys/resource.h>
class CoreTests:public QObject {
    Q_OBJECT
    private slots:
    void units() {
        QCOMPARE(df::formatBytes(1024,"KB"),QString("1.0 KB"));
        QCOMPARE(df::formatBytes(1048576,"MB"),QString("1.00 MB"));
        QCOMPARE(df::formatBytes(1073741824,"GB"),QString("1.00 GB"));
        QCOMPARE(df::formatBytes(1073741824,"auto"),QString("1.00 GB"));
        QCOMPARE(df::formatBytes(-1,"MB"),QString("—"));
    }
    void cdDetection() {
        auto d=df::parseCd("ATIP info from disk:\nATIP start of lead out: 359849", "track: 1 lba: 0\nsession 2", "12345,25678\n");
        QVERIFY(d.writable());
        QCOMPARE(d.nextSector,qint64(25678));
        QCOMPARE(d.sessions,2);
        QCOMPARE(d.free,(359849LL-25678)*2048);
        auto b=df::parseCd("ATIP info from disk:\nATIP start of lead out: 359849", "Cannot read TOC header", "Cannot read session offset");
        QVERIFY(b.blank());
        QCOMPARE(b.free,b.total);
        auto closed=df::parseCd("ATIP info from disk:\nATIP start of lead out: 359849", "track: 1 lba: 0", "Cannot read session offset");
        QCOMPARE(closed.state,QString("closed"));
        QVERIFY(!closed.writable());
        auto unknown=df::parseCd("ATIP info from disk:\nManufacturer: test", "unrecognized", "");
        QVERIFY(!unknown.writable());
        QVERIFY(!unknown.blank());
        auto empty=df::parseCd("no medium found", "", "");
        QCOMPARE(empty.state,QString("empty"));
    }
    void dvdDetection() {
        auto d=df::parseDvd("Mounted Media: 1Bh, DVD+R\nDisc status: appendable\nREAD CAPACITY: 2295104*2048\nFree Blocks: 2000000*2KB\nSession 2");
        QVERIFY(d.writable());
        QCOMPARE(d.free,2000000LL*2048);
        QCOMPARE(d.sessions,2);
        auto blank=df::parseDvd("Mounted Media: 1Bh, DVD+R\nDisc status: blank\nFree Blocks: 2295104*2KB");
        QVERIFY(blank.blank());
        QVERIFY(blank.writable());
        auto closed=df::parseDvd("Mounted Media: 1Bh, DVD+R\nDisc status: complete\nREAD CAPACITY: 2295104*2048\nFree Blocks: 0*2KB");
        QVERIFY(!closed.writable());
        auto unsupported=df::parseDvd("Mounted Media: 43h, BD-RE\nDisc status: blank\nFree Blocks: 9000000*2KB");
        QVERIFY(!unsupported.writable());
    }
    void commands() {
        df::Disc d;
        d.family="cd";
        d.state="appendable";
        d.device="/dev/sr0";
        d.msinfo="0,20000";
        auto iso=df::isoArguments("/tmp/root","/tmp/out.iso","研究资料",d);
        QVERIFY(iso.contains("-o"));
        QVERIFY(iso.contains("-C"));
        QVERIFY(iso.contains("-M"));
        QVERIFY(iso.contains("-J"));
        QVERIFY(!iso.contains("-graft-points"));
        d.msinfo="bad";
        QVERIFY_EXCEPTION_THROWN(df::isoArguments("a","b","c",d),std::runtime_error);
        df::Options o;
        o.device="/dev/sr0";
        o.label="研究资料";
        auto cd=df::cdArguments("/tmp/image.iso",o);
        QVERIFY(cd.contains("-multi"));
        QVERIFY(cd.contains("-tao"));
        QVERIFY(cd[1].startsWith("fs="));
        QVERIFY(cd[1].mid(3).toLongLong() <= 4LL * 1024 * 1024);
        QCOMPARE(cd.last(),QString("/tmp/image.iso"));
        QVERIFY(!cd.contains("-"));
        o.finalize=true;
        QVERIFY(!df::cdArguments("image",o).contains("-multi"));
        d.family="dvd";
        d.state="appendable";
        QVERIFY(df::dvdArguments("root",o,d).contains("-M"));
        QVERIFY(df::dvdArguments("root",o,d).contains("-dvd-compat"));
        d.state="blank";
        QVERIFY(df::dvdArguments("root",o,d).contains("-Z"));
    }
    void fifoBudget() {
        QCOMPARE(df::cdFifoBytes(8ULL*1024*1024), 4LL*1024*1024);
        QCOMPARE(df::cdFifoBytes(4ULL*1024*1024), 2LL*1024*1024);
        QCOMPARE(df::cdFifoBytes(64ULL*1024), 0LL);
        QCOMPARE(df::cdFifoBytes(0), 0LL);
        QCOMPARE(df::cdFifoBytes(std::numeric_limits<quint64>::max()), 4LL*1024*1024);
    }
    void lowMemlockCommand() {
        struct rlimit original {};
        QVERIFY(getrlimit(RLIMIT_MEMLOCK, &original) == 0);
        struct rlimit low = original;
        low.rlim_cur = qMin<rlim_t>(original.rlim_cur, 64 * 1024);
        QVERIFY(setrlimit(RLIMIT_MEMLOCK, &low) == 0);
        df::Options options;
        options.device = "/dev/sr0";
        const auto args = df::cdArguments("/tmp/image.iso", options);
        const bool restored = setrlimit(RLIMIT_MEMLOCK, &original) == 0;
        QVERIFY(restored);
        QVERIFY(args.contains("fs=0"));
    }
    void wodimLockIsolation() {
        struct rlimit before {};
        QVERIFY(getrlimit(RLIMIT_MEMLOCK, &before) == 0);
        QTemporaryDir directory;
        const QString path = directory.path() + "/wodim";
        QFile fake(path);
        QVERIFY(fake.open(QIODevice::WriteOnly));
        fake.write("#!/bin/sh\nulimit -Sl\nulimit -Hl\n");
        fake.close();
        QVERIFY(fake.setPermissions(QFile::ReadOwner|QFile::WriteOwner|QFile::ExeOwner));
        const auto result = df::command(path, {});
        QCOMPARE(result.code, 0);
        QCOMPARE(result.out.trimmed(), QByteArray("0\n0"));
        struct rlimit after {};
        QVERIFY(getrlimit(RLIMIT_MEMLOCK, &after) == 0);
        QCOMPARE(after.rlim_cur, before.rlim_cur);
        QCOMPARE(after.rlim_max, before.rlim_max);
        const auto other = df::command("sh", {"-c", "ulimit -Sl"});
        QCOMPARE(other.code, 0);
        if (before.rlim_cur != RLIM_INFINITY)
            QCOMPARE(other.out.trimmed().toULongLong(), quint64(before.rlim_cur / 1024));
    }
    void lateAllocationRegression() {
        const QString probe = QCoreApplication::applicationDirPath() + "/lock_probe";
        struct rlimit limit {};
        QVERIFY(getrlimit(RLIMIT_MEMLOCK, &limit) == 0);
        if (limit.rlim_max != RLIM_INFINITY && limit.rlim_max < 8388608)
            QSKIP("The baseline requires an 8 MiB hard limit");
        const auto baseline = df::command("prlimit", {"--memlock=8388608:8388608", "--", probe});
        QCOMPARE(baseline.code, 2);
        QVERIFY(baseline.out.contains("locked=yes allocated=no"));
        QTemporaryDir directory;
        const QString child = directory.path() + "/wodim";
        QVERIFY(QFile::copy(probe, child));
        const auto fixed = df::command(child, {});
        QCOMPARE(fixed.code, 0);
        QVERIFY(fixed.out.contains("locked=no allocated=yes"));
    }
    void stageAndVerify() {
        QTemporaryDir t;
        QDir().mkpath(t.path()+"/source/实验数据");
        QFile f(t.path()+"/source/实验数据/结果 $(x).txt");
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("payload\n");
        f.close();
        auto e=df::entry(t.path()+"/source/实验数据");
        QCOMPARE(e.bytes,qint64(8));
        QString root=t.path()+"/staged";
        QDir().mkpath(root);
        df::stageEntries( {
            e
        },root);
        auto expected=df::manifest(root);
        QCOMPARE(expected.size(),1);
        QVERIFY(df::verifyManifest(root,expected).isEmpty());
        QFile modified(root+"/实验数据/结果 $(x).txt");
        QVERIFY(modified.open(QIODevice::WriteOnly|QIODevice::Truncate));
        modified.write("corrupt");
        modified.close();
        QVERIFY(!df::verifyManifest(root,expected).isEmpty());
        QVERIFY_EXCEPTION_THROWN(df::stageEntries( {
            e,e
        },t.path()+"/duplicate"),std::runtime_error);
        QFile::link(f.fileName(),t.path()+"/link");
        QVERIFY_EXCEPTION_THROWN(df::entry(t.path()+"/link"),std::runtime_error);
    }
    void windowsNames() {
        QTemporaryDir t;
        for(const auto& name:QStringList {
            "CON.txt","bad:name","trailing.","version.txt;1","version;1.txt"
        }) {
            QFile f(t.path()+"/"+name);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.close();
            QVERIFY_EXCEPTION_THROWN(df::entry(f.fileName()),std::runtime_error);
        }QDir().mkpath(t.path()+"/source");
        for(const auto& name:QStringList {
            "a.txt","A.txt"
        }) {
            QFile f(t.path()+"/source/"+name);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write("test");
        }auto e=df::entry(t.path()+"/source");
        QVERIFY_EXCEPTION_THROWN(df::stageEntries( {
            e
        },t.path()+"/stage"),std::runtime_error);
    }
    void isolatedDiagnostics() {
        auto r=df::command("sh", {
            "-c","printf payload; printf diagnostic >&2"
        });
        QCOMPARE(r.code,0);
        QCOMPARE(r.out,QByteArray("payload"));
        QCOMPARE(r.err,QByteArray("diagnostic"));
        auto timeout=df::command("sh", {
            "-c","exec sleep 5"
        },50);
        QVERIFY(timeout.timedOut);
        QVERIFY(timeout.code!=0);
    }
    void toolProgress() {
        df::Progress p;
        QVERIFY(df::parseToolProgress("Track 01:    1 of    2 MB written (fifo 100%) [buf 100%] 3.3x.", p));
        QCOMPARE(p.stage, QString("写入数据"));
        QCOMPARE(p.percent, 50);
        QVERIFY(df::parseToolProgress("Fixating...", p));
        QCOMPARE(p.stage, QString("关闭区段"));
        QCOMPARE(p.percent, -1);
        QVERIFY(df::parseToolProgress("Performing OPC...", p));
        QCOMPARE(p.stage, QString("校准光驱"));
        QVERIFY(!df::parseToolProgress("fifo min fill was 100%.", p));
        QVERIFY(df::parseToolProgress(" 23.45% done, estimate finish", p));
        QCOMPARE(p.percent, 23);
        QVERIFY(df::parseToolProgress("12345/23456 (52.6%) @2.4x", p));
        QCOMPARE(p.percent, 52);
    }
    void splitProgressLines() {
        QStringList lines;
        const auto r = df::command("sh", {"-c", "printf 'Track 01: 1'; sleep 0.05; printf ' of 2 MB written.\\rFixating...\\n'"}, 1000,
                                   [&](const QString& line) { lines << line; });
        QCOMPARE(r.code, 0);
        QCOMPARE(lines, QStringList({"Track 01: 1 of 2 MB written.", "Fixating..."}));
    }
    void mountReadinessRetry() {
        int mounts = 0, rescans = 0, pauses = 0;
        bool mounted = false;
        df::MountOps ops;
        ops.current = [&] { return mounted ? QString("/media/disc") : QString(); };
        ops.rescan = [&] { ++rescans; };
        ops.pause = [&](int milliseconds) { QCOMPARE(milliseconds, 2000); ++pauses; };
        ops.mount = [&] {
            df::CommandResult r;
            ++mounts;
            if (mounts < 3) { r.code = 1; r.err = "Object is not a mountable filesystem."; }
            else { r.code = 0; mounted = true; }
            return r;
        };
        QCOMPARE(df::mountWithRetry(ops, {}, 4), QString("/media/disc"));
        QCOMPARE(mounts, 3);
        QCOMPARE(rescans, 1);
        QCOMPARE(pauses, 2);
        mounted = false;
        mounts = 0;
        ops.mount = [&] { ++mounts; df::CommandResult r; r.code = 1; r.err = "Not authorized"; return r; };
        QVERIFY_EXCEPTION_THROWN(df::mountWithRetry(ops, {}, 4), std::runtime_error);
        QCOMPARE(mounts, 1);
        mounts = 0;
        ops.mount = [&] { ++mounts; df::CommandResult r; r.code = 1; r.err = "not a mountable filesystem"; return r; };
        QVERIFY_EXCEPTION_THROWN(df::mountWithRetry(ops, {}, 3), std::runtime_error);
        QCOMPARE(mounts, 3);
        ops.mount = [&] { mounted = true; df::CommandResult r; r.code = 1; r.err = "already mounted"; return r; };
        QCOMPARE(df::mountWithRetry(ops, {}, 3), QString("/media/disc"));
    }
    void postWriteOutcomes() {
        df::Options options;
        int verified = 0, ejected = 0;
        df::PostWriteOps ops;
        ops.verify = [&] { ++verified; throw std::runtime_error("filesystem not ready"); };
        ops.eject = [&] { ++ejected; };
        df::BurnResult result;
        result.written = true;
        df::finishWrittenDisc(result, options, ops);
        QVERIFY(!result.success);
        QVERIFY(result.written);
        QVERIFY(!result.verified);
        QVERIFY(result.ejected);
        QCOMPARE(ejected, 1);
        QVERIFY(result.message.contains("数据写入完成"));
        QVERIFY(result.message.contains("校验未完成"));
        QVERIFY(result.message.contains("光盘已弹出"));
        result = {};
        result.written = true;
        ops.verify = [&] { ++verified; };
        ops.eject = [&] { ++ejected; throw std::runtime_error("tray busy"); };
        df::finishWrittenDisc(result, options, ops);
        QVERIFY(!result.success);
        QVERIFY(result.verified);
        QVERIFY(!result.ejected);
        QVERIFY(result.ejectError.contains("tray busy"));
        result = {};
        result.written = true;
        options.verify = false;
        ops.eject = [&] { ++ejected; };
        const int prior = verified;
        df::finishWrittenDisc(result, options, ops);
        QVERIFY(result.success);
        QVERIFY(!result.verified);
        QVERIFY(result.ejected);
        QCOMPARE(verified, prior);
        result = {};
        const int previous = ejected;
        df::finishWrittenDisc(result, options, ops);
        QCOMPARE(ejected, previous); // No automatic post-actions before a completed write.
        options.verify = true;
        options.eject = false;
        result = {};
        result.written = true;
        df::finishWrittenDisc(result, options, ops);
        QVERIFY(result.success);
        QVERIFY(result.verified);
        QCOMPARE(ejected, previous);
        options.eject = true;
        result = {};
        result.written = true;
        df::finishWrittenDisc(result, options, ops);
        QVERIFY(result.success);
        QVERIFY(result.verified);
        QVERIFY(result.ejected);
    }
    void verificationProgress() {
        QTemporaryDir dir;
        QFile file(dir.path() + "/example");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QByteArray(2*1024*1024, 'a'));
        file.close();
        const auto manifest = df::manifest(dir.path());
        QList<int> updates;
        QCOMPARE(df::verifyManifest(dir.path(), manifest, [&](const df::Progress& p) { updates << p.percent; }), QString());
        QVERIFY(updates.contains(0));
        QCOMPARE(updates.last(), 100);
        QVERIFY(file.open(QIODevice::WriteOnly|QIODevice::Truncate));
        file.write("bad data");
        file.close();
        updates.clear();
        QVERIFY(!df::verifyManifest(dir.path(), manifest, [&](const df::Progress& p) { updates << p.percent; }).isEmpty());
        QVERIFY(!updates.contains(100));
    }
    void rawSessionVerification() {
        QTemporaryDir tmp;
        const auto first = tmp.path() + "/first";
        const auto second = tmp.path() + "/second";
        QVERIFY(QDir().mkpath(first + "/旧资料/子目录"));
        QVERIFY(QDir().mkpath(second + "/新资料"));
        auto put = [](const QString& path, const QByteArray& data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(put(first + "/旧资料/子目录/测试文档.txt", QByteArray(180123, 'a')));
        QVERIFY(put(first + "/旧资料/空文件.txt", {}));
        QVERIFY(put(second + "/新资料/结果一.txt", "new contents"));
        QVERIFY(put(second + "/新资料/结果.txt", "literal version suffix"));
        const auto iso1 = tmp.path() + "/first.iso";
        df::createImage(first, iso1, "CHECK");
        QFile original(iso1);
        QVERIFY(original.open(QIODevice::ReadOnly));
        QByteArray media = original.readAll();
        original.close();
        df::SectorReader reader = [&](quint32 lba, quint32 count) {
            return media.mid(qint64(lba) * 2048, count * 2048);
        };
        const auto old = df::manifest(first);
        QList<int> updates;
        QVERIFY2(df::verifyIso(reader, 0, old, [&](const df::Progress& p) { updates << p.percent; }).isEmpty(),
                 qPrintable(df::verifyIso(reader, 0, old)));
        QCOMPARE(updates.last(), 100);
        df::Disc disc;
        disc.family = "cd";
        disc.state = "appendable";
        disc.device = iso1;
        disc.msinfo = "0,20000";
        const auto iso2 = tmp.path() + "/second.iso";
        const auto result = df::command("genisoimage", df::isoArguments(second, iso2, "CHECK", disc));
        QCOMPARE(result.code, 0);
        QFile session(iso2);
        QVERIFY(session.open(QIODevice::ReadOnly));
        const auto added = session.readAll();
        media.resize(20000 * 2048);
        media += added;
        auto expected = old;
        const auto incoming = df::manifest(second);
        for (auto it = incoming.cbegin(); it != incoming.cend(); ++it) expected.insert(it.key(), it.value());
        // The first session's descriptors remain untouched. Read latest at its real LBA.
        QCOMPARE(df::verifyIso(reader, 20000, expected), QString());
        QVERIFY(df::verifyIso(reader, 0, expected).contains("缺失文件"));
        auto missing = expected;
        missing.insert("不存在.txt", QByteArray(32, 'x'));
        QVERIFY(df::verifyIso(reader, 20000, missing).contains("缺失文件"));
        auto wrong = expected;
        wrong["旧资料/子目录/测试文档.txt"] = QByteArray(32, 'x');
        updates.clear();
        QVERIFY(df::verifyIso(reader, 20000, wrong, [&](const df::Progress& p) { updates << p.percent; }).contains("校验失败"));
        QVERIFY(!updates.contains(100));
        const auto good = media;
        const int payload = media.indexOf(QByteArray(256, 'a'));
        QVERIFY(payload > 0);
        media[payload] = 'b';
        QVERIFY(df::verifyIso(reader, 20000, expected).contains("校验失败"));
        media = good;
        const auto svd = media.mid(20017 * 2048, 2048);
        const auto root = qFromLittleEndian<quint32>(svd.constData() + 158);
        media[root * 2048] = 2; // Truncated record must never reach the checksum loop.
        QVERIFY(df::verifyIso(reader, 20000, expected).contains("记录截断"));
        media = good;
        media[20017 * 2048 + 84] = char(quint8(media[20017 * 2048 + 84]) ^ 1);
        QVERIFY(df::verifyIso(reader, 20000, expected).contains("数值损坏"));
        media = good;
        media[20016 * 2048 + 1] = 'X';
        QVERIFY(df::verifyIso(reader, 20000, expected).contains("卷描述符"));
        media = good.left(20017 * 2048);
        QVERIFY(!df::verifyIso(reader, 20000, expected).isEmpty());
        QVERIFY(!df::verifyIso(reader, 0xfffffff0, expected).isEmpty());
        QVERIFY(!df::verifyCd("/tmp/not-an-optical-drive", 0, expected).isEmpty());
    }
    void imageIntegration() {
        QTemporaryDir t;
        QString first=t.path()+"/first",second=t.path()+"/second";
        QDir().mkpath(first);
        QDir().mkpath(second);
        QByteArray original("original-123\n"),added("added-456\n");
        QFile a(first+"/初次资料.txt");
        QVERIFY(a.open(QIODevice::WriteOnly));
        a.write(original);
        a.close();
        QFile b(second+"/追加资料.txt");
        QVERIFY(b.open(QIODevice::WriteOnly));
        b.write(added);
        b.close();
        QString iso1=t.path()+"/first.iso";
        df::createImage(first,iso1,"RESEARCH");
        QFile image(iso1);
        QVERIFY(image.open(QIODevice::ReadOnly));
        QVERIFY(image.seek(16*2048));
        auto pvd=image.read(2048);
        QCOMPARE(pvd.mid(1,5),QByteArray("CD001"));
        QVERIFY(image.seek(17*2048));
        auto joliet=image.read(2048);
        QCOMPARE(joliet[0],char(2));
        QCOMPARE(joliet.mid(88,2),QByteArray("%/"));
        image.close();
        df::Disc d;
        d.family="cd";
        d.state="appendable";
        d.device=iso1;
        d.msinfo="0,20000";
        QString iso2=t.path()+"/second.iso";
        auto r=df::command("genisoimage",df::isoArguments(second,iso2,"RESEARCH",d),0);
        QVERIFY2(r.code==0,qPrintable(r.text()));
        QFile merged(t.path()+"/disc.img");
        QVERIFY(merged.open(QIODevice::WriteOnly));
        QVERIFY(image.open(QIODevice::ReadOnly));
        merged.write(image.readAll());
        image.close();
        QVERIFY(merged.seek(20000LL*2048));
        QFile session(iso2);
        QVERIFY(session.open(QIODevice::ReadOnly));
        merged.write(session.readAll());
        /* Expose latest-session volume descriptors at the image origin for isoinfo. Data extents retain their physical sector addresses. This models an OS selecting the latest TOC session, not a hardware write. */ QVERIFY(session.seek(16*2048));
        auto descriptors=session.read(3*2048);
        QVERIFY(merged.seek(16*2048));
        merged.write(descriptors);
        merged.close();
        for(auto pair: {
            qMakePair(QString("/初次资料.txt"),original),qMakePair(QString("/追加资料.txt"),added)
        }) {
            auto content=df::command("isoinfo", {
                "-R","-i",merged.fileName(),"-x",pair.first
            });
            QVERIFY2(content.code==0,qPrintable(content.text()));
            QCOMPARE(content.out,pair.second);
        }QFile disc(merged.fileName());
        QVERIFY(disc.open(QIODevice::ReadOnly));
        QVERIFY(disc.seek(17*2048));
        auto svd=disc.read(2048);
        QCOMPARE(svd.mid(88,2),QByteArray("%/"));
        auto extent=qFromLittleEndian<quint32>(svd.constData()+158),length=qFromLittleEndian<quint32>(svd.constData()+166);
        QVERIFY(disc.seek(qint64(extent)*2048));
        auto directory=disc.read(length);
        QMap<QString,QByteArray> jolietFiles;
        for(int pos=0;pos<directory.size();) {
            int record=quint8(directory[pos]);
            if(!record) {
                pos=(pos/2048+1)*2048;
                continue;
            }auto data=directory.mid(pos,record);
            if(data.size()<34)break;
            int size=quint8(data[32]);
            auto nameBytes=data.mid(33,size);
            if(size>1) {
                QString name;
                for(int j=0;j+1<nameBytes.size();j+=2)name.append(QChar(qFromBigEndian<quint16>(nameBytes.constData()+j)));
                name.remove(QRegularExpression(";[0-9]+$"));
                auto start=qFromLittleEndian<quint32>(data.constData()+2),bytes=qFromLittleEndian<quint32>(data.constData()+10);
                QVERIFY(disc.seek(qint64(start)*2048));
                jolietFiles.insert(name,disc.read(bytes));
            }pos+=record;
        }QCOMPARE(jolietFiles.value("初次资料.txt"),original);
        QCOMPARE(jolietFiles.value("追加资料.txt"),added);
    }
};
QTEST_GUILESS_MAIN(CoreTests)
#include "core_tests.moc"
