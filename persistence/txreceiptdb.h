
#ifndef PERSIST_RECEIPTDB_H
#define PERSIST_RECEIPTDB_H

#include "basics/serialize.h"
#include "objects/receipt.h"
#include "dbaccess.h"
#include "dbconf.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

class IneryTxReceiptDBCache {
public:
    IneryTxReceiptDBCache() {}
    IneryTxReceiptDBCache(CDBAccess *pDbAccess) : tx_receipt_cache(pDbAccess), block_receipt_cache(pDbAccess) {}

public:
    bool SetTxReceipts(const TxID &txid, const vector<CReceipt> &receipts);

    bool GetTxReceipts(const TxID &txid, vector<CReceipt> &receipts);

    bool SetBlockReceipts(const uint256 &blockHash, const vector<CReceipt> &receipts);

    bool GetBlockReceipts(const uint256 &blockHash, vector<CReceipt> &receipts);

    void Flush();

    uint32_t GetCacheSize() const { return tx_receipt_cache.GetCacheSize() + block_receipt_cache.GetCacheSize(); }

    void SetBaseViewPtr(IneryTxReceiptDBCache *pBaseIn) {
        tx_receipt_cache.SetBase(&pBaseIn->tx_receipt_cache);
        block_receipt_cache.SetBase(&pBaseIn->block_receipt_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        tx_receipt_cache.SetDbOpLogMap(pDbOpLogMapIn);
        block_receipt_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        tx_receipt_cache.RegisterUndoFunc(undoDataFuncMap);
        block_receipt_cache.RegisterUndoFunc(undoDataFuncMap);
    }
public:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // txid -> vector<CReceipt>
    IneryCompositeKVCache< dbk::TX_RECEIPT,            TxID,                   vector<CReceipt> >     tx_receipt_cache;
    IneryCompositeKVCache< dbk::BLOCK_RECEIPT,         uint256,                vector<CReceipt> >     block_receipt_cache;
};

#endif // PERSIST_RECEIPTDB_H