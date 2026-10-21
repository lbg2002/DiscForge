#pragma once
#include <QtCore>
#include <functional>
namespace df {
    using Log = std::function<void(const QString&)>;
    struct Progress {
        QString stage;
        int percent = -1;
    };
    using ProgressCallback = std::function<void(const Progress&)>;
    struct CommandResult {
        int code = -1;
        QByteArray out, err;
        bool timedOut = false;
        QString text() const {
            return QString::fromUtf8(out + '\n' + err);
        }
    };
    CommandResult command(const QString& program, const QStringList& args, int timeoutMs = 20000, Log log = {
    });
    QString executable(const QString& name);
    struct Disc {
        QString device, family, profile, state = "unknown", message, raw, msinfo;
        qint64 total = -1, free = -1, nextSector = 0;
        int sessions = 0;
        bool writable() const {
            return (state == "blank" || state == "appendable") && free > 0;
        }
        bool blank() const {
            return state == "blank";
        }
        QString chineseType() const;
    };
    struct Entry {
        QString path, name;
        qint64 bytes = 0;
    };
    struct Options {
        QString device, label;
        int speed = 0;
        bool finalize = false, verify = true, eject = true;
    };
    struct BurnResult {
        bool success = false, written = false, verified = false, ejected = false;
        QString verificationError, ejectError;
        QString message;
        qint64 bytes = 0;
        QStringList log;
    };
    using Manifest = QMap<QString,QByteArray>;
    struct MountOps {
        std::function<QString()> current;
        std::function<CommandResult()> mount;
        std::function<void()> rescan;
        std::function<void(int)> pause;
    };
    struct PostWriteOps {
        std::function<void()> verify, eject;
    };
    QString mountWithRetry(const MountOps&, Log log = {}, int attempts = 16);
    void finishWrittenDisc(BurnResult&, const Options&, const PostWriteOps&, Log log = {}, ProgressCallback progress = {});
    bool parseToolProgress(const QString&, Progress&);
    Disc parseCd(const QString& atip, const QString& toc, const QString& msinfo);
    Disc parseDvd(const QString& text);
    Disc inspect(const QString& device);
    QStringList devices();
    QString formatBytes(qint64 bytes, const QString& unit);
    Entry entry(const QString& path);
    void stageEntries(const QList<Entry>& entries, const QString& target);
    Manifest manifest(const QString& root);
    QString verifyManifest(const QString& root, const Manifest& expected, ProgressCallback progress = {});
    QStringList isoArguments(const QString& root, const QString& image, const QString& label, const Disc& disc);
    QStringList cdArguments(const QString& image, const Options& options);
    qint64 cdFifoBytes(quint64 memlockLimit);
    QStringList dvdArguments(const QString& root, const Options& options, const Disc& disc);
    QString mountPoint(const QString& device);
    QString mountDisc(const QString& device, Log log = {});
    void rescanDisc(const QString& device, Log log = {});
    void unmountDisc(const QString& device);
    BurnResult burn(const QList<Entry>& entries, const Options& options, Log log = {}, ProgressCallback progress = {});
    void createImage(const QString& root, const QString& image, const QString& label, Log log = {
    });
}
