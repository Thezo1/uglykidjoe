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
        char *filename = __FILE__;

        DEBUG_ReadFileResult result = DEBUGPlatformReadEntireFile(filename);
        void *file_memory = result.contents;
        if(file_memory)
        {
            DEBUGPlatformWriteEntireFile("data/test.out", result.content_size, result.contents);
            DEBUGPlatormFreeFileMemory(file_memory);
        }

        game_state->tone_hz = 256;
        memory->is_initialized = true;
    }

    for(int controller_index = 0;
        controller_index < ArrayCount(input->controllers);
        ++controller_index)
    {
        GameControllerInput *controller = &input->controllers[controller_index];
        if(controller->is_analog)
        {
            game_state->tone_hz = 256 + (int)(128.0f *(controller->stick_average_x));
            game_state->blue_offset += (int)4.0f*(controller->stick_average_y);
        }
        else
        {
        }

        if(controller->move_down.ended_down)
        {
            game_state->green_offset += 1;
        }

        if(controller->move_left.ended_down)
        {
            game_state->blue_offset -= 1;
        }

        if(controller->move_right.ended_down)
        {
            game_state->blue_offset += 1;
        }
    }


    game_output_sound(sound_buffer);
    render_gradient(buffer, game_state->blue_offset, game_state->green_offset);
}