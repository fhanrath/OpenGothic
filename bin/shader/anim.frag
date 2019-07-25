#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 2) uniform sampler2D textureD;
layout(binding = 3) uniform sampler2D textureSm;

#ifdef SHADOW_MAP
layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inShadowPos;
#else
layout(location = 0) in vec2 inUV;
layout(location = 1) in vec4 inShadowPos;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in vec3 inLight;
#endif

layout(location = 0) out vec4 outColor;

float implShadowVal(in vec2 uv, in float shPosZ, in int layer) {
  float shMap = texture(textureSm,uv)[layer];
  float shZ   = min(0.99,shPosZ);

  return step(shZ-0.001,shMap);
  }

float shadowVal(in vec2 uv, in float shPosZ, in int layer) {
  // TODO: uniform shadowmap resolution
  float d1 = 0.5/2048.0;
  float d2 = 1.5/2048.0;
  float ret = implShadowVal(uv+vec2(-d2, d1),shPosZ,layer) +
              implShadowVal(uv+vec2( d1, d1),shPosZ,layer) +
              implShadowVal(uv+vec2(-d2, d2),shPosZ,layer) +
              implShadowVal(uv+vec2( d1,-d2),shPosZ,layer);

  return ret*0.25;
  }

void main() {
  vec4 t = texture(textureD,inUV);
  if(t.a<0.5)
    discard;
#ifdef SHADOW_MAP
  outColor = vec4(inShadowPos.zzz,0.0);
#else
  float lambert = max(0.0,dot(inLight,normalize(inNormal)));
  vec3  shPos0  = (inShadowPos.xyz)/inShadowPos.w;
  vec3  shPos1  = (inShadowPos.xyz*vec3(0.2))/inShadowPos.w;

  float light = lambert;
  if(abs(shPos0.x)<0.99 && abs(shPos0.y)<0.99) {
    light = lambert*shadowVal(shPos0.xy*vec2(0.5,0.5)+vec2(0.5),shPos0.z,0);
    } else {
    if(abs(shPos1.x)<1.0 && abs(shPos1.y)<1.0)
      light = lambert*implShadowVal(shPos1.xy*vec2(0.5,0.5)+vec2(0.5),shPos1.z,1);
    }

  vec3  ambient = vec3(0.25);//*inColor.xyz
  vec3  diffuse = vec3(1.0)*inColor.xyz;
  vec3  color   = mix(ambient,diffuse,clamp(light,0.0,1.0));
  outColor      = vec4(t.rgb*color,t.a);

  //outColor = vec4(vec3(shMap),1.0);
  //outColor = vec4(vec3(light),1.0);
#endif
  }
