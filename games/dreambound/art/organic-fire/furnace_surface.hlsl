// Subdermal heat follows the posed torso frame and its breathing. Skin stays lit.
float3 d=World-HeatCenter;
float3 q=float3(dot(d,HeatForward),dot(d,HeatRight),dot(d,HeatUp));
float torso=1-smoothstep(.34,1.,length(q/float3(31,24,37)));
float throat=1-smoothstep(.18,1.,length((q-float3(2,0,26))/float3(19,13,24)));
float region=max(torso,throat*.80);
float noise=N.Fbm(q*.11+float3(0,0,-Time*.8));
float branches=pow(saturate(1-abs(noise-.49)*10),3);
float deep=region*(.25+.75*branches);
float pulse=.83+.17*sin(Time*13+q.z*.15);
float hot=saturate(Heat)*pulse;
return float3(2.8,.19,.012)*deep*hot*hot+float3(.5,.012,.001)*region*hot;
