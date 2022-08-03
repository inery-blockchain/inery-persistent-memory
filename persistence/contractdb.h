
#ifndef PERSIST_CONTRACTDB_H
#define PERSIST_CONTRACTDB_H

#include "objects/account.h"
#include "objects/contract.h"
#include "objects/key.h"
#include "basics/arith_uint256.h"
#include "basics/uint256.h"
#include "dbaccess.h"
#include "dbiterator.h"
#include "persistence/disk.h"
#include "vm/inevm/appaccount.h"

#include <map>
#include <string>
#include <utility>
#include <vector>

class IneryKeyID;
class IneryRegID;
class IneryAccount;
class CContractDB;
class CCacheWrapper;

struct IneryDiskTxPos;

typedef dbk::CDBTailKey<MAX_CONTRACT_KEY_SIZE> CDBContractKey;

/*  IneryCompositeKVCache     prefixType                       key                       value         variable           */
/*  -------------------- --------------------         ----------------------------  ---------   --------------------- */
    // pair<contractRegId, contractKey> -> contractData
typedef IneryCompositeKVCache< dbk::CONTRACT_DATA,        pair<IneryRegIDKey, CDBContractKey>, string>     DBContractDataCache;

class CDBContractDataIterator: public CDBPrefixIterator<DBContractDataCache, DBContractDataCache::KeyType> {
private:
    typedef typename DBContractDataCache::KeyType KeyType;
    typedef CDBPrefixIterator<DBContractDataCache, DBContractDataCache::KeyType> Base;
public:
    CDBContractDataIterator(DBContractDataCache &dbCache, const IneryRegID &regidIn,
                            const string &contractKeyPrefix)
        : Base(dbCache, KeyType(IneryRegIDKey(regidIn), contractKeyPrefix)) {}

    bool SeekUpper(const string *pLastContractKey) {
        if (pLastContractKey == nullptr || db_util::IsEmpty(*pLastContractKey))
            return First();
        if (pLastContractKey->size() > CDBContractKey::MAX_KEY_SIZE)
            return false;
        KeyType lastKey(GetPrefixElement().first, *pLastContractKey);
        return sp_it_Impl->SeekUpper(&lastKey);
    }

    const string& GetContractKey() const {
        return GetKey().second.GetKey();
    }
};

class CContractDBCache {
public:
    CContractDBCache() {}

    CContractDBCache(CDBAccess *pDbAccess):
        contractCache(pDbAccess),
        contractDataCache(pDbAccess),
        contractAccountCache(pDbAccess),
        contractTracesCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::CONTRACT);
    };

    CContractDBCache(CContractDBCache *pBaseIn):
        contractCache(pBaseIn->contractCache),
        contractDataCache(pBaseIn->contractDataCache),
        contractAccountCache(pBaseIn->contractAccountCache),
        contractTracesCache(pBaseIn->contractTracesCache) {};

    bool GetContractAccount(const IneryRegID &contractRegId, const string &accountKey, CAppUserAccount &appAccOut);
    bool SetContractAccount(const IneryRegID &contractRegId, const CAppUserAccount &appAccIn);

    bool GetContract(const IneryRegID &contractRegId, CUniversalContractStore &contractStore);
    bool SaveContract(const IneryRegID &contractRegId, const CUniversalContractStore &contractStore);
    bool HasContract(const IneryRegID &contractRegId);
    bool EraseContract(const IneryRegID &contractRegId);

    bool GetContractData(const IneryRegID &contractRegId, const string &contractKey, string &contractData);
    bool SetContractData(const IneryRegID &contractRegId, const string &contractKey, const string &contractData);
    bool HasContractData(const IneryRegID &contractRegId, const string &contractKey);
    bool EraseContractData(const IneryRegID &contractRegId, const string &contractKey);

    bool GetContractTraces(const uint256 &txid, string &contractTraces);
    bool SetContractTraces(const uint256 &txid, const string &contractTraces);

    bool Flush();
    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(CContractDBCache *pBaseIn) {
        contractCache.SetBase(&pBaseIn->contractCache);
        contractDataCache.SetBase(&pBaseIn->contractDataCache);
        contractAccountCache.SetBase(&pBaseIn->contractAccountCache);
        contractTracesCache.SetBase(&pBaseIn->contractTracesCache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        contractCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractDataCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractAccountCache.SetDbOpLogMap(pDbOpLogMapIn);
        contractTracesCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        contractCache.RegisterUndoFunc(undoDataFuncMap);
        contractDataCache.RegisterUndoFunc(undoDataFuncMap);
        contractAccountCache.RegisterUndoFunc(undoDataFuncMap);
        contractTracesCache.RegisterUndoFunc(undoDataFuncMap);
    }

    shared_ptr<CDBContractDataIterator> CreateContractDataIterator(const IneryRegID &contractRegid,
        const string &contractKeyPrefix);

public:
/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// ContractDB
    // contract $RegIdKey -> CUniversalContractStore
    IneryCompositeKVCache< dbk::CONTRACT_DEF,         IneryRegIDKey,                  CUniversalContractStore >   contractCache;

    // pair<contractRegId, contractKey> -> contractData
    DBContractDataCache contractDataCache;

    // pair<contractRegId, accountKey> -> appUserAccount
    IneryCompositeKVCache< dbk::CONTRACT_ACCOUNT,     pair<IneryRegIDKey, string>,     CAppUserAccount >           contractAccountCache;

    // txid -> contract_traces
    IneryCompositeKVCache< dbk::CONTRACT_TRACES,     uint256,                      string >                    contractTracesCache;
};

#endif  // PERSIST_CONTRACTDB_H