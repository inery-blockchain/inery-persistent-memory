
#include "logdb.h"
#include "config/chainparams.h"

bool IneryLogDBCache::SetExecuteFail(const int32_t blockHeight, const uint256 txid, const uint8_t errorCode,
                                 const string &errorMessage) {
    if (!SysCfg().IsLogFailures())
        return true;
    return executeFailCache.SetData(make_pair(CFixedUInt32(blockHeight), txid),
                                    std::make_pair(errorCode, errorMessage));
}

void IneryLogDBCache::Flush() { executeFailCache.Flush(); }
