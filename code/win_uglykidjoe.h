#ifndef _UGLYKIDJOE_H
#define _UGLYKIDJOE_H
#include<stdint.h>

#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359f

typedef uint8_t u8;     // 1-byte long unsigned integer
typedef uint16_t u16;   // 2-byte long unsigned integer
typedef uint32_t u32;   // 4-byte long unsigned integer
typedef uint64_t u64;   // 8-byte long unsigned integer

typedef int8_t i8;      // 1-byte long signed integer
typedef int16_t i16;    // 2-byte long signed integer
typedef int32_t i32;    // 4-byte long signed integer
typedef int64_t i64;    // 8-byte long signed integer

typedef float f32;
typedef double f64;

typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);

#define XInputGetState XInputGetState_ 
#define XInputSetState XInputSetState_ 

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
// Define a type of a function
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

typedef struct Win32_OffScreenBuffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
} Win32_OffScreenBuffer;

typedef struct Win32_WindowDimension
{
    int Width;
    int Height;
} Win32_WindowDimension;

typedef struct Win32_SoundOutput
{
    int SamplesPerSecond;
    int BytesPerSample;
    int SecondaryBufferSize;
    u32 RunningSampleIndex;
    int ToneHz;
    int ToneVolume;
    int WavePeriod;
    f32 TSine;
    int LatencySampleCount;
} Win32_SoundOutput;

#endif
