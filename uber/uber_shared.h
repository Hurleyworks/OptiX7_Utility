﻿#pragma once

#include "../common/common.h"

#define USE_NATIVE_BLOCK_BUFFER2D

namespace Shared {
    static constexpr float Pi = 3.14159265358979323846f;



    enum RayType {
        RayType_Search = 0,
        RayType_Visibility,
        NumRayTypes
    };



    struct Vertex {
        float3 position;
        float3 normal;
        float2 texCoord;
    };

    struct Triangle {
        uint32_t index0, index1, index2;
    };

    struct AABB {
        float3 minP;
        float3 maxP;
    };



    class PCG32RNG {
        uint64_t state;

    public:
        CUDA_DEVICE_FUNCTION PCG32RNG() {}

        void setState(uint32_t _state) { state = _state; }

        CUDA_DEVICE_FUNCTION uint32_t operator()() {
            uint64_t oldstate = state;
            // Advance internal state
            state = oldstate * 6364136223846793005ULL + 1;
            // Calculate output function (XSH RR), uses old state for max ILP
            uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
            uint32_t rot = oldstate >> 59u;
            return (xorshifted >> rot) | (xorshifted << ((-(int32_t)rot) & 31));
        }

        CUDA_DEVICE_FUNCTION float getFloat0cTo1o() {
            uint32_t fractionBits = ((*this)() >> 9) | 0x3f800000;
            return *(float*)&fractionBits - 1.0f;
        }
    };



    struct PerspectiveCamera {
        float aspect;
        float fovY;
        float3 position;
        Matrix3x3 orientation;
    };



    struct SphereParameter {
        float3 center;
        float radius;
        float texCoordMultiplier;
    };



    struct HitPointParameter {
        float b0, b1;
        int32_t primIndex;

#if defined(__CUDA_ARCH__) || defined(__INTELLISENSE__)
        CUDA_DEVICE_FUNCTION static HitPointParameter get() {
            HitPointParameter ret;
            if (optixGetPrimitiveType() == OPTIX_PRIMITIVE_TYPE_TRIANGLE) {
                float2 bc = optixGetTriangleBarycentrics();
                ret.b0 = 1 - bc.x - bc.y;
                ret.b1 = bc.x;
            }
            else {
                optixu::getAttributes(&ret.b0, &ret.b1);
            }
            ret.primIndex = optixGetPrimitiveIndex();
            return ret;
        }
#endif
    };


    
    struct GeometryData;
    
    using ProgDecodeHitPoint = optixu::DirectCallableProgramID<void(const HitPointParameter &, const GeometryData &, float3*, float3*, float2*)>;
    
    struct GeometryData {
        union {
            struct {
                const Vertex* vertexBuffer;
                const Triangle* triangleBuffer;
            };
            struct {
                const AABB* aabbBuffer;
                const SphereParameter* paramBuffer;
            };
        };
        ProgDecodeHitPoint decodeHitPointFunc;

#if defined(__CUDA_ARCH__) || defined(__INTELLISENSE__)
        CUDA_DEVICE_FUNCTION void decodeHitPoint(const HitPointParameter &hitPointParam,
                                        float3* p, float3* sn, float2* texCoord) const {
            decodeHitPointFunc(hitPointParam, *this, p, sn, texCoord);
            *p = optixTransformPointFromObjectToWorldSpace(*p);
            *sn = normalize(optixTransformNormalFromObjectToWorldSpace(*sn));
        }
#endif
    };

    struct MaterialData {
        float3 albedo;
        union {
            struct {
                unsigned int program : 16;
                unsigned int texID : 16;
            };
            uint32_t misc;
        };

        MaterialData() :
            albedo(make_float3(0.0f, 0.0f, 0.5f)),
            misc(0xFFFFFFFF) {}
    };



    struct PipelineLaunchParameters {
        const OptixTraversableHandle* travHandles;
        const MaterialData* materialData;
        const GeometryData* geomInstData;
        uint32_t travIndex;
        int2 imageSize;
        uint32_t numAccumFrames;
        optixu::BlockBuffer2D<PCG32RNG, 1> rngBuffer;
#if defined(USE_NATIVE_BLOCK_BUFFER2D)
        optixu::NativeBlockBuffer2D<float4> accumBuffer;
#else
        optixu::BlockBuffer2D<float4, 1> accumBuffer;
#endif
        PerspectiveCamera camera;
        uint32_t matLightIndex;
        CUtexObject* textures;
    };
}
