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

#if UGLYKIDJOE_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define KiloBytes(value) ((value)*1024LL)
#define MegaBytes(value) (KiloBytes(value)*1024LL)
#define GigaBytes(value) (MegaBytes(value)*1024LL)
#define TeraBytes(value) (GigaBytes(value)*1024LL)

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

    real32 start_x;
    real32 start_y;

    real32 min_x;
    real32 min_y;

    real32 max_x;
    real32 max_y;

    real32 end_x;
    real32 end_y;

    union
    {
        GameButtonState buttons[6];
        struct
        {
            GameButtonState up;
            GameButtonState down;
            GameButtonState left;
            GameButtonState right;
            GameButtonState left_shoulder;
            GameButtonState right_shoulder;
        };
    };
}GameControllerInput;

typedef struct GameInput
{
    GameControllerInput controllers[4];
}GameInput;

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
