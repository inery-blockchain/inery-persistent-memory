
#ifndef PERSIST_TXDB_H
#define PERSIST_TXDB_H

#include "objects/account.h"
#include "objects/id.h"
#include "basics/serialize.h"
#include "basics/json/json_spirit_value.h"
#include "dbaccess.h"
#include "dbconf.h"
#include "block.h"

#include <map>
#include <vector>

using namespace std;
using namespace json_spirit;

class IneryTxMemCache {
public:
    typedef unordered_map<uint256, bool, CUint256Hasher> TxIdMap;
public:
    IneryTxMemCache() : pBase(nullptr) {}
    IneryTxMemCache(IneryTxMemCache *pBaseIn) : pBase(pBaseIn) {}

public:
    bool HasTx(const uint256 &txid);

    bool AddBlockTx(const IneryBlock &block);
    bool RemoveBlockTx(const IneryBlock &block);

    void Clear();
    void SetBaseViewPtr(IneryTxMemCache *pBaseIn) { pBase = pBaseIn; }
    void Flush();

    Object ToJsonObj() const;
    uint64_t GetSize();

private:
    void BatchWrite(const TxIdMap &txidsIn);

private:
    TxIdMap txids;
    IneryTxMemCache *pBase;
};

#endif // PERSIST_TXDB_H