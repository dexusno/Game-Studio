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
        float Jaw = 0.f;
        float Grip[2] = { 0.f, 0.f };
        float Crest = 0.f;
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
            Out.Grip[S] = (A.Grip[S] - B.Grip[S]) * Scale;
        }
        Out.Throat = (A.Throat - B.Throat) * Scale;
        Out.Jaw = (A.Jaw - B.Jaw) * Scale;
        Out.Crest = (A.Crest - B.Crest) * Scale;
        return Out;
    }
    inline FPose Sample(const FKey* Keys, int32 Count, float Time, bool Loop = false)
    {
        if (Time <= Keys[0].Time) return Keys[0].Pose;
        if (Time >= Keys[Count - 1].Time) return Keys[Count - 1].Pose;
        int32 I = 0;
        while (I + 1 < Count && Time > Keys[I + 1].Time) ++I;
        const float Duration = Keys[I + 1].Time - Keys[I].Time;
        const float T = (Time - Keys[I].Time) / Duration;
        auto Tangent = [&](int32 K)
        {
            if (K == 0 || K == Count - 1)
                return Loop ? Difference(Keys[1].Pose, Keys[Count - 2].Pose,
                    Keys[K].Flow / (Keys[1].Time - Keys[0].Time + Keys[Count - 1].Time - Keys[Count - 2].Time)) : FPose();
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
            Out.Grip[S] = FMath::Clamp(FMath::CubicInterp(P.Grip[S], A.Grip[S] * Duration,
                Q.Grip[S], B.Grip[S] * Duration, T), 0.f, 1.f);
        }
        Out.Throat = FMath::Clamp(FMath::CubicInterp(P.Throat, A.Throat * Duration, Q.Throat, B.Throat * Duration, T), 0.f, 1.f);
        Out.Jaw = FMath::Clamp(FMath::CubicInterp(P.Jaw, A.Jaw * Duration, Q.Jaw, B.Jaw * Duration, T), 0.f, 1.f);
        Out.Crest = FMath::Clamp(FMath::CubicInterp(P.Crest, A.Crest * Duration, Q.Crest, B.Crest * Duration, T), 0.f, 1.f);
        return Out;
    }
    inline FPose BlendPose(const FPose& A, const FPose& B, float Weight)
    {
        FPose P;
        P.Offset = FMath::Lerp(A.Offset,B.Offset,Weight);
        for (int32 J=0;J<JointCount;++J) P.Joint[J]=A.Joint[J]+(B.Joint[J]-A.Joint[J])*Weight;
        for (int32 S=0;S<2;++S)
        {
            P.HandTarget[S]=FMath::Lerp(A.HandTarget[S],B.HandTarget[S],Weight);
            P.HandWeight[S]=FMath::Lerp(A.HandWeight[S],B.HandWeight[S],Weight);
            P.Grip[S]=FMath::Lerp(A.Grip[S],B.Grip[S],Weight);
        }
        P.Throat=FMath::Lerp(A.Throat,B.Throat,Weight);
        P.Jaw=FMath::Lerp(A.Jaw,B.Jaw,Weight);
        P.Crest=FMath::Lerp(A.Crest,B.Crest,Weight);
        return P;
    }
    inline FPose SamplePerformance(const FKey* Keys, int32 Count, float Time, float HipLead, float ChestDelay, float HeadDelay)
    {
        FPose P=Sample(Keys,Count,Time);
        const FPose Support=Sample(Keys,Count,Time+HipLead);
        const FPose Ribs=Sample(Keys,Count,Time-ChestDelay);
        // Attention starts in the head during preparation. On commitment the
        // heavy head follows the ribs; this change itself blends continuously.
        const float HeadOffset=FMath::Lerp(.055f,-HeadDelay,FMath::SmoothStep(-.14f,.04f,Time));
        const FPose Attention=Sample(Keys,Count,Time+HeadOffset);
        P.Offset=Support.Offset; P.Joint[Hips]=Support.Joint[Hips]; P.Joint[Lumbar]=Support.Joint[Lumbar];
        P.Joint[Spine]=Ribs.Joint[Spine]; P.Joint[Chest]=Ribs.Joint[Chest];
        P.Joint[Neck]=Attention.Joint[Neck]; P.Joint[Head]=Attention.Joint[Head];
        // Wrist targets retain the unshifted action clock: damage/projectile
        // contact must not be postponed by secondary body overlap.
        return P;
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
        P.Jaw = Hunter ? .12f : Caster ? .07f : .04f;
        P.Grip[0] = Caster ? .13f : Hunter ? .38f : .22f;
        P.Grip[1] = Caster ? .21f : Hunter ? .48f : .31f;
        P.Crest = Caster ? .18f : 0.f;
        P.Throat = Caster ? .09f : 0.f;
        return P;
    }
    inline void Carry(FPose& P, ERole Role)
    {
        const bool Caster = Role == ERole::Caster, Hunter = Role == ERole::Hunter;
        P.HandTarget[0] = FVector(Hunter ? 17.f : Caster ? 14.f : 5.f, -7.f, Caster ? -57.f : -71.f);
        P.HandTarget[1] = FVector(Hunter ? 22.f : Caster ? 20.f : 9.f, 7.f, Caster ? -52.f : -68.f);
        P.HandWeight[0] = P.HandWeight[1] = 1.f;
    }
    inline FPose CasterCarriage(bool HandTargets)
    {
        FPose P=Ready(ERole::Caster);
        P.Offset=FVector(5.f,-2.f,-5.f);
        R(P,Hips,-13.f,-5.f,-2.f); R(P,Lumbar,-6.f,3.f,1.f);
        R(P,Spine,-6.f,2.f,1.f); R(P,Chest,-1.f,7.f,-2.f);
        R(P,Neck,12.f,-5.f); R(P,Head,8.f,-3.f);
        R(P,ClavicleL,-1.f,-5.f,-3.f); R(P,ClavicleR,3.f,8.f,5.f);
        Arms(P,FRotator(26.f,-7.f,-15.f),FRotator(40.f,12.f,15.f),15.f,31.f,-3.f,-12.f);
        if (HandTargets)
        {
            P.HandTarget[0]=FVector(8.f,-11.f,-61.f);
            P.HandTarget[1]=FVector(31.f,12.f,-42.f);
            P.HandWeight[0]=P.HandWeight[1]=1.f;
        }
        P.Grip[0]=.12f; P.Grip[1]=.33f;
        return P;
    }
    inline FPose Idle(ERole Role, float Time)
    {
        FPose Base = Ready(Role); Carry(Base, Role);
        const bool Caster = Role == ERole::Caster, Hunter = Role == ERole::Hunter, Boss = Role == ERole::Boss;
        if (Caster) Base=CasterCarriage(true);
        FPose Inhale = Base, Listen = Base, Release = Base, Resettle = Base, Check = Base;
        // An inhalation opens the ribs first. The head checks forward before
        // the body shifts back onto the other support; neither arm mirrors it.
        Inhale.Offset += FVector(-.5f, -.7f, .3f);
        Inhale.Joint[Lumbar].Pitch += 1.f;
        Inhale.Joint[Chest] += FRotator(Caster ? 3.f : 2.f, 1.f, -.5f);
        Inhale.Joint[ClavicleL].Pitch += 1.5f;
        Inhale.Joint[ClavicleR].Roll += 1.f;
        Inhale.HandTarget[0] += FVector(-1.f, -1.f, 1.f);
        Inhale.Jaw = Caster ? .10f : .12f;
        Inhale.Throat = Caster ? .26f : 0.f;
        Inhale.Crest = Caster ? .30f : 0.f;
        Inhale.Grip[0] *= .7f;
        Listen = Inhale;
        Listen.Joint[Neck] += FRotator(-2.f, -3.f, 1.f);
        Listen.Joint[Head] += FRotator(Hunter ? -4.f : -2.f, 3.f, -1.f);
        Listen.Jaw = Hunter ? .22f : .08f;
        Listen.HandTarget[1] += FVector(2.f, -1.f, 2.f);
        Listen.Grip[1] += .12f;
        Release.Offset += FVector(1.f, .4f, -.6f);
        Release.Joint[Hips] += FRotator(-1.f, 1.f, .5f);
        Release.Joint[Spine] += FRotator(-1.f, -1.f, 0.f);
        Release.Joint[Chest] += FRotator(-1.5f, -2.f, .5f);
        Release.Joint[Neck].Pitch += 2.f;
        Release.Jaw = Caster ? .24f : .18f;
        Release.Grip[1] *= .65f;
        Release.Throat = Caster ? .025f : 0.f;
        Release.Crest = Caster ? .13f : 0.f;
        Resettle.Offset += FVector(.3f, 1.f, -.2f);
        Resettle.Joint[Hips] += FRotator(-.5f, -2.f, 1.f);
        Resettle.Joint[Lumbar].Yaw += 1.f;
        Resettle.Joint[Chest] += FRotator(.5f, 2.f, -1.f);
        Resettle.HandTarget[0] += FVector(1.5f, 1.f, -1.f);
        Resettle.HandTarget[1] += FVector(-1.5f, 1.f, -.5f);
        Resettle.Grip[0] += .13f;
        Check = Base;
        Check.Joint[Neck] += FRotator(Hunter ? -3.f : -1.f, 2.f, -.5f);
        Check.Joint[Head] += FRotator(1.f, -2.f, .5f);
        Check.HandTarget[1] += FVector(1.f, 0.f, 1.f);
        Check.Crest = Caster ? .25f : 0.f;
        if (Caster)
        {
            Inhale.Offset+=FVector(-2.f,-1.f,-1.f);
            Inhale.Joint[Lumbar].Pitch-=1.f;
            Inhale.Joint[Chest]+=FRotator(2.f,-2.f,1.f);
            Inhale.HandTarget[1]+=FVector(-3.f,3.f,3.f);
            Listen.Offset+=FVector(-2.f,-1.f,-1.f);
            Listen.HandTarget[1]+=FVector(-2.f,3.f,3.f);
            Release.Offset+=FVector(2.f,1.f,-2.f);
            Release.Joint[Chest].Pitch-=2.f;
            Release.HandTarget[0]+=FVector(2.f,-2.f,-2.f);
            Resettle.Offset+=FVector(-1.f,2.f,-1.f);
            Resettle.Joint[Hips].Roll+=1.5f;
            Resettle.Joint[Chest].Roll-=1.5f;
        }
        const FKey Keys[] = {{0.f, Base, .65f}, {1.4f, Inhale, .7f}, {2.05f, Listen, .4f},
            {3.2f, Release, .65f}, {4.9f, Resettle, .6f}, {6.45f, Check, .5f}, {8.4f, Base, .65f}};
        const float Clock = Time * (Boss ? .78f : Hunter ? 1.14f : 1.f);
        return Sample(Keys, UE_ARRAY_COUNT(Keys), FMath::Fmod(FMath::Max(0.f, Clock), 8.4f), true);
    }
    inline FPose Travel(ERole Role, float Blend, float Cycle, float SideLoad,
        float Turn, FVector Velocity, FVector2D PlantLoad, float Time)
    {
        FPose Base = Ready(Role); Carry(Base, Role);
        const bool Caster = Role == ERole::Caster, Hunter = Role == ERole::Hunter, Boss = Role == ERole::Boss;
        if (Caster)
        {
            Base=CasterCarriage(true);
            // Travel releases the gathered casting arm and the permanent squat.
            // Weight is accepted over each support instead of held at one height.
            Base.Offset=FVector(7.f,0.f,-2.f);
            R(Base,Hips,-16.f,-2.f); R(Base,Lumbar,-8.f,1.f);
            R(Base,Spine,-6.f,1.f); R(Base,Chest,1.f,2.f);
            R(Base,Neck,14.f,-2.f); R(Base,Head,8.f,-1.f);
            Base.HandTarget[0]=FVector(14.f,-13.f,-58.f);
            Base.HandTarget[1]=FVector(20.f,14.f,-54.f);
        }
        else if (!Hunter)
        {
            Base.Offset=FVector(Boss?7.f:9.f,-1.f,-3.f);
            R(Base,Hips,-9.f,-2.f); R(Base,Lumbar,-6.f,2.f);
            R(Base,Spine,-5.f,2.f); R(Base,Chest,-3.f,2.f);
            R(Base,Neck,7.f,-3.f); R(Base,Head,5.f,-2.f);
            Base.HandTarget[0]=FVector(8.f,-8.f,-68.f);
            Base.HandTarget[1]=FVector(16.f,10.f,-62.f);
        }
        // Each half-stride has a contact silhouette, delayed acceptance of
        // weight, low arm passing and a catch. This is not a sine on a fixed
        // torso: the heavy creature folds/unfolds its back over each support.
        auto StepPose = [&](float Lead, int32 Beat)
        {
            FPose P = Base;
            const bool Load = Beat == 1, Passing = Beat == 2, Catch = Beat == 3;
            const float Twist = Lead * (Passing ? .25f : Catch ? -.65f : 1.f);
            if (Caster)
            {
                P.Offset += FVector(Load ? 3.f : Passing ? -1.f : Catch ? 1.f : 0.f,
                    Twist * 2.5f, Load ? -4.5f : Passing ? 1.5f : Catch ? -1.5f : 0.f);
                P.Joint[Hips] += FRotator(Load ? -4.f : Passing ? 2.f : -1.f, -Twist * 9.f, Twist * 4.f);
                P.Joint[Lumbar] += FRotator(Load ? -3.f : Passing ? 2.f : 0.f, Twist * 5.f, -Twist * 1.5f);
                P.Joint[Spine] += FRotator(Load ? -2.f : Passing ? 1.f : 0.f, Twist * 5.f, -Twist * 2.f);
                P.Joint[Chest] += FRotator(Load ? -3.f : Passing ? 3.f : 1.f, Twist * 9.f, -Twist * 4.f);
                P.Joint[Neck] += FRotator(Load ? 5.f : Passing ? -2.f : 0.f, -Twist * 3.f, Twist);
                P.Joint[Head] += FRotator(Passing ? -2.f : 1.f, -Twist, -Twist * .5f);
            }
            else if (Hunter)
            {
                P.Offset += FVector(Load ? 3.f : Passing ? -1.f : 1.f, Twist, Load ? -2.4f : Passing ? .5f : -.5f);
                P.Joint[Hips] += FRotator(Load ? -5.f : Passing ? 3.f : -1.f, -Twist * 6.f, Twist * 2.f);
                P.Joint[Lumbar] += FRotator(Load ? -2.f : Passing ? 1.f : 0.f, Twist * 2.f, -Twist);
                P.Joint[Spine] += FRotator(Load ? -3.f : Passing ? 2.f : -1.f, Twist * 4.f, -Twist * 1.5f);
                P.Joint[Chest] += FRotator(Load ? -4.f : Passing ? 3.f : 0.f, Twist * 5.f, -Twist * 2.f);
                P.Joint[Neck] += FRotator(Load ? 6.f : Passing ? -3.f : 1.f, -Twist * 5.f, 0.f);
                P.Joint[Head] += FRotator(Load ? 2.f : Passing ? -2.f : 0.f, -Twist * 3.f, 0.f);
            }
            else
            {
                // The pelvis passes over support while the opposite shoulder
                // pulls forward. Keep a pursuing lean between load beats;
                // lifting the whole back on every pass looked like jogging
                // underneath an otherwise passive head in the v1 frames.
                P.Offset += FVector(Load ? 3.f : Passing ? -.5f : 1.f, Twist * (Boss ? 2.5f : 2.f),
                    Load ? -2.2f : Passing ? -.2f : -.8f);
                P.Joint[Hips] += FRotator(Load ? -2.f : Passing ? 1.f : -1.f, -Twist * 6.f, Twist * 4.f);
                P.Joint[Lumbar] += FRotator(Load ? -1.f : Passing ? 1.f : 0.f, Twist * 5.f, -Twist * 2.f);
                P.Joint[Spine] += FRotator(Load ? -2.f : Passing ? 2.f : -1.f, Twist * 5.f, -Twist * 3.f);
                P.Joint[Chest] += FRotator(Load ? -3.f : Passing ? 2.f : -1.f, Twist * (Boss ? 10.f : 12.f), -Twist * 4.f);
                P.Joint[Neck] += FRotator(Load ? 2.f : Passing ? -1.f : 0.f, -Twist * 4.f, Twist);
                P.Joint[Head] += FRotator(Load ? 1.f : 0.f, -Twist * 2.f, -Twist * .5f);
            }
            for (int32 S = 0; S < 2; ++S)
            {
                const float Sign = S == 0 ? -1.f : 1.f;
                const float ArmLead = Lead * -Sign * (Catch ? -.70f : Load ? .85f : 1.f);
                const bool Forward = ArmLead > 0.f;
                const float Front = Caster ? 40.f : Hunter ? 46.f : Boss ? 31.f : 38.f;
                const float Rear = Caster ? -20.f : Hunter ? -10.f : Boss ? -22.f : -26.f;
                P.HandTarget[S] = Passing
                    ? FVector(Caster ? 8.f : Hunter ? 16.f : (S == 0 ? -1.f : 5.f), Sign * (Caster ? 15.f : 7.f), Caster ? -68.f : -77.f)
                    : FVector(FMath::Lerp(Base.HandTarget[S].X, Forward ? Front : Rear, FMath::Abs(ArmLead)),
                        Sign * (Forward ? 4.f : 11.f), Caster ? (Forward ? -47.f : -58.f) : (Forward ? -57.f : -66.f));
                // The shoulder arrives before the loosely carried claw. The
                // low pass releases grip; it curls again on the forward catch.
                P.Joint[S == 0 ? ForearmL : ForearmR].Pitch += Passing ? -3.f : Forward ? 12.f : -2.f;
                P.Joint[S == 0 ? HandL : HandR].Pitch += Passing ? 9.f : Forward ? -8.f : 4.f;
                P.Joint[S == 0 ? ClavicleL : ClavicleR] += FRotator(-ArmLead * 4.f,
                    -Sign * ArmLead * 6.f, Sign * (Load ? 3.f : 1.f));
                if (Caster)
                {
                    // Both forelimbs counter the supporting leg. The right
                    // reaches a little farther; neither is parked at the throat.
                    P.HandTarget[S].X += S==1 ? 3.f : -2.f;
                    P.HandTarget[S].Y = Sign * (Passing ? 15.f : Forward ? 12.f : 19.f);
                    P.HandTarget[S].Z = Passing ? -68.f : Forward ? -48.f : -66.f;
                    P.Joint[S==0?HandL:HandR].Pitch += Passing ? 6.f : Forward ? -5.f : 2.f;
                }
                else if (!Hunter)
                {
                    const float LeadReach=S==0?(Boss?30.f:33.f):(Boss?36.f:42.f);
                    if (!Passing)
                    {
                        P.HandTarget[S].X=FMath::Lerp(Base.HandTarget[S].X,Forward?LeadReach:-17.f,FMath::Abs(ArmLead));
                        P.HandTarget[S].Z=Forward?(S==0?-58.f:-51.f):-65.f;
                    }
                    P.Joint[S==0?ClavicleL:ClavicleR]+=FRotator(Forward?-2.f:1.f,
                        -Sign*ArmLead*4.f,Sign*(Forward?2.f:0.f));
                }
                P.Grip[S] = Caster ? (Passing ? .09f : Forward ? .26f : .17f)
                    : Hunter ? (Passing ? .30f : Forward ? .58f : .40f)
                    : Passing ? .12f : Forward ? .37f : .25f;
            }
            P.Jaw = Caster ? (Load ? .12f : .07f) : Hunter ? (Load ? .24f : .12f) : Load ? .17f : .05f;
            P.Crest = Caster ? (Load ? .23f : Passing ? .16f : .19f) : 0.f;
            P.Throat = Caster ? (Load ? .06f : Passing ? .14f : .10f) : 0.f;
            return P;
        };
        const FKey Keys[] = {{0.f,StepPose(1.f,0),.8f},{.10f,StepPose(1.f,1),.65f},
            {.245f,StepPose(1.f,2),.9f},{.405f,StepPose(1.f,3),.85f},{.5f,StepPose(-1.f,0),.8f},
            {.61f,StepPose(-1.f,1),.65f},{.755f,StepPose(-1.f,2),.9f},{.91f,StepPose(-1.f,3),.85f},
            {1.f,StepPose(1.f,0),.8f}};
        auto AtCycle = [&](float Phase)
        { return Sample(Keys, UE_ARRAY_COUNT(Keys), Phase - FMath::FloorToFloat(Phase), true); };
        FPose P = AtCycle(Cycle);
        const FPose Ribs = AtCycle(Cycle - (Caster ? .045f : Hunter ? .04f : Boss ? .12f : .09f));
        const FPose Attention = AtCycle(Cycle - (Caster ? .08f : Hunter ? .055f : Boss ? .18f : .15f));
        const FPose Claws = AtCycle(Cycle - (Caster ? .025f : Hunter ? .035f : .065f));
        P.Joint[Spine]=Ribs.Joint[Spine]; P.Joint[Chest]=Ribs.Joint[Chest];
        P.Joint[Neck]=Attention.Joint[Neck]; P.Joint[Head]=Attention.Joint[Head];
        P.Jaw=Attention.Jaw; P.Crest=Attention.Crest;
        const float ForeAft = FMath::Clamp(static_cast<float>(Velocity.X), -1.f, 1.f);
        const float Lateral = FMath::Clamp(static_cast<float>(Velocity.Y), -1.f, 1.f);
        for (int32 S = 0; S < 2; ++S)
        {
            P.HandTarget[S]=Claws.HandTarget[S]; P.Grip[S]=Claws.Grip[S];
            const float Swing = P.HandTarget[S].X - Base.HandTarget[S].X;
            P.HandTarget[S].X = Base.HandTarget[S].X + Swing * ForeAft;
            P.HandTarget[S].Y += FMath::Clamp(Swing * Lateral * .45f, -13.f, 13.f);
        }
        P = BlendPose(Idle(Role, Time), P, FMath::Clamp(Blend, 0.f, 1.f));
        const float Load = (PlantLoad.X + PlantLoad.Y) * Blend;
        const float LoadSide = (PlantLoad.Y - PlantLoad.X) * Blend;
        P.Joint[Hips] += FRotator(-FMath::Max(0.f, ForeAft) * Blend * (Hunter ? 4.f : 2.f), -Lateral * Blend * 4.f, SideLoad * .35f + LoadSide * 2.f);
        P.Joint[Lumbar] += FRotator(-Load, -Turn, -SideLoad * .20f);
        P.Joint[Spine] += FRotator(-Load, -Turn * 1.5f, -LoadSide);
        P.Joint[Chest] += FRotator(-Load * 1.5f, -Turn * 2.f, -SideLoad * .30f - LoadSide);
        P.Joint[Neck].Pitch += Load * 2.f;
        P.Joint[Neck].Yaw += Turn * 4.f;
        P.Joint[Head].Yaw += Turn * 2.f;
        if (Caster)
        {
            const float Pivot=FMath::Abs(Turn)*(1.f-FMath::Clamp(static_cast<float>(Velocity.Size2D()),0.f,1.f));
            P.Offset.Z -= Pivot*2.5f;
            P.Joint[Hips] += FRotator(-Pivot*3.f,Turn*5.f,Turn*2.f);
            P.Joint[Chest] += FRotator(-Pivot*2.f,Turn*7.f,-Turn*3.f);
            P.HandTarget[0].Y -= Pivot*7.f;
            P.HandTarget[1].Y += Pivot*7.f;
        }
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
        Shock.Jaw=.62f; Shock.Grip[CatchHand]=.10f; Shock.Grip[HurtHand]=.70f;
        Shock.Crest=Role==ERole::Caster?.80f:0.f; Shock.Throat=Role==ERole::Caster?.04f:0.f;
        Brace = Shock; Brace.Offset = FVector(-1.f,Side * 5.f,-9.f);
        R(Brace,Hips,-14.f,Side * 8.f,Side * 5.f); R(Brace,Lumbar,-7.f,-Side * 4.f);
        R(Brace,Spine,-4.f,-Side * 4.f); R(Brace,Chest,-9.f,-Side * 8.f,-Side * 4.f);
        R(Brace,Neck,12.f,Side * 7.f); R(Brace,Head,9.f,Side * 5.f);
        Brace.HandTarget[CatchHand] = FVector(29.f,Side * 9.f,-61.f);
        Brace.HandTarget[HurtHand] = FVector(12.f,-Side * 12.f,-39.f);
        Brace.Jaw=.31f; Brace.Grip[CatchHand]=.18f; Brace.Grip[HurtHand]=.56f;
        Brace.Crest=Role==ERole::Caster?.35f:0.f;
        Balance = Base; Balance.Offset = FVector(1.f,-Side * 2.f,-4.f);
        R(Balance,Hips,-8.f,-Side * 4.f,-Side * 2.f); R(Balance,Chest,-10.f,Side * 6.f,Side * 3.f);
        R(Balance,Neck,12.f,-Side * 4.f); R(Balance,Head,7.f,-Side * 3.f);
        Balance.HandTarget[CatchHand] = FVector(12.f,Side * 8.f,-70.f);
        Balance.HandTarget[HurtHand] = FVector(17.f,-Side * 9.f,-60.f);
        Balance.Jaw=.16f; Balance.Grip[CatchHand]=.33f; Balance.Grip[HurtHand]=.20f;
        Balance.Crest=Role==ERole::Caster?.24f:0.f; Balance.Throat=Role==ERole::Caster?.23f:0.f;
        const float End = FMath::Max(.20f, Duration);
        const FKey Keys[] = {{0.f,Base,0.f},{End * .12f,Shock,1.f},{End * .32f,Brace,.65f},
            {End * .66f,Balance,.7f},{End * .96f,Base,.3f},{End,Base,0.f}};
        return Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
    }
    inline float FallProgress(float Time)
    {
        // A failing support accelerates into contact. Smoothstep over the
        // whole descent decelerated the living body as if it chose to sit.
        return FMath::Pow(FMath::Clamp((Time - .16f) / .43f, 0.f, 1.f), 1.55f);
    }
    inline FPose Dying(ERole Role, float Time, FVector Direction)
    {
        FPose P = Ready(Role);
        const float Side = Direction.Y >= 0.f ? 1.f : -1.f;
        const float Back = Direction.X >= 0.f ? -1.f : 1.f;
        const float Yield = FMath::SmoothStep(0.f, .16f, Time);
        const float Fall = FallProgress(Time);
        const float SupportLoss = Yield * (1.f - FMath::SmoothStep(.19f, .44f, Time));
        const float RibFall = FallProgress(Time - .045f);
        const float TorsoContact = FMath::SmoothStep(.48f, .67f, Time);
        const float HeadFall = FMath::SmoothStep(.36f, .81f, Time);
        const float ContactAge = FMath::Max(0.f, Time - .59f);
        const float BodySettle = FMath::Exp(-ContactAge * 10.f) * FMath::Sin(ContactAge * 22.f);
        const float HeadAge = FMath::Max(0.f, Time - .81f);
        const float HeadSettle = FMath::Exp(-HeadAge * 8.f) * FMath::Sin(HeadAge * 18.f);
        // A knee gives first. The ribs fold toward the surviving support and
        // the head tries to remain up; only then does the trunk tip past it.
        // The final grounded pose is retained, with a later head arrival.
        R(P,Hips,Back * (4.f * Yield + 69.f * Fall) - 11.f * SupportLoss,
            Side * (10.f * Fall + 8.f * SupportLoss),Side * (7.f * Yield + 23.f * Fall));
        R(P,Lumbar,Back * 3.f * TorsoContact - 8.f * SupportLoss,
            -Side * (2.f * Fall + 6.f * SupportLoss),Side * 2.f * Fall);
        R(P,Spine,Back * (2.f * TorsoContact + BodySettle * 2.f) - 5.f * SupportLoss,
            -Side * 5.f * SupportLoss,-Side * 3.f * SupportLoss);
        R(P,Chest,Back * (5.f * TorsoContact - BodySettle * 3.f) - 10.f * SupportLoss,
            Side * (4.f * RibFall - 8.f * SupportLoss),-Side * (3.f * Fall + 8.f * SupportLoss));
        R(P,Neck,-Back * (11.f * Yield - 18.f * HeadFall) + 17.f * SupportLoss,
            Side * (6.f * HeadFall + 9.f * SupportLoss),Side * 5.f * HeadFall);
        R(P,Head,-Back * (8.f * Yield - 12.f * HeadFall) + 11.f * SupportLoss + HeadSettle * 4.f,
            Side * (9.f * HeadFall + 6.f * SupportLoss),Side * (7.f * HeadFall - 5.f * SupportLoss));
        for (int32 S = 0; S < 2; ++S)
        {
            const float Sign = S == 0 ? -1.f : 1.f;
            const bool CatchSide = Sign == Side;
            const float Reach = CatchSide ? FMath::SmoothStep(.16f,.44f,Time) : FMath::SmoothStep(.37f,.83f,Time);
            const float ArmFall = CatchSide ? Fall : FMath::SmoothStep(.39f,.84f,Time);
            P.Joint[S == 0 ? ClavicleL : ClavicleR] = FRotator(CatchSide ? -5.f * Yield : 4.f * Fall,
                Sign * (6.f * Fall + (CatchSide ? 5.f : -7.f) * SupportLoss),Sign * (CatchSide ? 7.f : 4.f) * Yield);
            P.Joint[S == 0 ? ArmL : ArmR] = FRotator((CatchSide ? 31.f : 38.f) - Back * ArmFall * 59.f
                    + (CatchSide ? 8.f : 16.f) * SupportLoss,
                Sign * (CatchSide ? 8.f : 18.f) * Yield,Sign * (CatchSide ? 5.f : 10.f));
            R(P,S == 0 ? ForearmL : ForearmR,FMath::Lerp(CatchSide ? 52.f : 76.f,CatchSide ? 9.f : 2.f,Reach));
            R(P,S == 0 ? HandL : HandR,FMath::Lerp(CatchSide ? -8.f : -16.f,CatchSide ? 18.f : 9.f,Reach)
                + (CatchSide ? 0.f : HeadSettle * 3.f));
            P.Grip[S] = (CatchSide ? .38f : .63f) * (1.f - Reach);
        }
        P.Throat = .12f * (1.f - Fall);
        P.Jaw = .12f + .35f * Yield * (1.f - TorsoContact) + .08f * HeadFall;
        P.Crest = Role == ERole::Caster ? .5f * (1.f - FMath::SmoothStep(.18f,.92f,Time)) : 0.f;
        return P;
    }
    inline FPose Melee(float Time)
    {
        FPose Base=Ready(ERole::Melee); Carry(Base,ERole::Melee);
        FPose Notice=Base, Rise=Base, Gather=Base, Coil=Base, Drive=Base, Contact=Base,
            Cut=Base, Follow=Base, Rebound=Base, Catch=Base, Recover=Base, Settle=Base;
        Notice.Offset=FVector(3.f,-1.f,-1.f);
        R(Notice,Hips,-7.f,-2.f,1.f); R(Notice,Chest,-3.f,3.f,-1.f);
        R(Notice,Neck,5.f,-4.f); R(Notice,Head,3.f,-2.f);
        Notice.HandTarget[1]=FVector(17.f,10.f,-60.f); Notice.Grip[1]=.52f; Notice.Jaw=.18f;

        // The threat rises through the chest, then rolls into the loaded
        // right shoulder. The counter-hand stays low and close to the ribs.
        Rise.Offset=FVector(-3.f,2.f,-2.f);
        R(Rise,Hips,-5.f,5.f,-2.f); R(Rise,Lumbar,-1.f,3.f); R(Rise,Spine,2.f,4.f);
        R(Rise,Chest,6.f,8.f,-3.f); R(Rise,Neck,0.f,-10.f); R(Rise,Head,2.f,-5.f);
        R(Rise,ClavicleR,4.f,8.f,5.f); R(Rise,ClavicleL,1.f,-5.f,-2.f);
        Rise.HandTarget[1]=FVector(-4.f,28.f,-27.f); Rise.HandTarget[0]=FVector(14.f,-9.f,-61.f);
        R(Rise,HandR,-8.f,10.f,-8.f); Rise.Grip[1]=.66f; Rise.Grip[0]=.33f; Rise.Jaw=.48f;
        Gather=Rise; Gather.Offset=FVector(-7.f,4.f,-6.f);
        R(Gather,Hips,-12.f,12.f,-3.f); R(Gather,Lumbar,-5.f,7.f); R(Gather,Spine,0.f,8.f);
        R(Gather,Chest,8.f,13.f,-4.f); R(Gather,Neck,6.f,-20.f); R(Gather,Head,3.f,-11.f);
        R(Gather,ClavicleR,5.f,13.f,7.f);
        Gather.HandTarget[1]=FVector(-24.f,37.f,2.f); Gather.HandTarget[0]=FVector(19.f,-8.f,-48.f);
        R(Gather,HandR,-13.f,16.f,-10.f); Gather.Grip[1]=.73f; Gather.Grip[0]=.45f; Gather.Jaw=.57f;
        Coil=Gather; Coil.Offset=FVector(-9.f,5.f,-10.f);
        R(Coil,Hips,-17.f,15.f,-4.f); R(Coil,Lumbar,-7.f,9.f); R(Coil,Spine,1.f,10.f);
        R(Coil,Chest,7.f,14.f,-5.f); R(Coil,Neck,11.f,-24.f); R(Coil,Head,3.f,-14.f);
        Coil.HandTarget[1]=FVector(-26.f,37.f,12.f); Coil.HandTarget[0]=FVector(22.f,-6.f,-44.f);
        Coil.Grip[1]=.58f; Coil.Jaw=.43f;
        Drive=Coil; Drive.Offset=FVector(1.f,2.f,-11.f);
        R(Drive,Hips,-18.f,-8.f,-1.f); R(Drive,Lumbar,-7.f,-2.f); R(Drive,Spine,-1.f,4.f);
        R(Drive,Chest,2.f,9.f,-2.f); R(Drive,Neck,12.f,-10.f); R(Drive,Head,5.f,-7.f);
        Drive.HandTarget[1]=FVector(16.f,37.f,15.f); Drive.HandTarget[0]=FVector(1.f,-10.f,-49.f);
        R(Drive,HandR,2.f,8.f,-8.f); Drive.Grip[1]=.24f; Drive.Jaw=.70f;

        // The diagonal rake is high/outside the muzzle on entry, moves
        // THROUGH the .14-.28 damage interval, then drops across the body.
        // Wrist targets use the unchanged action clock, not head/rib delay.
        Contact.Offset=FVector(13.f,-2.f,-12.f);
        R(Contact,Hips,-17.f,-15.f,3.f); R(Contact,Lumbar,-8.f,-8.f); R(Contact,Spine,-8.f,-9.f);
        R(Contact,Chest,-12.f,-10.f,4.f); R(Contact,Neck,18.f,14.f); R(Contact,Head,11.f,9.f);
        R(Contact,ClavicleR,-5.f,-13.f,-4.f); R(Contact,ClavicleL,3.f,7.f,4.f);
        Contact.HandTarget[1]=FVector(74.f,13.f,5.f); Contact.HandTarget[0]=FVector(-22.f,-9.f,-51.f);
        R(Contact,HandR,12.f,-8.f,-5.f); R(Contact,HandL,14.f,-6.f,2.f);
        Contact.Grip[1]=.10f; Contact.Grip[0]=.54f; Contact.Jaw=.88f;
        Cut=Contact; Cut.Offset=FVector(14.f,-4.f,-13.f);
        R(Cut,Hips,-18.f,-16.f,4.f); R(Cut,Spine,-10.f,-10.f); R(Cut,Chest,-15.f,-14.f,6.f);
        R(Cut,Neck,20.f,20.f); R(Cut,Head,12.f,13.f);
        Cut.HandTarget[1]=FVector(79.f,-9.f,-18.f); Cut.HandTarget[0]=FVector(-24.f,-11.f,-57.f);
        R(Cut,HandR,24.f,-15.f,-7.f); Cut.Grip[1]=.20f; Cut.Jaw=.73f;
        Follow=Cut; Follow.Offset=FVector(10.f,-5.f,-14.f);
        R(Follow,Hips,-19.f,-12.f,4.f); R(Follow,Lumbar,-10.f,-7.f); R(Follow,Spine,-10.f,-9.f);
        R(Follow,Chest,-13.f,-15.f,6.f); R(Follow,Neck,18.f,23.f); R(Follow,Head,11.f,14.f);
        Follow.HandTarget[1]=FVector(28.f,-29.f,-65.f); Follow.HandTarget[0]=FVector(-16.f,-12.f,-65.f);
        R(Follow,HandR,29.f,-10.f,-6.f); Follow.Grip[1]=.46f; Follow.Jaw=.48f;
        Rebound=Follow; Rebound.Offset=FVector(7.f,-3.f,-12.f);
        R(Rebound,Hips,-17.f,-6.f,2.f); R(Rebound,Lumbar,-8.f,-3.f); R(Rebound,Spine,-8.f,-6.f);
        R(Rebound,Chest,-11.f,-12.f,4.f); R(Rebound,Neck,13.f,16.f); R(Rebound,Head,7.f,10.f);
        Rebound.HandTarget[1]=FVector(-5.f,-15.f,-70.f); Rebound.HandTarget[0]=FVector(10.f,-8.f,-61.f);
        Rebound.Grip[1]=.37f; Rebound.Grip[0]=.29f; Rebound.Jaw=.29f;
        Catch=Base; Catch.Offset=FVector(3.f,-2.f,-9.f);
        R(Catch,Hips,-13.f,-3.f,2.f); R(Catch,Lumbar,-7.f,2.f); R(Catch,Spine,-6.f,1.f);
        R(Catch,Chest,-8.f,4.f,-2.f); R(Catch,Neck,10.f,-5.f); R(Catch,Head,4.f,-3.f);
        Catch.HandTarget[1]=FVector(7.f,12.f,-68.f); Catch.HandTarget[0]=FVector(19.f,-9.f,-52.f);
        R(Catch,ClavicleR,2.f,6.f,5.f); Catch.Grip[1]=.17f; Catch.Jaw=.17f;
        Recover=Catch; Recover.Offset=FVector(0.f,1.f,-4.f);
        R(Recover,Hips,-8.f,3.f,-1.f); R(Recover,Lumbar,-4.f,1.f); R(Recover,Spine,-3.f,2.f);
        R(Recover,Chest,-2.f,-3.f,-2.f); R(Recover,Neck,5.f,4.f); R(Recover,Head,6.f,2.f);
        Recover.HandTarget[1]=FVector(17.f,11.f,-58.f); Recover.HandTarget[0]=FVector(8.f,-7.f,-69.f);
        Recover.Grip[1]=.46f; Recover.Grip[0]=.14f; Recover.Jaw=.10f;
        Settle=Base; Settle.Offset=FVector(2.f,.5f,-1.f);
        R(Settle,Hips,-6.f,1.f,-.5f); R(Settle,Chest,-7.f,-1.f,.5f);
        R(Settle,Neck,11.f,2.f); R(Settle,Head,8.f,-1.f);
        Settle.HandTarget[1]=FVector(10.f,8.f,-66.f); Settle.Grip[1]=.25f; Settle.Jaw=.08f;
        const FKey Keys[]={{-.88f,Base,0.f},{-.76f,Notice,.65f},{-.55f,Rise,.75f},
            {-.30f,Gather,.6f},{-.09f,Coil,.3f},{.025f,Coil,.4f},{.085f,Drive,1.f},
            {.145f,Contact,1.f},{.235f,Cut,1.f},{.39f,Follow,.8f},{.60f,Rebound,.7f},
            {.88f,Catch,.65f},{1.19f,Recover,.6f},{1.51f,Settle,.45f},{1.75f,Base,0.f}};
        return SamplePerformance(Keys,UE_ARRAY_COUNT(Keys),Time,.018f,.026f,.047f);
    }
    inline FPose BlockedPounce(float Time)
    {
        FPose Base=Ready(ERole::Hunter); Carry(Base,ERole::Hunter);
        FPose Touch=Base, Brace=Base, Recoil=Base, Withdraw=Base, Breathe=Base, Settle=Base;
        Touch.Offset=FVector(1.f,0.f,-6.f);
        R(Touch,Hips,-14.f,2.f); R(Touch,Lumbar,-9.f,-2.f); R(Touch,Chest,-17.f,3.f,-2.f);
        R(Touch,Neck,22.f,-3.f); R(Touch,Head,9.f,1.f);
        Touch.HandTarget[0]=FVector(52.f,-11.f,-24.f); Touch.HandTarget[1]=FVector(43.f,14.f,-31.f);
        Touch.Grip[0]=.14f; Touch.Grip[1]=.24f; Touch.Jaw=.61f;
        Brace=Touch; Brace.Offset=FVector(-3.f,-1.f,-11.f);
        R(Brace,Hips,-20.f,4.f,-2.f); R(Brace,Lumbar,-11.f,-3.f); R(Brace,Spine,-11.f,-2.f);
        R(Brace,Chest,-18.f,5.f,-3.f); R(Brace,Neck,26.f,-5.f); R(Brace,Head,13.f,-2.f);
        Brace.HandTarget[0]=FVector(32.f,-14.f,-29.f); Brace.HandTarget[1]=FVector(42.f,15.f,-28.f);
        R(Brace,HandL,-12.f,0.f,8.f); R(Brace,HandR,-7.f,0.f,-5.f);
        Brace.Grip[0]=.68f; Brace.Grip[1]=.49f; Brace.Jaw=.78f;
        Recoil=Brace; Recoil.Offset=FVector(-8.f,-4.f,-8.f);
        R(Recoil,Hips,-15.f,-6.f,3.f); R(Recoil,Lumbar,-7.f,3.f); R(Recoil,Spine,-4.f,3.f);
        R(Recoil,Chest,2.f,-8.f,5.f); R(Recoil,Neck,8.f,7.f); R(Recoil,Head,-4.f,4.f);
        Recoil.HandTarget[0]=FVector(11.f,-15.f,-37.f); Recoil.HandTarget[1]=FVector(29.f,16.f,-42.f);
        Recoil.Grip[0]=.52f; Recoil.Grip[1]=.72f; Recoil.Jaw=.84f;
        Withdraw=Base; Withdraw.Offset=FVector(-4.f,-3.f,-7.f);
        R(Withdraw,Hips,-17.f,-3.f,2.f); R(Withdraw,Chest,-7.f,-5.f,3.f);
        R(Withdraw,Neck,14.f,6.f); R(Withdraw,Head,10.f,3.f);
        Withdraw.HandTarget[0]=FVector(12.f,-13.f,-43.f); Withdraw.HandTarget[1]=FVector(25.f,11.f,-52.f);
        Withdraw.Grip[0]=.24f; Withdraw.Grip[1]=.49f; Withdraw.Jaw=.44f;
        Breathe=Withdraw; Breathe.Offset=FVector(-1.f,1.f,-5.f);
        R(Breathe,Hips,-14.f,2.f,-1.f); R(Breathe,Chest,-7.f,3.f,-2.f);
        R(Breathe,Neck,15.f,-4.f); R(Breathe,Head,13.f,-2.f);
        Breathe.HandTarget[0]=FVector(21.f,-8.f,-56.f); Breathe.HandTarget[1]=FVector(15.f,8.f,-62.f);
        Breathe.Jaw=.27f; Breathe.Grip[0]=.43f; Breathe.Grip[1]=.26f;
        Settle=Base; Settle.Offset+=FVector(-1.f,.5f,-1.f);
        R(Settle,Chest,-8.f,-1.f); R(Settle,Head,13.f,2.f); Settle.Jaw=.18f;
        const FKey Keys[]={{0.f,Touch,.7f},{.07f,Brace,.45f},{.18f,Recoil,.55f},
            {.38f,Withdraw,.65f},{.64f,Breathe,.7f},{.88f,Settle,.6f},{1.12f,Base,0.f}};
        return SamplePerformance(Keys,UE_ARRAY_COUNT(Keys),Time,.01f,.018f,.038f);
    }
    inline FPose Hunter(float Time)
    {
        FPose Base=Ready(ERole::Hunter); Carry(Base,ERole::Hunter);
        FPose Sight=Base, Stalk=Base, Coil=Base, Launch=Base, Flight=Base, Brace=Base,
            Land=Base, Absorb=Base, Rise=Base, Shake=Base, Settle=Base;
        Sight.Offset=FVector(7.f,-1.f,-4.f);
        R(Sight,Hips,-14.f,-3.f,1.f); R(Sight,Neck,12.f,3.f); R(Sight,Head,9.f,2.f);
        Sight.HandTarget[1]=FVector(30.f,12.f,-48.f); Sight.HandTarget[0]=FVector(13.f,-10.f,-66.f);
        Sight.Jaw=.23f; Sight.Grip[1]=.60f;
        Stalk=Sight; Stalk.Offset=FVector(-2.f,2.f,-10.f);
        R(Stalk,Hips,-21.f,6.f,-2.f); R(Stalk,Lumbar,-12.f,3.f); R(Stalk,Chest,-5.f,4.f,-2.f);
        R(Stalk,Neck,21.f,-6.f); R(Stalk,Head,14.f,-5.f);
        Stalk.HandTarget[0]=FVector(-12.f,-17.f,-54.f); Stalk.HandTarget[1]=FVector(14.f,15.f,-35.f);
        R(Stalk,ClavicleR,4.f,8.f,5.f); Stalk.Jaw=.41f; Stalk.Grip[0]=.52f; Stalk.Grip[1]=.72f;
        Coil.Offset=FVector(-8.f,2.f,-15.f);
        R(Coil,Hips,-24.f,8.f,-3.f); R(Coil,Lumbar,-14.f,4.f); R(Coil,Spine,-8.f,3.f); R(Coil,Chest,-8.f,5.f);
        R(Coil,Neck,25.f,-7.f); R(Coil,Head,24.f,-7.f);
        Arms(Coil,FRotator(-25.f,-10.f,8.f),FRotator(18.f,12.f,-8.f),-56.f,-51.f,12.f,18.f);
        Coil.HandTarget[0]=FVector(-16.f,-20.f,-48.f); Coil.HandTarget[1]=FVector(13.f,18.f,-28.f);
        R(Coil,ClavicleL,2.f,-7.f,-4.f); R(Coil,ClavicleR,5.f,10.f,6.f);
        Coil.Jaw=.32f; Coil.Grip[0]=.63f; Coil.Grip[1]=.70f;
        Launch=Coil; Launch.Offset=FVector(9.f,0.f,-4.f);
        R(Launch,Hips,-31.f,-5.f); R(Launch,Lumbar,-15.f,-3.f); R(Launch,Spine,-7.f,-4.f); R(Launch,Chest,-5.f,-7.f);
        R(Launch,Neck,28.f,8.f); R(Launch,Head,24.f,9.f);
        Launch.HandTarget[0]=FVector(12.f,-23.f,-41.f); Launch.HandTarget[1]=FVector(72.f,18.f,-12.f);
        Launch.HandWeight[0]=Launch.HandWeight[1]=1.f;
        R(Launch,ClavicleL,1.f,-4.f,-3.f); R(Launch,ClavicleR,-6.f,-10.f,-4.f);
        R(Launch,HandR,8.f,-6.f,-7.f); Launch.Jaw=.80f; Launch.Grip[0]=.30f; Launch.Grip[1]=.08f;
        Flight=Launch; Flight.Offset=FVector(12.f,0.f,0.f); R(Flight,Hips,-35.f,-7.f); R(Flight,Chest,-7.f,-8.f);
        Flight.HandTarget[0]=FVector(14.f,-22.f,-46.f); Flight.HandTarget[1]=FVector(67.f,20.f,-18.f);
        R(Flight,Neck,25.f,9.f); R(Flight,Head,20.f,5.f);
        Flight.Jaw=.67f; Flight.Grip[0]=.19f; Flight.Grip[1]=.19f;
        Brace=Flight; Brace.Offset=FVector(7.f,0.f,-3.f); R(Brace,Hips,-24.f,-3.f); R(Brace,Lumbar,-10.f);
        R(Brace,Chest,-11.f,-4.f); Brace.HandTarget[0]=FVector(22.f,-18.f,-57.f); Brace.HandTarget[1]=FVector(39.f,17.f,-51.f);
        R(Brace,Neck,20.f,5.f); R(Brace,Head,13.f,4.f);
        Brace.Jaw=.36f; Brace.Grip[0]=.05f; Brace.Grip[1]=.10f;
        Land=Brace; Land.Offset=FVector(8.f,0.f,-15.f); R(Land,Hips,-27.f); R(Land,Lumbar,-12.f); R(Land,Chest,-13.f);
        Land.HandTarget[0]=FVector(20.f,-17.f,-66.f); Land.HandTarget[1]=FVector(29.f,12.f,-64.f);
        R(Land,ClavicleL,-3.f,6.f,4.f); R(Land,ClavicleR,-4.f,-7.f,-4.f);
        Land.Jaw=.28f; Land.Grip[0]=.07f; Land.Grip[1]=.13f;
        Absorb=Land; Absorb.Offset=FVector(6.f,-2.f,-12.f);
        R(Absorb,Hips,-24.f,-4.f,2.f); R(Absorb,Lumbar,-12.f,3.f); R(Absorb,Spine,-9.f,2.f);
        R(Absorb,Chest,-14.f,4.f,-3.f); R(Absorb,Neck,23.f,-5.f); R(Absorb,Head,11.f,-4.f);
        Absorb.HandTarget[0]=FVector(17.f,-16.f,-67.f); Absorb.HandTarget[1]=FVector(23.f,15.f,-59.f);
        Absorb.Jaw=.17f; Absorb.Grip[1]=.34f;
        Rise=Base; Rise.Offset=FVector(5.f,0.f,-7.f); R(Rise,Hips,-18.f); R(Rise,Neck,23.f); R(Rise,Head,17.f);
        Rise.HandTarget[0]=FVector(14.f,-24.f,-64.f); Rise.HandTarget[1]=FVector(20.f,21.f,-65.f);
        R(Rise,Chest,-7.f,-3.f,1.f); Rise.Jaw=.12f; Rise.Grip[0]=.28f; Rise.Grip[1]=.47f;
        Shake=Base; Shake.Offset=FVector(3.f,1.f,-4.f);
        R(Shake,Hips,-14.f,2.f,-1.f); R(Shake,Chest,-5.f,-4.f,-2.f);
        R(Shake,Neck,14.f,5.f,2.f); R(Shake,Head,10.f,-3.f,-2.f);
        Shake.HandTarget[0]=FVector(23.f,-12.f,-55.f); Shake.HandTarget[1]=FVector(16.f,12.f,-64.f);
        Shake.Grip[0]=.54f; Shake.Grip[1]=.25f; Shake.Jaw=.22f;
        Settle=Base; Settle.Offset=FVector(5.f,.3f,-3.5f);
        R(Settle,Neck,16.f,-2.f); R(Settle,Head,12.f,2.f); Settle.Grip[0]=.32f;
        const FKey Keys[]={{-.78f,Base,0.f},{-.65f,Sight,.65f},{-.43f,Stalk,.65f},{-.17f,Coil,.35f},
            {-.035f,Coil,.3f},{.07f,Launch,1.f},{.22f,Flight,.8f},{.39f,Brace,1.f},
            {.49f,Land,.35f},{.62f,Absorb,.6f},{.86f,Rise,.6f},{1.13f,Shake,.6f},{1.34f,Settle,.4f},{1.48f,Base,0.f}};
        return SamplePerformance(Keys,UE_ARRAY_COUNT(Keys),Time,0.f,.018f,.036f);
    }
    inline FPose Cast(float Time, bool Boss, bool Guard=false)
    {
        const ERole Role=Boss?ERole::Boss:ERole::Caster;
        FPose Base=Ready(Role), Notice=Base, Gather=Base, Set=Base, Release=Base, Reload=Base,
            Second=Base, Exhaust=Base, Spent=Base, Regain=Base, Breathe=Base, Settle=Base;
        Notice.Offset=FVector(1.f,-3.f,-3.f);
        R(Notice,Hips,-10.f,-8.f,-2.f); R(Notice,Chest,3.f,9.f,2.f);
        R(Notice,Neck,-2.f,-8.f); R(Notice,Head,1.f,-4.f);
        R(Notice,ClavicleL,2.f,-7.f,-3.f); R(Notice,ClavicleR,2.f,8.f,4.f);
        Notice.Jaw=.20f; Notice.Crest=Boss?0.f:.44f; Notice.Throat=Boss?0.f:.32f;
        Notice.Grip[0]=.27f; Notice.Grip[1]=.40f;
        Gather.Offset=FVector(-5.f,-5.f,-6.f);
        R(Gather,Hips,-13.f,-12.f,-4.f); R(Gather,Lumbar,-5.f,5.f); R(Gather,Spine,1.f,6.f); R(Gather,Chest,10.f,13.f,5.f);
        R(Gather,Neck,0.f,-12.f); R(Gather,Head,3.f,-10.f);
        R(Gather,ClavicleL,4.f,-10.f,-5.f); R(Gather,ClavicleR,-3.f,12.f,5.f);
        Arms(Gather,FRotator(20.f,-14.f,10.f),FRotator(-32.f,18.f,-16.f),37.f,54.f,-17.f,-21.f);
        Gather.HandTarget[0]=FVector(-19.f,-18.f,-7.f); Gather.HandTarget[1]=FVector(-38.f,24.f,24.f);
        Gather.HandWeight[0]=Gather.HandWeight[1]=1.f; Gather.Throat=.8f;
        Gather.Jaw=.14f; Gather.Crest=Boss?0.f:.72f; Gather.Grip[0]=.39f; Gather.Grip[1]=.58f;
        Set=Gather; Set.Offset=FVector(-8.f,-4.f,-8.f); R(Set,Chest,13.f,16.f,5.f); R(Set,Neck,-3.f,-14.f);
        Set.HandTarget[1]=FVector(-19.f,20.f,8.f); Set.Throat=1.f;
        R(Set,HandL,-14.f,-7.f,-5.f); R(Set,HandR,-20.f,9.f,5.f);
        Set.Jaw=.06f; Set.Crest=Boss?0.f:.93f; Set.Grip[0]=.53f; Set.Grip[1]=.67f;
        Release=Set; Release.Offset=FVector(8.f,2.f,-10.f);
        R(Release,Hips,-16.f,6.f,2.f); R(Release,Lumbar,-9.f,-4.f); R(Release,Spine,-8.f,-5.f); R(Release,Chest,-14.f,-9.f,-4.f);
        R(Release,Neck,13.f,8.f); R(Release,Head,9.f,7.f); R(Release,ClavicleR,-5.f,-12.f,-5.f);
        Release.HandTarget[0]=FVector(-18.f,-17.f,-5.f); Release.HandTarget[1]=FVector(-3.f,13.f,-4.f); Release.Throat=.18f;
        R(Release,HandR,2.f,-9.f,-5.f); Release.Jaw=.76f; Release.Crest=Boss?0.f:1.f;
        Release.Grip[0]=.36f; Release.Grip[1]=.02f;
        Reload=Gather; Reload.Offset=FVector(-2.f,3.f,-7.f); R(Reload,Hips,-13.f,7.f,2.f); R(Reload,Chest,7.f,-12.f,-4.f);
        Reload.HandTarget[0]=FVector(-28.f,-22.f,14.f); Reload.HandTarget[1]=FVector(-18.f,15.f,-6.f); Reload.Throat=.85f;
        R(Reload,Neck,2.f,10.f,-2.f); R(Reload,Head,4.f,7.f,-1.f);
        R(Reload,HandL,-18.f,-8.f,-4.f); R(Reload,HandR,-5.f,6.f,3.f);
        Reload.Jaw=.16f; Reload.Crest=Boss?0.f:.74f; Reload.Grip[0]=.65f; Reload.Grip[1]=.18f;
        Second=Release; Second.Offset=FVector(7.f,-2.f,-10.f); R(Second,Hips,-16.f,-5.f,-2.f); R(Second,Chest,-13.f,10.f,4.f);
        R(Second,Neck,13.f,-9.f); R(Second,Head,9.f,-7.f); R(Second,ClavicleL,-5.f,12.f,5.f);
        Second.HandTarget[0]=FVector(-4.f,-13.f,-4.f); Second.HandTarget[1]=FVector(-19.f,17.f,-6.f);
        R(Second,HandL,3.f,8.f,4.f); Second.Jaw=.84f; Second.Crest=Boss?0.f:1.f;
        Second.Grip[0]=.02f; Second.Grip[1]=.30f;
        Exhaust=Base; Exhaust.Offset=FVector(4.f,-3.f,-12.f); R(Exhaust,Hips,-17.f,-4.f,-3.f); R(Exhaust,Lumbar,-8.f);
        R(Exhaust,Chest,-9.f,7.f); R(Exhaust,Neck,8.f,-6.f); R(Exhaust,Head,12.f);
        Exhaust.HandTarget[0]=FVector(-30.f,-24.f,-24.f); Exhaust.HandTarget[1]=FVector(-36.f,22.f,-18.f);
        Exhaust.HandWeight[0]=Exhaust.HandWeight[1]=.8f; Exhaust.Throat=.08f;
        R(Exhaust,ClavicleL,-3.f,4.f,-2.f); R(Exhaust,ClavicleR,1.f,-5.f,3.f);
        R(Exhaust,HandL,8.f,-4.f,-2.f); R(Exhaust,HandR,-4.f,6.f,3.f);
        Exhaust.Jaw=.48f; Exhaust.Crest=Boss?0.f:.45f; Exhaust.Grip[0]=.09f; Exhaust.Grip[1]=.16f;
        Spent=Exhaust; Spent.Offset=FVector(2.f,-4.f,-11.f);
        R(Spent,Hips,-16.f,-6.f,-3.f); R(Spent,Lumbar,-8.f,2.f); R(Spent,Spine,-7.f,3.f);
        R(Spent,Chest,-10.f,6.f,1.f); R(Spent,Neck,8.f,-8.f); R(Spent,Head,8.f,-4.f);
        Spent.HandTarget[0]=FVector(-41.f,-22.f,-39.f); Spent.HandTarget[1]=FVector(-29.f,20.f,-27.f);
        Spent.HandWeight[0]=.60f; Spent.HandWeight[1]=.78f;
        Spent.Jaw=.28f; Spent.Crest=Boss?0.f:.18f; Spent.Throat=.06f;
        // The spent left arm lowers before the right. The lower back starts
        // recovering while the ribs remain deflated, then a shallow breath
        // and a final shoulder release finish the actual vulnerability time.
        Regain=Spent; Regain.Offset=FVector(-1.f,1.f,-6.f);
        R(Regain,Hips,-11.f,3.f,2.f); R(Regain,Lumbar,-5.f); R(Regain,Spine,-5.f,-2.f);
        R(Regain,Chest,-2.f,-5.f,-2.f); R(Regain,Neck,6.f,4.f); R(Regain,Head,5.f,3.f);
        Regain.HandTarget[0]=FVector(-44.f,-20.f,-42.f); Regain.HandTarget[1]=FVector(-29.f,16.f,-25.f);
        Regain.HandWeight[0]=.40f; Regain.HandWeight[1]=.70f;
        Regain.Jaw=.12f; Regain.Crest=Boss?0.f:.29f; Regain.Throat=.27f;
        Regain.Grip[0]=.18f; Regain.Grip[1]=.34f;
        Breathe=Base; Breathe.Offset=FVector(-1.f,-1.f,-3.f);
        R(Breathe,Hips,-9.f,-3.f,1.f); R(Breathe,Lumbar,-3.f,2.f); R(Breathe,Spine,-2.f,-1.f);
        R(Breathe,Chest,Boss?-3.f:7.f,2.f,-1.f); R(Breathe,Neck,1.f,-2.f); R(Breathe,Head,5.f,-1.f);
        R(Breathe,ClavicleL,1.f,-4.f,-2.f); R(Breathe,ClavicleR,3.f,7.f,4.f);
        Breathe.HandTarget[0]=FVector(-48.f,-18.f,-46.f); Breathe.HandTarget[1]=FVector(-34.f,14.f,-35.f);
        Breathe.HandWeight[0]=.16f; Breathe.HandWeight[1]=.38f;
        Breathe.Jaw=.08f; Breathe.Crest=Boss?0.f:.24f; Breathe.Throat=.32f;
        Breathe.Grip[0]=.12f; Breathe.Grip[1]=.25f;
        Settle=Base; Settle.Offset+=FVector(0.f,.5f,-.8f);
        Settle.Joint[Chest].Pitch-=1.5f; Settle.Joint[Neck].Pitch+=1.f;
        Settle.HandTarget[0]=Breathe.HandTarget[0]; Settle.HandTarget[1]=FVector(-41.f,13.f,-44.f);
        Settle.HandWeight[0]=.035f; Settle.HandWeight[1]=.10f;
        Settle.Jaw=.12f; Settle.Throat=Boss?0.f:.12f; Settle.Crest=Boss?0.f:.17f;
        if (Guard)
        {
            Set.Offset=FVector(-7.f,0.f,-10.f); R(Set,Hips,-17.f); R(Set,Chest,-12.f);
            Set.HandTarget[0]=FVector(-8.f,14.f,15.f); Set.HandTarget[1]=FVector(-8.f,-14.f,30.f);
            Set.Jaw=.35f; Set.Grip[0]=.42f; Set.Grip[1]=.48f;
            FPose Resist=Set;
            Resist.Offset+=FVector(-2.f,2.f,-2.f); R(Resist,Hips,-19.f,3.f,2.f);
            R(Resist,Chest,-14.f,-3.f,-2.f); R(Resist,Neck,16.f,3.f); R(Resist,Head,10.f,2.f);
            Resist.HandTarget[0]+=FVector(-2.f,1.f,-1.f); Resist.HandTarget[1]+=FVector(-1.f,-1.f,1.f);
            Resist.Jaw=.49f; Resist.Grip[0]=.61f; Resist.Grip[1]=.55f;
            const FKey Keys[]={{-1.f,Base,0.f},{-.80f,Notice,.6f},{-.50f,Gather,.6f},{-.14f,Set,.3f},
                {.30f,Resist,.4f},{.72f,Set,.45f},{1.12f,Exhaust,.55f},{1.52f,Spent,.6f},
                {1.93f,Regain,.6f},{2.29f,Breathe,.5f},{2.55f,Base,0.f}};
            return SamplePerformance(Keys,UE_ARRAY_COUNT(Keys),Time,.014f,.022f,.040f);
        }
        if (Boss)
        {
            const FKey Keys[]={{-1.35f,Base,0.f},{-1.13f,Notice,.6f},{-.76f,Gather,.6f},{-.12f,Set,.2f},{.025f,Release,1.f},
                {.14f,Reload,.7f},{.225f,Second,1.f},{.34f,Set,.7f},{.425f,Release,1.f},
                {.73f,Exhaust,.6f},{1.08f,Spent,.6f},{1.50f,Regain,.6f},
                {1.93f,Breathe,.6f},{2.20f,Settle,.4f},{2.35f,Base,0.f}};
            return SamplePerformance(Keys,UE_ARRAY_COUNT(Keys),Time,.018f,.026f,.050f);
        }
        // MireSeer furnace: inhale into the torso, draw heat into the right
        // hand, then throw from alternating sides. Wrist contact stays on
        // the shared projectile clock; hips lead and the head follows ribs.
        // This branch does not change the guardian's salvo or interception.
        Base=CasterCarriage(false);
        Notice=Base; Notice.Offset=FVector(0.f,-2.f,-8.f);
        R(Notice,Hips,-15.f,-6.f,-2.f); R(Notice,Lumbar,-5.f,3.f,1.f);
        R(Notice,Spine,-2.f,3.f,1.f); R(Notice,Chest,6.f,6.f,2.f);
        R(Notice,Neck,4.f,-7.f); R(Notice,Head,0.f,-3.f);
        R(Notice,ClavicleL,3.f,-5.f,-4.f); R(Notice,ClavicleR,4.f,7.f,4.f);
        Notice.HandTarget[0]=FVector(-55.f,-18.f,-20.f); Notice.HandTarget[1]=FVector(-57.f,22.f,-11.f);
        Notice.HandWeight[0]=.55f; Notice.HandWeight[1]=.65f;
        Notice.Jaw=.55f; Notice.Throat=.45f; Notice.Crest=.54f;
        Notice.Grip[0]=.18f; Notice.Grip[1]=.31f;
        // The sternum opens first; both hands acknowledge the heat source
        // before the right hand draws it away from the body.
        Gather=Notice; Gather.Offset=FVector(-8.f,2.f,-11.f);
        R(Gather,Hips,-16.f,-7.f,-3.f); R(Gather,Lumbar,-5.f,4.f,1.f);
        R(Gather,Spine,0.f,7.f,1.f); R(Gather,Chest,14.f,10.f,3.f);
        R(Gather,Neck,-4.f,-14.f); R(Gather,Head,-2.f,-5.f);
        R(Gather,ClavicleL,4.f,-6.f,-5.f); R(Gather,ClavicleR,5.f,8.f,5.f);
        Gather.HandTarget[0]=FVector(-58.f,-16.f,-9.f); Gather.HandTarget[1]=FVector(-55.f,19.f,-2.f);
        Gather.HandWeight[0]=Gather.HandWeight[1]=1.f;
        R(Gather,HandL,-12.f,-5.f,-3.f); R(Gather,HandR,-16.f,8.f,5.f);
        Gather.Jaw=.40f; Gather.Throat=1.f; Gather.Crest=.88f;
        Gather.Grip[0]=.26f; Gather.Grip[1]=.50f;
        FPose DrawFire=Gather;
        DrawFire.Offset=FVector(-10.f,-1.f,-14.f);
        R(DrawFire,Hips,-18.f,-13.f,-4.f); R(DrawFire,Lumbar,-6.f,4.f,2.f);
        R(DrawFire,Spine,0.f,10.f,1.f); R(DrawFire,Chest,8.f,15.f,4.f);
        R(DrawFire,Neck,6.f,-15.f); R(DrawFire,Head,3.f,-5.f);
        R(DrawFire,ClavicleL,-2.f,5.f,-3.f); R(DrawFire,ClavicleR,6.f,12.f,-4.f);
        DrawFire.HandTarget[0]=FVector(-49.f,-20.f,-22.f); DrawFire.HandTarget[1]=FVector(-62.f,46.f,8.f);
        R(DrawFire,HandR,-18.f,14.f,7.f);
        DrawFire.Jaw=.15f; DrawFire.Throat=.86f; DrawFire.Crest=.95f;
        DrawFire.Grip[0]=.30f; DrawFire.Grip[1]=.76f;
        FPose WindRight=DrawFire;
        WindRight.Offset=FVector(-13.f,-5.f,-17.f);
        R(WindRight,Hips,-21.f,-17.f,-5.f); R(WindRight,Lumbar,-7.f,4.f,2.f);
        R(WindRight,Spine,-1.f,11.f,1.f); R(WindRight,Chest,7.f,18.f,5.f);
        R(WindRight,Neck,9.f,-15.f,-2.f); R(WindRight,Head,4.f,-4.f);
        R(WindRight,ClavicleL,-3.f,7.f,-3.f); R(WindRight,ClavicleR,7.f,14.f,-6.f);
        WindRight.HandTarget[0]=FVector(-43.f,-23.f,-29.f); WindRight.HandTarget[1]=FVector(-93.f,58.f,16.f);
        R(WindRight,HandR,-19.f,18.f,8.f);
        WindRight.Jaw=.08f; WindRight.Throat=.68f; WindRight.Crest=1.f;
        WindRight.Grip[0]=.34f; WindRight.Grip[1]=.85f;
        // The hand finishes cocking while the ribs compress toward release.
        // The counterarm stays below the face rather than sharing the orb.
        Set=WindRight; Set.Offset=FVector(-12.f,-5.f,-18.f);
        R(Set,Hips,-21.f,-12.f,-4.f); R(Set,Lumbar,-8.f,4.f,2.f);
        R(Set,Spine,-2.f,11.f,1.f); R(Set,Chest,2.f,17.f,4.f);
        R(Set,Neck,13.f,-16.f,-2.f); R(Set,Head,6.f,-4.f);
        Set.HandTarget[0]=FVector(-42.f,-24.f,-30.f); Set.HandTarget[1]=FVector(-86.f,62.f,27.f);
        Set.Throat=.58f; Set.Grip[1]=.90f;
        FPose DriveRight=Set; DriveRight.Offset=FVector(2.f,-3.f,-17.f);
        R(DriveRight,Hips,-21.f,4.f,1.f); R(DriveRight,Lumbar,-9.f,-1.f);
        R(DriveRight,Spine,-6.f,5.f); R(DriveRight,Chest,-6.f,6.f,1.f);
        R(DriveRight,Neck,16.f,-6.f); R(DriveRight,Head,7.f,-2.f);
        R(DriveRight,ClavicleR,0.f,1.f,-5.f); R(DriveRight,ClavicleL,1.f,-4.f,-2.f);
        DriveRight.HandTarget[0]=FVector(-52.f,-27.f,-29.f); DriveRight.HandTarget[1]=FVector(-39.f,48.f,22.f);
        R(DriveRight,HandR,-8.f,6.f,2.f);
        DriveRight.Jaw=.35f; DriveRight.Throat=.33f; DriveRight.Grip[1]=.62f;
        Release=DriveRight; Release.Offset=FVector(14.f,5.f,-19.f);
        R(Release,Hips,-22.f,10.f,3.f); R(Release,Lumbar,-10.f,-4.f,-1.f);
        R(Release,Spine,-10.f,-7.f,-1.f); R(Release,Chest,-17.f,-16.f,-5.f);
        R(Release,Neck,20.f,14.f,1.f); R(Release,Head,10.f,5.f);
        R(Release,ClavicleR,-6.f,-14.f,-6.f); R(Release,ClavicleL,3.f,-4.f,-2.f);
        Release.HandTarget[0]=FVector(-67.f,-27.f,-26.f); Release.HandTarget[1]=FVector(27.f,13.f,-4.f);
        R(Release,HandR,14.f,-18.f,-8.f);
        Release.Jaw=.88f; Release.Throat=.08f; Release.Crest=1.f;
        Release.Grip[0]=.48f; Release.Grip[1]=0.f;
        FPose FollowRight=Release; FollowRight.Offset=FVector(15.f,7.f,-20.f);
        R(FollowRight,Hips,-23.f,15.f,4.f); R(FollowRight,Lumbar,-10.f,-6.f,-2.f);
        R(FollowRight,Spine,-11.f,-8.f,-2.f); R(FollowRight,Chest,-18.f,-19.f,-6.f);
        R(FollowRight,Neck,18.f,17.f,2.f); R(FollowRight,Head,9.f,5.f);
        FollowRight.HandTarget[0]=FVector(-53.f,-20.f,-16.f); FollowRight.HandTarget[1]=FVector(20.f,-4.f,-20.f);
        R(FollowRight,HandR,24.f,-20.f,-9.f);
        FollowRight.Jaw=.60f; FollowRight.Throat=.12f; FollowRight.Grip[0]=.57f;
        // Recoil reloads the other side: the returning right arm hangs low
        // while the left extracts a second charge and winds outside the head.
        Reload=DrawFire; Reload.Offset=FVector(0.f,7.f,-19.f);
        R(Reload,Hips,-21.f,17.f,4.f); R(Reload,Lumbar,-8.f,-5.f,-2.f);
        R(Reload,Spine,-3.f,-9.f,-2.f); R(Reload,Chest,5.f,-14.f,-6.f);
        R(Reload,Neck,11.f,12.f,2.f); R(Reload,Head,6.f,5.f);
        R(Reload,ClavicleL,6.f,-12.f,5.f); R(Reload,ClavicleR,-2.f,3.f,3.f);
        Reload.HandTarget[0]=FVector(-62.f,-42.f,4.f); Reload.HandTarget[1]=FVector(-35.f,5.f,-40.f);
        R(Reload,HandL,-17.f,-14.f,-7.f); R(Reload,HandR,8.f,-5.f,-2.f);
        Reload.Jaw=.26f; Reload.Throat=.74f; Reload.Crest=.86f;
        Reload.Grip[0]=.74f; Reload.Grip[1]=.10f;
        FPose WindLeft=Reload; WindLeft.Offset=FVector(-9.f,8.f,-19.f);
        R(WindLeft,Hips,-21.f,16.f,4.f); R(WindLeft,Lumbar,-8.f,-5.f,-2.f);
        R(WindLeft,Spine,0.f,-10.f,-2.f); R(WindLeft,Chest,8.f,-19.f,-5.f);
        R(WindLeft,Neck,10.f,17.f,2.f); R(WindLeft,Head,5.f,4.f);
        R(WindLeft,ClavicleL,7.f,-14.f,6.f); R(WindLeft,ClavicleR,-2.f,5.f,3.f);
        WindLeft.HandTarget[0]=FVector(-90.f,-58.f,20.f); WindLeft.HandTarget[1]=FVector(-60.f,25.f,-35.f);
        R(WindLeft,HandL,-19.f,-18.f,-8.f);
        WindLeft.Jaw=.10f; WindLeft.Throat=.88f; WindLeft.Crest=.97f; WindLeft.Grip[0]=.88f;
        FPose DriveLeft=WindLeft; DriveLeft.Offset=FVector(4.f,2.f,-20.f);
        R(DriveLeft,Hips,-22.f,-2.f,-1.f); R(DriveLeft,Lumbar,-9.f,2.f,1.f);
        R(DriveLeft,Spine,-6.f,-5.f,-1.f); R(DriveLeft,Chest,-7.f,-7.f,-1.f);
        R(DriveLeft,Neck,17.f,7.f); R(DriveLeft,Head,8.f,2.f);
        R(DriveLeft,ClavicleL,0.f,-1.f,5.f); R(DriveLeft,ClavicleR,2.f,4.f,2.f);
        DriveLeft.HandTarget[0]=FVector(-42.f,-47.f,15.f); DriveLeft.HandTarget[1]=FVector(-65.f,25.f,-31.f);
        R(DriveLeft,HandL,-7.f,-6.f,-2.f);
        DriveLeft.Jaw=.39f; DriveLeft.Throat=.37f; DriveLeft.Grip[0]=.58f;
        Second=DriveLeft; Second.Offset=FVector(15.f,-6.f,-21.f);
        R(Second,Hips,-23.f,-10.f,-4.f); R(Second,Lumbar,-10.f,3.f,2.f);
        R(Second,Spine,-11.f,7.f,1.f); R(Second,Chest,-17.f,17.f,5.f);
        R(Second,Neck,21.f,-15.f,-1.f); R(Second,Head,12.f,-6.f);
        R(Second,ClavicleL,-6.f,14.f,6.f); R(Second,ClavicleR,3.f,4.f,2.f);
        Second.HandTarget[0]=FVector(27.f,-13.f,-4.f); Second.HandTarget[1]=FVector(-69.f,29.f,-31.f);
        R(Second,HandL,14.f,18.f,8.f);
        Second.Jaw=.95f; Second.Throat=.025f; Second.Crest=1.f;
        Second.Grip[0]=0.f; Second.Grip[1]=.18f;
        FPose FollowLeft=Second; FollowLeft.Offset=FVector(17.f,-8.f,-21.f);
        R(FollowLeft,Hips,-23.f,-16.f,-5.f); R(FollowLeft,Lumbar,-10.f,6.f,2.f);
        R(FollowLeft,Spine,-12.f,11.f,2.f); R(FollowLeft,Chest,-19.f,19.f,6.f);
        R(FollowLeft,Neck,19.f,-17.f,-2.f); R(FollowLeft,Head,11.f,-6.f);
        FollowLeft.HandTarget[0]=FVector(20.f,6.f,-23.f); FollowLeft.HandTarget[1]=FVector(-57.f,32.f,-38.f);
        R(FollowLeft,HandL,24.f,20.f,9.f);
        FollowLeft.Jaw=.65f; FollowLeft.Throat=.035f; FollowLeft.Crest=.70f;
        Exhaust=Base; Exhaust.Offset=FVector(9.f,-5.f,-20.f);
        R(Exhaust,Hips,-23.f,-8.f,-4.f); R(Exhaust,Lumbar,-10.f,3.f,1.f);
        R(Exhaust,Spine,-11.f,4.f,1.f); R(Exhaust,Chest,-14.f,6.f,2.f);
        R(Exhaust,Neck,17.f,-7.f); R(Exhaust,Head,8.f,-2.f);
        R(Exhaust,ClavicleL,-3.f,4.f,-2.f); R(Exhaust,ClavicleR,2.f,-5.f,4.f);
        Arms(Exhaust,FRotator(32.f,-7.f,-17.f),FRotator(41.f,12.f,16.f),14.f,33.f,8.f,-10.f);
        Exhaust.HandTarget[0]=FVector(-25.f,-31.f,-44.f); Exhaust.HandTarget[1]=FVector(-54.f,19.f,-21.f);
        Exhaust.HandWeight[0]=Exhaust.HandWeight[1]=1.f;
        Exhaust.Jaw=.45f; Exhaust.Throat=.025f; Exhaust.Crest=.42f;
        Exhaust.Grip[0]=.05f; Exhaust.Grip[1]=.20f;
        // No new pose at Attack->Recovery: the throw loses height first,
        // then a quiet asymmetric breath regains the same carried stance.
        Spent=Exhaust; Spent.Offset=FVector(2.f,-5.f,-21.f);
        R(Spent,Hips,-23.f,-7.f,-3.f); R(Spent,Lumbar,-10.f,3.f,1.f);
        R(Spent,Spine,-9.f,4.f,1.f); R(Spent,Chest,-12.f,8.f,2.f);
        R(Spent,Neck,15.f,-8.f); R(Spent,Head,7.f,-3.f);
        Spent.HandTarget[0]=FVector(-50.f,-23.f,-46.f); Spent.HandTarget[1]=FVector(-42.f,23.f,-29.f);
        Spent.HandWeight[0]=.85f; Spent.HandWeight[1]=1.f;
        Spent.Jaw=.28f; Spent.Throat=.06f; Spent.Crest=.18f; Spent.Grip[1]=.24f;
        FPose Respire=Spent; Respire.Offset=FVector(-1.f,3.f,-16.f);
        R(Respire,Hips,-18.f,4.f,3.f); R(Respire,Lumbar,-8.f,-2.f,-1.f);
        R(Respire,Spine,-8.f,-3.f,-1.f); R(Respire,Chest,-5.f,-5.f,-3.f);
        R(Respire,Neck,12.f,5.f); R(Respire,Head,7.f,3.f);
        Respire.HandTarget[0]=FVector(-53.f,-18.f,-51.f); Respire.HandTarget[1]=FVector(-34.f,26.f,-34.f);
        Respire.HandWeight[0]=.70f; Respire.HandWeight[1]=.90f;
        Respire.Jaw=.17f; Respire.Throat=.28f; Respire.Crest=.28f;
        Respire.Grip[0]=.13f; Respire.Grip[1]=.38f;
        Regain=Respire; Regain.Offset=FVector(-3.f,2.f,-11.f);
        R(Regain,Hips,-15.f,3.f,2.f); R(Regain,Lumbar,-7.f,-2.f,-1.f);
        R(Regain,Spine,-6.f,-2.f,-1.f); R(Regain,Chest,-5.f,-3.f,-2.f);
        R(Regain,Neck,10.f,4.f); R(Regain,Head,6.f,2.f);
        Regain.HandTarget[0]=FVector(-50.f,-16.f,-50.f); Regain.HandTarget[1]=FVector(-39.f,22.f,-43.f);
        Regain.HandWeight[0]=.40f; Regain.HandWeight[1]=.65f;
        Regain.Jaw=.10f; Regain.Throat=.24f; Regain.Crest=.24f;
        Regain.Grip[0]=.15f; Regain.Grip[1]=.33f;
        Breathe=Base; Breathe.Offset=FVector(3.f,-1.f,-7.f);
        R(Breathe,Hips,-13.f,-2.f,-1.f); R(Breathe,Lumbar,-5.f,1.f);
        R(Breathe,Spine,-5.f,1.f); R(Breathe,Chest,0.f,4.f,-1.f);
        R(Breathe,Neck,9.f,-2.f); R(Breathe,Head,7.f,-1.f);
        Breathe.HandTarget[0]=FVector(-51.f,-15.f,-50.f); Breathe.HandTarget[1]=FVector(-42.f,15.f,-45.f);
        Breathe.HandWeight[0]=.08f; Breathe.HandWeight[1]=.25f;
        Breathe.Jaw=.08f; Breathe.Throat=.17f; Breathe.Crest=.22f;
        Settle=Base; Settle.Offset+=FVector(.5f,0.f,-.7f);
        Settle.Joint[Chest].Pitch-=1.f; Settle.Joint[Neck].Pitch+=.5f;
        Settle.HandTarget[0]=Breathe.HandTarget[0]; Settle.HandTarget[1]=Breathe.HandTarget[1];
        Settle.HandWeight[0]=.015f; Settle.HandWeight[1]=.07f;
        Settle.Jaw=.10f; Settle.Throat=.11f; Settle.Crest=.18f;
        const FKey Keys[]={{-1.35f,Base,0.f},{-1.12f,Notice,.65f},{-.89f,Gather,.6f},
            {-.66f,DrawFire,.7f},{-.43f,WindRight,.65f},{-.18f,Set,.7f},
            {-.07f,DriveRight,1.f},{.03f,Release,1.f},{.10f,FollowRight,.85f},
            {.22f,Reload,.65f},{.34f,WindLeft,.65f},{.405f,DriveLeft,1.f},
            {.47f,Second,1.f},{.58f,FollowLeft,.85f},{.72f,Exhaust,.7f},
            {.87f,Spent,.6f},{1.12f,Respire,.65f},{1.45f,Regain,.7f},
            {1.78f,Breathe,.6f},{1.98f,Settle,.4f},{2.09f,Base,0.f}};
        return SamplePerformance(Keys,UE_ARRAY_COUNT(Keys),Time,.020f,.026f,.046f);
    }
    inline FPose Ground(float Time, bool Slam)
    {
        FPose Base=Ready(ERole::Boss), Notice=Base, Gather=Base, Lift=Base, High=Base, Drive=Base,
            Impact=Base, Hold=Base, Rise=Base, Recenter=Base, Breathe=Base, Settle=Base;
        Notice.Offset=FVector(3.f,-2.f,-3.f);
        R(Notice,Hips,-8.f,-3.f,2.f); R(Notice,Lumbar,-4.f,2.f); R(Notice,Chest,-3.f,3.f,-2.f);
        R(Notice,Neck,4.f,-3.f); R(Notice,Head,1.f,-2.f);
        R(Notice,ClavicleL,3.f,-6.f,-4.f); R(Notice,ClavicleR,1.f,8.f,3.f);
        Notice.Jaw=.24f; Notice.Grip[0]=.46f; Notice.Grip[1]=.58f;
        Gather.Offset=FVector(-5.f,0.f,-10.f); R(Gather,Hips,-15.f); R(Gather,Lumbar,-8.f); R(Gather,Spine,-4.f);
        R(Gather,Chest,6.f,4.f,-2.f); R(Gather,Neck,12.f,-4.f); R(Gather,Head,7.f,-2.f);
        Arms(Gather,FRotator(-68.f,-8.f,-6.f),FRotator(-43.f,12.f,5.f),-39.f,-44.f,6.f,14.f);
        Gather.Jaw=.56f; Gather.Grip[0]=.68f; Gather.Grip[1]=.72f;
        Lift=Gather; Lift.Offset=FVector(-8.f,2.f,-8.f);
        R(Lift,Hips,-3.f,4.f,-2.f); R(Lift,Lumbar,-2.f,3.f); R(Lift,Spine,3.f,2.f);
        R(Lift,Chest,10.f,-3.f,1.f); R(Lift,Neck,-2.f,-2.f); R(Lift,Head,-5.f,2.f);
        R(Lift,ClavicleL,6.f,-9.f,-6.f); R(Lift,ClavicleR,3.f,7.f,4.f);
        Arms(Lift,FRotator(-109.f,-8.f,-5.f),FRotator(-94.f,11.f,5.f),-23.f,-32.f,-6.f,-1.f);
        Lift.Jaw=.78f; Lift.Grip[0]=.54f; Lift.Grip[1]=.64f;
        High.Offset=FVector(-7.f,0.f,-6.f); R(High,Hips,7.f); R(High,Lumbar,0.f); R(High,Spine,4.f); R(High,Chest,10.f);
        R(High,Neck,-7.f); R(High,Head,-10.f); R(High,ClavicleL,5.f,-8.f,-6.f); R(High,ClavicleR,5.f,8.f,6.f);
        Arms(High,FRotator(-112.f,-7.f,-4.f),FRotator(-119.f,8.f,4.f),-20.f,-17.f,-8.f,-11.f);
        High.Jaw=.62f; High.Grip[0]=.34f; High.Grip[1]=.40f;
        Drive=High; Drive.Offset=FVector(3.f,0.f,-14.f); R(Drive,Hips,-17.f); R(Drive,Lumbar,-9.f); R(Drive,Spine,-7.f);
        R(Drive,Chest,-4.f); R(Drive,Neck,10.f); R(Drive,Head,5.f);
        Drive.Jaw=.92f; Drive.Grip[0]=Drive.Grip[1]=0.f;
        Impact=Base; Impact.Offset=FVector(8.f,0.f,-25.f);
        R(Impact,Hips,-22.f); R(Impact,Lumbar,-13.f); R(Impact,Spine,-14.f); R(Impact,Chest,-13.f);
        R(Impact,Neck,16.f); R(Impact,Head,11.f); R(Impact,ClavicleL,-4.f,9.f,4.f); R(Impact,ClavicleR,-4.f,-9.f,-4.f);
        Arms(Impact,FRotator(42.f,-6.f,0.f),FRotator(45.f,6.f,0.f),-33.f,-31.f,24.f,24.f);
        Impact.Jaw=.75f; Impact.Grip[0]=Impact.Grip[1]=0.f;
        Hold=Impact; Hold.Offset.Z=-27.f; R(Hold,Neck,12.f); R(Hold,Head,7.f);
        Hold.Jaw=.43f;
        Rise=Base; Rise.Offset=FVector(5.f,-3.f,-14.f); R(Rise,Hips,-16.f,4.f,-3.f); R(Rise,Lumbar,-9.f);
        R(Rise,Chest,-13.f,-4.f); R(Rise,Neck,18.f); R(Rise,Head,12.f);
        R(Rise,ClavicleL,1.f,-4.f,-2.f); R(Rise,ClavicleR,4.f,8.f,5.f);
        Arms(Rise,FRotator(43.f,-4.f,-20.f),FRotator(48.f,5.f,21.f),7.f,15.f,8.f,10.f);
        Rise.Jaw=.20f; Rise.Grip[0]=.12f; Rise.Grip[1]=.24f;
        Recenter=Base; Recenter.Offset=FVector(0.f,2.f,-8.f);
        R(Recenter,Hips,-11.f,-3.f,2.f); R(Recenter,Lumbar,-5.f,-2.f); R(Recenter,Spine,-4.f,2.f);
        R(Recenter,Chest,-4.f,4.f,-2.f); R(Recenter,Neck,7.f,-4.f); R(Recenter,Head,4.f,-2.f);
        R(Recenter,ClavicleL,1.f,-4.f,-3.f); R(Recenter,ClavicleR,3.f,7.f,5.f);
        Arms(Recenter,FRotator(27.f,-5.f,-20.f),FRotator(37.f,8.f,19.f),5.f,15.f,9.f,4.f);
        Recenter.Jaw=.12f; Recenter.Grip[0]=.24f; Recenter.Grip[1]=.43f;
        Breathe=Base; Breathe.Offset=FVector(-1.f,1.f,-3.f);
        R(Breathe,Hips,-7.f,-2.f,1.f); R(Breathe,Lumbar,-3.f,1.f); R(Breathe,Spine,-2.f,1.f);
        R(Breathe,Chest,-2.f,-2.f,-1.f); R(Breathe,Neck,3.f,3.f); R(Breathe,Head,5.f,1.f);
        Arms(Breathe,FRotator(17.f,-3.f,-21.f),FRotator(25.f,5.f,20.f),6.f,12.f,7.f,5.f);
        Breathe.Jaw=.07f; Breathe.Grip[0]=.18f; Breathe.Grip[1]=.27f;
        Settle=Base; Settle.Offset+=FVector(.5f,-.5f,-.7f);
        R(Settle,Chest,-9.f,1.f,.5f); R(Settle,Neck,12.f,-1.f); R(Settle,Head,8.f,1.f);
        Settle.Jaw=.13f; Settle.Grip[1]=.34f;
        if (Slam)
        {
            // The former windup stopped both claws behind shoulder height.
            // Draw them forward and above the crown with flexed elbows. The
            // left shoulder takes the load first; the right catches as the
            // ribs open over support.
            R(Gather,ClavicleL,6.f,-5.f,4.f); R(Gather,ClavicleR,1.f,5.f,-1.f);
            Arms(Gather,FRotator(72.f,-13.f,-17.f),FRotator(52.f,15.f,18.f),42.f,48.f,6.f,14.f);
            Lift.Offset.Z=-5.f;
            R(Lift,Spine,1.f,4.f); R(Lift,Chest,9.f,-5.f,-3.f);
            R(Lift,Neck,-3.f,-3.f,2.f); R(Lift,Head,-2.f,1.f);
            R(Lift,ClavicleL,7.f,-8.f,7.f); R(Lift,ClavicleR,2.f,7.f,-3.f);
            Arms(Lift,FRotator(125.f,-11.f,-19.f),FRotator(100.f,13.f,17.f),35.f,42.f,-6.f,-1.f);
            High.Offset.Z=-1.f;
            R(High,Hips,1.f); R(High,Lumbar,2.f); R(High,Spine,5.f,-2.f);
            R(High,Chest,13.f,2.f,1.f); R(High,Neck,-12.f,-1.f,-1.f); R(High,Head,-7.f);
            R(High,ClavicleL,4.f,-5.f,8.f); R(High,ClavicleR,6.f,5.f,-8.f);
            Arms(High,FRotator(158.f,-12.f,-16.f),FRotator(166.f,10.f,16.f),28.f,36.f,-8.f,-11.f);
            // Equivalent Drive rotations keep the last pre-attack samples on
            // the raised arc, including frames faster than the 30Hz study.
            Drive.Joint[ArmL].Pitch+=360.f; Drive.Joint[ArmR].Pitch+=360.f;
            // Drive's body, Impact and contact timing stay unchanged. The
            // world-space descent starts from the actual raised hand pose.
        }
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
            Lift=High; Lift.Offset=FVector(-8.f,3.f,-11.f);
            R(Lift,Hips,-12.f,10.f,-3.f); R(Lift,Chest,5.f,10.f,4.f);
            Lift.HandTarget[1]=FVector(-24.f,24.f,9.f); Lift.Jaw=.70f; Lift.Grip[1]=.63f;
            High.Jaw=.54f; High.Grip[1]=.44f;
            Drive.Jaw=.83f; Drive.Grip[1]=.18f;
            Impact.Jaw=.75f; Impact.Grip[1]=.08f;
            Hold.Grip[1]=.33f; Hold.Jaw=.42f;
            Recenter.HandTarget[1]=FVector(23.f,15.f,-57.f); Recenter.HandWeight[1]=.38f;
            Breathe.HandTarget[1]=FVector(15.f,11.f,-64.f); Breathe.HandWeight[1]=.18f;
            Settle.HandTarget[1]=FVector(10.f,8.f,-66.f); Settle.HandWeight[1]=.04f;
        }
        const FKey Keys[]={{Slam?-1.1f:-1.5f,Base,0.f},{Slam?-.94f:-1.29f,Notice,.6f},
            {Slam?-.73f:-1.f,Gather,.65f},{-.40f,Lift,.65f},{-.18f,High,.3f},
            {-.025f,High,0.f},{.075f,Drive,1.f},{.17f,Impact,1.f},{.32f,Hold,.1f},
            {.57f,Hold,0.f},{.94f,Rise,.65f},{1.32f,Recenter,.65f},
            {1.74f,Breathe,.6f},{2.08f,Settle,.45f},{2.29f,Base,0.f}};
        FPose P=Sample(Keys,UE_ARRAY_COUNT(Keys),Time);
        if (Slam)
        {
            // Floor targets were measured with open/rest digits. The same
            // geometry stays fixed through the impact and early release;
            // flex resumes only after the hands have begun leaving ground.
            const float Open=FMath::SmoothStep(.025f,.105f,Time);
            const float ReleaseGrip=FMath::SmoothStep(.64f,.96f,Time);
            for (int32 S=0;S<2;++S) P.Grip[S]*=1.f-Open*(1.f-ReleaseGrip);
            const FPose HeadFollow=Sample(Keys,UE_ARRAY_COUNT(Keys),Time-.045f);
            P.Joint[Neck]=HeadFollow.Joint[Neck]; P.Joint[Head]=HeadFollow.Joint[Head];
            // The established hip/rib descent is kept on the contact clock.
            // A delayed head and one-sided rise supply overlap around it.
            return P;
        }
        return SamplePerformance(Keys,UE_ARRAY_COUNT(Keys),Time,.015f,.024f,.047f);
    }
}
