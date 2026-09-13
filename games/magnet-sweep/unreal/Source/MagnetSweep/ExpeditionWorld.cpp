#include "ExpeditionWorld.h"
#include "ExpeditionRig.h"

namespace MagnetSweep
{
namespace
{
bool Live(const FExpeditionBody& B) { return B.State == EExpeditionBodyState::Available || B.State == EExpeditionBodyState::Pulling || B.State == EExpeditionBodyState::Cargo; }
bool Held(const FExpeditionBody& B) { return B.State == EExpeditionBodyState::Cargo || B.State == EExpeditionBodyState::Pulling; }
bool Has(const FExpeditionRig& Rig, const TCHAR* Id) { return Rig.Has(FName(Id)); }
bool Finite(const FVector2D& V) { return FMath::IsFinite(V.X) && FMath::IsFinite(V.Y); }
FVector2D Limit(const FVector2D& V, float Max) { const double S = V.Size(); return S > Max ? V * (Max / S) : V; }
FVector2D InTray(const FVector2D& P, float R = 16.f) { return FVector2D(FMath::Clamp(P.X, -500.0 + R, 500.0 - R), FMath::Clamp(P.Y, -300.0 + R, 300.0 - R)); }
bool Near(const FVector2D& A, const FVector2D& B, float R) { return FVector2D::DistSquared(A, B) <= R * R; }
bool IsTerminalBody(const FExpeditionBody& B) {return B.Role==TEXT("ordinary_terminal")||B.Role==TEXT("receiver_terminal")||B.Role==TEXT("source_terminal")||B.Role==TEXT("frame_input")||B.Role==TEXT("frame_contact");}
void Damage(FExpeditionBody& B,int32 Amount)
{ const int32 Old=B.Quality;B.Quality=FMath::Clamp(Old-Amount,0,Old);if(Old>0)B.Appraisal=int32(int64(B.Appraisal)*B.Quality/Old); }
FName ToolFor(EExpeditionAction Action)
{
    switch (Action)
    {
    case EExpeditionAction::Extract: return TEXT("extraction_coil");
    case EExpeditionAction::Launch: case EExpeditionAction::Weld: return TEXT("rail_impeller");
    case EExpeditionAction::Arc: case EExpeditionAction::Ground: case EExpeditionAction::ArmRelay: return TEXT("arc_driver");
    case EExpeditionAction::Winch: case EExpeditionAction::SwitchAnchor: return TEXT("anchor_winch");
    case EExpeditionAction::Vector: return TEXT("vector_emitter");
    case EExpeditionAction::Relay: case EExpeditionAction::LayGuide: return TEXT("relay_projector");
    default: return NAME_None;
    }
}
}

FExpeditionWorld::FExpeditionWorld() { StartSite(0, 1); }

void FExpeditionWorld::BuildObstacles()
{
    SiteDefinition=FExpeditionSiteDefinition();
    SiteDefinition.LayoutId=State.LayoutId;SiteDefinition.LayoutRevision=State.LayoutRevision;
    SiteDefinition.Name=State.SiteIndex==0?TEXT("Preload Workshop"):State.SiteIndex==1?TEXT("Salvage Exchange"):TEXT("Preload Workshop — original layout");
    SiteDefinition.Summary=TEXT("Release the preload, park the ballast and brace the arm; deliver the protected machine separately from scrap.");
    SiteDefinition.Supports={{9,TEXT("counterweight"),NAME_None,8}};
    SiteDefinition.Markers={
        {TEXT("collar"),TEXT("PRELOAD STOP"),CollarStop(),34,true},
        {TEXT("ballast"),TEXT("BALLAST CATCH"),BallastCatch(),62,true},
        {TEXT("arm"),TEXT("ARM STOPPER"),ArmStopper(),53,true},
        {TEXT("circuit"),TEXT("CONDUCTOR SOCKET"),CircuitSocket(),36,false},
        {TEXT("counterweight"),TEXT("COUNTERWEIGHT PAD"),CounterweightPad(),55,false,9},
        {TEXT("furnace"),TEXT("SCRAP FURNACE"),FurnacePosition(),95,false},
        {TEXT("receiver"),TEXT("MACHINE RECEIVER"),ReceiverPosition(),100,true}};
    Obstacles.Reset();
    Obstacles.Add({FVector2D(25, 35), FVector2D(18, 58), TEXT("rebound_wall")});
    Obstacles.Add({FVector2D(-60, -215), FVector2D(76, 12), TEXT("socket_lower")});
    Obstacles.Add({FVector2D(-60, -105), FVector2D(76, 12), TEXT("socket_upper")});
    if(State.LayoutId==TEXT("balanced_rack"))
    {
        SiteDefinition.Name=TEXT("Balanced Rack");
        SiteDefinition.Summary=TEXT("Balance two live weight platforms to release a 16 kg machine. Three difficult salvage targets reward rigging, impact, heat control or a preserved generator.");
        SiteDefinition.bHasArm=false;SiteDefinition.bHasPress=false;
        SiteDefinition.Markers={
            {TEXT("balance_left"),TEXT("LEFT: 10–14 KG"),FVector2D(-250,160),60,true,0},
            {TEXT("balance_right"),TEXT("RIGHT: 10–14 KG"),FVector2D(250,160),60,true,0},
            {TEXT("counterweight"),TEXT("MACHINE SUPPORT"),FVector2D(-370,-130),55,false,9},
            {TEXT("circuit"),TEXT("MACHINE POWER DOCK"),FVector2D(-220,-210),25,false,9},
            {TEXT("brace_power"),TEXT("BRACE POWER DOCK"),FVector2D(190,-230),25,false,10},
            {TEXT("furnace"),TEXT("SCRAP FURNACE"),FurnacePosition(),95,false},
            {TEXT("receiver"),TEXT("MACHINE RECEIVER"),ReceiverPosition(),100,true}};
        Obstacles={{FVector2D(-70,-160),FVector2D(10,55),TEXT("rack_left_partition")},{FVector2D(75,-160),FVector2D(10,55),TEXT("rack_right_partition")}};
    }
    else if(State.LayoutId==TEXT("counterweight_exchange"))
    {
        SiteDefinition.Supports.Reset();
        SiteDefinition.Name=TEXT("Counterweight Exchange");
        SiteDefinition.Summary=TEXT("Lift the 20 kg machine, then replace its counterweight or power the remote latch. Stage your haul, or use remote tools to keep the core secured.");
        SiteDefinition.bHasArm=false;SiteDefinition.PressCenter=FVector2D(450,-50);SiteDefinition.PressHalfSize=FVector2D(22,120);
        SiteDefinition.Markers={
            {TEXT("cover"),TEXT("SET COVER ASIDE"),FVector2D(-80,0),55,true},
            {TEXT("counterbalance"),TEXT("20 KG COUNTERWEIGHT"),FVector2D(300,0),28,true,0},
            {TEXT("staging"),TEXT("SAFE CORE STAGING"),FVector2D(430,-230),38,false},
            {TEXT("circuit"),TEXT("GENERATOR DOCK"),FVector2D(-205,-70),25,false,0},
            {TEXT("furnace"),TEXT("SCRAP FURNACE"),FurnacePosition(),95,false},
            {TEXT("receiver"),TEXT("MACHINE RECEIVER"),ReceiverPosition(),100,true}};
        Obstacles={{FVector2D(80,-110),FVector2D(12,60),TEXT("exchange_partition")}};
    }
    if(State.LayoutRevision==2&&State.LayoutId!=TEXT("e1"))
    {
        const FVector2D F(State.SiteIndex==2?0:200,-150);
        SiteDefinition.Markers.Append({
            {TEXT("frame_receiver"),TEXT("E: RECOVER FRAME"),F+FVector2D(64,48),24,false,61},
            {TEXT("frame_support"),TEXT("FRAME SUPPORT: 8+ KG"),F+FVector2D(-204,202),55,false,61},
            {TEXT("frame_support_receiver"),TEXT("ARRIVING SUPPORT"),F+FVector2D(-140,250),20,false,61},
            {TEXT("frame_reaction"),State.SiteIndex==2?TEXT("REACTION SETUP: 12 KG"):TEXT("REACTION SETUP: 20 KG"),State.SiteIndex==2?FVector2D(73.3333,260):F+FVector2D(-12,346),28,false,61},
            {TEXT("frame_loop"),TEXT("DIRECTED RETURN TILE"),F+FVector2D(-20,-40),16,false,61},
            {TEXT("frame_input"),TEXT("EXPOSED LOOP INPUT"),F+FVector2D(0,-62),18,false,61},
            {TEXT("frame_contact"),TEXT("COVERED HOIST CONTACT"),F,12,false,61},
            {TEXT("frame_service"),TEXT("EXPOSE FLOOR CONTACT"),F+FVector2D(55,0),18,false,61}});
        SiteDefinition.Supports.Add({61,TEXT("frame_support"),TEXT("frame_support_receiver"),8});
        if(State.SiteIndex==2)for(auto& O:Obstacles)if(O.Role==TEXT("rack_right_partition"))O.Center=FVector2D(115,-180);
    }
    SiteDefinition.Obstacles=Obstacles;
}

bool FExpeditionWorld::IsKnownLayout(FName Id,int32 Revision,int32 SiteIndex)
{
    return (Revision==1&&SiteIndex>=0&&SiteIndex<=3&&Id==TEXT("e1"))||((Revision==1||Revision==2)&&((SiteIndex==2&&Id==TEXT("balanced_rack"))||(SiteIndex==3&&Id==TEXT("counterweight_exchange"))));
}

bool FExpeditionWorld::StartSite(int32 SiteIndex, int32 Seed, int32 InitialBattery,FName LayoutId,int32 LayoutRevision)
{
    if(LayoutId.IsNone()) {LayoutId=SiteIndex==2?TEXT("balanced_rack"):SiteIndex==3?TEXT("counterweight_exchange"):TEXT("e1");LayoutRevision=SiteIndex>=2?2:1;}
    if(!IsKnownLayout(LayoutId,LayoutRevision,FMath::Clamp(SiteIndex,0,3)))return false;
    State = FExpeditionWorldState();
    State.SiteIndex = FMath::Clamp(SiteIndex, 0, 3);
    State.LayoutId=LayoutId;State.LayoutRevision=LayoutRevision;
    State.Seed = Seed;
    State.Battery = FMath::Clamp(InitialBattery, 0, 120);
    Events.Reset();
    PendingCyclones.Reset();
    BuildObstacles();
    auto Add = [&](FName Role, EExpeditionMaterial Material, FVector2D P, float Mass, int32 Value, float Radius = 15.f) -> int32
    {
        FExpeditionBody B;
        B.Id = State.NextBodyId++;
        B.Role = Role;
        B.Material = Material;
        B.Position = P;
        B.Mass = Mass;
        B.Value = Value;
        B.Appraisal = Value;
        B.Radius = Radius;
        B.SourceIds.Add(B.Id);
        B.bHot = Material == EExpeditionMaterial::HotCell;
        State.Bodies.Add(B);
        return B.Id;
    };
    Add(TEXT("core"), EExpeditionMaterial::Core, FVector2D(350, 0), 8.f + 4.f * State.SiteIndex, 0, 27);
    State.Bodies[0].bGoal = true; State.Bodies[0].bAnchored = true;
    Add(TEXT("collar"), EExpeditionMaterial::Mechanism, FVector2D(270, -120), 6, 0, 20);
    Add(TEXT("ballast"), EExpeditionMaterial::Iron, FVector2D(280, 100), 12, 24, 25);
    Add(TEXT("brace"), EExpeditionMaterial::Iron, FVector2D(-150, 130), 8, 16, 21);
    Add(TEXT("receiver_terminal"), EExpeditionMaterial::Mechanism, FVector2D(60, -160), 6, 0, 18);
    Add(TEXT("source_terminal"), EExpeditionMaterial::Mechanism, FVector2D(-180, -160), 6, 0, 18);
    State.Bodies[4].bAnchored = true; State.Bodies[5].bAnchored = true; State.Bodies[5].Charge = 2;
    Add(TEXT("conductor"), EExpeditionMaterial::Copper, FVector2D(-280, -100), 6, 36, 21);
    Add(TEXT("conductor_ballast_a"), EExpeditionMaterial::Iron, FVector2D(-326, -100), 2, 4);
    Add(TEXT("conductor_ballast_b"), EExpeditionMaterial::Iron, FVector2D(-234, -100), 2, 4);
    State.Bodies[6].Links = {7, 8}; State.Bodies[7].Links = {6}; State.Bodies[8].Links = {6};
    Add(TEXT("supported_machine"), EExpeditionMaterial::Alloy, FVector2D(-20, 105), 20, 100, 24);
    State.Bodies[9].bAnchored = true;
    Add(TEXT("brittle_brace"), EExpeditionMaterial::Iron, FVector2D(150, 0), 6, 12, 23);
    State.Bodies[10].bAnchored = true; State.Bodies[10].bBrittle = true;
    Add(TEXT("ordinary_terminal"), EExpeditionMaterial::Mechanism, FVector2D(410, -170), 3, 0, 18);
    State.Bodies[11].bAnchored = true;
    FRandomStream Random(Seed);
    // Stable tutorial/combination fixtures above; seeds vary only ordinary salvage pockets.
    for (int32 Row = 0; Row < 4; ++Row)
    {
        for (int32 Col = 0; Col < 6; ++Col)
        {
            const FVector2D P(-440 + Col * 43 + Random.FRandRange(-3, 3), -30 + Row * 44 + Random.FRandRange(-3, 3));
            const bool Copper = Row == 3;
            Add(TEXT("loose_scrap"), Copper ? EExpeditionMaterial::Copper : EExpeditionMaterial::Iron, P, Copper ? 3 : 2, Copper ? 12 : 4);
        }
    }
    for (int32 I = 0; I < 10; ++I)
    {
        Add(TEXT("rich_scrap"), EExpeditionMaterial::Alloy, FVector2D(55 + (I % 5) * 65, -270 + (I / 5) * 48), 4, 24 + State.SiteIndex * 4, 17);
    }
    for (int32 I = 0; I < 3; ++I)
    {
        const int32 A = Add(TEXT("rich_bundle"), EExpeditionMaterial::Alloy, FVector2D(335 + I * 52, 120), 4, 24, 16);
        const int32 B = Add(TEXT("bundle_iron"), EExpeditionMaterial::Iron, FVector2D(335 + I * 52, 163), 2, 4, 14);
        State.Bodies[A].Links.Add(B); State.Bodies[B].Links.Add(A);
        const int32 C = Add(TEXT("bundle_tail"), EExpeditionMaterial::Iron, FVector2D(335 + I * 52, 207), 2, 4, 14);
        State.Bodies[B].Links.Add(C); State.Bodies[C].Links.Add(B);
    }
    const int32 Hot = Add(TEXT("hot_cell"), EExpeditionMaterial::HotCell, FVector2D(-405, -225), 2, 0, 17);
    const int32 LiveAlloy = Add(TEXT("live_alloy"), EExpeditionMaterial::Alloy, FVector2D(-355, -225), 4, 36, 17);
    State.Bodies[Hot].Links.Add(LiveAlloy); State.Bodies[LiveAlloy].Links.Add(Hot);
    const int32 Sink = Add(TEXT("heat_sink_iron"), EExpeditionMaterial::Iron, FVector2D(-305,-225),2,4);
    State.Bodies[LiveAlloy].Links.Add(Sink); State.Bodies[Sink].Links.Add(LiveAlloy);
    const int32 Generator=Add(TEXT("portable_generator"),EExpeditionMaterial::Alloy,FVector2D(-430,220),8,64,22);
    State.Bodies[Generator].bFunctional=true; State.Bodies[Generator].Charge=2;
    const int32 Cage=Add(TEXT("generator_cage"),EExpeditionMaterial::Iron,FVector2D(-382,220),4,8,18);
    State.Bodies[Generator].Links.Add(Cage); State.Bodies[Cage].Links.Add(Generator);
    const int32 Diode=Add(TEXT("return_diode"),EExpeditionMaterial::Mechanism,FVector2D(-55,-263),3,0,12);
    State.Bodies[Diode].bAnchored=true; State.Bodies[Diode].FlowDirection=FVector2D(-125,103).GetSafeNormal();
    State.Bodies[Diode].Links={4,5}; State.Bodies[4].Links.Add(Diode); State.Bodies[5].Links.Add(Diode);
    State.Bodies[4].Links.Add(11); State.Bodies[11].Links.Add(4);
    // A real moving drum gives braking a use before another tool creates a projectile.
    State.Bodies[12].Velocity=FVector2D(180,0);
    if(State.SiteIndex==1)
    {
        // Preserve total source value while making two physical recoveries economically consequential.
        for(auto& B:State.Bodies)
        {
            if(B.Role==TEXT("loose_scrap"))B.Value=B.Material==EExpeditionMaterial::Iron?1:3;
            else if(B.Role==TEXT("rich_scrap"))B.Value=7;
            else if(B.Role==TEXT("rich_bundle"))B.Value=6;
            else if(B.Role==TEXT("bundle_iron")||B.Role==TEXT("bundle_tail"))B.Value=1;
            else if(B.Role==TEXT("conductor"))B.Value=9;
            else if(B.Role==TEXT("conductor_ballast_a")||B.Role==TEXT("conductor_ballast_b"))B.Value=1;
            else if(B.Role==TEXT("portable_generator"))B.Value=16;
            else if(B.Role==TEXT("generator_cage"))B.Value=1;
            else if(B.Role==TEXT("ballast"))B.Value=6;
            else if(B.Role==TEXT("brace"))B.Value=4;
            else if(B.Role==TEXT("supported_machine"))B.Value=320;
            else if(B.Role==TEXT("live_alloy"))B.Value=324;
            B.Appraisal=B.Value;
        }
    }
    ConfigureAuthoredLayout();
    ConfigurePowerFrame();
    return true;
}

void FExpeditionWorld::ConfigurePowerFrame()
{
    if(State.LayoutRevision!=2||State.LayoutId==TEXT("e1"))return;
    const FVector2D F(State.SiteIndex==2?0:200,-150);
    auto Add=[&](FName Role,EExpeditionMaterial Material,FVector2D Position,float Mass,int32 Value,float Radius)
    {
        FExpeditionBody B;B.Id=State.NextBodyId++;B.Role=Role;B.Material=Material;B.Position=Position;B.Mass=Mass;B.Value=Value;B.Appraisal=Value;B.Radius=Radius;B.SourceIds={B.Id};State.Bodies.Add(B);
    };
    Add(TEXT("power_frame"),EExpeditionMaterial::Alloy,F,40,320,35);
    auto* Frame=MutableBody(61);Frame->bAnchored=true;Frame->bFunctional=true;Frame->Charge=2;
    Add(TEXT("frame_input"),EExpeditionMaterial::Mechanism,F+FVector2D(0,-62),2,0,14);MutableBody(62)->bAnchored=true;
    Add(TEXT("frame_contact"),EExpeditionMaterial::Mechanism,F,2,0,10);MutableBody(63)->bAnchored=true;MutableBody(63)->bFloorContact=true;
    Add(TEXT("frame_return"),EExpeditionMaterial::Copper,F+FVector2D(-85,70),2,0,8);MutableBody(64)->FlowDirection=FVector2D(20,40).GetSafeNormal();
    if(State.SiteIndex==2)
    {
        MutableBody(33)->Position=FVector2D(350,250);MutableBody(34)->Position=FVector2D(400,250); // Clear the real 12 kg reaction setup.
        MutableBody(38)->Position=FVector2D(310,-270); // Leave the frame receiving ring free of ordinary alloy.
    }
    if(State.SiteIndex==3)MutableBody(38)->Position=FVector2D(100,-270); // The exposed input is not hidden inside ordinary alloy.
}

void FExpeditionWorld::ConfigureAuthoredLayout()
{
    if(State.LayoutId==TEXT("e1"))return;
    auto Place=[&](int32 Id,FVector2D P){if(auto* B=MutableBody(Id)){B->Position=P;B->Velocity=FVector2D::ZeroVector;}};
    auto Price=[&](int32 Id,int32 Value){if(auto* B=MutableBody(Id)){B->Value=Value;B->Appraisal=Value;}};
    if(State.LayoutId==TEXT("balanced_rack"))
    {
        // These three old fittings are absent, not rendered as paid controls without an effect.
        State.Bodies.RemoveAll([](const FExpeditionBody& B){return B.Id==1||B.Id==5||B.Id==60;});
        Place(0,{0,70});Place(2,{-320,30});Place(3,{320,30});
        Place(4,{-180,-210});Place(11,{150,-230});MutableBody(11)->Role=TEXT("receiver_terminal");
        Place(9,{-180,-130});Place(10,{150,-140});
        Place(6,{-390,-60});Place(7,{-436,-60});Place(8,{-344,-60});
        Place(12,{280,-20});Place(13,{324,-20});
        for(int32 Id=14;Id<=29;++Id)Place(Id,{-450.f+45.f*((Id-14)%8),-260.f+40.f*((Id-14)/8)});
        Place(28,{-80,-250}); // Keep ordinary scrap clear of the isolated left receiver.
        for(int32 Id=30;Id<=35;++Id)Place(Id,{-80.f+45.f*(Id-30),270});
        for(int32 Id=36;Id<=45;++Id)Place(Id,{-45.f+55.f*((Id-36)%5),-55.f+50.f*((Id-36)/5)});
        for(int32 Group=0;Group<3;++Group)for(int32 Part=0;Part<3;++Part)Place(46+Group*3+Part,{383.f+46.f*Part,-15.f+60.f*Group});
        Place(55,{430,-230});Place(56,{330,-170});Place(57,{380,-170});
        MutableBody(56)->bHot=true;for(int32 Id:{55,56,57})MutableBody(Id)->Links.Reset();
        Place(58,{-420,180});Place(59,{-374,180});
        // 208 ordinary output + three distinct 220-output recoveries = the original 868 total.
        for(int32 Id=12;Id<=35;++Id)Price(Id,Id<30?1:3);
        for(int32 Id=36;Id<=45;++Id)Price(Id,8);
        for(int32 Id=46;Id<=54;++Id)Price(Id,(Id-46)%3==0?8:1);
        Price(6,10);Price(7,1);Price(8,1);Price(58,12);Price(59,2);Price(2,12);Price(3,8);Price(57,16);
        for(int32 Id:{9,10,56})Price(Id,220);
    }
    else
    {
        State.Bodies.RemoveAll([](const FExpeditionBody& B){return B.Id==5||(B.Id>=6&&B.Id<=11)||(B.Id>=20&&B.Id<=35)||(B.Id>=40&&B.Id<=54)||B.Id==60;});
        Place(0,{300,0});MutableBody(0)->Radius=30;
        auto* Cover=MutableBody(1);Cover->Role=TEXT("counterweight_cover");Cover->Material=EExpeditionMaterial::Iron;Cover->Mass=20;Cover->Radius=28;Price(1,40);Place(1,{220,0});
        Place(2,{-330,180});Place(3,{-220,180});
        Place(4,{-250,-70});
        for(int32 Id=12;Id<=19;++Id)Place(Id,{-420.f+45.f*((Id-12)%4),40.f+50.f*((Id-12)/4)});
        for(int32 Id=36;Id<=39;++Id)Place(Id,{100.f+50.f*(Id-36),-220});
        Place(55,{-420,-200});Place(56,{-370,-200});Place(57,{-320,-200});
        Place(58,{-420,235});Place(59,{-374,235});
    }
    for(auto& B:State.Bodies)
    {
        B.Links.RemoveAll([&](int32 Id){return !FindBody(Id);});
        if(B.Role==TEXT("receiver_terminal"))B.Links.Reset();
    }
}

const FExpeditionBody* FExpeditionWorld::FindBody(int32 Id) const { return State.Bodies.FindByPredicate([Id](const FExpeditionBody& B) { return B.Id == Id; }); }
FExpeditionBody* FExpeditionWorld::MutableBody(int32 Id) { return State.Bodies.FindByPredicate([Id](const FExpeditionBody& B) { return B.Id == Id; }); }

int32 FExpeditionWorld::FindBodyAt(const FVector2D& Position, float ExtraRadius, bool bIncludeCargo) const
{
    int32 Id = INDEX_NONE; double Best = TNumericLimits<double>::Max();
    for (const FExpeditionBody& B : State.Bodies)
    {
        if (!Live(B) || (!bIncludeCargo && Held(B))) continue;
        const double D = FVector2D::DistSquared(Position, B.Position);
        if (D <= FMath::Square(B.Radius + ExtraRadius) && (D < Best || (D == Best && B.Id < Id))) { Best = D; Id = B.Id; }
    }
    return Id;
}

TArray<int32> FExpeditionWorld::GetGroup(int32 Id) const
{
    TArray<int32> Result;
    const FExpeditionBody* First = FindBody(Id);
    if (!First || !Live(*First)) return Result;
    Result.Add(Id);
    for (int32 I = 0; I < Result.Num(); ++I)
        if (const FExpeditionBody* B = FindBody(Result[I]))
            for (int32 Link : B->Links) if (const FExpeditionBody* L = FindBody(Link); L && Live(*L)) Result.AddUnique(Link);
    Result.Sort();
    return Result;
}

float FExpeditionWorld::GetCargoMass() const { float Mass = 0; for (const auto& B : State.Bodies) if (Held(B) && B.Material != EExpeditionMaterial::Mechanism) Mass += B.Mass; return Mass; }
int32 FExpeditionWorld::GetCargoValue() const { int32 Value = 0; for (const auto& B : State.Bodies) if (Held(B) && !B.bGoal) Value += B.Appraisal; return Value; }
bool FExpeditionWorld::IsUnsafe() const { if (GetCargoMass() > SafeCapacity + .01f) return true; for (const auto& B : State.Bodies) if (Held(B) && B.bHot) return true; return false; }
bool FExpeditionWorld::IsArmSafe() const { return !SiteDefinition.bHasArm || State.bArmBraced || FMath::Fmod(State.WorldTime, 6.f) < 3.7f; }
bool FExpeditionWorld::IsPressSafe() const { return !SiteDefinition.bHasPress || FMath::Fmod(State.WorldTime,5.5f)<3.4f; }
FVector2D FExpeditionWorld::GetArmTip() const
{
    const float Phase=State.bArmBraced?0.f:FMath::Fmod(State.WorldTime,6.f)/6.f;
    return SiteDefinition.ArmPivot+FVector2D(-100.f*FMath::Cos(Phase*PI),-95.f*FMath::Sin(Phase*PI));
}

const FExpeditionSiteMarker* FExpeditionWorld::FindMarker(FName Id) const
{return SiteDefinition.Markers.FindByPredicate([Id](const FExpeditionSiteMarker& M){return M.Id==Id;});}

const FExpeditionSupportDefinition* FExpeditionWorld::FindSupport(int32 Id) const
{return SiteDefinition.Supports.FindByPredicate([Id](const FExpeditionSupportDefinition& S){return S.PayloadId==Id;});}

const FExpeditionBody* FExpeditionWorld::FindFrameBody() const
{return State.Bodies.FindByPredicate([](const FExpeditionBody& B){return B.Role==TEXT("power_frame");});}

bool FExpeditionWorld::IsManualTargetExposed(int32 Id) const
{
    const auto* Target=FindBody(Id);const auto* Frame=FindFrameBody();
    return !Target||Target->Role!=TEXT("frame_contact")||!Frame||!Live(*Frame)||!Near(Target->Position,Frame->Position,Frame->Radius+Target->Radius);
}

bool FExpeditionWorld::HasReceivingSupport(int32 Id) const
{
    const auto* Definition=FindSupport(Id);
    const auto* Pad=Definition?FindMarker(Definition->ReceivingMarker):nullptr;
    if(!Definition||!Pad)return false;
    for(const auto& K:State.Constraints)
    {
        if(!K.bActive||K.BodyA!=Id)continue;
        const auto* Support=FindBody(K.BodyB);
        if(!Support||Support->State!=EExpeditionBodyState::Available||Support->bAnchored||Support->Mass<Definition->MinimumMass||Support->Velocity.Size()>=45||!Near(Support->Position,Pad->Position,Pad->Radius))continue;
        // A fixed support at its old foundation cannot serve the arrival. A real pawl
        // can hold the payload while that support is moved to the new receiving point.
        if(K.Kind!=EExpeditionConstraintKind::Counterweight||Near(Support->Position,K.Anchor,55)||State.LatchedBody==Id)return true;
    }
    return false;
}

bool FExpeditionWorld::CanReceiveFrame(FString& Reason) const
{
    const auto* Frame=FindFrameBody();const auto* Dock=FindMarker(TEXT("frame_receiver"));
    if(!Frame||!Dock){Reason=TEXT("This worksite has no heavy-frame receiving dock");return false;}
    if(IsEnded()||State.bFrameReceived||Frame->State==EExpeditionBodyState::Banked){Reason=TEXT("This frame handoff is already settled");return false;}
    if(Frame->State!=EExpeditionBodyState::Available||Frame->bAnchored||Frame->Velocity.Size()>=45||!Near(Frame->Position,Dock->Position,Dock->Radius)){Reason=TEXT("Bring the actual released 40 kg frame into its receiving ring and let it settle");return false;}
    if(!State.bFrameHoistPowered&&!HasReceivingSupport(Frame->Id)){Reason=TEXT("The receiving support must arrive with the frame, or the wired hoist contact must be powered");return false;}
    if(!Near(State.Magnet,Dock->Position,95)){Reason=TEXT("Move the magnet to E: RECOVER FRAME for the handoff");return false;}
    Reason.Reset();return true;
}

FExpeditionResult FExpeditionWorld::ReceiveFrame()
{
    FExpeditionResult R;if(!CanReceiveFrame(R.Message))return R;
    auto* Frame=MutableBody(FindFrameBody()->Id);
    R.bSucceeded=true;R.Output=Frame->Appraisal;R.BodyIds={Frame->Id};
    State.Output+=R.Output;State.bFrameReceived=true;State.bFrameHoistActive=false;
    Frame->State=EExpeditionBodyState::Banked;Frame->Velocity=FVector2D::ZeroVector;
    for(auto& K:State.Constraints)if(K.BodyA==Frame->Id||K.BodyB==Frame->Id)K.bActive=false;
    if(State.TetherBody==Frame->Id)State.TetherBody=INDEX_NONE;
    if(State.SecondTetherBody==Frame->Id)State.SecondTetherBody=INDEX_NONE;
    if(State.LatchedBody==Frame->Id)State.LatchedBody=INDEX_NONE;
    if(State.LayoutId==TEXT("counterweight_exchange"))State.bCircuitClosed=true;
    R.Message=FString::Printf(TEXT("Frame installed — +%d actual output%s"),R.Output,State.LayoutId==TEXT("counterweight_exchange")?TEXT("; final counterbalance supplied"):TEXT(""));
    AddEvent(EExpeditionEventKind::Banked,R.Message,Frame->Position,R.BodyIds,R.Output);
    return R;
}

bool FExpeditionWorld::IsCoreReleased() const
{
    if(State.LayoutId==TEXT("balanced_rack"))return State.bCoreSecured||State.bCollarReleased;
    if(State.LayoutId==TEXT("counterweight_exchange"))return State.bCoreSecured||State.bCollarReleased;
    return State.bCollarReleased&&State.bBallastCleared&&State.bArmBraced;
}

float FExpeditionWorld::PadMass(FVector2D Center,float Radius) const
{
    float Mass=0;
    for(const auto& B:State.Bodies)if(B.State==EExpeditionBodyState::Available&&!B.bGoal&&!B.bAnchored&&B.Material!=EExpeditionMaterial::Mechanism&&B.Velocity.Size()<45&&Near(B.Position,Center,Radius))Mass+=B.Mass;
    return Mass;
}

bool FExpeditionWorld::IsTerminalPowered(int32 Id) const
{
    if(State.PoweredTerminals.Contains(Id))return true;
    if(State.LayoutId!=TEXT("e1"))return false;
    return Id==11?State.bArmBraced:(Id==4||Id==5)&&State.bCircuitClosed;
}

bool FExpeditionWorld::HasTerminalWork(int32 Id) const
{
    if(const auto* Frame=FindFrameBody())
    {
        if(Id==62)return Live(*Frame)&&(Frame->bAnchored||State.Constraints.ContainsByPredicate([&](const FExpeditionConstraint& K){return K.bActive&&K.BodyA==Frame->Id;}));
        if(Id==63)return Live(*Frame)&&!State.bFrameHoistPowered;
    }
    if(State.LayoutId!=TEXT("balanced_rack"))return !IsTerminalPowered(Id);
    const auto* Payload=FindBody(Id==4?9:10);
    return Payload&&Live(*Payload)&&!Held(*Payload)&&(Payload->bAnchored||State.Constraints.ContainsByPredicate([&](const FExpeditionConstraint& K){return K.bActive&&K.BodyA==Payload->Id;}));
}

TArray<FExpeditionSiteObjective> FExpeditionWorld::GetObjectives() const
{
    auto WithFrame=[&](TArray<FExpeditionSiteObjective> Items)
    {
        if(const auto* Frame=FindFrameBody())
        {
            FString Reason;const bool Ready=CanReceiveFrame(Reason);
            const FString Hint=State.bFrameReceived?(State.LayoutId==TEXT("counterweight_exchange")?TEXT("Frame installed and its output banked; the final counterbalance is supplied."):TEXT("Frame installed; its actual output is already banked.")):(Ready?TEXT("Press E at E: RECOVER FRAME. This banks the actual frame output once."):Reason);
            Items.Add({TEXT("frame"),State.bFrameReceived?TEXT("Optional frame installed"):FString::Printf(TEXT("Optional frame: %d output"),Frame->Appraisal),Hint,State.bFrameReceived,false});
        }
        return Items;
    };
    const auto* Core=FindBody(0);
    const bool Carried=Core&&Core->State==EExpeditionBodyState::Cargo;
    const FString Reward=State.SiteIndex==3?TEXT("Final delivery archives this rig."):TEXT("Delivery earns recovery credits.");
    const FString DeliveryHint=Carried?FString::Printf(TEXT("Carry the core around hazards to MACHINE RECEIVER. Wait for it to settle, then press E. %s"),*Reward):FString::Printf(TEXT("Make room for %.0f kg (%.0f / 24 carried). Shift + LMB the core; carry it to MACHINE RECEIVER, wait to settle, then E. %s"),Core?Core->Mass:0,GetCargoMass(),*Reward);
    if(State.LayoutId==TEXT("balanced_rack"))
    {
        const float Left=PadMass({-250,160},60),Right=PadMass({250,160},60);
        const bool LeftReady=Left>=10&&Left<=14,RightReady=Right>=10&&Right<=14;
        const bool Balanced=LeftReady&&RightReady&&FMath::Abs(Left-Right)<=2;
        return WithFrame({
            {TEXT("balance_left"),FString::Printf(TEXT("Left %.0f kg / 10–14"),Left),TEXT("Shift + LMB the 12 kg ballast; carry it to LEFT: 10–14 KG and right-click. Only resting loose mass counts. Keep both platforms within 2 kg."),State.bCoreSecured||LeftReady,true},
            {TEXT("balance_right"),FString::Printf(TEXT("Right %.0f kg; difference %.0f"),Right,FMath::Abs(Left-Right)),TEXT("Shift + LMB the 8 kg brace and two 2 kg irons, then right-click on RIGHT: 10–14 KG. Each platform needs 10–14 kg; their difference must be at most 2 kg."),State.bCoreSecured||Balanced,true},
            {TEXT("core"),TEXT("Secure the 16 kg machine"),TEXT("While both platforms stay balanced, make room for 16 kg and Shift + LMB the core. Once actually secured, the rack stays unlocked; you may reclaim the weights."),State.bCoreSecured,true},
            {TEXT("receiver"),TEXT("Deliver the machine"),DeliveryHint,State.bDispatched,true}});
    }
    if(State.LayoutId==TEXT("counterweight_exchange"))
    {
        const bool Replaced=State.bBallastCleared||State.bCircuitClosed;
        const float Mass=PadMass({300,0},28);
        return WithFrame({
            {TEXT("cover"),TEXT("Clear the heavy cover"),TEXT("Shift + LMB the 20 kg cover. Move it clear of the core, then right-click near SET COVER ASIDE. The marked parking spot is a suggestion; leave room to recover the core."),State.bCoreSecured||State.bCollarReleased,true},
            {TEXT("core"),TEXT("Secure the 20 kg core"),TEXT("With the cover clear and room for 20 kg, Shift + LMB the core. Two 20 kg loads exceed 36 kg: plan to stage the core or move the replacement remotely."),State.bCoreSecured,true},
            {TEXT("counterbalance"),FString::Printf(TEXT("Replace weight %.0f / 20 kg"),Mass),TEXT("RMB the core at SAFE CORE STAGING; move 20 kg to its empty cradle, then regrip. Or keep it held and move weight with Winch/Relay. Place a preserved generator at GENERATOR DOCK, then Arc the receiver."),Replaced,true},
            {TEXT("receiver"),TEXT("Deliver the machine"),DeliveryHint,State.bDispatched,true}});
    }
    return WithFrame({
        {TEXT("collar"),TEXT("Release preload"),TEXT("Hold LMB over the brass collar, then move left to PRELOAD STOP. Shift narrows pickup. The collar slides along its guide; it is not furnace scrap."),State.bCollarReleased,true},
        {TEXT("ballast"),TEXT("Park the ballast"),TEXT("Use Shift + LMB to collect the 12 kg ballast. Carry it to BALLAST CATCH, then right-click to place it. Placement releases the retaining latch."),State.bBallastCleared,true},
        {TEXT("arm"),TEXT("Hold the arm clear"),TEXT("Collect the 8 kg iron brace. Carry it to ARM STOPPER and right-click during the arm's clear phase. It locks the arm away from the core."),State.bArmBraced,true},
        {TEXT("receiver"),TEXT("Deliver the machine"),DeliveryHint,State.bDispatched,true}});
}

FString FExpeditionWorld::GetGoalText() const
{
    if (State.bDispatched) return TEXT("Machine secured — depot reward ready");
    if (State.bEvacuated) return TEXT("Evacuated — retry the site-entry rig and battery");
    if(State.LayoutId!=TEXT("e1"))for(const auto& Objective:GetObjectives())if(Objective.bRequired&&!Objective.bSatisfied)return Objective.Label;
    if (!State.bCollarReleased) return TEXT("Release preload: pull the collar left into its stop");
    if (!State.bBallastCleared) return TEXT("Move the ballast into the side catch, or release its powered latch");
    if (!State.bArmBraced) return TEXT("Place iron in the arm stopper during its clear phase");
    return TEXT("Core released — carry it to the separate MACHINE RECEIVER");
}

void FExpeditionWorld::AddEvent(EExpeditionEventKind Kind, const FString& Message, const FVector2D& Position, const TArray<int32>& Ids, int32 Amount)
{
    if (Events.Num() >= 128) Events.RemoveAt(0, 1, EAllowShrinking::No);
    FExpeditionEvent E; E.Kind = Kind; E.ActionId = State.ActionSerial; E.Position = Position; E.BodyIds = Ids; E.Amount = Amount; E.Message = Message; Events.Add(MoveTemp(E));
}
TArray<FExpeditionEvent> FExpeditionWorld::DrainEvents() { TArray<FExpeditionEvent> Result = MoveTemp(Events); Events.Reset(); return Result; }

TArray<int32> FExpeditionWorld::ConductiveNeighbors(int32 Id,const FExpeditionRig& Rig) const
{
    TArray<int32> Result; const auto* A=FindBody(Id);
    if(!A||!Live(*A)||!A->bConductive||Held(*A)||Id==State.GroundBody)return Result;
    auto SocketReady=[](const FExpeditionBody& B){return (B.Role!=TEXT("conductor")&&B.Role!=TEXT("portable_generator"))||B.Links.IsEmpty();};
    for(const auto& B:State.Bodies)
    {
        if(B.Id==Id||!Live(B)||!B.bConductive||Held(B))continue;
        const FVector2D D=B.Position-A->Position;
        if(!A->FlowDirection.IsNearlyZero()&&FVector2D::DotProduct(D,A->FlowDirection)<=0)continue;
        if(!B.FlowDirection.IsNearlyZero()&&FVector2D::DotProduct(D,B.FlowDirection)<=0)continue;
        bool Connected=A->Links.Contains(B.Id);
        const float ReachA=A->Role==TEXT("conductor")?108.f:A->Radius+8.f;
        const float ReachB=B.Role==TEXT("conductor")?108.f:B.Radius+8.f;
        if(SocketReady(*A)&&SocketReady(B)&&D.Size()<=ReachA+ReachB)Connected=true;
        if(Has(Rig,TEXT("conductive_tether")))
        {
            if(State.TetherBody==Id&&Near(B.Position,State.TetherAnchor,B.Radius+35))Connected=true;
            if(State.TetherBody==B.Id&&Near(A->Position,State.TetherAnchor,A->Radius+35))Connected=true;
            if(State.SecondTetherBody==Id&&Near(B.Position,State.SecondTetherAnchor,B.Radius+35))Connected=true;
            if(State.SecondTetherBody==B.Id&&Near(A->Position,State.SecondTetherAnchor,A->Radius+35))Connected=true;
        }
        if(Connected)Result.Add(B.Id);
    }
    Result.Sort();return Result;
}

bool FExpeditionWorld::ArcPath(int32 Target,const FExpeditionRig& Rig,TArray<int32>& OutPath,bool* bUsedBridge) const
{
    OutPath.Reset(); if(bUsedBridge)*bUsedBridge=false;
    // Aiming the input terminal actuates its remote receiver, never an empty pulse.
    if(Target==5)Target=4;
    const auto* T=FindBody(Target);
    if(!T||!Live(*T)||Target==State.GroundBody)return false;
    if(T->Role==TEXT("ordinary_terminal")){OutPath.Add(Target);return true;}
    if(!IsTerminalBody(*T))return false;
    // Prefer a wired route, then search (body, gap-used) states. Exploring an unhelpful
    // candidate must not consume the one permitted gap for every other branch.
    for(int32 Pass=0;Pass<=(Has(Rig,TEXT("induction_bridge"))?1:0);++Pass)
    {
        TArray<int32> Queue={10}; TMap<int32,int32> Parent; Parent.Add(10,INDEX_NONE);
        for(const auto& B:State.Bodies)if(B.bFunctional&&B.Charge>0&&Live(B)&&!Held(B)&&B.Links.IsEmpty()) {Queue.AddUnique(B.Id*2);Parent.Add(B.Id*2,INDEX_NONE);}
        for(int32 I=0;I<Queue.Num();++I)
        {
            const int32 Key=Queue[I],Id=Key/2; const bool Used=(Key&1)!=0;
            if(Id==Target)
            {
                for(int32 P=Key;P!=INDEX_NONE;P=Parent[P])OutPath.Insert(P/2,0);
                if(bUsedBridge)*bUsedBridge=Used;
                return true;
            }
            auto Enqueue=[&](int32 NextId,bool Gap){const int32 NextKey=NextId*2+(Gap?1:0);if(!Parent.Contains(NextKey)){Parent.Add(NextKey,Key);Queue.Add(NextKey);}};
            const auto Next=ConductiveNeighbors(Id,Rig);
            for(int32 N:Next)Enqueue(N,Used);
            const auto* A=FindBody(Id);
            if(Pass==0||Used||!A||!Live(*A)||Held(*A)||!A->bConductive||Id==State.GroundBody)continue;
            for(const auto& B:State.Bodies)
            {
                if(B.Id==Id||!Live(B)||Held(B)||!B.bConductive||!B.Links.IsEmpty()||Next.Contains(B.Id)||!Near(B.Position,A->Position,80+B.Radius+A->Radius))continue;
                const FVector2D Direction=B.Position-A->Position;
                if(!A->FlowDirection.IsNearlyZero()&&FVector2D::DotProduct(Direction,A->FlowDirection)<=0)continue;
                if(!B.FlowDirection.IsNearlyZero()&&FVector2D::DotProduct(Direction,B.FlowDirection)<=0)continue;
                Enqueue(B.Id,true);
            }
        }
    }
    return false;
}

bool FExpeditionWorld::CircuitLoop(const FExpeditionRig& Rig,TArray<int32>& Reachable,int32 SourceId) const
{
    const auto* Source=FindBody(SourceId);
    if(!Source||!Live(*Source)||!Source->bConductive){Reachable.Reset();return false;}
    Reachable={SourceId};
    for(int32 I=0;I<Reachable.Num();++I)for(int32 N:ConductiveNeighbors(Reachable[I],Rig))Reachable.AddUnique(N);
    for(int32 Node:Reachable)
    {
        const auto* B=FindBody(Node); if(!B||B->FlowDirection.IsNearlyZero())continue;
        // A reachable directed return physically closes the source path; a two-node backtrack is not a circuit.
        if(ConductiveNeighbors(Node,Rig).Contains(SourceId)&&Reachable.ContainsByPredicate([&](int32 Id){const auto* T=FindBody(Id);return Id!=SourceId&&T&&IsTerminalBody(*T);}))return true;
    }
    return false;
}

FExpeditionPreview FExpeditionWorld::Preview(const FExpeditionCommand& C, const FExpeditionRig& Rig) const
{
    FExpeditionPreview P; P.Action = C.Action;
    auto Refuse = [&](const TCHAR* Why) { P.Reason = Why; return P; };
    if (IsEnded()) return Refuse(TEXT("This site has ended"));
    if (!Finite(C.Magnet) || !Finite(C.Aim) || !Finite(C.Destination) || !Finite(C.SecondaryPoint)) return Refuse(TEXT("Invalid aiming coordinates"));
    const FName Tool = ToolFor(C.Action);
    if (!Tool.IsNone() && !Rig.Has(Tool)) return Refuse(TEXT("Fit the corresponding active tool first"));
    if (C.Action != EExpeditionAction::Attract && IsUnsafe()) return Refuse(TEXT("Unsafe haul — right-click to drop the whole haul"));
    if (!State.PullIds.IsEmpty()) return Refuse(TEXT("Let the paid pull settle, or release it first"));
    if ((State.RelayBody != INDEX_NONE || State.SecondRelayBody != INDEX_NONE) && (C.Action==EExpeditionAction::Attract || C.Action==EExpeditionAction::Extract || C.Action==EExpeditionAction::Relay)) return Refuse(TEXT("The committed field transfer is still moving"));
    const int32 TargetId = C.TargetId == INDEX_NONE ? FindBodyAt(C.Aim, 30, C.Action == EExpeditionAction::Launch) : C.TargetId;
    const FExpeditionBody* Target = FindBody(TargetId);
    P.TargetId = TargetId;
    switch (C.Action)
    {
    case EExpeditionAction::Attract:
    {
        P.BatteryCost = 6;
        TArray<int32> Candidates;
        const float Radius = C.bPrecision ? 42.f : 110.f;
        for (const auto& B : State.Bodies) if (B.State == EExpeditionBodyState::Available && !B.bAnchored && B.RecoverDelay <= 0 && Near(B.Position, C.Aim, Radius)) Candidates.Add(B.Id);
        Candidates.Sort([&](int32 A, int32 B) { const double DA = FVector2D::DistSquared(FindBody(A)->Position, C.Aim), DB = FVector2D::DistSquared(FindBody(B)->Position, C.Aim); return DA == DB ? A < B : DA < DB; });
        float SelectedMass = GetCargoMass();
        for (int32 Id : Candidates)
        {
            if (P.BodyIds.Contains(Id)) continue;
            const TArray<int32> Group = GetGroup(Id);
            bool Valid = !Group.IsEmpty(); float Mass = 0;
            for (int32 G : Group) { const auto* B = FindBody(G); Valid &= B && !B->bAnchored && B->State == EExpeditionBodyState::Available && B->RecoverDelay <= 0; if (B && B->Material != EExpeditionMaterial::Mechanism) Mass += B->Mass; }
            if (!Valid || SelectedMass + Mass > HardCapacity + .01f) continue;
            P.BodyIds.Append(Group); SelectedMass += Mass;
        }
        if (P.BodyIds.IsEmpty()) return Refuse(TEXT("Aim at available metal; a whole linked group must fit 36 kg"));
        break;
    }
    case EExpeditionAction::Extract:
        if (!Target || Target->State != EExpeditionBodyState::Available || Target->bAnchored || Target->Links.IsEmpty()) return Refuse(TEXT("Aim at an available linked piece; anchored machinery needs its supports released"));
        if (GetCargoMass() + Target->Mass > SafeCapacity) return Refuse(TEXT("The selected piece must fit the safe 24 kg capacity"));
        P.BodyIds = {TargetId}; P.BatteryCost = 6;
        if (Has(Rig,TEXT("cold_seam"))) P.BatteryCost += 2;
        if (Has(Rig,TEXT("crack_follower"))) P.BatteryCost += 3;
        if(Has(Rig,TEXT("intact_recovery")))
        {
            bool Preserves=Target->bFunctional;
            for(int32 Id:Target->Links)if(const auto* B=FindBody(Id);B&&B->bFunctional&&B->Links.Num()==1)Preserves=true;
            if(Preserves)P.BatteryCost+=6;
        }
        if(!Has(Rig,TEXT("insulated_jaw")))for(int32 L:Target->Links)if(const auto* B=FindBody(L);B&&B->bHot)P.bUnsafe=true;
        break;
    case EExpeditionAction::Weld:
        if (!Has(Rig,TEXT("slug_press"))) return Refuse(TEXT("Slug Press is required to weld a counterweight"));
        if(C.BodyIds.IsEmpty())return Refuse(TEXT("Select at least two carried iron pieces before paying to weld"));
        for (const auto& B : State.Bodies) if (B.State == EExpeditionBodyState::Cargo && B.Material == EExpeditionMaterial::Iron && !B.bHot && C.BodyIds.Contains(B.Id)) P.BodyIds.Add(B.Id);
        if (P.BodyIds.Num() < 2) return Refuse(TEXT("Carry at least two ordinary iron pieces to weld"));
        P.BatteryCost = 4; break;
    case EExpeditionAction::Launch:
        if (!C.BodyIds.IsEmpty()) Target = FindBody(C.BodyIds[0]);
        if (!Target || (Target->State != EExpeditionBodyState::Cargo&&!State.CycloneIds.Contains(Target->Id)) || Target->bGoal || Target->Material == EExpeditionMaterial::Mechanism) return Refuse(TEXT("Choose carried scrap or an actual orbit fragment as ammunition"));
        if (Near(C.Aim, C.Magnet, 20)) return Refuse(TEXT("Aim away from the magnet to launch"));
        P.TargetId = Target->Id; P.BodyIds = {Target->Id}; P.BatteryCost = 8;
        if (Has(Rig,TEXT("punch_through_collar"))) P.BatteryCost += 3;
        if(Has(Rig,TEXT("salvage_cyclone")))P.BatteryCost+=6;
        break;
    case EExpeditionAction::Arc:
    {
        if(!IsManualTargetExposed(TargetId))return Refuse(TEXT("Contact covered by the frame. Move the frame, or energize the exposed loop input."));
        TArray<int32> Path; bool UsedBridge=false;
        if (!ArcPath(TargetId, Rig, Path, &UsedBridge)) return Refuse(State.LayoutId==TEXT("e1")?TEXT("No conducting path: free the long conductor, place it in the socket, then pulse the terminal"):TEXT("Connect a free, working generator with stored charge to this receiver; check where the source actually landed"));
        P.BodyIds = Path; P.BatteryCost = 8;
        if (UsedBridge) P.BatteryCost += 4;
        const int32 Terminal=TargetId==5?4:TargetId;
        if(IsTerminalPowered(Terminal)||!HasTerminalWork(Terminal))
        {
            TArray<int32> Reachable;
            const bool UsefulReturn=Has(Rig,TEXT("closed_circuit"))&&!Path.IsEmpty()&&CircuitLoop(Rig,Reachable,Path[0])&&Reachable.ContainsByPredicate([&](int32 Id){const auto* B=FindBody(Id);return Id!=Terminal&&Id!=State.GroundBody&&B&&IsTerminalBody(*B)&&!IsTerminalPowered(Id)&&HasTerminalWork(Id);});
            if(!UsefulReturn)return Refuse(TEXT("This terminal has no new mechanism work; complete a live return circuit to power an unpowered branch"));
        }
        break;
    }
    case EExpeditionAction::Winch:
    {
        if (!Target || !Live(*Target) || Target->bGoal && !IsCoreReleased()) return Refuse(TEXT("Choose a movable body or the supported machine"));
        if (Target->Material == EExpeditionMaterial::Mechanism && Target->Role != TEXT("collar")) return Refuse(TEXT("That fixed terminal is not a tow point"));
        if (Target->bAnchored)
        {
            const auto* Support=FindSupport(TargetId);
            if(!Support)return Refuse(TEXT("This fixed mount needs a real impact or its powered release; a counterweight cannot lift an arbitrary anchor"));
            bool Weight = false;
            const auto* Pad=FindMarker(Support->AnchorMarker);
            for (const auto& B : State.Bodies) if (Pad&&B.Id!=Target->Id&&Live(B) && !Held(B) && B.Mass >= Support->MinimumMass && Near(B.Position,Pad->Position,Pad->Radius)&&B.Velocity.Size()<45) Weight = true;
            if ((!Has(Rig,TEXT("counterweight_hook"))&&!Has(Rig,TEXT("walking_gantry"))) || !Weight) return Refuse(TEXT("Support needs one real 8 kg body resting on the marked counterweight pad"));
        }
        if (!Near(C.Destination, InTray(C.Destination), 1)) return Refuse(TEXT("Place the anchor inside the worksite"));
        P.BodyIds = GetGroup(TargetId); P.BatteryCost = 8;
        if (Has(Rig,TEXT("ratchet_pawl"))) P.BatteryCost += 2;
        if(Has(Rig,TEXT("walking_gantry")))P.BatteryCost+=6;
        break;
    }
    case EExpeditionAction::Vector:
        if (!Target || Target->State != EExpeditionBodyState::Available || Target->bAnchored) return Refuse(TEXT("Aim at an unanchored body to shove"));
        if (Near(C.Aim,C.Magnet,4)) return Refuse(TEXT("Aim across the magnet to choose a shove direction"));
        P.BodyIds = GetGroup(TargetId); P.BatteryCost = 6;
        if (Has(Rig,TEXT("eddy_brake"))&&Target->Velocity.Size()>100) P.BatteryCost += 2;
        if(Has(Rig,TEXT("shear_gate"))&&P.BodyIds.ContainsByPredicate([&](int32 Id){const auto* B=FindBody(Id);return B&&!B->bAnchored&&!B->bGoal&&!B->Links.IsEmpty();}))
        {
            P.BatteryCost+=3;
            if(Has(Rig,TEXT("cold_seam")))P.BatteryCost+=2;
            if(Has(Rig,TEXT("crack_follower")))P.BatteryCost+=3;
            if(Has(Rig,TEXT("intact_recovery"))&&P.BodyIds.ContainsByPredicate([&](int32 Id){return FindBody(Id)->bFunctional;}))P.BatteryCost+=6;
            const FVector2D Direction=(C.Aim-C.Magnet).GetSafeNormal();
            const FVector2D Origin=Target->Position+Direction*55.f;
            const FVector2D Across(-Direction.Y,Direction.X);
            P.PathPoints={Origin-Across*75.f,Origin+Across*75.f};
        }
        if(Has(Rig,TEXT("reaction_frame"))&&C.SecondaryId!=INDEX_NONE)
        {
            const auto* Partner=FindBody(C.SecondaryId);
            if(!Partner||Partner->Id==Target->Id||!Live(*Partner)||Held(*Partner)||!FindSupport(Partner->Id))return Refuse(TEXT("Choose a visibly supported payload as the reaction partner"));
            if(Target->Mass<8)return Refuse(TEXT("This support needs at least 8 kg of real ballast to lift its payload"));
            P.BatteryCost+=6;if(P.PathPoints.IsEmpty())P.PathPoints={Target->Position,Partner->Position};
        }
        break;
    case EExpeditionAction::Relay:
        if (!Target || Target->State != EExpeditionBodyState::Available || Target->bAnchored) return Refuse(TEXT("Choose an available source body"));
        P.BodyIds = GetGroup(TargetId); P.BatteryCost = 10;
        if (!Near(C.Destination,InTray(C.Destination,25),1) || !Near(Target->Position,C.Destination,440)) return Refuse(TEXT("Receiver must be inside the site and within 440 units"));
        for (int32 Id : P.BodyIds) if (FindBody(Id)->bAnchored) return Refuse(TEXT("Release the connected supports before transfer"));
        P.PathPoints={Target->Position,C.Destination};
        if(Has(Rig,TEXT("flow_splitter"))&&C.bHasSecondaryPoint)
        {
            if(!Near(C.SecondaryPoint,InTray(C.SecondaryPoint,25),1)||!Near(Target->Position,C.SecondaryPoint,440)||Near(C.Destination,C.SecondaryPoint,55))return Refuse(TEXT("Choose two separate receivers within the 440-unit source range"));
            for(const auto& B:State.Bodies)if(B.State==EExpeditionBodyState::Available&&!B.bAnchored&&!P.BodyIds.Contains(B.Id)&&Near(B.Position,Target->Position,110))
            {
                const auto Group=GetGroup(B.Id);bool Valid=true;float Mass=0;for(int32 I:P.BodyIds)Mass+=FindBody(I)->Mass;
                for(int32 I:Group){const auto* G=FindBody(I);Valid&=!G->bAnchored&&!Held(*G);Mass+=G->Mass;}
                if(Valid&&Mass<=SafeCapacity)P.BodyIds.Append(Group);
            }
            if(P.BodyIds.Num()==GetGroup(TargetId).Num())return Refuse(TEXT("A split field needs two independent groups; it cannot cut intact links"));
            P.BatteryCost+=4;P.PathPoints.Add(Target->Position);P.PathPoints.Add(C.SecondaryPoint);
        }
        break;
    case EExpeditionAction::Ground:
        if(!Has(Rig,TEXT("ground_clip"))||!Target||!Live(*Target)||!Target->bConductive||Held(*Target))return Refuse(TEXT("Choose an available conductive endpoint for Ground Clip"));
        P.BodyIds={TargetId};P.BatteryCost=0;break;
    case EExpeditionAction::ArmRelay:
    {
        const auto* Receiver=FindBody(C.SecondaryId);
        if(!Has(Rig,TEXT("escapement_relay"))||!Target||!Live(*Target)||!Receiver||!IsTerminalBody(*Receiver))return Refuse(TEXT("Choose a physical sensor body, then a receiver terminal"));
        if(!IsManualTargetExposed(C.SecondaryId))return Refuse(TEXT("Contact covered by the frame. Move the frame, or energize the exposed loop input."));
        if(State.DeferredCharge>0)return Refuse(TEXT("A real relay charge is already waiting for its sensor impact"));
        if(IsTerminalPowered(C.SecondaryId))return Refuse(TEXT("That terminal has already actuated its mechanism"));
        if(!HasTerminalWork(C.SecondaryId))return Refuse(TEXT("That payload is already independently released or recovered"));
        if(State.LayoutId!=TEXT("e1"))
        {
            bool UsedBridge=false;
            if(!ArcPath(C.SecondaryId,Rig,P.BodyIds,&UsedBridge)||P.BodyIds.IsEmpty())return Refuse(TEXT("This isolated receiver needs a connected, preserved generator with a stored charge"));
            const auto* Source=FindBody(P.BodyIds[0]);
            if(!Source||!Source->bFunctional||Source->Charge<=0)return Refuse(TEXT("No finite generator charge is available to reserve"));
            P.BodyIds.AddUnique(TargetId);P.BatteryCost=11+(UsedBridge?4:0);
        }
        else {P.BodyIds={TargetId,C.SecondaryId};P.BatteryCost=11;}
        break;
    }
    case EExpeditionAction::TransferHeat:
    {
        const auto* Sink=FindBody(C.SecondaryId);
        if(!Has(Rig,TEXT("heat_sink_mould"))||(!Has(Rig,TEXT("vector_emitter"))&&!Has(Rig,TEXT("relay_projector"))))return Refuse(TEXT("Heat transfer requires Heat-Sink Mould and a field tool"));
        if(!Target||!Sink||Target->Id==Sink->Id||Target->State!=EExpeditionBodyState::Available||Sink->State!=EExpeditionBodyState::Available||!Target->bHot||Sink->bHot||Sink->bSinkUsed||Sink->Material!=EExpeditionMaterial::Iron||!Near(Target->Position,Sink->Position,110)||Target->Velocity.Size()>45||Sink->Velocity.Size()>45)return Refuse(TEXT("Choose a resting hot source and nearby unused cool iron sink"));
        P.BodyIds={TargetId,C.SecondaryId};P.BatteryCost=4;break;
    }
    case EExpeditionAction::SwitchAnchor:
        if(!Has(Rig,TEXT("twin_anchor"))||State.SecondTetherBody==INDEX_NONE)return Refuse(TEXT("Place a second paid anchor before switching"));
        P.BatteryCost=0;P.TargetId=State.SecondTetherBody;P.BodyIds={State.SecondTetherBody};break;
    case EExpeditionAction::LayGuide:
        if(!Has(Rig,TEXT("field_loom"))||!C.bHasSecondaryPoint)return Refuse(TEXT("Field Loom needs an entrance, bend and exit"));
        P.PathPoints={C.Aim,C.SecondaryPoint,C.Destination};P.BatteryCost=16;
        for(const auto& Point:P.PathPoints)if(!Near(Point,InTray(Point,28),1))return Refuse(TEXT("Keep the entire guide inside the worksite"));
        for(int32 I=0;I<2;++I)
        {
            const FVector2D A=P.PathPoints[I],B=P.PathPoints[I+1];
            if(!Near(A,B,440)||Near(A,B,45))return Refuse(TEXT("Each guide leg must be 45 to 440 units long"));
            for(int32 S=0;S<=50;++S){const FVector2D Q=FMath::Lerp(A,B,S/50.f);for(const auto& O:Obstacles)if(FMath::Abs(Q.X-O.Center.X)<O.HalfSize.X+23&&FMath::Abs(Q.Y-O.Center.Y)<O.HalfSize.Y+23)return Refuse(TEXT("The guide corridor crosses solid geometry"));}
        }
        break;
    case EExpeditionAction::Interact:
        P.BatteryCost = 0; return Refuse(TEXT("The visible fittings respond to moved metal; E unloads or dispatches at the receivers"));
    default: return Refuse(TEXT("Unknown operation"));
    }
    for (int32 Id : P.BodyIds) if (const auto* B = FindBody(Id)) { P.Mass += B->Mass; P.Value += B->Appraisal; P.bUnsafe |= B->bHot; }
    if (C.Action == EExpeditionAction::Relay && P.Mass > SafeCapacity) return Refuse(TEXT("A field transfer carries one group up to 24 kg"));
    P.bUnsafe |= (C.Action == EExpeditionAction::Attract || C.Action == EExpeditionAction::Extract) && GetCargoMass() + P.Mass > SafeCapacity;
    if (State.Battery < P.BatteryCost) return Refuse(TEXT("Not enough battery; retained cargo can still be unloaded or dispatched"));
    P.bAllowed = true;
    P.Reason = FString::Printf(TEXT("%d battery — %.0f kg%s"),P.BatteryCost,P.Mass,P.bUnsafe ? TEXT(" — unsafe haul: 3-second fuse") : TEXT(""));
    return P;
}

void FExpeditionWorld::Sever(int32 Id, const FExpeditionRig& Rig)
{
    auto* B = MutableBody(Id); if (!B) return;
    const TArray<int32> OldLinks = B->Links;
    auto MountReleased=[&](FExpeditionBody& Part)
    {if(Part.bFunctional&&Part.Links.IsEmpty()&&!Has(Rig,TEXT("intact_recovery"))){Part.bFunctional=false;Part.Charge=0;Damage(Part,20);}};
    bool LiveCut = false;
    for (int32 Link : OldLinks)
    {
        if (auto* L = MutableBody(Link))
        {
            LiveCut |= L->bHot;
            L->Links.Remove(Id);
            MountReleased(*L);
            if (Has(Rig,TEXT("cold_seam"))) L->bBrittle = true;
        }
    }
    B->Links.Reset();
    MountReleased(*B);
    if (LiveCut && !Has(Rig,TEXT("insulated_jaw"))) B->bHot = true;
    if (Has(Rig,TEXT("crack_follower")))
    {
        for (int32 Link : OldLinks)
        {
            auto* L = MutableBody(Link);
            if (!L || L->Links.IsEmpty()) continue;
            const int32 Next = L->Links[0]; L->Links.Remove(Next);MountReleased(*L); if (auto* N = MutableBody(Next)){N->Links.Remove(Link);MountReleased(*N);}
            break; // One additional edge, never a recursive module invocation.
        }
    }
    AddEvent(EExpeditionEventKind::Severed, TEXT("Selected seam released — ballast stays behind"), B->Position, {Id});
}

FExpeditionResult FExpeditionWorld::Execute(const FExpeditionCommand& C, const FExpeditionRig& Rig)
{
    const FExpeditionPreview P = Preview(C,Rig);
    FExpeditionResult R; R.Message = P.Reason;
    if (!P.bAllowed) return R;
    State.Battery -= P.BatteryCost; ++State.ActionSerial;
    R.bSucceeded = true; R.BatterySpent = P.BatteryCost; R.BodyIds = P.BodyIds;
    State.Magnet = C.Magnet;
    switch (C.Action)
    {
    case EExpeditionAction::Attract: case EExpeditionAction::Extract:
        if (C.Action == EExpeditionAction::Extract) Sever(P.TargetId,Rig);
        State.PullIds = P.BodyIds; State.PullRemaining = 1.2f;
        {
            TSet<int32> Prepared; int32 GroupIndex=0;
            for(int32 Id:P.BodyIds)
            {
                if(Prepared.Contains(Id))continue;
                const auto Group=GetGroup(Id); FVector2D Center=FVector2D::ZeroVector;
                for(int32 G:Group)Center+=FindBody(G)->Position;
                Center/=FMath::Max(1,Group.Num());
                const FVector2D Local(GroupIndex==0?0:((GroupIndex+1)/2)*(GroupIndex%2?24:-24),GroupIndex>3?22:0);
                ++GroupIndex;
                for(int32 G:Group)if(auto* B=MutableBody(G);B&&P.BodyIds.Contains(G)) { State.CycloneIds.Remove(G);B->CargoOffset=B->Position-Center+Local; B->State=EExpeditionBodyState::Pulling; B->RootAction=State.ActionSerial; Prepared.Add(G); }
            }
        }
        R.Message = C.Action == EExpeditionAction::Extract ? TEXT("Extracting selected piece") : TEXT("Field committed — steer the pull");
        break;
    case EExpeditionAction::Weld:
    {
        FExpeditionBody Slug; Slug.Id = State.NextBodyId++; Slug.Role = TEXT("welded_slug"); Slug.State = EExpeditionBodyState::Cargo; Slug.Position = C.Magnet; Slug.Mass = 0; Slug.Value = 0; Slug.Appraisal=0; Slug.RootAction = State.ActionSerial;
        for (int32 Id : P.BodyIds)
        {
            auto* B = MutableBody(Id); Slug.Mass += B->Mass; Slug.Value += B->Value; Slug.Appraisal+=B->Appraisal;Slug.Quality = FMath::Min(Slug.Quality,B->Quality); Slug.SourceIds.Append(B->SourceIds);
            const auto Links = B->Links; for (int32 Link : Links) if (auto* L = MutableBody(Link)) L->Links.Remove(Id);
            B->Links.Reset(); B->State = EExpeditionBodyState::Consumed; B->Velocity = FVector2D::ZeroVector;
        }
        Slug.Radius = FMath::Clamp(12.f + Slug.Mass, 20.f, 34.f);
        State.Bodies.Add(Slug); R.BodyIds = {Slug.Id}; R.Message = TEXT("Counterweight welded — 4 battery paid; switch tool to keep it, or aim and launch for 8");
        AddEvent(EExpeditionEventKind::Welded,R.Message,Slug.Position,R.BodyIds);
        break;
    }
    case EExpeditionAction::Launch:
    {
        auto* B = MutableBody(P.TargetId);
        const auto Links = B->Links; for (int32 L : Links) if (auto* Other = MutableBody(L)) Other->Links.Remove(B->Id); B->Links.Reset();
        const bool Orbit=State.CycloneIds.Remove(B->Id)>0;
        B->State = EExpeditionBodyState::Available; if(!Orbit)B->Position = InTray(C.Magnet,B->Radius); B->Velocity = (C.Aim-(Orbit?B->Position:C.Magnet)).GetSafeNormal() * (560.f + FMath::Min(B->Mass,12.f)*12.f); B->bLaunched = true; B->bRebounded = false; B->bPunched = false; B->PenetratedBody=INDEX_NONE; B->RootAction = State.ActionSerial; B->RecoverDelay = .5f; B->FuseDelay = -1;
        R.Message = TEXT("Metal launched — collision and remaining mass determine the result");
        break;
    }
    case EExpeditionAction::Arc:
    {
        const int32 Terminal=P.TargetId==5?4:P.TargetId;
        const bool PrimaryWork=!IsTerminalPowered(Terminal)&&HasTerminalWork(Terminal);
        if(PrimaryWork)FireTerminal(Terminal);
        R.Message=Terminal==11?TEXT("Local terminal powered — arm parked clear"):TEXT("Conducting path actuated the remote latch");
        if(Terminal==62)R.Message=PrimaryWork?TEXT("Frame mount released by its stored charge"):TEXT("Exposed input passes the remaining charge into the completed return");
        else if(Terminal==63)R.Message=TEXT("Floor contact powered — the receiving hoist is moving the real frame");
        else if(State.LayoutId==TEXT("balanced_rack"))R.Message=Terminal==4?TEXT("Stored generator charge released the supported machine"):TEXT("Stored generator charge released the brittle brace intact");
        else if(State.LayoutId==TEXT("counterweight_exchange"))R.Message=TEXT("Stored generator charge latched the dispatch counterbalance");
        // A portable recovered source is depleted when it supplies an otherwise unavailable path.
        if(PrimaryWork&&!P.BodyIds.IsEmpty())if(auto* Source=MutableBody(P.BodyIds[0]);Source&&Source->bFunctional&&Source->Charge>0)--Source->Charge;
        TArray<int32> Reachable=P.BodyIds;
        for(int32 I=0;I<Reachable.Num();++I)for(int32 N:ConductiveNeighbors(Reachable[I],Rig))Reachable.AddUnique(N);
        const bool GroundOnBranch=State.GroundBody!=INDEX_NONE&&Reachable.Contains(State.GroundBody);
        const int32 SourceId=P.BodyIds.IsEmpty()?INDEX_NONE:P.BodyIds[0];
        if(Has(Rig,TEXT("closed_circuit"))&&CircuitLoop(Rig,Reachable,SourceId))
        {
            TArray<int32> Terminals;for(int32 Id:Reachable)if(const auto* B=FindBody(Id);B&&IsTerminalBody(*B)&&Id!=Terminal&&Id!=State.GroundBody)Terminals.AddUnique(Id);
            for(int32 Id:Terminals)
            {
                FExpeditionBody* Source=MutableBody(SourceId);
                if(!Source||Source->Charge<=0||!Live(*Source))Source=nullptr;
                if(IsTerminalPowered(Id)||!HasTerminalWork(Id))continue;
                if(!Source)break;--Source->Charge;FireTerminal(Id);R.BodyIds.AddUnique(Id);
                AddEvent(EExpeditionEventKind::Discharge,TEXT("Closed return circuit — distinct stored charge powers another branch"),FindBody(Id)->Position,{Source->Id,Id});
            }
        }
        else if(P.BodyIds.Num()>1&&!GroundOnBranch)
        {
            if(auto* Source=MutableBody(5);Source&&Source->Charge>0){--Source->Charge;if(auto* Conductor=MutableBody(6))Conductor->bHot=true;}
        }
        AddEvent(EExpeditionEventKind::Discharge,R.Message,C.Aim,P.BodyIds); UpdateMechanisms(); break;
    }
    case EExpeditionAction::Winch:
    {
        auto* B = MutableBody(P.TargetId);
        const auto* Support=FindSupport(B->Id);
        if(B->bAnchored||(B->Role==TEXT("power_frame")&&!State.bFrameHoistPowered&&(Has(Rig,TEXT("walking_gantry"))||Has(Rig,TEXT("counterweight_hook")))))
        {
            check(Support); // Preview permits only an authored physical support relation.
            const auto* Pad=FindMarker(Support->AnchorMarker);
            int32 Weight=INDEX_NONE;for(const auto& W:State.Bodies)if(Pad&&W.Id!=B->Id&&Live(W)&&!Held(W)&&W.Mass>=Support->MinimumMass&&W.Velocity.Size()<45&&Near(W.Position,Pad->Position,Pad->Radius)){Weight=W.Id;break;}
            if(Weight!=INDEX_NONE)
            {
                State.Constraints.RemoveAll([&](const FExpeditionConstraint& K){return K.BodyA==B->Id;});
                FExpeditionConstraint K;K.Kind=Has(Rig,TEXT("walking_gantry"))?EExpeditionConstraintKind::Gantry:EExpeditionConstraintKind::Counterweight;K.BodyA=B->Id;K.BodyB=Weight;K.Anchor=Pad->Position;K.Offset=FindBody(Weight)->Position-B->Position;State.Constraints.Add(K);
                B->bAnchored=false;State.bCounterweightUsed=true;
                AddEvent(EExpeditionEventKind::Mechanism,TEXT("Real counterweight supports the intact machine — keep that support in place"),B->Position,{B->Id,Weight});
            }
        }
        else if(Has(Rig,TEXT("walking_gantry")))for(auto& K:State.Constraints)if(K.bActive&&K.BodyA==B->Id){K.Kind=EExpeditionConstraintKind::Gantry;K.Offset=FindBody(K.BodyB)->Position-B->Position;}
        if (State.TetherBody != INDEX_NONE && Has(Rig,TEXT("twin_anchor"))) { State.SecondTetherBody = State.TetherBody; State.SecondTetherAnchor = State.TetherAnchor; }
        State.TetherBody = B->Id; State.TetherAnchor = C.Destination;
        for (int32 Id : P.BodyIds) if (auto* G = MutableBody(Id); G && Held(*G)) G->State = EExpeditionBodyState::Available;
        R.Message = TEXT("Anchor placed — tension moves the load"); break;
    }
    case EExpeditionAction::Vector: case EExpeditionAction::Relay:
    {
        if (C.Action == EExpeditionAction::Vector)
        {
            const FVector2D Direction = (C.Aim-C.Magnet).GetSafeNormal();
            for (int32 Id : P.BodyIds) if (auto* B = MutableBody(Id))
            {
                if(Has(Rig,TEXT("eddy_brake"))&&B->Velocity.Size()>100) { B->Velocity*=.15f;B->bHot=true;AddEvent(EExpeditionEventKind::Impact,TEXT("Eddy brake absorbs real motion into heat"),B->Position,{Id}); }
                else B->Velocity+=Direction*360.f;
                B->RootAction = State.ActionSerial;
            }
            if(Has(Rig,TEXT("shear_gate"))&&P.BodyIds.ContainsByPredicate([&](int32 Id){const auto* B=FindBody(Id);return B&&!B->bAnchored&&!B->bGoal&&!B->Links.IsEmpty();})) {State.ShearOrigin=FindBody(P.TargetId)->Position+Direction*55.f;State.ShearNormal=Direction;State.ShearRemaining=2.f;State.ShearIds=P.BodyIds;}
            if(Has(Rig,TEXT("reaction_frame"))&&C.SecondaryId!=INDEX_NONE)
            {
                auto* A=MutableBody(C.SecondaryId);auto* B=MutableBody(P.TargetId);A->bAnchored=false;A->Velocity-=Direction*360.f*(B->Mass/A->Mass);
                FExpeditionConstraint K;K.Kind=EExpeditionConstraintKind::Reaction;K.BodyA=A->Id;K.BodyB=B->Id;K.Anchor=(A->Position*A->Mass+B->Position*B->Mass)/(A->Mass+B->Mass);K.Offset=B->Position-A->Position;
                State.Constraints.Add(K);
                AddEvent(EExpeditionEventKind::Mechanism,TEXT("Support routes opposite force into the second real load"),K.Anchor,{A->Id,B->Id});
            }
            R.Message = TEXT("Directional impulse committed");
        }
        else
        {
            State.RelayBody=P.TargetId;State.RelayDestination=C.Destination;State.RelayRemaining=4.f;State.RelayIds=GetGroup(P.TargetId);State.SecondRelayIds.Reset();State.SecondRelayBody=INDEX_NONE;
            if(Has(Rig,TEXT("flow_splitter"))&&C.bHasSecondaryPoint)for(int32 Id:P.BodyIds)if(!State.RelayIds.Contains(Id))State.SecondRelayIds.Add(Id);
            if(!State.SecondRelayIds.IsEmpty()){State.SecondRelayBody=State.SecondRelayIds[0];State.SecondRelayDestination=C.SecondaryPoint;}
            R.Message=TEXT("Committed fields move conserved groups to their chosen receivers");
        }
        break;
    }
    case EExpeditionAction::Ground:
        State.GroundBody=P.TargetId;R.Message=TEXT("Ground endpoint placed — discharge stops at that body");break;
    case EExpeditionAction::ArmRelay:
        if(State.LayoutId!=TEXT("e1"))if(auto* Source=MutableBody(P.BodyIds[0]))--Source->Charge;
        State.DeferredSensor=P.TargetId;State.DeferredTerminal=C.SecondaryId;State.DeferredCharge=1;R.Message=TEXT("Charge reserved — only the chosen sensor's physical impact will fire it");break;
    case EExpeditionAction::TransferHeat:
        if(auto* Hot=MutableBody(P.TargetId))if(auto* Sink=MutableBody(C.SecondaryId)){Hot->bHot=false;Sink->bHot=true;Sink->bSinkUsed=true;R.Message=TEXT("Hazard moved into sacrificial iron; separate the bodies before recovery");}break;
    case EExpeditionAction::SwitchAnchor:
        Swap(State.TetherBody,State.SecondTetherBody);Swap(State.TetherAnchor,State.SecondTetherAnchor);R.Message=TEXT("Switched to the other previously paid anchor");break;
    case EExpeditionAction::LayGuide:
        State.GuidePoints=P.PathPoints;State.GuideUses=1;State.GuideBody=INDEX_NONE;State.GuideSegment=0;State.GuideSpeed=0;R.Message=TEXT("One physical guide armed — launch through its entrance when ready");break;
    default: break;
    }
    AddEvent(EExpeditionEventKind::Action,R.Message,C.Aim,R.BodyIds,P.BatteryCost);
    return R;
}

void FExpeditionWorld::CancelPull()
{
    for (int32 Id : State.PullIds) if (auto* B = MutableBody(Id); B && B->State == EExpeditionBodyState::Pulling) { B->State = EExpeditionBodyState::Available; B->Position = InTray(B->Position,B->Radius); B->RecoverDelay = .2f; }
    State.PullIds.Reset(); State.PullRemaining = 0;
}

FExpeditionResult FExpeditionWorld::Drop()
{
    FExpeditionResult R;
    const FVector2D Origin = State.Magnet;
    TArray<int32> Ids;
    for (const auto& B : State.Bodies) if (Held(B)) Ids.Add(B.Id);
    // Preserve group-relative positions. Different components are placed on separated deterministic grid cells.
    TSet<int32> Placed; int32 Slot = 0;
    for (int32 Id : Ids)
    {
        if (Placed.Contains(Id)) continue;
        const TArray<int32> Group = GetGroup(Id);
        FVector2D Center = FVector2D::ZeroVector; int32 Count = 0;
        for (int32 G : Group) if (const auto* B = FindBody(G); B && Held(*B)) { Center += B->Position; ++Count; }
        if (!Count) continue;
        Center /= Count;
        const FVector2D Offset = Slot==0 ? FVector2D::ZeroVector : FVector2D(((Slot-1)%3-1)*54.f,((Slot-1)/3+1)*54.f); ++Slot;
        // Clamp the component as a whole against its real translated bounds. A
        // singleton can be placed at the visible cursor near an edge; linked
        // members keep their offsets instead of being individually crushed there.
        FVector2D Lower(MinX,MinY),Upper(MaxX,MaxY);
        for(int32 G:Group)if(const auto* B=FindBody(G);B&&Held(*B))
        {
            const FVector2D Relative=B->Position-Center;
            Lower.X=FMath::Max(Lower.X,double(MinX+B->Radius-Relative.X));
            Lower.Y=FMath::Max(Lower.Y,double(MinY+B->Radius-Relative.Y));
            Upper.X=FMath::Min(Upper.X,double(MaxX-B->Radius-Relative.X));
            Upper.Y=FMath::Min(Upper.Y,double(MaxY-B->Radius-Relative.Y));
        }
        const FVector2D Goal(FMath::Clamp(Origin.X+Offset.X,Lower.X,Upper.X),FMath::Clamp(Origin.Y+Offset.Y,Lower.Y,Upper.Y));
        for (int32 G : Group) if (auto* B = MutableBody(G); B && Held(*B))
        {
            B->State = EExpeditionBodyState::Available; B->Position = InTray(Goal+(B->Position-Center),B->Radius); B->Velocity = FVector2D::ZeroVector; B->RecoverDelay = .5f; B->CargoOffset = FVector2D::ZeroVector; R.BodyIds.Add(G); Placed.Add(G);
        }
    }
    CancelPull(); State.UnsafeElapsed = 0;
    State.RelayBody=INDEX_NONE;State.SecondRelayBody=INDEX_NONE;State.RelayRemaining=0;State.RelayIds.Reset();State.SecondRelayIds.Reset();
    State.CycloneIds.Reset();
    State.GuideBody=INDEX_NONE;State.ShearIds.Reset();State.ShearRemaining=0;
    State.TetherBody = INDEX_NONE; State.SecondTetherBody = INDEX_NONE;
    R.bSucceeded = !R.BodyIds.IsEmpty(); R.Message = TEXT("Whole haul dropped — every piece remains recoverable");
    if (R.bSucceeded) AddEvent(EExpeditionEventKind::Dropped,R.Message,Origin,R.BodyIds);
    return R;
}

void FExpeditionWorld::Quench(const FString& Reason)
{
    if (State.HazardCooldown > 0 || IsEnded()) return;
    const auto R = Drop();
    if (!R.bSucceeded) return;
    const int32 Cost = FMath::Min(12,State.Battery); State.Battery -= Cost; State.HazardCooldown = 1.5f;
    AddEvent(EExpeditionEventKind::Quench,Reason+TEXT(" — recoverable spill, 12 battery"),State.Magnet,R.BodyIds,Cost);
}

bool FExpeditionWorld::CanBank(FString& Reason) const
{
    if (IsEnded()) { Reason=TEXT("Site ended"); return false; }
    if (!Near(State.Magnet,FurnacePosition(),95)) { Reason=TEXT("Move the magnet to the scrap furnace"); return false; }
    if (IsUnsafe()) { Reason=TEXT("Unsafe haul — right-click to drop the whole haul"); return false; }
    if (!State.PullIds.IsEmpty()) { Reason=TEXT("Let the paid attraction settle"); return false; }
    for(const auto& B:State.Bodies) if(B.State==EExpeditionBodyState::Cargo&&!B.bGoal&&!Near(B.Position,FurnacePosition(),150)) { Reason=TEXT("Bring the actual scrap haul into the furnace bay"); return false; }
    if (GetCargoValue() <= 0) { Reason=TEXT("The furnace accepts scrap; take the unique machine to its receiver"); return false; }
    Reason.Reset(); return true;
}
FExpeditionResult FExpeditionWorld::Bank()
{
    FExpeditionResult R; if (!CanBank(R.Message)) return R;
    for (auto& B : State.Bodies) if (B.State == EExpeditionBodyState::Cargo && !B.bGoal && B.Value > 0)
    { R.Output += B.Appraisal; R.BodyIds.Add(B.Id); const auto Links=B.Links; for(int32 Link:Links)if(auto* L=MutableBody(Link))L->Links.Remove(B.Id); B.Links.Reset(); B.State = EExpeditionBodyState::Banked; B.Velocity = FVector2D::ZeroVector; }
    State.Output += R.Output; R.bSucceeded = R.Output > 0; R.Message=FString::Printf(TEXT("Refined +%d — output %d"),R.Output,State.Output);
    AddEvent(EExpeditionEventKind::Banked,R.Message,FurnacePosition(),R.BodyIds,R.Output); return R;
}
bool FExpeditionWorld::CanDispatch(FString& Reason) const
{
    if (IsEnded()) { Reason=TEXT("This objective was already settled"); return false; }
    if(State.LayoutId==TEXT("counterweight_exchange")&&!(State.bBallastCleared||State.bCircuitClosed)){Reason=TEXT("Replace 20 kg at the empty core cradle, or power its isolated latch with a preserved generator");return false;}
    if (!Near(State.Magnet,ReceiverPosition(),100)) { Reason=TEXT("Move the secured machine to the separate receiver"); return false; }
    if (IsUnsafe()) { Reason=TEXT("Stabilize the haul before dispatch"); return false; }
    for (const auto& B : State.Bodies) if (B.bGoal && B.State == EExpeditionBodyState::Cargo && Near(B.Position,ReceiverPosition(),110)) { Reason.Reset(); return true; }
    Reason=State.LayoutId==TEXT("e1")?TEXT("Release the three physical restraints, then secure the machine"):GetGoalText(); return false;
}
FExpeditionResult FExpeditionWorld::Dispatch()
{
    FExpeditionResult R; if (!CanDispatch(R.Message)) return R;
    for (auto& B : State.Bodies) if (B.bGoal && B.State == EExpeditionBodyState::Cargo) { B.State = EExpeditionBodyState::Dispatched; R.BodyIds.Add(B.Id); }
    State.bDispatched=true; R.bSucceeded=true; R.Message=TEXT("Machine delivered — site complete"); AddEvent(EExpeditionEventKind::Dispatched,R.Message,ReceiverPosition(),R.BodyIds); return R;
}
void FExpeditionWorld::Evacuate() { if (!IsEnded()) { Drop(); State.bEvacuated=true; } }

void FExpeditionWorld::UpdateMechanisms()
{
    const bool WasReleased = IsCoreReleased();
    if(State.LayoutId!=TEXT("e1"))
    {
        if(State.LayoutId==TEXT("balanced_rack"))
        {
            const float Left=PadMass({-250,160},60),Right=PadMass({250,160},60);
            State.bCollarReleased=Left>=10&&Left<=14&&Right>=10&&Right<=14&&FMath::Abs(Left-Right)<=2;
        }
        else
        {
            const auto* Cover=FindBody(1);const auto* Core=FindBody(0);
            State.bCollarReleased=!Cover||!Live(*Cover)||!Near(Cover->Position,{300,0},130);
            const bool Empty=Core&&!Near(Core->Position,{300,0},Core->Radius+28);
            State.bBallastCleared=Empty&&PadMass({300,0},28)>=20;
        }
        if(auto* Core=MutableBody(0))
        {
            Core->bAnchored=!IsCoreReleased();
            if(Core->bAnchored&&Core->State==EExpeditionBodyState::Pulling)
            {
                Core->State=EExpeditionBodyState::Available;Core->Velocity=FVector2D::ZeroVector;State.PullIds.Remove(0);
                AddEvent(EExpeditionEventKind::Mechanism,TEXT("Support changed before capture — restore the marked condition before pulling the core"),Core->Position,{0});
            }
        }
        if(!WasReleased&&IsCoreReleased())AddEvent(EExpeditionEventKind::Mechanism,TEXT("Machine accessible — keep the support valid until the core is actually secured"),FindBody(0)->Position,{0});
        return;
    }
    if (const auto* B=FindBody(1); B && B->Position.X <= CollarStop().X+18) State.bCollarReleased=true;
    if (const auto* B=FindBody(2); B && B->State == EExpeditionBodyState::Available && Near(B->Position,BallastCatch(),62)) State.bBallastCleared=true;
    if(const auto* B=FindBody(9);B&&!B->bAnchored&&Live(*B)&&!Near(B->Position,FVector2D(-20,105),38))State.bCollarReleased=true;
    if (!State.bArmBraced && IsArmSafe())
        for (auto& B : State.Bodies) if (B.State==EExpeditionBodyState::Available && B.Material==EExpeditionMaterial::Iron && B.Mass>=6 && Near(B.Position,ArmStopper(),53)) { State.bArmBraced=true; B.bAnchored=true; B.Velocity=FVector2D::ZeroVector; break; }
    if (IsCoreReleased()) if (auto* Core=MutableBody(0)) Core->bAnchored=false;
    if (!WasReleased && IsCoreReleased()) AddEvent(EExpeditionEventKind::Mechanism,TEXT("All restraints released — secure the unique machine"),FindBody(0)->Position,{0});
}

void FExpeditionWorld::FireTerminal(int32 Id)
{
    if(IsTerminalPowered(Id)||!HasTerminalWork(Id))return;
    State.PoweredTerminals.AddUnique(Id);
    if((Id==62||Id==63)&&FindFrameBody())
    {
        const int32 FrameId=FindFrameBody()->Id;
        State.Constraints.RemoveAll([&](const FExpeditionConstraint& K){return K.BodyA==FrameId;});
        MutableBody(FrameId)->bAnchored=false;
        if(Id==63)
        {
            State.bFrameHoistPowered=true;State.bFrameHoistActive=true;
            if(State.TetherBody==FrameId)State.TetherBody=INDEX_NONE;
            if(State.SecondTetherBody==FrameId)State.SecondTetherBody=INDEX_NONE;
            if(State.LatchedBody==FrameId)State.LatchedBody=INDEX_NONE;
        }
    }
    else if(State.LayoutId==TEXT("balanced_rack"))
    {
        if(auto* B=MutableBody(Id==4?9:10))
        {
            State.Constraints.RemoveAll([&](const FExpeditionConstraint& K){return K.BodyA==B->Id;});
            B->bAnchored=false;
        }
    }
    else if(State.LayoutId==TEXT("counterweight_exchange"))State.bCircuitClosed=true;
    else if(Id==11)State.bArmBraced=true;
    else if(Id==4||Id==5){State.bBallastCleared=true;State.bCircuitClosed=true;}
    UpdateMechanisms();
}
void FExpeditionWorld::ResolveSensor(int32 Id)
{
    if(State.DeferredCharge!=1||State.DeferredSensor!=Id)return;
    const int32 Terminal=State.DeferredTerminal;
    State.DeferredCharge=0;State.DeferredSensor=INDEX_NONE;State.DeferredTerminal=INDEX_NONE;
    if(Terminal!=State.GroundBody)FireTerminal(Terminal);
    AddEvent(EExpeditionEventKind::Discharge,TEXT("Selected sensor impact spent its reserved charge"),FindBody(Id)->Position,{Id,Terminal});
}

void FExpeditionWorld::UpdateConstraints(float Delta)
{
    for(auto& K:State.Constraints)
    {
        if(!K.bActive)continue;
        auto* A=MutableBody(K.BodyA);auto* B=MutableBody(K.BodyB);
        if(!A||!B||!Live(*A)||Held(*A)){K.bActive=false;continue;} // An independently secured/delivered load is never clawed back.
        const bool PawlHeld=State.LatchedBody==A->Id;
        const bool Supported=Live(*B)&&!Held(*B)&&B->Mass>=8&&(K.Kind!=EExpeditionConstraintKind::Counterweight||Near(B->Position,K.Anchor,55));
        if(!Supported&&!PawlHeld)
        {
            K.bActive=false;A->bAnchored=true;A->Velocity=FVector2D::ZeroVector;Damage(*A,15);
            if(State.TetherBody==A->Id)State.TetherBody=INDEX_NONE;
            AddEvent(EExpeditionEventKind::Mechanism,TEXT("Counterweight left its support — the unsecured machine settled and lost quality"),A->Position,{A->Id,B?B->Id:INDEX_NONE});continue;
        }
        if(K.Kind==EExpeditionConstraintKind::Gantry&&B&&Live(*B)&&!Held(*B))
        {B->Velocity+=Limit((A->Position+K.Offset-B->Position)*40.f-B->Velocity*6.f,1700.f)*Delta;}
        if(K.Kind==EExpeditionConstraintKind::Reaction&&B&&Live(*B)&&!Held(*B))
        {
            const FVector2D Center=(A->Position*A->Mass+B->Position*B->Mass)/(A->Mass+B->Mass);
            const FVector2D Correction=Limit((K.Anchor-Center)*24.f,900.f)*Delta;
            A->Velocity+=Correction;B->Velocity+=Correction;
        }
    }
}

void FExpeditionWorld::MakeCyclone(int32 BodyId)
{
    auto* Original=MutableBody(BodyId);
    if(!Original||!Live(*Original)||!Original->bLaunched||Original->bGoal)return;
    const FExpeditionBody Saved=*Original;
    State.CycloneCenter=Saved.Position;State.CycloneIds.Reset();
    if(Saved.SourceIds.Num()==1)
    {
        Original->bLaunched=false;Original->Velocity*=.3f;Original->RecoverDelay=.3f;State.CycloneIds.Add(BodyId);
    }
    else
    {
        Original->State=EExpeditionBodyState::Consumed;Original->Velocity=FVector2D::ZeroVector;Original->Links.Reset();
        int32 AppraisalLeft=Saved.Appraisal;
        for(int32 Part=0;Part<FMath::Min(6,Saved.SourceIds.Num());++Part)
        {
            const int32 Begin=Part,End=Part==5?Saved.SourceIds.Num():Part+1;
            FExpeditionBody Fragment;
            if(End-Begin==1)Fragment=*FindBody(Saved.SourceIds[Begin]);
            else {Fragment.Id=State.NextBodyId++;Fragment.Material=Saved.Material;Fragment.Mass=0;Fragment.Value=0;Fragment.SourceIds.Reset();for(int32 I=Begin;I<End;++I){const auto* Source=FindBody(Saved.SourceIds[I]);Fragment.SourceIds.Append(Source->SourceIds);Fragment.Mass+=Source->Mass;Fragment.Value+=Source->Value;}}
            Fragment.Role=TEXT("cyclone_fragment");Fragment.State=EExpeditionBodyState::Available;Fragment.Links.Reset();Fragment.bAnchored=false;Fragment.bLaunched=false;Fragment.bRebounded=false;Fragment.bPunched=false;Fragment.PenetratedBody=INDEX_NONE;Fragment.RootAction=Saved.RootAction;Fragment.Quality=Saved.Quality;Fragment.FuseDelay=Part==0&&Saved.FuseDelay>=0?Saved.FuseDelay:-2;Fragment.RecoverDelay=.3f;Fragment.Position=Saved.Position;Fragment.Radius=FMath::Clamp(12.f+Fragment.Mass,15.f,30.f);
            const bool Last=End==Saved.SourceIds.Num();Fragment.Appraisal=Last?AppraisalLeft:(Saved.Value>0?int32(int64(Saved.Appraisal)*Fragment.Value/Saved.Value):0);AppraisalLeft-=Fragment.Appraisal;
            const float Angle=Part*2.f*PI/FMath::Min(6,Saved.SourceIds.Num());Fragment.Velocity=FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*100.f;
            if(auto* Existing=MutableBody(Fragment.Id))*Existing=Fragment;else State.Bodies.Add(Fragment);
            State.CycloneIds.Add(Fragment.Id);
        }
    }
    AddEvent(EExpeditionEventKind::Mechanism,TEXT("Surviving source material held in a finite orbit — select a fragment for the next paid shot"),Saved.Position,State.CycloneIds);
}

void FExpeditionWorld::Impact(FExpeditionBody& B, const FVector2D& Normal, float Speed, const FExpeditionRig& Rig, bool bResolveVelocity)
{
    if (Speed < 75) return;
    ResolveSensor(B.Id);
    if(State.GuideBody==B.Id){State.GuideBody=INDEX_NONE;State.GuideUses=0;AddEvent(EExpeditionEventKind::Impact,TEXT("Physical obstruction stopped the field guide"),B.Position,{B.Id});}
    AddEvent(EExpeditionEventKind::Impact,TEXT("Metal impact"),B.Position,{B.Id},FMath::RoundToInt(Speed));
    if (B.Material == EExpeditionMaterial::Alloy || B.bGoal) Damage(B,Speed>360?15:5);
    if (B.bLaunched && Has(Rig,TEXT("impact_fuse")) && FMath::IsNearlyEqual(B.FuseDelay,-1.f)) B.FuseDelay=.35f;
    if(B.bLaunched&&Has(Rig,TEXT("salvage_cyclone")))PendingCyclones.AddUnique(B.Id);
    if(!bResolveVelocity)return; // Fracture already spent momentum; do not bounce the penetrating shot.
    if (B.bLaunched && Has(Rig,TEXT("rebound_plate")) && !B.bRebounded)
    { B.Velocity = (B.Velocity-2.f*FVector2D::DotProduct(B.Velocity,Normal)*Normal)*.85f; B.bRebounded=true; }
    else B.Velocity -= 1.15f*FVector2D::DotProduct(B.Velocity,Normal)*Normal;
}

void FExpeditionWorld::Integrate(float Delta, const FExpeditionRig& Rig)
{
    UpdateMechanisms();
    State.WorldTime += Delta; State.HazardCooldown=FMath::Max(0.f,State.HazardCooldown-Delta);
    State.PullRemaining=FMath::Max(0.f,State.PullRemaining-Delta);
    State.RelayRemaining=FMath::Max(0.f,State.RelayRemaining-Delta);
    if(State.LatchedBody!=INDEX_NONE)
    {
        const auto* Latched=FindBody(State.LatchedBody);
        if(!Latched||!Live(*Latched)||Held(*Latched))State.LatchedBody=INDEX_NONE;
    }
    UpdateConstraints(Delta);
    State.ShearRemaining=FMath::Max(0.f,State.ShearRemaining-Delta);
    if(State.ShearRemaining<=0)State.ShearIds.Reset();
    TArray<int32> RelayIds=State.RelayIds;
    const auto* RelayRoot=FindBody(State.RelayBody);
    const FVector2D RelayOrigin=RelayRoot?RelayRoot->Position:FVector2D::ZeroVector;
    const auto* SecondRoot=FindBody(State.SecondRelayBody);
    const FVector2D SecondOrigin=SecondRoot?SecondRoot->Position:FVector2D::ZeroVector;
    for (auto& B : State.Bodies)
    {
        B.RecoverDelay=FMath::Max(0.f,B.RecoverDelay-Delta);
        if (!Live(B)) continue;
        if (B.State==EExpeditionBodyState::Cargo)
        {
            const FVector2D Target=State.Magnet+B.CargoOffset;
            B.Velocity=Limit((Target-B.Position)*14.f,900.f); B.Position+=B.Velocity*Delta;
            continue; // Secured cargo may leave the tray for the two receivers.
        }
        if (B.bAnchored) { B.Velocity=FVector2D::ZeroVector; continue; }
        FVector2D Accel=FVector2D::ZeroVector;
        if(State.bFrameHoistActive&&B.Role==TEXT("power_frame"))
        {
            if(const auto* Dock=FindMarker(TEXT("frame_receiver")))
            {
                Accel+=Limit((Dock->Position-B.Position)*22.f-B.Velocity*9.f,1800.f);
                if(Near(B.Position,Dock->Position,8)&&B.Velocity.Size()<25)State.bFrameHoistActive=false;
            }
        }
        if(State.CycloneIds.Contains(B.Id))
        {
            const int32 Index=State.CycloneIds.Find(B.Id);const float Angle=State.WorldTime*1.8f+Index*2.f*PI/FMath::Max(1,State.CycloneIds.Num());
            const FVector2D Orbit=State.CycloneCenter+FVector2D(FMath::Cos(Angle),FMath::Sin(Angle))*58.f;
            Accel+=Limit((Orbit-B.Position)*32.f-B.Velocity*7.f,1600.f);
        }
        if(State.GuideUses>0&&State.GuidePoints.Num()==3&&(B.bLaunched||B.Velocity.Size()>100)&&Near(B.Position,State.GuidePoints[0],40))
        {State.GuideUses=0;State.GuideBody=B.Id;State.GuideSegment=1;State.GuideSpeed=B.Velocity.Size();}
        if(State.GuideBody==B.Id&&State.GuidePoints.IsValidIndex(State.GuideSegment))
        {
            if(Near(B.Position,State.GuidePoints[State.GuideSegment],23))++State.GuideSegment;
            if(State.GuideSegment>=State.GuidePoints.Num()){State.GuideBody=INDEX_NONE;AddEvent(EExpeditionEventKind::Mechanism,TEXT("Guided projectile exits along its selected physical route"),B.Position,{B.Id});}
            else B.Velocity=(State.GuidePoints[State.GuideSegment]-B.Position).GetSafeNormal()*State.GuideSpeed;
        }
        if (B.State==EExpeditionBodyState::Pulling)
        {
            const FVector2D D=State.Magnet+B.CargoOffset-B.Position;
            Accel+=Limit(D*50.f-B.Velocity*11.f,2600.f);
            if (D.Size()<30 && B.Material!=EExpeditionMaterial::Mechanism)
            {
                B.State=EExpeditionBodyState::Cargo; State.PullIds.Remove(B.Id);
                if(B.bGoal)State.bCoreSecured=true;
                AddEvent(EExpeditionEventKind::Captured,TEXT("Metal secured"),B.Position,{B.Id},B.Value);
            }
        }
        if (RelayIds.Contains(B.Id) && State.RelayRemaining>0) Accel+=Limit((State.RelayDestination+(B.Position-RelayOrigin)-B.Position)*22.f-B.Velocity*9.f,1800.f);
        if(State.SecondRelayIds.Contains(B.Id)&&State.RelayRemaining>0)Accel+=Limit((State.SecondRelayDestination-SecondOrigin)*22.f-B.Velocity*9.f,1800.f);
        auto Tow=[&](int32 BodyId,const FVector2D& Anchor)
        {
            if (BodyId==INDEX_NONE || !GetGroup(BodyId).Contains(B.Id)) return;
            const auto* Root=FindBody(BodyId); if (!Root) return;
            Accel+=Limit((Anchor-Root->Position)*18.f-B.Velocity*8.f,1800.f);
            if (Near(Root->Position,Anchor,24) && Has(Rig,TEXT("ratchet_pawl")) && (State.LatchedBody==INDEX_NONE||State.LatchedBody==BodyId)) { State.LatchedBody=BodyId; State.LatchPosition=Anchor; }
        };
        Tow(State.TetherBody,State.TetherAnchor); // The second paid anchor is saved, not another simultaneous force.
        if (State.LatchedBody==B.Id && State.TetherBody!=B.Id && State.SecondTetherBody!=B.Id) Accel+=Limit((State.LatchPosition-B.Position)*35.f-B.Velocity*10.f,1800.f);
        B.Velocity+=Accel*Delta;
        B.Velocity*=FMath::Exp(-(B.bLaunched?.32f:2.5f)*Delta);
        B.Velocity=Limit(B.Velocity,900.f);
        B.Position+=B.Velocity*Delta;
        if(State.ShearRemaining>0&&State.ShearIds.Contains(B.Id)&&!B.bAnchored&&!B.bGoal&&!B.Links.IsEmpty()&&FVector2D::DotProduct(B.Position-State.ShearOrigin,State.ShearNormal)>=0&&FMath::Abs(FVector2D::DotProduct(B.Position-State.ShearOrigin,FVector2D(-State.ShearNormal.Y,State.ShearNormal.X)))<=75.f+B.Radius)
        {State.ShearIds.Remove(B.Id);Sever(B.Id,Rig);AddEvent(EExpeditionEventKind::Severed,TEXT("Moving seam crossed the committed shear plane"),B.Position,{B.Id});}
        if (B.Role==TEXT("collar")) { B.Position.Y=-120; B.Position.X=FMath::Clamp(B.Position.X,165.0,280.0); B.Velocity.Y=0; }
        const FVector2D Clamped=InTray(B.Position,B.Radius);
        if (!Near(Clamped,B.Position,.01f)) { const FVector2D N=(Clamped-B.Position).GetSafeNormal(); Impact(B,N,B.Velocity.Size(),Rig); B.Position=Clamped; }
        for (const auto& O:Obstacles)
        {
            const FVector2D Closest(FMath::Clamp(B.Position.X,O.Center.X-O.HalfSize.X,O.Center.X+O.HalfSize.X),FMath::Clamp(B.Position.Y,O.Center.Y-O.HalfSize.Y,O.Center.Y+O.HalfSize.Y));
            FVector2D Diff=B.Position-Closest;
            if (Diff.SizeSquared()>=B.Radius*B.Radius) continue;
            if (Diff.IsNearlyZero()) Diff=FVector2D(B.Position.X<O.Center.X?-1:1,0);
            const FVector2D N=Diff.GetSafeNormal(); B.Position=Closest+N*(B.Radius+.3f); Impact(B,N,B.Velocity.Size(),Rig);
        }
        if (B.bLaunched && B.Velocity.Size()<35) B.bLaunched=false;
        if (B.FuseDelay>=0)
        {
            B.FuseDelay-=Delta;
            if (B.FuseDelay<=0)
            {
                B.FuseDelay=-2; Damage(B,20);
                for (auto& Other:State.Bodies) if (Other.Id!=B.Id && Live(Other) && !Other.bAnchored && Near(Other.Position,B.Position,95)) Other.Velocity+=(Other.Position-B.Position).GetSafeNormal()*260.f;
                AddEvent(EExpeditionEventKind::Impact,TEXT("Delayed impact fuse — finite secondary impulse"),B.Position,{B.Id});
            }
        }
    }
    // Pair collisions share impulses; linked members instead retain their original resting separation.
    for (int32 I=0;I<State.Bodies.Num();++I) for (int32 J=I+1;J<State.Bodies.Num();++J)
    {
        auto& A=State.Bodies[I]; auto& B=State.Bodies[J];
        if (!Live(A)||!Live(B)||Held(A)||Held(B)||A.bFloorContact||B.bFloorContact) continue;
        if((A.bLaunched&&A.PenetratedBody==B.Id)||(B.bLaunched&&B.PenetratedBody==A.Id))continue;
        FVector2D D=B.Position-A.Position; float Distance=D.Size();
        if (A.Links.Contains(B.Id))
        {
            const float Rest=46.f;
            if (Distance>.01f) { const FVector2D Correction=D*((Distance-Rest)/Distance*.35f); if(!A.bAnchored)A.Position+=Correction; if(!B.bAnchored)B.Position-=Correction; }
            continue;
        }
        const float Gap=A.Radius+B.Radius;
        if (Distance>=Gap) continue;
        const FVector2D N=Distance>.001f?D/Distance:FVector2D(1,0);
        float InvA=A.bAnchored?0.f:1.f/A.Mass, InvB=B.bAnchored?0.f:1.f/B.Mass;
        if (InvA+InvB<=0) continue;
        const float Closing=FVector2D::DotProduct(A.Velocity-B.Velocity,N);
        if (Closing>40)
        {
            auto Break=[&](FExpeditionBody& Hit,FExpeditionBody& Shot) -> bool
            {
                if (!Hit.bBrittle || !Shot.bLaunched || Shot.Mass<4 || Closing<170) return false;
                Hit.bAnchored=false; Hit.bBrittle=false;
                const auto Links=Hit.Links; for(int32 L:Links)if(auto* Other=MutableBody(L))Other->Links.Remove(Hit.Id); Hit.Links.Reset();
                if(Hit.Role==TEXT("brittle_brace")) State.bCollarReleased=true;
                const bool Punch=Has(Rig,TEXT("punch_through_collar"))&&!Shot.bPunched;
                if(Punch) { Shot.bPunched=true; Shot.PenetratedBody=Hit.Id; }
                AddEvent(EExpeditionEventKind::Mechanism,TEXT("Physical brace broken by the slug"),Hit.Position,{Hit.Id,Shot.Id});
                return Punch;
            };
            const bool PunchA=Break(B,A),PunchB=Break(A,B);
            if(PunchA||PunchB){InvA=A.bAnchored?0.f:1.f/A.Mass;InvB=B.bAnchored?0.f:1.f/B.Mass;}
            const float Impulse=(PunchA||PunchB?.35f:1.25f)*Closing/(InvA+InvB);
            A.Velocity-=N*Impulse*InvA; B.Velocity+=N*Impulse*InvB;
            Impact(A,-N,Closing,Rig,!PunchA); Impact(B,N,Closing,Rig,!PunchB);
            if(A.bHot && B.Material==EExpeditionMaterial::Alloy) Damage(B,5);
            if(B.bHot && A.Material==EExpeditionMaterial::Alloy) Damage(A,5);
        }
        const float Penetration=Gap-Distance+.1f;
        A.Position-=N*Penetration*InvA/(InvA+InvB); B.Position+=N*Penetration*InvB/(InvA+InvB);
    }
    if(State.PullRemaining<=0) { for(int32 Id:State.PullIds)if(auto* B=MutableBody(Id);B&&B->State==EExpeditionBodyState::Pulling)B->State=EExpeditionBodyState::Available; State.PullIds.Reset(); }
    if(State.RelayBody!=INDEX_NONE)
    {
        const auto* B=FindBody(State.RelayBody);
        if(!B||State.RelayRemaining<=0||Near(B->Position,State.RelayDestination,20))
        { for(int32 Id:RelayIds)if(auto* G=MutableBody(Id))G->Velocity*=.2f; State.RelayBody=INDEX_NONE;State.RelayIds.Reset(); }
    }
    if(State.SecondRelayBody!=INDEX_NONE)
    {const auto* B=FindBody(State.SecondRelayBody);if(!B||State.RelayRemaining<=0||Near(B->Position,State.SecondRelayDestination,20)){for(int32 Id:State.SecondRelayIds)if(auto* G=MutableBody(Id))G->Velocity*=.2f;State.SecondRelayBody=INDEX_NONE;State.SecondRelayIds.Reset();}}
    if(State.RelayBody==INDEX_NONE&&State.SecondRelayBody==INDEX_NONE)State.RelayRemaining=0;
    const auto Cyclones=MoveTemp(PendingCyclones);PendingCyclones.Reset();for(int32 Id:Cyclones)MakeCyclone(Id);
    UpdateMechanisms();
    if(IsUnsafe()) { State.UnsafeElapsed+=Delta; if(State.UnsafeElapsed>=3.f)Quench(TEXT("Unsafe haul tripped the coil")); } else State.UnsafeElapsed=0;
    if(State.HazardCooldown<=0 && !IsPressSafe())
    {
        bool Struck=false;
        for(auto& B:State.Bodies)
        {
            if(!Held(B))continue;
            const FVector2D D=B.Position-SiteDefinition.PressCenter;
            if(FMath::Abs(D.X)>SiteDefinition.PressHalfSize.X+B.Radius||FMath::Abs(D.Y)>SiteDefinition.PressHalfSize.Y+B.Radius)continue;
            int32 Interceptor=INDEX_NONE;
            for(int32 Id:State.CycloneIds)if(const auto* Fragment=FindBody(Id);Fragment&&Live(*Fragment)&&Near(Fragment->Position,B.Position,65)&&FMath::Abs(Fragment->Position.X-SiteDefinition.PressCenter.X)<SiteDefinition.PressHalfSize.X+Fragment->Radius){Interceptor=Id;break;}
            if(Interceptor!=INDEX_NONE)
            {
                auto* Fragment=MutableBody(Interceptor);Damage(*Fragment,25);Fragment->Velocity=FVector2D(-250,120);State.CycloneIds.Remove(Interceptor);State.HazardCooldown=.35f;
                AddEvent(EExpeditionEventKind::Impact,TEXT("A real orbit fragment intercepted the press — fragment released"),Fragment->Position,{Interceptor});break;
            }
            Damage(B,10); Struck=true;
        }
        if(Struck)Quench(TEXT("Press struck the carried haul"));
    }
}

void FExpeditionWorld::Tick(float Delta,const FVector2D& Magnet,const FExpeditionRig& Rig)
{
    if(!FMath::IsFinite(Delta)||Delta<=0||!Finite(Magnet)||IsEnded())return;
    State.Magnet=FVector2D(FMath::Clamp(Magnet.X,-720.0,720.0),FMath::Clamp(Magnet.Y,-380.0,380.0));
    float Remaining=FMath::Min(Delta,.25f);
    while(Remaining>0) { const float Step=FMath::Min(Remaining,1.f/90.f); Integrate(Step,Rig); Remaining-=Step; }
}

bool FExpeditionWorld::IsModuleImplemented(FName Id)
{
    static const TArray<FName> Implemented={TEXT("extraction_coil"),TEXT("rail_impeller"),TEXT("arc_driver"),TEXT("anchor_winch"),TEXT("vector_emitter"),TEXT("relay_projector"),TEXT("insulated_jaw"),TEXT("crack_follower"),TEXT("slug_press"),TEXT("impact_fuse"),TEXT("ground_clip"),TEXT("conductive_tether"),TEXT("counterweight_hook"),TEXT("twin_anchor"),TEXT("rebound_plate"),TEXT("cold_seam"),TEXT("ratchet_pawl"),TEXT("induction_bridge"),TEXT("eddy_brake"),TEXT("shear_gate"),TEXT("heat_sink_mould"),TEXT("escapement_relay"),TEXT("punch_through_collar"),TEXT("flow_splitter"),TEXT("intact_recovery"),TEXT("salvage_cyclone"),TEXT("closed_circuit"),TEXT("walking_gantry"),TEXT("reaction_frame"),TEXT("field_loom")};
    return Implemented.Contains(Id);
}
TArray<FName> FExpeditionWorld::GetOpportunityTags() const
{
    TArray<FName> Tags={TEXT("Sever"),TEXT("Launch"),TEXT("Arc"),TEXT("PhysicalTether"),TEXT("Vector"),TEXT("FieldTransfer"),TEXT("IronPair"),TEXT("AdjacentSeams"),TEXT("ReboundSurface"),TEXT("BrittleEligible"),TEXT("FixedAnchor"),TEXT("MovingBody"),TEXT("ShearSeam"),TEXT("ConductiveGap"),TEXT("HotIronPair"),TEXT("TimedTerminal"),TEXT("BrittleBrace"),TEXT("SupportedLoad"),TEXT("LiveSource"),TEXT("FunctionalAssembly"),TEXT("ClearGuide")};
    if(State.LayoutId==TEXT("e1"))Tags.Add(TEXT("ClosedReturn"));
    else
    {
        if(FindFrameBody()&&!State.bFrameReceived)Tags.Add(TEXT("ClosedReturn"));
        if(!State.Bodies.ContainsByPredicate([&](const FExpeditionBody& B){return Live(B)&&FindSupport(B.Id);}))Tags.Remove(TEXT("SupportedLoad"));
        if(!State.Bodies.ContainsByPredicate([](const FExpeditionBody& B){return Live(B)&&B.bBrittle;})){Tags.Remove(TEXT("BrittleBrace"));Tags.Remove(TEXT("BrittleEligible"));}
        if(!State.Bodies.ContainsByPredicate([](const FExpeditionBody& B){return Live(B)&&!Held(B)&&B.Velocity.Size()>100;}))Tags.Remove(TEXT("MovingBody"));
    }
    return Tags;
}

namespace
{
void PutVec(FJsonObject& J,const TCHAR* Key,const FVector2D& V)
{ TArray<TSharedPtr<FJsonValue>> A; A.Add(MakeShared<FJsonValueNumber>(V.X)); A.Add(MakeShared<FJsonValueNumber>(V.Y)); J.SetArrayField(Key,A); }
void PutIds(FJsonObject& J,const TCHAR* Key,const TArray<int32>& Ids)
{ TArray<TSharedPtr<FJsonValue>> A; for(int32 Id:Ids)A.Add(MakeShared<FJsonValueNumber>(Id)); J.SetArrayField(Key,A); }
void PutVecs(FJsonObject& J,const TCHAR* Key,const TArray<FVector2D>& Points)
{TArray<TSharedPtr<FJsonValue>> A;for(const auto& P:Points){auto O=MakeShared<FJsonObject>();PutVec(*O,TEXT("p"),P);A.Add(MakeShared<FJsonValueObject>(O));}J.SetArrayField(Key,A);}
bool GetNumber(const FJsonObject& J,const TCHAR* Key,double& N)
{ const auto V=J.TryGetField(Key); return V.IsValid()&&V->Type==EJson::Number&&V->TryGetNumber(N)&&FMath::IsFinite(N); }
bool GetInt(const FJsonObject& J,const TCHAR* Key,int32& N)
{ double D=0; if(!GetNumber(J,Key,D)||D<MIN_int32||D>MAX_int32||FMath::FloorToDouble(D)!=D)return false; N=int32(D); return true; }
bool GetFloat(const FJsonObject& J,const TCHAR* Key,float& N)
{ double D=0; if(!GetNumber(J,Key,D)||FMath::Abs(D)>1.e8)return false; N=float(D); return true; }
bool GetBool(const FJsonObject& J,const TCHAR* Key,bool& B)
{ const auto V=J.TryGetField(Key); return V.IsValid()&&V->Type==EJson::Boolean&&V->TryGetBool(B); }
bool GetVec(const FJsonObject& J,const TCHAR* Key,FVector2D& V)
{
    const TArray<TSharedPtr<FJsonValue>>* A=nullptr;
    if(!J.TryGetArrayField(Key,A)||!A||A->Num()!=2)return false;
    double X=0,Y=0;
    if(!(*A)[0].IsValid()||!(*A)[1].IsValid()||(*A)[0]->Type!=EJson::Number||(*A)[1]->Type!=EJson::Number||!(*A)[0]->TryGetNumber(X)||!(*A)[1]->TryGetNumber(Y)||!FMath::IsFinite(X)||!FMath::IsFinite(Y))return false;
    V=FVector2D(X,Y); return true;
}
bool GetIds(const FJsonObject& J,const TCHAR* Key,TArray<int32>& Ids,int32 Max=256)
{
    const TArray<TSharedPtr<FJsonValue>>* A=nullptr;
    if(!J.TryGetArrayField(Key,A)||!A||A->Num()>Max)return false;
    Ids.Reset();
    for(const auto& V:*A) { double D=0; if(!V.IsValid()||V->Type!=EJson::Number||!V->TryGetNumber(D)||!FMath::IsFinite(D)||D<0||D>1000000||FMath::FloorToDouble(D)!=D||Ids.Contains(int32(D)))return false; Ids.Add(int32(D)); }
    return true;
}
bool GetVecs(const FJsonObject& J,const TCHAR* Key,TArray<FVector2D>& Points)
{const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!J.TryGetArrayField(Key,A)||!A||A->Num()>8)return false;Points.Reset();for(const auto& V:*A){if(!V.IsValid()||V->Type!=EJson::Object)return false;FVector2D P;if(!GetVec(*V->AsObject(),TEXT("p"),P))return false;Points.Add(P);}return true;}
}

TSharedPtr<FJsonObject> FExpeditionWorld::ToJson() const
{
    auto J=MakeShared<FJsonObject>();
#define EW_INT(Name) J->SetNumberField(TEXT(#Name),State.Name)
#define EW_BOOL(Name) J->SetBoolField(TEXT(#Name),State.Name)
#define EW_VEC(Name) PutVec(*J,TEXT(#Name),State.Name)
    EW_INT(Version); EW_INT(SiteIndex); EW_INT(Seed); EW_INT(Battery); EW_INT(Output); EW_INT(ActionSerial); EW_INT(NextBodyId);
    J->SetStringField(TEXT("LayoutId"),State.LayoutId.ToString());EW_INT(LayoutRevision);
    EW_INT(WorldTime); EW_INT(UnsafeElapsed); EW_INT(HazardCooldown); EW_INT(PullRemaining); EW_INT(RelayRemaining);
    EW_INT(TetherBody); EW_INT(SecondTetherBody); EW_INT(LatchedBody); EW_INT(RelayBody); EW_INT(DeferredTerminal); EW_INT(DeferredCharge);
    EW_INT(DeferredSensor);EW_INT(GroundBody);EW_INT(ShearRemaining);EW_INT(SecondRelayBody);EW_INT(GuideUses);EW_INT(GuideBody);EW_INT(GuideSegment);EW_INT(GuideSpeed);
    EW_BOOL(bCollarReleased); EW_BOOL(bBallastCleared); EW_BOOL(bArmBraced); EW_BOOL(bCircuitClosed); EW_BOOL(bCounterweightUsed); EW_BOOL(bDispatched); EW_BOOL(bEvacuated); EW_BOOL(bDeferredWasSafe);
    EW_BOOL(bCoreSecured);PutIds(*J,TEXT("PoweredTerminals"),State.PoweredTerminals);
    EW_BOOL(bFrameHoistPowered);EW_BOOL(bFrameHoistActive);EW_BOOL(bFrameReceived);
    EW_VEC(Magnet); EW_VEC(TetherAnchor); EW_VEC(SecondTetherAnchor); EW_VEC(LatchPosition); EW_VEC(RelayDestination); PutIds(*J,TEXT("PullIds"),State.PullIds);
    EW_VEC(ShearOrigin);EW_VEC(ShearNormal);EW_VEC(SecondRelayDestination);EW_VEC(CycloneCenter);
    PutIds(*J,TEXT("ShearIds"),State.ShearIds);PutIds(*J,TEXT("RelayIds"),State.RelayIds);PutIds(*J,TEXT("SecondRelayIds"),State.SecondRelayIds);PutIds(*J,TEXT("CycloneIds"),State.CycloneIds);PutVecs(*J,TEXT("GuidePoints"),State.GuidePoints);
#undef EW_INT
#undef EW_BOOL
#undef EW_VEC
    TArray<TSharedPtr<FJsonValue>> Bodies;
    for(const auto& B:State.Bodies)
    {
        auto Item=MakeShared<FJsonObject>();
#define EB_NUM(Name) Item->SetNumberField(TEXT(#Name),B.Name)
#define EB_BOOL(Name) Item->SetBoolField(TEXT(#Name),B.Name)
        EB_NUM(Id); Item->SetStringField(TEXT("Role"),B.Role.ToString()); Item->SetNumberField(TEXT("Material"),int32(B.Material)); Item->SetNumberField(TEXT("State"),int32(B.State));
        PutVec(*Item,TEXT("Position"),B.Position); PutVec(*Item,TEXT("Velocity"),B.Velocity); PutVec(*Item,TEXT("CargoOffset"),B.CargoOffset);PutVec(*Item,TEXT("FlowDirection"),B.FlowDirection);
        EB_NUM(Radius); EB_NUM(Mass); EB_NUM(Value);EB_NUM(Appraisal); EB_NUM(Quality); EB_NUM(Charge); EB_NUM(FuseDelay); EB_NUM(RecoverDelay); EB_NUM(RootAction);EB_NUM(PenetratedBody);
        EB_BOOL(bAnchored); EB_BOOL(bConductive); EB_BOOL(bHot); EB_BOOL(bBrittle); EB_BOOL(bGoal); EB_BOOL(bLaunched); EB_BOOL(bRebounded); EB_BOOL(bPunched); EB_BOOL(bSinkUsed);
        EB_BOOL(bFunctional);EB_BOOL(bFloorContact);
        PutIds(*Item,TEXT("Links"),B.Links); PutIds(*Item,TEXT("SourceIds"),B.SourceIds);
#undef EB_NUM
#undef EB_BOOL
        Bodies.Add(MakeShared<FJsonValueObject>(Item));
    }
    J->SetArrayField(TEXT("Bodies"),Bodies);
    TArray<TSharedPtr<FJsonValue>> Constraints;for(const auto& K:State.Constraints){auto O=MakeShared<FJsonObject>();O->SetNumberField(TEXT("Kind"),int32(K.Kind));O->SetNumberField(TEXT("BodyA"),K.BodyA);O->SetNumberField(TEXT("BodyB"),K.BodyB);PutVec(*O,TEXT("Anchor"),K.Anchor);PutVec(*O,TEXT("Offset"),K.Offset);O->SetBoolField(TEXT("Active"),K.bActive);Constraints.Add(MakeShared<FJsonValueObject>(O));}J->SetArrayField(TEXT("Constraints"),Constraints);return J;
}

bool FExpeditionWorld::FromJson(const TSharedPtr<FJsonObject>& J,FString& Error)
{
    Error=TEXT("Invalid expedition world JSON");
    if(!J.IsValid())return false;
    FExpeditionWorld Candidate;
    auto& S=Candidate.State;
#define READ_INT(Name) if(!GetInt(*J,TEXT(#Name),S.Name))return false
#define READ_FLOAT(Name) if(!GetFloat(*J,TEXT(#Name),S.Name))return false
#define READ_BOOL(Name) if(!GetBool(*J,TEXT(#Name),S.Name))return false
#define READ_VEC(Name) if(!GetVec(*J,TEXT(#Name),S.Name))return false
    READ_INT(Version); READ_INT(SiteIndex); READ_INT(Seed); READ_INT(Battery); READ_INT(Output); READ_INT(ActionSerial); READ_INT(NextBodyId);
    if(S.Version==2)
    {
        // Version 2 shipped every site with this exact definition. Never substitute a later default.
        S.LayoutId=TEXT("e1");S.LayoutRevision=1;S.Version=3;
    }
    else if(S.Version==3)
    {
        FString Layout;
        if(!J->TryGetStringField(TEXT("LayoutId"),Layout)||Layout.IsEmpty()||Layout.Len()>64||!GetInt(*J,TEXT("LayoutRevision"),S.LayoutRevision))return false;
        S.LayoutId=FName(Layout);
    }
    else return false;
    if(!IsKnownLayout(S.LayoutId,S.LayoutRevision,S.SiteIndex)){Error=TEXT("Unknown expedition layout or revision");return false;}
    Candidate.BuildObstacles();
    READ_FLOAT(WorldTime); READ_FLOAT(UnsafeElapsed); READ_FLOAT(HazardCooldown); READ_FLOAT(PullRemaining); READ_FLOAT(RelayRemaining);
    READ_INT(TetherBody); READ_INT(SecondTetherBody); READ_INT(LatchedBody); READ_INT(RelayBody); READ_INT(DeferredTerminal); READ_INT(DeferredCharge);
    READ_INT(DeferredSensor);READ_INT(GroundBody);READ_FLOAT(ShearRemaining);READ_INT(SecondRelayBody);READ_INT(GuideUses);READ_INT(GuideBody);READ_INT(GuideSegment);READ_FLOAT(GuideSpeed);
    READ_BOOL(bCollarReleased); READ_BOOL(bBallastCleared); READ_BOOL(bArmBraced); READ_BOOL(bCircuitClosed); READ_BOOL(bCounterweightUsed); READ_BOOL(bDispatched); READ_BOOL(bEvacuated); READ_BOOL(bDeferredWasSafe);
    if(J->HasField(TEXT("bCoreSecured"))&&!GetBool(*J,TEXT("bCoreSecured"),S.bCoreSecured))return false;
    if(J->HasField(TEXT("PoweredTerminals"))&&!GetIds(*J,TEXT("PoweredTerminals"),S.PoweredTerminals))return false;
    if(S.LayoutId!=TEXT("e1")&&(!J->HasField(TEXT("bCoreSecured"))||!J->HasField(TEXT("PoweredTerminals"))))return false;
    for(const TCHAR* Key:{TEXT("bFrameHoistPowered"),TEXT("bFrameHoistActive"),TEXT("bFrameReceived")})if(S.LayoutRevision==2&&!J->HasField(Key))return false;
    if(J->HasField(TEXT("bFrameHoistPowered"))&&!GetBool(*J,TEXT("bFrameHoistPowered"),S.bFrameHoistPowered))return false;
    if(J->HasField(TEXT("bFrameHoistActive"))&&!GetBool(*J,TEXT("bFrameHoistActive"),S.bFrameHoistActive))return false;
    if(J->HasField(TEXT("bFrameReceived"))&&!GetBool(*J,TEXT("bFrameReceived"),S.bFrameReceived))return false;
    READ_VEC(Magnet); READ_VEC(TetherAnchor); READ_VEC(SecondTetherAnchor); READ_VEC(LatchPosition); READ_VEC(RelayDestination);
    READ_VEC(ShearOrigin);READ_VEC(ShearNormal);READ_VEC(SecondRelayDestination);READ_VEC(CycloneCenter);
    if(!GetIds(*J,TEXT("PullIds"),S.PullIds))return false;
    if(!GetIds(*J,TEXT("ShearIds"),S.ShearIds)||!GetIds(*J,TEXT("RelayIds"),S.RelayIds)||!GetIds(*J,TEXT("SecondRelayIds"),S.SecondRelayIds)||!GetIds(*J,TEXT("CycloneIds"),S.CycloneIds)||!GetVecs(*J,TEXT("GuidePoints"),S.GuidePoints))return false;
#undef READ_INT
#undef READ_FLOAT
#undef READ_BOOL
#undef READ_VEC
    const TArray<TSharedPtr<FJsonValue>>* Bodies=nullptr;
    if(!J->TryGetArrayField(TEXT("Bodies"),Bodies)||!Bodies||Bodies->IsEmpty()||Bodies->Num()>256)return false;
    S.Bodies.Reset();
    for(const auto& Value:*Bodies)
    {
        if(!Value.IsValid()||Value->Type!=EJson::Object)return false;
        const auto Item=Value->AsObject(); if(!Item.IsValid())return false;
        FExpeditionBody B; FString Role; int32 Material=0,BodyState=0;
        if(!GetInt(*Item,TEXT("Id"),B.Id)||!Item->TryGetStringField(TEXT("Role"),Role)||Role.Len()>80||!GetInt(*Item,TEXT("Material"),Material)||Material<0||Material>5||!GetInt(*Item,TEXT("State"),BodyState)||BodyState<0||BodyState>5)return false;
        B.Role=FName(Role); B.Material=EExpeditionMaterial(Material); B.State=EExpeditionBodyState(BodyState);
#define READ_B_INT(Name) if(!GetInt(*Item,TEXT(#Name),B.Name))return false
#define READ_B_FLOAT(Name) if(!GetFloat(*Item,TEXT(#Name),B.Name))return false
#define READ_B_BOOL(Name) if(!GetBool(*Item,TEXT(#Name),B.Name))return false
        READ_B_FLOAT(Radius); READ_B_FLOAT(Mass); READ_B_INT(Value);READ_B_INT(Appraisal); READ_B_INT(Quality); READ_B_INT(Charge); READ_B_FLOAT(FuseDelay); READ_B_FLOAT(RecoverDelay); READ_B_INT(RootAction);
        if(Item->HasField(TEXT("PenetratedBody"))&&!GetInt(*Item,TEXT("PenetratedBody"),B.PenetratedBody))return false;
        READ_B_BOOL(bAnchored); READ_B_BOOL(bConductive); READ_B_BOOL(bHot); READ_B_BOOL(bBrittle); READ_B_BOOL(bGoal); READ_B_BOOL(bLaunched); READ_B_BOOL(bRebounded); READ_B_BOOL(bPunched); READ_B_BOOL(bSinkUsed);
        READ_B_BOOL(bFunctional);
        if(S.LayoutRevision==2&&!Item->HasField(TEXT("bFloorContact")))return false;
        if(Item->HasField(TEXT("bFloorContact"))&&!GetBool(*Item,TEXT("bFloorContact"),B.bFloorContact))return false;
#undef READ_B_INT
#undef READ_B_FLOAT
#undef READ_B_BOOL
        if(!GetVec(*Item,TEXT("Position"),B.Position)||!GetVec(*Item,TEXT("Velocity"),B.Velocity)||!GetVec(*Item,TEXT("CargoOffset"),B.CargoOffset)||!GetVec(*Item,TEXT("FlowDirection"),B.FlowDirection)||!GetIds(*Item,TEXT("Links"),B.Links)||!GetIds(*Item,TEXT("SourceIds"),B.SourceIds))return false;
        S.Bodies.Add(MoveTemp(B));
    }
    const TArray<TSharedPtr<FJsonValue>>* Constraints=nullptr;if(!J->TryGetArrayField(TEXT("Constraints"),Constraints)||!Constraints||Constraints->Num()>64)return false;S.Constraints.Reset();
    for(const auto& V:*Constraints){if(!V.IsValid()||V->Type!=EJson::Object)return false;const auto O=V->AsObject();FExpeditionConstraint K;int32 Kind=0;if(!GetInt(*O,TEXT("Kind"),Kind)||Kind<0||Kind>2||!GetInt(*O,TEXT("BodyA"),K.BodyA)||!GetInt(*O,TEXT("BodyB"),K.BodyB)||!GetVec(*O,TEXT("Anchor"),K.Anchor)||!GetVec(*O,TEXT("Offset"),K.Offset)||!GetBool(*O,TEXT("Active"),K.bActive))return false;K.Kind=EExpeditionConstraintKind(Kind);S.Constraints.Add(K);}
    if(!Candidate.CheckInvariants(Error))return false;
    State=MoveTemp(Candidate.State);SiteDefinition=MoveTemp(Candidate.SiteDefinition);Obstacles=MoveTemp(Candidate.Obstacles); Events.Reset();PendingCyclones.Reset(); Error.Reset(); return true;
}

bool FExpeditionWorld::CheckInvariants(FString& Error) const
{
    auto Fail=[&](const TCHAR* Why){Error=Why;return false;};
    if(State.Version!=3||!IsKnownLayout(State.LayoutId,State.LayoutRevision,State.SiteIndex)||State.Battery<0||State.Battery>120||State.Output<0||State.ActionSerial<0||State.NextBodyId<0||State.NextBodyId>1000000)return Fail(TEXT("Invalid or incompatible world ledger"));
    if(!Finite(State.Magnet)||FMath::Abs(State.Magnet.X)>720.01||FMath::Abs(State.Magnet.Y)>380.01||State.WorldTime<0||!FMath::IsFinite(State.WorldTime)||State.UnsafeElapsed<0||State.UnsafeElapsed>3.26||State.HazardCooldown<0||State.HazardCooldown>2||State.PullRemaining<0||State.PullRemaining>1.21||State.RelayRemaining<0||State.RelayRemaining>4.01)return Fail(TEXT("Invalid physical clock or magnet"));
    if(State.bDispatched&&State.bEvacuated)return Fail(TEXT("Contradictory site settlement"));
    if(State.DeferredCharge<0||State.DeferredCharge>1)return Fail(TEXT("Invalid reserved charge"));
    if(State.Bodies.IsEmpty()||State.Bodies.Num()>256)return Fail(TEXT("Invalid body count"));
    FExpeditionWorld Original; Original.StartSite(State.SiteIndex,State.Seed,100,State.LayoutId,State.LayoutRevision);
    TSet<int32> Ids,Sources; int32 Goals=0,Banked=0,DispatchCount=0;
    for(const auto& B:State.Bodies)
    {
        if(B.Id<0||B.Id>=State.NextBodyId||Ids.Contains(B.Id)||!Finite(B.Position)||!Finite(B.Velocity)||!Finite(B.CargoOffset)||B.Velocity.Size()>3000||B.Radius<5||B.Radius>100||B.Mass<=0||B.Mass>200||B.Value<0||B.Value>100000||B.Quality<0||B.Quality>100||B.Charge<0||B.Charge>2||B.RootAction<0||B.RootAction>State.ActionSerial||B.RecoverDelay<0||B.RecoverDelay>5||B.FuseDelay < -2.01||B.FuseDelay>1)return Fail(TEXT("Invalid body values"));
        Ids.Add(B.Id);
        if(!Held(B)&&Live(B)&&(FMath::Abs(B.Position.X)>550||FMath::Abs(B.Position.Y)>350))return Fail(TEXT("Available body escaped worksite"));
        if(B.bGoal) { ++Goals; if(B.Id!=0||B.Value!=0||B.Material!=EExpeditionMaterial::Core||B.State==EExpeditionBodyState::Consumed||B.State==EExpeditionBodyState::Banked)return Fail(TEXT("Unique objective was altered or melted")); }
        if(B.State==EExpeditionBodyState::Dispatched){++DispatchCount;if(!B.bGoal)return Fail(TEXT("Ordinary scrap dispatched as goal"));}
        if(B.Appraisal<0||B.Appraisal>B.Value||!Finite(B.FlowDirection))return Fail(TEXT("Invalid appraisal or conductor direction"));
        if(B.PenetratedBody!=INDEX_NONE&&(!B.bPunched||B.PenetratedBody==B.Id||!FindBody(B.PenetratedBody)))return Fail(TEXT("Invalid paid penetration target"));
        const auto* Authored=Original.FindBody(B.Id);
        if(B.bFloorContact!=(Authored&&Authored->bFloorContact))return Fail(TEXT("Changed physical floor-contact geometry"));
        if(State.LayoutRevision==2&&B.Id>=61&&B.Id<=64&&(!Authored||B.Role!=Authored->Role||!FMath::IsNearlyEqual(B.Radius,Authored->Radius,.01f)))return Fail(TEXT("Changed authored frame component"));
        if(B.State==EExpeditionBodyState::Banked)Banked+=B.Appraisal;
        float ExpectedMass=0; int32 ExpectedValue=0;
        if(B.SourceIds.IsEmpty())return Fail(TEXT("Body lost source lineage"));
        for(int32 Source:B.SourceIds)
        {
            const auto* O=Original.FindBody(Source); if(!O)return Fail(TEXT("Unknown material source"));
            ExpectedMass+=O->Mass; ExpectedValue+=O->Value;
            if(B.State!=EExpeditionBodyState::Consumed) { if(Sources.Contains(Source))return Fail(TEXT("Material source duplicated")); Sources.Add(Source); }
        }
        if(!FMath::IsNearlyEqual(B.Mass,ExpectedMass,.01f)||B.Value!=ExpectedValue)return Fail(TEXT("Welding changed source mass or raw value"));
        if(B.State==EExpeditionBodyState::Pulling&&!State.PullIds.Contains(B.Id))return Fail(TEXT("Unpaid pending attraction"));
        for(int32 Link:B.Links) { const auto* L=FindBody(Link); if(Link==B.Id||!L||!L->Links.Contains(B.Id)||!Live(B)||!Live(*L))return Fail(TEXT("Asymmetric or inactive link")); }
    }
    if(Goals!=1||Banked!=State.Output||Sources.Num()!=Original.State.Bodies.Num()||DispatchCount!=(State.bDispatched?1:0))return Fail(TEXT("World settlement or source conservation mismatch"));
    if(State.bCoreSecured&&FindBody(0)->bAnchored)return Fail(TEXT("A secured core was re-anchored"));
    if(State.LayoutId!=TEXT("e1")&&(FindBody(0)->State==EExpeditionBodyState::Cargo||State.bDispatched)&&!State.bCoreSecured)return Fail(TEXT("Captured objective lost its secured latch"));
    for(int32 Id:State.PoweredTerminals)if(const auto* B=FindBody(Id);!B||!IsTerminalBody(*B))return Fail(TEXT("Unknown powered terminal"));
    const auto* Frame=FindFrameBody();
    if(!Frame&&(State.bFrameHoistPowered||State.bFrameHoistActive||State.bFrameReceived))return Fail(TEXT("Frame state on a worksite without a frame"));
    if(Frame)
    {
        if(Frame->Id!=61||State.LayoutRevision!=2||Frame->State==EExpeditionBodyState::Consumed||Held(*Frame))return Fail(TEXT("Heavy frame bypassed physical transport"));
        if(State.bFrameReceived!=(Frame->State==EExpeditionBodyState::Banked))return Fail(TEXT("Frame receiving ledger disagrees with its real body"));
        if(State.bFrameHoistPowered!=State.PoweredTerminals.Contains(63))return Fail(TEXT("Frame hoist lost its paid contact state"));
        if(State.bFrameHoistActive&&(!State.bFrameHoistPowered||State.bFrameReceived||!Live(*Frame)||Frame->bAnchored))return Fail(TEXT("Invalid active receiving hoist"));
        if(State.bFrameReceived&&(Frame->bAnchored||!FindMarker(TEXT("frame_receiver"))||!Near(Frame->Position,FindMarker(TEXT("frame_receiver"))->Position,FindMarker(TEXT("frame_receiver"))->Radius)))return Fail(TEXT("Installed frame is outside its physical receiver"));
        if(State.bFrameReceived&&State.LayoutId==TEXT("counterweight_exchange")&&!State.bCircuitClosed)return Fail(TEXT("Installed frame lost final counterbalance"));
    }
    for(int32 Id:State.PullIds){const auto* B=FindBody(Id);if(!B||B->State!=EExpeditionBodyState::Pulling)return Fail(TEXT("Invalid pending pull target"));}
    for(int32 Id:{State.TetherBody,State.SecondTetherBody,State.LatchedBody,State.RelayBody,State.SecondRelayBody,State.GuideBody,State.DeferredSensor,State.DeferredTerminal,State.GroundBody})if(Id!=INDEX_NONE&&!FindBody(Id))return Fail(TEXT("Unknown physical constraint body"));
    if(State.GuideUses<0||State.GuideUses>1||State.GuideSegment<0||State.GuideSegment>3||State.GuideSpeed<0||State.GuideSpeed>1000||State.ShearRemaining<0||State.ShearRemaining>2.01||State.CycloneIds.Num()>6)return Fail(TEXT("Invalid bounded secondary operation"));
    if((State.GuideUses>0||State.GuideBody!=INDEX_NONE)&&State.GuidePoints.Num()!=3)return Fail(TEXT("Missing paid guide geometry"));
    for(const auto& Point:State.GuidePoints)if(!Finite(Point)||!Near(Point,InTray(Point,25),4))return Fail(TEXT("Guide outside worksite"));
    for(const auto& Set:{State.RelayIds,State.SecondRelayIds,State.ShearIds,State.CycloneIds})for(int32 Id:Set)if(const auto* B=FindBody(Id);!B||!Live(*B))return Fail(TEXT("Secondary effect targets unavailable material"));
    if(State.DeferredCharge>0&&(State.DeferredSensor==INDEX_NONE||State.DeferredTerminal==INDEX_NONE))return Fail(TEXT("Reserved relay charge lacks its physical endpoints"));
    for(const auto& K:State.Constraints)if(!FindBody(K.BodyA)||!FindBody(K.BodyB)||K.BodyA==K.BodyB||!Finite(K.Anchor)||!Finite(K.Offset))return Fail(TEXT("Invalid support constraint"));
    if(State.bDispatched&&!IsCoreReleased())return Fail(TEXT("Objective dispatched before restraints released"));
    Error.Reset();return true;
}
}
