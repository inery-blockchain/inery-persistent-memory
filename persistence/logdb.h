#ifndef PERSIST_LOGDB_H
#define PERSIST_LOGDB_H

#include "objects/account.h"
#include "objects/id.h"
#include "basics/serialize.h"
#include "dbaccess.h"
#include "dbconf.h"
#include "basics/leb128.h"

#include <map>
#include <set>

#include <vector>

using namespace std;

// TODO: should erase history log?
class IneryLogDBCache {
public:
    IneryLogDBCache() {}
    IneryLogDBCache(CDBAccess *pDbAccess) : executeFailCache(pDbAccess) {}

public:
    bool SetExecuteFail(const int32_t blockHeight, const uint256 txid, const uint8_t errorCode,
                        const string &errorMessage);

    void Flush();

    uint32_t GetCacheSize() const { return executeFailCache.GetCacheSize(); }

    void SetBaseViewPtr(IneryLogDBCache *pBaseIn) { executeFailCache.SetBase(&pBaseIn->executeFailCache); }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) { executeFailCache.SetDbOpLogMap(pDbOpLogMapIn); }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        executeFailCache.RegisterUndoFunc(undoDataFuncMap);
    }
public:
/*  IneryCompositeKVCache    prefixType             key                 value                        variable      */
/*  -------------------- --------------------- ------------------  ---------------------------  -------------- */
    // [prefix]{height}{txid} --> {error code, error message}
    IneryCompositeKVCache<dbk::TX_EXECUTE_FAIL,    pair<CFixedUInt32, uint256>,    std::pair<uint8_t, string> > executeFailCache;
};

#endif // PERSIST_LOGDB_H
