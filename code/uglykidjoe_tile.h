#ifndef _UGLYKIDJOE_TILE_H_
#define _UGLYKIDJOE_TILE_H_

typedef struct TileChunk
{
    uint32 *tiles;
}TileChunk;

typedef struct TileChunkPosition
{
    uint32 tile_chunk_x;
    uint32 tile_chunk_y;
    uint32 tile_chunk_z;

    uint32 rel_tile_x;
    uint32 rel_tile_y;
}TileChunkPosition;

typedef struct TileMap
{
    real32 tile_side_in_meters;

    int32 chunk_dim;
    int32 chunk_shift_value;
    int32 chunk_mask;

    uint32 tile_chunk_count_x;
    uint32 tile_chunk_count_y;
    uint32 tile_chunk_count_z;

    TileChunk *tile_chunks;
}TileMap;

typedef struct TileMapPosition
{
    // NOTE(): These are fixed point tile locations
    // the high bit stores the tile chunk world index
    // and the low bit stores tile index in a chunk
    // eliminates the tile map and introduces chunks (0-256)
    uint32 abs_tile_x;
    uint32 abs_tile_y;
    uint32 abs_tile_z;

    // NOTE(): These are the offsets from tile center
    v2 offset; 
}TileMapPosition;

typedef struct TileMapDifference
{
    v2 dxy;
    real32 dz;
}TileMapDifference;

#endif
