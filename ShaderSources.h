#pragma once

const auto vertexShader2DText =
"cbuffer vertexBuffer : register(b0) \
            {\
            float4x4 projectionMatrix;\
            float4x4 viewMatrix;\
            float4x4 modelMatrix;\
            };\
            struct VS_INPUT\
            {\
              float3 pos : POSITION;\
              float3 normal : NORMAL;\
              float2 uv  : TEXCOORD0;\
              float4 col : COLOR0;\
            };\
            \
            struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 normal : NORMAL;\
              float2 uv  : TEXCOORD0;\
              float4 col : COLOR0;\
            };\
            \
            PS_INPUT main(VS_INPUT input)\
            {\
              PS_INPUT output;\
              output.pos = mul(mul(mul(float4(input.pos.xyz, 1.0), modelMatrix), viewMatrix), projectionMatrix);\
              output.normal = mul(float4(input.normal, 0.0), modelMatrix);\
              output.uv  = input.uv;\
              output.col = input.col;\
              return output;\
            }";

const auto pixelShader2DText = "struct PS_INPUT\
            {\
              float4 pos : SV_POSITION;\
              float4 normal : NORMAL;\
              float2 uv  : TEXCOORD0;\
              float4 col : COLOR0;\
            };\
            SamplerState sampler0 : register(s0);\
            Texture2D texture0 : register(t0);\
            \
            float4 main(PS_INPUT input) : SV_Target\
            {\
              float4 out_col = texture0.Sample(sampler0, input.uv); \
              return out_col; \
            }";