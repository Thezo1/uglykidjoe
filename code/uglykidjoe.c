#include "uglykidjoe.h"

static void render_gradient(GameOffScreenBuffer *buffer, int blue_offset, int green_offset)
{
    int width = buffer->width;
    int heigth = buffer->height;
    int pitch = buffer->pitch;

    uint8 *row = (uint8 *)buffer->memory;
    for(int y = 0;y<buffer->height;y++)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x = 0;x<buffer->width;x++)
        {
            uint8 blue = (x + blue_offset);
            uint8 green = (y + green_offset);
            *pixel++ = ((green<<16) | blue);
        }

        row += pitch;
    }
}

internal void render_player(GameOffScreenBuffer *buffer, int player_x, int player_y)
{
    uint8 *end_of_buffer = (uint8 *)buffer->memory + 
        buffer->bytes_per_pixel * buffer->width+ 
        buffer->pitch * buffer->height;
    uint32 color = 0xFFFFFF;
    int top = player_y;
    int bottom = player_y + 10;
    for(int x = player_x;
        x < player_x+10;
        x++)
    {
        uint8 *pixel = ((uint8 *)buffer->memory 
                + x*buffer->bytes_per_pixel 
                + top*buffer->pitch);

        for(int y = top;
            y < bottom;
            y++)
         {
            if((pixel >= (uint8 *)buffer->memory) && 
                        (pixel < end_of_buffer))
            {
                *(uint32 *)pixel = color;
                pixel += buffer->pitch;
            }
         }
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

#if 1
        real32 sine_value = sinf(game_state->tsine);
        int16 sample_value = (int16)(sine_value * tone_volume);
#else
        int16 sample_value = 0;
#endif
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        game_state->tsine += 2.0f*Pi32*1.0f/(real32)wave_period;
        if(game_state->tsine > 2.0f*Pi32*1.0f)
        {
            game_state->tsine -= 2.0f*Pi32*1.0f ;
        }
    }
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    if(!memory->is_initialized)
    {
        char *filename = __FILE__;

        DEBUG_ReadFileResult result = memory->DEBUG_platform_read_entire_file(thread, filename);
        void *file_memory = result.contents;
        if(file_memory)
        {
            //TODO: dont't hard code test file
            memory->DEBUG_platform_write_entire_file(thread, "test.out", result.content_size, result.contents);
            memory->DEBUG_platform_free_file_memory(thread, file_memory);
        }

        game_state->tone_hz = 256;
        game_state->tsine = 0.0f;

        game_state->player_x = 100;
        game_state->player_y = 100;

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
            game_state->player_y += (int)4.0f*(5);
        }

        if(controller->move_up.ended_down)
        {
            game_state->player_y -= (int)4.0f*(5);
        }

        if(controller->move_left.ended_down)
        {
            game_state->tone_hz -= 10;
            game_state->blue_offset -= 5;
            game_state->player_x -= (int)4.0f*(2);
        }

        if(controller->move_right.ended_down)
        {
            game_state->tone_hz += 10;
            game_state->blue_offset += 5;
            game_state->player_x += (int)4.0f*(2);
        }

        if(controller->action_down.ended_down)
        {
            game_state->player_y -= (int)4.0f*(5) + 4.0*(2.0*Pi32)*sinf(game_state->tjump);
            game_state->tjump = 1.0; 
        }
        game_state->tjump -= 0.033f;

    }

    render_gradient(buffer, game_state->blue_offset, game_state->green_offset);
    render_player(buffer, game_state->player_x, game_state->player_y);

    render_player(buffer, input->mouse_x, input->mouse_y);

    for(int button_index = 0;
        button_index < ArrayCount(input->mouse_buttons);
        button_index++)
    if(input->mouse_buttons[button_index].ended_down)
    {
        render_player(buffer, 10 + 20 * button_index, 10);
    }
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);
    GameState *game_state = (GameState *)memory->permanent_storage;

    game_output_sound(sound_buffer, game_state);
}
