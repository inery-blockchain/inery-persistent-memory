
#include "disk.h"
#include "logging.h"
#include "boost/filesystem.hpp"

////////////////////////////////////////////////////////////////////////////////
// class IneryBlockFileInfo

string IneryBlockFileInfo::ToString() const {
    return strprintf("IneryBlockFileInfo(blocks=%u, size=%u, heights=%u...%u, time=%s...%s)", nBlocks,
                     nSize, nHeightFirst, nHeightLast,
                     DateTimeStrFormat("%Y-%m-%d", nTimeFirst).c_str(),
                     DateTimeStrFormat("%Y-%m-%d", nTimeLast).c_str());
}

void IneryBlockFileInfo::AddBlock(uint32_t nHeightIn, uint64_t nTimeIn) {
    if (nBlocks == 0 || nHeightFirst > nHeightIn)
        nHeightFirst = nHeightIn;

    if (nBlocks == 0 || nTimeFirst > nTimeIn)
        nTimeFirst = nTimeIn;

    nBlocks++;

    if (nHeightIn > nHeightLast)
        nHeightLast = nHeightIn;

    if (nTimeIn > nTimeLast)
        nTimeLast = nTimeIn;
}

////////////////////////////////////////////////////////////////////////////////
// global functions

FILE *OpenDiskFile(const IneryDiskBlockPos &pos, const char *prefix, bool fReadOnly) {
    if (pos.IsNull())
        return nullptr;
    boost::filesystem::path path = GetDataDir() / "blocks" / strprintf("%s%05u.dat", prefix, pos.nFile);
    boost::filesystem::create_directories(path.parent_path());
    FILE *file = fopen(path.string().c_str(), "rb+");
    if (!file && !fReadOnly)
        file = fopen(path.string().c_str(), "wb+");
    if (!file) {
        LogPrint(BCLog::ERROR, "Unable to open file %s\n", path.string());
        return nullptr;
    }

    if (pos.nPos) {
        if (fseek(file, pos.nPos, SEEK_SET)) {
            LogPrint(BCLog::ERROR, "Unable to seek to position %u of %s\n", pos.nPos, path.string());
            fclose(file);
            return nullptr;
        }
    }
    return file;
}

FILE *OpenBlockFile(const IneryDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "blk", fReadOnly);
}
