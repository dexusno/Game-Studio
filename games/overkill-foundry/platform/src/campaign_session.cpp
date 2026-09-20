#include "overkill/campaign_session.hpp"
#include <stdexcept>

namespace overkill {
const Campaign& CampaignSession::state() const {
    if(!loaded_)throw std::logic_error("No verified campaign is loaded.");
    return campaign_;
}
bool CampaignSession::accept(const StoredSave& stored,std::string& error){
    if(!stored.ok){error=stored.error;return false;}
    Campaign parsed;
    if(!deserializeCampaign(stored.payload,parsed,error))return false;
    if(parsed.manifestHash!=rules_.content().manifestHash){error="Saved campaign requires an explicit content migration.";return false;}
    if(parsed.preStart){error="An internal fight-entry checkpoint cannot be loaded as the current campaign.";return false;}
    campaign_=std::move(parsed);token_=stored.token;revision_=stored.revision;loaded_=true;return true;
}
SessionResult CampaignSession::load(){
    const auto stored=store_.read();std::string error;
    if(!accept(stored,error)){loaded_=false;return {false,false,false,false,error,{}};}
    return {true,false,false,stored.recoveredPrevious,"Loaded committed campaign.",{}};
}
SessionResult CampaignSession::newGame(std::uint64_t seed,const std::string& runId,const std::function<void(SavePoint)>& faultProbe){
    try{
        if(loaded_ && campaign_.runId==runId){
            if(campaign_.seed!=seed)return {false,false,false,false,"New Game identity was reused with another seed.",{}};
            return {true,true,false,false,"This New Game already exists.",{}};
        }
        auto candidate=rules_.newGame(seed,runId,loaded_?campaign_.profile:ProfileFacts{});
        return persist(std::move(candidate),{true,false,"New Game committed.",{}},nullptr,faultProbe);
    }catch(const std::exception& e){return {false,false,false,false,e.what(),{}};}
}
SessionResult CampaignSession::apply(const CampaignAction& action,const std::function<void(SavePoint)>& faultProbe){
    if(!loaded_)return {false,false,false,false,"Load a verified campaign first.",{}};
    auto candidate=campaign_;auto result=rules_.apply(candidate,action);
    if(!result.ok || result.replayed)return {result.ok,result.replayed,false,false,result.reason,result.events};
    return persist(std::move(candidate),std::move(result),&action,faultProbe);
}
SessionResult CampaignSession::persist(Campaign candidate,CampaignResult result,const CampaignAction* action,
                                       const std::function<void(SavePoint)>& faultProbe){
    try{
        const auto payload=serializeCampaign(candidate);const auto desiredRun=candidate.runId;
        const auto committed=store_.commit(payload,loaded_?token_:std::string{},faultProbe);
        if(committed.ok){
            campaign_=std::move(candidate);token_=committed.token;revision_=committed.revision;loaded_=true;
            return {true,result.replayed,false,false,result.reason,std::move(result.events)};
        }
        // Replacement may have happened before an IO error. Read the latest
        // durable envelope and reconcile the exact operation, never retry a
        // speculative purchase or publish success based only on its local copy.
        const auto latest=store_.read();std::string error;
        if(!accept(latest,error)){
            loaded_=false;return {false,false,true,false,committed.error+" Reload failed: "+error,{}};
        }
        if(latest.payload==payload)return {true,result.replayed,true,latest.recoveredPrevious,result.reason,std::move(result.events)};
        if(action && campaign_.runId==desiredRun){
            auto probe=campaign_;const auto receipt=rules_.apply(probe,*action);
            if(receipt.ok && receipt.replayed)return {true,true,true,latest.recoveredPrevious,receipt.reason,receipt.events};
        }
        return {false,false,true,latest.recoveredPrevious,committed.error+" Current committed state was reloaded; the requested transaction was not confirmed.",{}};
    }catch(const std::exception& e){return {false,false,false,false,e.what(),{}};}
}
} // namespace overkill
