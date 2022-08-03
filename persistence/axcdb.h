
#ifndef PERSIST_AXCDB_H
#define PERSIST_AXCDB_H

#include <cstdint>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <tuple>
#include <algorithm>

#include "basics/serialize.h"
#include "dbaccess.h"
#include "objects/proposal.h"

using namespace std;

class CAxcDBCache {
public:
    CAxcDBCache() {}
    CAxcDBCache(CDBAccess *pDbAccess) : axc_swapin_cache(pDbAccess) {}
    CAxcDBCache(CAxcDBCache *pBaseIn) : axc_swapin_cache(pBaseIn->axc_swapin_cache){};

    bool Flush() {
        axc_swapin_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return axc_swapin_cache.GetCacheSize();
    }

    void SetBaseViewPtr(CAxcDBCache *pBaseIn) {
        axc_swapin_cache.SetBase(&pBaseIn->axc_swapin_cache);

    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        axc_swapin_cache.SetDbOpLogMap(pDbOpLogMapIn);

    }

    bool SetSwapInMintRecord(ChainType peerChainType, const string& peerChainTxId, const uint64_t mintAmount) {
        auto key = make_pair((uint8_t)peerChainType, peerChainTxId);
        return axc_swapin_cache.SetData(key, IneryVarIntValue(mintAmount));
    }

    bool GetSwapInMintRecord(ChainType peerChainType, const string& peerChainTxId, uint64_t &mintAmount) {
        auto key = make_pair((uint8_t)peerChainType, peerChainTxId);
        IneryVarIntValue<uint64_t> amount;
        if (!axc_swapin_cache.GetData(key, amount))
            return false;

        mintAmount = amount.get();
        return true;
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        axc_swapin_cache.RegisterUndoFunc(undoDataFuncMap);
    }


public:
/*  InerySimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */


/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
//swap_in
// $peer_chain_id, $peer_chain_txid -> coin_amount_to_mint
    IneryCompositeKVCache<dbk::AXC_SWAP_IN,        pair<uint8_t, string>,                  IneryVarIntValue<uint64_t> > axc_swapin_cache;
};

#endif //PERSIST_AXCDB_H