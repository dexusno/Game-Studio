#include "SalvageModel.h"
#include "Math/RandomStream.h"

namespace MagnetSweep
{
namespace
{
constexpr int32 CareerLimit = 100000000;
const FJobDefinition Jobs[] = {
    {TEXT("First Pour"), 120, 180, 80, 40, 1, 24, 8, 2, 2},
    {TEXT("Copper Knot"), 180, 260, 120, 60, 2, 24, 12, 6, 3},
    {TEXT("Live Wire"), 240, 360, 160, 80, 3, 24, 16, 10, 4},
    {TEXT("Heavy Freight"), 300, 440, 200, 100, 4, 28, 18, 14, 5},
    {TEXT("Split Seam"), 380, 540, 240, 120, 4, 28, 20, 18, 6},
    {TEXT("Furnace Crown"), 460, 660, 280, 140, 4, 30, 22, 22, 6}
};
bool FinitePoint(const FVector2D& P) { return FMath::IsFinite(P.X) && FMath::IsFinite(P.Y); }
int32 MaterialMass(EMaterial M) { return M == EMaterial::Iron ? 2 : M == EMaterial::Copper ? 3 : 4; }
int32 MaterialValue(EMaterial M)
{
    switch (M) { case EMaterial::Iron: return 4; case EMaterial::Copper: return 12;
    case EMaterial::Alloy: return 24; case EMaterial::Core: return 40; default: return 0; }
}
void LinkPieces(TArray<FSalvagePiece>& Pieces, int32 A, int32 B)
{
    Pieces[A].DirectLinks.AddUnique(B); Pieces[B].DirectLinks.AddUnique(A);
    Pieces[A].Kind = EPieceKind::Tangle; Pieces[B].Kind = EPieceKind::Tangle;
}
}

FSalvageModel::FSalvageModel() { BuildLayout(0, State.Seed); }
const FJobDefinition& FSalvageModel::JobDefinition(int32 Index) { return Jobs[FMath::Clamp(Index, 0, LayoutCount - 1)]; }
void FSalvageModel::IncrementEpoch() { if (++State.Epoch == 0) State.Epoch = 1; }
int32 FSalvageModel::LevelForXP(int32 XP) { return XP >= 700 ? 4 : XP >= 350 ? 3 : XP >= 120 ? 2 : 1; }
int32 FSalvageModel::XPForLevel(int32 Level) { return Level >= 4 ? 700 : Level == 3 ? 350 : Level == 2 ? 120 : 0; }
int32 FSalvageModel::GetPlayerLevel() const { return LevelForXP(State.XP); }
int32 FSalvageModel::GetUpgradeTier(EUpgrade Upgrade) const
{
    const int32 Index = static_cast<int32>(Upgrade);
    return State.UpgradeTiers.IsValidIndex(Index) ? State.UpgradeTiers[Index] : 0;
}
int32 FSalvageModel::GetUpgradePrice(EUpgrade Upgrade) const
{
    const int32 Tier = GetUpgradeTier(Upgrade);
    return Tier == 0 ? 150 : Tier == 1 ? 300 : Tier == 2 ? 500 : 0;
}
bool FSalvageModel::CanPurchaseUpgrade(EUpgrade Upgrade, FString& Reason) const
{
    const int32 Index = static_cast<int32>(Upgrade);
    if (!State.UpgradeTiers.IsValidIndex(Index)) { Reason = TEXT("Unknown mod."); return false; }
    const int32 Tier = GetUpgradeTier(Upgrade);
    if (Tier >= 3) { Reason = TEXT("Fully upgraded."); return false; }
    if (GetPlayerLevel() < Tier + 2) { Reason = FString::Printf(TEXT("Requires operator level %d."), Tier + 2); return false; }
    if (State.CargoMass > 0) { Reason = TEXT("Smelt or leave the current haul before fitting a mod."); return false; }
    if (State.Wallet < GetUpgradePrice(Upgrade)) { Reason = TEXT("Earn more credits from salvage contracts."); return false; }
    Reason.Reset(); return true;
}
bool FSalvageModel::PurchaseUpgrade(EUpgrade Upgrade)
{
    FString Reason; if (!CanPurchaseUpgrade(Upgrade, Reason)) return false;
    State.Wallet -= GetUpgradePrice(Upgrade);
    ++State.UpgradeTiers[static_cast<int32>(Upgrade)]; ++State.UpgradeLevel;
    IncrementEpoch(); return true;
}
FString FSalvageModel::MaterialName(EMaterial Material)
{
    switch (Material) { case EMaterial::Iron: return TEXT("Iron scrap"); case EMaterial::Copper: return TEXT("Copper salvage");
    case EMaterial::Alloy: return TEXT("Alloy salvage"); case EMaterial::Core: return TEXT("Rare core"); default: return TEXT("Hot cell"); }
}
FString FSalvageModel::CoreName(int32 Id)
{
    static const TCHAR* Names[] = {TEXT("Amber Dynamo"), TEXT("Copper Heart"), TEXT("Blue Capacitor"),
        TEXT("Freight Gyroscope"), TEXT("Twin Spark"), TEXT("Furnace Crown")};
    return Id >= 0 && Id < LayoutCount ? Names[Id] : TEXT("Unknown core");
}
FString FSalvageModel::PieceName(int32 Id) const
{
    const FSalvagePiece* Piece = FindPiece(Id);
    return Piece ? Piece->Material == EMaterial::Core ? CoreName(Piece->CoreId) : MaterialName(Piece->Material) : TEXT("No salvage");
}
bool FSalvageModel::IsInsideTray(const FVector2D& P)
{
    return FinitePoint(P) && P.X >= TrayMinX && P.X <= TrayMaxX && P.Y >= TrayMinY && P.Y <= TrayMaxY;
}
FVector2D FSalvageModel::ClampToTray(const FVector2D& P)
{
    return FinitePoint(P) ? FVector2D(FMath::Clamp(P.X, double(TrayMinX), double(TrayMaxX)),
        FMath::Clamp(P.Y, double(TrayMinY), double(TrayMaxY))) : FVector2D::ZeroVector;
}
bool FSalvageModel::IsInsideCapsule(const FVector2D& P, const FVector2D& A, const FVector2D& B, float Radius)
{
    if (!FinitePoint(P) || !FinitePoint(A) || !FinitePoint(B) || !FMath::IsFinite(Radius) || Radius < 0) return false;
    const FVector2D D = B-A; const double Length = D.SizeSquared();
    const double T = Length > UE_DOUBLE_SMALL_NUMBER ? FMath::Clamp(FVector2D::DotProduct(P-A,D) / Length, 0., 1.) : 0.;
    return (P-(A+D*T)).SizeSquared() <= double(Radius)*Radius + .0001;
}
const FSalvagePiece* FSalvageModel::FindPiece(int32 Id) const
{
    return State.Pieces.FindByPredicate([Id](const FSalvagePiece& Piece) { return Piece.Id == Id; });
}
int32 FSalvageModel::GetAvailableAmount() const
{
    int32 Total = 0; for (const auto& P : State.Pieces) if (P.State == EPieceState::Available) Total += P.Amount; return Total;
}
int32 FSalvageModel::GetTotalAmount() const
{
    int32 Total = 0; for (const auto& P : State.Pieces) Total += P.Amount; return Total;
}
bool FSalvageModel::HasHotCell() const
{
    return State.Pieces.ContainsByPredicate([](const FSalvagePiece& P) { return P.State == EPieceState::Cargo && P.Material == EMaterial::HotCell; });
}
const FSalvagePiece* FSalvageModel::GetAtRiskPiece() const
{
    const FSalvagePiece* Best = nullptr;
    for (const FSalvagePiece& P : State.Pieces)
        if (P.State == EPieceState::Cargo && P.Material != EMaterial::HotCell
            && (!Best || P.Amount > Best->Amount || (P.Amount == Best->Amount && P.Id < Best->Id))) Best = &P;
    return Best;
}
bool FSalvageModel::IsJobFailed() const
{
    return !State.bDeliveryCompleted && (State.bJobEnded || State.HeatsUsed >= HeatsPerJob
        || State.Banked + State.Cargo + GetAvailableAmount() < State.Goal);
}
void FSalvageModel::UpdateFailure() { if (IsJobFailed()) State.bJobEnded = true; }
bool FSalvageModel::IsJobUnlocked(int32 Index) const
{
    return Index >= 0 && Index < LayoutCount && GetPlayerLevel() >= JobDefinition(Index).RequiredLevel;
}
bool FSalvageModel::IsJobCleared(int32 Index) const { return State.ClearedJobs.IsValidIndex(Index) && State.ClearedJobs[Index]; }
bool FSalvageModel::IsJobGold(int32 Index) const { return State.GoldJobs.IsValidIndex(Index) && State.GoldJobs[Index]; }
TArray<int32> FSalvageModel::GetCaptureGroup(int32 Id) const
{
    TArray<int32> Group;
    const FSalvagePiece* Start = FindPiece(Id);
    if (!Start || Start->State != EPieceState::Available) return Group;
    Group.Add(Id);
    for (int32 Cursor = 0; Cursor < Group.Num(); ++Cursor)
    {
        const FSalvagePiece* Piece = FindPiece(Group[Cursor]);
        for (int32 Link : Piece->DirectLinks)
        {
            const FSalvagePiece* Other = FindPiece(Link);
            if (Other && Other->State == EPieceState::Available) Group.AddUnique(Link);
        }
    }
    Group.Sort(); return Group;
}
bool FSalvageModel::CanCaptureGroup(int32 Id) const
{
    if (State.bJobEnded || IsJobFailed() || State.HeatsUsed >= HeatsPerJob) return false;
    const TArray<int32> Group = GetCaptureGroup(Id);
    if (Group.IsEmpty()) return false;
    int32 Mass = State.CargoMass;
    for (int32 PieceId : Group) Mass += FindPiece(PieceId)->Mass;
    return Mass <= GetMaxCaptureMass();
}
FRecoveryResult FSalvageModel::CapturePieces(const TArray<int32>& Ids)
{
    FRecoveryResult Result;
    if (State.bJobEnded || IsJobFailed() || State.HeatsUsed >= HeatsPerJob) return Result;
    TArray<int32> Expanded;
    for (int32 Id : Ids) for (int32 Linked : GetCaptureGroup(Id)) Expanded.AddUnique(Linked);
    Expanded.Sort();
    int32 Mass = 0; for (int32 Id : Expanded) Mass += FindPiece(Id)->Mass;
    if (Mass + State.CargoMass > GetMaxCaptureMass()) { Result.bCapacityRefused = true; return Result; }
    for (FSalvagePiece& P : State.Pieces)
        if (Expanded.Contains(P.Id))
        {
            P.State = EPieceState::Cargo; P.CaptureOrder = ++State.CaptureSerial;
            Result.PieceIds.Add(P.Id); Result.Amount += P.Amount; Result.Mass += P.Mass;
        }
    if (Result.Succeeded()) { RecountCargo(); IncrementEpoch(); }
    return Result;
}
bool FSalvageModel::MoveAvailablePiece(int32 Id, const FVector2D& Position)
{
    if (!IsInsideTray(Position)) return false;
    for (FSalvagePiece& P : State.Pieces)
        if (P.Id == Id && P.State == EPieceState::Available) { P.Position = Position; return true; }
    return false;
}
void FSalvageModel::RecountCargo()
{
    State.Cargo = 0; State.CargoMass = 0;
    for (const FSalvagePiece& P : State.Pieces)
        if (P.State == EPieceState::Cargo) { State.Cargo += P.Amount; State.CargoMass += P.Mass; }
    if (!IsCargoUnsafe()) State.FuseElapsed = 0;
}
FSpillResult FSalvageModel::VentCargo(const FVector2D& Origin)
{
    FSpillResult Result;
    if (State.bJobEnded || State.CargoMass == 0 || !FinitePoint(Origin)) return Result;
    PlaceDroppedCargo(Origin, Result.ReleasedPieceIds);
    RecountCargo(); IncrementEpoch();
    return Result;
}
void FSalvageModel::PlaceDroppedCargo(const FVector2D& Origin, TArray<int32>& OutIds)
{
    struct FDropGroup { TArray<int32> Ids; };
    struct FPlaced { FVector2D Position; bool Hot; };
    TArray<FDropGroup> Groups;
    TSet<int32> Dropped;
    for (const auto& P : State.Pieces)
    {
        if (P.State != EPieceState::Cargo || Dropped.Contains(P.Id)) continue;
        FDropGroup Group; Group.Ids.Add(P.Id); Dropped.Add(P.Id);
        for (int32 I = 0; I < Group.Ids.Num(); ++I)
            for (int32 Linked : FindPiece(Group.Ids[I])->DirectLinks)
            {
                const auto* Other = FindPiece(Linked);
                if (Other && Other->State == EPieceState::Cargo && !Dropped.Contains(Linked))
                { Group.Ids.Add(Linked); Dropped.Add(Linked); }
            }
        Group.Ids.Sort(); Groups.Add(MoveTemp(Group));
    }
    Groups.Sort([](const FDropGroup& A, const FDropGroup& B) {
        return A.Ids.Num() == B.Ids.Num() ? A.Ids[0] < B.Ids[0] : A.Ids.Num() > B.Ids.Num();
    });
    TArray<FPlaced> Placed;
    const FVector2D Center = ClampToTray(Origin);
    for (const FDropGroup& Group : Groups)
    {
        FVector2D Mean = FVector2D::ZeroVector;
        for (int32 Id : Group.Ids) Mean += FindPiece(Id)->Position;
        Mean /= Group.Ids.Num();
        TArray<FVector2D> Offsets;
        double Extent = 0;
        for (int32 Id : Group.Ids) { const auto Offset = FindPiece(Id)->Position-Mean; Offsets.Add(Offset); Extent = FMath::Max(Extent, Offset.Size()); }
        // Keep normal authored bundle geometry. Compact a previously scattered bundle
        // uniformly so its links remain readable and it can be placed near a tray edge.
        if (Extent > 90) for (auto& Offset : Offsets) Offset *= 90. / Extent;
        FVector2D Low = FVector2D::ZeroVector, High = FVector2D::ZeroVector;
        for (const auto& Offset : Offsets)
        { Low.X=FMath::Min(Low.X,Offset.X); Low.Y=FMath::Min(Low.Y,Offset.Y); High.X=FMath::Max(High.X,Offset.X); High.Y=FMath::Max(High.Y,Offset.Y); }
        FVector2D Best = Center;
        double BestPenalty = TNumericLimits<double>::Max();
        // A fixed fine grid is deterministic and bounded. Prefer nearby empty space;
        // never clamp pieces independently, which would collapse a bundle at a corner.
        for (double Y=TrayMinY+24; Y<=TrayMaxY-24; Y+=28)
            for (double X=TrayMinX+24; X<=TrayMaxX-24; X+=28)
            {
                const FVector2D Candidate(FMath::Clamp(X,TrayMinX+24.-Low.X,TrayMaxX-24.-High.X),
                    FMath::Clamp(Y,TrayMinY+24.-Low.Y,TrayMaxY-24.-High.Y));
                bool Blocked = false; double Penalty = (Candidate-Center).SizeSquared();
                for (int32 I=0; I<Group.Ids.Num() && !Blocked; ++I)
                {
                    const bool Hot = FindPiece(Group.Ids[I])->Material == EMaterial::HotCell;
                    const auto Point = Candidate+Offsets[I];
                    for (const auto& Other : Placed)
                    {
                        const double Gap = Hot || Other.Hot ? 118. : 60.;
                        if ((Point-Other.Position).SizeSquared() < Gap*Gap) { Blocked=true; break; }
                    }
                    // Existing unmoved scrap is a soft obstacle in dense trays; the
                    // released pieces themselves always keep the hard separation above.
                    for (const auto& Other : State.Pieces)
                        if (Other.State == EPieceState::Available && !Dropped.Contains(Other.Id))
                        {
                            const double Gap = Hot || Other.Material == EMaterial::HotCell ? 118. : 60.;
                            const double Distance = (Point-Other.Position).Size();
                            if (Distance < Gap) Penalty += 1000000. + FMath::Square(Gap-Distance)*100.;
                        }
                }
                if (!Blocked && Penalty < BestPenalty) { BestPenalty=Penalty; Best=Candidate; }
            }
        for (int32 I=0; I<Group.Ids.Num(); ++I)
            for (auto& P : State.Pieces) if (P.Id == Group.Ids[I])
            {
                P.Position=ClampToTray(Best+Offsets[I]); P.State=EPieceState::Available; P.CaptureOrder=0;
                OutIds.Add(P.Id); Placed.Add({P.Position,P.Material==EMaterial::HotCell}); break;
            }
    }
    OutIds.Sort();
}
FSpillResult FSalvageModel::TriggerOverload(const FVector2D& Origin)
{
    FSpillResult Result;
    if (State.bJobEnded || State.HeatsUsed >= HeatsPerJob || !IsCargoUnsafe() || !FinitePoint(Origin)) return Result;
    const FSalvagePiece* AtRisk = GetAtRiskPiece();
    const int32 LostId = AtRisk ? AtRisk->Id : INDEX_NONE;
    Result.bTripped = true;
    for (FSalvagePiece& P : State.Pieces)
        if (P.State == EPieceState::Cargo)
        {
            if (P.Id == LostId || P.Material == EMaterial::HotCell)
            {
                P.State = EPieceState::Lost; Result.DestroyedPieceIds.Add(P.Id); Result.LostValue += P.Amount;
            }
        }
    PlaceDroppedCargo(Origin, Result.ReleasedPieceIds);
    // An emergency quench spends the same finite fuel used by a smelt. Even a lone
    // zero-value hot cell costs a charge, so deliberately tripping is not free disposal.
    State.HeatsUsed = FMath::Min(HeatsPerJob, State.HeatsUsed + 1);
    if (State.HeatsUsed == HeatsPerJob) State.bJobEnded = true;
    RecountCargo(); State.FuseElapsed = 0; UpdateFailure(); IncrementEpoch(); return Result;
}
FSpillResult FSalvageModel::AdvanceRisk(float Delta, const FVector2D& Origin)
{
    if (!FMath::IsFinite(Delta) || Delta <= 0 || !FinitePoint(Origin) || State.bJobEnded) return {};
    if (!IsCargoUnsafe()) { State.FuseElapsed = 0; return {}; }
    State.FuseElapsed = FMath::Min(GetFuseDuration(), State.FuseElapsed + Delta);
    if (State.FuseElapsed >= GetFuseDuration()) return TriggerOverload(Origin);
    return {};
}
bool FSalvageModel::CanSmelt(FString& Reason) const
{
    if (State.bJobEnded || IsJobFailed()) { Reason = TEXT("Contract ended. Choose a new attempt in the workshop."); return false; }
    if (State.HeatsUsed >= HeatsPerJob) { Reason = TEXT("No furnace heats left."); return false; }
    if (State.CargoMass == 0) { Reason = TEXT("Collect a useful haul first."); return false; }
    if (HasHotCell()) { Reason = TEXT("Hot cell in cargo. Right-click to drop the haul before smelting."); return false; }
    if (State.CargoMass > GetCapacity()) { Reason = TEXT("Overloaded. Right-click to drop the haul before smelting."); return false; }
    Reason.Reset(); return true;
}
FBankResult FSalvageModel::BankCargo()
{
    FBankResult Result;
    Result.PreviousPlayerLevel = Result.NewPlayerLevel = GetPlayerLevel();
    Result.PreviousUpgradeLevel = Result.NewUpgradeLevel = GetUpgradeLevel();
    FString Reason; if (!CanSmelt(Reason)) return Result;
    Result.Amount = State.Cargo; Result.Mass = State.CargoMass;
    for (FSalvagePiece& P : State.Pieces)
        if (P.State == EPieceState::Cargo)
        {
            P.State = EPieceState::Banked; Result.PieceIds.Add(P.Id);
            if (P.Material == EMaterial::Core && !State.CollectedCores.Contains(P.CoreId))
            { State.CollectedCores.Add(P.CoreId); Result.NewCoreIds.Add(P.CoreId); }
        }
    State.CollectedCores.Sort(); State.Banked += Result.Amount; ++State.HeatsUsed; RecountCargo();
    int32 Award = Result.Amount;
    if (!State.bDeliveryCompleted && State.Banked >= State.Goal)
    {
        State.bDeliveryCompleted = true; Result.bCompletedNow = true;
        ++State.CompletedDeliveryCount; State.ClearedJobs[State.LayoutIndex] = true;
        Award += GetJob().CompletionBonus;
        const int32 Best = State.BestHeats[State.LayoutIndex];
        State.BestHeats[State.LayoutIndex] = Best == 0 ? State.HeatsUsed : FMath::Min(Best, State.HeatsUsed);
    }
    if (!State.bGoldAwarded && State.Banked >= GetGoldGoal())
    {
        State.bGoldAwarded = true; Result.bGoldNow = true;
        State.GoldJobs[State.LayoutIndex] = true; Award += GetJob().GoldBonus;
    }
    State.BestBanked[State.LayoutIndex] = FMath::Max(State.BestBanked[State.LayoutIndex], State.Banked);
    Result.CashAwarded = FMath::Min(Award, CareerLimit - State.Wallet);
    Result.XPAwarded = FMath::Min(Award, CareerLimit - State.XP);
    State.Wallet += Result.CashAwarded; State.XP += Result.XPAwarded;
    Result.NewPlayerLevel = GetPlayerLevel();
    if (State.HeatsUsed >= HeatsPerJob) State.bJobEnded = true;
    UpdateFailure(); IncrementEpoch(); return Result;
}
bool FSalvageModel::StartJob(int32 Index, int32 Seed)
{
    if (!IsJobUnlocked(Index) || Seed <= 0) return false;
    const bool bUntouched = State.HeatsUsed == 0 && State.CaptureSerial == 0;
    if (!State.bJobEnded && !bUntouched) return false;
    BuildLayout(Index, Seed); return true;
}
void FSalvageModel::RetryJob(bool bNewSeed)
{
    const int32 Seed = bNewSeed ? State.Seed == MAX_int32 ? 1 : State.Seed + 1 : State.Seed;
    BuildLayout(State.LayoutIndex, Seed);
}
bool FSalvageModel::FinishJob()
{
    if (!State.bDeliveryCompleted || State.CargoMass > 0) return false;
    State.bJobEnded = true; IncrementEpoch(); return true;
}
void FSalvageModel::AbandonJob()
{
    for (FSalvagePiece& P : State.Pieces) if (P.State == EPieceState::Cargo) P.State = EPieceState::Lost;
    RecountCargo(); State.bJobEnded = true; IncrementEpoch();
}
void FSalvageModel::BuildLayout(int32 Index, int32 Seed)
{
    State.LayoutIndex = FMath::Clamp(Index, 0, LayoutCount - 1); State.Seed = Seed;
    State.Pieces.Reset(); State.Cargo = 0; State.CargoMass = 0; State.Banked = 0;
    State.HeatsUsed = 0; State.CaptureSerial = 0; State.FuseElapsed = 0;
    State.Goal = GetJob().Quota; State.bDeliveryCompleted = false; State.bGoldAwarded = false; State.bJobEnded = false;
    IncrementEpoch();
    FRandomStream Random(Seed);
    const FJobDefinition& Job = GetJob();
    // A safe iron crescent remains in every job. Rich clusters rotate/mirror across a clear grid.
    // Seeds change cluster approach angles and membership; no opaque required item can block quota.
    TArray<FVector2D> Slots;
    for (int32 Row = 0; Row < 8; ++Row)
        for (int32 Col = 0; Col < 11; ++Col) Slots.Add(FVector2D(-445 + Col * 88, -258 + Row * 73));
    for (int32 I = Slots.Num()-1; I > 0; --I) Slots.Swap(I, Random.RandRange(0, I));
    int32 Slot = 0;
    const auto Add = [&](EMaterial Material, int32 Count)
    {
        for (int32 I = 0; I < Count; ++I)
        {
            FSalvagePiece P; P.Id = State.Pieces.Num(); P.Material = Material;
            P.Mass = MaterialMass(Material); P.Amount = MaterialValue(Material);
            P.CoreId = Material == EMaterial::Core ? State.LayoutIndex : INDEX_NONE;
            const FVector2D Jitter(Random.FRandRange(-9,9), Random.FRandRange(-8,8));
            P.Position = ClampToTray(Slots[Slot++] + Jitter);
            State.Pieces.Add(P);
        }
    };
    Add(EMaterial::Iron, Job.IronCount); Add(EMaterial::Copper, Job.CopperCount);
    Add(EMaterial::Alloy, Job.AlloyCount); Add(EMaterial::Core, 1); Add(EMaterial::HotCell, Job.CellCount);
    // Broad iron opening is separated from dangerous clusters. Position it deliberately.
    for (int32 I = 0; I < 12; ++I)
        State.Pieces[I].Position = FVector2D(-428 + (I % 6) * 69, -255 + (I / 6) * 52);
    // Two readable small copper/alloy bundles. Atomic group mass remains <=24kg in all jobs.
    const int32 RichStart = Job.IronCount;
    const int32 BundleCount = 2 + State.LayoutIndex / 2;
    for (int32 B = 0; B < BundleCount; ++B)
    {
        const int32 First = RichStart + B * 3;
        if (First + 2 >= RichStart + Job.CopperCount) break;
        const double A = Random.FRandRange(-.8f,.8f) + (State.LayoutIndex % 2 ? 1.15 : .15);
        const FVector2D Center(-260 + B * 172, 35 + ((B + State.LayoutIndex) % 2) * 135);
        for (int32 K = 0; K < 3; ++K)
            State.Pieces[First + K].Position = ClampToTray(Center + FVector2D(FMath::Cos(A),FMath::Sin(A)) * ((K-1) * 46.));
        LinkPieces(State.Pieces, First, First+1); LinkPieces(State.Pieces, First+1, First+2);
    }
    // Core and cells are visually inspectable adjacent opportunities, not mandatory linked contamination.
    const int32 CoreIndex = Job.IronCount + Job.CopperCount + Job.AlloyCount;
    State.Pieces[CoreIndex].Position = FVector2D(335, -70 + 35 * (State.LayoutIndex % 3));
    for (int32 I = 0; I < Job.CellCount; ++I)
    {
        const double Angle = I * 2.39996 + Random.FRandRange(-.35f,.35f);
        const FVector2D Center = I < 2 ? State.Pieces[CoreIndex].Position : FVector2D(-30 + (I-2)*105,130);
        State.Pieces[CoreIndex+1+I].Position = ClampToTray(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * (80 + I*7));
    }
}

// Integration compatibility: legacy corridor selection is deliberately inactive.
int32 FSalvageModel::FindAvailableRing(const FVector2D& Position) const { return INDEX_NONE; }
FRecoveryResult FSalvageModel::SweepAt(const FVector2D& Position)
{
    if (!IsInsideTray(Position)) return {};
    TArray<int32> Nearby;
    for (const FSalvagePiece& P : State.Pieces)
        if (P.State == EPieceState::Available && (P.Position-Position).SizeSquared() <= SweepRadius*SweepRadius) Nearby.Add(P.Id);
    return CapturePieces(Nearby);
}
FPullSelection FSalvageModel::PreviewPull(int32 RingId, const FVector2D& DesiredEndpoint) const { return {}; }
FRecoveryResult FSalvageModel::CommitPull(const FPullSelection& Preview) { return {}; }
bool FSalvageModel::AdvanceLayout()
{
    if (!State.bDeliveryCompleted || State.CargoMass > 0) return false;
    FinishJob();
    for (int32 Offset = 1; Offset <= LayoutCount; ++Offset)
    {
        const int32 Next = (State.LayoutIndex + Offset) % LayoutCount;
        if (IsJobUnlocked(Next)) return StartJob(Next, State.Seed == MAX_int32 ? 1 : State.Seed + 1);
    }
    return false;
}

bool FSalvageModel::ValidateSnapshot(const FSalvageSnapshot& S, FString& Error)
{
    const auto Fail = [&Error](const TCHAR* Text) { Error = Text; return false; };
    if (S.Version != SnapshotVersion) return Fail(TEXT("Unsupported salvage career save version."));
    if (S.LayoutIndex < 0 || S.LayoutIndex >= LayoutCount || S.Seed <= 0 || S.Epoch == 0) return Fail(TEXT("Invalid contract identity."));
    if (S.Wallet < 0 || S.Wallet > CareerLimit || S.XP < 0 || S.XP > CareerLimit || S.Wallet > S.XP)
        return Fail(TEXT("Invalid career cash or experience ledger."));
    if (S.UpgradeTiers.Num() != 3 || S.BestBanked.Num() != LayoutCount || S.BestHeats.Num() != LayoutCount
        || S.ClearedJobs.Num() != LayoutCount || S.GoldJobs.Num() != LayoutCount) return Fail(TEXT("Invalid career record sizes."));
    int32 TierSum = 0, Spent = 0;
    for (int32 Tier : S.UpgradeTiers)
    {
        if (Tier < 0 || Tier > 3 || Tier > LevelForXP(S.XP)-1) return Fail(TEXT("Unowned or locked rig tier."));
        TierSum += Tier; if (Tier >= 1) Spent += 150; if (Tier >= 2) Spent += 300; if (Tier >= 3) Spent += 500;
    }
    if (S.UpgradeLevel != TierSum || (S.XP < CareerLimit && S.XP - S.Wallet != Spent))
        return Fail(TEXT("Purchased rig does not match the cash ledger."));
    if (LevelForXP(S.XP) < JobDefinition(S.LayoutIndex).RequiredLevel) return Fail(TEXT("Active contract is locked."));
    if (S.CompletedDeliveryCount < 0 || S.CompletedDeliveryCount > 1000000 || S.HeatsUsed < 0 || S.HeatsUsed > HeatsPerJob
        || S.CaptureSerial < 0 || S.CaptureSerial > 100000000 || S.Goal != JobDefinition(S.LayoutIndex).Quota)
        return Fail(TEXT("Invalid attempt counters or goal."));
    int32 Clears = 0;
    for (int32 I = 0; I < LayoutCount; ++I)
    {
        if (S.BestBanked[I] < 0 || S.BestBanked[I] > 100000 || S.BestHeats[I] < 0 || S.BestHeats[I] > HeatsPerJob
            || (S.GoldJobs[I] && !S.ClearedJobs[I]) || (S.ClearedJobs[I] != (S.BestHeats[I] > 0))
            || (S.ClearedJobs[I] && S.BestBanked[I] < JobDefinition(I).Quota)
            || (S.GoldJobs[I] && S.BestBanked[I] < JobDefinition(I).GoldGoal)) return Fail(TEXT("Inconsistent completed contract records."));
        if (S.ClearedJobs[I]) ++Clears;
    }
    if (S.CompletedDeliveryCount < Clears) return Fail(TEXT("Completed contract count is inconsistent."));
    TSet<int32> CoreIds;
    for (int32 Id : S.CollectedCores)
    { if (Id < 0 || Id >= LayoutCount || CoreIds.Contains(Id)) return Fail(TEXT("Invalid rare collection.")); CoreIds.Add(Id); }
    if (S.Pieces.IsEmpty() || S.Pieces.Num() > 256) return Fail(TEXT("Invalid salvage population."));
    TSet<int32> Ids, CaptureOrders;
    int32 Cargo = 0, Mass = 0, Banked = 0, Available = 0, SpentHeatWitnesses = 0; bool bHot = false;
    for (const FSalvagePiece& P : S.Pieces)
    {
        if (P.Id < 0 || Ids.Contains(P.Id)) return Fail(TEXT("Duplicate or invalid salvage identity.")); Ids.Add(P.Id);
        if (!IsInsideTray(P.Position) || static_cast<int32>(P.Material) > static_cast<int32>(EMaterial::HotCell)
            || static_cast<int32>(P.State) > static_cast<int32>(EPieceState::Lost)
            || static_cast<int32>(P.Kind) > static_cast<int32>(EPieceKind::Tangle)
            || P.Mass != MaterialMass(P.Material) || P.Amount != MaterialValue(P.Material)) return Fail(TEXT("Invalid salvage geometry, type, mass or value."));
        if ((P.Material == EMaterial::Core && P.CoreId != S.LayoutIndex)
            || (P.Material != EMaterial::Core && P.CoreId != INDEX_NONE)) return Fail(TEXT("Invalid core identity."));
        if (P.CaptureOrder < 0 || P.CaptureOrder > S.CaptureSerial) return Fail(TEXT("Invalid capture order."));
        if (P.State == EPieceState::Cargo)
        {
            if (P.CaptureOrder == 0 || CaptureOrders.Contains(P.CaptureOrder)) return Fail(TEXT("Duplicate cargo capture order."));
            CaptureOrders.Add(P.CaptureOrder); Cargo += P.Amount; Mass += P.Mass; bHot |= P.Material == EMaterial::HotCell;
        }
        if (P.State == EPieceState::Banked)
        {
            if (P.Material == EMaterial::HotCell || (P.Material == EMaterial::Core && !CoreIds.Contains(P.CoreId)))
                return Fail(TEXT("Unsafe or unrecorded banked item."));
            Banked += P.Amount;
            ++SpentHeatWitnesses;
        }
        // Every quench destroys at least one previously captured piece, including
        // zero-value cells. Every smelt banks at least one piece. A spent charge
        // therefore needs a distinct terminal piece as a ledger witness; discarded
        // untouched fixture pieces cannot fabricate evidence of consumed fuel.
        if (P.State == EPieceState::Lost && P.CaptureOrder > 0) ++SpentHeatWitnesses;
        if (P.State == EPieceState::Available) Available += P.Amount;
        if ((P.DirectLinks.IsEmpty() && P.Kind != EPieceKind::Loose) || (!P.DirectLinks.IsEmpty() && P.Kind != EPieceKind::Tangle))
            return Fail(TEXT("Link presentation does not match group membership."));
        TSet<int32> Links;
        for (int32 Id : P.DirectLinks)
        {
            const FSalvagePiece* Other = S.Pieces.FindByPredicate([Id](const FSalvagePiece& Q) { return Q.Id == Id; });
            if (Id == P.Id || Links.Contains(Id) || !Other || !Other->DirectLinks.Contains(P.Id)) return Fail(TEXT("Dangling or asymmetric salvage link."));
            Links.Add(Id);
        }
    }
    if (S.Cargo != Cargo || S.CargoMass != Mass || S.Banked != Banked || Mass > (24 + 8*S.UpgradeTiers[0])*3/2)
        return Fail(TEXT("Attempt material ledger does not match ownership."));
    const bool bUnsafe = bHot || Mass > 24 + 8*S.UpgradeTiers[0];
    if (!FMath::IsFinite(S.FuseElapsed) || S.FuseElapsed < 0 || S.FuseElapsed >= 3.f + S.UpgradeTiers[2]
        || (!bUnsafe && S.FuseElapsed != 0)) return Fail(TEXT("Invalid saved instability fuse."));
    if (S.bDeliveryCompleted != (Banked >= S.Goal) || S.bGoldAwarded != (Banked >= JobDefinition(S.LayoutIndex).GoldGoal)
        || (S.bDeliveryCompleted && (!S.ClearedJobs[S.LayoutIndex] || S.CompletedDeliveryCount == 0))
        || (S.bGoldAwarded && !S.GoldJobs[S.LayoutIndex])) return Fail(TEXT("Contract bonus state is inconsistent."));
    if ((S.HeatsUsed == 0 && Banked != 0) || S.HeatsUsed > SpentHeatWitnesses
        || (S.HeatsUsed == HeatsPerJob && !S.bJobEnded)
        || (!S.bDeliveryCompleted && Banked + Cargo + Available < S.Goal && !S.bJobEnded))
        return Fail(TEXT("Invalid furnace heat or failed-contract state."));
    const int32 EarnedThisJob = Banked + (S.bDeliveryCompleted ? JobDefinition(S.LayoutIndex).CompletionBonus : 0)
        + (S.bGoldAwarded ? JobDefinition(S.LayoutIndex).GoldBonus : 0);
    if (S.XP < EarnedThisJob || S.BestBanked[S.LayoutIndex] < Banked) return Fail(TEXT("Career ledger is missing a committed smelt."));
    Error.Reset(); return true;
}
bool FSalvageModel::CheckInvariants(FString& Error) const { return ValidateSnapshot(State, Error); }
bool FSalvageModel::RestoreSnapshot(const FSalvageSnapshot& Snapshot, FString& Error)
{
    if (!ValidateSnapshot(Snapshot, Error)) return false;
    const uint32 Prior = State.Epoch; State = Snapshot; State.Epoch = FMath::Max(Prior, State.Epoch); IncrementEpoch(); return true;
}
}
