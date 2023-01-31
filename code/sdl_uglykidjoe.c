#include<stdio.h>
#include<stdbool.h>
#include<sys/mman.h>
#include<SDL2/SDL.h>

#define global_variable static
#define MAX_CONTROLLERS 4

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

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

global_variable SDL_OffScreenBuffer global_back_buffer;
SDL_GameController *controller_handles[MAX_CONTROLLERS];
SDL_Haptic *rumble_handles[MAX_CONTROLLERS];

bool handle_event(SDL_Event *event);
static void SDL_ResizeTexture(SDL_OffScreenBuffer *buffer, SDL_Renderer *renderer, int width, int height);
static void SDL_UpdateWindow(SDL_OffScreenBuffer buffer, SDL_Window *Window, SDL_Renderer *renderer);
static void render_gradient(SDL_OffScreenBuffer *buffer, int blue_offset, int green_offset);
static SDL_WindowDimensionResult SDL_GetWindowDimension(SDL_Window *window);

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) != 0)
    {
        // TODO:(zourt) SDL did not init
    }

    //TODO:(zourt) put into a function
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
        render_gradient(&global_back_buffer, x_offset, y_offset);
        SDL_UpdateWindow(global_back_buffer, window, renderer);

        ++x_offset;

        for(int controller_index = 0;controller_index<MAX_CONTROLLERS;controller_index++)
        {
            if(controller_handles[controller_index] != 0 && SDL_GameControllerGetAttached(controller_handles[controller_index]))
            {
                bool Up = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_UP);
                bool Down = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_DOWN);
                bool Left = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_LEFT);
                bool Right = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
                bool Start = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_START);
                bool Back = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_BACK);
                bool LeftShoulder = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
                bool RightShoulder = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
                bool AButton = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_A);
                bool BButton = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_B);
                bool XButton = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_X);
                bool YButton = SDL_GameControllerGetButton(controller_handles[controller_index], SDL_CONTROLLER_BUTTON_Y);

                int16 StickX = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTX);
                int16 StickY = SDL_GameControllerGetAxis(controller_handles[controller_index], SDL_CONTROLLER_AXIS_LEFTY);

                if (AButton)
                {
                    y_offset += 2;
                }

                if (BButton)
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

    }

    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "ugly kid joe", "This is Ugly Kid Joe", 0);
    for(int controller_index = 0;controller_index<MAX_CONTROLLERS;controller_index++)
    {
        if(controller_handles[controller_index])
        {
            SDL_GameControllerClose(controller_handles[controller_index]);
        }
    }
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

            if(event->key.repeat == 1)
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
    // buffer->memory = malloc(width*height*buffer->bytes_per_pixel);
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
