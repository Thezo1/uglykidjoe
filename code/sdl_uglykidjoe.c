#include<stdio.h>
#include<stdbool.h>
#include<sys/mman.h>
#include<math.h>
#include<SDL2/SDL.h>
#include<x86intrin.h>

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

bool handle_event(SDL_Event *event)
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

        case (SDL_KEYDOWN):
        case (SDL_KEYUP):
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

            bool alt_key_was_down = (event->key.keysym.mod & KMOD_ALT);

            if(keycode == SDLK_F4 && alt_key_was_down)
            {
                should_quit = true;
            }

            if(event->key.repeat == 0)
            {
                if(keycode == SDLK_w)
                {
                }
                else if(keycode == SDLK_a)
                {
                }
                else if(keycode == SDLK_s)
                {
                }
                else if(keycode == SDLK_d)
                {
                }
                else if(keycode == SDLK_q)
                {
                }
                else if(keycode == SDLK_e)
                {
                }
                else if(keycode == SDLK_UP)
                {
                }
                else if(keycode == SDLK_LEFT)
                {
                }
                else if(keycode == SDLK_DOWN)
                {
                }
                else if(keycode == SDLK_RIGHT)
                {
                }
                else if(keycode == SDLK_ESCAPE)
                {
                    printf("ESCAPE: ");
                    if(is_down)
                    {
                        printf("IsDown ");
                    }
                    if(was_down)
                    {
                        printf("WasDown");
                    }
                    printf("\n");
                }
                else if(keycode == SDLK_SPACE)
                {
                }
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

internal void SDLProcessGameController(GameButtonState *old_state,
                                     GameButtonState *new_state,
                                     SDL_GameController *controller_handle,
                                     SDL_GameControllerButton button
)
{
    new_state->ended_down = SDL_GameControllerGetButton(controller_handle, button);
    new_state->half_transition_count = 
        (new_state->ended_down == old_state->ended_down) ? 1 : 0;
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
        SDL_Event event;
        SDL_PollEvent(&event);

        if(handle_event(&event))
        {
            running = false;
        }

        for(int controller_index = 0;
                controller_index<MAX_CONTROLLERS;
                controller_index++)
        {
            if(controller_handles[controller_index] != 0 && SDL_GameControllerGetAttached(controller_handles[controller_index]))
            {
                GameControllerInput *old_controller = &old_input->controllers[controller_index];
                GameControllerInput *new_controller = &new_input->controllers[controller_index];

                new_controller->is_analog = true;

                // Dpad
                bool up = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_UP);
                bool down = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                bool left = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
                bool right = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

                new_controller->start_x = old_controller->end_x;
                new_controller->start_y = old_controller->end_y;

                int16 stick_y = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTY);
                int16 stick_x = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTX);

                if(stick_y < 0)
                {
                    new_controller->end_y = stick_y / -32768.0f;
                }
                else
                {
                    new_controller->end_y = stick_y / -32767.0f;
                }

                new_controller->min_y = new_controller->max_y = new_controller->end_y;

                if(stick_x < 0)
                {
                    new_controller->end_x = stick_x / -32768.0f;
                }
                else
                {
                    new_controller->end_x = stick_x / -32767.0f;
                }

                new_controller->min_x = new_controller->max_x = new_controller->end_x;

                SDLProcessGameController(&old_controller->left_shoulder, 
                                         &new_controller->left_shoulder, 
                                         controller_handles[controller_index], 
                                         SDL_CONTROLLER_BUTTON_LEFTSHOULDER);

                SDLProcessGameController(&old_controller->left_shoulder, 
                                         &new_controller->left_shoulder, 
                                         controller_handles[controller_index], 
                                         SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

                SDLProcessGameController(&old_controller->down, 
                                         &new_controller->down, 
                                         controller_handles[controller_index], 
                                         SDL_CONTROLLER_BUTTON_A);

                SDLProcessGameController(&old_controller->right, 
                                         &new_controller->right, 
                                         controller_handles[controller_index], 
                                         SDL_CONTROLLER_BUTTON_B);

                SDLProcessGameController(&old_controller->left, 
                                         &new_controller->left, 
                                         controller_handles[controller_index], 
                                         SDL_CONTROLLER_BUTTON_X);

                SDLProcessGameController(&old_controller->up, 
                                         &new_controller->up, 
                                         controller_handles[controller_index], 
                                         SDL_CONTROLLER_BUTTON_Y);

                // bool start = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_START);
                // bool back = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_BACK);
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

        // char buffer[256];
        // sprintf(buffer, "%loms at %lofps\n", ms_per_frame, fps);
        printf("%loms/f at %lofps, %fmc/f\n", ms_per_frame, fps, mcpf);

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
