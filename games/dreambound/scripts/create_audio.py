"""Original layered beta effects. No samples or third-party input."""
from pathlib import Path
import math, random, wave, struct
OUT=Path(__file__).resolve().parents[1]/"assets"/"audio"
OUT.mkdir(parents=True,exist_ok=True)
RATE=44100
specs={
"S_Attack":(.19,420,.14,"shot"),"S_Guard":(.24,150,.22,"metal"),
"S_Parry":(.60,820,.10,"chime"),"S_Dash":(.32,85,.6,"air"),
"S_Impact":(.48,63,.4,"impact"),"S_Equip":(1.05,310,.11,"chime"),
"S_Hurt":(.31,110,.5,"impact"),"S_Encounter":(1.8,82,.08,"bell"),
"S_Clear":(2.2,220,.06,"chord"),"S_EnemyFire":(.32,250,.3,"shot"),
"S_EnemyHit":(.22,95,.4,"metal"),"S_BossTell":(1.4,65,.1,"bell")}
for name,(duration,freq,noise,kind) in specs.items():
    rng=random.Random(name);samples=[];prev=0.
    for i in range(int(duration*RATE)):
        t=i/RATE;p=t/duration;n=rng.uniform(-1,1);prev=.78*prev+.22*n
        env=min(1,t/.007)*math.exp(-p*(3.8 if kind!="bell" else 2.7))*(1-p)**.35
        pitch=freq*(1+(.7*math.exp(-t*45) if kind in ("shot","impact") else 0))
        tone=math.sin(2*math.pi*pitch*t)*.6
        if kind in ("metal","bell","chime"):
            tone+=.24*math.sin(2*math.pi*freq*2.756*t)*math.exp(-t*7)+.15*math.sin(2*math.pi*freq*5.407*t)*math.exp(-t*12)
        elif kind=="chord":
            tone+=.35*math.sin(2*math.pi*freq*1.25*t)+.25*math.sin(2*math.pi*freq*1.5*t)
        elif kind=="air":tone*=.15
        elif kind=="impact":tone+=.4*math.sin(2*math.pi*42*t)*math.exp(-t*9)
        samples.append(math.tanh((tone+noise*(prev if kind=="air" else n))*env*1.1)*.63)
    peak=max(abs(v) for v in samples) or 1
    with wave.open(str(OUT/(name+".wav")),"wb") as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h",int(v/peak*.72*32767)) for v in samples))
print(f"Created {len(specs)} original WAV effects in {OUT}")
