// Original deterministic volume noise. No sampled or third-party texture.
struct FOrganicNoise
{
    float Hash(float3 p)
    {
        p = frac(p * .1031);
        p += dot(p, p.yzx + 33.33);
        return frac((p.x + p.y) * p.z);
    }
    float Noise(float3 p)
    {
        float3 i = floor(p), f = frac(p);
        f = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(lerp(Hash(i),Hash(i+float3(1,0,0)),f.x),
                         lerp(Hash(i+float3(0,1,0)),Hash(i+float3(1,1,0)),f.x),f.y),
                    lerp(lerp(Hash(i+float3(0,0,1)),Hash(i+float3(1,0,1)),f.x),
                         lerp(Hash(i+float3(0,1,1)),Hash(i+float3(1,1,1)),f.x),f.y),f.z);
    }
    float Fbm(float3 p)
    {
        return Noise(p)*.57 + Noise(p*2.03+7.1)*.28 + Noise(p*4.11-11.7)*.15;
    }
};
FOrganicNoise N;
