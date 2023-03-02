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
            *pixel++ = ((green<<0) | blue);
        }

        row += pitch;
    }
}

static void game_output_sound(GameSoundOutputBuffer *sound_buffer, GameState *game_state)
{
    int16 tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second/game_state->tone_hz;

    int16 *sample_out = sound_buffer->samples;
    for(int sample_index = 0;
        sample_index < sound_buffer->sample_count;
        ++sample_index)
    {
        real32 sine_value = sinf(game_state->tsine);
        int16 sample_value = (int16)(sine_value * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        game_state->tsine += 2.0f*Pi32*1.0f/(real32)wave_period;
    }
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    if(!memory->is_initialized)
    {
        char *filename = __FILE__;

        DEBUG_ReadFileResult result = memory->DEBUG_platform_read_entire_file(filename);
        void *file_memory = result.contents;
        if(file_memory)
        {
            memory->DEBUG_platform_write_entire_file("data/test.out", result.content_size, result.contents);
            memory->DEBUG_platform_free_file_memory(file_memory);
        }

        game_state->tone_hz = 256;
        game_state->tsine = 0.0f;
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
            game_state->tone_hz -= 10;
        }

        if(controller->move_right.ended_down)
        {
            game_state->tone_hz += 10;
        }
    }

    render_gradient(buffer, game_state->blue_offset, game_state->green_offset);
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);
    GameState *game_state = (GameState *)memory->permanent_storage;

    game_output_sound(sound_buffer, game_state);
}
