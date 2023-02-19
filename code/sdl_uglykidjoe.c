#include<stdio.h>
#include<stdbool.h>
#include<sys/mman.h>
#include<math.h>
#include<SDL2/SDL.h>

#define global_variable static
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

// back buffer stuff
// NOTE:(zourt) each pixel is 4 bytes || 32 bits
typedef struct SDL_OffScreenBuffer
{
    SDL_Texture *texture;
    void *memory;
    int width;
    int heigth;
    int pitch;
} SDL_OffScreenBuffer;

typedef struct SDL_WindowDimensionResult
{
    int width;
    int heigth;
}SDL_WindowDimensionResult;

typedef struct RingBuffer
{
    int size;
    int write_cursor;
    int play_cursor;
    void* data;
}RingBuffer;

typedef struct SDL_SoundOutput
{
    int samples_per_second;
    int tone_hz;
    int16 tone_volume;
    uint32 running_sample_index;
    int wave_period;
    int bytes_per_sample;
    int secondary_buffer_size;
    
} SDL_SoundOutput;


global_variable SDL_OffScreenBuffer global_back_buffer;
SDL_GameController *controller_handles[MAX_CONTROLLERS];
SDL_Haptic *rumble_handles[MAX_CONTROLLERS];
RingBuffer audio_ring_buffer;

bool handle_event(SDL_Event *event);
static void SDL_ResizeTexture(SDL_OffScreenBuffer *buffer, SDL_Renderer *renderer, int width, int height);
static void SDL_UpdateWindow(SDL_OffScreenBuffer buffer, SDL_Window *Window, SDL_Renderer *renderer);
static void render_gradient(SDL_OffScreenBuffer *buffer, int blue_offset, int green_offset);
static SDL_WindowDimensionResult SDL_GetWindowDimension(SDL_Window *window);
static void SDLAudioCallback(void *user_data, uint8 *audio_data, int length);
static void SDL_OpenGameController();
static void SDL_CloseCotroller();
static void SDL_InitAudio(int32 samples_per_second, int32 buffer_size);
void sdl_fill_sound_buffer(SDL_SoundOutput *sound_output, int byte_to_lock, int bytes_to_write);

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC | SDL_INIT_AUDIO) != 0)
    {
        // TODO:(zourt) SDL did not init
    }

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

    // NOTE: Test sound
    SDL_SoundOutput sound_output = {};

    sound_output.samples_per_second = 48000;
    sound_output.tone_hz = 256;
    sound_output.tone_volume = 3000;
    sound_output.running_sample_index = 0;
    sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
    sound_output.bytes_per_sample = sizeof(int16) * 2;
    sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;

    // NOTE: Open audio device
    SDL_InitAudio(48000, sound_output.secondary_buffer_size);
    sdl_fill_sound_buffer(&sound_output, 0, sound_output.bytes_per_sample);
    SDL_PauseAudio(0);

    int x_offset = 0;
    int y_offset = 0;
    while(running)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
        if(handle_event(&event))
        {
            running = false;
        }

        for(int controller_index = 0;controller_index<MAX_CONTROLLERS;controller_index++)
        {
            if(controller_handles[controller_index] != 0 && SDL_GameControllerGetAttached(controller_handles[controller_index]))
            {
                bool up = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_UP);
                bool down = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                bool left = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
                bool right = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
                bool start = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_START);
                bool back = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_BACK);
                bool left_shoulder = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                bool right_shoulder = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                bool a_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_A);
                bool b_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_B);
                bool x_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_X);
                bool y_button = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_Y);

                int16 stick_x = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTX);
                int16 stick_y = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTY);

                if (a_button)
                {
                    y_offset += 2;
                }

                if (b_button)
                {
                    if (rumble_handles[controller_index])
                    {
                        SDL_HapticRumblePlay(rumble_handles[controller_index], 0.5f, 2000);
                    }
                }
            }

            else
            {
                //TODO: The contoller is not plugged
            }
        }

        render_gradient(&global_back_buffer, x_offset, y_offset);

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
        sdl_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write);

        SDL_UpdateWindow(global_back_buffer, window, renderer);
        ++x_offset;
    }

    SDL_CloseCotroller();
    SDL_Quit();
    printf("Hello Something\n");
    return 0;
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

static void SDL_ResizeTexture(SDL_OffScreenBuffer *buffer, SDL_Renderer *renderer, int width, int height)
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

static void SDL_UpdateWindow(SDL_OffScreenBuffer buffer, SDL_Window *Window, SDL_Renderer *renderer)
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

static void render_gradient(SDL_OffScreenBuffer *buffer, int blue_offset, int green_offset)
{
    int width = buffer->width;
    int heigth = buffer->heigth;
    int pitch = buffer->pitch;

    uint8 *row = (uint8 *)buffer->memory;
    for(int y = 0;y<buffer->heigth;y++)
    {
        uint32 *pixel = (uint32 *)row;
        for(int x = 0;x<buffer->width;x++)
        {
            uint8 blue = (x + blue_offset);
            uint8 green = (y + green_offset);
            *pixel++ = ((green<<8) | blue);
        }

        row += pitch;
    }
}

static SDL_WindowDimensionResult SDL_GetWindowDimension(SDL_Window *window)
{
    SDL_WindowDimensionResult result;

    SDL_GetWindowSize(window, &result.width, &result.heigth);

    return result;
}

static void SDL_OpenGameController()
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

static void SDLAudioCallback(void *user_data, uint8 *audio_data, int length)
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

static void SDL_InitAudio(int32 samples_per_second, int32 buffer_size)
{
    SDL_AudioSpec audio_settings = {0};

    audio_settings.freq = samples_per_second;
    audio_settings.format = AUDIO_S16LSB;
    audio_settings.channels = 2;

    audio_settings.samples = buffer_size / audio_settings.channels;
    audio_settings.callback = &SDLAudioCallback;
    audio_settings.userdata = &audio_ring_buffer;

    audio_ring_buffer.size = buffer_size;
    audio_ring_buffer.data = malloc(buffer_size);
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

static void SDL_CloseCotroller()
{
    for(int controller_index = 0;controller_index<MAX_CONTROLLERS;controller_index++)
    {
        if(controller_handles[controller_index])
        {
            SDL_GameControllerClose(controller_handles[controller_index]);
        }
    }
}

void sdl_fill_sound_buffer(SDL_SoundOutput *sound_output, int byte_to_lock, int bytes_to_write)
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
        int16 *sample_out = (int16 *)region1;

        for(int sample_index = 0;
            sample_index < region1_sample_count;
            ++sample_index)
        {
            real32 t = 2.0f * Pi32 * (real32)sound_output->running_sample_index / (real32)sound_output->wave_period;
            real32 sine_value = sinf(t);
            int16 sample_value = (int16)(sine_value * sound_output->tone_volume);
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;

            ++sound_output->running_sample_index;
        }

        int region2_sample_count = region2_size/sound_output->bytes_per_sample;
        sample_out = (int16 *)region2;
        for(int sample_index = 0;
            sample_index < region2_sample_count;
            ++sample_index)
        {
            real32 t = 2.0f * Pi32 * ((real32)sound_output->running_sample_index / (real32)sound_output->wave_period);
            real32 sine_value = sinf(t);
            int16 sample_value = (int16)(sine_value * sound_output->tone_volume);
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;

            ++sound_output->running_sample_index;
        }
}
