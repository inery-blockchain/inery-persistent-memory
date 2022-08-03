
#ifndef PERSIST_CACHEWRAPPER_H
#define PERSIST_CACHEWRAPPER_H

#include "accountdb.h"
#include "assetdb.h"
#include "blockdb.h"
#include "cdpdb.h"
#include "basics/uint256.h"
#include "contractdb.h"
#include "delegatedb.h"
#include "dexdb.h"
#include "pricefeeddb.h"
#include "sysparamdb.h"
#include "txdb.h"
#include "txreceiptdb.h"
#include "txutxodb.h"
#include "axcdb.h"
#include "sysgoverndb.h"
#include "logdb.h"

class CCacheDBManager;

class CCacheWrapper {
public:
    InerySysParamDBCache    sysParamCache;
    IneryBlockDBCache       blockCache;
    IneryAccountDBCache     accountCache;
    CAssetDbCache       assetCache;
    CContractDBCache    contractCache;
    IneryDelegateDBCache    delegateCache;
    IneryCdpDBCache         cdpCache;
    CClosedCdpDBCache   closedCdpCache;
    CDexDBCache         dexCache;
    IneryTxReceiptDBCache   txReceiptCache;
    IneryTxUTXODBCache      txUtxoCache;
    InerySysGovernDBCache   sysGovernCache;
    CAxcDBCache         axcCache;

    IneryTxMemCache         txCache;
    IneryPricePointMemCache ppCache;
    IneryPriceFeedCache     priceFeedCache;

public:
    static std::shared_ptr<CCacheWrapper> NewCopyFrom(CCacheDBManager* pCdMan);

public:
    CCacheWrapper();

    CCacheWrapper(CCacheWrapper* cwIn);
    CCacheWrapper(CCacheDBManager* pCdMan);

    CCacheWrapper& operator=(CCacheWrapper& other);

    void CopyFrom(CCacheDBManager* pCdMan);

    void Flush();

    UndoDataFuncMap GetUndoDataFuncMap();

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMap);

private:
    CCacheWrapper(const CCacheWrapper&) = delete;
    CCacheWrapper& operator=(const CCacheWrapper&) = delete;

};

class CCacheDBManager {
public:
    CDBAccess           *pSysParamDb;
    InerySysParamDBCache    *pSysParamCache;

    CDBAccess           *pAccountDb;
    IneryAccountDBCache     *pAccountCache;

    CDBAccess           *pAssetDb;
    CAssetDbCache       *pAssetCache;

    CDBAccess           *pContractDb;
    CContractDBCache    *pContractCache;

    CDBAccess           *pDelegateDb;
    IneryDelegateDBCache    *pDelegateCache;

    CDBAccess           *pCdpDb;
    IneryCdpDBCache         *pCdpCache;

    CDBAccess           *pClosedCdpDb;
    CClosedCdpDBCache   *pClosedCdpCache;

    CDBAccess           *pDexDb;
    CDexDBCache         *pDexCache;

    IneryBlockIndexDB       *pBlockIndexDb;

    CDBAccess           *pBlockDb;
    IneryBlockDBCache       *pBlockCache;

    CDBAccess           *pLogDb;
    IneryLogDBCache         *pLogCache;

    CDBAccess           *pReceiptDb;
    IneryTxReceiptDBCache   *pReceiptCache;

    CDBAccess           *pUtxoDb;
    IneryTxUTXODBCache      *pUtxoCache;

    CDBAccess           *pAxcDb;
    CAxcDBCache         *pAxcCache;

    CDBAccess           *pSysGovernDb;
    InerySysGovernDBCache   *pSysGovernCache;

    CDBAccess           *pPriceFeedDb;
    IneryPriceFeedCache     *pPriceFeedCache;

    IneryTxMemCache         *pTxCache;
    IneryPricePointMemCache *pPpCache;

public:
    CCacheDBManager(bool isReindex, bool isMemory);

    ~CCacheDBManager();

    bool Flush();
private:
    CDBAccess* CreateDbAccess(DBNameType dbNameTypeIn);
private:
    bool is_reindex = false;
    bool is_memory = false;
};  // CCacheDBManager

const IneryRegID& GetBlockBpRegid(const IneryBlock &block);

#endif //PERSIST_CACHEWRAPPER_H