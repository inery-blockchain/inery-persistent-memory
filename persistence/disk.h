
#ifndef PERSIST_DISK_H
#define PERSIST_DISK_H

#include "basics/util/util.h"
#include "basics/serialize.h"
#include "objects/id.h"

struct IneryDiskBlockPos {
    int32_t nFile;
    uint32_t nPos;

    IMPLEMENT_SERIALIZE(
        READWRITE(nFile); /* TODO: write with var signed int format */
        READWRITE(VARINT(nPos));
    )

    IneryDiskBlockPos() {
        SetNull();
    }

    IneryDiskBlockPos(uint32_t nFileIn, uint32_t nPosIn) {
        nFile = nFileIn;
        nPos  = nPosIn;
    }

    friend bool operator==(const IneryDiskBlockPos &a, const IneryDiskBlockPos &b) {
        return (a.nFile == b.nFile && a.nPos == b.nPos);
    }

    friend bool operator!=(const IneryDiskBlockPos &a, const IneryDiskBlockPos &b) {
        return !(a == b);
    }

    void SetNull() {
        nFile = -1;
        nPos  = 0;
    }
    bool IsNull() const { return (nFile == -1); }

    bool IsEmpty() const { return IsNull(); }

    string ToString() const {
        return strprintf("file=%d", nFile) + ", " +
        strprintf("pos=%d", nPos);
    }
};

struct IneryDiskTxPos : public IneryDiskBlockPos {
    uint32_t nTxOffset;  // after header
    IneryTxCord  tx_cord;

    IMPLEMENT_SERIALIZE(
        READWRITE(*(IneryDiskBlockPos *)this);
        READWRITE(VARINT(nTxOffset));
        READWRITE(tx_cord);
    )

    IneryDiskTxPos(const IneryDiskBlockPos &blockIn, uint32_t nTxOffsetIn, const IneryTxCord &txCordIn) :
        IneryDiskBlockPos(blockIn.nFile, blockIn.nPos), nTxOffset(nTxOffsetIn), tx_cord(txCordIn) {
    }

    IneryDiskTxPos() {
        SetNull();
    }

    void SetNull() {
        IneryDiskBlockPos::SetNull();
        nTxOffset = 0;
    }

    void SetEmpty() { SetNull(); }

    string ToString() const {
        return strprintf("%s, tx_offset=%d", IneryDiskBlockPos::ToString(), nTxOffset);
    }
};

class IneryBlockFileInfo {
public:
    uint32_t nBlocks;       // number of blocks stored in file
    uint32_t nSize;         // number of used bytes of block file
    uint32_t nUndoSize;     // number of used bytes in the undo file
    uint32_t nHeightFirst;  // lowest height of block in file
    uint32_t nHeightLast;   // highest height of block in file
    uint64_t nTimeFirst;    // earliest time of block in file
    uint64_t nTimeLast;     // latest time of block in file

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nBlocks));
        READWRITE(VARINT(nSize));
        READWRITE(VARINT(nUndoSize));
        READWRITE(VARINT(nHeightFirst));
        READWRITE(VARINT(nHeightLast));
        READWRITE(VARINT(nTimeFirst));
        READWRITE(VARINT(nTimeLast));)

    void SetNull() {
        nBlocks      = 0;
        nSize        = 0;
        nUndoSize    = 0;
        nHeightFirst = 0;
        nHeightLast  = 0;
        nTimeFirst   = 0;
        nTimeLast    = 0;
    }

    bool IsEmpty() { return nBlocks == 0 && nSize == 0; }
    void SetEmpty() { SetNull(); }

    IneryBlockFileInfo() {
        SetNull();
    }

    string ToString() const;

    // update statistics (does not update nSize)
    void AddBlock(uint32_t nHeightIn, uint64_t nTimeIn);
};

FILE *OpenDiskFile(const IneryDiskBlockPos &pos, const char *prefix, bool fReadOnly);

/** Open a block file (blk?????.dat) */
FILE *OpenBlockFile(const IneryDiskBlockPos &pos, bool fReadOnly = false);

#endif //PERSIST_DISK_H