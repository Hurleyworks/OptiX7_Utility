#pragma once

// Platform defines
#if defined(_WIN32) || defined(_WIN64)
#    define HP_Platform_Windows
#    if defined(_MSC_VER)
#        define HP_Platform_Windows_MSVC
#    endif
#elif defined(__APPLE__)
#    define HP_Platform_macOS
#endif

#ifdef _DEBUG
#   define ENABLE_ASSERT
#   define DEBUG_SELECT(A, B) A
#else
#   define DEBUG_SELECT(A, B) B
#endif



// #includes
#if defined(__CUDA_ARCH__)
#else
#   include <cstdio>
#   include <cstdlib>
#   include <cstdint>
#   include <cmath>

#   include <fstream>
#   include <sstream>
#   include <vector>
#   include <set>
#   include <unordered_set>
#   include <random>
#   include <algorithm>
#   include <filesystem>
#   include <functional>
#   include <thread>
#   include <chrono>

#   include <immintrin.h>

#   include "GLToolkit.h"
#   include "stopwatch.h"
#endif

#include "../optix_util.h"
#if !defined(__CUDA_ARCH__)
#   define CUDA_DEVICE_FUNCTION inline
#endif



#if defined(HP_Platform_Windows_MSVC)
#   define NOMINMAX
#   define _USE_MATH_DEFINES
#   include <Windows.h>
#   undef near
#   undef far
#   undef RGB
#endif

#ifdef HP_Platform_Windows_MSVC
void devPrintf(const char* fmt, ...);
#else
#   define devPrintf(fmt, ...) printf(fmt, ##__VA_ARGS__);
#endif

#ifdef ENABLE_ASSERT
#   if defined(__CUDA_ARCH__)
#       define Assert(expr, fmt, ...) if (!(expr)) { devPrintf("%s @%s: %u:\n", #expr, __FILE__, __LINE__); devPrintf(fmt"\n", ##__VA_ARGS__); assert(false); } 0
#   else
#       define Assert(expr, fmt, ...) if (!(expr)) { devPrintf("%s @%s: %u:\n", #expr, __FILE__, __LINE__); devPrintf(fmt"\n", ##__VA_ARGS__); abort(); } 0
#   endif
#else
#   define Assert(expr, fmt, ...)
#endif

#define Assert_ShouldNotBeCalled() Assert(false, "Should not be called!")
#define Assert_NotImplemented() Assert(false, "Not implemented yet!")



template <typename T, size_t size>
constexpr size_t lengthof(const T (&array)[size]) {
    return size;
}



CUDA_DEVICE_FUNCTION uint32_t tzcnt(uint32_t x) {
#if defined(__CUDA_ARCH__)
    return __clz(__brev(x));
#else
    return _tzcnt_u32(x);
#endif
}

CUDA_DEVICE_FUNCTION uint32_t lzcnt(uint32_t x) {
#if defined(__CUDA_ARCH__)
    return __clz(x);
#else
    return _lzcnt_u32(x);
#endif
}

CUDA_DEVICE_FUNCTION int32_t popcnt(uint32_t x) {
#if defined(__CUDA_ARCH__)
    return __popc(x);
#else
    return _mm_popcnt_u32(x);
#endif
}

//     0: 0
//     1: 0
//  2- 3: 1
//  4- 7: 2
//  8-15: 3
// 16-31: 4
// ...
CUDA_DEVICE_FUNCTION uint32_t prevPowOf2Exponent(uint32_t x) {
    if (x == 0)
        return 0;
    return 31 - lzcnt(x);
}

//    0: 0
//    1: 0
//    2: 1
// 3- 4: 2
// 5- 8: 3
// 9-16: 4
// ...
CUDA_DEVICE_FUNCTION uint32_t nextPowOf2Exponent(uint32_t x) {
    if (x == 0)
        return 0;
    return 32 - lzcnt(x - 1);
}

//     0: 0
//     1: 1
//  2- 3: 2
//  4- 7: 4
//  8-15: 8
// 16-31: 16
// ...
CUDA_DEVICE_FUNCTION uint32_t prevPowerOf2(uint32_t x) {
    if (x == 0)
        return 0;
    return 1 << prevPowOf2Exponent(x);
}

//    0: 0
//    1: 1
//    2: 2
// 3- 4: 4
// 5- 8: 8
// 9-16: 16
// ...
CUDA_DEVICE_FUNCTION uint32_t nextPowerOf2(uint32_t x) {
    if (x == 0)
        return 0;
    return 1 << nextPowOf2Exponent(x);
}

template <typename IntType>
CUDA_DEVICE_FUNCTION constexpr IntType nextMultiplesForPowOf2(IntType x, uint32_t exponent) {
    IntType mask = (1 << exponent) - 1;
    return (x + mask) & ~mask;
}

template <typename IntType>
CUDA_DEVICE_FUNCTION constexpr IntType nextMultiplierForPowOf2(IntType x, uint32_t exponent) {
    return nextMultiplesForPowOf2(x, exponent) >> exponent;
}

CUDA_DEVICE_FUNCTION uint32_t nthSetBit(uint32_t value, int32_t n) {
    uint32_t idx = 0;
    int32_t count;
    if (n >= popcnt(value))
        return 0xFFFFFFFF;

    for (uint32_t width = 16; width >= 1; width >>= 1) {
        if (value == 0)
            return 0xFFFFFFFF;

        uint32_t mask = (1 << width) - 1;
        count = popcnt(value & mask);
        if (n >= count) {
            value >>= width;
            n -= count;
            idx += width;
        }
    }

    return idx;
}



#if !defined(__CUDA_ARCH__) || defined(__INTELLISENSE__)

#if 1
#   define hpprintf(fmt, ...) do { devPrintf(fmt, ##__VA_ARGS__); printf(fmt, ##__VA_ARGS__); } while (0)
#else
#   define hpprintf(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif



struct alignas(8) int2 {
    int32_t x, y;
    constexpr int2(int32_t v = 0) : x(v), y(v) {}
    constexpr int2(int32_t xx, int32_t yy) : x(xx), y(yy) {}
};
inline constexpr int2 make_int2(int32_t x, int32_t y) {
    return int2(x, y);
}
struct int3 {
    int32_t x, y, z;
    constexpr int3(int32_t v = 0) : x(v), y(v), z(v) {}
    constexpr int3(int32_t xx, int32_t yy, int32_t zz) : x(xx), y(yy), z(zz) {}
};
inline constexpr int3 make_int3(int32_t x, int32_t y, int32_t z) {
    return int3(x, y, z);
}
struct alignas(16) int4 {
    int32_t x, y, z, w;
    constexpr int4(int32_t v = 0) : x(v), y(v), z(v), w(v) {}
    constexpr int4(int32_t xx, int32_t yy, int32_t zz, int32_t ww) : x(xx), y(yy), z(zz), w(ww) {}
};
inline constexpr int4 make_int4(int32_t x, int32_t y, int32_t z, int32_t w) {
    return int4(x, y, z, w);
}
struct alignas(8) uint2 {
    uint32_t x, y;
    constexpr uint2(uint32_t v = 0) : x(v), y(v) {}
    constexpr uint2(uint32_t xx, uint32_t yy) : x(xx), y(yy) {}
};
inline constexpr uint2 make_uint2(uint32_t x, uint32_t y) {
    return uint2(x, y);
}
struct uint3 {
    uint32_t x, y, z;
    constexpr uint3(uint32_t v = 0) : x(v), y(v), z(v) {}
    constexpr uint3(uint32_t xx, uint32_t yy, uint32_t zz) : x(xx), y(yy), z(zz) {}
};
inline constexpr uint3 make_uint3(uint32_t x, uint32_t y, uint32_t z) {
    return uint3(x, y, z);
}
struct uint4 {
    uint32_t x, y, z, w;
    constexpr uint4(uint32_t v = 0) : x(v), y(v), z(v), w(v) {}
    constexpr uint4(uint32_t xx, uint32_t yy, uint32_t zz, uint32_t ww) : x(xx), y(yy), z(zz), w(ww) {}
};
inline constexpr uint4 make_uint4(uint32_t x, uint32_t y, uint32_t z, uint32_t w) {
    return uint4(x, y, z, w);
}
struct alignas(8) float2 {
    float x, y;
    constexpr float2(float v = 0) : x(v), y(v) {}
    constexpr float2(float xx, float yy) : x(xx), y(yy) {}
};
inline float2 make_float2(float x, float y) {
    return float2(x, y);
}
struct float3 {
    float x, y, z;
    constexpr float3(float v = 0) : x(v), y(v), z(v) {}
    constexpr float3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
};
inline constexpr float3 make_float3(float x, float y, float z) {
    return float3(x, y, z);
}
struct alignas(16) float4 {
    float x, y, z, w;
    constexpr float4(float v = 0) : x(v), y(v), z(v), w(v) {}
    constexpr float4(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) {}
};
inline constexpr float4 make_float4(float x, float y, float z, float w) {
    return float4(x, y, z, w);
}



template <typename T, typename Deleter, typename ...ArgTypes>
std::shared_ptr<T> make_shared_with_deleter(const Deleter &deleter, ArgTypes&&... args) {
    return std::shared_ptr<T>(new T(std::forward<ArgTypes>(args)...),
                              deleter);
}

std::filesystem::path getExecutableDirectory();

std::string readTxtFile(const std::filesystem::path& filepath);

float sRGB_degamma_s(float value);



class SlotFinder {
    uint32_t m_numLayers;
    uint32_t m_numLowestFlagBins;
    uint32_t m_numTotalCompiledFlagBins;
    uint32_t* m_flagBins;
    uint32_t* m_offsetsToOR_AND;
    uint32_t* m_numUsedFlagsUnderBinList;
    uint32_t* m_offsetsToNumUsedFlags;
    uint32_t* m_numFlagsInLayerList;

    SlotFinder(const SlotFinder &) = delete;
    SlotFinder &operator=(const SlotFinder &) = delete;

    void aggregate();

    uint32_t getNumLayers() const {
        return m_numLayers;
    }

    const uint32_t* getOffsetsToOR_AND() const {
        return m_offsetsToOR_AND;
    }

    const uint32_t* getOffsetsToNumUsedFlags() const {
        return m_offsetsToNumUsedFlags;
    }

    const uint32_t* getNumFlagsInLayerList() const {
        return m_numFlagsInLayerList;
    }

public:
    static constexpr uint32_t InvalidSlotIndex = 0xFFFFFFFF;

    SlotFinder() :
        m_numLayers(0), m_numLowestFlagBins(0), m_numTotalCompiledFlagBins(0),
        m_flagBins(nullptr), m_offsetsToOR_AND(nullptr),
        m_numUsedFlagsUnderBinList(nullptr), m_offsetsToNumUsedFlags(nullptr),
        m_numFlagsInLayerList(nullptr) {
    }
    ~SlotFinder() {
    }

    void initialize(uint32_t numSlots);

    void finalize();

    SlotFinder &operator=(SlotFinder &&inst) {
        finalize();

        m_numLayers = inst.m_numLayers;
        m_numLowestFlagBins = inst.m_numLowestFlagBins;
        m_numTotalCompiledFlagBins = inst.m_numTotalCompiledFlagBins;
        m_flagBins = inst.m_flagBins;
        m_offsetsToOR_AND = inst.m_offsetsToOR_AND;
        m_numUsedFlagsUnderBinList = inst.m_numUsedFlagsUnderBinList;
        m_offsetsToNumUsedFlags = inst.m_offsetsToNumUsedFlags;
        m_numFlagsInLayerList = inst.m_numFlagsInLayerList;
        inst.m_flagBins = nullptr;
        inst.m_offsetsToOR_AND = nullptr;
        inst.m_numUsedFlagsUnderBinList = nullptr;
        inst.m_offsetsToNumUsedFlags = nullptr;
        inst.m_numFlagsInLayerList = nullptr;

        return *this;
    }
    SlotFinder(SlotFinder &&inst) {
        *this = std::move(inst);
    }

    void resize(uint32_t numSlots);

    void reset() {
        std::fill_n(m_flagBins, m_numLowestFlagBins + m_numTotalCompiledFlagBins, 0);
        std::fill_n(m_numUsedFlagsUnderBinList, m_numLowestFlagBins + m_numTotalCompiledFlagBins / 2, 0);
    }



    void setInUse(uint32_t slotIdx);

    void setNotInUse(uint32_t slotIdx);

    bool getUsage(uint32_t slotIdx) const {
        uint32_t binIdx = slotIdx / 32;
        uint32_t flagIdxInBin = slotIdx % 32;
        uint32_t flagBin = m_flagBins[binIdx];

        return (bool)((flagBin >> flagIdxInBin) & 0x1);
    }

    uint32_t getFirstAvailableSlot() const;

    uint32_t getFirstUsedSlot() const;

    uint32_t find_nthUsedSlot(uint32_t n) const;

    uint32_t getNumSlots() const {
        return m_numFlagsInLayerList[0];
    }

    uint32_t getNumUsed() const {
        return m_numUsedFlagsUnderBinList[m_offsetsToNumUsedFlags[m_numLayers - 1]];
    }

    void debugPrint() const;
};

#endif



CUDA_DEVICE_FUNCTION float3 getXYZ(const float4 &v) {
    return make_float3(v.x, v.y, v.z);
}

CUDA_DEVICE_FUNCTION float2 make_float2(float v) {
    return make_float2(v, v);
}
CUDA_DEVICE_FUNCTION float2 operator-(const float2 &v) {
    return make_float2(-v.x, -v.y);
}
CUDA_DEVICE_FUNCTION float2 operator+(const float2 &v0, const float2 &v1) {
    return make_float2(v0.x + v1.x, v0.y + v1.y);
}
CUDA_DEVICE_FUNCTION float2 operator-(const float2 &v0, const float2 &v1) {
    return make_float2(v0.x - v1.x, v0.y - v1.y);
}
CUDA_DEVICE_FUNCTION float2 operator*(const float2 &v0, const float2 &v1) {
    return make_float2(v0.x * v1.x, v0.y * v1.y);
}
CUDA_DEVICE_FUNCTION float2 operator*(float s, const float2 &v) {
    return make_float2(s * v.x, s * v.y);
}
CUDA_DEVICE_FUNCTION float2 operator*(const float2 &v, float s) {
    return make_float2(s * v.x, s * v.y);
}
CUDA_DEVICE_FUNCTION float2 operator/(const float2 &v, float s) {
    float r = 1 / s;
    return r * v;
}

CUDA_DEVICE_FUNCTION float3 make_float3(float v) {
    return make_float3(v, v, v);
}
CUDA_DEVICE_FUNCTION float3 operator-(const float3 &v) {
    return make_float3(-v.x, -v.y, -v.z);
}
CUDA_DEVICE_FUNCTION float3 operator+(const float3 &v0, const float3 &v1) {
    return make_float3(v0.x + v1.x, v0.y + v1.y, v0.z + v1.z);
}
CUDA_DEVICE_FUNCTION float3 &operator+=(float3 &v0, const float3 &v1) {
    v0.x += v1.x;
    v0.y += v1.y;
    v0.z += v1.z;
    return v0;
}
CUDA_DEVICE_FUNCTION float3 operator-(const float3 &v0, const float3 &v1) {
    return make_float3(v0.x - v1.x, v0.y - v1.y, v0.z - v1.z);
}
CUDA_DEVICE_FUNCTION float3 operator*(const float3 &v0, const float3 &v1) {
    return make_float3(v0.x * v1.x, v0.y * v1.y, v0.z * v1.z);
}
CUDA_DEVICE_FUNCTION float3 operator*(float s, const float3 &v) {
    return make_float3(s * v.x, s * v.y, s * v.z);
}
CUDA_DEVICE_FUNCTION float3 operator*(const float3 &v, float s) {
    return make_float3(s * v.x, s * v.y, s * v.z);
}
CUDA_DEVICE_FUNCTION float3 &operator*=(float3 &v, float s) {
    v.x *= s;
    v.y *= s;
    v.z *= s;
    return v;
}
CUDA_DEVICE_FUNCTION float3 operator/(const float3 &v0, const float3 &v1) {
    return make_float3(v0.x / v1.x, v0.y / v1.y, v0.z / v1.z);
}
CUDA_DEVICE_FUNCTION float3 operator/(const float3 &v, float s) {
    float r = 1 / s;
    return r * v;
}
CUDA_DEVICE_FUNCTION float3 &operator/=(float3 &v, float s) {
    float r = 1 / s;
    return v *= r;
}
CUDA_DEVICE_FUNCTION bool allFinite(const float3 &v) {
#if !defined(__CUDA_ARCH__)
    using std::isfinite;
#endif
    return isfinite(v.x) && isfinite(v.y) && isfinite(v.z);
}

CUDA_DEVICE_FUNCTION float4 make_float4(float v) {
    return make_float4(v, v, v, v);
}
CUDA_DEVICE_FUNCTION float4 make_float4(const float3 &v, float w) {
    return make_float4(v.x, v.y, v.z, w);
}

CUDA_DEVICE_FUNCTION float dot(const float3 &v0, const float3 &v1) {
    return v0.x * v1.x + v0.y * v1.y + v0.z * v1.z;
}
CUDA_DEVICE_FUNCTION float3 cross(const float3 &v0, const float3 &v1) {
    return make_float3(v0.y * v1.z - v0.z * v1.y,
                       v0.z * v1.x - v0.x * v1.z,
                       v0.x * v1.y - v0.y * v1.x);
}
CUDA_DEVICE_FUNCTION float length(const float3 &v) {
    return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
CUDA_DEVICE_FUNCTION float sqLength(const float3 &v) {
    return v.x * v.x + v.y * v.y + v.z * v.z;
}
CUDA_DEVICE_FUNCTION float3 normalize(const float3 &v) {
    return v / length(v);
}



struct Matrix3x3 {
    union {
        struct { float m00, m10, m20; };
        float3 c0;
    };
    union {
        struct { float m01, m11, m21; };
        float3 c1;
    };
    union {
        struct { float m02, m12, m22; };
        float3 c2;
    };

    CUDA_DEVICE_FUNCTION constexpr Matrix3x3() :
        c0(make_float3(1, 0, 0)),
        c1(make_float3(0, 1, 0)),
        c2(make_float3(0, 0, 1)) { }
    CUDA_DEVICE_FUNCTION Matrix3x3(const float array[9]) :
        m00(array[0]), m10(array[1]), m20(array[2]),
        m01(array[3]), m11(array[4]), m21(array[5]),
        m02(array[6]), m12(array[7]), m22(array[8]) { }
    CUDA_DEVICE_FUNCTION constexpr Matrix3x3(const float3 &col0, const float3 &col1, const float3 &col2) :
        c0(col0), c1(col1), c2(col2)
    { }

    CUDA_DEVICE_FUNCTION Matrix3x3 operator+() const { return *this; }
    CUDA_DEVICE_FUNCTION Matrix3x3 operator-() const { return Matrix3x3(-c0, -c1, -c2); }

    CUDA_DEVICE_FUNCTION Matrix3x3 operator+(const Matrix3x3 &mat) const { return Matrix3x3(c0 + mat.c0, c1 + mat.c1, c2 + mat.c2); }
    CUDA_DEVICE_FUNCTION Matrix3x3 operator-(const Matrix3x3 &mat) const { return Matrix3x3(c0 - mat.c0, c1 - mat.c1, c2 - mat.c2); }
    CUDA_DEVICE_FUNCTION Matrix3x3 operator*(const Matrix3x3 &mat) const {
        const float3 r[] = { row(0), row(1), row(2) };
        return Matrix3x3(make_float3(dot(r[0], mat.c0), dot(r[1], mat.c0), dot(r[2], mat.c0)),
                         make_float3(dot(r[0], mat.c1), dot(r[1], mat.c1), dot(r[2], mat.c1)),
                         make_float3(dot(r[0], mat.c2), dot(r[1], mat.c2), dot(r[2], mat.c2)));
    }
    CUDA_DEVICE_FUNCTION float3 operator*(const float3 &v) const {
        const float3 r[] = { row(0), row(1), row(2) };
        return make_float3(dot(r[0], v),
                           dot(r[1], v),
                           dot(r[2], v));
    }

    CUDA_DEVICE_FUNCTION Matrix3x3 &operator*=(const Matrix3x3 &mat) {
        const float3 r[] = { row(0), row(1), row(2) };
        c0 = make_float3(dot(r[0], mat.c0), dot(r[1], mat.c0), dot(r[2], mat.c0));
        c1 = make_float3(dot(r[0], mat.c1), dot(r[1], mat.c1), dot(r[2], mat.c1));
        c2 = make_float3(dot(r[0], mat.c2), dot(r[1], mat.c2), dot(r[2], mat.c2));
        return *this;
    }

    CUDA_DEVICE_FUNCTION float3 row(unsigned int r) const {
        //Assert(r < 3, "\"r\" is out of range [0, 2].");
        switch (r) {
        case 0:
            return make_float3(m00, m01, m02);
        case 1:
            return make_float3(m10, m11, m12);
        case 2:
            return make_float3(m20, m21, m22);
        default:
            return make_float3(0, 0, 0);
        }
    }

    CUDA_DEVICE_FUNCTION Matrix3x3 &transpose() {
        std::swap(m10, m01); std::swap(m20, m02);
        std::swap(m21, m12);
        return *this;
    }
};

CUDA_DEVICE_FUNCTION Matrix3x3 scale3x3(const float3 &s) {
    return Matrix3x3(s.x * make_float3(1, 0, 0),
                     s.y * make_float3(0, 1, 0),
                     s.z * make_float3(0, 0, 1));
}
CUDA_DEVICE_FUNCTION Matrix3x3 scale3x3(float sx, float sy, float sz) {
    return scale3x3(make_float3(sx, sy, sz));
}
CUDA_DEVICE_FUNCTION Matrix3x3 scale3x3(float s) {
    return scale3x3(make_float3(s, s, s));
}

CUDA_DEVICE_FUNCTION Matrix3x3 rotate3x3(float angle, const float3 &axis) {
    Matrix3x3 matrix;
    float3 nAxis = normalize(axis);
    float s = std::sin(angle);
    float c = std::cos(angle);
    float oneMinusC = 1 - c;

    matrix.m00 = nAxis.x * nAxis.x * oneMinusC + c;
    matrix.m10 = nAxis.x * nAxis.y * oneMinusC + nAxis.z * s;
    matrix.m20 = nAxis.z * nAxis.x * oneMinusC - nAxis.y * s;
    matrix.m01 = nAxis.x * nAxis.y * oneMinusC - nAxis.z * s;
    matrix.m11 = nAxis.y * nAxis.y * oneMinusC + c;
    matrix.m21 = nAxis.y * nAxis.z * oneMinusC + nAxis.x * s;
    matrix.m02 = nAxis.z * nAxis.x * oneMinusC + nAxis.y * s;
    matrix.m12 = nAxis.y * nAxis.z * oneMinusC - nAxis.x * s;
    matrix.m22 = nAxis.z * nAxis.z * oneMinusC + c;

    return matrix;
}
CUDA_DEVICE_FUNCTION Matrix3x3 rotate3x3(float angle, float ax, float ay, float az) {
    return rotate3x3(angle, make_float3(ax, ay, az));
}
CUDA_DEVICE_FUNCTION Matrix3x3 rotateX3x3(float angle) { return rotate3x3(angle, make_float3(1, 0, 0)); }
CUDA_DEVICE_FUNCTION Matrix3x3 rotateY3x3(float angle) { return rotate3x3(angle, make_float3(0, 1, 0)); }
CUDA_DEVICE_FUNCTION Matrix3x3 rotateZ3x3(float angle) { return rotate3x3(angle, make_float3(0, 0, 1)); }



struct Quaternion {
    union {
        float3 v;
        struct {
            float x;
            float y;
            float z;
        };
    };
    float w;

    CUDA_DEVICE_FUNCTION constexpr Quaternion() : v(), w(1) {}
    CUDA_DEVICE_FUNCTION constexpr Quaternion(float xx, float yy, float zz, float ww) : v(make_float3(xx, yy, zz)), w(ww) {}
    CUDA_DEVICE_FUNCTION constexpr Quaternion(const float3 &vv, float ww) : v(vv), w(ww) {}

    CUDA_DEVICE_FUNCTION Quaternion operator+() const { return *this; }
    CUDA_DEVICE_FUNCTION Quaternion operator-() const { return Quaternion(-v, -w); }

    CUDA_DEVICE_FUNCTION Quaternion operator*(const Quaternion &q) const {
        return Quaternion(cross(v, q.v) + w * q.v + q.w * v, w * q.w - dot(v, q.v));
    }
    CUDA_DEVICE_FUNCTION Quaternion operator*(float s) const { return Quaternion(v * s, w * s); }
    CUDA_DEVICE_FUNCTION Quaternion operator/(float s) const { float r = 1 / s; return *this * r; }
    CUDA_DEVICE_FUNCTION friend Quaternion operator*(float s, const Quaternion &q) { return q * s; }

    CUDA_DEVICE_FUNCTION void toEulerAngles(float* roll, float* pitch, float* yaw) const {
        float xx = x * x;
        float xy = x * y;
        float xz = x * z;
        float xw = x * w;
        float yy = y * y;
        float yz = y * z;
        float yw = y * w;
        float zz = z * z;
        float zw = z * w;
        float ww = w * w;
        *pitch = std::atan2(2 * (xw + yz), ww - xx - yy + zz); // around x
        *yaw = std::asin(std::fmin(std::fmax(2 * (yw - xz), -1), 1)); // around y
        *roll = std::atan2(2 * (zw + xy), ww + xx - yy - zz); // around z
    }
    CUDA_DEVICE_FUNCTION Matrix3x3 toMatrix3x3() const {
        float xx = x * x, yy = y * y, zz = z * z;
        float xy = x * y, yz = y * z, zx = z * x;
        float xw = x * w, yw = y * w, zw = z * w;
        return Matrix3x3(make_float3(1 - 2 * (yy + zz), 2 * (xy + zw), 2 * (zx - yw)),
                         make_float3(2 * (xy - zw), 1 - 2 * (xx + zz), 2 * (yz + xw)),
                         make_float3(2 * (zx + yw), 2 * (yz - xw), 1 - 2 * (xx + yy)));
    }
};

CUDA_DEVICE_FUNCTION static Quaternion qRotate(float angle, const float3 &axis) {
    float ha = angle / 2;
    float s = std::sin(ha), c = std::cos(ha);
    return Quaternion(s * normalize(axis), c);
}
CUDA_DEVICE_FUNCTION static Quaternion qRotate(float angle, float ax, float ay, float az) {
    return qRotate(angle, make_float3(ax, ay, az));
}
CUDA_DEVICE_FUNCTION static Quaternion qRotateX(float angle) { return qRotate(angle, make_float3(1, 0, 0)); }
CUDA_DEVICE_FUNCTION static Quaternion qRotateY(float angle) { return qRotate(angle, make_float3(0, 1, 0)); }
CUDA_DEVICE_FUNCTION static Quaternion qRotateZ(float angle) { return qRotate(angle, make_float3(0, 0, 1)); }

CUDA_DEVICE_FUNCTION inline Quaternion qFromEulerAngles(float roll, float pitch, float yaw) {
    return qRotateZ(roll) * qRotateY(yaw) * qRotateX(pitch);
}
