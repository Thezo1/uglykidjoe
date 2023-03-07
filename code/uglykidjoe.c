#include "uglykidjoe.h"

internal inline int32 truncate_real_32_to_int32(real32 real_32)
{
    int32 result = (int32)real_32;
    return(result);
}

internal inline uint32 round_real_32_to_uint_32(real32 real_32)
{
    int32 result = (uint32)(real_32 + 0.5f);
    return(result);
}

internal inline int32 round_real_32_to_int_32(real32 real_32)
{
    int32 result = (int32)(real_32 + 0.5f);
    return(result);
}

internal void draw_rectangle(GameOffScreenBuffer *buffer, 
                            real32 real_min_x, real32 real_min_y,
                            real32 real_max_x, real32 real_max_y,
                            real32 r, real32 g, real32 b)
{
    int32 min_x = round_real_32_to_int_32(real_min_x);
    int32 min_y = round_real_32_to_int_32(real_min_y);
    int32 max_x = round_real_32_to_int_32(real_max_x);
    int32 max_y = round_real_32_to_int_32(real_max_y);

    if(min_x < 0)
    {
        min_x = 0;
    }

    if(min_y < 0)
    {
        min_y = 0;
    }

    if(max_x > buffer->width)
    {
        max_x = buffer->width;
    }

    if(max_y > buffer->height)
    {
        max_y = buffer->height;
    }

    // 0x AA RR BB GG
    uint32 color = (uint32)((round_real_32_to_uint_32(r*255.0f) << 16)|
                            (round_real_32_to_uint_32(g*255.0f) << 8)|
                            (round_real_32_to_uint_32(b*255.0f) << 0));

    uint8 *row = ((uint8 *)buffer->memory 
            + min_x*buffer->bytes_per_pixel 
            + min_y*buffer->pitch);

    for(int y = min_y;
            y < max_y;
            y++)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x = min_x;
                x < max_x;
                x++)
         {
             *pixel++ = color;
         }
        row += buffer->pitch;
    }
}

static void game_output_sound(GameSoundOutputBuffer *sound_buffer, int tone_hz)
{
    int16 tone_volume = 3000;
    int wave_period = sound_buffer->samples_per_second/tone_hz;

    int16 *sample_out = sound_buffer->samples;
    for(int sample_index = 0;
        sample_index < sound_buffer->sample_count;
        ++sample_index)
    {

#if 0
        real32 sine_value = sinf(game_state->tsine);
        int16 sample_value = (int16)(sine_value * tone_volume);
#else
        int16 sample_value = 0;
#endif
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

#if 0
        game_state->tsine += 2.0f*Pi32*1.0f/(real32)wave_period;
        if(game_state->tsine > 2.0f*Pi32*1.0f)
        {
            game_state->tsine -= 2.0f*Pi32*1.0f ;
        }
#endif
    }
}

internal inline uint32 get_tilemap_value_checked(TileMap *tilemap, int32 tile_x, int32 tile_y)
{
    uint32 tilemap_value = tilemap->tiles[tile_y*tilemap->count_x + tile_x];
    return(tilemap_value);
}

internal bool is_tilemap_point_empty(TileMap *tilemap, real32 test_x, real32 test_y)
{
    bool empty;
    int player_tile_x = truncate_real_32_to_int32((test_x - tilemap->upperleft_x) / tilemap->tile_width);
    int player_tile_y = truncate_real_32_to_int32((test_y - tilemap->upperleft_y) / tilemap->tile_heigth);

    if((player_tile_x >= 0) && (player_tile_x < tilemap->count_x) && 
            (player_tile_y >= 0) && (player_tile_y < tilemap->count_y))
    {
        uint32 tilemap_value = get_tilemap_value_checked(tilemap, player_tile_x, player_tile_y);
        empty = (tilemap_value == 0);
    }
    return(empty);
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);

#define TILEMAP_COUNT_X 17
#define TILEMAP_COUNT_Y 9

    uint32 tiles0[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1},
        {1,0,0,1, 0,0,1,0, 1,0,0,0, 0,0,0,0, 1},
        {1,0,0,1, 1,0,1,0, 1,0,0,0, 0,0,1,0, 1},
        {1,0,0,1, 0,0,0,0, 1,0,0,0, 0,0,0,0, 1},
        {1,0,0,1, 0,0,1,0, 1,0,0,0, 0,0,1,0, 1},
        {1,0,0,0, 0,0,1,0, 1,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,1,0, 1,0,0,0, 0,0,0,0, 1},
        {1,0,1,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, 1},
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1},
    };

    uint32 tiles1[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1},
    };

    TileMap tilemaps[2];
    tilemaps[0].count_x = TILEMAP_COUNT_X;
    tilemaps[0].count_y = TILEMAP_COUNT_Y;

    tilemaps[0].upperleft_x = -30;
    tilemaps[0].upperleft_y = 0;
    tilemaps[0].tile_width = 60;
    tilemaps[0].tile_heigth = 60;

    tilemaps[0].tiles = (uint32*)tiles0;

    tilemaps[1] = tilemaps[0];
    tilemaps[1].tiles = (uint32*)tiles1;

    TileMap *tilemap = &tilemaps[0];

    real32 player_width = 0.75f * tilemap->tile_width;

    GameState *game_state = (GameState *)memory->permanent_storage;
    if(!memory->is_initialized)
    {
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
        }
        else
        {
            real32 d_player_x = 0.0f;
            real32 d_player_y = 0.0f;

            if(controller->move_up.ended_down)
            {
                d_player_y = -1.0f;
            }
            if(controller->move_down.ended_down)
            {
                d_player_y = 1.0f;
            }
            if(controller->move_left.ended_down)
            {
                d_player_x = -1.0f;
            }
            if(controller->move_right.ended_down)
            {
                d_player_x = 1.0f;
            }
            d_player_x *= 64.0f;
            d_player_y *= 64.0f;

            real32 new_player_x = game_state->player_x + input->dt_for_frame*d_player_x;
            real32 new_player_y = game_state->player_y + input->dt_for_frame*d_player_y;

            if(is_tilemap_point_empty(tilemap, new_player_x - 0.5f*player_width, new_player_y) &&
               is_tilemap_point_empty(tilemap, new_player_x + 0.5f*player_width, new_player_y) &&
               is_tilemap_point_empty(tilemap, new_player_x, new_player_y))
            {
                game_state->player_x = new_player_x;
                game_state->player_y = new_player_y;
            }

        }
    }

    draw_rectangle(buffer, 0.0f, 0.0f, (real32)buffer->width, (real32)buffer->height, 
                   1.0f, 0.0f, 0.1f);

    for(int row = 0;
        row < TILEMAP_COUNT_Y;
        row++)
    {
        for(int column = 0;
                column < TILEMAP_COUNT_X;
                column++)
        {
            uint32 tile_id = get_tilemap_value_checked(tilemap, column, row);
            real32 gray = 0.5f;
            if(tile_id == 1)
            {
                gray = 1.0f;
            }

            real32 min_x = (real32)(tilemap->upperleft_x + (column * tilemap->tile_width)); real32 min_y = (real32)(tilemap->upperleft_y + (row * tilemap->tile_heigth));
            real32 max_x = min_x + tilemap->tile_width;
            real32 max_y = min_y + tilemap->tile_heigth;
            draw_rectangle(buffer, min_x, min_y, max_x, max_y, gray, gray, gray);
        }
    }

    real32 player_r = 1.0f;
    real32 player_g = 1.0f;
    real32 player_b = 0.0f;
    real32 player_height = tilemap->tile_heigth;
    real32 player_left = game_state->player_x - 0.5f*player_width;
    real32 player_top = game_state->player_y - player_height;

    draw_rectangle(buffer,
                   player_left, player_top,
                   player_left + player_width,
                   player_top + player_height,
                   player_r, player_g, player_b);
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    GameState *game_state = (GameState *)memory->permanent_storage;

    game_output_sound(sound_buffer, 400);
}


/*
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
 *
*/
