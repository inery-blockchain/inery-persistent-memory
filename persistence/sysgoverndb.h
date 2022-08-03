
#include <cstdint>
#include <unordered_set>
#include <string>
#include <cstdint>
#include <tuple>
#include <algorithm>

#include "basics/serialize.h"
#include "persistence/dbaccess.h"
#include "objects/proposal.h"


using namespace std;

class InerySysGovernDBCache {
public:
    InerySysGovernDBCache() {}
    InerySysGovernDBCache(CDBAccess *pDbAccess) : governors_cache(pDbAccess),
                                              proposals_cache(pDbAccess),
                                              approvals_cache(pDbAccess) {}
    InerySysGovernDBCache(InerySysGovernDBCache *pBaseIn) : governors_cache(pBaseIn->governors_cache),
                                                    proposals_cache(pBaseIn->proposals_cache),
                                                    approvals_cache(pBaseIn->approvals_cache) {};

    bool Flush() {
        governors_cache.Flush();
        proposals_cache.Flush();
        approvals_cache.Flush();
        return true;
    }

    uint32_t GetCacheSize() const {
        return governors_cache.GetCacheSize()
               + proposals_cache.GetCacheSize()
               + approvals_cache.GetCacheSize();
    }

    void SetBaseViewPtr(InerySysGovernDBCache *pBaseIn) {
        governors_cache.SetBase(&pBaseIn->governors_cache);
        proposals_cache.SetBase(&pBaseIn->proposals_cache);
        approvals_cache.SetBase(&pBaseIn->approvals_cache);
    }

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        governors_cache.SetDbOpLogMap(pDbOpLogMapIn);
        proposals_cache.SetDbOpLogMap(pDbOpLogMapIn);
        approvals_cache.SetDbOpLogMap(pDbOpLogMapIn);
        approvals_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }


    uint8_t GetGovernorApprovalMinCount(){

        set<IneryRegID> regids;
        if (GetGovernors(regids)) {
            return regids.size() - (regids.size() / 3);
        }

        throw runtime_error("get governer list error");
    }

    bool SetProposal(const uint256& txid,  shared_ptr<CProposal>& proposal) {
        return proposals_cache.SetData(txid, CProposalStorageBean(proposal));
    }

    bool GetProposal(const uint256& txid, shared_ptr<CProposal>& proposal) {

        CProposalStorageBean bean;
        if(proposals_cache.GetData(txid, bean) ){
            proposal = bean.sp_proposal;
            return true;
        }

        return false;
    }

    int GetApprovalCount(const uint256 &proposalId){
        vector<IneryRegID> v;
        approvals_cache.GetData(proposalId, v);
        return static_cast<int>(v.size());
    }

    bool GetApprovalList(const uint256& proposalId, vector<IneryRegID>& v){
        return  approvals_cache.GetData(proposalId, v);
    }

    bool SetApproval(const uint256 &proposalId, const IneryRegID &governor){

        vector<IneryRegID> v;
        if(approvals_cache.GetData(proposalId, v)){
            if(find(v.begin(),v.end(),governor) != v.end()){
                return ERRORMSG("governor(regid= %s) had approvaled this proposal(proposalid=%s)",
                                governor.ToString(), proposalId.ToString());
            }
        }
        v.push_back(governor);
        return approvals_cache.SetData(proposalId,v);
    }

    bool CheckIsGovernor(const IneryRegID &candidateRegId) {
        set<IneryRegID> regids;
        if(GetGovernors(regids)){
           return regids.count(candidateRegId) > 0;
        }

        return false;
    }

    bool GetGovernors(set<IneryRegID>& governors) {
        governors_cache.GetData(governors);
        if(governors.empty())
            governors.insert(IneryRegID(SysCfg().GetVer2GenesisHeight(), 2));

        return true;
    }

    bool AddGovernor(const IneryRegID governor){
        set<IneryRegID> governors;
        governors_cache.GetData(governors);
        governors.insert(governor);
        return governors_cache.SetData(governors);
    }

    bool EraseGovernor(const IneryRegID governor){
        set<IneryRegID> governors;
        governors_cache.GetData(governors);
        governors.erase(governor);
        return governors_cache.SetData(governors);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        governors_cache.RegisterUndoFunc(undoDataFuncMap);
        proposals_cache.RegisterUndoFunc(undoDataFuncMap);
        approvals_cache.RegisterUndoFunc(undoDataFuncMap);
    }

public:
/*  InerySimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    InerySimpleKVCache< dbk::SYS_GOVERN,            set<IneryRegID>>        governors_cache;    // list of governors

/*       type               prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    /////////// SysParamDB
    // pgvn{txid} -> proposal
    IneryCompositeKVCache< dbk::GOVN_PROP,             uint256,     CProposalStorageBean>          proposals_cache;
    // sgvn{txid}->vector(regid)
    IneryCompositeKVCache< dbk::GOVN_APPROVAL_LIST,    uint256,     vector<IneryRegID> >               approvals_cache;


};