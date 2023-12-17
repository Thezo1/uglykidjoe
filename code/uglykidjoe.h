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
#include<stdio.h>

#define global_variable static
#define local_persist static
#define internal static

#define MAX_CONTROLLERS 4
#define Pi32 3.14159265358979f

// compilers
#ifndef COMPILER_GCC
#define COMPILER_GCC 0
#endif

#ifndef COMPILER_LLVM
#define COMPILER_LLVM 0
#endif

#if !COMPILER_GCC && !COMPILER_LLVM
#if __GNUC__
#undef COMPILER_GCC
#define COMPILER_GCC 1
#else

#undef COMPILER_LLVM
#define COMPILER_LLVM 1

#endif
#endif

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef size_t memory_index;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef  float real32;
typedef  double real64;

global_variable bool global_running;

typedef struct ThreadContext
{
    int place_holder;
}ThreadContext;

// NOTE: services the platform provides to the game
#if UGLYKIDJOE_INTERNAL
    typedef struct DEBUG_ReadFileResult
    {
        uint32 content_size;
        void *contents;
    }DEBUG_ReadFileResult;

    #define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(ThreadContext *thread, void *memory)
    typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

    #define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) DEBUG_ReadFileResult name(ThreadContext *thread, char *filename)
    typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

    #define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool name(ThreadContext *thread, char *filename, uint64 memory_size, void *memory)
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
    /*
        NOTE(me): 

        0 = left button, 
        1 = right button, 
        2 = middle button
    */
    GameButtonState mouse_buttons[5];
    int32 mouse_x, mouse_y, mouse_z;

    real32 dt_for_frame;

    GameControllerInput controllers[5];
}GameInput;

extern inline GameControllerInput *get_controller(GameInput *input, int controller_index)
{
    Assert(controller_index < ArrayCount(input->controllers));
    GameControllerInput *result = &input->controllers[controller_index];
    return result;
 }

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


#define GAME_UPDATE_AND_RENDER(name) void name(ThreadContext *thread, GameMemory *memory, GameInput *input, GameOffScreenBuffer *buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

// NOTE: keep this function below 1 millisecond
#define GAME_GET_SOUND_SAMPLES(name) void name(ThreadContext *thread, GameMemory *memory, GameSoundOutputBuffer *sound_buffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

typedef struct MemoryArena
{
    memory_index size;
    memory_index used;
    uint8 *base;
}MemoryArena;

#define push_struct(arena, type) (type *)push_size_(arena, sizeof(type))
#define push_array(arena, count, type) (type *)push_size_(arena, (count)*sizeof(type))
internal void *push_size_(MemoryArena *arena, memory_index size)
{
    Assert((arena->used + size) <= arena->size);
    void *result = arena->base+arena->used;
    arena->used += size;
    return(result);
}

#include "uglykidjoe_math.h"
#include "uglykidjoe_intrinsics.h"
#include "uglykidjoe_tile.h"
typedef struct World
{
    TileMap *tilemap;
}World;

typedef struct LoadedBitMap
{
    int32 width;
    int32 height;
    uint32 *pixels;
}LoadedBitMap;

typedef struct HeroineBitmaps
{
    int32 align_x;
    int32 align_y;

    LoadedBitMap base;
    LoadedBitMap cape;
    LoadedBitMap weapon;
}HeroineBitmaps;

typedef struct Entity
{
    bool exists;
    TileMapPosition p;
    v2 dp;

    real32 width;
    real32 height;
} Entity;

typedef struct GameState
{
    MemoryArena world_arena;
    World *world;
    
    uint32 camera_following_entity_index;
    TileMapPosition camera_p;

    uint32 entity_count;
    uint32 player_index_for_controller[ArrayCount(((GameInput *)0)->controllers)];
    Entity entities[256];
    
    LoadedBitMap background;
    HeroineBitmaps heroine_bitmaps[1];
}GameState;


#endif
