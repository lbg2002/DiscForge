#include "iso_reader.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <scsi/sg.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <memory>

namespace df {
namespace {
[[noreturn]] void invalid(const QString& text) {
    throw std::runtime_error(text.toUtf8().constData());
}
struct FileDescriptor {
    int value;
    ~FileDescriptor() { if (value >= 0) ::close(value); }
};
quint32 both32(const char* p) {
    const auto n = qFromLittleEndian<quint32>(p);
    if (n != qFromBigEndian<quint32>(p + 4)) invalid("光盘目录数值损坏");
    return n;
}
struct Extent { quint32 lba, bytes; };
class Iso {
    const SectorReader& reader;
    quint32 blocks = 0;
    QMap<QString, Extent> files;
    QSet<quint32> directories;
    QByteArray read(quint32 lba, quint32 count) const {
        if (!count || count > 32 || quint64(lba) + count > blocks)
            invalid("光盘目录引用越界");
        auto bytes = reader(lba, count);
        if (bytes.size() != qint64(count) * 2048) invalid("光盘扇区读取不完整");
        return bytes;
    }
    Extent extent(const QByteArray& record) const {
        if (record.size() < 34 || quint8(record[0]) != record.size() ||
            quint8(record[1]) != 0 || quint8(record[26]) != 0 || quint8(record[27]) != 0 ||
            (quint8(record[25]) & 0x80)) invalid("不支持或损坏的光盘目录记录");
        Extent e{both32(record.constData() + 2), both32(record.constData() + 10)};
        if (quint64(e.lba) + (quint64(e.bytes) + 2047) / 2048 > blocks)
            invalid("光盘文件位置越界");
        return e;
    }
    void walk(const Extent& directory, const QString& prefix, int depth) {
        if (depth > 64 || directory.bytes > 32 * 1024 * 1024 ||
            !directory.bytes || directories.contains(directory.lba)) invalid("光盘目录结构异常");
        directories.insert(directory.lba);
        QByteArray data;
        const quint32 sectors = (directory.bytes + 2047) / 2048;
        for (quint32 n = 0; n < sectors;) {
            const auto count = qMin<quint32>(32, sectors - n);
            data += read(directory.lba + n, count);
            n += count;
        }
        data.truncate(directory.bytes);
        QSet<QString> names;
        for (int pos = 0; pos < data.size();) {
            const int length = quint8(data[pos]);
            if (!length) { pos = (pos / 2048 + 1) * 2048; continue; }
            if (length < 34 || length > data.size() - pos || (pos % 2048) + length > 2048)
                invalid("光盘目录记录截断");
            const auto record = data.mid(pos, length);
            pos += length;
            const int nameLength = quint8(record[32]);
            if (33 + nameLength > length) invalid("光盘文件名记录截断");
            const auto raw = record.mid(33, nameLength);
            if (nameLength == 1 && (raw[0] == 0 || raw[0] == 1)) continue;
            if (!nameLength || nameLength % 2) invalid("光盘中文文件名编码异常");
            QString name;
            for (int j = 0; j < nameLength; j += 2) {
                const auto c = qFromBigEndian<quint16>(raw.constData() + j);
                if (!c || QChar(c).isSurrogate()) invalid("光盘中文文件名编码异常");
                name += QChar(c);
            }
            if (name == "." || name == ".." || name.contains('/') || name.contains('\\'))
                invalid("光盘目录路径异常");
            if (names.contains(name)) invalid("光盘目录存在重名");
            names.insert(name);
            if (files.size() + directories.size() > 100000) invalid("光盘文件数量超过校验上限");
            const auto e = extent(record);
            const auto path = prefix + name;
            if (quint8(record[25]) & 2) walk(e, path + '/', depth + 1);
            else files.insert(path, e);
        }
    }
public:
    explicit Iso(const SectorReader& r) : reader(r) {}
    QString verify(quint32 start, const Manifest& expected, ProgressCallback progress) {
        if (start > 0xfffffff0U - 64) invalid("光盘区段位置无效");
        QByteArray joliet;
        bool terminated = false;
        // Descriptors are session-relative; their data extents are absolute LBAs.
        for (quint32 n = 16; n < 80; ++n) {
            const auto d = reader(start + n, 1);
            if (d.size() != 2048 || d.mid(1, 5) != "CD001" || d[6] != 1)
                invalid("未读到最新区段的有效卷描述符");
            if (quint8(d[0]) == 255) { terminated = true; break; }
            if (d[0] == 2 && (d.mid(88, 3) == "%/@" || d.mid(88, 3) == "%/C" || d.mid(88, 3) == "%/E"))
                joliet = d;
        }
        if (!terminated || joliet.isEmpty()) invalid("最新区段缺少完整中文目录");
        // genisoimage's append image declares the session's block count,
        // while directory extents already include the session offset.
        const quint64 end = quint64(start) + both32(joliet.constData() + 80);
        if (end > 0xffffffffULL) invalid("光盘卷容量越界");
        blocks = quint32(end);
        if (quint64(start) + 18 >= blocks || qFromLittleEndian<quint16>(joliet.constData() + 128) != 2048 ||
            qFromBigEndian<quint16>(joliet.constData() + 130) != 2048)
            invalid("光盘卷容量或扇区大小异常");
        const auto root = joliet.mid(156, quint8(joliet[156]));
        if (root.size() < 34 || !(quint8(root[25]) & 2)) invalid("光盘根目录无效");
        walk(extent(root), {}, 0);
        QMap<QString, Extent> targets;
        qint64 total = 0, done = 0;
        for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
            QString key = it.key();
            // Prefer the exact name, including legitimate semicolons. Only
            // remove an ISO version suffix when resolving a missing name.
            if (!files.contains(key)) {
                const QString versioned = key + ";1";
                if (!files.contains(versioned)) return "光盘缺失文件：" + key;
                key = versioned;
            }
            targets.insert(it.key(), files.value(key));
            total += files.value(key).bytes;
        }
        if (progress) progress({"读取光盘校验", total ? 0 : -1});
        QElapsedTimer timer;
        timer.start();
        for (auto it = expected.cbegin(); it != expected.cend(); ++it) {
            const auto e = targets.value(it.key());
            QCryptographicHash hash(QCryptographicHash::Sha256);
            quint32 remaining = e.bytes, offset = 0;
            while (remaining) {
                const quint32 count = qMin<quint32>(32, (quint64(remaining) + 2047) / 2048);
                auto data = read(e.lba + offset, count);
                const auto take = qMin<quint32>(remaining, data.size());
#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
                hash.addData(QByteArrayView(data.constData(), take));
#else
                hash.addData(data.left(take));
#endif
                remaining -= take;
                offset += count;
                done += take;
                if (progress && timer.elapsed() >= 100) {
                    progress({"读取光盘校验", int(qMin<qint64>(99, done * 100 / qMax<qint64>(1, total)))});
                    timer.restart();
                }
            }
            if (hash.result() != it.value()) return "文件校验失败：" + it.key();
        }
        if (progress) progress({"读取光盘校验", 100});
        return {};
    }
};
}
QString verifyIso(const SectorReader& reader, quint32 start, const Manifest& expected, ProgressCallback progress) {
    try { return Iso(reader).verify(start, expected, progress); }
    catch (const std::exception& e) { return QString::fromUtf8(e.what()); }
}
QString verifyCd(const QString& device, quint32 start, const Manifest& expected, ProgressCallback progress) {
    if (!QRegularExpression("^/dev/sr[0-9]+$").match(device).hasMatch()) return "设备必须为系统光驱";
    FileDescriptor fd{::open(QFile::encodeName(device).constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC)};
    if (fd.value < 0) return "无法只读打开光驱：" + QString::fromLocal8Bit(std::strerror(errno));
    SectorReader read = [&](quint32 lba, quint32 count) {
        if (!count || count > 32) invalid("光盘读取请求超限");
        QByteArray bytes(count * 2048, '\0');
        for (int attempt = 0; attempt < 4; ++attempt) {
            unsigned char cdb[10] = {0x28, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // READ(10) only; never writes.
            qToBigEndian<quint32>(lba, cdb + 2);
            qToBigEndian<quint16>(quint16(count), cdb + 7);
            unsigned char sense[64] = {};
            sg_io_hdr_t io{};
            io.interface_id = 'S';
            io.dxfer_direction = SG_DXFER_FROM_DEV;
            io.cmd_len = sizeof(cdb);
            io.mx_sb_len = sizeof(sense);
            io.dxfer_len = bytes.size();
            io.dxferp = bytes.data();
            io.cmdp = cdb;
            io.sbp = sense;
            io.timeout = 5000;
            if (::ioctl(fd.value, SG_IO, &io) < 0)
                invalid("光驱扇区读取失败：" + QString::fromLocal8Bit(std::strerror(errno)));
            if ((io.info & SG_INFO_OK_MASK) == SG_INFO_OK && !io.resid) return bytes;
            const bool descriptor = (sense[0] & 0x7f) == 0x72 || (sense[0] & 0x7f) == 0x73;
            const int key = descriptor ? sense[1] & 15 : sense[2] & 15;
            const int asc = descriptor ? sense[2] : sense[12];
            // Retry only unit attention or becoming-ready, never damaged data.
            if (attempt < 3 && !io.host_status && (key == 6 || (key == 2 && asc == 4))) {
                QThread::msleep(500);
                continue;
            }
            invalid(QString("读取扇区 %1 失败（状态 %2，主机 %3，感知码 %4/%5）")
                .arg(lba).arg(io.status).arg(io.host_status).arg(key).arg(asc));
        }
        return QByteArray();
    };
    return verifyIso(read, start, expected, progress);
}
QString opticalMediaStatus(const QString& device) {
    FileDescriptor fd{::open(QFile::encodeName(device).constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC)};
    if (fd.value < 0) return {};
    const int status = ::ioctl(fd.value, CDROM_DRIVE_STATUS, CDSL_CURRENT);
    if (status == CDS_NO_DISC) return "未插入光盘";
    if (status == CDS_TRAY_OPEN) return "光驱托盘已打开";
    return {};
}
}
