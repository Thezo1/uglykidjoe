#ifndef _SDL_UGLYKIDJOE_H_
#define _SDL_UGLYKIDJOE_H_

// back buffer stuff
// NOTE:(zourt) each pixel is 4 bytes || 32 bits
typedef struct SDL_OffScreenBuffer
{
    SDL_Texture *texture;
    void *memory;
    int width;
    int heigth;
    int pitch;
    int bytes_per_pixel;
} SDL_OffScreenBuffer;

typedef struct SDL_WindowDimensionResult
{
    int width;
    int heigth;
}SDL_WindowDimensionResult;

typedef struct RingBuffer
{
    int size;
    int write_cursor;
    int play_cursor;
    void* data;
}RingBuffer;

typedef struct SDL_SoundOutput
{
    int samples_per_second;
    uint32 running_sample_index;
    int bytes_per_sample;
    int secondary_buffer_size;
    int latency_sample_count;
    
}SDL_SoundOutput;

typedef struct DebugTimeMarker
{
    int play_cursor;
    int write_cursor;
}DebugTimeMarker;

#endif
