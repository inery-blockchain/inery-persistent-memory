
#include "pricefeeddb.h"
#include "config/scoin.h"
#include "tx/pricefeedtx.h"
#include "basics/types.h"

static inline bool ReadSlideWindow(InerySysParamDBCache &sysParamCache, uint64_t &slideWindow, const char* pTitle) {
    if (!sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindow)) {
        return ERRORMSG("%s, read sys param MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error", pTitle);
    }
    return true;
}

void IneryConsecutiveBlockPrice::AddUserPrice(const HeightType blockHeight, const IneryRegID &regId, const uint64_t price) {
    mapBlockUserPrices[blockHeight][regId] = price;
}

void IneryConsecutiveBlockPrice::DeleteUserPrice(const HeightType blockHeight) {
    // Marked the value empty, the base cache will delete it when Flush() is called.
    mapBlockUserPrices[blockHeight].clear();
}

bool IneryConsecutiveBlockPrice::ExistBlockUserPrice(const HeightType blockHeight, const IneryRegID &regId) {
    if (mapBlockUserPrices.count(blockHeight) == 0)
        return false;

    return mapBlockUserPrices[blockHeight].count(regId);
}

////////////////////////////////////////////////////////////////////////////////
//IneryPricePointMemCache

bool IneryPricePointMemCache::ReleadBlocks(InerySysParamDBCache &sysParamCache, IneryBlockIndex *pTipBlockIdx) {

    int64_t start       = GetTimeMillis();
    IneryBlockIndex *pBlockIdx  = pTipBlockIdx;

    uint64_t slideWindow = 0;
    if (!ReadSlideWindow(sysParamCache, slideWindow, __func__)) return false;
    uint32_t count       = 0;

    while (pBlockIdx && count < slideWindow ) {
        IneryBlock block;
        if (!ReadBlockFromDisk(pBlockIdx, block))
            return ERRORMSG("read block=[%d]%s failed",pBlockIdx->height,
                    pBlockIdx->GetBlockHash().ToString());

        if (!AddPriceByBlock(block))
            return ERRORMSG("add block=[%d]%s to price point memory cache failed",
                    pBlockIdx->height, pBlockIdx->GetBlockHash().ToString());

        pBlockIdx = pBlockIdx->pprev;
        ++count;
    }
    LogPrint(BCLog::INFO, "Reload the latest %d blocks to price point memory cache (%d ms)\n", count, GetTimeMillis() - start);
    return true;
}

bool IneryPricePointMemCache::PushBlock(InerySysParamDBCache &sysParamCache, IneryBlockIndex *pTipBlockIdx) {

    uint64_t slideWindow = 0;
    if (!ReadSlideWindow(sysParamCache, slideWindow, __func__)) return false;
    // remove the oldest block
    if ((HeightType)pTipBlockIdx->height > (HeightType)slideWindow) {
        IneryBlockIndex *pDeleteBlockIndex = pTipBlockIdx;
        int idx           = slideWindow;
        while (pDeleteBlockIndex && idx-- > 0) {
            pDeleteBlockIndex = pDeleteBlockIndex->pprev;
        }

        if (pDeleteBlockIndex) {
            IneryBlock deleteBlock;
            if (!ReadBlockFromDisk(pDeleteBlockIndex, deleteBlock)) {
                return ERRORMSG("read block=[%d]%s failed", pDeleteBlockIndex->height,
                        pDeleteBlockIndex->GetBlockHash().ToString());
            }

            if (!DeleteBlockFromCache(deleteBlock)) {
                return ERRORMSG("delete block==[%d]%s from price point memory cache failed",
                        pDeleteBlockIndex->height, pDeleteBlockIndex->GetBlockHash().ToString());
            }
        } else {
            return ERRORMSG("[WARN]the slide index=%u block not found in block index",
                    idx);
        }
    }
    return true;
}

bool IneryPricePointMemCache::UndoBlock(InerySysParamDBCache &sysParamCache, IneryBlockIndex *pTipBlockIdx, IneryBlock &block) {

    uint64_t slideWindow = 0;
    if (!ReadSlideWindow(sysParamCache, slideWindow, __func__)) return false;
    // Delete the disconnected block's pricefeed items from price point memory cache.
    if (!DeleteBlockFromCache(block)) {
        return ERRORMSG("delete block=[%d]%s from price point memory cache failed",
                        pTipBlockIdx->height, pTipBlockIdx->GetBlockHash().ToString());
    }
    if ((HeightType)pTipBlockIdx->height > (HeightType)slideWindow) {
        IneryBlockIndex *pReLoadBlockIndex = pTipBlockIdx;
        int nCacheHeight           = slideWindow;
        while (pReLoadBlockIndex && nCacheHeight-- > 0) {
            pReLoadBlockIndex = pReLoadBlockIndex->pprev;
        }

        IneryBlock reLoadblock;
        if (!ReadBlockFromDisk(pReLoadBlockIndex, reLoadblock)) {
            return ERRORMSG("[%d] read block=%s failed", pReLoadBlockIndex->height,
                            pReLoadBlockIndex->GetBlockHash().ToString());
        }

        if (!AddPriceByBlock(reLoadblock)) {
            return ERRORMSG("[%d] add block=%s into price point memory cache failed",
                            pReLoadBlockIndex->height, pReLoadBlockIndex->GetBlockHash().ToString());
        }
    }
    return true;
}

bool IneryPricePointMemCache::AddPrice(const HeightType blockHeight, const IneryRegID &regId,
                                                    const vector<IneryPricePoint> &pps) {
    for (IneryPricePoint pp : pps) {
        if (ExistBlockUserPrice(blockHeight, regId, pp.GetCoinPricePair())) {
            LogPrint(BCLog::PRICEFEED,
                     "[%d] existing block user price, redId: %s, pricePoint: %s\n",
                     blockHeight, regId.ToString(), pp.ToString());

            return false;
        }

        IneryConsecutiveBlockPrice &cbp = mapCoinPricePointCache[pp.GetCoinPricePair()];
        cbp.AddUserPrice(blockHeight, regId, pp.GetPrice());
        LogPrint(BCLog::PRICEFEED,
                 "[%d] add block user price, redId: %s, pricePoint: %s\n",
                 blockHeight, regId.ToString(), pp.ToString());
    }

    return true;
}

bool IneryPricePointMemCache::ExistBlockUserPrice(const HeightType blockHeight, const IneryRegID &regId,
                                              const PriceCoinPair &coinPricePair) {
    if (mapCoinPricePointCache.count(coinPricePair) &&
        mapCoinPricePointCache[coinPricePair].ExistBlockUserPrice(blockHeight, regId))
        return true;

    if (pBase)
        return pBase->ExistBlockUserPrice(blockHeight, regId, coinPricePair);
    else
        return false;
}

bool IneryPricePointMemCache::AddPriceByBlock(const IneryBlock &block) {
    // index[0]: block reward transaction
    // index[1 ~ n - 1]: price feed transactions if existing
    // index[n]: block median price transaction

    if (block.vptx.size() < 3) {
        return true;
    }

    // More than 3 transactions in the block.
    for (uint32_t i = 1; i < block.vptx.size(); ++i) {
        if (!block.vptx[i]->IsPriceFeedTx()) {
            break;
        }

        IneryPriceFeedTx *priceFeedTx = (IneryPriceFeedTx *)block.vptx[i].get();
        AddPrice(block.GetHeight(), priceFeedTx->txUid.get<IneryRegID>(), priceFeedTx->price_points);
    }

    return true;
}

bool IneryPricePointMemCache::DeleteBlockFromCache(const IneryBlock &block) {
    set<PriceCoinPair> deletingSet;
    for (uint32_t i = 1; i < block.vptx.size(); ++i) {
        auto pBaseTx = block.vptx[i];
        if (!pBaseTx->IsPriceFeedTx()) {
            break;
        }

        IneryPriceFeedTx *priceFeedTx = dynamic_cast<IneryPriceFeedTx*>(pBaseTx.get());
        for (auto &pricePoint : priceFeedTx->price_points) {
            deletingSet.insert(pricePoint.GetCoinPricePair());
        }
    }
    auto height = block.GetHeight();
    for (auto &coinPair : deletingSet) {
        mapCoinPricePointCache[coinPair].mapBlockUserPrices[height].clear();
    }

    for (auto &item : mapCoinPricePointCache) {
        auto &blockPrices = item.second.mapBlockUserPrices[height];
        if (!blockPrices.empty()) {
            LogPrint(BCLog::ERROR, "[WARN] the price should be erased!, coin_pair=%s, height=%u\n",
                CoinPairToString(item.first), height);
        }
        blockPrices.clear();
    }

    return true;
}

void IneryPricePointMemCache::BatchWrite(const CoinPricePointMap &mapCoinPricePointCacheIn) {
    for (const auto &item : mapCoinPricePointCacheIn) {
        // map<HeightType /* block height */, map<IneryRegID, uint64_t /* price */>>
        const auto &mapBlockUserPrices = item.second.mapBlockUserPrices;
        for (const auto &userPrice : mapBlockUserPrices) {
            if (userPrice.second.empty()) {
                mapCoinPricePointCache[item.first /* PriceCoinPair */].mapBlockUserPrices.erase(userPrice.first /* height */);
            } else {
                // map<IneryRegID, uint64_t /* price */>;
                for (const auto &priceItem : userPrice.second) {
                    mapCoinPricePointCache[item.first /* PriceCoinPair */]
                        .mapBlockUserPrices[userPrice.first /* height */]
                        .emplace(priceItem.first /* IneryRegID */, priceItem.second /* price */);
                }
            }
        }
    }
}

void IneryPricePointMemCache::SetBaseViewPtr(IneryPricePointMemCache *pBaseIn) {
    pBase                        = pBaseIn;
}

void IneryPricePointMemCache::Flush() {
    assert(pBase);

    pBase->BatchWrite(mapCoinPricePointCache);
    mapCoinPricePointCache.clear();
}

bool IneryPricePointMemCache::GetBlockUserPrices(const PriceCoinPair &coinPricePair, set<HeightType> &expired,
                                             BlockUserPriceMap &blockUserPrices) {
    const auto &iter = mapCoinPricePointCache.find(coinPricePair);
    if (iter != mapCoinPricePointCache.end()) {
        const auto &mapBlockUserPrices = iter->second.mapBlockUserPrices;
        for (const auto &item : mapBlockUserPrices) {
            if (item.second.empty()) {
                expired.insert(item.first);
            } else if (expired.count(item.first) || blockUserPrices.count(item.first)) {
                // TODO: log
                continue;
            } else {
                // Got a valid item.
                blockUserPrices[item.first] = item.second;
            }
        }
    }

    if (pBase != nullptr) {
        return pBase->GetBlockUserPrices(coinPricePair, expired, blockUserPrices);
    }

    return true;
}

bool IneryPricePointMemCache::GetBlockUserPrices(const PriceCoinPair &coinPricePair, BlockUserPriceMap &blockUserPrices) {
    set<HeightType /* block height */> expired;
    if (!GetBlockUserPrices(coinPricePair, expired, blockUserPrices)) {
        // TODO: log
        return false;
    }

    return true;
}

CMedianPriceDetail IneryPricePointMemCache::ComputeBlockMedianPrice(const HeightType blockHeight, const uint64_t slideWindow,
                                                      const PriceCoinPair &coinPricePair) {
    // 1. merge block user prices with base cache.
    BlockUserPriceMap blockUserPrices;
    if (!GetBlockUserPrices(coinPricePair, blockUserPrices) || blockUserPrices.empty()) {
        return CMedianPriceDetail(); // {0, 0}
    }

    // 2. compute block median price.
    return ComputeBlockMedianPrice(blockHeight, slideWindow, blockUserPrices);
}

CMedianPriceDetail IneryPricePointMemCache::ComputeBlockMedianPrice(const HeightType blockHeight, const uint64_t slideWindow,
                                                      const BlockUserPriceMap &blockUserPrices) {
    // for (const auto &item : blockUserPrices) {
    //     string price;
    //     for (const auto &userPrice : item.second) {
    //         price += strprintf("{user:%s, price:%lld}", userPrice.first.ToString(), userPrice.second);
    //     }

    //     LogPrint(BCLog::PRICEFEED, "IneryPricePointMemCache::ComputeBlockMedianPrice, height: %d, userPrice: %s\n",
    //              item.first, price);
    // }

    CMedianPriceDetail priceDetail;
    vector<uint64_t> prices;
    HeightType beginBlockHeight = 0;
    if (blockHeight > slideWindow)
        beginBlockHeight = blockHeight - slideWindow;
    for (HeightType height = blockHeight; height > beginBlockHeight; --height) {
        const auto &iter = blockUserPrices.find(height);
        if (iter != blockUserPrices.end()) {
            if (height == blockHeight)
                priceDetail.last_feed_height = blockHeight; // current block has price feed

            for (const auto &userPrice : iter->second) {
                prices.push_back(userPrice.second);
            }
        }
    }

    // for (const auto &item : prices) {
    //     LogPrint(BCLog::PRICEFEED, "IneryPricePointMemCache::ComputeBlockMedianPrice, found a candidate price: %llu\n", item);
    // }

    priceDetail.price = ComputeMedianNumber(prices);
    LogPrint(BCLog::PRICEFEED, "[%d] computed median number: %llu\n", blockHeight, priceDetail.price);

    return priceDetail;
}

uint64_t IneryPricePointMemCache::ComputeMedianNumber(vector<uint64_t> &numbers) {
    uint32_t size = numbers.size();
    if (size < 2) {
        return size == 0 ? 0 : numbers[0];
    }
    sort(numbers.begin(), numbers.end());
    return (size % 2 == 0) ? (numbers[size / 2 - 1] + numbers[size / 2]) / 2 : numbers[size / 2];
}

CMedianPriceDetail IneryPricePointMemCache::GetMedianPrice(const HeightType blockHeight, const uint64_t slideWindow,
                                             const PriceCoinPair &coinPricePair) {
    CMedianPriceDetail priceDetail;
    priceDetail = ComputeBlockMedianPrice(blockHeight, slideWindow, coinPricePair);
    auto it = latest_median_prices.find(coinPricePair);
    if (it != latest_median_prices.end()) {
        if (priceDetail.price == 0) {
            priceDetail = it->second;
            LogPrint(BCLog::PRICEFEED,
                    "[%d] reuse pre-block-median-price: "
                    "coin_pair={%s}, new_price={%s}\n",
                    blockHeight, CoinPairToString(coinPricePair), priceDetail.ToString());

        } else if (priceDetail.last_feed_height != blockHeight) {
            priceDetail.last_feed_height = it->second.last_feed_height;

            LogPrint(BCLog::PRICEFEED, "[%d] current block has NO price feed! new_price={%s}\n",
                     blockHeight, priceDetail.ToString());
        }
    }

    return priceDetail;
}

bool IneryPricePointMemCache::CalcMedianPrices(CCacheWrapper &cw, const HeightType blockHeight, PriceMap &medianPrices) {

    PriceDetailMap priceDetailMap;
    if (!CalcMedianPriceDetails(cw, blockHeight, priceDetailMap))
        return false;

    for (auto item : priceDetailMap) {
        medianPrices.emplace(item.first, item.second.price);
    }
    return true;
}

bool IneryPricePointMemCache::CalcMedianPriceDetails(CCacheWrapper &cw, const HeightType blockHeight, PriceDetailMap &medianPrices)  {

    // TODO: support more price pair
    uint64_t slideWindow = 0;
    if (!ReadSlideWindow(cw.sysParamCache, slideWindow, __func__)) return false;

    latest_median_prices = cw.priceFeedCache.GetMedianPrices();

    set<PriceCoinPair> coinPairSet;
    if (cw.priceFeedCache.GetFeedCoinPairs(coinPairSet)) {
        // check the base asset has price feed permission
        for (auto it = coinPairSet.begin(); it != coinPairSet.end(); ) {
            CAsset asset;
            if (!cw.assetCache.CheckAsset(it->first, asset))
                return ERRORMSG("asset(%s) not exist", it->first);

            if (!asset.HasPerms(AssetPermType::PERM_PRICE_FEED)) {
                LogPrint(BCLog::PRICEFEED, "asset(%s) has no PERM_PRICE_FEED\n", it->first);
                it = coinPairSet.erase(it);
                continue;
            }
            it++;
        }
    }

    medianPrices.clear();
    for (const auto& item : coinPairSet) {
        CMedianPriceDetail INRMedianPrice = GetMedianPrice(blockHeight, slideWindow, item);
        if (INRMedianPrice.price == 0) {
            LogPrint(BCLog::PRICEFEED, "[%d] calc median price=0 of coin_pair={%s}, ignore\n",
                    blockHeight, CoinPairToString(item));
            continue;
        }
        medianPrices.emplace(item, INRMedianPrice);
        LogPrint(BCLog::PRICEFEED, "[%d] calc median price=%llu of coin_pair={%s}\n",
                blockHeight, INRMedianPrice.price, CoinPairToString(item));
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// IneryPriceFeedCache

uint64_t IneryPriceFeedCache::GetMedianPrice(const PriceCoinPair &coinPricePair) {
    PriceDetailMap medianPrices;
    if (median_price_cache.GetData(medianPrices)) {
        auto it = medianPrices.find(coinPricePair);
        if (it != medianPrices.end())
            return it->second.price;
    }
    return 0;
}

bool IneryPriceFeedCache::GetMedianPriceDetail(const PriceCoinPair &coinPricePair, CMedianPriceDetail &priceDetail) {
    PriceDetailMap medianPrices;
    if (median_price_cache.GetData(medianPrices)) {
        auto it = medianPrices.find(coinPricePair);
        if (it != medianPrices.end()) {
            priceDetail = it->second;
            return true;
        }
    }
    return false;
}

PriceDetailMap IneryPriceFeedCache::GetMedianPrices() const {
    PriceDetailMap medianPrices;
    if (median_price_cache.GetData(medianPrices)) {
        return medianPrices;
    }
    return {};
}

bool IneryPriceFeedCache::SetMedianPrices(const PriceDetailMap &medianPrices) {
    return median_price_cache.SetData(medianPrices);
}

bool IneryPriceFeedCache::AddFeedCoinPair(const PriceCoinPair &coinPair) {

    set<PriceCoinPair> coinPairs;
    GetFeedCoinPairs(coinPairs);
    if (coinPairs.count(coinPair) > 0)
        return true;

    coinPairs.insert(coinPair);
    return price_feed_coin_pairs_cache.SetData(coinPairs);
}

bool IneryPriceFeedCache::EraseFeedCoinPair(const PriceCoinPair &coinPair) {

    if (kPriceFeedCoinPairSet.count(coinPair))
        return true;

    set<PriceCoinPair> coins;
    price_feed_coin_pairs_cache.GetData(coins);
    if (coins.count(coinPair) == 0 )
        return true;

    coins.erase(coinPair);
    return price_feed_coin_pairs_cache.SetData(coins);
}

bool IneryPriceFeedCache::HasFeedCoinPair(const PriceCoinPair &coinPair) {
    set<PriceCoinPair> coins;
    price_feed_coin_pairs_cache.GetData(coins);
    return coins.count(coinPair);
}

bool IneryPriceFeedCache::GetFeedCoinPairs(set<PriceCoinPair>& coinPairSet) {
    return price_feed_coin_pairs_cache.GetData(coinPairSet);
}
