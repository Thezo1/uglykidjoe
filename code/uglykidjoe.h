#ifndef _UGLYKIDJOE_H_
#define _UGLYKIDJOE_H_

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

void GameUpdateAndRender(GameInput *input, GameOffScreenBuffer *buffer, GameSoundOutputBuffer *sound_buffer);

#endif
