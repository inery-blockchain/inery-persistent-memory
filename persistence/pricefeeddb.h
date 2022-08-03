
#ifndef PERSIST_PRICEFEED_H
#define PERSIST_PRICEFEED_H

#include "block.h"
#include "basics/serialize.h"
#include "objects/account.h"
#include "objects/asset.h"
#include "objects/id.h"
#include "objects/price.h"
#include "tx/tx.h"
#include "persistence/dbaccess.h"

#include <map>
#include <string>
#include <vector>

using namespace std;

class InerySysParamDBCache; // need to read slide window param
class IneryConsecutiveBlockPrice;

typedef map<HeightType /* block height */, map<IneryRegID, uint64_t /* price */>> BlockUserPriceMap;
typedef map<PriceCoinPair, IneryConsecutiveBlockPrice> CoinPricePointMap;

// Price Points in 11 consecutive blocks
class IneryConsecutiveBlockPrice {
public:
    void AddUserPrice(const HeightType blockHeight, const IneryRegID &regId, const uint64_t price);
    // delete user price by specific block height.
    void DeleteUserPrice(const HeightType blockHeight);
    bool ExistBlockUserPrice(const HeightType blockHeight, const IneryRegID &regId);

public:
    BlockUserPriceMap mapBlockUserPrices;
};

class IneryPricePointMemCache {
public:
    IneryPricePointMemCache() : pBase(nullptr) {}
    IneryPricePointMemCache(IneryPricePointMemCache *pBaseIn)
        : pBase(pBaseIn) {}

public:
    bool ReleadBlocks(InerySysParamDBCache &sysParamCache, IneryBlockIndex *pTipBlockIdx);
    bool PushBlock(InerySysParamDBCache &sysParamCache, IneryBlockIndex *pTipBlockIdx);
    bool UndoBlock(InerySysParamDBCache &sysParamCache, IneryBlockIndex *pTipBlockIdx, IneryBlock &block);
    bool AddPrice(const HeightType blockHeight, const IneryRegID &regId, const vector<IneryPricePoint> &pps);

    bool CalcMedianPrices(CCacheWrapper &cw, const HeightType blockHeight, PriceMap &medianPrices);
    bool CalcMedianPriceDetails(CCacheWrapper &cw, const HeightType blockHeight, PriceDetailMap &medianPrices);

    void SetBaseViewPtr(IneryPricePointMemCache *pBaseIn);
    void Flush();

private:
    CMedianPriceDetail GetMedianPrice(const HeightType blockHeight, const uint64_t slideWindow, const PriceCoinPair &coinPricePair);

    bool AddPriceByBlock(const IneryBlock &block);
    // delete block price point by specific block height.
    bool DeleteBlockFromCache(const IneryBlock &block);

    bool ExistBlockUserPrice(const HeightType blockHeight, const IneryRegID &regId, const PriceCoinPair &coinPricePair);

    void BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn);

    bool GetBlockUserPrices(const PriceCoinPair &coinPricePair, set<HeightType> &expired, BlockUserPriceMap &blockUserPrices);
    bool GetBlockUserPrices(const PriceCoinPair &coinPricePair, BlockUserPriceMap &blockUserPrices);

    CMedianPriceDetail ComputeBlockMedianPrice(const HeightType blockHeight, const uint64_t slideWindow,
                                     const PriceCoinPair &coinPricePair);
    CMedianPriceDetail ComputeBlockMedianPrice(const HeightType blockHeight, const uint64_t slideWindow,
                                     const BlockUserPriceMap &blockUserPrices);
    static uint64_t ComputeMedianNumber(vector<uint64_t> &numbers);

private:
    CoinPricePointMap mapCoinPricePointCache;  // coinPriceType -> consecutiveBlockPrice
    IneryPricePointMemCache *pBase;
    PriceDetailMap latest_median_prices;

};

class IneryPriceFeedCache {
public:
    IneryPriceFeedCache() {}
    IneryPriceFeedCache(CDBAccess *pDbAccess)
    : price_feed_coin_pairs_cache(pDbAccess),
      median_price_cache(pDbAccess) {};
public:
    bool Flush() {
        price_feed_coin_pairs_cache.Flush();
        median_price_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return  price_feed_coin_pairs_cache.GetCacheSize() +
                median_price_cache.GetCacheSize();
    }
    void SetBaseViewPtr(IneryPriceFeedCache *pBaseIn) {
        price_feed_coin_pairs_cache.SetBase(&pBaseIn->price_feed_coin_pairs_cache);
        median_price_cache.SetBase(&pBaseIn->median_price_cache);
    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        price_feed_coin_pairs_cache.SetDbOpLogMap(pDbOpLogMapIn);
        median_price_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        price_feed_coin_pairs_cache.RegisterUndoFunc(undoDataFuncMap);
        median_price_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    bool AddFeedCoinPair(const PriceCoinPair &coinPair);
    bool EraseFeedCoinPair(const PriceCoinPair &coinPair);
    bool HasFeedCoinPair(const PriceCoinPair &coinPair);
    bool GetFeedCoinPairs(set<PriceCoinPair>& coinPairSet);

    bool CheckIsPriceFeeder(const IneryRegID &candidateRegId);
    bool SetPriceFeeders(const vector<IneryRegID> &governors);
    bool GetPriceFeeders(vector<IneryRegID>& priceFeeders);

    uint64_t GetMedianPrice(const PriceCoinPair &coinPricePair);
    bool GetMedianPriceDetail(const PriceCoinPair &coinPricePair, CMedianPriceDetail &priceDetail);
    PriceDetailMap GetMedianPrices() const;
    bool SetMedianPrices(const PriceDetailMap &medianPrices);

public:

/*  InerySimpleKVCache          prefixType          value           variable           */
/*  -------------------- --------------------  -------------   --------------------- */
    /////////// PriceFeedDB
    // [prefix] -> feed pair
    InerySimpleKVCache< dbk::PRICE_FEED_COIN_PAIRS, set<PriceCoinPair>>   price_feed_coin_pairs_cache;
    // [prefix] -> median price map
    InerySimpleKVCache< dbk::MEDIAN_PRICES,     PriceDetailMap>     median_price_cache;
};

#endif  // PERSIST_PRICEFEED_H