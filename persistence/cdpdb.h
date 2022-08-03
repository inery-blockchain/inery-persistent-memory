
#ifndef PERSIST_CDPDB_H
#define PERSIST_CDPDB_H

#include "basics/uint256.h"
#include "basics/leb128.h"
#include "objects/cdp.h"
#include "dbaccess.h"
#include "dbiterator.h"

#include <map>
#include <set>
#include <string>
#include <cstdint>

using namespace std;

/*  IneryCompositeKVCache     prefixType        key                  value           variable  */
/*  ----------------   --------------      -----------------    --------------   -----------*/
// cdpr{$Ratio}{$height}{$cdpid} -> IneryUserCDP
// height: allows data of the same ratio to be sorted by height, from low to high
typedef IneryCompositeKVCache<dbk::CDP_RATIO_INDEX, tuple<IneryCdpCoinPair, CFixedUInt64, CFixedUInt64, uint256>, IneryUserCDP>  IneryCdpRatioIndexCache;
// cdpr{$height}{$cdpid} -> IneryUserCDP
// cdp sort by height, from low to high
typedef IneryCompositeKVCache<dbk::CDP_HEIGHT_INDEX, tuple<IneryCdpCoinPair, CFixedUInt64, uint256>, IneryUserCDP> IneryCdpHeightIndexCache;

class CDBCdpHeightIndexIt: public CDBPrefixIterator<IneryCdpHeightIndexCache, IneryCdpCoinPair> {
public:
    typedef CDBPrefixIterator<IneryCdpHeightIndexCache, IneryCdpCoinPair> Base;
    using Base::Base;

    uint64_t GetHeight() const {
        return std::get<1>(GetKey()).value;
    }
    const uint256& GetCdpId() const {
        return std::get<2>(GetKey());
    }
};

class IneryCdpDBCache {
public:
    IneryCdpDBCache() {}
    IneryCdpDBCache(CDBAccess *pDbAccess);
    IneryCdpDBCache(IneryCdpDBCache *pBaseIn);

    bool NewCDP(const int32_t blockHeight, IneryUserCDP &cdp);
    bool EraseCDP(const IneryUserCDP &oldCDP, const IneryUserCDP &cdp);
    bool UpdateCDP(const IneryUserCDP &oldCDP, const IneryUserCDP &newCDP);

    bool UserHaveCdp(const IneryRegID &regId, const TokenSymbol &assetSymbol, const TokenSymbol &scoinSymbol);
    bool GetCDPList(const IneryRegID &regId, vector<IneryUserCDP> &cdpList);
    bool GetCDP(const uint256 cdpid, IneryUserCDP &cdp);

    list<IneryUserCDP> GetCdpListByCollateralRatio(const IneryCdpCoinPair &cdpCoinPair, const uint64_t collateralRatio,
            const uint64_t INRMedianPrice);


    shared_ptr<CDBCdpHeightIndexIt> CreateCdpHeightIndexIt(const IneryCdpCoinPair &cdpCoinPair) {
        return make_shared<CDBCdpHeightIndexIt>(cdp_height_index_cache, cdpCoinPair);
    }

    inline uint64_t GetGlobalStakedINRs() const;
    inline uint64_t GetGlobalOwedScoins() const;
    IneryCdpGlobalData GetCdpGlobalData(const IneryCdpCoinPair &cdpCoinPair) const;

    bool GetCdpINR(const TokenSymbol &INRSymbol, IneryCdpINRDetail &cdpINR);
    bool IsCdpINRActivated(const TokenSymbol &INRSymbol);
    bool SetCdpINR(const TokenSymbol &INRSymbol, const IneryCdpINRDetail &cdpINR);

    void SetBaseViewPtr(IneryCdpDBCache *pBaseIn);
    void SetDbOpLogMap(CDBOpLogMap * pDbOpLogMapIn);

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        cdp_global_data_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_INR_cache.RegisterUndoFunc(undoDataFuncMap);
        user_cdp_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_ratio_index_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_height_index_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    uint32_t GetCacheSize() const;
    bool Flush();
private:
    bool SaveCDPToDB(const IneryUserCDP &cdp);
    bool EraseCDPFromDB(const IneryUserCDP &cdp);

    bool SaveCdpIndexData(const IneryUserCDP &userCdp);
    bool EraseCdpIndexData(const IneryUserCDP &userCdp);

    IneryCdpRatioIndexCache::KeyType MakeCdpRatioIndexKey(const IneryUserCDP &cdp);
    IneryCdpHeightIndexCache::KeyType MakeCdpHeightIndexKey(const IneryUserCDP &cdp);
public:
    /*  IneryCompositeKVCache  prefixType       key                            value             variable  */
    /*  ---------------- --------------   ------------                --------------    ----- --------*/
    // cdpCoinPair -> total staked assets
    IneryCompositeKVCache<  dbk::CDP_GLOBAL_DATA, IneryCdpCoinPair,   IneryCdpGlobalData>    cdp_global_data_cache;
    // cdp{$cdpid} -> IneryUserCDP
    IneryCompositeKVCache<  dbk::CDP,       uint256,                    IneryUserCDP>           cdp_cache;
    // cbca{$INR_symbol} -> $cdpINRDetail
    IneryCompositeKVCache<  dbk::CDP_INR, TokenSymbol,    IneryCdpINRDetail>           cdp_INR_cache;
    // ucdp${IneryRegID}{$cdpCoinPair} -> set<cdpid>
    IneryCompositeKVCache<  dbk::USER_CDP, pair<IneryRegIDKey, IneryCdpCoinPair>, optional<uint256>> user_cdp_cache;
    // cdpr{Ratio}{$cdpid} -> IneryUserCDP
    IneryCdpRatioIndexCache          cdp_ratio_index_cache;
    IneryCdpHeightIndexCache         cdp_height_index_cache;
};

enum CDPCloseType: uint8_t {
    BY_REDEEM = 0,
    BY_MANUAL_LIQUIDATE,
    BY_FORCE_LIQUIDATE
};

string GetCdpCloseTypeName(const CDPCloseType type);

class CClosedCdpDBCache {
public:
    CClosedCdpDBCache() {}

    CClosedCdpDBCache(CDBAccess *pDbAccess) : closedCdpTxCache(pDbAccess), closedTxCdpCache(pDbAccess) {}

    CClosedCdpDBCache(CClosedCdpDBCache *pBaseIn)
        : closedCdpTxCache(&pBaseIn->closedCdpTxCache), closedTxCdpCache(&pBaseIn->closedTxCdpCache) {}

public:
    bool AddClosedCdpIndex(const uint256& closedCdpId, const uint256& closedCdpTxId, CDPCloseType closeType) {
        return closedCdpTxCache.SetData(closedCdpId, {closedCdpTxId, (uint8_t)closeType});
    }

    bool AddClosedCdpTxIndex(const uint256& closedCdpTxId, const uint256& closedCdpId, CDPCloseType closeType) {
        return  closedTxCdpCache.SetData(closedCdpTxId, {closedCdpId, closeType});
    }

    bool GetClosedCdpById(const uint256& closedCdpId, std::pair<uint256, uint8_t>& cdp) {
        return closedCdpTxCache.GetData(closedCdpId, cdp);
    }

    bool GetClosedCdpByTxId(const uint256& closedCdpTxId, std::pair<uint256, uint8_t>& cdp) {
        return closedTxCdpCache.GetData(closedCdpTxId, cdp);
    }

    uint32_t GetCacheSize() const { return closedCdpTxCache.GetCacheSize() + closedTxCdpCache.GetCacheSize(); }

    void SetBaseViewPtr(CClosedCdpDBCache *pBaseIn) {
        closedCdpTxCache.SetBase(&pBaseIn->closedCdpTxCache);
        closedTxCdpCache.SetBase(&pBaseIn->closedTxCdpCache);
    }

    void Flush() {
        closedCdpTxCache.Flush();
        closedTxCdpCache.Flush();
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        closedCdpTxCache.SetDbOpLogMap(pDbOpLogMapIn);
        closedTxCdpCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        closedCdpTxCache.RegisterUndoFunc(undoDataFuncMap);
        closedTxCdpCache.RegisterUndoFunc(undoDataFuncMap);
    }
public:
    /*  IneryCompositeKVCache     prefixType     key               value             variable  */
    /*  ----------------   --------------   ------------   --------------    ----- --------*/
    // ccdp${closed_cdpid} -> <closedCdpTxId, closeType>
    IneryCompositeKVCache< dbk::CLOSED_CDP_TX, uint256, std::pair<uint256, uint8_t> > closedCdpTxCache;
    // ctx${$closed_cdp_txid} -> <closedCdpId, closeType> (no-force-liquidation)
    IneryCompositeKVCache< dbk::CLOSED_TX_CDP, uint256, std::pair<uint256, uint8_t> > closedTxCdpCache;
};

#endif  // PERSIST_CDPDB_H
