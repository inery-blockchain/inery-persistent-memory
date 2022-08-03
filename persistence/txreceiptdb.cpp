
#include "txreceiptdb.h"
#include "config/chainparams.h"

bool IneryTxReceiptDBCache::SetTxReceipts(const TxID &txid, const vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return true;

    return tx_receipt_cache.SetData(txid, receipts);
}

bool IneryTxReceiptDBCache::GetTxReceipts(const TxID &txid, vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return false;

    return tx_receipt_cache.GetData(txid, receipts);
}

bool IneryTxReceiptDBCache::SetBlockReceipts(const uint256 &blockHash, const vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return true;

    return block_receipt_cache.SetData(blockHash, receipts);
}

bool IneryTxReceiptDBCache::GetBlockReceipts(const uint256 &blockHash, vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return false;

    return block_receipt_cache.GetData(blockHash, receipts);
}

void IneryTxReceiptDBCache::Flush() {
    tx_receipt_cache.Flush();
    block_receipt_cache.Flush();
}
