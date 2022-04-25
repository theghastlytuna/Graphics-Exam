#version 430

layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;

uniform layout(binding = 0) sampler2D s_Albedo;
uniform layout(binding = 1) sampler2D s_NormalsMetallic;
uniform layout(binding = 2) sampler2D s_DiffuseAccumulation;
uniform layout(binding = 3) sampler2D s_SpecularAccumulation;
uniform layout(binding = 4) sampler2D s_Emissive;

#include "../fragments/frame_uniforms.glsl"
#include "../fragments/color_correction.glsl"
#include "../fragments/multiple_point_lights.glsl"

void main() {
    vec3 albedo = texture(s_Albedo, inUV).rgb;
    vec3 diffuse = texture(s_DiffuseAccumulation, inUV).rgb;
    vec3 specular = texture(s_SpecularAccumulation, inUV).rgb;
    vec4 emissive = texture(s_Emissive, inUV);

    vec3 normal = texture(s_NormalsMetallic, inUV).rgb;

    if (IsFlagSet(FLAG_ENABLE_DIFFUSE_LIGHT))
    {

        outColor = vec4(albedo * (diffuse + 0 + (emissive.rgb * emissive.a)), 1.0);
    }
    else if (IsFlagSet(FLAG_ENABLE_AMBIENT_LIGHT))
    {
        outColor = vec4(albedo * (0 + 0 + (emissive.rgb * emissive.a)), 1.0);
    }

    else if (IsFlagSet(FLAG_ENABLE_SPECULAR_LIGHT))
    {
        outColor = vec4(albedo * (0 + specular + (emissive.rgb * emissive.a)), 1.0);
    }

    else outColor = vec4(albedo * (diffuse + specular + (emissive.rgb * emissive.a)), 1.0);
    
}