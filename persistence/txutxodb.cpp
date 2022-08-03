
#include "txutxodb.h"
#include "config/chainparams.h"

//////////////////////////////////
//// UTXO Cache
/////////////////////////////////
bool IneryTxUTXODBCache::SetUtxoTx(const pair<TxID, CFixedUInt16> &utxoKey) {
    return tx_utxo_cache.SetData(utxoKey, 1);
}

bool IneryTxUTXODBCache::GetUtxoTx(const pair<TxID, CFixedUInt16> &utxoKey) {
    uint8_t data;
    bool result = tx_utxo_cache.GetData(utxoKey, data);
    if (!result)
        return false;
    
    return true;
}
bool IneryTxUTXODBCache::DelUtoxTx(const pair<TxID, CFixedUInt16> &utxoKey) {
    return tx_utxo_cache.EraseData(utxoKey);
}

//////////////////////////////////
//// Password Proof Cache
/////////////////////////////////
bool IneryTxUTXODBCache::SetUtxoPasswordProof(const tuple<TxID, CFixedUInt16, IneryRegIDKey> &proofKey, uint256 &proof) {
    return tx_utxo_password_proof_cache.SetData(proofKey, proof);
}

bool IneryTxUTXODBCache::GetUtxoPasswordProof(const tuple<TxID, CFixedUInt16, IneryRegIDKey> &proofKey, uint256 &proof) {
    return tx_utxo_password_proof_cache.GetData(proofKey, proof);
}

bool IneryTxUTXODBCache::DelUtoxPasswordProof(const tuple<TxID, CFixedUInt16, IneryRegIDKey> &proofKey) {
    return tx_utxo_password_proof_cache.EraseData(proofKey);
}