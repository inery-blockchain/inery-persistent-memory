
#ifndef PERSIST_DELEGATEDB_H
#define PERSIST_DELEGATEDB_H

#include "objects/account.h"
#include "objects/id.h"
#include "objects/vote.h"
#include "basics/serialize.h"
#include "dbaccess.h"
#include "dbiterator.h"
#include "dbconf.h"

#include <map>
#include <set>
#include <vector>

using namespace std;

/*  IneryCompositeKVCache  prefixType     key                              value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // {vote(MAX - $votedINRs)}{$RegIdRawData} -> 1
    // vote(MAX - $votedINRs) save as CFixedUInt64 to ensure that the keys are sorted by vote value from big to small
    // $RegIdRawData must use raw data format of regid to be compatible old data
typedef IneryCompositeKVCache<dbk::VOTE,  std::pair<CFixedUInt64, UnsignedCharArray>,  uint8_t>         CVoteRegIdCache;

class CTopDelegatesIterator: public CDbIterator<CVoteRegIdCache> {
public:
    typedef CDbIterator<CVoteRegIdCache> Base;
    using Base::Base;

    uint64_t GetVote() const;

    IneryRegID GetRegId() const;
};

class IneryDelegateDBCache {
public:
    IneryDelegateDBCache() {}
    IneryDelegateDBCache(CDBAccess *pDbAccess)
        : voteRegIdCache(pDbAccess),
          regId2VoteCache(pDbAccess),
          last_vote_height_cache(pDbAccess),
          pending_delegates_cache(pDbAccess),
          active_delegates_cache(pDbAccess) {}

    IneryDelegateDBCache(IneryDelegateDBCache *pBaseIn)
        : voteRegIdCache(pBaseIn->voteRegIdCache),
        regId2VoteCache(pBaseIn->regId2VoteCache),
        last_vote_height_cache(pBaseIn->last_vote_height_cache),
        pending_delegates_cache(pBaseIn->pending_delegates_cache),
        active_delegates_cache(pBaseIn->active_delegates_cache) {}

    bool GetTopVoteDelegates(uint32_t delegateNum, uint64_t delegateVoteMin,
                             VoteDelegateVector &topVoteDelegates, bool isR3Fork);

    bool SetDelegateVotes(const IneryRegID &regid, const uint64_t votes);
    bool EraseDelegateVotes(const IneryRegID &regid, const uint64_t votes);

    int32_t GetLastVoteHeight();
    bool SetLastVoteHeight(int32_t height);
    uint32_t GetActivedDelegateNum();

    bool GetPendingDelegates(PendingDelegates &delegates);
    bool SetPendingDelegates(const PendingDelegates &delegates);

    bool IsActiveDelegate(const IneryRegID &regid);
    bool GetActiveDelegate(const IneryRegID &regid, VoteDelegate &voteDelegate);
    bool GetActiveDelegates(VoteDelegateVector &voteDelegates);
    bool SetActiveDelegates(const VoteDelegateVector &voteDelegates);

    bool SetCandidateVotes(const IneryRegID &regid, const vector<CCandidateReceivedVote> &candidateVotes);
    bool GetCandidateVotes(const IneryRegID &regid, vector<CCandidateReceivedVote> &candidateVotes);

    // There’s no reason to worry about performance issues as it will used only in stable coin genesis height.
    bool GetVoterList(map<IneryRegIDKey, vector<CCandidateReceivedVote>> &regId2Vote);

    bool Flush();
    uint32_t GetCacheSize() const;
    void Clear();

    void SetBaseViewPtr(IneryDelegateDBCache *pBaseIn) {
        voteRegIdCache.SetBase(&pBaseIn->voteRegIdCache);
        regId2VoteCache.SetBase(&pBaseIn->regId2VoteCache);
        last_vote_height_cache.SetBase(&pBaseIn->last_vote_height_cache);
        pending_delegates_cache.SetBase(&pBaseIn->pending_delegates_cache);
        active_delegates_cache.SetBase(&pBaseIn->active_delegates_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        voteRegIdCache.SetDbOpLogMap(pDbOpLogMapIn);
        regId2VoteCache.SetDbOpLogMap(pDbOpLogMapIn);
        last_vote_height_cache.SetDbOpLogMap(pDbOpLogMapIn);
        pending_delegates_cache.SetDbOpLogMap(pDbOpLogMapIn);
        active_delegates_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        voteRegIdCache.RegisterUndoFunc(undoDataFuncMap);
        regId2VoteCache.RegisterUndoFunc(undoDataFuncMap);
        last_vote_height_cache.RegisterUndoFunc(undoDataFuncMap);
        pending_delegates_cache.RegisterUndoFunc(undoDataFuncMap);
        active_delegates_cache.RegisterUndoFunc(undoDataFuncMap);
    }

    shared_ptr<CTopDelegatesIterator> CreateTopDelegateIterator();
public:
/*  IneryCompositeKVCache  prefixType     key                              value                   variable       */
/*  -------------------- -------------- --------------------------  ----------------------- -------------- */
    // {vote(MAX - $votedINRs)}{$RegId} -> 1
    // vote(MAX - $votedINRs) save as CFixedUInt64 to ensure that the keys are sorted by vote value from big to small
    CVoteRegIdCache voteRegIdCache;

    IneryCompositeKVCache<dbk::REGID_VOTE, IneryRegIDKey,         vector<CCandidateReceivedVote>> regId2VoteCache;

    InerySimpleKVCache<dbk::LAST_VOTE_HEIGHT, IneryVarIntValue<uint32_t>> last_vote_height_cache;
    InerySimpleKVCache<dbk::PENDING_DELEGATES, PendingDelegates> pending_delegates_cache;
    InerySimpleKVCache<dbk::ACTIVE_DELEGATES, VoteDelegateVector> active_delegates_cache;

    vector<IneryRegID> delegateRegIds;
};

#endif // PERSIST_DELEGATEDB_H