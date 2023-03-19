#include "uglykidjoe.h"
#include "uglykidjoe_tile.c"
#include "uglykidjoe_random.h"

internal void draw_rectangle(GameOffScreenBuffer *buffer, 
                            real32 real_min_x, real32 real_min_y,
                            real32 real_max_x, real32 real_max_y,
                            real32 r, real32 g, real32 b)
{
    int32 min_x = round_real32_to_int32(real_min_x);
    int32 min_y = round_real32_to_int32(real_min_y);
    int32 max_x = round_real32_to_int32(real_max_x);
    int32 max_y = round_real32_to_int32(real_max_y);

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
    uint32 color = (uint32)((round_real32_to_uint32(r*255.0f) << 16)|
                            (round_real32_to_uint32(g*255.0f) << 8)|
                            (round_real32_to_uint32(b*255.0f) << 0));

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

internal void initialize_arena(MemoryArena *arena, memory_index size, uint8 *base)
{
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);

    real32 player_height = 1.4f;
    real32 player_width = 0.75f * player_height;

    GameState *game_state = (GameState *)memory->permanent_storage;
    if(!memory->is_initialized)
    {
        game_state->player_p.abs_tile_x = 10;
        game_state->player_p.abs_tile_y = 3;
        game_state->player_p.offset_x = 0.0f;
        game_state->player_p.offset_y = 0.0f;

        initialize_arena(&game_state->world_arena, memory->permanent_storage_size - sizeof(GameState),
                         (uint8 *)memory->permanent_storage + sizeof(GameState));

        game_state->world = push_struct(&game_state->world_arena, World);
        World *world = game_state->world;
        world->tilemap = push_struct(&game_state->world_arena, TileMap);

        TileMap *tilemap = world->tilemap;

        // NOTE: Set by using 256 x 256 tile chunks
        tilemap->chunk_shift_value = 4;
        tilemap->chunk_mask = (1 << tilemap->chunk_shift_value) - 1;
        tilemap->chunk_dim = (1 << tilemap->chunk_shift_value);

        tilemap->tile_side_in_meters = 1.4f;

        tilemap->tile_chunk_count_x = 128;
        tilemap->tile_chunk_count_y = 128;
        tilemap->tile_chunk_count_z = 2;
        tilemap->tile_chunks = push_array(&game_state->world_arena, tilemap->tile_chunk_count_x * tilemap->tile_chunk_count_y * tilemap->tile_chunk_count_z, 
                                            TileChunk);

        uint32 random_number_index = 0;
        uint32 tiles_per_width = 17;
        uint32 tiles_per_height = 9;
        uint32 screen_x = 0;
        uint32 screen_y = 0;
        uint32 abs_tile_z = 0;

        bool door_left  = false;
        bool door_right  = false;
        bool door_top  = false;
        bool door_bottom  = false;
        bool door_up = false;
        bool door_down = false;
        for(uint32 screen_index = 0;
                screen_index < 100;
                screen_index++)
        {
            Assert(random_number_index < ArrayCount(random_number_table));

            // TODO(me): account for the random placement of screens
            uint32 random_choice; 
            if(door_up || door_down)
            {
                random_choice = random_number_table[random_number_index++] % 2;
            }
            else
            {
                random_choice = random_number_table[random_number_index++] % 3;
            }

            bool created_z_door = false;
            if(random_choice == 2)
            {
                created_z_door = true;
                if(abs_tile_z == 0) 
                {
                    door_up = true;
                }
                else
                {
                    door_down = true;
                }
            }
            else if(random_choice == 1)
            {
                door_right = true;
            }
            else
            {
                door_top = true;
            }

            for(uint32 tile_y = 0;
                    tile_y < tiles_per_height;
                    tile_y++)
            {

                for(uint32 tile_x = 0;
                        tile_x < tiles_per_width;
                        tile_x++)
                {
                    uint32 abs_tile_x = screen_x*tiles_per_width + tile_x;
                    uint32 abs_tile_y = screen_y*tiles_per_height + tile_y;

                    uint32 tile_value = 1;
                    if(tile_x == 0 && (!door_left || (tile_y != (tiles_per_height/2))))
                    {
                        tile_value = 2;
                    }

                    if(tile_x == (tiles_per_width - 1) && (!door_right || (tile_y != (tiles_per_height/2))))
                    {
                        tile_value = 2;
                    }

                    if(tile_y == 0 && (!door_bottom || (tile_x != (tiles_per_width/2))))
                    {
                        tile_value = 2;
                    }

                    if(tile_y == (tiles_per_height - 1) && (!door_top || (tile_x != (tiles_per_width/2))))
                    {
                        tile_value = 2;
                    }

                    if((tile_x == 10) && (tile_y == 6))
                    {
                        if(door_up)
                        {
                            tile_value = 3;
                        }
                        if(door_down)
                        {
                            tile_value = 4;
                        }
                    }
                    set_tile_value(&game_state->world_arena, world->tilemap, abs_tile_x, abs_tile_y, abs_tile_z, tile_value);
                }

            }

            door_left  = door_right;
            door_bottom  = door_top;

            if(created_z_door)
            {
                door_down = !door_down;
                door_up = !door_up;
            }
            else
            {
                door_up = false;
                door_down = false;
            }

            door_right  = false;
            door_top  = false;

            if(random_choice == 2)
            {
                if(abs_tile_z == 0) 
                {
                    abs_tile_z = 1;
                }
                else
                {
                    abs_tile_z = 0;
                }
            }

            else if(random_choice == 1)
            {
                screen_x += 1;
            }
            else
            {
                screen_y += 1;
            }
        }
        memory->is_initialized = true;
    }

    World *world = game_state->world;
    TileMap *tilemap = world->tilemap;

    int32 tile_side_in_pixels = 60;
    real32 meters_to_pixels = (real32)tile_side_in_pixels / (real32)tilemap->tile_side_in_meters;

    real32 lowerleft_y = (real32)buffer->height;
    real32 lowerleft_x = -(real32)(tile_side_in_pixels/2);

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

            real32 player_speed = 2.0f;
            if(controller->action_up.ended_down)
            {
                player_speed = 10.0f;
            }

            d_player_x *= player_speed;
            d_player_y *= player_speed;

            TileMapPosition new_player_p = game_state->player_p;
            new_player_p.offset_x += input->dt_for_frame*d_player_x;
            new_player_p.offset_y += input->dt_for_frame*d_player_y;

            new_player_p = re_cannonicalize_position(tilemap, new_player_p);
            // TODO: delta function that auto recannonocalize?

            TileMapPosition player_left = new_player_p;
            player_left.offset_x -= 0.5f*player_width;
            player_left = re_cannonicalize_position(tilemap, player_left);

            TileMapPosition player_right = new_player_p;
            player_right.offset_x += 0.5f*player_width;
            player_right = re_cannonicalize_position(tilemap, player_right);

            if(is_tilemap_point_empty(tilemap, new_player_p) &&
               is_tilemap_point_empty(tilemap, player_left) &&
               is_tilemap_point_empty(tilemap, player_right))
            {
                if(!are_on_same_tile(&game_state->player_p, &new_player_p))
                {
                    uint32 new_tile_value = get_tile_value_(tilemap, new_player_p);
                    if(new_tile_value == 3)
                    {
                        new_player_p.abs_tile_z++;
                    }
                    else if(new_tile_value == 4)
                    {
                        new_player_p.abs_tile_z--;
                    }
                }
                game_state->player_p = new_player_p;
            }
        }
    }

    draw_rectangle(buffer, 0.0f, 0.0f, (real32)buffer->width, (real32)buffer->height, 
                   1.0f, 0.0f, 0.1f);

    real32 screen_center_x = 0.5f*(real32)buffer->width;
    real32 screen_center_y = 0.5f*(real32)buffer->height;

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
            uint32 tile_id = get_tile_value(tilemap, column, row, game_state->player_p.abs_tile_z);
            if(tile_id > 0)
            {
                real32 gray = 0.5f;
                if(tile_id == 2)
                {
                    gray = 1.0f;
                }

                if(tile_id == 3)
                {
                    gray = 0.2f;
                }
                if(tile_id == 4)
                {
                    gray = 0.8f;
                }

                if((column == game_state->player_p.abs_tile_x) && 
                   (row == game_state->player_p.abs_tile_y))
                {
                    gray = 0.0f;
                }

                real32 center_x = screen_center_x - meters_to_pixels * game_state->player_p.offset_x + (real32)((rel_column * tile_side_in_pixels)); 
                real32 center_y = screen_center_y + meters_to_pixels * game_state->player_p.offset_y - (real32)((rel_row * tile_side_in_pixels));
                real32 min_x = center_x - 0.5*tile_side_in_pixels;
                real32 min_y = center_y - 0.5*tile_side_in_pixels;
                real32 max_x = center_x + 0.5*tile_side_in_pixels;
                real32 max_y = center_y + 0.5*tile_side_in_pixels;
                draw_rectangle(buffer, min_x, min_y, max_x, max_y,  gray, gray, gray);
            }
        }
    }

    real32 player_r = 1.0f;
    real32 player_g = 1.0f;
    real32 player_b = 0.0f;

    real32 player_left = screen_center_x  - 0.5f*meters_to_pixels*player_width;
    real32 player_top = screen_center_y  - meters_to_pixels * player_height;

    draw_rectangle(buffer,
                   player_left, player_top,
                   player_left + meters_to_pixels * player_width,
                   player_top + meters_to_pixels * player_height,
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
