#include "FoundrySession.h"

#include <algorithm>

DEFINE_LOG_CATEGORY_STATIC(LogFoundryCore, Log, All);

namespace
{
FString Text(const std::string& Value) { return UTF8_TO_TCHAR(Value.c_str()); }
bool Living(const overkill::Enemy& Enemy) { return !Enemy.dead && !Enemy.escaped; }
}

FFoundrySession::FFoundrySession(uint64 InSeed) : Seed(InSeed) { Restart(); }

void FFoundrySession::Restart()
{
    State = overkill::Rules::teachingEncounter(Seed, true);
    Selection.clear();
    SpreadTargets.Empty();
    Target = State.enemies.empty() ? 0 : State.enemies.front().id;
    InventoryPage = 0;
    RecentEvents.Empty();
    CommittedEvents.clear();
    Message = TEXT("Mara / Mite + Ram technical encounter. Choose steering, then Collect.");
    LastAction = TEXT("restart");
    UE_LOG(LogFoundryCore, Display, TEXT("New technical encounter seed=%llu hash=%s"), Seed, *Text(overkill::stateHash(State)));
}

void FFoundrySession::FixSelection()
{
    Selection.erase(std::remove_if(Selection.begin(), Selection.end(), [&](uint64 Id)
    {
        for (const auto& Part : State.parts) if (Part.id == Id && Part.place == overkill::Place::Reserve) return false;
        return true;
    }), Selection.end());
    bool bFoundTarget = false;
    for (const auto& Enemy : State.enemies) if (Enemy.id == Target && Living(Enemy)) bFoundTarget = true;
    if (!bFoundTarget) for (const auto& Enemy : State.enemies) if (Living(Enemy)) { Target = Enemy.id; break; }
    for (const auto& Part : State.parts)
    {
        if (Part.kind != overkill::Kind::Spread || Part.place != overkill::Place::Loaded || SpreadTargets.Contains(Part.id)) continue;
        for (const auto& Enemy : State.enemies) if (Living(Enemy) && Enemy.id != Target) { SpreadTargets.Add(Part.id, Enemy.id); break; }
    }
}

bool FFoundrySession::Submit(const overkill::Action& Action, const FString& Description)
{
    const overkill::Result Result = Rules.apply(State, Action);
    LastAction = Description;
    if (!Result.ok)
    {
        Message = Description + TEXT(": ") + Text(Result.reason);
        UE_LOG(LogFoundryCore, Display, TEXT("REJECTED %s"), *Message);
        return false;
    }
    Message = Description + TEXT(" committed.");
    CommittedEvents.insert(CommittedEvents.end(), Result.events.begin(), Result.events.end());
    for (const auto& Event : Result.events)
    {
        const FString Json = Text(overkill::eventJson(Event));
        UE_LOG(LogFoundryCore, Display, TEXT("EVENT %s"), *Json);
        FString Line = Text(Event.type);
        if (Event.target) Line += TEXT(" / ") + NameFor(Event.target);
        if (Event.type == "hit" || Event.type == "status_damage" || Event.type == "player_damage")
            Line += FString::Printf(TEXT(" / %d HP, %d Shield"), Event.amount, Event.secondary);
        else if (Event.type == "fire") Line += FString::Printf(TEXT(" / shot %d, Heat %d"), Event.amount, Event.secondary);
        else if (Event.amount || Event.secondary) Line += FString::Printf(TEXT(" / value %d, auxiliary %d"), Event.amount, Event.secondary);
        RecentEvents.Add(Line);
    }
    while (RecentEvents.Num() > 8) RecentEvents.RemoveAt(0);
    FixSelection();
    UE_LOG(LogFoundryCore, Display, TEXT("ACTION %s hash=%s"), *Description, *Text(overkill::stateHash(State)));
    return true;
}

overkill::Action FFoundrySession::FireAction() const
{
    std::vector<overkill::SpreadTarget> Spreads;
    for (const auto& Part : State.parts)
    {
        if (Part.kind == overkill::Kind::Spread && Part.place == overkill::Place::Loaded)
        {
            if (const uint64* Choice = SpreadTargets.Find(Part.id)) Spreads.push_back({Part.id, *Choice});
        }
    }
    return overkill::Action::fire(Target, std::move(Spreads));
}

overkill::Preview FFoundrySession::Preview(const overkill::Action& Action) const { return Rules.preview(State, Action); }

FString FFoundrySession::PreviewText(const overkill::Action& Action) const
{
    const auto Result = Preview(Action);
    if (!Result.result.ok) return Text(Result.result.reason);
    FString Detail = TEXT("Ready");
    for (const auto& Event : Result.result.events)
    {
        if (Event.type == "hit" || Event.type == "status_damage")
            Detail += FString::Printf(TEXT(" | %s: %d HP, %d Shield"), *NameFor(Event.target), Event.amount, Event.secondary);
        else if (Event.type == "player_damage")
            Detail += FString::Printf(TEXT(" | Mara: %d HP, %d Shield"), Event.amount, Event.secondary);
        else if (Event.type == "victory") Detail += TEXT(" | Victory");
        else if (Event.type == "defeat") Detail += TEXT(" | Defeat");
    }
    return Detail;
}

FString FFoundrySession::NameFor(uint64 Id) const
{
    for (const auto& Enemy : State.enemies) if (Enemy.id == Id) return Text(Enemy.name);
    for (const auto& Part : State.parts) if (Part.id == Id) return Text(Part.output);
    return FString::Printf(TEXT("#%llu"), Id);
}

FString FFoundrySession::DescribePart(const overkill::Part& Part) const
{
    FString Place;
    switch (Part.place)
    {
    case overkill::Place::Installed: Place = TEXT("Installed"); break;
    case overkill::Place::Loaded: Place = TEXT("Loaded"); break;
    case overkill::Place::Fitted: Place = TEXT("Fitted"); break;
    default: Place = TEXT("Reserve"); break;
    }
    FString Result = FString::Printf(TEXT("#%llu %s / %s"), Part.id, *Text(Part.output), *Place);
    if (Part.kind == overkill::Kind::Shield && Part.everInstalled) Result += FString::Printf(TEXT(" / %d Shield left"), Part.shield);
    if (Part.createdRound < State.round) Result += TEXT(" / saved");
    return Result;
}

bool FFoundrySession::Control(const FString& Command)
{
    FString Verb, Value;
    if (!Command.Split(TEXT(":"), &Verb, &Value)) Verb = Command;
    const uint64 Id = FCString::Strtoui64(*Value, nullptr, 10);
    if (Verb == TEXT("collect")) return Submit(overkill::Action::collect(Steering, Precision), TEXT("collect"));
    if (Verb == TEXT("steer")) { Steering = (Steering + 1) % 5; return false; }
    if (Verb == TEXT("precision")) { Precision = Precision == 2 ? -1 : Precision + 1; return false; }
    if (Verb == TEXT("craft")) return Submit(overkill::Action::craft(Id), TEXT("craft"));
    if (Verb == TEXT("load")) return Submit(overkill::Action::load(Selection), TEXT("load"));
    if (Verb == TEXT("unload")) { overkill::Action Action; Action.type = overkill::ActionType::Unload; return Submit(Action, TEXT("unload")); }
    if (Verb == TEXT("fire")) return Submit(FireAction(), TEXT("fire"));
    if (Verb == TEXT("end")) return Submit(overkill::Action::endTurn(), TEXT("end turn"));
    if (Verb == TEXT("restart")) { Restart(); return true; }
    if (Verb == TEXT("panels")) { bShowPanels = !bShowPanels; return false; }
    if (Verb == TEXT("target"))
    {
        for (const auto& Enemy : State.enemies)
            if (Enemy.id == Id && Living(Enemy)) { Target = Id; Message = TEXT("Main target: ") + NameFor(Id); return false; }
        Message = TEXT("Choose an enemy that is still in the fight.");
        return false;
    }
    if (Verb == TEXT("page")) { InventoryPage = FMath::Max(0, InventoryPage + FCString::Atoi(*Value)); return false; }
    if (Verb == TEXT("spread"))
    {
        TArray<uint64> Candidates;
        for (const auto& Enemy : State.enemies) if (Living(Enemy) && Enemy.id != Target) Candidates.Add(Enemy.id);
        if (!Candidates.IsEmpty())
        {
            const uint64 Current = SpreadTargets.FindRef(Id);
            const int32 Index = Candidates.IndexOfByKey(Current);
            SpreadTargets.Add(Id, Candidates[(Index + 1) % Candidates.Num()]);
            Message = TEXT("Spread target: ") + NameFor(SpreadTargets[Id]);
        }
        return false;
    }
    if (Verb == TEXT("part"))
    {
        for (const auto& Part : State.parts)
        {
            if (Part.id != Id) continue;
            if (Part.kind == overkill::Kind::Shield)
                return Submit(Part.place == overkill::Place::Installed ? overkill::Action::remove(Id) : overkill::Action::install(Id), TEXT("Shield installation"));
            if (Part.kind == overkill::Kind::Magnet || Part.kind == overkill::Kind::Modifier)
            {
                overkill::Action Action; Action.type = overkill::ActionType::Activate; Action.subject = Id;
                return Submit(Action, TEXT("activate part"));
            }
            if (Part.place == overkill::Place::Loaded) { Message = TEXT("Unload first to change loaded parts."); return false; }
            const auto Found = std::find(Selection.begin(), Selection.end(), Id);
            if (Found == Selection.end()) Selection.push_back(Id); else Selection.erase(Found);
            Message = FString::Printf(TEXT("%llu bullet parts selected. Load commits this selection."), static_cast<uint64>(Selection.size()));
            return false;
        }
    }
    return false;
}

bool FFoundrySession::RunInputProbe(FString& Report)
{
    Restart();
    const std::string Original = overkill::stateHash(State);
    const auto Reject = [&Report](const TCHAR* Step) { Report = FString(TEXT("Adapter input probe failed at ")) + Step; return false; };
    const auto CopyFor = [&](const char* Recipe) -> uint64
    {
        for (const auto& Copy : State.memory) if (Copy.recipe == Recipe) return Copy.id;
        return 0;
    };
    const auto PartFor = [&](const char* Recipe) -> uint64
    {
        for (const auto& Part : State.parts) if (Part.recipe == Recipe) return Part.id;
        return 0;
    };
    if (Control(TEXT("fire")) || overkill::stateHash(State) != Original) return Reject(TEXT("invalid Fire rollback"));
    Steering = 3;
    Precision = -1;
    if (!Control(TEXT("collect"))) return Reject(TEXT("collection"));
    const std::string Collected = overkill::stateHash(State);
    if (Control(TEXT("collect")) || overkill::stateHash(State) != Collected) return Reject(TEXT("duplicate collection rejection"));
    if (!Control(FString::Printf(TEXT("craft:%llu"), CopyFor("SH002")))) return Reject(TEXT("Shield crafting"));
    const uint64 Shield = PartFor("SH002");
    if (!Control(FString::Printf(TEXT("part:%llu"), Shield))) return Reject(TEXT("Shield install"));
    if (!Control(FString::Printf(TEXT("part:%llu"), Shield))) return Reject(TEXT("Shield remove"));
    if (!Control(FString::Printf(TEXT("part:%llu"), Shield))) return Reject(TEXT("Shield reinstall"));
    if (!Control(FString::Printf(TEXT("craft:%llu"), CopyFor("SH004")))) return Reject(TEXT("ammunition crafting"));
    const uint64 Ammo = PartFor("SH004");
    Control(FString::Printf(TEXT("part:%llu"), Ammo));
    if (!Control(TEXT("load"))) return Reject(TEXT("load selected"));
    if (!Control(TEXT("unload"))) return Reject(TEXT("unload"));
    Control(FString::Printf(TEXT("part:%llu"), Ammo));
    if (!Control(TEXT("load"))) return Reject(TEXT("reload"));
    Control(FString::Printf(TEXT("target:%llu"), State.enemies.front().id));
    const int32 RoundBeforeFire = State.round;
    const auto Previewed = Preview(FireAction());
    if (!Previewed.result.ok || !Control(TEXT("fire"))) return Reject(TEXT("Fire"));
    if (overkill::stateHash(State) != overkill::stateHash(Previewed.state)) return Reject(TEXT("preview/apply parity"));
    if (State.round != RoundBeforeFire) return Reject(TEXT("Fire must preserve player round"));
    if (!Control(TEXT("end")) || State.round <= RoundBeforeFire) return Reject(TEXT("End Turn"));
    Control(TEXT("restart"));
    if (overkill::stateHash(State) != Original) return Reject(TEXT("same-seed restart"));
    Report = TEXT("Commands: rejected Fire, Collect/reject duplicate, craft, install/remove/reinstall, select/load/unload/reload, target, preview/Fire, End Turn, same-seed restart.");
    return true;
}
