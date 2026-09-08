"""Original physical-contact effects: no samples, external service or preset."""
from pathlib import Path
import hashlib, json, wave
import numpy as np
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'assets'/'audio';OUT.mkdir(parents=True,exist_ok=True)
RATE=48000

def noise(rng,n,low=40,high=8000,slope=-.35):
    f=np.fft.rfftfreq(n,1/RATE)
    w=((f>=low)&(f<=high))*np.maximum(f,low)**slope
    v=np.fft.irfft(np.fft.rfft(rng.normal(size=n))*w,n)
    return v/max(np.std(v),1e-9)

def modes(t,root,decay,count=19):
    ratios=[1,1.47,2.09,2.78,3.63,4.91,6.1,7.52,9.13,11.2,13.46,16.51,20.2,24.5,29.1,34.2,39.9,45.7,52.3]
    v=np.zeros_like(t)
    for i,r in enumerate(ratios[:count]):
        v+=np.sin(2*np.pi*root*r*t+i*.37)*np.exp(-t*decay*(1+r*.14))/(1+i*.45)
    return v

def contact(t,rng,root=96,weight=1):
    thud=(np.sin(2*np.pi*57*t)+.42*np.sin(2*np.pi*109*t))*np.exp(-t*15)
    grit=noise(rng,len(t),400,10500,-.2)*np.exp(-t*77)
    scrape=noise(rng,len(t),700,6500,-.4)*np.exp(-t*16)*np.sin(t*240)**2
    return (1-np.exp(-t*1400))*(thud*.7*weight+modes(t,root,5.5)*.24+grit*.30+scrape*.08)

def delay(v,seconds,gain):
    offset=int(seconds*RATE);out=np.zeros_like(v)
    if 0<offset<len(v):out[offset:]=v[:-offset]*gain
    return out

SPECS={
 'S_Attack':('throw',.62),'S_Guard':('guard',.52),'S_Parry':('parry',1.05),
 'S_Dash':('dash',.46),'S_Impact':('impact',.92),'S_Catch':('catch',.78),
 'S_Recall':('recall',.75),'S_Equip':('equip',1.6),'S_Hurt':('hurt',.5),
 'S_Encounter':('bell',2.5),'S_Clear':('clear',3.),'S_EnemyFire':('enemy_fire',.58),
 'S_EnemyHit':('enemy_hit',.58),'S_BossTell':('warning',1.3),
 'S_EnemyTell':('tell',.7),'S_EnemyDefeat':('defeat',1.3)}
report=[]
for name,(kind,duration) in SPECS.items():
    seed=int.from_bytes(hashlib.sha256(name.encode()).digest()[:8],'little')
    rng=np.random.default_rng(seed);t=np.arange(int(duration*RATE))/RATE
    air=noise(rng,len(t),90,7500,-.65)
    if kind=='throw':
        env=np.sin(np.pi*np.minimum(t/.28,1))**1.6*(t<.28)
        x=.28*air*env+.47*contact(t,rng,88,.65)+delay(contact(t,rng,138,.35),.042,.4)
    elif kind=='guard':
        x=contact(t,rng,124,.8)*.85+delay(contact(t,rng,191,.4),.055,.38)
    elif kind in ('impact','parry','enemy_hit','catch'):
        root={'impact':74,'parry':152,'enemy_hit':101,'catch':83}[kind]
        x=contact(t,rng,root,1.15 if kind in ('impact','catch') else .8)
        if kind=='parry':x+=modes(t,376,2.8,10)*.19*(1-np.exp(-t*700))
        if kind=='catch':x+=delay(contact(t,rng,173,.4),.048,.34)+delay(contact(t,rng,226,.3),.093,.20)
    elif kind in ('dash','recall'):
        env=np.sin(np.pi*np.clip(t/duration,0,1))**(1.2 if kind=='recall' else .7)
        motor=noise(rng,len(t),140,1200,-.8)*(1+.32*np.sin(t*2*np.pi*47))
        x=(.38*air+.20*motor)*env
        if kind=='dash':x+=np.sin(t*2*np.pi*62)*np.exp(-t*12)*.17
    elif kind=='hurt':x=contact(t,rng,54,1.1)*.75+air*np.exp(-t*20)*.11
    elif kind=='enemy_fire':x=contact(t,rng,133,.7)*.65+noise(rng,len(t),400,3600,-.4)*np.exp(-t*10)*.13
    elif kind in ('tell','warning'):
        env=(1-np.exp(-t*15))*np.exp(-t*3.3)
        x=modes(t,58 if kind=='warning' else 116,2.6,13)*env*.28
        x+=noise(rng,len(t),110,1600,-.7)*env*.12+delay(contact(t,rng,211,.4),.16,.2)
    elif kind=='defeat':
        x=contact(t,rng,65,1.2)
        for d,g,r in ((.17,.65,129),(.33,.43,203),(.53,.28,147),(.76,.16,291)):x+=delay(contact(t,rng,r,.55),d,g)
    else:
        root={'bell':74,'clear':147,'equip':196}[kind]
        x=modes(t,root,1.8 if kind!='equip' else 3.2)* (1-np.exp(-t*320))*.23
        if kind in ('clear','equip'):x+=delay(modes(t,root*1.5,1.9,11),.13,.12)
        x+=contact(t,rng,173,.45)*.15
    x+=delay(x.copy(),.043,.10)+delay(x.copy(),.087,.07)
    x-=x.mean()
    x*=np.minimum(1,t/.002)*np.minimum(1,np.maximum(duration-t,0)/.035)
    x=np.tanh(x*1.25);x*=.84/max(np.max(np.abs(x)),1e-9)
    pcm=np.clip(x*32767,-32768,32767).astype('<i2')
    with wave.open(str(OUT/(name+'.wav')),'wb') as w:
        w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(pcm.tobytes())
    report.append({'asset':name,'seconds':duration,'peak_dbfs':round(20*np.log10(np.max(np.abs(x))),2),
                   'rms_dbfs':round(20*np.log10(np.sqrt(np.mean(x*x))),2),'clipped_samples':int(np.sum(np.abs(pcm)>=32767))})
(ROOT/'assets'/'audio-generation.json').write_text(json.dumps({'method':'original deterministic modal/contact synthesis; no external samples','rate':RATE,'files':report},indent=2)+'\n',encoding='utf-8')
print(f'Created {len(report)} original physical-contact effects; no clipped samples: {all(r["clipped_samples"]==0 for r in report)}')
