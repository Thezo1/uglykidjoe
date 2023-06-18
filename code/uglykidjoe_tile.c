#include "uglykidjoe_tile.h"

internal inline TileChunk *get_tile_chunk(TileMap *tilemap, uint32 tile_chunk_x, uint32 tile_chunk_y, uint32 tile_chunk_z)
{
    TileChunk *tile_chunk = 0;
    if((tile_chunk_x >= 0) && (tile_chunk_x < tilemap->tile_chunk_count_x) &&
        (tile_chunk_y >= 0) && (tile_chunk_y < tilemap->tile_chunk_count_y) &&
        (tile_chunk_z >= 0) && (tile_chunk_z < tilemap->tile_chunk_count_z))
    {
        tile_chunk = &tilemap->tile_chunks
        [
            tile_chunk_z*tilemap->tile_chunk_count_y*tilemap->tile_chunk_count_x +
            tile_chunk_y*tilemap->tile_chunk_count_x + 
            tile_chunk_x
        ];
    }
    return(tile_chunk);
}
internal inline uint32 get_tile_value_unchecked(TileMap *tilemap, TileChunk *tile_chunk, uint32 tile_chunk_x, uint32 tile_chunk_y)
{
    Assert(tile_chunk);
    Assert((tile_chunk_x < tilemap->chunk_dim) &&
           (tile_chunk_y < tilemap->chunk_dim));

    uint32 tile_chunk_value = tile_chunk->tiles[tile_chunk_y*tilemap->chunk_dim + tile_chunk_x];
    return(tile_chunk_value);
}

internal inline void set_tile_value_unchecked(TileMap *tilemap, TileChunk *tile_chunk, uint32 tile_chunk_x, uint32 tile_chunk_y, uint32 tile_value)
{
    Assert(tile_chunk);
    Assert((tile_chunk_x < tilemap->chunk_dim) &&
           (tile_chunk_y < tilemap->chunk_dim));

    tile_chunk->tiles[tile_chunk_y*tilemap->chunk_dim + tile_chunk_x] = tile_value;
}

internal uint32 get_tile_chunk_value(TileMap *tilemap, TileChunk *tile_chunk, int32 test_tile_x, int32 test_tile_y)
{
    uint32 tile_chunk_value = 0;
    if(tile_chunk && tile_chunk->tiles)
    {
        tile_chunk_value = get_tile_value_unchecked(tilemap, tile_chunk, test_tile_x, test_tile_y);
    }
    return(tile_chunk_value);
}

internal inline void set_tile_chunk_value(TileMap *tilemap, TileChunk *tile_chunk, int32 test_tile_x, int32 test_tile_y, uint32 tile_value)
{
    if(tile_chunk && tile_chunk->tiles)
    {
        set_tile_value_unchecked(tilemap, tile_chunk, test_tile_x, test_tile_y, tile_value);
    }
}

internal inline TileChunkPosition get_chunk_position(TileMap *tilemap, uint32 abs_tile_x, uint32 abs_tile_y, uint32 abs_tile_z)
{
    TileChunkPosition result;

    result.tile_chunk_x = abs_tile_x >> tilemap->chunk_shift_value;
    result.tile_chunk_y = abs_tile_y >> tilemap->chunk_shift_value;
    result.tile_chunk_z = abs_tile_z;
    result.rel_tile_x = abs_tile_x & tilemap->chunk_mask;
    result.rel_tile_y = abs_tile_y & tilemap->chunk_mask;

    return(result);
}

internal uint32 get_tile_value(TileMap *tilemap, uint32 abs_tile_x, uint32 abs_tile_y, uint32 abs_tile_z)
{
    TileChunkPosition chunk_pos = get_chunk_position(tilemap, abs_tile_x, abs_tile_y, abs_tile_z);
    TileChunk *tile_chunk = get_tile_chunk(tilemap, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);
    uint32 tile_chunk_value = get_tile_chunk_value(tilemap, tile_chunk, chunk_pos.rel_tile_x, chunk_pos.rel_tile_y);

    return(tile_chunk_value);
}

internal uint32 get_tile_value_(TileMap *tilemap, TileMapPosition pos)
{
    uint32 tile_chunk_value = get_tile_value(tilemap, pos.abs_tile_x , pos.abs_tile_y, pos.abs_tile_z);
    return(tile_chunk_value);
}

internal bool is_tilemap_point_empty(TileMap *tilemap, TileMapPosition pos)
{
    bool empty = false;

    uint32 tile_chunk_value = get_tile_value(tilemap, pos.abs_tile_x, pos.abs_tile_y, pos.abs_tile_z);
    empty = (tile_chunk_value == 1) ||
        (tile_chunk_value == 3)||
        (tile_chunk_value == 4);

    return(empty);
}

internal inline void re_cannocicalize_coord(TileMap *tilemap, uint32 *tile, real32 *rel_tile)
{
    // TODO: Do something that doesn't use the divide/multiply method for recanalization 
    // because this can end up rounding back on to the tile you just came from


    // TODO: Add bounds checking to prevent wrapping
    int32 offset = round_real32_to_int32(*rel_tile / tilemap->tile_side_in_meters);

    // NOTE: tilemap is assumed to be toroidal, if you step off one end, you wrap back around
    // tilemap is not flat...
    *tile += offset;
    *rel_tile -= offset*tilemap->tile_side_in_meters;

    Assert(*rel_tile >= -0.5f*tilemap->tile_side_in_meters)
    Assert(*rel_tile <= 0.5f*tilemap->tile_side_in_meters);
}

internal inline TileMapPosition re_cannonicalize_position(TileMap *tilemap, TileMapPosition pos)
{
    TileMapPosition result = pos;

    // TODO(me): bug in tilemap_count_x?
    re_cannocicalize_coord(tilemap, &result.abs_tile_x, &result.offset_x);
    re_cannocicalize_coord(tilemap, &result.abs_tile_y, &result.offset_y);

    return(result);
}

internal void set_tile_value(MemoryArena *arena, TileMap *tilemap, uint32 abs_tile_x, uint32 abs_tile_y, uint32 abs_tile_z, uint32 tile_value)
{
    TileChunkPosition chunk_pos = get_chunk_position(tilemap, abs_tile_x, abs_tile_y, abs_tile_z);
    TileChunk *tile_chunk = get_tile_chunk(tilemap, chunk_pos.tile_chunk_x, chunk_pos.tile_chunk_y, chunk_pos.tile_chunk_z);

    Assert(tile_chunk);
    if(!tile_chunk->tiles)
    {
        uint32 tile_count = tilemap->chunk_dim*tilemap->chunk_dim;
        tile_chunk->tiles = push_array(arena, tile_count, uint32);

        for(uint32 tile_index = 0;
                tile_index < tile_count;
                tile_index++)
        {
            tile_chunk->tiles[tile_index] = 1;
        }
    }
    set_tile_chunk_value(tilemap, tile_chunk, chunk_pos.rel_tile_x, chunk_pos.rel_tile_y, tile_value);
}

internal bool are_on_same_tile(TileMapPosition *a, TileMapPosition *b)
{
    bool result = ((a->abs_tile_x == b->abs_tile_x) &&
                   (a->abs_tile_y == b->abs_tile_y) &&
                   (a->abs_tile_z == b->abs_tile_z));
    return(result);
}
