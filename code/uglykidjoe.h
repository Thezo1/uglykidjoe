#ifndef _UGLYKIDJOE_H_
#define _UGLYKIDJOE_H_

/*
    NOTE:
    UGLYKIDJOE_INTERNAL
    0-public
    1-private

    NOTE:
    UGLYKIDJOE_SLOW
    0- no slow
    1- slow
*/

#include<math.h>
#include<stdint.h>
#include<stdbool.h>

#define global_variable static
#define local_persist static
#define internal static

#define MAX_CONTROLLERS 4
#define Pi32 3.14159265358979f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef  float real32;
typedef  double real64;

global_variable bool global_running;

// NOTE: services the platform provides to the game
#if UGLYKIDJOE_INTERNAL
    typedef struct DEBUG_ReadFileResult
    {
        uint32 content_size;
        void *contents;
    }DEBUG_ReadFileResult;

    #define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void *memory)
    typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

    #define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DEBUG_ReadFileResult name(char *filename)
    typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

    #define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool name(char *filename, uint64 memory_size, void *memory)
    typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif

#if UGLYKIDJOE_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define KiloBytes(value) ((value)*1024LL)
#define MegaBytes(value) (KiloBytes(value)*1024LL)
#define GigaBytes(value) (MegaBytes(value)*1024LL)
#define TeraBytes(value) (GigaBytes(value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct GameOffScreenBuffer
{
    void *memory;
    int width;
    int height;
    int pitch;
    int bytes_per_pixel;
} GameOffScreenBuffer;

typedef struct GameSoundOutputBuffer
{
    int samples_per_second;
    int sample_count;
    int16 *samples;
}GameSoundOutputBuffer;

typedef struct GameButtonState
{
    int half_transition_count;
    bool ended_down;
}GameButtonState;

typedef struct GameControllerInput
{
    bool is_analog;
    bool is_connected;
    real32 stick_average_x;
    real32 stick_average_y;

    union
    {
        GameButtonState buttons[12];
        struct
        {
            GameButtonState move_up;
            GameButtonState move_down;
            GameButtonState move_left;
            GameButtonState move_right;

            GameButtonState action_up;
            GameButtonState action_down;
            GameButtonState action_left;
            GameButtonState action_right;

            GameButtonState left_shoulder;
            GameButtonState right_shoulder;

            GameButtonState start;
            GameButtonState back;
        };
    };
}GameControllerInput;

typedef struct GameInput
{
    GameControllerInput controllers[5];
}GameInput;

extern inline GameControllerInput *get_controller(GameInput *input, int controller_index)
{
    Assert(controller_index < ArrayCount(input->controllers));
    GameControllerInput *result = &input->controllers[controller_index];
    return result;
 }

typedef struct GameState
{
    int tone_hz;
    int green_offset;
    int blue_offset;

    real32 tsine;

    int player_x;
    int player_y;
    real32 tjump;
}GameState;

typedef struct GameMemory
{
    bool is_initialized;

    uint64 permanent_storage_size;
    void *permanent_storage; // NOTE: cleared to zero

    uint64 transient_storage_size;
    void *transient_storage; // NOTE: cleared to zero

    debug_platform_free_file_memory *DEBUG_platform_free_file_memory;
    debug_platform_read_entire_file *DEBUG_platform_read_entire_file;
    debug_platform_write_entire_file *DEBUG_platform_write_entire_file;
}GameMemory;


#define GAME_UPDATE_AND_RENDER(name) void name(GameMemory *memory, GameInput *input, GameOffScreenBuffer *buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE: keep this function below 1 millisecond
#define GAME_GET_SOUND_SAMPLES(name) void name(GameMemory *memory, GameSoundOutputBuffer *sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

#endif
