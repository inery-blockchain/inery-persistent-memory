
#include "blockundo.h"
#include "main.h"

/** Open an undo file (rev?????.dat) */
FILE *OpenUndoFile(const IneryDiskBlockPos &pos, bool fReadOnly) {
    return OpenDiskFile(pos, "rev", fReadOnly);
}

////////////////////////////////////////////////////////////////////////////////
// class IneryTxUndo
string IneryTxUndo::ToString() const {
    string str;
    str += "txid:" + txid.GetHex() + "\n";
    str += "db_oplog_map:" + dbOpLogMap.ToString();
    return str;
}

// uint256 IneryTxUndo::CalcStateHash(){
//     CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
//     hasher << dbOpLogMap.ToString();
//     return hasher.GetHash();
// }

////////////////////////////////////////////////////////////////////////////////
// class IneryBlockUndo


// uint256 IneryBlockUndo::CalcStateHash(uint256 preHash){
//     CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
//     hasher << preHash;
//     for(auto undo:vtxundo){
//         hasher << undo.CalcStateHash();
//     }
//     return hasher.GetHash();
// }

bool IneryBlockUndo::WriteToDisk(IneryDiskBlockPos &pos, const uint256 &blockHash) {
    // Open history file to append
    CAutoFile fileout = CAutoFile(OpenUndoFile(pos), SER_DISK, CLIENT_VERSION);
    if (!fileout)
        return ERRORMSG("IneryBlockUndo::WriteToDisk : OpenUndoFile failed");

    // Write index header
    uint32_t nSize = fileout.GetSerializeSize(*this);
    fileout << FLATDATA(SysCfg().MessageStart()) << nSize;

    // Write undo data
    long fileOutPos = ftell(fileout);
    if (fileOutPos < 0)
        return ERRORMSG("IneryBlockUndo::WriteToDisk : ftell failed");
    pos.nPos = (uint32_t)fileOutPos;
    fileout << *this;

    // calculate & write checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << blockHash;
    hasher << *this;

    fileout << hasher.GetHash();

    // Flush stdio buffers and commit to disk before returning
    fflush(fileout);
    if (!IsInitialBlockDownload())
        FileCommit(fileout);

    return true;
}

bool IneryBlockUndo::ReadFromDisk(const IneryDiskBlockPos &pos, const uint256 &blockHash) {
    // Open history file to read
    CAutoFile filein = CAutoFile(OpenUndoFile(pos, true), SER_DISK, CLIENT_VERSION);
    if (!filein)
        return ERRORMSG("IneryBlockUndo::ReadFromDisk : OpenBlockFile failed");

    // Read block
    uint256 hashChecksum;
    try {
        filein >> *this;
        filein >> hashChecksum;
    } catch (std::exception &e) {
        return ERRORMSG("Deserialize or I/O error - %s", e.what());
    }

    // Verify checksum
    CHashWriter hasher(SER_GETHASH, PROTOCOL_VERSION);
    hasher << blockHash;
    hasher << *this;

    if (hashChecksum != hasher.GetHash())
        return ERRORMSG("IneryBlockUndo::ReadFromDisk : Checksum mismatch");
    return true;
}

string IneryBlockUndo::ToString() const {
    string str;
    vector<IneryTxUndo>::const_iterator iterUndo = vtxundo.begin();
    for (; iterUndo != vtxundo.end(); ++iterUndo) {
        str += iterUndo->ToString();
    }
    return str;
}


////////////////////////////////////////////////////////////////////////////////
// class IneryBlockUndoExecutor

bool IneryBlockUndoExecutor::Execute() {
    // undoFuncMap
    // RegisterUndoFunc();
    const UndoDataFuncMap &undoDataFuncMap = cw.GetUndoDataFuncMap();

    for (auto it = block_undo.vtxundo.rbegin(); it != block_undo.vtxundo.rend(); it++) {
        for (const auto &opLogPair : it->dbOpLogMap.GetMap()) {
            dbk::PrefixType prefixType = dbk::ParseKeyPrefixType(opLogPair.first);
            if (prefixType == dbk::EMPTY)
                return ERRORMSG("%s(), unkown prefix! prefix_type=%s", __FUNCTION__,
                                opLogPair.first);
            auto funcMapIt = undoDataFuncMap.find(prefixType);
            if (funcMapIt == undoDataFuncMap.end()) {
                return ERRORMSG("%s(), unfound prefix in db! prefix_type=%s", __FUNCTION__,
                                opLogPair.first);
            }
            funcMapIt->second(opLogPair.second);
        }
    }
    return true;
}