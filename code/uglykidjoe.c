#include "uglykidjoe.h"
#include "uglykidjoe_tile.c"
#include "uglykidjoe_random.h"
#include "uglykidjoe_math.h"

#pragma pack(push, 1)
typedef struct BitMapHeader
{
    uint16   FileType;     
    uint32  FileSize;     
    uint16   Reserved1;    
    uint16   Reserved2;    
    uint32  BitmapOffset;
    uint32 Size;
    int32  Width;
    int32  Height;
    uint16  Planes;
    uint16  BitsPerPixel;

    uint32 Compression;
	uint32 SizeOfBitmap;
	int32  HorzResolution;
	int32  VertResolution; 
	uint32 ColorsUsed;
	uint32 ColorsImportant;

    uint32 RedMask;
    uint32 GreenMask;
    uint32 BlueMask;

}BitMapHeader;
#pragma pack(pop)

internal LoadedBitMap DEBUG_load_bmp(ThreadContext *thread, debug_platform_read_entire_file *read_entire_file,
                             char *filename)
{
    LoadedBitMap result = {};
    DEBUG_ReadFileResult read_result = read_entire_file(thread, filename);

    if(read_result.content_size != 0)
    {
        BitMapHeader *header = (BitMapHeader *)read_result.contents;
        Assert(header->Compression == 3);

        // NOTE(me): Experiment with loading a 24 bits per pixel image
        // This is a 32 bits per pixel image

        result.pixels = (uint32 *)((uint8 *)read_result.contents + header->BitmapOffset);
        result.height = header->Height;
        result.width = header->Width;

        // NOTE: The byte order in memory is decided by the header
        // so we have to read out the masks and convert the pixels
        uint32 red_mask = header->RedMask;
        uint32 green_mask = header->GreenMask;
        uint32 blue_mask = header->BlueMask;
        uint32 alpha_mask = ~(red_mask | green_mask | blue_mask);

        BitScanResult red_shift = find_least_significant_set_bit(red_mask);
        BitScanResult green_shift = find_least_significant_set_bit(green_mask);
        BitScanResult blue_shift = find_least_significant_set_bit(blue_mask);
        BitScanResult alpha_shift = find_least_significant_set_bit(alpha_mask);

        Assert(red_shift.found);
        Assert(green_shift.found);
        Assert(blue_shift.found);
        Assert(alpha_shift.found);

        uint32 *source_dest = result.pixels;
        for(int32 y = 0;
                y < header->Height;
                ++y)
        {
            for(int32 x = 0;
                    x < header->Width;
                    ++x)
            {
                uint32 c = *source_dest;
                *source_dest++ = ((((c >> alpha_shift.index) & 0xFF) << 24) | 
                                  (((c >> red_shift.index) & 0xFF) << 16) |
                                  (((c >> green_shift.index) & 0xFF) << 8) |
                                  (((c >> blue_shift.index) & 0xFF) << 0));
            }
        }
    }

    return(result);
}

internal void game_output_sound(GameSoundOutputBuffer *sound_buffer, int tone_hz)
{
    int16 *sample_out = sound_buffer->samples;
    for(int sample_index = 0;
        sample_index < sound_buffer->sample_count;
        ++sample_index)
    {
        int16 sample_value = 0;
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;

        ++sample_value;
    }
}

internal void initialize_arena(MemoryArena *arena, memory_index size, uint8 *base)
{
    arena->size = size;
    arena->base = base;
    arena->used = 0;
}

internal void draw_rectangle(GameOffScreenBuffer *buffer, 
        v2 v_min, v2 v_max,
        real32 r, real32 g, real32 b)
{
    int32 min_x = round_real32_to_int32(v_min.x);
    int32 min_y = round_real32_to_int32(v_min.y);
    int32 max_x = round_real32_to_int32(v_max.x);
    int32 max_y = round_real32_to_int32(v_max.y);

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

internal void draw_bitmap(GameOffScreenBuffer *buffer, LoadedBitMap *bitmap, 
        real32 real_x, real32 real_y, 
        int32 align_x, int32 align_y)
{
    real_x -= (real32)align_x;
    real_y -= (real32)align_y;

    int32 min_x = round_real32_to_int32(real_x);
    int32 min_y = round_real32_to_int32(real_y);
    int32 max_x = round_real32_to_int32(real_x + bitmap->width);
    int32 max_y = round_real32_to_int32(real_y + bitmap->height);

    int32 blit_width = bitmap->width;
    int32 blit_height = bitmap->height;

    int source_offset_x = 0;
    if(min_x < 0)
    {
        source_offset_x = -min_x;
        min_x = 0;
    }

    int32 source_offset_y = 0;
    if(min_y < 0)
    {
        source_offset_y = -min_y;
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

    // TODO: source_row needs to be changed based on clipping
    uint32 *source_row = bitmap->pixels + bitmap->width*(bitmap->height - 1);
    source_row += -source_offset_y*bitmap->width + source_offset_x;
    uint8 *dest_row = ((uint8 *)buffer->memory +
                        min_x*buffer->bytes_per_pixel +
                        min_y*buffer->pitch);

    for(int32 y = min_y;
            y < max_y;
            y++)
    {
        uint32 *dest = (uint32 *)dest_row;
        uint32 *source = source_row;
        for(int32 x = min_x;
                x < max_x;
                x++)
        {
            real32 a = ((*source >> 24) & 0xFF) / 255.0f;
            real32 source_r = ((*source >> 16) & 0xFF);
            real32 source_g = ((*source >> 8) & 0xFF);
            real32 source_b = ((*source >> 0) & 0xFF);

            real32 dest_r = ((*dest >> 16) & 0xFF);
            real32 dest_g = ((*dest >> 8) & 0xFF);
            real32 dest_b = ((*dest >> 0) & 0xFF);

            real32 r = (1.0f-a)*dest_r + a*source_r;
            real32 g = (1.0f-a)*dest_g + a*source_g;
            real32 b = (1.0f-a)*dest_b + a*source_b;

            *dest = (((uint32)(r + 0.5f) << 16) | 
                    ((uint32)(g + 0.5f) << 8) | 
                    ((uint32)(b + 0.5f) << 0));
            ++dest;
            ++source;
        }
        dest_row += buffer->pitch;
        source_row -= bitmap->width;
    }
}

internal void move_player(GameState *game_state, Entity *entity, real32 dt, v2 dd_player)
{
    TileMap *tilemap = game_state->world->tilemap;
    if((dd_player.x != 0.0f) && (dd_player.y != 0.0f))
    {
        dd_player = v2_scalar_mul(dd_player, 0.7071067811865475244f);

    }
    real32 player_speed = 50.0f;
    dd_player = v2_scalar_mul(dd_player, player_speed);
    dd_player = v2_add(v2_scalar_mul(entity->dp, -8.0f), dd_player);
            
    // Player colision
    TileMapPosition old_player_p = entity->p;
    TileMapPosition new_player_p = old_player_p;
            
    // (0.5 * a * t^2) + (v * t) + p
    v2 player_delta = v2_add(v2_scalar_mul(v2_scalar_mul(dd_player, 0.5f), square(dt)), 
                             v2_scalar_mul(entity->dp, dt));
    new_player_p.offset = v2_add(new_player_p.offset, player_delta);

    // (a * t) + (p)
    entity->dp = v2_add(v2_scalar_mul(dd_player, dt), entity->dp);

    new_player_p = re_cannonicalize_position(tilemap, new_player_p);
            
    // TODO: delta function that auto recannonocalize?
            
#if 1            
    TileMapPosition player_left = new_player_p;
    player_left.offset.x -= 0.5f*entity->width;
    player_left = re_cannonicalize_position(tilemap, player_left);

    TileMapPosition player_right = new_player_p;
    player_right.offset.x += 0.5f*entity->width;
    player_right = re_cannonicalize_position(tilemap, player_right);

    bool collided = false;
    TileMapPosition col_p = {};
    if(!is_tilemap_point_empty(tilemap, new_player_p))
    {
        col_p = new_player_p;
        collided = true;
    }

    if(!is_tilemap_point_empty(tilemap, player_left))
    {
        col_p = player_left;
        collided = true;
    }

    if(!is_tilemap_point_empty(tilemap, player_right))
    {
        col_p = player_right;
        collided = true;
    }

    if(collided)
    {
        v2 r = {0.0f, 0.0f};
        if(col_p.abs_tile_x < entity->p.abs_tile_x)
        {
            r = V2(1.0f, 0.0f);
        }

        if(col_p.abs_tile_y < entity->p.abs_tile_y)
        {
            r = V2(0.0f, 1.0f);
        }

        if(col_p.abs_tile_x > entity->p.abs_tile_x)
        {
            r = V2(-1.0f, 0.0f);
        }

        if(col_p.abs_tile_y > entity->p.abs_tile_y)
        {
            r = V2(0.0f, -1.0f);
        }

        entity->dp = v2_sub(entity->dp,  v2_scalar_mul(r, 1 * inner(entity->dp, r)));
    }
    else
    {
        entity->p = new_player_p;
    }
#else
    uint32 min_tile_x = 0;
    uint32 min_tile_y = 0;
    uint32 one_past_max_tile_x = 0;
    uint32 one_past_max_tile_y = 0;
    uint32 abs_tile_z = game_state->player_p.abs_tile_z;
    TileMapPosition best_player_p = game_state->player_p;
    real32 best_difference = length_sq(player_delta);
            
    for(uint32 abs_tile_y = min_tile_y;
        abs_tile_y != one_past_max_tile_y;
        ++abs_tile_y)
    {
        for(uint32 abs_tile_x = min_tile_x;
            abs_tile_x != one_past_max_tile_x;
            ++abs_tile_x)
        {

            TileMapPosition test_tile_p = centered_tile_point(abs_tile_x, abs_tile_y, abs_tile_z);
            uint32 tile_value = get_tile_value_(tilemap, test_tile_p);
                    
            if(is_tile_value_empty(tile_value))
            {
                v2 min_corner = {(-0.5 * tilemap->tile_side_in_meters), (-0.5f * tilemap->tile_side_in_meters)};
                v2 max_corner = {(0.5 * tilemap->tile_side_in_meters), (0.5f * tilemap->tile_side_in_meters)};
                        
                TileMapDifference rel_new_player_p = subtract_in_real32(tilemap, &test_tile_p, &new_player_p);
                v2 test_p = closest_point_in_rectangle(min_corner, max_corner, rel_new_player_p);

                if(...)
                {
                }
            }
        }
    }

    if(!are_on_same_tile(&old_player_p, &game_state->player_p))
    {
        uint32 new_tile_value = get_tile_value_(tilemap, game_state->player_p);
        if(new_tile_value == 3)
        {
            ++game_state->player_p.abs_tile_z;
        }
        else if(new_tile_value == 4)
        {
            --game_state->player_p.abs_tile_z;
        }
    }

#endif
}

inline internal Entity *get_entity(GameState *game_state, uint32 index)
{
    Entity *entity = 0;
    // Assert();
    if((index >0) && (index < ArrayCount(game_state->entities)))
    {
        entity = &game_state->entities[index];
    }
    return(entity);
}

internal void initialize_player(Entity *entity)
{
    entity->exists = true;
    entity->p.abs_tile_x = 1;
    entity->p.abs_tile_y = 3;
    entity->p.offset.x = 5.0f;
    entity->p.offset.y = 5.0f;
    entity->height = 1.4f;
    entity->width = 0.75f * entity->height;

}

internal uint32 add_entity(GameState *game_state)
{
    uint32 entity_index = game_state->entity_count++;
    Assert(game_state->entity_count < ArrayCount(game_state->entities));
    Entity *entity = &game_state->entities[entity_index];
    *entity = (Entity){ 0 };
    return(entity_index);
}

GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((sizeof(GameState)) <= memory->permanent_storage_size);

    GameState *game_state = (GameState *)memory->permanent_storage;
    if(!memory->is_initialized)
    {
        // NOTE: Reserve slot zero for the null entity index
        add_entity(game_state);
        game_state->background = DEBUG_load_bmp(thread, memory->DEBUG_platform_read_entire_file, "test/background001.bmp");

        HeroineBitmaps *bitmap;
        bitmap = game_state->heroine_bitmaps;
        bitmap->base = DEBUG_load_bmp(thread, memory->DEBUG_platform_read_entire_file, "test/base0.bmp");
        bitmap->cape = DEBUG_load_bmp(thread, memory->DEBUG_platform_read_entire_file, "test/armor0.bmp");
        bitmap->weapon = DEBUG_load_bmp(thread, memory->DEBUG_platform_read_entire_file, "test/hammer0.bmp");
        bitmap->align_x = 79;
        bitmap->align_y = 105;

        game_state->camera_p.abs_tile_x = 17/2;
        game_state->camera_p.abs_tile_y = 9/2;

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
    real32 lowerleft_x = -((real32)tile_side_in_pixels/2.0f);
    

    for(int controller_index = 0;
        controller_index < ArrayCount(input->controllers);
        ++controller_index)
    {
        GameControllerInput *controller = get_controller(input, controller_index);
        Entity *controlling_entity = get_entity(game_state, game_state->player_index_for_controller[controller_index]);
        
        if(controlling_entity)
        {
            v2 dd_player = {};
            if(controller->is_analog)
            {
                // TODO: if windows y is normal, push this negation up to the sdl code
                dd_player = V2(controller->stick_average_x, -controller->stick_average_y);
            }
            else
            {
                if(controller->move_up.ended_down)
                {
                    dd_player.y = 1.0f;
                }
                if(controller->move_down.ended_down)
                {
                    dd_player.y = -1.0f;
                }
                if(controller->move_left.ended_down)
                {
                    dd_player.x = -1.0f;
                }
                if(controller->move_right.ended_down)
                {
                    dd_player.x = 1.0f;
                }
            }
            move_player(game_state, controlling_entity, input->dt_for_frame, dd_player);
        }
        else
        {
            if(controller->start.ended_down)
            {
                uint32 entity_index = add_entity(game_state);
                controlling_entity = get_entity(game_state, entity_index);
                initialize_player(controlling_entity);
                game_state->player_index_for_controller[controller_index] = entity_index;
            }
        }
    }

    Entity *camera_following_entity = get_entity(game_state, game_state->camera_following_entity_index);
    if(camera_following_entity)
    {
        game_state->camera_p.abs_tile_z = camera_following_entity->p.abs_tile_z;
        
        TileMapDifference diff = subtract_in_real32(tilemap, &camera_following_entity->p, &game_state->camera_p);
        if(diff.dxy.x > (9.0f*tilemap->tile_side_in_meters))
        {
            game_state->camera_p.abs_tile_x += 17;
        }
        if(diff.dxy.x < -(9.0f*tilemap->tile_side_in_meters))
        {
            game_state->camera_p.abs_tile_x -= 17;
        }

        if(diff.dxy.y > (5.0f*tilemap->tile_side_in_meters))
        {
            game_state->camera_p.abs_tile_y += 9;
        }
        if(diff.dxy.y < -(5.0f*tilemap->tile_side_in_meters))
        {
            game_state->camera_p.abs_tile_y -= 9;
        }

    }

    draw_bitmap(buffer, &game_state->background, 0, 0, 0, 0);

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
            uint32 column = game_state->camera_p.abs_tile_x + rel_column;
            uint32 row = game_state->camera_p.abs_tile_y + rel_row;
            uint32 tile_id = get_tile_value(tilemap, column, row, game_state->camera_p.abs_tile_z);
            if(tile_id > 1)
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

                if((column == game_state->camera_p.abs_tile_x) && 
                   (row == game_state->camera_p.abs_tile_y))
                {
                    gray = 0.0f;
                }

                v2 tile_side = { 0.5*tile_side_in_pixels, 0.5*tile_side_in_pixels };

                v2 center = { screen_center_x - meters_to_pixels * game_state->camera_p.offset.x + (real32)((rel_column * tile_side_in_pixels)),
                        screen_center_y + meters_to_pixels * game_state->camera_p.offset.y - (real32)((rel_row * tile_side_in_pixels)) };

                v2 min = v2_sub(center, tile_side);
                v2 max = v2_add(center, tile_side);

                draw_rectangle(buffer, min, max, gray, gray, gray);
            }
        }
    }

    Entity *entity = game_state->entities;
    for(uint32 entity_index = 0;
        entity_index < game_state->entity_count;
        ++entity_index, ++entity)
    {
        if(entity->exists)
        {
            TileMapDifference diff = subtract_in_real32(tilemap, &entity->p, &game_state->camera_p);
    
            real32 player_r = 1.0f;
            real32 player_g = 1.0f;
            real32 player_b = 0.0f;

            real32 player_ground_point_x = screen_center_x + meters_to_pixels*diff.dxy.x;
            real32 player_ground_point_y = screen_center_y - meters_to_pixels*diff.dxy.y;

            real32 player_left = player_ground_point_x  - 0.5f*meters_to_pixels*entity->width;
            real32 player_top = player_ground_point_y  - meters_to_pixels * entity->height;

            v2 player_left_top = { player_left, player_top };
            v2 player_width_height = { player_left + meters_to_pixels*entity->width, player_top + meters_to_pixels*entity->height };

            draw_rectangle(buffer, player_left_top, player_width_height, player_r, player_g, player_b);

            HeroineBitmaps *heroine_bitmaps = &game_state->heroine_bitmaps[0];
            draw_bitmap(buffer, &heroine_bitmaps->base, player_ground_point_x, 
                        player_ground_point_y, heroine_bitmaps->align_x, heroine_bitmaps->align_y);

            draw_bitmap(buffer, &heroine_bitmaps->cape, player_ground_point_x, 
                        player_ground_point_y, heroine_bitmaps->align_x, heroine_bitmaps->align_y);

            draw_bitmap(buffer, &heroine_bitmaps->weapon, player_ground_point_x, 
                        player_ground_point_y, heroine_bitmaps->align_x, heroine_bitmaps->align_y);
        }
    }
}

GAME_GET_SOUND_SAMPLES(GameGetSoundSamples)
{
    GameState *game_state = (GameState *)memory->permanent_storage;

    game_output_sound(sound_buffer, 400);
}
