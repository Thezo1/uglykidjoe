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

#if UGLYKIDJOE_INTERNAL
    typedef struct DEBUG_ReadFileResult
    {
        uint32 content_size;
        void *contents;
    }DEBUG_ReadFileResult;

    internal DEBUG_ReadFileResult DEBUGPlatformReadEntireFile(char *filename);
    internal void DEBUGPlatormFreeFileMemory(void *memory);
    internal bool DEBUGPlatformWriteEntireFile(char *filename, uint64 memory_size, void *memory);
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
    int heigth;
    int pitch;
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
        GameButtonState buttons[10];
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
}GameState;

typedef struct GameMemory
{
    bool is_initialized;

    uint64 permanent_storage_size;
    void *permanent_storage; // NOTE: cleared to zero

    uint64 transient_storage_size;
    void *transient_storage; // NOTE: cleared too zero
}GameMemory;

void GameUpdateAndRender(GameMemory *memory, GameInput *input, GameOffScreenBuffer *buffer, GameSoundOutputBuffer *sound_buffer);

#endif
