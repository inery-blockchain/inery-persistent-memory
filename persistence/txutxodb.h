#include <map>
#include <set>
#include <vector>

#include "basics/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"
#include "tx/coinutxotx.h"

using namespace std;

class IneryTxUTXODBCache {
public:
    IneryTxUTXODBCache() {};
    IneryTxUTXODBCache(CDBAccess *pDbAccess) : tx_utxo_cache(pDbAccess), tx_utxo_password_proof_cache(pDbAccess) {};
    IneryTxUTXODBCache(IneryTxUTXODBCache* pBaseIn): tx_utxo_cache(pBaseIn->tx_utxo_cache),
                tx_utxo_password_proof_cache(pBaseIn->tx_utxo_password_proof_cache) {} ;

public:
    bool SetUtxoTx(const pair<TxID, CFixedUInt16> &utxoKey);
    bool GetUtxoTx(const pair<TxID, CFixedUInt16> &utxoKey);
    bool DelUtoxTx(const pair<TxID, CFixedUInt16> &utxoKey);

    bool SetUtxoPasswordProof(const tuple<TxID, CFixedUInt16, IneryRegIDKey> &proofKey, uint256 &proof);
    bool GetUtxoPasswordProof(const tuple<TxID, CFixedUInt16, IneryRegIDKey> &proofKey, uint256 &proof);
    bool DelUtoxPasswordProof(const tuple<TxID, CFixedUInt16, IneryRegIDKey> &proofKey);

    void Flush() { 
        tx_utxo_cache.Flush();
        tx_utxo_password_proof_cache.Flush();
    }

    uint32_t GetCacheSize() const { 
        return tx_utxo_cache.GetCacheSize() + tx_utxo_password_proof_cache.GetCacheSize();
    }

    void SetBaseViewPtr(IneryTxUTXODBCache *pBaseIn) { 
        tx_utxo_cache.SetBase(&pBaseIn->tx_utxo_cache);
        tx_utxo_password_proof_cache.SetBase(&pBaseIn->tx_utxo_password_proof_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) { 
        tx_utxo_cache.SetDbOpLogMap(pDbOpLogMapIn);
        tx_utxo_password_proof_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        tx_utxo_cache.RegisterUndoFunc(undoDataFuncMap);
        tx_utxo_password_proof_cache.RegisterUndoFunc(undoDataFuncMap);
    }

public:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// UTXO DB
    // $txid$vout_index -> 1
    IneryCompositeKVCache<   dbk::TX_UTXO,            pair<TxID, CFixedUInt16>,      uint8_t >          tx_utxo_cache;

    // $txid$vout_index$userID -> (hash)
    IneryCompositeKVCache<   dbk::UTXO_PWSDPRF,       tuple<TxID, CFixedUInt16, IneryRegIDKey>, uint256 >   tx_utxo_password_proof_cache;
};

#endif // PERSIST_TXUTXODB_H