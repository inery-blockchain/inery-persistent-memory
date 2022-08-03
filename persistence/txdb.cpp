
#include "txdb.h"

#include "config/chainparams.h"
#include "basics/serialize.h"
#include "main.h"
#include "persistence/block.h"
#include "basics/util/util.h"
#include "vm/inevm/inevmrunenv.h"

#include <algorithm>

bool IneryTxMemCache::AddBlockTx(const IneryBlock &block) {
    for (auto &ptx : block.vptx) {
        txids[ptx->GetHash()] = true;
    }
    return true;
}

bool IneryTxMemCache::RemoveBlockTx(const IneryBlock &block) {
    for (auto &ptx : block.vptx) {
        if (pBase == nullptr) {
            txids.erase(ptx->GetHash());
        } else {
            txids[ptx->GetHash()] = false;
        }
    }
    return true;
}

bool IneryTxMemCache::HasTx(const uint256 &txid) {
    auto it = txids.find(txid);
    if (it != txids.end()) {
        return it->second;
    }

    if (pBase != nullptr) {
        return pBase->HasTx(txid);
    }
    return false;
}

void IneryTxMemCache::BatchWrite(const TxIdMap &txidsIn) {
    for (const auto &item : txidsIn) {
        if (pBase == nullptr && !item.second) {
            txids.erase(item.first);
        } else {
            txids[item.first] = item.second;

        }
    }
}

void IneryTxMemCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(txids);
    txids.clear();
}

void IneryTxMemCache::Clear() { txids.clear(); }

uint64_t IneryTxMemCache::GetSize() { return txids.size(); }

Object IneryTxMemCache::ToJsonObj() const {
    Array txArray;
    for (auto &item : txids) {
        Object obj;
        obj.push_back(Pair("txid", item.first.ToString()));
        obj.push_back(Pair("existed", item.second));
        txArray.push_back(obj);
    }

    Object txCacheObj;
    txCacheObj.push_back(Pair("tx_cache", txArray));
    return txCacheObj;
}
