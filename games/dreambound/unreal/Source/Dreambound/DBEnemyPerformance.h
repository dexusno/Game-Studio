#pragma once

#include "CoreMinimal.h"

// Original performance poses. Angles are anatomical actor-space deltas; the
// consumer maps them through the imported reference hierarchy. Keys preserve
// a velocity through contact rather than easing every joint to a stop there.
namespace DBEnemyPerformance
{
    enum EJoint : uint8 { Hips, Lumbar, Spine, Chest, Neck, Head, ClavicleL, ClavicleR,
        ArmL, ArmR, ForearmL, ForearmR, HandL, HandR, JointCount };
    enum class ERole : uint8 { Melee, Hunter, Caster, Boss };
    struct FPose
    {
        FVector Offset = FVector::ZeroVector;
        FRotator Joint[JointCount] = {};
        FVector HandTarget[2] = { FVector::ZeroVector, FVector::ZeroVector };
        float HandWeight[2] = { 0.f, 0.f };
        float Throat = 0.f;
        FPose() { for (FRotator& Rotation : Joint) Rotation = FRotator::ZeroRotator; }
    };
    struct FKey { float Time; FPose Pose; float Flow = 1.f; };

    inline FPose Difference(const FPose& A, const FPose& B, float Scale)
    {
        FPose Out;
        Out.Offset = (A.Offset - B.Offset) * Scale;
        for (int32 J = 0; J < JointCount; ++J) Out.Joint[J] = (A.Joint[J] - B.Joint[J]) * Scale;
        for (int32 S = 0; S < 2; ++S)
        {
            Out.HandTarget[S] = (A.HandTarget[S] - B.HandTarget[S]) * Scale;
            Out.HandWeight[S] = (A.HandWeight[S] - B.HandWeight[S]) * Scale;
        }
        Out.Throat = (A.Throat - B.Throat) * Scale;
        return Out;
    }
    inline FPose Sample(const FKey* Keys, int32 Count, float Time)
    {
        if (Time <= Keys[0].Time) return Keys[0].Pose;
        if (Time >= Keys[Count - 1].Time) return Keys[Count - 1].Pose;
        int32 I = 0;
        while (I + 1 < Count && Time > Keys[I + 1].Time) ++I;
        const float Duration = Keys[I + 1].Time - Keys[I].Time;
        const float T = (Time - Keys[I].Time) / Duration;
        auto Tangent = [&](int32 K)
        {
            if (K == 0 || K == Count - 1) return FPose();
            return Difference(Keys[K + 1].Pose, Keys[K - 1].Pose,
                Keys[K].Flow / (Keys[K + 1].Time - Keys[K - 1].Time));
        };
        const FPose A = Tangent(I), B = Tangent(I + 1);
        const FPose& P = Keys[I].Pose;
        const FPose& Q = Keys[I + 1].Pose;
        FPose Out;
        Out.Offset = FMath::CubicInterp(P.Offset, A.Offset * Duration, Q.Offset, B.Offset * Duration, T);
        for (int32 J = 0; J < JointCount; ++J)
            Out.Joint[J] = FRotator(
                FMath::CubicInterp(P.Joint[J].Pitch, A.Joint[J].Pitch * Duration, Q.Joint[J].Pitch, B.Joint[J].Pitch * Duration, T),
                FMath::CubicInterp(P.Joint[J].Yaw, A.Joint[J].Yaw * Duration, Q.Joint[J].Yaw, B.Joint[J].Yaw * Duration, T),
                FMath::CubicInterp(P.Joint[J].Roll, A.Joint[J].Roll * Duration, Q.Joint[J].Roll, B.Joint[J].Roll * Duration, T));
        for (int32 S = 0; S < 2; ++S)
        {
            Out.HandTarget[S] = FMath::CubicInterp(P.HandTarget[S], A.HandTarget[S] * Duration,
                Q.HandTarget[S], B.HandTarget[S] * Duration, T);
            Out.HandWeight[S] = FMath::Clamp(FMath::CubicInterp(P.HandWeight[S], A.HandWeight[S] * Duration,
                Q.HandWeight[S], B.HandWeight[S] * Duration, T), 0.f, 1.f);
        }
        Out.Throat = FMath::Clamp(FMath::CubicInterp(P.Throat, A.Throat * Duration, Q.Throat, B.Throat * Duration, T), 0.f, 1.f);
        return Out;
    }
    inline void R(FPose& P, EJoint J, float Pitch, float Yaw = 0.f, float Roll = 0.f)
    { P.Joint[J] = FRotator(Pitch, Yaw, Roll); }
    inline void Arms(FPose& P, FRotator Left, FRotator Right, float ElbowLeft, float ElbowRight,
        float WristLeft = 0.f, float WristRight = 0.f)
    {
        P.Joint[ArmL] = Left; P.Joint[ArmR] = Right;
        R(P, ForearmL, ElbowLeft); R(P, ForearmR, ElbowRight);
        R(P, HandL, WristLeft); R(P, HandR, WristRight);
    }
    inline FPose Ready(ERole Role)
    {
        FPose P;
        const bool Caster = Role == ERole::Caster, Hunter = Role == ERole::Hunter, Boss = Role == ERole::Boss;
        P.Offset = FVector(Hunter ? 5.f : 2.f, Caster ? -2.f : 0.f, Hunter ? -3.f : 0.f);
        R(P, Hips, Hunter ? -12.f : Caster ? -8.f : -5.f, Caster ? -6.f : 0.f);
        R(P, Lumbar, Hunter ? -10.f : -4.f, Caster ? 4.f : 0.f);
        R(P, Spine, Hunter ? -7.f : Caster ? -5.f : -6.f);
        R(P, Chest, Hunter ? -9.f : Caster ? 6.f : -6.f, Caster ? 7.f : 0.f);
        R(P, Neck, Hunter ? 18.f : Caster ? 3.f : 10.f, Caster ? -5.f : 0.f);
        R(P, Head, Hunter ? 15.f : Caster ? 7.f : 10.f);
        R(P, ClavicleL, 0.f, Caster ? -8.f : -3.f, -3.f);
        R(P, ClavicleR, 0.f, Caster ? 5.f : 3.f, 3.f);
        // Compensate the loaded torso with a hanging upper arm and a soft
        // elbow. The old negative shoulder + elbow angles added to the body
        // lean, parking both claws behind the hips even at zero travel blend.
        Arms(P, FRotator(Caster ? 12.f : Hunter ? 40.f : 20.f, Caster ? -10.f : -3.f, Caster ? -13.f : -22.f),
            FRotator(Caster ? 18.f : Hunter ? 45.f : 24.f, Caster ? 12.f : 3.f, Caster ? 13.f : 22.f),
            Caster ? 21.f : 7.f, Caster ? 28.f : 11.f, Caster ? -9.f : 7.f, Caster ? -12.f : 10.f);
        if (Boss) { R(P, Chest, -8.f); R(P, ClavicleL, 2.f, -7.f, -4.f); R(P, ClavicleR, 2.f, 7.f, 4.f); }
        return P;
    }
    inline void Carry(FPose& P, ERole Role)
    {
        const bool Caster = Role == ERole::Caster, Hunter = Role == ERole::Hunter;
        P.HandTarget[0] = FVector(Hunter ? 17.f : Caster ? 14.f : 5.f, -7.f, Caster ? -57.f : -71.f);
        P.HandTarget[1] = FVector(Hunter ? 22.f : Caster ? 20.f : 9.f, 7.f, Caster ? -52.f : -68.f);
        P.HandWeight[0] = P.HandWeight[1] = 1.f;
    }
    inline FPose Travel(ERole Role, float Blend, float Cycle, float SideLoad,
        float Turn, FVector Velocity, FVector2D PlantLoad, float Time)
    {
        FPose Base = Ready(Role);
        Carry(Base, Role);
        const bool Caster = Role == ERole::Caster, Hunter = Role == ERole::Hunter, Boss = Role == ERole::Boss;
        const float Weight = Caster ? .65f : Boss ? 1.15f : 1.f;
        auto StepPose = [&](float Lead, bool Passing)
        {
            FPose P = Base;
            const float Oppose = Lead * Weight;
            P.Joint[Hips] += FRotator(Passing ? 1.f : -2.f, -Oppose * 7.f, Oppose * 2.f);
            P.Joint[Lumbar] += FRotator(Passing ? -1.f : 1.f, Oppose * 4.f, -Oppose);
            P.Joint[Spine] += FRotator(Passing ? 2.f : -2.f, Oppose * 5.f, -Oppose * 1.5f);
            P.Joint[Chest] += FRotator(Passing ? 1.f : -3.f, Oppose * 6.f, -Oppose * 2.f);
            P.Joint[Neck] += FRotator(Passing ? -2.f : 4.f, -Oppose * 5.f, 0.f);
            P.Joint[Head] += FRotator(Passing ? -1.f : 2.f, -Oppose * 3.f, 0.f);
            for (int32 S = 0; S < 2; ++S)
            {
                const float Sign = S == 0 ? -1.f : 1.f;
                const float ArmLead = Lead * -Sign;
                const float Front = Caster ? 28.f : Hunter ? 45.f : Boss ? 30.f : 38.f;
                const float Rear = Caster ? -10.f : Hunter ? -9.f : Boss ? -23.f : -27.f;
                P.HandTarget[S] = Passing ? FVector(Caster ? 8.f : Hunter ? 15.f : 1.f, Sign * 7.f, Caster ? -66.f : -78.f)
                    : FVector(ArmLead > 0.f ? Front : Rear, Sign * (ArmLead > 0.f ? 5.f : 10.f),
                        Caster ? (ArmLead > 0.f ? -48.f : -58.f) : (ArmLead > 0.f ? -58.f : -65.f));
                // The forearm releases on the low passing arc, then folds
                // slightly on the forward reach. Hands keep their whole-claw
                // shape; this rig has no finger curl to counterfeit a fist.
                P.Joint[S == 0 ? ForearmL : ForearmR].Pitch += Passing ? -3.f : FMath::Max(0.f, ArmLead) * 10.f;
                P.Joint[S == 0 ? HandL : HandR].Pitch += Passing ? 7.f : -ArmLead * 5.f;
                P.Joint[S == 0 ? ClavicleL : ClavicleR] += FRotator(-ArmLead * 3.f, -Sign * ArmLead * 5.f, Sign * 2.f);
            }
            return P;
        };
        const FPose LeftReach = StepPose(1.f, false), RightReach = StepPose(-1.f, false);
        FPose LeftPassing = StepPose(.15f, true), RightPassing = StepPose(-.15f, true);
        // Right foot plant / left claw reach -> low passing -> opposite plant.
        // This phase is supplied by actual swing/plant events, never a clock.
        const FKey Keys[] = { {0.f, LeftReach,0.f},{.23f,LeftPassing,1.f},{.5f,RightReach,1.f},
            {.73f,RightPassing,1.f},{1.f,LeftReach,0.f} };
        FPose P = Sample(Keys, UE_ARRAY_COUNT(Keys), Cycle - FMath::FloorToFloat(Cycle));
        const float ForeAft = FMath::Clamp(static_cast<float>(Velocity.X), -1.f, 1.f);
        const float Lateral = FMath::Clamp(static_cast<float>(Velocity.Y), -1.f, 1.f);
        for (int32 S = 0; S < 2; ++S)
        {
            const float Swing = P.HandTarget[S].X - Base.HandTarget[S].X;
            P.HandTarget[S].X = Base.HandTarget[S].X + Swing * ForeAft;
            P.HandTarget[S].Y += FMath::Clamp(Swing * Lateral * .45f, -13.f, 13.f);
            P.HandTarget[S] = FMath::Lerp(Base.HandTarget[S], P.HandTarget[S], Blend);
        }
        for (int32 J = 0; J < JointCount; ++J)
            P.Joint[J] = Base.Joint[J] + (P.Joint[J] - Base.Joint[J]) * Blend;
        const float Load = (PlantLoad.X + PlantLoad.Y) * Blend * Weight;
        const float LoadSide = (PlantLoad.Y - PlantLoad.X) * Blend * Weight;
        P.Joint[Hips] += FRotator(-FMath::Max(0.f, ForeAft) * Blend * (Hunter ? 4.f : 2.f), -Lateral * Blend * 4.f, SideLoad * .35f + LoadSide * 2.f);
        P.Joint[Lumbar] += FRotator(-Load * 2.f, -Turn * 2.f, -SideLoad * .20f);
        P.Joint[Spine] += FRotator(-Load * 2.f, -Turn * 3.f, -LoadSide * 1.5f);
        P.Joint[Chest] += FRotator(-Load * 3.f, -Turn * 4.f, -SideLoad * .30f - LoadSide * 2.f);
        P.Joint[Neck].Pitch += Load * 4.f;
        P.Joint[Neck].Yaw += Turn * 8.f;
        P.Joint[Head].Yaw += Turn * 4.f;
        const float Breath = FMath::Sin(Time * (Caster ? 1.8f : Boss ? 1.25f : 1.55f));
        P.Joint[Chest].Pitch += Breath * .85f;
        P.Joint[Neck].Pitch -= Breath * .35f;
        P.Throat = Caster ? .10f + .045f * Breath : 0.f;
        return P;
    }
    inline FPose Staggered(ERole Role, float Time, float Duration, FVector Direction)
    {
        FPose Base = Ready(Role); Carry(Base, Role);
        FPose Shock = Base, Brace = Base, Balance = Base;
        const float Side = FMath::Abs(Direction.Y) > .18f ? FMath::Sign(Direction.Y) : 1.f;
        const int32 CatchHand = Side > 0.f ? 1 : 0, HurtHand = 1 - CatchHand;
        Shock.Offset = FVector(Direction.X * 3.f, Side * 3.f, -5.f);
        R(Shock,Hips,-11.f,Side * 5.f,Side * 4.f); R(Shock,Lumbar,-7.f,-Side * 3.f);
        R(Shock,Chest,4.f,Side * 9.f,-Side * 4.f); R(Shock,Neck,7.f,-Side * 8.f); R(Shock,Head,13.f,-Side * 5.f);
        Shock.HandTarget[CatchHand] = FVector(23.f,Side * 13.f,-48.f);
        Shock.HandTarget[HurtHand] = FVector(-5.f,-Side * 16.f,-30.f);
        Brace = Shock; Brace.Offset = FVector(-1.f,Side * 5.f,-9.f);
        R(Brace,Hips,-14.f,Side * 8.f,Side * 5.f); R(Brace,Lumbar,-7.f,-Side * 4.f);
        R(Brace,Spine,-4.f,-Side * 4.f); R(Brace,Chest,-9.f,-Side * 8.f,-Side * 4.f);
        R(Brace,Neck,12.f,Side * 7.f); R(Brace,Head,9.f,Side * 5.f);
        Brace.HandTarget[CatchHand] = FVector(29.f,Side * 9.f,-61.f);
        Brace.HandTarget[HurtHand] = FVector(12.f,-Side * 12.f,-39.f);
        Balance = Base; Balance.Offset = FVector(1.f,-Side * 2.f,-4.f);
        R(Balance,Hips,-8.f,-Side * 4.f,-Side * 2.f); R(Balance,Chest,-10.f,Side * 6.f,Side * 3.f);
        R(Balance,Neck,12.f,-Side * 4.f); R(Balance,Head,7.f,-Side * 3.f);
        Balance.HandTarget[CatchHand] = FVector(12.f,Side * 8.f,-70.f);
        Balance.HandTarget[HurtHand] = FVector(17.f,-Side * 9.f,-60.f);
        const float End = FMath::Max(.20f, Duration);
        const FKey Keys[] = {{0.f,Base,0.f},{End * .12f,Shock,1.f},{End * .32f,Brace,.65f},
            {End * .66f,Balance,.7f},{End * .96f,Base,.3f},{End,Base,0.f}};
        return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
    }
    inline float FallProgress(float Time)
    {
        // A failing support accelerates into contact. Smoothstep over the
        // whole descent decelerated the living body as if it chose to sit.
        return FMath::Pow(FMath::Clamp((Time - .075f) / .455f, 0.f, 1.f), 1.55f);
    }
    inline FPose Dying(ERole Role, float Time, FVector Direction)
    {
        FPose P = Ready(Role);
        const float Side = Direction.Y >= 0.f ? 1.f : -1.f;
        const float Back = Direction.X >= 0.f ? -1.f : 1.f;
        const float Yield = FMath::SmoothStep(0.f, .18f, Time);
        const float Fall = FallProgress(Time);
        const float TorsoContact = FMath::SmoothStep(.42f, .61f, Time);
        const float HeadFall = FMath::SmoothStep(.25f, .74f, Time);
        const float ContactAge = FMath::Max(0.f, Time - .53f);
        const float BodySettle = FMath::Exp(-ContactAge * 10.f) * FMath::Sin(ContactAge * 22.f);
        const float HeadAge = FMath::Max(0.f, Time - .72f);
        const float HeadSettle = FMath::Exp(-HeadAge * 8.f) * FMath::Sin(HeadAge * 18.f);
        R(P,Hips,Back * (4.f * Yield + 69.f * Fall),Side * 10.f * Fall,Side * (7.f * Yield + 23.f * Fall));
        R(P,Lumbar,Back * 3.f * TorsoContact,-Side * 2.f * Fall,Side * 2.f * Fall);
        R(P,Spine,Back * (2.f * TorsoContact + BodySettle * 2.f));
        R(P,Chest,Back * (5.f * TorsoContact - BodySettle * 3.f),Side * 4.f * Fall,-Side * 3.f * Fall);
        R(P,Neck,-Back * (11.f * Yield - 18.f * HeadFall),Side * 6.f * HeadFall,Side * 5.f * HeadFall);
        R(P,Head,-Back * (8.f * Yield - 12.f * HeadFall) + HeadSettle * 4.f,Side * 9.f * HeadFall,Side * 7.f * HeadFall);
        for (int32 S = 0; S < 2; ++S)
        {
            const float Sign = S == 0 ? -1.f : 1.f;
            const bool CatchSide = Sign == Side;
            const float ArmFall = CatchSide ? Fall : FMath::SmoothStep(.24f,.74f,Time);
            P.Joint[S == 0 ? ClavicleL : ClavicleR] = FRotator(CatchSide ? -5.f * Yield : 4.f * Fall,
                Sign * 6.f * Fall,Sign * (CatchSide ? 7.f : 4.f) * Yield);
            P.Joint[S == 0 ? ArmL : ArmR] = FRotator((CatchSide ? 31.f : 38.f) - Back * ArmFall * 59.f,
                Sign * (CatchSide ? 8.f : 18.f) * Yield,Sign * (CatchSide ? 5.f : 10.f));
            R(P,S == 0 ? ForearmL : ForearmR,(CatchSide ? 18.f : 31.f) - ArmFall * (CatchSide ? 9.f : 29.f));
            R(P,S == 0 ? HandL : HandR,(CatchSide ? 18.f : 9.f) * ArmFall + (CatchSide ? 0.f : HeadSettle * 3.f));
        }
        P.Throat = .12f * (1.f - Fall);
        return P;
    }
    inline FPose Melee(float Time)
    {
        FPose ReadyPose = Ready(ERole::Melee), Gather = ReadyPose, Coil = ReadyPose, Drive = ReadyPose,
            Contact = ReadyPose, Follow = ReadyPose, Recover = ReadyPose;
        Gather.Offset = FVector(-5.f, 3.f, -5.f);
        R(Gather, Hips, -11.f, 10.f, -3.f); R(Gather, Lumbar, -5.f, 8.f); R(Gather, Spine, -3.f, 9.f);
        R(Gather, Chest, 7.f, 13.f, -4.f); R(Gather, Neck, 8.f, -18.f); R(Gather, Head, 6.f, -11.f);
        R(Gather, ClavicleR, 4.f, 13.f, 7.f);
        Arms(Gather, FRotator(18.f,-16.f,14.f), FRotator(-54.f,22.f,-25.f), -49.f,-54.f, 14.f,-12.f);
        Gather.HandTarget[1] = FVector(-22.f, 39.f, 3.f); Gather.HandWeight[1] = .7f;
        Coil = Gather; Coil.Offset = FVector(-9.f, 5.f, -9.f);
        R(Coil, Hips, -16.f, 16.f, -4.f); R(Coil, Lumbar, -7.f, 10.f); R(Coil, Spine, 2.f, 11.f);
        R(Coil, Chest, 9.f, 15.f, -5.f); R(Coil, Neck, 10.f, -25.f); R(Coil, Head, 4.f, -16.f);
        Coil.HandTarget[1] = FVector(-30.f, 42.f, 14.f); Coil.HandWeight[1] = 1.f;
        Drive = Coil; Drive.Offset = FVector(0.f, 2.f, -10.f);
        R(Drive, Hips, -17.f,-8.f,-2.f); R(Drive,Lumbar,-6.f,2.f); R(Drive,Spine,-2.f,7.f);
        R(Drive,Chest,4.f,12.f,-3.f); Drive.HandTarget[1] = FVector(2.f,42.f,14.f);
        Contact = ReadyPose; Contact.Offset = FVector(14.f,-3.f,-12.f);
        R(Contact,Hips,-16.f,-16.f,3.f); R(Contact,Lumbar,-8.f,-9.f); R(Contact,Spine,-9.f,-10.f);
        R(Contact,Chest,-14.f,-12.f,5.f); R(Contact,Neck,20.f,18.f); R(Contact,Head,13.f,12.f);
        R(Contact,ClavicleR,-5.f,-13.f,-4.f); R(Contact,ClavicleL,3.f,8.f,4.f);
        Arms(Contact,FRotator(-25.f,-18.f,8.f),FRotator(64.f,-12.f,-12.f),-47.f,-24.f,8.f,21.f);
        Contact.HandTarget[1] = FVector(77.f,-16.f,-15.f); Contact.HandWeight[1] = 1.f;
        Contact.HandTarget[0] = FVector(-24.f,-9.f,-48.f); Contact.HandWeight[0] = .85f;
        Follow = Contact; Follow.Offset = FVector(9.f,-5.f,-13.f);
        R(Follow,Hips,-18.f,-11.f,4.f); R(Follow,Lumbar,-10.f,-6.f); R(Follow,Spine,-9.f,-9.f);
        R(Follow,Chest,-11.f,-14.f,6.f); R(Follow,Neck,18.f,22.f); R(Follow,Head,14.f,12.f);
        Follow.HandTarget[1] = FVector(28.f,-28.f,-67.f); R(Follow,HandR,28.f);
        Follow.HandTarget[0] = FVector(-19.f,-13.f,-62.f);
        Recover = ReadyPose; Recover.Offset = FVector(2.f,-2.f,-6.f);
        R(Recover,Hips,-9.f,-5.f,2.f); R(Recover,Chest,-9.f,-5.f); R(Recover,Neck,12.f,5.f);
        Recover.HandTarget[1] = FVector(10.f,22.f,-66.f); Recover.HandWeight[1] = .65f;
        Recover.HandTarget[0] = FVector(2.f,-8.f,-70.f); Recover.HandWeight[0] = .65f;
        const FKey Keys[] = { {-.88f,ReadyPose,0.f},{-.56f,Gather,.65f},{-.16f,Coil,.15f},{-.035f,Coil,0.f},
            {.065f,Drive,1.f},{.17f,Contact,1.f},{.36f,Follow,.65f},{.57f,Follow,.1f},
            {.90f,Recover,.7f},{1.42f,ReadyPose,.25f},{1.75f,ReadyPose,0.f} };
        return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
    }
    inline FPose Hunter(float Time)
    {
        FPose Base=Ready(ERole::Hunter), Coil=Base, Launch=Base, Flight=Base, Brace=Base, Land=Base, Rise=Base;
        Coil.Offset=FVector(-8.f,2.f,-15.f);
        R(Coil,Hips,-24.f,8.f,-3.f); R(Coil,Lumbar,-14.f,4.f); R(Coil,Spine,-8.f,3.f); R(Coil,Chest,-8.f,5.f);
        R(Coil,Neck,25.f,-7.f); R(Coil,Head,24.f,-7.f);
        Arms(Coil,FRotator(-25.f,-10.f,8.f),FRotator(18.f,12.f,-8.f),-56.f,-51.f,12.f,18.f);
        Launch=Coil; Launch.Offset=FVector(9.f,0.f,-4.f);
        R(Launch,Hips,-31.f,-5.f); R(Launch,Lumbar,-15.f,-3.f); R(Launch,Spine,-7.f,-4.f); R(Launch,Chest,-5.f,-7.f);
        R(Launch,Neck,28.f,8.f); R(Launch,Head,24.f,9.f);
        Launch.HandTarget[0]=FVector(12.f,-23.f,-41.f); Launch.HandTarget[1]=FVector(72.f,18.f,-12.f);
        Launch.HandWeight[0]=Launch.HandWeight[1]=1.f;
        Flight=Launch; Flight.Offset=FVector(12.f,0.f,0.f); R(Flight,Hips,-35.f,-7.f); R(Flight,Chest,-7.f,-8.f);
        Flight.HandTarget[0]=FVector(14.f,-22.f,-46.f); Flight.HandTarget[1]=FVector(67.f,20.f,-18.f);
        Brace=Flight; Brace.Offset=FVector(7.f,0.f,-3.f); R(Brace,Hips,-24.f,-3.f); R(Brace,Lumbar,-10.f);
        R(Brace,Chest,-11.f,-4.f); Brace.HandTarget[0]=FVector(22.f,-18.f,-57.f); Brace.HandTarget[1]=FVector(39.f,17.f,-51.f);
        Land=Brace; Land.Offset=FVector(8.f,0.f,-15.f); R(Land,Hips,-27.f); R(Land,Lumbar,-12.f); R(Land,Chest,-13.f);
        Land.HandTarget[0]=FVector(20.f,-17.f,-66.f); Land.HandTarget[1]=FVector(29.f,12.f,-64.f);
        Rise=Base; Rise.Offset=FVector(5.f,0.f,-7.f); R(Rise,Hips,-18.f); R(Rise,Neck,23.f); R(Rise,Head,17.f);
        Rise.HandTarget[0]=FVector(14.f,-24.f,-64.f); Rise.HandTarget[1]=FVector(20.f,21.f,-65.f);
        Rise.HandWeight[0]=Rise.HandWeight[1]=.45f;
        const FKey Keys[]={{-.78f,Base,0.f},{-.34f,Coil,.35f},{-.06f,Coil,0.f},{.07f,Launch,1.f},
            {.22f,Flight,.7f},{.39f,Brace,1.f},{.49f,Land,.35f},{.68f,Rise,.65f},{1.05f,Base,.3f},{1.48f,Base,0.f}};
        return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
    }
    inline FPose Cast(float Time, bool Boss, bool Guard=false)
    {
        const ERole Role=Boss?ERole::Boss:ERole::Caster;
        FPose Base=Ready(Role), Gather=Base, Set=Base, Release=Base, Reload=Base, Second=Base, Exhaust=Base;
        Gather.Offset=FVector(-5.f,-5.f,-6.f);
        R(Gather,Hips,-13.f,-12.f,-4.f); R(Gather,Lumbar,-5.f,5.f); R(Gather,Spine,1.f,6.f); R(Gather,Chest,10.f,13.f,5.f);
        R(Gather,Neck,0.f,-12.f); R(Gather,Head,3.f,-10.f);
        R(Gather,ClavicleL,4.f,-10.f,-5.f); R(Gather,ClavicleR,-3.f,12.f,5.f);
        Arms(Gather,FRotator(20.f,-14.f,10.f),FRotator(-32.f,18.f,-16.f),37.f,54.f,-17.f,-21.f);
        Gather.HandTarget[0]=FVector(-19.f,-18.f,-7.f); Gather.HandTarget[1]=FVector(-38.f,24.f,24.f);
        Gather.HandWeight[0]=Gather.HandWeight[1]=1.f; Gather.Throat=.8f;
        Set=Gather; Set.Offset=FVector(-8.f,-4.f,-8.f); R(Set,Chest,13.f,16.f,5.f); R(Set,Neck,-3.f,-14.f);
        Set.HandTarget[1]=FVector(-19.f,20.f,8.f); Set.Throat=1.f;
        Release=Set; Release.Offset=FVector(8.f,2.f,-10.f);
        R(Release,Hips,-16.f,6.f,2.f); R(Release,Lumbar,-9.f,-4.f); R(Release,Spine,-8.f,-5.f); R(Release,Chest,-14.f,-9.f,-4.f);
        R(Release,Neck,13.f,8.f); R(Release,Head,9.f,7.f); R(Release,ClavicleR,-5.f,-12.f,-5.f);
        Release.HandTarget[0]=FVector(-18.f,-17.f,-5.f); Release.HandTarget[1]=FVector(-3.f,13.f,-4.f); Release.Throat=.18f;
        Reload=Gather; Reload.Offset=FVector(-2.f,3.f,-7.f); R(Reload,Hips,-13.f,7.f,2.f); R(Reload,Chest,7.f,-12.f,-4.f);
        Reload.HandTarget[0]=FVector(-28.f,-22.f,14.f); Reload.HandTarget[1]=FVector(-18.f,15.f,-6.f); Reload.Throat=.85f;
        Second=Release; Second.Offset=FVector(7.f,-2.f,-10.f); R(Second,Hips,-16.f,-5.f,-2.f); R(Second,Chest,-13.f,10.f,4.f);
        R(Second,Neck,13.f,-9.f); R(Second,Head,9.f,-7.f); R(Second,ClavicleL,-5.f,12.f,5.f);
        Second.HandTarget[0]=FVector(-4.f,-13.f,-4.f); Second.HandTarget[1]=FVector(-19.f,17.f,-6.f);
        Exhaust=Base; Exhaust.Offset=FVector(4.f,-3.f,-12.f); R(Exhaust,Hips,-17.f,-4.f,-3.f); R(Exhaust,Lumbar,-8.f);
        R(Exhaust,Chest,-9.f,7.f); R(Exhaust,Neck,8.f,-6.f); R(Exhaust,Head,12.f);
        Exhaust.HandTarget[0]=FVector(-30.f,-24.f,-24.f); Exhaust.HandTarget[1]=FVector(-36.f,22.f,-18.f);
        Exhaust.HandWeight[0]=Exhaust.HandWeight[1]=.8f; Exhaust.Throat=.08f;
        if (Guard)
        {
            Set.Offset=FVector(-7.f,0.f,-10.f); R(Set,Hips,-17.f); R(Set,Chest,-12.f);
            Set.HandTarget[0]=FVector(-8.f,14.f,15.f); Set.HandTarget[1]=FVector(-8.f,-14.f,30.f);
            const FKey Keys[]={{-1.f,Base,0.f},{-.5f,Gather,.5f},{-.14f,Set,.2f},{.72f,Set,0.f},{1.25f,Exhaust,.5f},{2.55f,Base,0.f}};
            return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
        }
        if (Boss)
        {
            const FKey Keys[]={{-1.35f,Base,0.f},{-.76f,Gather,.6f},{-.12f,Set,.2f},{.025f,Release,1.f},
                {.14f,Reload,.7f},{.225f,Second,1.f},{.34f,Set,.7f},{.425f,Release,1.f},
                {.73f,Exhaust,.6f},{1.15f,Exhaust,0.f},{1.95f,Base,.3f},{2.35f,Base,0.f}};
            return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
        }
        // Regain balance through the existing late vulnerability interval.
        // The spent left hand falls first; the right stays near the throat
        // for one shallow inhalation before it also returns to the carry.
        FPose Regain=Exhaust;
        Regain.Offset=FVector(-1.f,1.f,-6.f);
        R(Regain,Hips,-11.f,3.f,2.f); R(Regain,Lumbar,-5.f); R(Regain,Spine,-3.f);
        R(Regain,Chest,1.f,-4.f,-2.f); R(Regain,Neck,6.f,3.f); R(Regain,Head,8.f,2.f);
        Regain.HandTarget[0]=FVector(-44.f,-20.f,-42.f); Regain.HandTarget[1]=FVector(-29.f,16.f,-25.f);
        Regain.HandWeight[0]=.40f; Regain.HandWeight[1]=.70f; Regain.Throat=.28f;
        const FKey Keys[]={{-1.18f,Base,0.f},{-.72f,Gather,.55f},{-.10f,Set,.2f},{.035f,Release,1.f},
            {.16f,Reload,.7f},{.23f,Reload,.15f},{.31f,Second,1.f},{.52f,Exhaust,.55f},
            {.79f,Exhaust,.1f},{1.28f,Regain,.65f},{1.72f,Base,.2f},{1.79f,Base,0.f}};
        return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
    }
    inline FPose Ground(float Time, bool Slam)
    {
        FPose Base=Ready(ERole::Boss), Gather=Base, High=Base, Drive=Base, Impact=Base, Hold=Base, Rise=Base;
        Gather.Offset=FVector(-5.f,0.f,-10.f); R(Gather,Hips,-15.f); R(Gather,Lumbar,-8.f); R(Gather,Spine,-4.f);
        R(Gather,Chest,6.f); R(Gather,Neck,12.f); R(Gather,Head,10.f);
        Arms(Gather,FRotator(-50.f,-8.f,-4.f),FRotator(-56.f,10.f,4.f),-42.f,-38.f,9.f,13.f);
        High.Offset=FVector(-7.f,0.f,-6.f); R(High,Hips,7.f); R(High,Lumbar,0.f); R(High,Spine,4.f); R(High,Chest,10.f);
        R(High,Neck,-7.f); R(High,Head,-10.f); R(High,ClavicleL,5.f,-8.f,-6.f); R(High,ClavicleR,5.f,8.f,6.f);
        Arms(High,FRotator(-112.f,-7.f,-4.f),FRotator(-119.f,8.f,4.f),-20.f,-17.f,-8.f,-11.f);
        Drive=High; Drive.Offset=FVector(3.f,0.f,-14.f); R(Drive,Hips,-17.f); R(Drive,Lumbar,-9.f); R(Drive,Spine,-7.f);
        R(Drive,Chest,-4.f); R(Drive,Neck,10.f); R(Drive,Head,5.f);
        Impact=Base; Impact.Offset=FVector(8.f,0.f,-25.f);
        R(Impact,Hips,-22.f); R(Impact,Lumbar,-13.f); R(Impact,Spine,-14.f); R(Impact,Chest,-13.f);
        R(Impact,Neck,16.f); R(Impact,Head,11.f); R(Impact,ClavicleL,-4.f,9.f,4.f); R(Impact,ClavicleR,-4.f,-9.f,-4.f);
        Arms(Impact,FRotator(42.f,-6.f,0.f),FRotator(45.f,6.f,0.f),-33.f,-31.f,24.f,24.f);
        Hold=Impact; Hold.Offset.Z=-27.f; R(Hold,Neck,12.f); R(Hold,Head,7.f);
        Rise=Base; Rise.Offset=FVector(5.f,-3.f,-14.f); R(Rise,Hips,-16.f,4.f,-3.f); R(Rise,Lumbar,-9.f);
        R(Rise,Chest,-13.f,-4.f); R(Rise,Neck,18.f); R(Rise,Head,12.f);
        if (!Slam)
        {
            High.Offset=FVector(-7.f,4.f,-9.f); R(High,Hips,-9.f,12.f,-3.f); R(High,Chest,10.f,15.f,5.f);
            Arms(High,FRotator(-51.f,-16.f,5.f),FRotator(-110.f,17.f,-8.f),-48.f,-28.f,12.f,-14.f);
            High.HandTarget[1]=FVector(-20.f,28.f,28.f); High.HandWeight[1]=1.f;
            Drive=High; Drive.Offset=FVector(2.f,1.f,-13.f); R(Drive,Hips,-17.f,-4.f);
            R(Drive,Lumbar,-8.f,0.f); R(Drive,Chest,0.f,5.f);
            Drive.HandTarget[1]=FVector(20.f,24.f,4.f);
            Impact.Offset=FVector(9.f,-4.f,-14.f); R(Impact,Hips,-18.f,-12.f,3.f); R(Impact,Lumbar,-10.f,-7.f);
            R(Impact,Spine,-7.f,-8.f); R(Impact,Chest,-12.f,-12.f,4.f);
            Arms(Impact,FRotator(28.f,-18.f,9.f),FRotator(72.f,-8.f,-5.f),-40.f,-26.f,16.f,27.f);
            Impact.HandTarget[1]=FVector(72.f,-12.f,-35.f); Impact.HandWeight[1]=1.f;
            Hold=Impact; Hold.Offset.Z=-15.f;
            Hold.HandTarget[1]=FVector(37.f,-9.f,-65.f);
            Rise.HandTarget[1]=FVector(17.f,23.f,-66.f); Rise.HandWeight[1]=.60f;
        }
        const FKey Keys[]={{Slam?-1.1f:-1.5f,Base,0.f},{Slam?-.73f:-1.f,Gather,.65f},
            {-.18f,High,.25f},{-.025f,High,0.f},{.075f,Drive,1.f},{.17f,Impact,1.f},
            {.32f,Hold,.1f},{.57f,Hold,0.f},{.94f,Rise,.65f},{1.48f,Base,.35f},{2.29f,Base,0.f}};
        return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
    }
}
