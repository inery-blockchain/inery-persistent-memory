
#include "cdpdb.h"
#include "persistence/dbiterator.h"

IneryCdpDBCache::IneryCdpDBCache(CDBAccess *pDbAccess)
    : cdp_global_data_cache(pDbAccess),
      cdp_cache(pDbAccess),
      cdp_INR_cache(pDbAccess),
      user_cdp_cache(pDbAccess),
      cdp_ratio_index_cache(pDbAccess),
      cdp_height_index_cache(pDbAccess) {}

IneryCdpDBCache::IneryCdpDBCache(IneryCdpDBCache *pBaseIn)
    : cdp_global_data_cache(pBaseIn->cdp_global_data_cache),
      cdp_cache(pBaseIn->cdp_cache),
      cdp_INR_cache(pBaseIn->cdp_INR_cache),
      user_cdp_cache(pBaseIn->user_cdp_cache),
      cdp_ratio_index_cache(pBaseIn->cdp_ratio_index_cache),
      cdp_height_index_cache(pBaseIn->cdp_height_index_cache) {}

bool IneryCdpDBCache::NewCDP(const int32_t blockHeight, IneryUserCDP &cdp) {
    return cdp_cache.SetData(cdp.cdpid, cdp) &&
        user_cdp_cache.SetData(make_pair(IneryRegIDKey(cdp.owner_regid), cdp.GetCoinPair()), cdp.cdpid) &&
        SaveCdpIndexData(cdp);
}

bool IneryCdpDBCache::EraseCDP(const IneryUserCDP &oldCDP, const IneryUserCDP &cdp) {
    return cdp_cache.EraseData(cdp.cdpid) &&
        user_cdp_cache.EraseData(make_pair(IneryRegIDKey(cdp.owner_regid), cdp.GetCoinPair())) &&
        EraseCdpIndexData(oldCDP);
}

// Need to delete the old cdp(before updating cdp), then save the new cdp if necessary.
bool IneryCdpDBCache::UpdateCDP(const IneryUserCDP &oldCDP, const IneryUserCDP &newCDP) {
    assert(!newCDP.IsEmpty());
    return cdp_cache.SetData(newCDP.cdpid, newCDP) && EraseCdpIndexData(oldCDP) && SaveCdpIndexData(newCDP);
}

bool IneryCdpDBCache::UserHaveCdp(const IneryRegID &regid, const TokenSymbol &assetSymbol, const TokenSymbol &scoinSymbol) {
    return user_cdp_cache.HasData(make_pair(IneryRegIDKey(regid), IneryCdpCoinPair(assetSymbol, scoinSymbol)));
}

bool IneryCdpDBCache::GetCDPList(const IneryRegID &regid, vector<IneryUserCDP> &cdpList) {

    IneryRegIDKey prefixKey(regid);
    CDBPrefixIterator<decltype(user_cdp_cache), IneryRegIDKey> dbIt(user_cdp_cache, prefixKey);
    dbIt.First();
    for(dbIt.First(); dbIt.IsValid(); dbIt.Next()) {
        IneryUserCDP userCdp;
        if (!cdp_cache.GetData(dbIt.GetValue().value(), userCdp)) {
            return false; // has invalid data
        }

        cdpList.push_back(userCdp);
    }

    return true;
}

bool IneryCdpDBCache::GetCDP(const uint256 cdpid, IneryUserCDP &cdp) {
    return cdp_cache.GetData(cdpid, cdp);
}

// Attention: update cdp_cache and user_cdp_cache synchronously.
bool IneryCdpDBCache::SaveCDPToDB(const IneryUserCDP &cdp) {
    return cdp_cache.SetData(cdp.cdpid, cdp);
}

bool IneryCdpDBCache::EraseCDPFromDB(const IneryUserCDP &cdp) {
    return cdp_cache.EraseData(cdp.cdpid);
}

bool IneryCdpDBCache::SaveCdpIndexData(const IneryUserCDP &userCdp) {
    IneryCdpCoinPair cdpCoinPair = userCdp.GetCoinPair();
    IneryCdpGlobalData cdpGlobalData = GetCdpGlobalData(cdpCoinPair);

    cdpGlobalData.total_staked_assets += userCdp.total_staked_INRs;
    cdpGlobalData.total_owed_scoins += userCdp.total_owed_scoins;

    if (!cdp_global_data_cache.SetData(cdpCoinPair, cdpGlobalData))
        return false;

    if (!cdp_ratio_index_cache.SetData(MakeCdpRatioIndexKey(userCdp), userCdp))
        return false;

    if (!cdp_height_index_cache.SetData(MakeCdpHeightIndexKey(userCdp), userCdp))
        return false;
    return true;
}

bool IneryCdpDBCache::EraseCdpIndexData(const IneryUserCDP &userCdp) {
    IneryCdpCoinPair cdpCoinPair = userCdp.GetCoinPair();
    IneryCdpGlobalData cdpGlobalData = GetCdpGlobalData(cdpCoinPair);

    cdpGlobalData.total_staked_assets -= userCdp.total_staked_INRs;
    cdpGlobalData.total_owed_scoins -= userCdp.total_owed_scoins;

    cdp_global_data_cache.SetData(cdpCoinPair, cdpGlobalData);

    if (!cdp_ratio_index_cache.EraseData(MakeCdpRatioIndexKey(userCdp)))
        return false;

    if (!cdp_height_index_cache.EraseData(MakeCdpHeightIndexKey(userCdp)))
        return false;
    return true;
}

list<IneryUserCDP> IneryCdpDBCache::GetCdpListByCollateralRatio(const IneryCdpCoinPair &cdpCoinPair,
        const uint64_t collateralRatio, const uint64_t INRMedianPrice) {

    double ratio = (double(collateralRatio) / RATIO_BOOST) / (double(INRMedianPrice) / PRICE_BOOST);
    assert(uint64_t(ratio * CDP_BASE_RATIO_BOOST) < UINT64_MAX);
    uint64_t ratioBoost = uint64_t(ratio * CDP_BASE_RATIO_BOOST);
    list<IneryUserCDP> cdpList;
    auto dbIt = MakeDbPrefixIterator(cdp_ratio_index_cache, cdpCoinPair);
    for (dbIt->First(); dbIt->IsValid(); dbIt->Next()) {
        if (std::get<1>(dbIt->GetKey()).value > ratioBoost) {
            break;
        }
        cdpList.push_back(dbIt->GetValue());
    }
    return cdpList;
}

IneryCdpGlobalData IneryCdpDBCache::GetCdpGlobalData(const IneryCdpCoinPair &cdpCoinPair) const {
    IneryCdpGlobalData ret;
    cdp_global_data_cache.GetData(cdpCoinPair, ret);
    return ret;
}

bool IneryCdpDBCache::GetCdpINR(const TokenSymbol &INRSymbol, IneryCdpINRDetail &cdpINR) {
    auto it = kCdpINRSymbolSet.find(INRSymbol);
    if (it != kCdpINRSymbolSet.end()) {
        cdpINR.status = CdpINRStatus::STAKE_ON;
        cdpINR.init_tx_cord = IneryRegID(0, 1);
        return true;
    }
    if (INRSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(INRSymbol) > 0) {
        return false;
    }
    return cdp_INR_cache.GetData(INRSymbol, cdpINR);
}

bool IneryCdpDBCache::IsCdpINRActivated(const TokenSymbol &INRSymbol) {
    if (kCdpINRSymbolSet.count(INRSymbol) > 0) return true;
    if (INRSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(INRSymbol) > 0) return false;
    return cdp_INR_cache.HasData(INRSymbol);
}

bool IneryCdpDBCache::SetCdpINR(const TokenSymbol &INRSymbol, const IneryCdpINRDetail &cdpINR) {
    if (kCdpINRSymbolSet.count(INRSymbol) > 0) return false;
    if (INRSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(INRSymbol) > 0) return false;
    IneryCdpINRDetail oldCdpINR;
    if (cdp_INR_cache.GetData(INRSymbol, oldCdpINR)) {
        oldCdpINR.status = cdpINR.status; // just update the status
        return cdp_INR_cache.SetData(INRSymbol, oldCdpINR);
    } else {
        return cdp_INR_cache.SetData(INRSymbol, cdpINR);
    }
}

void IneryCdpDBCache::SetBaseViewPtr(IneryCdpDBCache *pBaseIn) {
    cdp_global_data_cache.SetBase(&pBaseIn->cdp_global_data_cache);
    cdp_cache.SetBase(&pBaseIn->cdp_cache);
    cdp_INR_cache.SetBase(&pBaseIn->cdp_INR_cache);
    user_cdp_cache.SetBase(&pBaseIn->user_cdp_cache);
    cdp_ratio_index_cache.SetBase(&pBaseIn->cdp_ratio_index_cache);
    cdp_height_index_cache.SetBase(&pBaseIn->cdp_height_index_cache);
}

void IneryCdpDBCache::SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
    cdp_global_data_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_INR_cache.SetDbOpLogMap(pDbOpLogMapIn);
    user_cdp_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_ratio_index_cache.SetDbOpLogMap(pDbOpLogMapIn);
    cdp_height_index_cache.SetDbOpLogMap(pDbOpLogMapIn);
}

uint32_t IneryCdpDBCache::GetCacheSize() const {
    return cdp_global_data_cache.GetCacheSize() + cdp_cache.GetCacheSize() + cdp_INR_cache.GetCacheSize() +
            user_cdp_cache.GetCacheSize() + cdp_ratio_index_cache.GetCacheSize() + cdp_height_index_cache.GetCacheSize();
}

bool IneryCdpDBCache::Flush() {
    cdp_global_data_cache.Flush();
    cdp_cache.Flush();
    cdp_INR_cache.Flush();
    user_cdp_cache.Flush();
    cdp_ratio_index_cache.Flush();
    cdp_height_index_cache.Flush();

    return true;
}

IneryCdpRatioIndexCache::KeyType IneryCdpDBCache::MakeCdpRatioIndexKey(const IneryUserCDP &cdp) {

    uint64_t boostedRatio = cdp.collateral_ratio_base * CDP_BASE_RATIO_BOOST;
    uint64_t ratio        = (boostedRatio < cdp.collateral_ratio_base /* overflown */) ? UINT64_MAX : boostedRatio;
    return { cdp.GetCoinPair(), CFixedUInt64(ratio), CFixedUInt64(cdp.block_height), cdp.cdpid };
}

IneryCdpHeightIndexCache::KeyType IneryCdpDBCache::MakeCdpHeightIndexKey(const IneryUserCDP &cdp) {

    return {cdp.GetCoinPair(), CFixedUInt64(cdp.block_height), cdp.cdpid};
}

string GetCdpCloseTypeName(const CDPCloseType type) {
    switch (type) {
        case CDPCloseType:: BY_REDEEM:
            return "redeem";
        case CDPCloseType:: BY_FORCE_LIQUIDATE :
            return "force_liquidate";
        case CDPCloseType ::BY_MANUAL_LIQUIDATE:
            return "manual_liquidate";
    }
    return "undefined";
}