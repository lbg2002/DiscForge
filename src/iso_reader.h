#pragma once
#include "core.h"
namespace df {
// Readers return exactly count * 2048 bytes at absolute physical LBAs.
using SectorReader = std::function<QByteArray(quint32, quint32)>;
QString verifyIso(const SectorReader&, quint32 sessionStart, const Manifest&, ProgressCallback = {});
QString verifyCd(const QString& device, quint32 sessionStart, const Manifest&, ProgressCallback = {});
QString opticalMediaStatus(const QString& device);
}
