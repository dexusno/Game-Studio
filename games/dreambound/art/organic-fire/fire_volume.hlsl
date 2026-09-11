// The cube is only a bounded volume. Its faces never supply the flame outline.
// Mode: 0 drawn charge, 1 travelling fireball, 2 impact/soot dissipation.
float3 extent=max(HalfSize,float3(1,1,1));
float3 w=World-Center, e=Camera-Center;
float3 p=float3(dot(w,Forward),dot(w,Right),dot(w,Up))/extent;
float3 eye=float3(dot(e,Forward),dot(e,Right),dot(e,Up))/extent;
float3 ray=normalize(p-eye);
float travel=step(.5,Mode)*(1-step(1.5,Mode));
float impact=step(1.5,Mode);
float3 sum=0; float opacity=0;
float stepSize=.080;
float jitter=N.Hash(floor(p*170)+Seed)*stepSize;
p+=ray*(.006+jitter);
[loop] for(int sample=0;sample<34;sample++)
{
    if(any(abs(p)>1.02)) break;
    float3 q=p;
    float t=Time;
    float3 flow=q*4.1+float3(t*(travel?6.8:1.2),Seed,-t*2.9);
    float large=N.Fbm(flow);
    float detail=N.Noise(flow*2.7-float3(0,t*.6,0));
    float3 head=q-float3(travel*.47,0,0);
    head.y+=sin(t*9+q.x*8+Seed)*.07;
    head.z+=cos(t*11+q.y*7+Seed)*.08;
    head.x+=(N.Noise(flow+13.1)-.5)*.17;
    float radius=length(head*lerp(float3(1.2,1.14,1.14),float3(2.1,1.25,1.25),travel));
    // Strong erosion is needed: a lightly noisy sphere becomes a smooth bulb
    // after integration and temporal AA. Hot pockets must have gaps between them.
    float flesh=.61-radius+(large-.48)*1.18+(detail-.5)*.22;
    float tail=saturate((.42-q.x)/1.32);
    float2 bend=float2(sin(q.x*7+t*9+Seed),cos(q.x*6-t*7+Seed))*.12*tail;
    float tailRadius=.27*pow(1-tail,.65)+.09*large;
    float plume=tailRadius-length(q.yz-bend)+(large-.5)*.63;
    plume*=smoothstep(-1.,-.74,q.x)*(1-smoothstep(.34,.64,q.x));
    float lift=saturate(q.z+.28)*(1-travel);
    float2 risingCenter=float2(sin(t*8+q.z*6+Seed),cos(t*6-q.z*7+Seed))*.15*lift;
    float tongues=.32-lift*.30-length(q.xy-risingCenter)+(large-.47)*.58;
    tongues*=smoothstep(-.28,.15,q.z)*(1-smoothstep(.76,.97,q.z));
    float density=max(flesh,max(plume*travel*.85,tongues*(1-travel)));
    // Irregular fingers of flame eat into the outer silhouette; no solid ball.
    density=max(0,density)*(1-smoothstep(.87,1.,max(abs(q.x),max(abs(q.y),abs(q.z)))));
    density*=max(0,Intensity)*lerp(3.6,2.8,impact);
    float hot=saturate((.12+flesh*.38+max(large-.35,0)*2.0+detail*.08)*Glow);
    float3 red=float3(.52,.014,.001), gold=float3(2.4,.31,.009), white=float3(4.1,1.6,.18);
    float3 fire=lerp(red,gold,smoothstep(.18,.62,hot));
    fire=lerp(fire,white,smoothstep(.74,.97,hot));
    float soot=impact*(1-saturate(Glow))*.80;
    float3 color=lerp(fire,float3(.025,.018,.012),soot);
    float a=1-exp(-density*stepSize*3.1);
    sum+=(1-opacity)*color*a;
    opacity+=(1-opacity)*a;
    if(opacity>.985)break;
    p+=ray*stepSize;
}
return float4(sum/max(opacity,.0001),saturate(opacity));
