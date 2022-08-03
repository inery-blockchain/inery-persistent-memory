
#include "contractdb.h"
#include "objects/account.h"


#include "objects/account.h"
#include "objects/id.h"
#include "objects/key.h"
#include "basics/uint256.h"
#include "basics/util/util.h"
#include "vm/inevm/inevmrunenv.h"

#include <stdint.h>

using namespace std;

/************************ contract account ******************************/
bool CContractDBCache::GetContractAccount(const IneryRegID &contractRegId, const string &accountKey,
                                          CAppUserAccount &appAccOut) {
    auto key = std::make_pair(IneryRegIDKey(contractRegId), accountKey);
    return contractAccountCache.GetData(key, appAccOut);
}

bool CContractDBCache::SetContractAccount(const IneryRegID &contractRegId, const CAppUserAccount &appAccIn) {
    if (appAccIn.IsEmpty()) {
        return false;
    }
    auto key = std::make_pair(IneryRegIDKey(contractRegId), appAccIn.GetAcIneryUserId());
    return contractAccountCache.SetData(key, appAccIn);
}

/************************ contract in cache ******************************/
bool CContractDBCache::GetContract(const IneryRegID &contractRegId, CUniversalContractStore &contractStore) {
    return contractCache.GetData(IneryRegIDKey(contractRegId), contractStore);
}

bool CContractDBCache::SaveContract(const IneryRegID &contractRegId, const CUniversalContractStore &contractStore) {
    return contractCache.SetData(IneryRegIDKey(contractRegId), contractStore);
}

bool CContractDBCache::HasContract(const IneryRegID &contractRegId) {
    return contractCache.HasData(IneryRegIDKey(contractRegId));
}

bool CContractDBCache::EraseContract(const IneryRegID &contractRegId) {
    return contractCache.EraseData(IneryRegIDKey(contractRegId));
}

/************************ contract managed APP data ******************************/
bool CContractDBCache::GetContractData(const IneryRegID &contractRegId, const string &contractKey, string &contractData) {
    auto key = std::make_pair(IneryRegIDKey(contractRegId), contractKey);
    return contractDataCache.GetData(key, contractData);
}

bool CContractDBCache::SetContractData(const IneryRegID &contractRegId, const string &contractKey,
                                       const string &contractData) {
    auto key = std::make_pair(IneryRegIDKey(contractRegId), contractKey);
    return contractDataCache.SetData(key, contractData);
}

bool CContractDBCache::HasContractData(const IneryRegID &contractRegId, const string &contractKey) {
    auto key = std::make_pair(IneryRegIDKey(contractRegId), contractKey);
    return contractDataCache.HasData(key);
}

bool CContractDBCache::EraseContractData(const IneryRegID &contractRegId, const string &contractKey) {
    auto key = std::make_pair(IneryRegIDKey(contractRegId), contractKey);
    return contractDataCache.EraseData(key);
}

bool CContractDBCache::GetContractTraces(const uint256 &txid, string &contractTraces) {
    return contractTracesCache.GetData(txid, contractTraces);
}

bool CContractDBCache::SetContractTraces(const uint256 &txid, const string &contractTraces) {
    return contractTracesCache.SetData(txid, contractTraces);
}

bool CContractDBCache::Flush() {
    contractCache.Flush();
    contractDataCache.Flush();
    contractAccountCache.Flush();
    contractTracesCache.Flush();

    return true;
}

uint32_t CContractDBCache::GetCacheSize() const {
    return contractCache.GetCacheSize() +
        contractDataCache.GetCacheSize() +
        contractTracesCache.GetCacheSize();
}

shared_ptr<CDBContractDataIterator> CContractDBCache::CreateContractDataIterator(const IneryRegID &contractRegid,
        const string &contractKeyPrefix) {

    if (contractKeyPrefix.size() > CDBContractKey::MAX_KEY_SIZE) {
        LogPrint(BCLog::ERROR, "CContractDBCache::CreateContractDatasGetter() contractKeyPrefix.size()=%u "
                 "exceeded the max size=%u", contractKeyPrefix.size(), CDBContractKey::MAX_KEY_SIZE);
        return nullptr;
    }
    return make_shared<CDBContractDataIterator>(contractDataCache, contractRegid, contractKeyPrefix);
}