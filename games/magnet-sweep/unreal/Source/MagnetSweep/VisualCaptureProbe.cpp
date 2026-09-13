#include "VisualCaptureProbe.h"
#include "ExpeditionRuntime.h"
#include "ExpeditionRig.h"
#include "ExpeditionWorld.h"
#include "WorkbenchRuntime.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
struct FVisualProbe
{
    FExpeditionRuntime* Runtime=nullptr;
    FString Directory,PendingFile,Error,SelectedId;
    int32 Stage=0,Frames=0;
    double Started=0,StageStarted=0,Requested=0;
    bool bRequested=false,bDone=false,bExtended=false;
    TArray<TSharedPtr<FJsonValue>> Captures;

    void Report(const TCHAR* Status)
    {
        auto Json=MakeShared<FJsonObject>();
        Json->SetNumberField(TEXT("schema"),1);
        Json->SetStringField(TEXT("status"),Status);
        Json->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
        Json->SetStringField(TEXT("profile"),Runtime->Profile);
        Json->SetStringField(TEXT("evidence"),TEXT("First three captures use actual starter/depot/depart callbacks. Optional captures four and five are unearned late-site rendering fixtures made with StartSite(index,42) and the unchanged starting rig. No earned-run, native-input, sound or enjoyment claim."));
        Json->SetBoolField(TEXT("extended"),bExtended);
        Json->SetNumberField(TEXT("expected_captures"),bExtended?5:3);
        Json->SetStringField(TEXT("screenshot_api"),TEXT("FScreenshotRequest; includeUI=true; restrictToGameViewport=true; PNG completion checked before advancing"));
        Json->SetStringField(TEXT("selected_module"),SelectedId);
        if(Runtime->Rig)Json->SetObjectField(TEXT("captured_rig"),Runtime->Rig->ToJson());
        Json->SetBoolField(TEXT("owner_profile_loaded"),false);
        Json->SetBoolField(TEXT("render_offscreen"),FParse::Param(FCommandLine::Get(),TEXT("RenderOffScreen")));
        Json->SetNumberField(TEXT("stage"),Stage);
        Json->SetNumberField(TEXT("elapsed_seconds"),FPlatformTime::Seconds()-Started);
        Json->SetStringField(TEXT("error"),Error);
        Json->SetArrayField(TEXT("captures"),Captures);
        FString Content;const auto Writer=TJsonWriterFactory<>::Create(&Content);
        FJsonSerializer::Serialize(Json,Writer);
        if(!FFileHelper::SaveStringToFile(Content,*FPaths::Combine(Directory,TEXT("report.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
            UE_LOG(LogTemp,Error,TEXT("VisualAudit could not write its report to %s"),*Directory);
    }
    void Finish(bool Success,const FString& Failure=FString())
    {
        bDone=true;Error=Failure;Report(Success?TEXT("complete"):TEXT("failed"));
        UE_LOG(LogTemp,Display,TEXT("VisualAudit %s: %s"),Success?TEXT("complete"):TEXT("failed"),*Error);
        FPlatformMisc::RequestExitWithStatus(false,Success?0:1);
    }
    void Next()
    {++Stage;Frames=0;StageStarted=FPlatformTime::Seconds();bRequested=false;PendingFile.Empty();Report(TEXT("running"));}
};
TUniquePtr<FVisualProbe> Probe;

bool ReadPngSize(const FString& File,int32& Width,int32& Height)
{
    TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*File)||Bytes.Num()<45)return false;
    const uint8 Header[]={137,80,78,71,13,10,26,10};
    for(int32 Index=0;Index<8;++Index)if(Bytes[Index]!=Header[Index])return false;
    if(Bytes[12]!='I'||Bytes[13]!='H'||Bytes[14]!='D'||Bytes[15]!='R')return false;
    const int32 End=Bytes.Num()-8;
    if(Bytes[End]!='I'||Bytes[End+1]!='E'||Bytes[End+2]!='N'||Bytes[End+3]!='D')return false;
    auto BigEndian=[&](int32 At){return int32((uint32(Bytes[At])<<24)|(uint32(Bytes[At+1])<<16)|(uint32(Bytes[At+2])<<8)|Bytes[At+3]);};
    Width=BigEndian(16);Height=BigEndian(20);return Width>0&&Height>0;
}

bool SetLatePresentationFixture(FExpeditionRuntime& Runtime,int32 SiteIndex)
{
    // These worlds are deliberately not a playable earned-run continuation. The
    // rig remains the actual unchanged first starter, and no save can be written.
    Runtime.SavePath.Empty();Runtime.bSaveAllowed=false;
    if(!Runtime.World->StartSite(SiteIndex,42))return false;
    Runtime.SiteIndex=SiteIndex;Runtime.Screen=EExpeditionScreen::Site;
    Runtime.SiteEntry.Reset();Runtime.Input.LoseFocus();Runtime.ClearPreparation();
    Runtime.SetPaused(false);
    for(auto& Entry:Runtime.Visuals)
    {
        if(Entry.Value.Mesh)Entry.Value.Mesh->DestroyComponent();
        if(Entry.Value.Detail)Entry.Value.Detail->DestroyComponent();
    }
    Runtime.Visuals.Reset();Runtime.BuildMechanismVisuals();
    return true;
}
}

bool InitializeVisualCaptureProbe(FExpeditionRuntime& Runtime)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("VisualAudit")))return false;
    Probe=MakeUnique<FVisualProbe>();Probe->Runtime=&Runtime;
    Probe->bExtended=FParse::Param(FCommandLine::Get(),TEXT("VisualAuditExtended"));
    Probe->Started=Probe->StageStarted=FPlatformTime::Seconds();
    Runtime.Profile=TEXT("visual_audit_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(24);
    Runtime.bSaveAllowed=false;
    if(!FParse::Value(FCommandLine::Get(),TEXT("VisualAuditDir="),Probe->Directory))
        Probe->Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("VisualAudit"),Runtime.Profile);
    Probe->Directory=FPaths::ConvertRelativePathToFull(Probe->Directory);
    if(IFileManager::Get().FileExists(*FPaths::Combine(Probe->Directory,TEXT("report.json"))))
    {
        UE_LOG(LogTemp,Error,TEXT("VisualAudit refuses existing report directory %s"),*Probe->Directory);
        Probe->bDone=true;FPlatformMisc::RequestExitWithStatus(false,1);return true;
    }
    if(!IFileManager::Get().MakeDirectory(*Probe->Directory,true))
    {Probe->Finish(false,TEXT("Could not create capture directory."));return true;}
    Probe->Report(TEXT("running"));return true;
}

void TickVisualCaptureProbe(FExpeditionRuntime& Runtime,float Delta)
{
    if(!Probe||Probe->bDone||Probe->Runtime!=&Runtime)return;
    (void)Delta;
    // An audit cannot persist a partial presentation fixture or a randomly chosen
    // first rig. Click still exercises the real callback, including Save's guard.
    Runtime.SavePath.Empty();Runtime.bSaveAllowed=false;
    Runtime.Input.LoseFocus();Runtime.Pointer=FVector2D(-100,-100);
    Runtime.Aim=Runtime.Magnet=Probe->Stage>=3?FVector2D(-380,280):FVector2D(0,-240);Runtime.bWorldHit=false;
    const double Now=FPlatformTime::Seconds();
    if(Now-Probe->Started>(Probe->bExtended?240:180)){Probe->Finish(false,TEXT("Timed out before the requested capture set completed."));return;}
    if(!GEngine||!GEngine->GameViewport||!GEngine->GameViewport->Viewport||!Runtime.Display)return;
    ++Probe->Frames;
    if(Probe->bRequested)
    {
        if(!FScreenshotRequest::IsScreenshotRequested()&&IFileManager::Get().FileSize(*Probe->PendingFile)>32)
        {
            int32 Width=0,Height=0;
            if(!ReadPngSize(Probe->PendingFile,Width,Height))
            {Probe->Finish(false,TEXT("Screenshot output was not a complete readable PNG header."));return;}
            if(Width!=1600||Height!=900)
            {Probe->Finish(false,FString::Printf(TEXT("Capture dimensions %d x %d differ from requested 1600 x 900."),Width,Height));return;}
            auto Item=MakeShared<FJsonObject>();Item->SetStringField(TEXT("file"),FPaths::GetCleanFilename(Probe->PendingFile));
            Item->SetNumberField(TEXT("width"),Width);Item->SetNumberField(TEXT("height"),Height);
            Item->SetNumberField(TEXT("bytes"),double(IFileManager::Get().FileSize(*Probe->PendingFile)));
            Item->SetNumberField(TEXT("screen"),int32(Runtime.Screen));Item->SetNumberField(TEXT("site"),Runtime.SiteIndex);
            Item->SetBoolField(TEXT("unearned_late_site_fixture"),Probe->Stage>=3);
            Item->SetStringField(TEXT("kind"),Probe->Stage>=3?TEXT("unearned late-site presentation fixture; StartSite(index,42); unchanged starting rig; no gear/reward injection; not gameplay proof"):
                TEXT("actual fresh-run presentation; no injected gear or rewards"));
            Probe->Captures.Add(MakeShared<FJsonValueObject>(Item));
            if(Probe->Stage==0)
            {
                Runtime.Click(100);
                if(Runtime.Screen!=EExpeditionScreen::Depot||Runtime.Rig->GetOffers().IsEmpty())
                {Probe->Finish(false,TEXT("Actual starter callback did not reach an outfitter with stock."));return;}
                const FName Selected=Runtime.Rig->GetOffers()[0];
                const int32 CatalogIndex=MagnetSweep::FExpeditionRig::Catalog().IndexOfByPredicate([&](const auto& Module){return Module.Id==Selected;});
                if(CatalogIndex==INDEX_NONE){Probe->Finish(false,TEXT("Generated offer was absent from the catalogue."));return;}
                Runtime.Click(1000+CatalogIndex);Probe->SelectedId=Selected.ToString();
            }
            else if(Probe->Stage==1)
            {
                Runtime.Click(10);
                if(Runtime.Screen!=EExpeditionScreen::Site||Runtime.SiteIndex!=0)
                {Probe->Finish(false,TEXT("Actual depart callback did not reach the first worksite."));return;}
                if(Runtime.bPaused)Runtime.Click(20);
            }
            else if(Probe->bExtended&&Probe->Stage<4)
            {
                const int32 FixtureSite=Probe->Stage==2?2:3;
                if(!SetLatePresentationFixture(Runtime,FixtureSite))
                {Probe->Finish(false,TEXT("Could not initialize the explicitly unearned late-site rendering fixture."));return;}
            }
            else {Probe->Finish(true);return;}
            Runtime.bSaveAllowed=false;Probe->Next();
        }
        else if(Now-Probe->Requested>30)Probe->Finish(false,TEXT("Engine did not produce the requested PNG within 30 seconds."));
        return;
    }
    // Several rendered frames after scene/asset setup let temporal rendering and
    // asynchronous uploads settle. This is a capture delay, not gameplay evidence.
    if(Probe->Frames<100||Now-Probe->StageStarted<8)return;
    const auto Size=GEngine->GameViewport->Viewport->GetSizeXY();
    if(Size.X!=1600||Size.Y!=900)
    {Probe->Finish(false,FString::Printf(TEXT("Viewport is %d x %d; use -ResX=1600 -ResY=900 -ForceRes."),Size.X,Size.Y));return;}
    if(Probe->Stage>=2&&Runtime.bPaused){Runtime.Click(20);Probe->Frames=0;Probe->StageStarted=Now;return;}
    const TCHAR* Names[]={TEXT("01-starter.png"),TEXT("02-outfitter-selected.png"),TEXT("03-first-worksite.png"),
        TEXT("04-balanced-rack-fixture.png"),TEXT("05-counterweight-exchange-fixture.png")};
    Probe->PendingFile=FPaths::Combine(Probe->Directory,Names[Probe->Stage]);
    if(IFileManager::Get().FileExists(*Probe->PendingFile))
    {Probe->Finish(false,TEXT("Refused to overwrite an existing capture."));return;}
    if(Probe->Stage>=3)Runtime.Show(TEXT("VISUAL AUDIT FIXTURE / Unplayed late site. Starting rig; no earned equipment or rewards. Rendering review only."));
    else Runtime.NoticeLife=0;
    FScreenshotRequest::RequestScreenshot(Probe->PendingFile,true,false,false,FIntRect(),true);
    Probe->bRequested=true;Probe->Requested=Now;
    UE_LOG(LogTemp,Display,TEXT("VisualAudit requested %s"),*Probe->PendingFile);
}
