#include "uglykidjoe.h"

static void render_gradient(GameOffScreenBuffer *buffer, int blue_offset, int green_offset)
{
    int width = buffer->width;
    int heigth = buffer->heigth;
    int pitch = buffer->pitch;

    uint8 *row = (uint8 *)buffer->memory;
    for(int y = 0;y<buffer->heigth;y++)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x = 0;x<buffer->width;x++)
        {
            uint8 blue = (x + blue_offset);
            uint8 green = (y + green_offset);
            *pixel++ = ((green<<8) | blue);
        }

        row += pitch;
    }
}

static void game_output_sound(GameSoundOutputBuffer *sound_buffer)
{
    local_persist real32 tsine;
    int16 tone_volume = 3000;
    int tone_hz = 256;
    int wave_period = sound_buffer->samples_per_second/tone_hz;

    int16 *sample_out = sound_buffer->samples;
    for(int sample_index = 0;
        sample_index < sound_buffer->sample_count;
        ++sample_index)
    {
        real32 sine_value = sinf(tsine);
        int16 sample_value = (int16)(sine_value * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        tsine += 2.0f*Pi32*1.0f/(real32)wave_period;
    }
}

void GameUpdateAndRender(GameMemory *memory, GameInput *input, GameOffScreenBuffer *buffer, GameSoundOutputBuffer *sound_buffer)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    if(!memory->is_initialized)
    {
        game_state->tone_hz = 256;
        memory->is_initialized = true;
    }

    GameControllerInput *input0 = &input->controllers[0];
    if(input0->is_analog)
    {
        game_state->tone_hz = 256 + (int)(128.0f *(input0->end_x));
        game_state->blue_offset += (int)4.0f*(input0->end_y);
    }
    else
    {
    }

    if(input0->down.ended_down)
    {
        game_state->green_offset += 1;
    }

    game_output_sound(sound_buffer);
    render_gradient(buffer, game_state->blue_offset, game_state->green_offset);
}
