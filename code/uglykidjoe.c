#include "uglykidjoe.h"
#include "uglykidjoe_intrinsics.h"

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

internal inline uint32 get_tile_value_unchecked(World *world, TileChunk *tile_chunk, uint32 tile_chunk_x, uint32 tile_chunk_y)
{
    Assert(tile_chunk);
    Assert((tile_chunk_x < world->chunk_dim) &&
           (tile_chunk_y < world->chunk_dim));

    uint32 tile_chunk_value = tile_chunk->tiles[tile_chunk_y*world->chunk_dim + tile_chunk_x];
    return(tile_chunk_value);
}

internal uint32 get_tile_chunk_value(World *world, TileChunk *tile_chunk, int32 test_tile_x, int32 test_tile_y)
{
    uint32 tile_chunk_value = 0;
    if(tile_chunk)
    {
        tile_chunk_value = get_tile_value_unchecked(world, tile_chunk, test_tile_x, test_tile_y);
    }
    return(tile_chunk_value);
}

internal inline TileChunk *get_tile_chunk(World *world, int32 tile_chunk_x, int32 tile_chunk_y)
{
    TileChunk *tile_chunk = 0;
    if(tile_chunk_x >= 0 && tile_chunk_x < world->tile_chunk_count_x &&
        tile_chunk_y >= 0 && tile_chunk_y < world->tile_chunk_count_y)
    {
        tile_chunk = &world->tile_chunks[tile_chunk_y*world->tile_chunk_count_x + tile_chunk_x];
    }
    return(tile_chunk);
}

internal inline void re_cannocicalize_coord(World *world, uint32 *tile, real32 *rel_tile)
{
    // TODO: Do something that doesn't use the divide/multiply method for recanalization 
    // because this can end up rounding back on to the tile you just came from


    // TODO: Add bounds checking to prevent wrapping
    int32 offset = floor_real_32_to_int32(*rel_tile / world->tile_side_in_meters);

    // NOTE: world is assumed to be toroidal, if you step off one end, you wrap back around
    // world is not flat...
    *tile += offset;
    *rel_tile -= offset*world->tile_side_in_meters;

    Assert(*rel_tile >= 0);
    Assert(*rel_tile <= world->tile_side_in_meters);
}

internal inline WorldPosition re_cannonicalize_position(World *world, WorldPosition pos)
{
    WorldPosition result = pos;

    // TODO(me): bug in tilemap_count_x?
    re_cannocicalize_coord(world, &result.abs_tile_x, &result.tile_rel_x);
    re_cannocicalize_coord(world, &result.abs_tile_y, &result.tile_rel_y);

    return(result);
}
internal inline TileChunkPosition get_chunk_position(World *world, uint32 abs_tile_x, uint32 abs_tile_y)
{
    TileChunkPosition result;

    result.tile_chunk_x = abs_tile_x >> world->chunk_shift_value;
    result.tile_chunk_y = abs_tile_y >> world->chunk_shift_value;
    result.rel_tile_x = abs_tile_x & world->chunk_mask;
    result.rel_tile_y = abs_tile_y & world->chunk_mask;

    return(result);
}

internal uint32 get_tile_value(World *world, uint32 abs_tile_x, uint32 abs_tile_y)
{
    bool empty = false;

    TileChunkPosition chunk_pos = get_chunk_position(world, abs_tile_x, abs_tile_y);
    TileChunk *tilemap = get_tile_chunk(world, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y);
    uint32 tile_chunk_value = get_tile_chunk_value(world, tilemap, chunk_pos.rel_tile_x, chunk_pos.rel_tile_y);

    return(tile_chunk_value);
}

internal bool is_world_point_empty(World *world, WorldPosition pos)
{
    bool empty = false;

    uint32 tile_chunk_value = get_tile_value(world, pos.abs_tile_x, pos.abs_tile_y);
    empty = (tile_chunk_value == 0);

    return(empty);
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);

#define TILEMAP_COUNT_X 256
#define TILEMAP_COUNT_Y 256

    uint32 temp_tiles[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,  1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1},
        {1,0,0,1, 0,0,1,0, 1,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,1, 1,0,1,0, 1,0,0,0, 0,0,1,0, 1,  1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,1, 0,0,0,0, 1,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,1,0, 1,0,0,0, 0,0,1,0, 0,  0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,1,0, 1,0,0,0, 0,0,0,0, 1,  1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,0, 1},
        {1,0,0,0, 0,0,1,0, 1,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,1,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, 1,  1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1},
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1,  1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1},
        {1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1,  1,1,1,1, 1,1,1,1, 0,1,1,1, 1,1,1,1, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,0,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 0,  0,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,1,0, 0,0,0,0, 0,0,1,0, 1},
        {1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, 1,  1,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,1,0, 1},
        {1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,  1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1},
    };

    TileChunk tile_chunks;
    tile_chunks.tiles = (uint32 *)temp_tiles;

    World world;

    // NOTE: Set by using 256 x 256 tile chunks
    world.chunk_shift_value = 8;
    world.chunk_mask = (1 << world.chunk_shift_value) - 1;
    world.chunk_dim = 256;

    world.tile_side_in_meters = 1.4f;
    world.tile_side_in_pixels = 60;
    world.meters_to_pixels = (real32)world.tile_side_in_pixels / (real32)world.tile_side_in_meters;

    real32 player_height = 1.4f;
    real32 player_width = 0.75f * player_height;

    real32 lowerleft_y = (real32)buffer->height;
    real32 lowerleft_x = -(real32)(world.tile_side_in_pixels/2);

    world.tile_chunk_count_x = 1;
    world.tile_chunk_count_y = 1;

    world.tile_chunks = &tile_chunks;

    GameState *game_state = (GameState *)memory->permanent_storage;
    if(!memory->is_initialized)
    {
        game_state->player_p.abs_tile_x = 10;
        game_state->player_p.abs_tile_y = 3;
        game_state->player_p.tile_rel_x = 0.0f;
        game_state->player_p.tile_rel_y = 0.0f;

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
                d_player_y = 1.0f;
            }
            if(controller->move_down.ended_down)
            {
                d_player_y = -1.0f;
            }
            if(controller->move_left.ended_down)
            {
                d_player_x = -1.0f;
            }
            if(controller->move_right.ended_down)
            {
                d_player_x = 1.0f;
            }
            d_player_x *= 2.0f;
            d_player_y *= 2.0f;

            WorldPosition new_player_p = game_state->player_p;
            new_player_p.tile_rel_x += input->dt_for_frame*d_player_x;
            new_player_p.tile_rel_y += input->dt_for_frame*d_player_y;

            new_player_p = re_cannonicalize_position(&world, new_player_p);
            // TODO: delta function that auto recannonocalize?

            WorldPosition player_left = new_player_p;
            player_left.tile_rel_x -= 0.5f*player_width;
            player_left = re_cannonicalize_position(&world, player_left);

            WorldPosition player_right = new_player_p;
            player_right.tile_rel_x += 0.5f*player_width;
            player_right = re_cannonicalize_position(&world, player_right);

            if(is_world_point_empty(&world, new_player_p) &&
               is_world_point_empty(&world, player_left) &&
               is_world_point_empty(&world, player_right))
            {
                game_state->player_p = new_player_p;
            }
        }
    }

    draw_rectangle(buffer, 0.0f, 0.0f, (real32)buffer->width, (real32)buffer->height, 
                   1.0f, 0.0f, 0.1f);

    real32 center_x = 0.5f*(real32)buffer->width;
    real32 center_y = 0.5f*(real32)buffer->height;

    for(int rel_row = -10;
            rel_row < 10;
            rel_row++)
    {
        for(int rel_column = -20;
                rel_column < 20;
                rel_column++)
        {
            uint32 column = game_state->player_p.abs_tile_x + rel_column;
            uint32 row = game_state->player_p.abs_tile_y + rel_row;
            uint32 tile_id = get_tile_value(&world, column, row);
            real32 gray = 0.5f;
            if(tile_id == 1)
            {
                gray = 1.0f;
            }

            if((column == game_state->player_p.abs_tile_x) && 
               (row == game_state->player_p.abs_tile_y))
            {
                gray = 0.0f;
            }

            real32 min_x = center_x + (real32)((rel_column * world.tile_side_in_pixels)); 
            real32 min_y = center_y - (real32)((rel_row * world.tile_side_in_pixels));
            real32 max_x = min_x + world.tile_side_in_pixels;
            real32 max_y = min_y - world.tile_side_in_pixels;
            draw_rectangle(buffer, min_x, max_y, max_x, min_y,  gray, gray, gray);
        }
    }

    real32 player_r = 1.0f;
    real32 player_g = 1.0f;
    real32 player_b = 0.0f;

    real32 player_left = center_x + world.meters_to_pixels * game_state->player_p.tile_rel_x - 0.5f*world.meters_to_pixels*player_width;
    real32 player_top = center_y - world.meters_to_pixels * game_state->player_p.tile_rel_y - world.meters_to_pixels * player_height;

    draw_rectangle(buffer,
                   player_left, player_top,
                   player_left + world.meters_to_pixels * player_width,
                   player_top + world.meters_to_pixels * player_height,
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
*/
