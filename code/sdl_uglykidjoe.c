#include<stdio.h>
#include<sys/mman.h>
#include<SDL2/SDL.h>
#include<x86intrin.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<dlfcn.h>
#include<errno.h>

#include "uglykidjoe.h"
#include "sdl_uglykidjoe.h"

global_variable SDL_OffScreenBuffer global_back_buffer;
global_variable int64 global_perf_count_frequency;
global_variable bool global_pause;

SDL_GameController *controller_handles[MAX_CONTROLLERS];
SDL_Haptic *rumble_handles[MAX_CONTROLLERS];
RingBuffer audio_ring_buffer;

/*
    TODO:(me) change from sdl audio to a lower level library.
    ALSA most likely or OSS.
*/

// NOTE:(me) copied from stack overflow, 
// so don't ask me how I wrote it.
internal int copy_file(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0)
        return -1;

    // NOTE:(me) O_CREAT on it's own creates the file,
    // and overwrites contents if file exists. 
    // Combining with O_EXCL jsut errors out without overwriting
    fd_to = open(to, O_WRONLY | O_CREAT, 0666);
    if (fd_to < 0)
        goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0)
    {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0)
    {
        if (close(fd_to) < 0)
        {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

  out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0)
        close(fd_to);

    errno = saved_errno;
    return -1;
}

internal struct timespec sdl_get_last_write_time(char *path)
{
    struct stat stat_buf = {};
    uint32 stat_result = stat(path, &stat_buf);
    struct timespec result = {};
    if(stat_result != 0)
    {
        printf("Unalbe to stat game source path: %s", path);
        exit(1);
    }

    result = stat_buf.st_mtim; 
    return (result);
}

internal void sdl_load_game_code(SDL_GameCode *game_code, char *source_name, char *temp_name)
{
    game_code->last_write_time = sdl_get_last_write_time(source_name);

    char *temp_so_name = temp_name;
    copy_file(temp_name, source_name);

    game_code->game_so = dlopen(temp_name, RTLD_LAZY);
    if(game_code->game_so)
    {
        game_code->update_and_render = (game_update_and_render*)
            dlsym(game_code->game_so, "GameUpdateAndRender");
        game_code->get_sound_samples = (game_get_sound_samples*)
            dlsym(game_code->game_so, "GameGetSoundSamples");

        game_code->is_valid = (game_code->update_and_render && 
            game_code->get_sound_samples);

        if (!game_code->is_valid)
        {
            game_code->update_and_render = 0;
            game_code->get_sound_samples = 0;
        }
    }
    else
    {
        // TODO(me): logging
        printf("something went wrong %s\n", dlerror());
        exit(1);
    }
}

internal void sdl_unload_game_code(SDL_GameCode *game)
{
    dlclose(game->game_so);
    game->game_so = 0;
    game->update_and_render = 0;
    game->get_sound_samples = 0;
    game->is_valid = false;
}

internal void SDL_ResizeTexture(SDL_OffScreenBuffer *buffer, SDL_Renderer *renderer, int width, int height)
{
    int bytes_per_pixel = 4;
    buffer->bytes_per_pixel = bytes_per_pixel;
    if(buffer->memory)
    {
        // NOTE:(zourt) for some reason this (munmap) dumps when window is resized
        // NOTE:(zourt) if application core dumps, this may be a culprit
        munmap(buffer->memory, (buffer->width * buffer->height) * bytes_per_pixel);

        // NOTE: uncomment this
        // free(buffer->memory);
    }
    buffer->width = width;
    buffer->height = height;

    if(buffer->texture)
    {
        SDL_DestroyTexture(buffer->texture);
    }

    buffer->texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                buffer->width,
                                buffer->height);
    buffer->pitch = width * bytes_per_pixel;
    /*  (Zourt): amount of bytes to allocate for buffer
        multiply by 4 since channel is ARGB8888=4bytes | 32bits
    */

    // NOTE:(zourt) if application core dumps, this may be a culprit
    // /*
    buffer->memory = mmap(0,
                         (width * height) * bytes_per_pixel,
                         PROT_READ | PROT_WRITE,
                         MAP_ANONYMOUS | MAP_PRIVATE,
                         -1,
                         0);
    // */

    // NOTE: uncomment this
    // buffer->memory = malloc(width*height*bytes_per_pixel);
}

internal void SDL_UpdateWindow(SDL_OffScreenBuffer buffer, SDL_Window *Window, SDL_Renderer *renderer)
{
    SDL_UpdateTexture(buffer.texture,
                      NULL,
                      buffer.memory,
                      buffer.pitch);

    // NOTE: for prototyping, 
    // always copy 1 to 1 pixel to not introduce artefacts
    int offset_x = 0;
    int offset_y = 0;
    SDL_Rect src_rect = {0, 0, buffer.width, buffer.height};
    SDL_Rect dest_rect = {offset_y, offset_x, buffer.width, buffer.height};

    SDL_RenderCopy(renderer,
                   buffer.texture,
                   &src_rect,
                   &dest_rect);
                   

    SDL_RenderPresent(renderer);
}

internal real32 SDLProcessGameControllerAxisValue(int16 value, int16 dead_zone_threshold)
{
    real32 result = 0;
    if(value < -dead_zone_threshold)
    {
        result = (real32)((value + dead_zone_threshold) / (32768.0f - dead_zone_threshold));
    }
    else if(value > dead_zone_threshold)
    {
        result = (real32)((value + dead_zone_threshold) / (32767.0f - dead_zone_threshold));
    }

    return result;
}

internal void SDLProcessKeyPress(GameButtonState *new_state, bool is_down)
{
    if(new_state->ended_down != is_down)
    {
        new_state->ended_down = is_down;
        ++new_state->half_transition_count;
    }
}

internal void SDLProcessGameControllerButton(GameButtonState *old_state, GameButtonState *new_state, bool value)
{
    new_state->ended_down = value;
    new_state->half_transition_count += ((new_state->ended_down == old_state->ended_down)?0:1);
}

void cat_strings(size_t source_a_count, char *source_a, 
                size_t source_b_count, char *source_b,
                size_t dest_count, char *dest)
{
    for(int index = 0;
        index < source_a_count;
        index++)
    {
        *dest++ = *source_a++;
    }

    for(int index = 0;
        index < source_b_count;
        index++)
    {
        *dest++ = *source_b++;
    }

    *dest++ = 0;
}

internal void sdl_get_exe_filename(SDL_State *state)
{
    // NOTE: Debug code only
    ssize_t size_of_filename = readlink("/proc/self/exe", state->exe_filename, sizeof(state->exe_filename));
    state->one_past_last_slash = state->exe_filename;
    for(char *scan = state->exe_filename;
        *scan;
        scan++)
    {
        if(*scan == '/')
        {
            state->one_past_last_slash = scan + 1;
        }
    }
}

internal int string_length(char *string)
{
    int count = 0;
    while(*string++)
    {
        count ++;
    }
    return(count);
}

internal void sdl_build_exe_filepath(SDL_State *state, char *filename,
                                    int dest_count, char *dest)
{
    cat_strings(state->one_past_last_slash - state->exe_filename, state->exe_filename,
            string_length(filename), filename,
            dest_count, dest);
}


internal void sdl_get_input_filename(SDL_State *state, int slot_index,
                                    int dest_count, char *dest)
{
    Assert(slot_index == 1);
    sdl_build_exe_filepath(state, "looped.ukj", dest_count, dest);
}

internal void sdl_get_input_file_location(SDL_State *state, bool input_stream, int slot_index, int dest_count, char *dest)
{
    char temp[64];
    sprintf(temp, "loop_edit_%d_%s.ukj", slot_index, input_stream ? "input" : "state");
    sdl_build_exe_filepath(state, temp, dest_count, dest);
}

internal SDL_ReplayBuffer *sdl_get_replay_buffer(SDL_State *state, unsigned int index)
{
    Assert(index < ArrayCount(state->replay_buffers));
    SDL_ReplayBuffer *result = &state->replay_buffers[index];
    return(result);
}

internal void sdl_begin_recording_input(SDL_State *state, int input_recording_index)
{
    SDL_ReplayBuffer *replay_buffer = sdl_get_replay_buffer(state, input_recording_index);
    if(replay_buffer->memory_block)
    {
        state->input_recording_index = input_recording_index;
        char filename[PATH_MAX];
        sdl_get_input_file_location(state, true, input_recording_index, sizeof(filename), filename);
        state->recording_handle = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

#if 0
        lseek(state->recording_handle, state->total_size, SEEK_SET);
#endif
        memcpy(replay_buffer->memory_block, state->game_memory_block, state->total_size);
    }
}

internal void sdl_end_recording_input(SDL_State *state)
{
    close(state->recording_handle);
    state->input_recording_index = 0;
}

internal void sdl_begin_input_playback(SDL_State *state, int input_playing_index)
{
    SDL_ReplayBuffer *replay_buffer = sdl_get_replay_buffer(state, input_playing_index);
    if(replay_buffer->memory_block)
    {
        state->input_playing_index = input_playing_index;
        char filename[PATH_MAX];
        sdl_get_input_file_location(state, true, input_playing_index, sizeof(filename), filename);

        state->playback_handle = open(filename, O_RDONLY);

#if 0
        lseek(state->recording_handle, state->total_size, SEEK_SET);
#endif
        memcpy(state->game_memory_block, replay_buffer->memory_block, state->total_size);
    }
}

internal void sdl_end_input_playback(SDL_State *state)
{
    close(state->playback_handle);
    state->input_playing_index = 0;
}

internal void sdl_record_input(SDL_State *state, GameInput *input)
{
    int bytes_written;
    bytes_written = write(state->recording_handle, input, sizeof(*input));
}

internal void sdl_playback_input(SDL_State *state, GameInput *input)
{
    int bytes_read = read(state->playback_handle, input, sizeof(*input));
    if(bytes_read == 0)
    {
        int playing_index = state->input_playing_index;
        sdl_end_input_playback(state);
        sdl_begin_input_playback(state, playing_index);
    }
}

internal bool handle_event(SDL_Event *event, SDL_State *state, GameControllerInput *new_keyboard_controller)
{
    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
    SDL_Renderer *renderer = SDL_GetRenderer(window);

    bool should_quit = false;
    switch(event->type)
    {
        case(SDL_QUIT):
        {
            printf("Bye Bye\n");
            should_quit = true;
        }break;

        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            SDL_KeyCode keycode = event->key.keysym.sym;
            bool is_down = (event->key.state == SDL_PRESSED);
            bool was_down = false;
            if(event->key.state == SDL_RELEASED)
            {
                was_down = true;
            }
            else if(event->key.repeat != 0)
            {
                was_down = true;
            }

            if(event->key.repeat == 0)
            {
                if(keycode == SDLK_w)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->move_up, is_down);
                }
                else if(keycode == SDLK_a)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->move_left, is_down);
                }
                else if(keycode == SDLK_s)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->move_down, is_down);
                }
                else if(keycode == SDLK_d)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->move_right, is_down);
                }
                else if(keycode == SDLK_q)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->left_shoulder, is_down);
                }
                else if(keycode == SDLK_e)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->right_shoulder, is_down);
                }
                else if(keycode == SDLK_UP)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->action_up, is_down);
                }
                else if(keycode == SDLK_LEFT)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->action_left, is_down);
                }
                else if(keycode == SDLK_DOWN)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->action_down, is_down);
                }
                else if(keycode == SDLK_RIGHT)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->action_right, is_down);
                }
                else if(keycode == SDLK_ESCAPE)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->back, is_down);
                }
                else if(keycode == SDLK_SPACE)
                {
                    SDLProcessKeyPress(&new_keyboard_controller->start, is_down);
                }
#if UGLYKIDJOE_INTERNAL
                else if(keycode == SDLK_p)
                {
                    if(is_down)
                    {
                        global_pause = !global_pause;
                    }
                }
#endif
                else if(keycode == SDLK_l)
                {
                    if(is_down)
                    {
                        if(state->input_playing_index == 0)
                        {
                            if(state->input_recording_index == 0)
                            {
                                sdl_begin_recording_input(state, 1);
                            }
                            else
                            {
                                sdl_end_recording_input(state);
                                sdl_begin_input_playback(state, 1);
                            }
                        }
                        else
                        {
                            sdl_end_input_playback(state);
                        }
                     }
                }
            }

            bool alt_key_was_down = (event->key.keysym.mod & KMOD_ALT);
            if(keycode == SDLK_F4 && alt_key_was_down)
            {
                should_quit = true;
            }
        }break;

        case(SDL_WINDOWEVENT):
        {
            switch(event->window.event)
            {
                case(SDL_WINDOWEVENT_RESIZED):
                {
                }break;

                case(SDL_WINDOWEVENT_SIZE_CHANGED):
                {
                    // SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    // SDL_Renderer *renderer = SDL_GetRenderer(window);
                    // SDL_ResizeTexture(&global_back_buffer, renderer, 640, 640);
                }break;

                case(SDL_WINDOWEVENT_EXPOSED):
                {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
                    SDL_UpdateWindow(global_back_buffer, window, renderer);
                    
                }break;
            }
        }break;
    }

    return should_quit;
}



internal SDL_WindowDimensionResult SDL_GetWindowDimension(SDL_Window *window)
{
    SDL_WindowDimensionResult result;

    SDL_GetWindowSize(window, &result.width, &result.heigth);

    return result;
}

internal void SDL_OpenGameController()
{
    int max_joysticks = SDL_NumJoysticks();
    int controller_index = 0;
    for(int joystick_index = 0;joystick_index<max_joysticks;joystick_index++)
    {
        if(!SDL_IsGameController(joystick_index))
        {
            continue;
        }

        if(controller_index >= MAX_CONTROLLERS)
        {
            break;
        }
        controller_handles[controller_index] = SDL_GameControllerOpen(joystick_index);
        rumble_handles[controller_index] = SDL_HapticOpen(joystick_index);

        if(rumble_handles[controller_index] && SDL_HapticRumbleInit(rumble_handles[controller_index]) != 0 )
        {
            SDL_HapticClose(rumble_handles[controller_index]);
            rumble_handles[controller_index] = 0;
        }

        controller_index ++;
    }
}

internal void SDLAudioCallback(void *user_data, uint8 *audio_data, int length)
{
    RingBuffer *audio_ring_buffer = (RingBuffer *)user_data;
    int region1_size = length;
    int region2_size = 0;
    if(audio_ring_buffer->play_cursor + length > audio_ring_buffer->size)
    {
        region1_size = audio_ring_buffer->size - audio_ring_buffer->play_cursor;
        region2_size = length - region1_size;
    }

    memcpy(audio_data, (uint8 *)(audio_ring_buffer->data) + audio_ring_buffer->play_cursor, region1_size);
    memcpy(&audio_data[region1_size], audio_ring_buffer->data, region2_size);

    audio_ring_buffer->play_cursor = (audio_ring_buffer->play_cursor + length) % audio_ring_buffer->size;
    audio_ring_buffer->write_cursor = (audio_ring_buffer->play_cursor + length) % audio_ring_buffer->size;
}

internal void SDL_InitAudio(int32 samples_per_second, int32 buffer_size)
{
    SDL_AudioSpec audio_settings = {0};

    audio_settings.freq = samples_per_second;
    audio_settings.format = AUDIO_S16LSB;
    audio_settings.channels = 2;

    // NOTE(me): 512 gives a lower latency also determines the length passed to callback
    // NOTE(me): But the callback funcion would be called more often
    audio_settings.samples = 1024;
    audio_settings.callback = &SDLAudioCallback;
    audio_settings.userdata = &audio_ring_buffer;

    audio_ring_buffer.size = buffer_size;
    audio_ring_buffer.data = calloc(buffer_size, 1);
    audio_ring_buffer.play_cursor = 0;
    audio_ring_buffer.write_cursor = 0;

    SDL_OpenAudio(&audio_settings, 0);
    printf("Initialised an Audio device at frequency %d Hz, %d Channels\n",
           audio_settings.freq, audio_settings.channels);

    if(audio_settings.format != AUDIO_S16LSB)
    {
        printf("Wrong audio format\n");
        SDL_CloseAudio();
    }

    SDL_PauseAudio(0);
}

internal void SDL_CloseCotroller()
{
    for(int controller_index = 0;controller_index<MAX_CONTROLLERS;controller_index++)
    {
        if(controller_handles[controller_index])
        {
            SDL_GameControllerClose(controller_handles[controller_index]);
        }
    }
}

void sdl_fill_sound_buffer(SDL_SoundOutput *sound_output, int byte_to_lock, int bytes_to_write, GameSoundOutputBuffer *sound_buffer)
{
    void *region1 = (uint8*)audio_ring_buffer.data + byte_to_lock;
    int region1_size = bytes_to_write;

    if (region1_size + byte_to_lock > sound_output->secondary_buffer_size) 
    {
        region1_size = sound_output->secondary_buffer_size - byte_to_lock;
    }

    void *region2 = audio_ring_buffer.data;
    int region2_size = bytes_to_write - region1_size;
    int region1_sample_count = region1_size/sound_output->bytes_per_sample;

    int16 *dest_sample = (int16 *)region1;
    int16 *source_sample = sound_buffer->samples;

    for(int sample_index = 0;
        sample_index < region1_sample_count;
        ++sample_index)
    {
        *dest_sample++ = *source_sample++;
        *dest_sample++ = *source_sample++;

        ++sound_output->running_sample_index;
    }

    int region2_sample_count = region2_size/sound_output->bytes_per_sample;
    dest_sample = (int16 *)region2;
    for(int sample_index = 0;
        sample_index < region2_sample_count;
        ++sample_index)
    {
        *dest_sample++ = *source_sample++;
        *dest_sample++ = *source_sample++;

        ++sound_output->running_sample_index;
    }
}

internal inline uint32 SafeTruncateUint64(uint64 value)
{
    Assert(value <= 0xFFFFFFFF);
    uint32 result = (uint32)value;
    return(result);
}

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUG_platform_free_file_memory)
{
    free(memory);
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUG_platform_read_entire_file)
{
    DEBUG_ReadFileResult result = {};
    int FILE = open(filename, O_RDONLY);
    if(FILE == -1)
    {
        return result;
    }

    struct stat file_status;
    if(fstat(FILE, &file_status) == -1)
    {
        close(FILE);
        return result;
    }
    result.content_size = SafeTruncateUint64(file_status.st_size);
    
    // TODO(me): change to mmap?
    result.contents = malloc(result.content_size);
    if(!result.contents)
    {
        close(FILE);
        result.content_size = 0;
        result.contents = 0;
        return result;
    }

    uint32 bytes_to_read = result.content_size;
    uint8 *next_byte_location = (uint8 *)result.contents;
    while(bytes_to_read)
    {
        uint32 bytes_read = read(FILE, next_byte_location, bytes_to_read);
        if(bytes_read == -1)
        {
            DEBUG_platform_free_file_memory(thread, result.contents);
            result.content_size = 0;
            result.contents = 0;
            close(FILE);
            return result;
        }
        bytes_to_read -= bytes_read;
        next_byte_location += bytes_read;
    }

    close(FILE);
    return result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUG_platform_write_entire_file)
{
    int FILE = open(filename, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH); 
    if(!FILE)
    {
        return false;
    }

    uint32 bytes_to_write = memory_size;
    uint8 *next_byte_location = (uint8 *)memory;
    while(bytes_to_write)
    {
        uint32 bytes_written = write(FILE, next_byte_location, bytes_to_write);
        if(bytes_written == -1)
        {
            free(memory);
            memory = 0;
            close(FILE);
            return false;
        }
        bytes_to_write -= bytes_written;
        next_byte_location += bytes_written;
    }

    close(FILE);
    return true;
}

int SDLGetWindowRefreshRate(SDL_Window *window)
{
    SDL_DisplayMode mode;
    int display_index = SDL_GetWindowDisplayIndex(window);
    
    // NOTE(me): cause for some reason can return 0
    int default_refresh_rate = 60;
    if(SDL_GetDesktopDisplayMode(display_index, &mode) != 0)
    {
        return (default_refresh_rate);
    }
    if(mode.refresh_rate == 0)
    {
        return (default_refresh_rate);
    }

    return (mode.refresh_rate);
}

internal inline uint64 SDL_GetWallClock()
{
    uint64 result = SDL_GetPerformanceCounter();
    return (result);
}

internal inline real32 SDL_GetSecondsElapsed(uint64 old_counter, uint64 current_counter)
{
    return ((real32)(current_counter - old_counter) / (real32)(SDL_GetPerformanceFrequency()));
}

#if 0
internal void SDL_DebugDrawVertical(SDL_OffScreenBuffer *global_back_buffer, int x, int top, int bottom, uint32 color)
{
    uint8* pixel = ((uint8*)((global_back_buffer->memory) + (x*global_back_buffer->bytes_per_pixel) + (top*global_back_buffer->pitch)));
    for(int y = top; y < bottom;++y)
    {
        *(uint32*)pixel = color;
        pixel += global_back_buffer->pitch;
    }
}

internal void SDL_DrawSoundBufferMarker(SDL_OffScreenBuffer *back_buffer, 
                                        SDL_SoundOutput *sound_output, 
                                        real32 c, int pad_x, 
                                        int top, int bottom, int value, uint32 color)
{
        Assert(value < sound_output->secondary_buffer_size);
        int x = pad_x + (int)(c * (real32)value);
        SDL_DebugDrawVertical(back_buffer, x, top, bottom, color);
}

internal void SDL_DebugSyncDisplay(SDL_OffScreenBuffer *back_buffer, 
                                   int marker_count,
                                   DebugTimeMarker *markers, 
                                   int current_marker_index,
                                   SDL_SoundOutput *sound_output,
                                   real32 target_seconds_per_frame)
{
    int pad_x = 16;
    int pad_y = 16;

    int line_height = 64;

    real32 c = (back_buffer->width - 2*pad_x) / (real32)sound_output->secondary_buffer_size;
    for(int marker_index = 0;
        marker_index < marker_count;
        ++marker_index)
    {
        int top = pad_y;
        int bottom = pad_y + line_height;

        int play_color = 0xFFFFFF;
        int write_color = 0xFF0000;
        if(marker_index == current_marker_index)
        {
            top += line_height+pad_y;
            bottom += line_height+pad_y;
        }

        DebugTimeMarker *this_marker = &markers[marker_index];
        SDL_DrawSoundBufferMarker(back_buffer, sound_output, c, pad_x, top, bottom, this_marker->play_cursor, play_color);
        SDL_DrawSoundBufferMarker(back_buffer, sound_output, c, pad_x, top, bottom, this_marker->write_cursor, write_color);
    }
}
#endif


int main(int argc, char *argv[])
{
    SDL_State state = {};

    sdl_get_exe_filename(&state);

    char source_game_code_full_path[PATH_MAX];
    sdl_build_exe_filepath(&state, "uglykidjoe.so", 
            sizeof(source_game_code_full_path),
            source_game_code_full_path);

    char temp_game_code_full_path[PATH_MAX];
    sdl_build_exe_filepath(&state, "uglykidjoe_temp.so", 
            sizeof(temp_game_code_full_path),
            temp_game_code_full_path);

    SDL_Window *window;
    SDL_Renderer *renderer;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO) != 0)
    {
        // TODO:(zourt) SDL did not init
    }

    uint64 perf_count_frequency = SDL_GetPerformanceFrequency();

    // Initialise game controllers
    SDL_OpenGameController();

    window = SDL_CreateWindow("uglykidjoe", 
                              SDL_WINDOWPOS_UNDEFINED, 
                              SDL_WINDOWPOS_UNDEFINED, 
                              960, 
                              540, 
                              SDL_WINDOW_RESIZABLE);
    if(window)
    {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
        if(renderer)
        {
            // NOTE(): 1000p display mode is 1920x1080 -> 0.5 of that is 960x540
            // when using cpu
            SDL_ResizeTexture(&global_back_buffer, renderer, 960, 540);

            int monitor_refresh_rate = SDLGetWindowRefreshRate(window);
            printf("Refresh rate is %d Hz\n", monitor_refresh_rate);

            real32 game_update_hz = (real32)(monitor_refresh_rate) / 2.0f;
            real32 target_seconds_per_frame = 1.0f / (real32)game_update_hz;

            int input_recording_index = 0;
            int input_playing_index = 0;

            global_running = true;
            
            // NOTE: Test sound
            SDL_SoundOutput sound_output = {};

            sound_output.samples_per_second = 48000;
            sound_output.running_sample_index = 0;
            sound_output.bytes_per_sample = sizeof(int16) * 2;
            sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;

            // TODO: compute this varience and see what The
            // lowest reasonable value is
            sound_output.safety_bytes = (int)(((real32)sound_output.samples_per_second*(real32)sound_output.bytes_per_sample)/game_update_hz)/3;

            // NOTE: Open audio device
            SDL_InitAudio(48000, sound_output.secondary_buffer_size);

            int16 *samples = (int16 *)calloc(sound_output.samples_per_second, sound_output.bytes_per_sample);
            SDL_PauseAudio(0);

#if UGLYKIDJOE_INTERNAL
            // NOTE: Will fail on a 32-bit architecture
            void *base_address = (void *)TeraBytes(2);
#else
            void *base_address = (void *)(0);
#endif
            GameMemory game_memory = {};
            game_memory.permanent_storage_size = MegaBytes(256);
            game_memory.transient_storage_size = GigaBytes(1);

            game_memory.DEBUG_platform_free_file_memory = DEBUG_platform_free_file_memory;
            game_memory.DEBUG_platform_read_entire_file = DEBUG_platform_read_entire_file;
            game_memory.DEBUG_platform_write_entire_file = DEBUG_platform_write_entire_file;

            state.total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;
            state.game_memory_block = mmap(base_address, 
                                           (size_t)state.total_size,
                                           PROT_READ | PROT_WRITE, 
                                           MAP_ANON | MAP_PRIVATE, 
                                           -1, 0);
            game_memory.permanent_storage = state.game_memory_block;
            Assert(game_memory.permanent_storage);

            game_memory.transient_storage = (uint8 *)(game_memory.permanent_storage) + game_memory.transient_storage_size;

            for(int replay_index = 0;
                replay_index < ArrayCount(state.replay_buffers);
                replay_index++)
            {
                SDL_ReplayBuffer *replay_buffer = &state.replay_buffers[replay_index];

                sdl_get_input_file_location(&state, false, replay_index, sizeof(replay_buffer->replay_filename), replay_buffer->replay_filename);

                replay_buffer->file_handle = open(replay_buffer->replay_filename,
                         O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);;

                ftruncate(replay_buffer->file_handle, state.total_size);

                replay_buffer->memory_block = mmap(0, 
                                                (size_t)state.total_size,
                                                PROT_READ | PROT_WRITE, 
                                                MAP_PRIVATE, 
                                                replay_buffer->file_handle, 0);

                // TODO: Change to log message
                Assert(replay_buffer->memory_block);
            }

            if (samples && game_memory.permanent_storage && game_memory.transient_storage)
            {
                GameInput input[2] = {};
                GameInput *new_input = &input[0];
                GameInput *old_input = &input[1];
            
                uint64 last_counter = SDL_GetWallClock();
                uint64 last_cycle_count = _rdtsc();

                int audio_latency_bytes = 0;
                real32 audio_latency_seconds = 0;

                int debug_time_marker_index = 0;
                DebugTimeMarker debug_time_markers[30/2] = {0};
                int last_play_cursor = 0;
                int last_write_cursor = 0;
                bool sound_is_valid = false;

                SDL_GameCode game = {};
                sdl_load_game_code(&game, source_game_code_full_path, temp_game_code_full_path);
                while(global_running)
                {
                    new_input->dt_for_frame = target_seconds_per_frame;

                    struct timespec new_source_write_time = sdl_get_last_write_time(source_game_code_full_path);
                    if(new_source_write_time.tv_nsec != game.last_write_time.tv_nsec)
                    {
                        sdl_unload_game_code(&game);
                        sdl_load_game_code(&game, source_game_code_full_path, temp_game_code_full_path);
                    }

                    GameControllerInput *old_keyboard_controller = get_controller(old_input, 0);
                    GameControllerInput *new_keyboard_controller = get_controller(new_input, 0);
                    GameControllerInput zero_controller = {};
                    *new_keyboard_controller = zero_controller;
                    for(int button_index = 0;
                        button_index < ArrayCount(new_keyboard_controller->buttons);
                        button_index++)
                    {
                        new_keyboard_controller->buttons[button_index].ended_down = 
                        old_keyboard_controller->buttons[button_index].ended_down; 
                    }

                    SDL_Event event;
                    while(SDL_PollEvent(&event))
                    {
                        if(handle_event(&event, &state, new_keyboard_controller))
                        {
                           global_running = false;
                        }
                    }

                    if(!global_pause)
                    {
                        int x, y;
                        uint32 mouse_button_state;
                        mouse_button_state = SDL_GetMouseState(&x, &y);

                        new_input->mouse_x = x;
                        new_input->mouse_y = y;
                        new_input->mouse_z = 0;

                        SDLProcessKeyPress(&new_input->mouse_buttons[0], (mouse_button_state & SDL_BUTTON(SDL_BUTTON_LEFT)));
                        SDLProcessKeyPress(&new_input->mouse_buttons[1], (mouse_button_state & SDL_BUTTON(SDL_BUTTON_RIGHT)));
                        SDLProcessKeyPress(&new_input->mouse_buttons[2], (mouse_button_state & SDL_BUTTON(SDL_BUTTON_MIDDLE)));

                        for(int controller_index = 0;
                                controller_index<MAX_CONTROLLERS;
                                controller_index++)
                        {
                            if(controller_handles[controller_index] != 0 && SDL_GameControllerGetAttached(controller_handles[controller_index]))
                            {
                                int our_controller_index = controller_index + 1;
                                GameControllerInput *old_controller = get_controller(old_input, our_controller_index);
                                GameControllerInput *new_controller = get_controller(new_input, our_controller_index);

                                new_controller->is_connected = true;
                                new_controller->is_analog = old_controller->is_analog;

                                if(SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_UP))
                                {
                                    new_controller->stick_average_y = 1.0f;
                                    new_controller->is_analog = false;
                                }
                                if(SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_DOWN))
                                {
                                    new_controller->stick_average_y = -1.0f;
                                    new_controller->is_analog = false;
                                }
                                if(SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_LEFT))
                                {
                                    new_controller->stick_average_x = -1.0f;
                                    new_controller->is_analog = false;
                                }
                                if(SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
                                {
                                    new_controller->stick_average_x = 1.0f;
                                    new_controller->is_analog = false;
                                }

                                int16 stick_y = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTY);
                                int16 stick_x = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTX);

                                real32 y = SDLProcessGameControllerAxisValue(SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTY), 1);
                                real32 x = SDLProcessGameControllerAxisValue(SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTX), 1);
                                new_controller->stick_average_x = x;
                                new_controller->stick_average_y = y;
                                if((new_controller->stick_average_x != 0.0f) || 
                                            (new_controller->stick_average_y != 0.0f))
                                {
                                    new_controller->is_analog = true;
                                }

                                SDLProcessGameControllerButton(&old_controller->left_shoulder, 
                                                        &new_controller->left_shoulder,
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_LEFTSHOULDER));

                                SDLProcessGameControllerButton(&old_controller->right_shoulder, 
                                                         &new_controller->right_shoulder, 
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

                                SDLProcessGameControllerButton(&old_controller->action_down, 
                                                         &new_controller->action_down, 
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_A));

                                SDLProcessGameControllerButton(&old_controller->action_right, 
                                                         &new_controller->action_right, 
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_B));

                                SDLProcessGameControllerButton(&old_controller->action_left, 
                                                         &new_controller->action_left, 
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_X));

                                SDLProcessGameControllerButton(&old_controller->action_up, 
                                                         &new_controller->action_up, 
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_Y));

                                SDLProcessGameControllerButton(&old_controller->start, 
                                                         &new_controller->start, 
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_START));

                                SDLProcessGameControllerButton(&old_controller->back, 
                                                         &new_controller->back, 
                                                        SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_BACK));

                                real32 threshold = 0.5f;
                                SDLProcessGameControllerButton(&(old_controller->move_left),
                                                               &(new_controller->move_left),
                                                               new_controller->stick_average_x < -threshold);
                                SDLProcessGameControllerButton(&(old_controller->move_right),
                                                               &(old_controller->move_right),
                                                               new_controller->stick_average_x > threshold);
                                SDLProcessGameControllerButton(&(old_controller->move_up),
                                                               &(new_controller->move_up),
                                                               new_controller->stick_average_y < -threshold);
                                SDLProcessGameControllerButton(&(old_controller->move_down),
                                                               &(new_controller->move_down),
                                                               new_controller->stick_average_y > threshold);

                            }

                            else
                            {
                                //TODO: The contoller is not plugged
                            }
                        }

                        ThreadContext thread = {};

                        // graphics
                        GameOffScreenBuffer buffer = {};
                        buffer.memory = global_back_buffer.memory;
                        buffer.width = global_back_buffer.width;
                        buffer.height = global_back_buffer.height;
                        buffer.pitch = global_back_buffer.pitch;
                        buffer.bytes_per_pixel = global_back_buffer.bytes_per_pixel;

                        if(state.input_recording_index)
                        {
                            sdl_record_input(&state, new_input);
                        }

                        if(state.input_playing_index)
                        {
                            sdl_playback_input(&state, new_input);
                        }

                        if(game.update_and_render)
                        {
                            game.update_and_render(&thread, &game_memory, new_input, &buffer);
                        }

                        SDL_LockAudio();
                        if(!sound_is_valid)
                        {
                            sound_output.running_sample_index = audio_ring_buffer.write_cursor / sound_output.samples_per_second;
                            sound_is_valid = true;
                        }

                        int byte_to_lock = (sound_output.running_sample_index*sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;

                        int expected_sound_bytes_per_frame = 
                            (int)((real32)(sound_output.samples_per_second*sound_output.bytes_per_sample) / 
                                    game_update_hz);

                        int expected_frame_boundry_byte = audio_ring_buffer.play_cursor + expected_sound_bytes_per_frame;

                        int safe_write_cursor = audio_ring_buffer.write_cursor;
                        if(safe_write_cursor < audio_ring_buffer.play_cursor)
                        {
                            safe_write_cursor += sound_output.secondary_buffer_size;
                        }
                        Assert(safe_write_cursor >= audio_ring_buffer.play_cursor);
                        safe_write_cursor +=(audio_ring_buffer.write_cursor + sound_output.safety_bytes);
                        bool audio_card_is_low_latent = (safe_write_cursor < expected_frame_boundry_byte);

                        int target_cursor = 0;
                        if(audio_card_is_low_latent)
                        {
                            target_cursor = (expected_frame_boundry_byte + expected_sound_bytes_per_frame); 
                        }
                        else
                        {
                            target_cursor = (audio_ring_buffer.write_cursor + 
                                expected_sound_bytes_per_frame + sound_output.safety_bytes);
                        }
                            
                        target_cursor = target_cursor % sound_output.secondary_buffer_size;
                        int bytes_to_write;
                        if(byte_to_lock > target_cursor)
                        {
                            bytes_to_write = (sound_output.secondary_buffer_size - byte_to_lock);
                            bytes_to_write += target_cursor;
                        }
                        else
                        {
                            bytes_to_write = target_cursor - byte_to_lock;
                        }
                        SDL_UnlockAudio();

                        // sound
                        GameSoundOutputBuffer sound_buffer = {};
                        sound_buffer.samples_per_second = sound_output.samples_per_second;
                        sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
                        sound_buffer.samples = samples;
                        if(game.get_sound_samples)
                        {
                            game.get_sound_samples(&thread, &game_memory, &sound_buffer);
                        }

#if 0
                        int unwrapped_write_cursor = audio_ring_buffer.write_cursor;
                        if(unwrapped_write_cursor < audio_ring_buffer.play_cursor)
                        {
                            unwrapped_write_cursor += sound_output.secondary_buffer_size;
                        }
                        audio_latency_bytes = unwrapped_write_cursor - audio_ring_buffer.play_cursor;
                        audio_latency_seconds = (((real32)audio_latency_bytes / (real32)sound_output.bytes_per_sample) / (real32)sound_output.samples_per_second);
                        printf("bl: %i, bw: %i, play cursor: %i, write cursor: %i, bytes_btwn: %i, sec_bwtn_samps: %fs\n", 
                               byte_to_lock, bytes_to_write, audio_ring_buffer.play_cursor, audio_ring_buffer.write_cursor, audio_latency_bytes, audio_latency_seconds);
#endif

                        sdl_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);

                        real32 seconds_elapsed_for_frame = SDL_GetSecondsElapsed(last_counter, SDL_GetWallClock());
                        if(seconds_elapsed_for_frame < target_seconds_per_frame)
                        {
                            uint32 time_to_sleep = ((target_seconds_per_frame - seconds_elapsed_for_frame) * 1000);
                            if(time_to_sleep > 0)
                            {
                                SDL_Delay(time_to_sleep);
                            }

                            if(seconds_elapsed_for_frame < target_seconds_per_frame)
                            {
                                //TODO: log missed sleep
                            }

                            while(seconds_elapsed_for_frame < target_seconds_per_frame)
                            {
                                // wait...
                                seconds_elapsed_for_frame = SDL_GetSecondsElapsed(last_counter, 
                                                            SDL_GetWallClock());
                            }
                        }
                        else
                        {
                        }

                        uint64 end_counter = SDL_GetWallClock();
                        uint64 counter_elapsed = end_counter - last_counter;
                        real64 ms_per_frame = 1000.0f*SDL_GetSecondsElapsed(last_counter, end_counter);
                        last_counter = end_counter;

#if 0
                        // SDL_DebugSyncDisplay(&global_back_buffer, ArrayCount(debug_time_markers), debug_time_markers, debug_time_marker_index, &sound_output, target_seconds_per_frame);
#endif

                        SDL_UpdateWindow(global_back_buffer, window, renderer);
                        last_write_cursor = audio_ring_buffer.write_cursor;
                        last_play_cursor = audio_ring_buffer.play_cursor;


#if 0
                        {
                            DebugTimeMarker *marker = &debug_time_markers[debug_time_marker_index ++];
                            marker->play_cursor = audio_ring_buffer.play_cursor;
                            marker->write_cursor = audio_ring_buffer.write_cursor;
                            if(debug_time_marker_index >= ArrayCount(debug_time_markers))
                            {
                                debug_time_marker_index = 0;
                            }
                        }
#endif

                        // NOTE(me): try to double check types before typing

                        GameInput *temp = new_input;
                        new_input = old_input;
                        old_input = temp;

                        uint64 end_cycle_count = _rdtsc();
                        uint64 cycles_elapsed = end_cycle_count - last_cycle_count;
                        real64 fps = 0.0f;
                        real64 mcpf = ((real64)cycles_elapsed / (1000.0f * 1000.0f));

                        last_cycle_count = end_cycle_count;
#if 0
                        printf("%fms/f at %ffps, %fmc/f\n", ms_per_frame, fps, mcpf);  
#endif

                    }
                }
            }
            else
            {
                // TOD0(zourt): do something
            }
        }
        else
        {
            // TODO: do something
        }
    }
    else
    {
        // TODO: do something
    }

    SDL_CloseCotroller();
    SDL_Quit();
    printf("Hello Something\n");
    return 0;
}
