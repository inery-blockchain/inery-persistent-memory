#include "accountdb.h"

#include <stdint.h>

using namespace std;

bool IneryAccountDBCache::GetAccount(const IneryKeyID &keyId, IneryAccount &account) const {
    return accountCache.GetData(keyId, account);
}

bool IneryAccountDBCache::GetAccount(const IneryRegID &regId, IneryAccount &account) const {
    if (regId.IsEmpty())
        return false;

    IneryKeyID keyId;
    if (regId2KeyIdCache.GetData(regId, keyId)) {
        return accountCache.GetData(keyId, account);
    }

    return false;
}

bool IneryAccountDBCache::GetAccount(const IneryUserID &userId, IneryAccount &account) const {
    bool ret = false;
    if (userId.is<IneryRegID>()) {
        ret = GetAccount(userId.get<IneryRegID>(), account);

    } else if (userId.is<IneryKeyID>()) {
        ret = GetAccount(userId.get<IneryKeyID>(), account);

    } else if (userId.is<IneryPubKey>()) {
        ret = GetAccount(userId.get<IneryPubKey>().GetKeyId(), account);

    } else if (userId.is<CNullID>()) {
        return ERRORMSG("GetAccount: userId can't be of CNullID type");
    }

    return ret;
}

bool IneryAccountDBCache::SetAccount(const IneryKeyID &keyId, const IneryAccount &account) {
    accountCache.SetData(keyId, account);
    return true;
}

bool IneryAccountDBCache::SetAccount(const IneryRegID &regId, const IneryAccount &account) {
    IneryKeyID keyId;
    if (regId2KeyIdCache.GetData(regId, keyId)) {
        return accountCache.SetData(keyId, account);
    }
    return false;
}

bool IneryAccountDBCache::HasAccount(const IneryKeyID &keyId) const {
    return accountCache.HasData(keyId);
}

bool IneryAccountDBCache::HasAccount(const IneryRegID &regId) const{
    return regId2KeyIdCache.HasData(regId);
}

bool IneryAccountDBCache::HasAccount(const IneryUserID &userId) const {
    if (userId.is<IneryRegID>()) {
        return HasAccount(userId.get<IneryRegID>());

    } else if (userId.is<IneryKeyID>()) {
        return HasAccount(userId.get<IneryKeyID>());

    } else if (userId.is<IneryPubKey>()) {
        return HasAccount(userId.get<IneryPubKey>().GetKeyId());

    } else if (userId.is<CNullID>()) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return false;
}

bool IneryAccountDBCache::EraseAccount(const IneryKeyID &keyId) {
    return accountCache.EraseData(keyId);
}

bool IneryAccountDBCache::NewRegId(const IneryRegID &regid, const IneryKeyID &keyId) {
    return regId2KeyIdCache.SetData(regid, keyId);
}

bool IneryAccountDBCache::GetKeyId(const IneryRegID &regId, IneryKeyID &keyId) const {
    return regId2KeyIdCache.GetData(regId, keyId);
}

bool IneryAccountDBCache::GetKeyId(const IneryUserID &userId, IneryKeyID &keyId) const {
    if (userId.is<IneryRegID>())
        return GetKeyId(userId.get<IneryRegID>(), keyId);

    if (userId.is<IneryPubKey>()) {
        keyId = userId.get<IneryPubKey>().GetKeyId();
        return true;
    }

    if (userId.is<IneryKeyID>()) {
        keyId = userId.get<IneryKeyID>();
        return true;
    }

    if (userId.is<CNullID>())
        return ERRORMSG("GetKeyId: userId can't be of CNullID type");

    return ERRORMSG("GetKeyId: userid type is unknown");
}

bool IneryAccountDBCache::EraseKeyId(const IneryRegID &regId) {
    return regId2KeyIdCache.EraseData(regId);
}

bool IneryAccountDBCache::SaveAccount(const IneryAccount &account) {
    if (!account.regid.IsEmpty())
        regId2KeyIdCache.SetData(IneryRegIDKey(account.regid), account.keyid);

    accountCache.SetData(account.keyid, account);

    return true;
}

bool IneryAccountDBCache::GetUserId(const string &addr, IneryUserID &userId) const {
    IneryRegID regId(addr);
    if (!regId.IsEmpty()) {
        userId = regId;
        return true;
    }

    IneryKeyID keyId(addr);
    if (!keyId.IsEmpty()) {
        userId = keyId;
        return true;
    }

    return false;
}

bool IneryAccountDBCache::GetRegId(const IneryKeyID &keyId, IneryRegID &regId) const {
    IneryAccount acct;
    if (accountCache.GetData(keyId, acct)) {
        regId = acct.regid;
        return true;
    }
    return false;
}

bool IneryAccountDBCache::GetRegId(const IneryUserID &userId, IneryRegID &regId) const {
    if (userId.is<IneryRegID>()) {
        regId = userId.get<IneryRegID>();

        return true;
    } else if (userId.is<IneryKeyID>()) {
        IneryAccount account;
        if (GetAccount(userId.get<IneryKeyID>(), account)) {
            regId = account.regid;

            return !regId.IsEmpty();
        }
    } else if (userId.is<IneryPubKey>()) {
        IneryAccount account;
        if (GetAccount(userId.get<IneryPubKey>().GetKeyId(), account)) {
            regId = account.regid;

            return !regId.IsEmpty();
        }
    }

    return false;
}

bool IneryAccountDBCache::SetAccount(const IneryUserID &userId, const IneryAccount &account) {
    if (userId.is<IneryRegID>()) {
        return SetAccount(userId.get<IneryRegID>(), account);

    } else if (userId.is<IneryKeyID>()) {
        return SetAccount(userId.get<IneryKeyID>(), account);

    } else if (userId.is<IneryPubKey>()) {
        return SetAccount(userId.get<IneryPubKey>().GetKeyId(), account);

    } else if (userId.is<CNullID>()) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return ERRORMSG("SetAccount input userid is unknow type");
}

bool IneryAccountDBCache::EraseAccount(const IneryUserID &userId) {
    if (userId.is<IneryKeyID>()) {
        return EraseAccount(userId.get<IneryKeyID>());
    } else if (userId.is<IneryPubKey>()) {
        return EraseAccount(userId.get<IneryPubKey>().GetKeyId());
    } else {
        return ERRORMSG("EraseAccount account type error!");
    }
    return false;
}

bool IneryAccountDBCache::EraseKeyId(const IneryUserID &userId) {
    if (userId.is<IneryRegID>()) {
        return EraseKeyId(userId.get<IneryRegID>());
    }

    return false;
}

uint64_t IneryAccountDBCache::GetAccountFreeAmount(const IneryKeyID &keyId, const TokenSymbol &tokenSymbol) {
    IneryAccount account;
    GetAccount(keyId, account);

    IneryAccountToken accountToken = account.GetToken(tokenSymbol);
    return accountToken.free_amount;
}

bool IneryAccountDBCache::Flush() {
    accountCache.Flush();
    regId2KeyIdCache.Flush();

    return true;
}

uint32_t IneryAccountDBCache::GetCacheSize() const {
    return accountCache.GetCacheSize() +
        regId2KeyIdCache.GetCacheSize();
}

Object IneryAccountDBCache::GetAccountDBStats() {
    uint64_t totalRegIds(0);
    uint64_t totalINRs(0);
    uint64_t totalSCoins(0);
    uint64_t totalFCoins(0);
    uint64_t INRsStates[5] = {0};
    uint64_t scoinsStates[5] = {0};
    uint64_t fcoinsStates[5] = {0};
    uint64_t totalReceivedVotes = 0;

    map<IneryKeyID, IneryAccount> items;
    CDbIterator it(accountCache);
    for (it.First(); it.IsValid(); it.Next()) {
        const IneryAccount &account = it.GetValue();
        totalRegIds++;

        IneryAccountToken wicc = account.GetToken(SYMB::WICC);
        IneryAccountToken wusd = account.GetToken(SYMB::WUSD);
        IneryAccountToken wgrt = account.GetToken(SYMB::WGRT);

        INRsStates[0] += wicc.free_amount;
        INRsStates[1] += wicc.voted_amount;
        INRsStates[2] += wicc.frozen_amount;
        INRsStates[3] += wicc.staked_amount;
        INRsStates[4] += wicc.pledged_amount;

        scoinsStates[0] += wusd.free_amount;
        scoinsStates[1] += wusd.voted_amount;
        scoinsStates[2] += wusd.frozen_amount;
        scoinsStates[3] += wusd.staked_amount;
        scoinsStates[4] += wusd.pledged_amount;

        fcoinsStates[0] += wgrt.free_amount;
        fcoinsStates[1] += wgrt.voted_amount;
        fcoinsStates[2] += wgrt.frozen_amount;
        fcoinsStates[3] += wgrt.staked_amount;
        fcoinsStates[4] += wgrt.pledged_amount;

        totalINRs += wicc.free_amount + wicc.voted_amount + wicc.frozen_amount + wicc.staked_amount + wicc.pledged_amount;
        totalSCoins += wusd.free_amount + wusd.voted_amount + wusd.frozen_amount + wusd.staked_amount + wusd.pledged_amount;
        totalFCoins += wgrt.free_amount + wgrt.voted_amount + wgrt.frozen_amount + wgrt.staked_amount + wgrt.pledged_amount;
        totalReceivedVotes += account.received_votes;
    }

    Object obj_wicc;
    obj_wicc.push_back(Pair("free_amount",      JsonValueFromAmount(INRsStates[0])));
    obj_wicc.push_back(Pair("voted_amount",     JsonValueFromAmount(INRsStates[1])));
    obj_wicc.push_back(Pair("frozen_amount",    JsonValueFromAmount(INRsStates[2])));
    obj_wicc.push_back(Pair("staked_amount",    JsonValueFromAmount(INRsStates[3])));
    obj_wicc.push_back(Pair("pledged_amount",   JsonValueFromAmount(INRsStates[4])));
    obj_wicc.push_back(Pair("total_amount",     JsonValueFromAmount(totalINRs)));

    Object obj_wusd;
    obj_wusd.push_back(Pair("free_amount",      JsonValueFromAmount(scoinsStates[0])));
    obj_wusd.push_back(Pair("voted_amount",     JsonValueFromAmount(scoinsStates[1])));
    obj_wusd.push_back(Pair("frozen_amount",    JsonValueFromAmount(scoinsStates[2])));
    obj_wusd.push_back(Pair("staked_amount",    JsonValueFromAmount(scoinsStates[3])));
    obj_wusd.push_back(Pair("pledged_amount",   JsonValueFromAmount(scoinsStates[4])));
    obj_wusd.push_back(Pair("total_amount",     JsonValueFromAmount(totalSCoins)));

    Object obj_wgrt;
    obj_wgrt.push_back(Pair("free_amount",      JsonValueFromAmount(fcoinsStates[0])));
    obj_wgrt.push_back(Pair("voted_amount",     JsonValueFromAmount(fcoinsStates[1])));
    obj_wgrt.push_back(Pair("frozen_amount",    JsonValueFromAmount(fcoinsStates[2])));
    obj_wgrt.push_back(Pair("staked_amount",    JsonValueFromAmount(fcoinsStates[3])));
    obj_wgrt.push_back(Pair("pledged_amount",   JsonValueFromAmount(fcoinsStates[4])));
    obj_wgrt.push_back(Pair("total_amount",     JsonValueFromAmount(totalFCoins)));

    Object obj;
    obj.push_back(Pair("WICC",          obj_wicc));
    obj.push_back(Pair("WUSD",          obj_wusd));
    obj.push_back(Pair("WGRT",          obj_wgrt));
    obj.push_back(Pair("total_received_votes",  totalReceivedVotes));
    obj.push_back(Pair("total_regids",  totalRegIds));

    return obj;

}

uint64_t IneryAccountDBCache::GetAssetTotalSupply(TokenSymbol assetSymbol) {
    uint64_t totalAssetCoins(0);

    CDbIterator it(accountCache);
    for (it.First(); it.IsValid(); it.Next()) {
        const IneryAccount &account = it.GetValue();

        IneryAccountToken assetCoind = account.GetToken(assetSymbol);

        totalAssetCoins += assetCoind.free_amount + assetCoind.voted_amount + assetCoind.frozen_amount + assetCoind.staked_amount + assetCoind.pledged_amount;
    }

    return totalAssetCoins;
}

