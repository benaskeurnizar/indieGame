#ifndef ENDIAN_H
#define ENDIAN_H


static inline int16_t LittleShort(int16_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#else
    return (int16_t)((uint16_t)x >> 8 | (uint16_t)x << 8);
#endif
}

static inline int32_t LittleLong(int32_t x)
{
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    return x;
#else
    return ((uint32_t)x >> 24) |
           (((uint32_t)x >> 8) & 0x0000FF00) |
           (((uint32_t)x << 8) & 0x00FF0000) |
           ((uint32_t)x << 24);
#endif
}


// Add this function:
float LittleFloat(float f) {
    // On little-endian systems (x86/x64), no swap needed
    #if defined(__LITTLE_ENDIAN__) || defined(_WIN32) || defined(__x86_64__) || defined(__i386__)
        return f;
    #else
        // On big-endian, swap bytes
        union { float f; uint32_t i; } u;
        u.f = f;
        u.i = ((u.i & 0xFF) << 24) | ((u.i & 0xFF00) << 8) | 
              ((u.i >> 8) & 0xFF00) | ((u.i >> 24) & 0xFF);
        return u.f;
    #endif
}



#endif
