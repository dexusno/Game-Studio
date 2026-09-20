#pragma once
#include "overkill/campaign.hpp"
#include <istream>
#include <string>
#include <vector>

// Strict historical-input adapter, shared by the CPU preparation and Unreal
// replay. It does not select actions or implement any gameplay arithmetic.
namespace foundry_city_probe {
struct Step {
    overkill::CampaignAction action;
    std::string encodedAction,reason,hash,expectedBytes,receiptCommand;
    std::vector<std::string> events;
    overkill::CityPhase beforePhase=overkill::CityPhase::Arrival,afterPhase=overkill::CityPhase::Arrival;
    int beforePosition=0,afterPosition=0,round=0,hp=0;
    std::string encounter,mystery;
};
struct Plan {
    std::string rulesVersion,contentVersion,manifest,policyVersion,policy,precision;
    std::string runId,initialBytes,finalBytes,finalHash;
    std::uint64_t seed=0;
    std::size_t searchBudget=0;
    std::vector<Step> steps;
};
std::string unhex(const std::string& text);
overkill::CampaignAction decodeAction(const std::string& text);
Plan readPlan(std::istream& input,const overkill::CampaignRules& rules);
void verifyCommitted(const Step& expected,const overkill::Campaign& actual,bool ok,
                     const std::string& reason,const std::vector<overkill::Event>& events);
std::string actionName(const overkill::CampaignAction& action);
std::string phaseName(overkill::CityPhase phase);
std::string jsonQuote(const std::string& text);
std::string stepJson(std::size_t index,const Step& step,std::uint64_t revision);
} // namespace foundry_city_probe
