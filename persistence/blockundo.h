
#ifndef PERSIST_BLOCK_UNDO_H
#define PERSIST_BLOCK_UNDO_H

#include "basics/serialize.h"
#include "basics/uint256.h"
#include "cachewrapper.h"
#include "leveldbwrapper.h"
#include "disk.h"

#include <stdint.h>
#include <memory>

class IneryTxUndo {
public:
    uint256     txid;
    CDBOpLogMap dbOpLogMap; // dbPrefix -> dbOpLogs

    IMPLEMENT_SERIALIZE(
        READWRITE(txid);
        READWRITE(dbOpLogMap);
	)

public:
    IneryTxUndo() {}

    IneryTxUndo(const uint256 &txidIn): txid(txidIn) {}

    // uint256 CalcStateHash();
    void SetTxID(const TxID &txidIn) { txid = txidIn; }

    void Clear() {
        txid       = uint256();
        dbOpLogMap.Clear();
    }

    string ToString() const;
};

/** Undo information for a IneryBlock */
class IneryBlockUndo {
public:
    vector<IneryTxUndo> vtxundo;

    IMPLEMENT_SERIALIZE(
        READWRITE(vtxundo);
    )

    // uint256 CalcStateHash(uint256 preHash);
    bool WriteToDisk(IneryDiskBlockPos &pos, const uint256 &blockHash);

    bool ReadFromDisk(const IneryDiskBlockPos &pos, const uint256 &blockHash);

    string ToString() const;
};

class IneryTxUndoOpLogger {
public:
    CCacheWrapper &cw;
    IneryBlockUndo &block_undo;
    IneryTxUndo tx_undo;

    IneryTxUndoOpLogger(CCacheWrapper& cwIn, const TxID& txidIn, IneryBlockUndo& blockUndoIn)
        : cw(cwIn), block_undo(blockUndoIn) {

        tx_undo.SetTxID(txidIn);
        cw.SetDbOpLogMap(&tx_undo.dbOpLogMap);
    }
    ~IneryTxUndoOpLogger() {
        block_undo.vtxundo.push_back(tx_undo);
        cw.SetDbOpLogMap(nullptr);
    }
};

class IneryBlockUndoExecutor {
public:
    CCacheWrapper &cw;
    IneryBlockUndo &block_undo;

    IneryBlockUndoExecutor(CCacheWrapper &cwIn, IneryBlockUndo &blockUndoIn)
        : cw(cwIn), block_undo(blockUndoIn) {}
    bool Execute();
};

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const IneryDiskBlockPos &pos, bool fReadOnly = false);

#endif  // PERSIST_BLOCK_UNDO_H