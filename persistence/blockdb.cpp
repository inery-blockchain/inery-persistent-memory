
#include "blockdb.h"
#include "objects/key.h"
#include "basics/uint256.h"
#include "basics/util/util.h"
#include "main.h"

#include <stdint.h>

using namespace std;

const IneryBlockInflatedReward IneryBlockInflatedReward::EMPTY = {};

/********************** IneryBlockIndexDB ********************************/

bool IneryBlockIndexDB::GetBlockIndex(const uint256 &hash, IneryDiskBlockIndex &blockIndex) {
    return Read(dbk::GenDbKey(dbk::BLOCK_INDEX, hash), blockIndex);
}

bool IneryBlockIndexDB::WriteBlockIndex(const IneryDiskBlockIndex &blockIndex) {
    return Write(dbk::GenDbKey(dbk::BLOCK_INDEX, blockIndex.GetBlockHash()), blockIndex);
}
bool IneryBlockIndexDB::EraseBlockIndex(const uint256 &blockHash) {
    return Erase(dbk::GenDbKey(dbk::BLOCK_INDEX, blockHash));
}

bool IneryBlockIndexDB::LoadBlockIndexes() {
    leveldb::Iterator *pCursor = NewIterator();
    const std::string &prefix = dbk::GetKeyPrefix(dbk::BLOCK_INDEX);

    pCursor->Seek(prefix);

    // Load mapBlockIndex
    while (pCursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pCursor->key();
            if (slKey.starts_with(prefix)) {
                leveldb::Slice slValue = pCursor->value();
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                IneryDiskBlockIndex diskIndex;
                ssValue >> diskIndex;

                // Construct block index object
                IneryBlockIndex *pIndexNew    = InsertBlockIndex(diskIndex.GetBlockHash());
                pIndexNew->pprev          = InsertBlockIndex(diskIndex.hashPrev);
                pIndexNew->height         = diskIndex.height;
                pIndexNew->nFile          = diskIndex.nFile;
                pIndexNew->nDataPos       = diskIndex.nDataPos;
                pIndexNew->nUndoPos       = diskIndex.nUndoPos;
                pIndexNew->nVersion       = diskIndex.nVersion;
                pIndexNew->nTime          = diskIndex.nTime;
                pIndexNew->nStatus        = diskIndex.nStatus;
                pIndexNew->nFuelFee       = diskIndex.nFuelFee;
                pIndexNew->nFuelRate      = diskIndex.nFuelRate;

                if (!pIndexNew->CheckIndex())
                    return ERRORMSG("LoadBlockIndex() : CheckIndex failed: %s", pIndexNew->ToString());

                pCursor->Next();
            } else {
                break;  // if shutdown requested or finished loading block index
            }
        } catch (std::exception &e) {
            return ERRORMSG("Deserialize or I/O error - %s", e.what());
        }
    }
    delete pCursor;

    return true;
}

bool IneryBlockIndexDB::WriteBlockFileInfo(int32_t nFile, const IneryBlockFileInfo &info) {
    return Write(dbk::GenDbKey(dbk::BLOCKFILE_NUM_INFO, nFile), info);
}
bool IneryBlockIndexDB::ReadBlockFileInfo(int32_t nFile, IneryBlockFileInfo &info) {
    return Read(dbk::GenDbKey(dbk::BLOCKFILE_NUM_INFO, nFile), info);
}

IneryBlockIndex *InsertBlockIndex(uint256 hash) {
    if (hash.IsNull())
        return nullptr;

    // Return existing
    map<uint256, IneryBlockIndex *>::iterator mi = mapBlockIndex.find(hash);
    if (mi != mapBlockIndex.end())
        return (*mi).second;

    // Create new
    IneryBlockIndex *pIndexNew = new IneryBlockIndex();
    if (!pIndexNew)
        throw runtime_error("new IneryBlockIndex failed");
    mi                    = mapBlockIndex.insert(make_pair(hash, pIndexNew)).first;
    pIndexNew->pBlockHash = &((*mi).first);

    return pIndexNew;
}


/************************* IneryBlockDBCache ****************************/
uint32_t IneryBlockDBCache::GetCacheSize() const {
    return
        tx_diskpos_cache.GetCacheSize() +
        flag_cache.GetCacheSize() +
        best_block_hash_cache.GetCacheSize() +
        last_block_file_cache.GetCacheSize() +
        reindex_cache.GetCacheSize() +
        finality_block_cache.GetCacheSize() +
        block_inflated_reward_cache.GetCacheSize();

}

bool IneryBlockDBCache::Flush() {
    tx_diskpos_cache.Flush();
    flag_cache.Flush();
    best_block_hash_cache.Flush();
    last_block_file_cache.Flush();
    reindex_cache.Flush();
    finality_block_cache.Flush();
    block_inflated_reward_cache.Flush();
    return true;
}

uint256 IneryBlockDBCache::GetBestBlockHash() const {
    uint256 blockHash;
    best_block_hash_cache.GetData(blockHash);
    return blockHash;
}

bool IneryBlockDBCache::SetBestBlock(const uint256 &blockHashIn) {
    return best_block_hash_cache.SetData(blockHashIn);
}

bool IneryBlockDBCache::WriteLastBlockFile(int32_t nFile) {
    return last_block_file_cache.SetData(nFile);
}
bool IneryBlockDBCache::ReadLastBlockFile(int32_t &nFile) {
    return last_block_file_cache.GetData(nFile);
}

bool IneryBlockDBCache::ReadTxIndex(const uint256 &txid, IneryDiskTxPos &pos) {
    return tx_diskpos_cache.GetData(txid, pos);
}

bool IneryBlockDBCache::SetTxIndex(const uint256 &txid, const IneryDiskTxPos &pos) {
    return tx_diskpos_cache.SetData(txid, pos);
}

bool IneryBlockDBCache::WriteTxIndexes(const vector<pair<uint256, IneryDiskTxPos> > &list) {
    for (auto it : list) {
        LogPrint(BCLog::DEBUG, "%-30s txid:%s dispos: nFile=%d, nPos=%d nTxOffset=%d\n",
                it.first.GetHex(), it.second.nFile, it.second.nPos, it.second.nTxOffset);

        if (!tx_diskpos_cache.SetData(it.first, it.second))
            return false;
    }
    return true;
}

bool IneryBlockDBCache::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return reindex_cache.SetData(true);
    else
        return reindex_cache.EraseData();
}
bool IneryBlockDBCache::ReadReindexing(bool &fReindexing) {
    return reindex_cache.GetData(fReindexing);
}

bool IneryBlockDBCache::WriteFlag(const string &name, bool fValue) {
    return flag_cache.SetData(name, fValue);
}
bool IneryBlockDBCache::ReadFlag(const string &name, bool &fValue) {
    return flag_cache.GetData(name, fValue);
}
bool IneryBlockDBCache::WriteGlobalFinBlock(const int32_t height, const uint256 hash) {
    finality_block_cache.SetData(std::make_pair(height, hash));
    return true;
}
bool IneryBlockDBCache::ReadGlobalFinBlock(std::pair<int32_t,uint256>& block) {
    return finality_block_cache.GetData(block);
}

bool IneryBlockDBCache::GetBlockInflatedReward(IneryBlockInflatedReward &value) {
    return block_inflated_reward_cache.GetData(value);
}

bool IneryBlockDBCache::SetBlockInflatedReward(const IneryBlockInflatedReward &value) {
    return block_inflated_reward_cache.SetData(value);
}