#pragma once
struct FExpeditionRuntime;

// Explicit -VisualAudit only. Call after profile parsing, before Load; a true
// result means skip Load. Hook Tick at the end of Expedition Tick.
bool InitializeVisualCaptureProbe(FExpeditionRuntime& Runtime);
void TickVisualCaptureProbe(FExpeditionRuntime& Runtime,float Delta);
