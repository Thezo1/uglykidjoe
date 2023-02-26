#include<stdio.h>
#include<stdbool.h>
#include<sys/mman.h>
#include<math.h>
#include<SDL2/SDL.h>
#include<x86intrin.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>

#define global_variable static
#define local_persist static
#define internal static

#define MAX_CONTROLLERS 4
#define Pi32 3.14159265358979f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef  float real32;
typedef  double real64;

#include "uglykidjoe.h"
#include "uglykidjoe.c"

#include "sdl_uglykidjoe.h"

global_variable SDL_OffScreenBuffer global_back_buffer;
SDL_GameController *controller_handles[MAX_CONTROLLERS];
SDL_Haptic *rumble_handles[MAX_CONTROLLERS];
RingBuffer audio_ring_buffer;

internal void SDL_UpdateWindow(SDL_OffScreenBuffer buffer, SDL_Window *Window, SDL_Renderer *renderer)
{
    SDL_UpdateTexture(buffer.texture,
                      NULL,
                      buffer.memory,
                      buffer.pitch);

    SDL_RenderCopy(renderer,
                   buffer.texture,
                   NULL,
                   NULL);
                   

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
    Assert(new_state->ended_down != is_down);
    new_state->ended_down = is_down;
    ++new_state->half_transition_count;
}

internal void SDLProcessGameControllerButton(GameButtonState *old_state, GameButtonState *new_state, bool value)
{
    new_state->ended_down = value;
    new_state->half_transition_count += ((new_state->ended_down == old_state->ended_down)?0:1);
}

bool handle_event(SDL_Event *event, GameControllerInput *new_keyboard_controller)
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
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
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

internal void SDL_ResizeTexture(SDL_OffScreenBuffer *buffer, SDL_Renderer *renderer, int width, int height)
{
    int bytes_per_pixel = 4;
    if(buffer->memory)
    {
        // NOTE:(zourt) for some reason this (munmap) dumps when window is resized
        // NOTE:(zourt) if application core dumps, this may be a culprit
        munmap(buffer->memory, (buffer->width * buffer->heigth) * bytes_per_pixel);

        // NOTE: uncomment this
        // free(buffer->memory);
    }
    if(buffer->texture)
    {
        SDL_DestroyTexture(buffer->texture);
    }

    buffer->texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                width,
                                height);
    buffer->width = width;
    buffer->heigth = height;
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

    // NOTE:(zourt) 2048 is the size of sdl's audio buffer in bytes
    audio_ring_buffer->play_cursor = (audio_ring_buffer->play_cursor + length) % audio_ring_buffer->size;
    audio_ring_buffer->write_cursor = (audio_ring_buffer->play_cursor + length) % audio_ring_buffer->size;
}

internal void SDL_InitAudio(int32 samples_per_second, int32 buffer_size)
{
    SDL_AudioSpec audio_settings = {0};

    audio_settings.freq = samples_per_second;
    audio_settings.format = AUDIO_S16LSB;
    audio_settings.channels = 2;

    audio_settings.samples = buffer_size / audio_settings.channels;
    audio_settings.callback = &SDLAudioCallback;
    audio_settings.userdata = &audio_ring_buffer;

    audio_ring_buffer.size = buffer_size;
    audio_ring_buffer.data = calloc(buffer_size, 1);
    audio_ring_buffer.play_cursor = audio_ring_buffer.write_cursor = 0;

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

internal DEBUG_ReadFileResult DEBUGPlatformReadEntireFile(char *filename)
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
            free(result.contents);
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

internal void DEBUGPlatormFreeFileMemory(void *memory)
{
    free(memory);
}

internal bool DEBUGPlatformWriteEntireFile(char *filename, uint64 memory_size, void *memory)
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

int main(int argc, char *argv[])
{
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
                              640, 
                              640, 
                              SDL_WINDOW_RESIZABLE);
    if(window == NULL)
    {
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if(renderer == NULL)
    {
        exit(1);
    }

    bool running = true;
    SDL_WindowDimensionResult result;
    result = SDL_GetWindowDimension(window);
    SDL_ResizeTexture(&global_back_buffer, renderer, result.width, result.heigth);
    
    GameInput input[2] = {};
    GameInput *new_input = &input[0];
    GameInput *old_input = &input[1];
    
    // NOTE: Test sound
    SDL_SoundOutput sound_output = {};

    sound_output.samples_per_second = 48000;
    sound_output.running_sample_index = 0;
    sound_output.bytes_per_sample = sizeof(int16) * 2;
    sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;

    // NOTE: Open audio device
    SDL_InitAudio(48000, sound_output.secondary_buffer_size);

    int16 *samples = (int16 *)calloc(sound_output.samples_per_second, sound_output.bytes_per_sample);
    SDL_PauseAudio(0);

#if UGLYKIDJOE_INTERNAL
    void *base_address = (void *)TeraBytes(2);
#else
    void *base_address = (void *)(0);
#endif
    GameMemory game_memory = {};
    game_memory.permanent_storage_size = MegaBytes(64);
    game_memory.transient_storage_size = GigaBytes(4);
    uint64 total_storage_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;

    game_memory.permanent_storage = mmap(base_address, 
                                         total_storage_size,
                                         PROT_READ | PROT_WRITE, 
                                         MAP_ANON | MAP_PRIVATE, 
                                         -1, 0);
    game_memory.transient_storage = (uint8 *)(game_memory.permanent_storage) + game_memory.transient_storage_size;

    if (game_memory.permanent_storage == NULL && samples == NULL)
    {
        // TOD0(zourt): do something
    }

    uint64 last_cycle_count = _rdtsc();
    uint64 last_counter = SDL_GetPerformanceCounter();
    while(running)
    {
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
            if(handle_event(&event, new_keyboard_controller))
            {
                running = false;
            }
        }

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

        SDL_LockAudio();
        int byte_to_lock = (sound_output.running_sample_index*sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;
        int bytes_to_write;
        if(byte_to_lock > audio_ring_buffer.play_cursor)
        {
            bytes_to_write = (sound_output.secondary_buffer_size - byte_to_lock);
            bytes_to_write += audio_ring_buffer.play_cursor;
        }
        else
        {
            bytes_to_write = audio_ring_buffer.play_cursor - byte_to_lock;
        }
        SDL_UnlockAudio();

        // sound
        GameSoundOutputBuffer sound_buffer = {};
        sound_buffer.samples_per_second = sound_output.samples_per_second;
        sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
        sound_buffer.samples = samples;

        // graphics
        GameOffScreenBuffer buffer = {};
        buffer.memory = global_back_buffer.memory;
        buffer.width = global_back_buffer.width;
        buffer.heigth = global_back_buffer.heigth;
        buffer.pitch = global_back_buffer.pitch;

        GameUpdateAndRender(&game_memory, new_input, &buffer, &sound_buffer);

        sdl_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);

        SDL_UpdateWindow(global_back_buffer, window, renderer);

        uint64 end_cycle_count = _rdtsc();
        uint64 cycles_elapsed = end_cycle_count - last_cycle_count;
        uint64 end_counter = SDL_GetPerformanceCounter();
        int64 counter_elapsed = end_counter - last_counter;
        int64 ms_per_frame = (1000 * (real64)counter_elapsed) / (real64)perf_count_frequency;
        int64 fps = (real64)perf_count_frequency / (real64)counter_elapsed;

        real64 mcpf = ((real64)cycles_elapsed / (1000.0f * 1000.0f));

        // printf("%jdms/f at %jdfps, %fmc/f\n", ms_per_frame, fps, mcpf);

        last_counter = end_counter;
        last_cycle_count = end_cycle_count;

        GameInput *temp = new_input;
        new_input = old_input;
        old_input = temp;
    }

    SDL_CloseCotroller();
    SDL_Quit();
    printf("Hello Something\n");
    return 0;
}
