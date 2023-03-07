#ifndef _SDL_UGLYKIDJOE_H_
#define _SDL_UGLYKIDJOE_H_

// back buffer stuff
// NOTE:(zourt) each pixel is 4 bytes || 32 bits
#define PATH_MAX 4096

typedef struct SDL_OffScreenBuffer
{
    SDL_Texture *texture;
    void *memory;
    int width;
    int height;
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
    int safety_bytes;
}SDL_SoundOutput;

typedef struct DebugTimeMarker
{
    int play_cursor;
    int write_cursor;
}DebugTimeMarker;

// function pointer can return null, so check whenver called
typedef struct SDL_GameCode
{
    void *game_so;
    struct timespec last_write_time;
    game_update_and_render *update_and_render;
    game_get_sound_samples *get_sound_samples;

    bool is_valid;
}SDL_GameCode;

typedef struct SDL_ReplayBuffer
{
    int file_handle;
    char replay_filename[PATH_MAX];
    void *memory_block;
}SDL_ReplayBuffer;

typedef struct SDL_State
{
    uint64 total_size;
    void *game_memory_block;
    SDL_ReplayBuffer replay_buffers[2];

    int recording_handle;
    int input_recording_index;

    int playback_handle;
    int input_playing_index;

    char exe_filename[PATH_MAX];
    char *one_past_last_slash;
}SDL_State;

#endif
