#include "SalvageModel.h"

namespace MagnetSweep
{
namespace
{
bool IsFinitePoint(const FVector2D& Point)
{
    return FMath::IsFinite(Point.X) && FMath::IsFinite(Point.Y);
}

void AddPiece(TArray<FSalvagePiece>& Pieces, int32 Id, EPieceKind Kind, double X,
    double Y, int32 Amount)
{
    FSalvagePiece Piece;
    Piece.Id = Id;
    Piece.Kind = Kind;
    Piece.Position = FVector2D(X, Y);
    Piece.Amount = Amount;
    Pieces.Add(MoveTemp(Piece));
}

void Link(TArray<FSalvagePiece>& Pieces, int32 A, int32 B)
{
    for (FSalvagePiece& Piece : Pieces)
    {
        if (Piece.Id == A) Piece.DirectLinks.Add(B);
        if (Piece.Id == B) Piece.DirectLinks.Add(A);
    }
}
}

FSalvageModel::FSalvageModel()
{
    BuildLayout(0);
}

void FSalvageModel::IncrementEpoch()
{
    ++State.Epoch;
    if (State.Epoch == 0) State.Epoch = 1;
}

bool FSalvageModel::IsInsideTray(const FVector2D& Position)
{
    return IsFinitePoint(Position) && Position.X >= TrayMinX && Position.X <= TrayMaxX
        && Position.Y >= TrayMinY && Position.Y <= TrayMaxY;
}

FVector2D FSalvageModel::ClampToTray(const FVector2D& Position)
{
    if (!IsFinitePoint(Position)) return FVector2D::ZeroVector;
    return FVector2D(FMath::Clamp(Position.X, double(TrayMinX), double(TrayMaxX)),
        FMath::Clamp(Position.Y, double(TrayMinY), double(TrayMaxY)));
}

bool FSalvageModel::IsInsideCapsule(const FVector2D& Point, const FVector2D& Start,
    const FVector2D& End, float Radius)
{
    if (!IsFinitePoint(Point) || !IsFinitePoint(Start) || !IsFinitePoint(End)
        || !FMath::IsFinite(Radius) || Radius < 0.0f) return false;
    const FVector2D Segment = End - Start;
    const double LengthSquared = Segment.SizeSquared();
    const double T = LengthSquared > UE_DOUBLE_SMALL_NUMBER
        ? FMath::Clamp(FVector2D::DotProduct(Point - Start, Segment) / LengthSquared, 0.0, 1.0)
        : 0.0;
    return (Point - (Start + Segment * T)).SizeSquared() <= double(Radius) * Radius + 0.0001;
}

const FSalvagePiece* FSalvageModel::FindPiece(int32 Id) const
{
    return State.Pieces.FindByPredicate([Id](const FSalvagePiece& Piece) { return Piece.Id == Id; });
}

int32 FSalvageModel::FindAvailableRing(const FVector2D& Position) const
{
    if (!IsInsideTray(Position)) return INDEX_NONE;
    int32 Id = INDEX_NONE;
    double Nearest = double(RingHitRadius) * RingHitRadius;
    for (const FSalvagePiece& Piece : State.Pieces)
    {
        if (Piece.Kind != EPieceKind::Tangle || Piece.State != EPieceState::Available) continue;
        const double Distance = (Piece.Position - Position).SizeSquared();
        if (Distance <= Nearest && (Distance < Nearest || Id == INDEX_NONE || Piece.Id < Id))
        {
            Id = Piece.Id;
            Nearest = Distance;
        }
    }
    return Id;
}

int32 FSalvageModel::GetAvailableAmount() const
{
    int32 Result = 0;
    for (const FSalvagePiece& Piece : State.Pieces)
        if (Piece.State == EPieceState::Available) Result += Piece.Amount;
    return Result;
}

int32 FSalvageModel::GetTotalAmount() const
{
    return GetAvailableAmount() + State.Cargo + State.Banked;
}

FString FSalvageModel::GetLayoutName() const
{
    return State.LayoutIndex == 0 ? TEXT("Linked Fan") : TEXT("Offset Fork");
}

FRecoveryResult FSalvageModel::Recover(const TArray<int32>& PieceIds)
{
    FRecoveryResult Result;
    // Private callers provide a complete selection; identity and Available state remain authoritative.
    for (FSalvagePiece& Piece : State.Pieces)
    {
        if (Piece.State == EPieceState::Available && PieceIds.Contains(Piece.Id))
        {
            Piece.State = EPieceState::Cargo;
            Result.PieceIds.Add(Piece.Id);
            Result.Amount += Piece.Amount;
        }
    }
    if (Result.Amount > 0)
    {
        Result.PieceIds.Sort();
        State.Cargo += Result.Amount;
        IncrementEpoch();
    }
    return Result;
}

FRecoveryResult FSalvageModel::SweepAt(const FVector2D& Position)
{
    if (!IsInsideTray(Position)) return FRecoveryResult();
    TArray<int32> Ids;
    for (const FSalvagePiece& Piece : State.Pieces)
    {
        if (Piece.Kind == EPieceKind::Loose && Piece.State == EPieceState::Available
            && (Piece.Position - Position).SizeSquared() <= double(SweepRadius) * SweepRadius)
            Ids.Add(Piece.Id);
    }
    return Recover(Ids);
}

FPullSelection FSalvageModel::PreviewPull(int32 RingId, const FVector2D& DesiredEndpoint) const
{
    FPullSelection Result;
    const FSalvagePiece* Ring = FindPiece(RingId);
    if (!Ring || Ring->Kind != EPieceKind::Tangle || Ring->State != EPieceState::Available
        || !IsFinitePoint(DesiredEndpoint)) return Result;
    Result.RingId = RingId;
    Result.Epoch = State.Epoch;
    const FVector2D Clamped = ClampToTray(DesiredEndpoint);
    const FVector2D Delta = Clamped - Ring->Position;
    const double Length = Delta.Size();
    Result.Endpoint = Ring->Position + Delta * (Length > GetReach() ? GetReach() / Length : 1.0);
    for (const FSalvagePiece& Piece : State.Pieces)
    {
        if (Piece.State != EPieceState::Available) continue;
        const bool bEligible = Piece.Id == RingId || Piece.Kind == EPieceKind::Loose
            || (HasBreakaway() && Ring->DirectLinks.Contains(Piece.Id));
        if (bEligible && (Piece.Id == RingId
            || IsInsideCapsule(Piece.Position, Ring->Position, Result.Endpoint, PullRadius)))
        {
            Result.PieceIds.Add(Piece.Id);
            Result.Amount += Piece.Amount;
        }
    }
    Result.PieceIds.Sort();
    return Result;
}

FRecoveryResult FSalvageModel::CommitPull(const FPullSelection& Preview)
{
    if (!Preview.IsValid() || Preview.Epoch != State.Epoch) return FRecoveryResult();
    const FPullSelection Current = PreviewPull(Preview.RingId, Preview.Endpoint);
    if (Current.PieceIds != Preview.PieceIds || Current.Amount != Preview.Amount
        || !Current.Endpoint.Equals(Preview.Endpoint, 0.001)) return FRecoveryResult();
    return Recover(Current.PieceIds);
}

FBankResult FSalvageModel::BankCargo()
{
    FBankResult Result;
    Result.PreviousUpgradeLevel = State.UpgradeLevel;
    Result.NewUpgradeLevel = State.UpgradeLevel;
    if (State.Cargo == 0) return Result;
    Result.Amount = State.Cargo;
    for (FSalvagePiece& Piece : State.Pieces)
    {
        if (Piece.State == EPieceState::Cargo)
        {
            Piece.State = EPieceState::Banked;
            Result.PieceIds.Add(Piece.Id);
        }
    }
    State.Banked += State.Cargo;
    State.Cargo = 0;
    if (!State.bDeliveryCompleted && State.Banked >= State.Goal)
    {
        State.bDeliveryCompleted = true;
        Result.bCompletedNow = true;
        State.CompletedDeliveryCount = FMath::Min(State.CompletedDeliveryCount + 1, 1000000);
        State.UpgradeLevel = FMath::Min(State.CompletedDeliveryCount, 2);
        Result.NewUpgradeLevel = State.UpgradeLevel;
    }
    IncrementEpoch();
    return Result;
}

void FSalvageModel::ResetLayout()
{
    BuildLayout(State.LayoutIndex);
}

bool FSalvageModel::AdvanceLayout()
{
    if (!State.bDeliveryCompleted || State.Cargo != 0) return false;
    BuildLayout((State.LayoutIndex + 1) % LayoutCount);
    return true;
}

void FSalvageModel::BuildLayout(int32 LayoutIndex)
{
    State.LayoutIndex = LayoutIndex;
    State.Pieces.Reset();
    State.Cargo = 0;
    State.Banked = 0;
    State.Goal = LayoutIndex == 0 ? 100 : 140;
    State.bDeliveryCompleted = false;
    IncrementEpoch();

    if (LayoutIndex == 0)
    {
        // The southern crescent and isolated tangle offer a generous unlinked opening.
        const FVector2D Opening[] = {
            {-430,-210},{-388,-242},{-340,-256},{-290,-247},{-245,-220},
            {-417,-165},{-373,-184},{-326,-198},{-279,-185},{-232,-155}
        };
        int32 Id = 0;
        for (const FVector2D& P : Opening) AddPiece(State.Pieces, Id++, EPieceKind::Loose, P.X, P.Y, 8);
        AddPiece(State.Pieces, 100, EPieceKind::Tangle, -355, -85, 40);
        // Horizontal fan: starting at the hub can reach all three directly linked leaves.
        AddPiece(State.Pieces, 101, EPieceKind::Tangle, -210, 90, 44);
        AddPiece(State.Pieces, 102, EPieceKind::Tangle, 30, 25, 36);
        AddPiece(State.Pieces, 103, EPieceKind::Tangle, 60, 95, 36);
        AddPiece(State.Pieces, 104, EPieceKind::Tangle, 20, 170, 36);
        Link(State.Pieces, 101, 102);
        Link(State.Pieces, 101, 103);
        Link(State.Pieces, 101, 104);
        const FVector2D Seam[] = {
            {-155,55},{-102,90},{-54,120},{112,55},{168,112},{225,82},
            {290,65},{343,105},{408,80},{445,136},{312,220},{380,-145},
            {430,-215},{210,-225},{125,-165},{-30,-170}
        };
        Id = 10;
        for (const FVector2D& P : Seam) AddPiece(State.Pieces, Id++, EPieceKind::Loose, P.X, P.Y, 8);
    }
    else
    {
        // Fork: hub directions disagree; the lower leaf offers a rich diagonal loose seam.
        AddPiece(State.Pieces, 100, EPieceKind::Tangle, -80, 30, 44);
        AddPiece(State.Pieces, 101, EPieceKind::Tangle, -280, 205, 36);
        AddPiece(State.Pieces, 102, EPieceKind::Tangle, 155, 210, 36);
        AddPiece(State.Pieces, 103, EPieceKind::Tangle, -260, -170, 40);
        AddPiece(State.Pieces, 104, EPieceKind::Tangle, 330, -180, 36);
        Link(State.Pieces, 100, 101);
        Link(State.Pieces, 100, 102);
        Link(State.Pieces, 100, 103);
        Link(State.Pieces, 102, 104);
        const FVector2D Loose[] = {
            {-405,-210},{-360,-190},{-205,-145},{-165,-110},{-110,-72},
            {-55,-38},{0,0},{50,35},{105,70},{165,105},{220,140},
            {280,180},{345,215},{405,245},{-370,190},{-320,250},
            {-165,230},{-120,180},{-10,230},{270,-220},{380,-120},
            {425,-195},{180,-120},{40,-205},{-70,-230},{-410,10}
        };
        int32 Id = 0;
        for (const FVector2D& P : Loose) AddPiece(State.Pieces, Id++, EPieceKind::Loose, P.X, P.Y, 8);
    }
}

bool FSalvageModel::ValidateSnapshot(const FSalvageSnapshot& Snapshot, FString& OutError)
{
    const auto Fail = [&OutError](const TCHAR* Message) { OutError = Message; return false; };
    if (Snapshot.Version != SnapshotVersion) return Fail(TEXT("Unsupported save version."));
    if (Snapshot.LayoutIndex < 0 || Snapshot.LayoutIndex >= LayoutCount || Snapshot.Epoch == 0)
        return Fail(TEXT("Invalid delivery identity."));
    if (Snapshot.CompletedDeliveryCount < 0 || Snapshot.CompletedDeliveryCount > 1000000
        || Snapshot.UpgradeLevel != FMath::Min(Snapshot.CompletedDeliveryCount, 2))
        return Fail(TEXT("Invalid retained improvement state."));
    if (Snapshot.Pieces.Num() == 0 || Snapshot.Pieces.Num() > 256)
        return Fail(TEXT("Invalid material population."));
    TSet<int32> Ids;
    int32 Cargo = 0;
    int32 Banked = 0;
    int32 Total = 0;
    for (const FSalvagePiece& Piece : Snapshot.Pieces)
    {
        if (Piece.Id < 0 || Ids.Contains(Piece.Id)) return Fail(TEXT("Duplicate or invalid material identity."));
        Ids.Add(Piece.Id);
        if (!IsInsideTray(Piece.Position) || Piece.Amount < 1 || Piece.Amount > 1000)
            return Fail(TEXT("Invalid material geometry or amount."));
        if (Piece.Kind != EPieceKind::Loose && Piece.Kind != EPieceKind::Tangle)
            return Fail(TEXT("Invalid material kind."));
        if (Piece.State != EPieceState::Available && Piece.State != EPieceState::Cargo
            && Piece.State != EPieceState::Banked) return Fail(TEXT("Invalid material ownership."));
        if (Piece.Kind == EPieceKind::Loose && !Piece.DirectLinks.IsEmpty())
            return Fail(TEXT("Loose material cannot own tangle links."));
        if (Piece.State == EPieceState::Cargo) Cargo += Piece.Amount;
        if (Piece.State == EPieceState::Banked) Banked += Piece.Amount;
        Total += Piece.Amount;
        TSet<int32> Links;
        for (int32 LinkId : Piece.DirectLinks)
        {
            if (LinkId == Piece.Id || Links.Contains(LinkId)) return Fail(TEXT("Duplicate or self link."));
            Links.Add(LinkId);
            const FSalvagePiece* Neighbor = Snapshot.Pieces.FindByPredicate(
                [LinkId](const FSalvagePiece& Other) { return Other.Id == LinkId; });
            if (!Neighbor || Neighbor->Kind != EPieceKind::Tangle || !Neighbor->DirectLinks.Contains(Piece.Id))
                return Fail(TEXT("Missing or asymmetric tangle link."));
        }
    }
    if (Snapshot.Goal <= 0 || Snapshot.Goal >= Total) return Fail(TEXT("Goal must leave optional material."));
    if (Snapshot.Cargo != Cargo || Snapshot.Banked != Banked)
        return Fail(TEXT("Material ownership does not match the saved ledger."));
    if (Snapshot.bDeliveryCompleted != (Banked >= Snapshot.Goal)
        || (Snapshot.bDeliveryCompleted && Snapshot.CompletedDeliveryCount == 0))
        return Fail(TEXT("Inconsistent delivery completion."));
    OutError.Reset();
    return true;
}

bool FSalvageModel::CheckInvariants(FString& OutError) const
{
    return ValidateSnapshot(State, OutError);
}

bool FSalvageModel::RestoreSnapshot(const FSalvageSnapshot& Snapshot, FString& OutError)
{
    if (!ValidateSnapshot(Snapshot, OutError)) return false;
    const uint32 PreviousEpoch = State.Epoch;
    State = Snapshot;
    // A loaded copy must invalidate any live gesture, even if loading a snapshot of itself.
    State.Epoch = FMath::Max(State.Epoch, PreviousEpoch);
    IncrementEpoch();
    return true;
}
}
