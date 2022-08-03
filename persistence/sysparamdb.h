
#include "basics/serialize.h"
#include "persistence/dbaccess.h"
#include "persistence/dbconf.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <cstdint>
#include "config/sysparams.h"

using namespace std;

struct IneryCdpInterestParams {
    uint64_t param_a = 0;
    uint64_t param_b = 0;

    IMPLEMENT_SERIALIZE(
            READWRITE(VARINT(param_a));
            READWRITE(VARINT(param_b));
    )

    string ToString() const {
        return strprintf("param_a=%llu", param_a) + ", " +
        strprintf("param_a=%llu", param_a);

    }
};

typedef map<IneryVarIntValue<int32_t>, IneryCdpInterestParams> IneryCdpInterestParamChangeMap;

struct IneryCdpInterestParamChange {
    int32_t begin_height = 0;
    int32_t end_height = 0;
    uint64_t param_a = 0;
    uint64_t param_b = 0;
};

class InerySysParamDBCache {
public:
    InerySysParamDBCache() {}
    InerySysParamDBCache(CDBAccess *pDbAccess) : sys_param_chache(pDbAccess),
                                             producer_fee_cache(pDbAccess),
                                             cdp_param_cache(pDbAccess),
                                             cdp_interest_param_changes_cache(pDbAccess),
                                             current_total_bps_size_cache(pDbAccess),
                                             new_total_bps_size_cache(pDbAccess){}

    InerySysParamDBCache(InerySysParamDBCache *pBaseIn) : sys_param_chache(pBaseIn->sys_param_chache),
                                                  producer_fee_cache(pBaseIn->producer_fee_cache),
                                                  cdp_param_cache(pBaseIn->cdp_param_cache),
                                                  cdp_interest_param_changes_cache(pBaseIn->cdp_interest_param_changes_cache),
                                                  current_total_bps_size_cache(pBaseIn->current_total_bps_size_cache),
                                                  new_total_bps_size_cache(pBaseIn->new_total_bps_size_cache){}

    bool GetParam(const SysParamType &paramType, uint64_t& paramValue) {
        if (kSysParamTable.count(paramType) == 0)
            return false;

        auto iter = kSysParamTable.find(paramType);
        IneryVarIntValue<uint64_t > value;
        if (!sys_param_chache.GetData(paramType, value)) {
            paramValue = std::get<0>(iter->second);
        } else{
            paramValue = value.get();
        }

        return true;
    }

    bool GetCdpParam(const IneryCdpCoinPair& coinPair, const CdpParamType &paramType, uint64_t& paramValue) {
        if (kCdpParamTable.count(paramType) == 0)
            return false;

        auto iter = kCdpParamTable.find(paramType);
        auto key = std::make_pair(coinPair, paramType);
        IneryVarIntValue<uint64_t > value;
        if (!cdp_param_cache.GetData(key, value)) {
            paramValue = std::get<0>(iter->second);
        } else{
            paramValue = value.get();
        }

        return true;
    }

    bool Flush() {
        sys_param_chache.Flush();
        producer_fee_cache.Flush();
        cdp_param_cache.Flush();
        cdp_interest_param_changes_cache.Flush();
        current_total_bps_size_cache.Flush();
        new_total_bps_size_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const { return sys_param_chache.GetCacheSize() +
                                    producer_fee_cache.GetCacheSize() +
                cdp_interest_param_changes_cache.GetCacheSize() +
                                    current_total_bps_size_cache.GetCacheSize() +
                                    new_total_bps_size_cache.GetCacheSize(); }

    void SetBaseViewPtr(InerySysParamDBCache *pBaseIn) {
        sys_param_chache.SetBase(&pBaseIn->sys_param_chache);
        producer_fee_cache.SetBase(&pBaseIn->producer_fee_cache);
        cdp_param_cache.SetBase(&pBaseIn->cdp_param_cache);
        cdp_interest_param_changes_cache.SetBase(&pBaseIn->cdp_interest_param_changes_cache);
        current_total_bps_size_cache.SetBase(&pBaseIn->current_total_bps_size_cache);
        new_total_bps_size_cache.SetBase(&pBaseIn->new_total_bps_size_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        sys_param_chache.SetDbOpLogMap(pDbOpLogMapIn);
        producer_fee_cache.SetDbOpLogMap(pDbOpLogMapIn);
        cdp_param_cache.SetDbOpLogMap(pDbOpLogMapIn);
        cdp_interest_param_changes_cache.SetDbOpLogMap(pDbOpLogMapIn);
        current_total_bps_size_cache.SetDbOpLogMap(pDbOpLogMapIn);
        new_total_bps_size_cache.SetDbOpLogMap(pDbOpLogMapIn);

    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        sys_param_chache.RegisterUndoFunc(undoDataFuncMap);
        producer_fee_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_param_cache.RegisterUndoFunc(undoDataFuncMap);
        cdp_interest_param_changes_cache.RegisterUndoFunc(undoDataFuncMap);
        current_total_bps_size_cache.RegisterUndoFunc(undoDataFuncMap);
        new_total_bps_size_cache.RegisterUndoFunc(undoDataFuncMap);
    }
    bool SetParam(const SysParamType& key, const uint64_t& value){
        return sys_param_chache.SetData(key, IneryVarIntValue(value));
    }

    bool SetCdpParam(const IneryCdpCoinPair& coinPair, const CdpParamType& paramkey, const uint64_t& value) {
        auto key = std::make_pair(coinPair, paramkey);
        return cdp_param_cache.SetData(key, value);
    }
    bool SetProducerFee( const TxType txType, const string feeSymbol, const uint64_t feeSawiAmount) {

        auto pa = std::make_pair(txType, feeSymbol);
        return producer_fee_cache.SetData(pa , IneryVarIntValue(feeSawiAmount));

    }

    bool SetCdpInterestParam(IneryCdpCoinPair& coinPair, CdpParamType paramType, int32_t height , uint64_t value){

        IneryCdpInterestParamChangeMap changeMap;
        cdp_interest_param_changes_cache.GetData(coinPair, changeMap);
        auto &item = changeMap[IneryVarIntValue(height)];
        if (paramType == CdpParamType::CDP_INTEREST_PARAM_A) {
            item.param_a = value;
            uint64_t param_b;
            if(!GetCdpParam(coinPair,CDP_INTEREST_PARAM_B, param_b))
                return false;
            item.param_b = param_b;
        } else if (paramType == CdpParamType::CDP_INTEREST_PARAM_B) {
            item.param_b = value;
            uint64_t param_a;
            if(!GetCdpParam(coinPair,CDP_INTEREST_PARAM_A, param_a))
                return false;
            item.param_a = param_a;
        } else {
            assert(false && "must be param_a || param_b");
            return false;
        }
        return cdp_interest_param_changes_cache.SetData(coinPair, changeMap);
    }

    bool GetCdpInterestParamChanges(const IneryCdpCoinPair& coinPair, int32_t beginHeight, int32_t endHeight,
            list<IneryCdpInterestParamChange> &changes) {
        // must validate the coinPair before call this func
        changes.clear();
        IneryCdpInterestParamChangeMap changeMap;
        cdp_interest_param_changes_cache.GetData(coinPair, changeMap);

        auto it = changeMap.begin();
        auto beginChangeIt = changeMap.end();
        // Find out which change the beginHeight should belong to
        for (; it != changeMap.end(); it++) {
            if (it->first.get() > beginHeight) {
                break;
            }
            beginChangeIt = it;
        }
        // add the first change to the change list, make sure the change list not empty
        if (beginChangeIt == changeMap.end()) { // not found, use default value
            changes.push_back({
                beginHeight,
                0, // will be set later
                GetCdpParamDefaultValue(CDP_INTEREST_PARAM_A),
                GetCdpParamDefaultValue(CDP_INTEREST_PARAM_B)
            });
            beginChangeIt = changeMap.begin();
        } else { // found
            changes.push_back({
                beginHeight,
                0,  // will be set later
                beginChangeIt->second.param_a,
                beginChangeIt->second.param_b
            });

            beginChangeIt++;
        }

        for (it = beginChangeIt; it != changeMap.end(); it++) {
            if (it->first.get() > endHeight)
                break;
            assert(!changes.empty());
            changes.back().end_height = it->first.get() - 1;
            changes.push_back({
                it->first.get(),
                0, // will be set later
                it->second.param_a,
                it->second.param_b
            });
        }
        changes.back().end_height = endHeight;

        return true;
    }

    bool GetProducerFee( const uint8_t txType, const string feeSymbol, uint64_t& feeSawiAmount) {

        auto pa = std::make_pair(txType, feeSymbol);
        IneryVarIntValue<uint64_t > value;
        bool result =  producer_fee_cache.GetData(pa , value);

        if(result)
            feeSawiAmount = value.get();
        return result;
    }

    bool GetAxcSwapGwRegId(IneryRegID& regid) {
        uint64_t param;
        if(GetParam(SysParamType::AXC_SWAP_GATEWAY_REGID, param)) {
            regid = IneryRegID(param);
            return !regid.IsEmpty();
        }
        return false;
    }

    IneryRegID GetDexMatchSvcRegId() {
        uint64_t param;
        IneryRegID regid;
        if(GetParam(SysParamType::DEX_MATCH_SVC_REGID, param)) {
            regid = IneryRegID(param);
            if(regid.IsEmpty()){
                regid = SysCfg().GetDexMatchSvcRegId();
            }
        }
        return regid;
    }
public:
    bool SetNewTotalBpsSize(uint8_t newTotalBpsSize, uint32_t effectiveHeight) {
        return new_total_bps_size_cache.SetData(std::make_pair(IneryVarIntValue(effectiveHeight), newTotalBpsSize));
    }
    bool SetCurrentTotalBpsSize(uint8_t totalBpsSize) {

        return current_total_bps_size_cache.SetData(totalBpsSize);
    }

    uint8_t GetTotalBpsSize(uint32_t height) {

        pair<IneryVarIntValue<uint32_t>,uint8_t> value;
        if(new_total_bps_size_cache.GetData(value)){
            auto effectiveHeight = std::get<0>(value);
            if(height >= effectiveHeight.get()) {
                return std::get<1>(value);
            }
        }
        uint8_t totalBpsSize;
        if(current_total_bps_size_cache.GetData(totalBpsSize)){
            return totalBpsSize;
        }

        return IniCfg().GetTotalDelegateNum();
    }

public:


/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // order tx id -> active order
    IneryCompositeKVCache< dbk::SYS_PARAM,     uint8_t,      IneryVarIntValue<uint64_t> >               sys_param_chache;
    IneryCompositeKVCache< dbk::PRODUCER_FEE,     pair<uint8_t, string>,  IneryVarIntValue<uint64_t> >     producer_fee_cache;
    IneryCompositeKVCache< dbk::CDP_PARAM,     pair<IneryCdpCoinPair,uint8_t>, IneryVarIntValue<uint64_t> > cdp_param_cache;
    // [prefix]cdpCoinPair -> cdp_interest_param_changes (contain all changes)
    IneryCompositeKVCache< dbk::CDP_INTEREST_PARAMS, IneryCdpCoinPair, IneryCdpInterestParamChangeMap>      cdp_interest_param_changes_cache;
    InerySimpleKVCache<dbk:: TOTAL_BPS_SIZE, uint8_t>                                               current_total_bps_size_cache;
    InerySimpleKVCache<dbk:: NEW_TOTAL_BPS_SIZE, pair<IneryVarIntValue<uint32_t>,uint8_t>>              new_total_bps_size_cache;
};