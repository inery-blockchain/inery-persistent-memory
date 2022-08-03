#include <map>
#include <string>
#include <utility>
#include <vector>
#include "objects/key.h"
#include "objects/account.h"

#include "basics/uint256.h"
#include "basics/util/util.h"
#include "basics/arith_uint256.h"

#include "leveldbwrapper.h"
#include "dbconf.h"
#include "dbaccess.h"

class uint256;
class IneryKeyID;

class IneryAccountDBCache {
public:
    IneryAccountDBCache() {}

    IneryAccountDBCache(CDBAccess *pDbAccess):
        regId2KeyIdCache(pDbAccess),
        accountCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ACCOUNT);
    }

    IneryAccountDBCache(IneryAccountDBCache *pBase):
        regId2KeyIdCache(pBase->regId2KeyIdCache),
        accountCache(pBase->accountCache) {}

    ~IneryAccountDBCache() {}

public:
    bool GetAccount(const IneryKeyID &keyId,    IneryAccount &account) const;
    bool GetAccount(const IneryRegID &regId,    IneryAccount &account) const;
    bool GetAccount(const IneryUserID &uid,     IneryAccount &account) const;

    bool SetAccount(const IneryKeyID &keyId,    const IneryAccount &account);
    bool SetAccount(const IneryRegID &regId,    const IneryAccount &account);
    bool SetAccount(const IneryUserID &uid,     const IneryAccount &account);
    bool SaveAccount(const IneryAccount &account);

    bool HasAccount(const IneryKeyID &keyId) const;
    bool HasAccount(const IneryRegID &regId) const;
    bool HasAccount(const IneryUserID &userId) const;

    bool EraseAccount(const IneryKeyID &keyId);
    bool EraseAccount(const IneryUserID &userId);

    bool NewRegId(const IneryRegID &regid, const IneryKeyID &keyId);

    bool BatchWrite(const map<IneryKeyID, IneryAccount> &mapAccounts,
                    const map<IneryRegID, IneryKeyID> &mapKeyIds,
                    const uint256 &blockHash);

    bool BatchWrite(const vector<IneryAccount> &accounts);

    bool GetKeyId(const IneryRegID &regId,  IneryKeyID &keyId) const;
    bool GetKeyId(const IneryUserID &uid,   IneryKeyID &keyId) const;

    bool EraseKeyId(const IneryRegID &regId);
    bool EraseKeyId(const IneryUserID &userId);

    std::tuple<uint64_t, uint64_t, uint64_t, uint64_t> TraverseAccount();
    Object GetAccountDBStats();
    uint64_t GetAssetTotalSupply(TokenSymbol assetSymbol);

    bool GetUserId(const string &addr, IneryUserID &userId) const;
    bool GetRegId(const IneryKeyID &keyId, IneryRegID &regId) const;
    bool GetRegId(const IneryUserID &userId, IneryRegID &regId) const;

    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(IneryAccountDBCache *pBaseIn) {
        accountCache.SetBase(&pBaseIn->accountCache);
        regId2KeyIdCache.SetBase(&pBaseIn->regId2KeyIdCache);
    };

    uint64_t GetAccountFreeAmount(const IneryKeyID &keyId, const TokenSymbol &tokenSymbol);

    bool Flush();

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        accountCache.SetDbOpLogMap(pDbOpLogMapIn);
        regId2KeyIdCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        regId2KeyIdCache.RegisterUndoFunc(undoDataFuncMap);
        accountCache.RegisterUndoFunc(undoDataFuncMap);
    }
public:
/*  IneryCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <prefix$RegID -> KeyID>
    IneryCompositeKVCache< dbk::REGID_KEYID,          IneryRegIDKey,       IneryKeyID >         regId2KeyIdCache;
    // <prefix$KeyID -> Account>
    IneryCompositeKVCache< dbk::KEYID_ACCOUNT,        IneryKeyID,       IneryAccount>        accountCache;

};

#endif  // PERSIST_ACCOUNTDB_H
