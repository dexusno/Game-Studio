#include "FoundryProfileProbe.h"
#if FOUNDRY_WITH_CAMPAIGN
#include "FoundryCampaign.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include <stdexcept>

bool RunFoundryProfileProbe(FString& Report)
{
    int32 Checks=0;
    try
    {
        auto Check=[&](bool Ok,const char* Label){++Checks;if(!Ok)throw std::runtime_error(Label);};
        const FString Root=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("ProfileProbe"),FGuid::NewGuid().ToString(EGuidFormats::Digits)));
        const FString Path=FPaths::Combine(Root,TEXT("profile.ofsave"));
        FFoundrySession Combat(1);FFoundryCampaign Model(Combat,Path,false);
        Check(!Model.Current()&&!Model.bFixedSavePath,"fresh profile host does not create a campaign");
        Model.Navigate(TEXT("profiles"));
        Check(Model.CreateProfile(TEXT("Engine Alpha")),"actual UE GUID profile creation succeeds");
        const auto Alpha=Model.Store.activeId();
        Check(Alpha.size()==32&&Alpha.find_first_not_of("0123456789abcdef")==std::string::npos,"generated profile identity is canonical lowercase ASCII");
        Check(Model.Page==TEXT("title")&&!Model.Current()&&!Model.HasActiveSave(),"creating and selecting does not start a run");
        Check(Model.StartNew(false,3002),"ordinary first New Game succeeds");
        const auto Hash=overkill::campaignHash(*Model.Current());const auto Revision=Model.Store.revision();
        Check(!Model.StartNew(false,3003)&&Model.Page==TEXT("confirm-new"),"unfinished run requires explicit replacement confirmation");
        Model.Close();
        Check(Model.Page==TEXT("title")&&overkill::campaignHash(*Model.Current())==Hash&&Model.Store.revision()==Revision,"cancel replacement preserves exact committed state");
        Model.Navigate(TEXT("collection"));Model.Close();
        Check(Model.Page==TEXT("title")&&overkill::campaignHash(*Model.Current())==Hash&&Model.Store.revision()==Revision,"Collection navigation performs no campaign transaction");
        Model.Navigate(TEXT("profiles"));
        Check(Model.CreateProfile(TEXT("Engine Beta")),"second independent UE profile creates");
        const auto Beta=Model.Store.activeId();
        Check(Alpha!=Beta&&!Model.Current()&&Combat.State.memory.empty(),"empty profile clears previous campaign read model");
        Check(Model.StartNew(false,3004),"second profile begins its own run");
        const auto BetaRun=Model.Current()->runId;
        Model.Navigate(TEXT("profiles"));Check(Model.SelectProfile(Alpha),"switch back to first profile");
        Check(overkill::campaignHash(*Model.Current())==Hash&&!Model.bCanResumeInMemory,"selection preserves first run and does not bypass Continue policy");
        Check(Model.Continue()&&Model.Page==TEXT("arrival"),"Continue restores exact between-fight phase");
        Model.Navigate(TEXT("profiles"));Check(Model.SelectProfile(Beta)&&Model.Current()->runId==BetaRun,"second campaign remained independent");
        FFoundrySession FixedCombat(2);FFoundryCampaign Fixed(FixedCombat,Path);
        Fixed.Navigate(TEXT("profiles"));
        Check(Fixed.bFixedSavePath&&!Fixed.CreateProfile(TEXT("Forbidden"))&&!Fixed.SelectProfile(Alpha),"existing fixed-path constructor bypasses profile mutation");
        Check(!Fixed.Current(),"fixed path ignores selected independent campaign");
        Report=FString::Printf(TEXT("%d engine profile checks passed; isolated SaveStore path=%s"),Checks,*Root);
        return true;
    }
    catch(const std::exception& Error)
    {Report=FString::Printf(TEXT("Profile check %d failed: %s"),Checks,UTF8_TO_TCHAR(Error.what()));return false;}
}
#endif
