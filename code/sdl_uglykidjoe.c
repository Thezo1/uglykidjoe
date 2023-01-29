#include<stdio.h>
#include<stdbool.h>
#include<SDL2/SDL.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGTH 640
#define global_variable static

// back buffer stuff
global_variable SDL_Texture *texture;
global_variable void *pixels;
global_variable int texture_width;

bool handle_event(SDL_Event *event);
static void SDL_ResizeTexture(SDL_Renderer *renderer, int width, int height);
static void SDL_UpdateWindow(SDL_Window *Window, SDL_Renderer *Renderer);

int main(int argc, char *argv[])
{
    SDL_Window *window;
    SDL_Renderer *renderer;

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        // TODO:(zourt) SDL did not init
    }

    window = SDL_CreateWindow("uglykidjoe", 
                              SDL_WINDOWPOS_UNDEFINED, 
                              SDL_WINDOWPOS_UNDEFINED, 
                              WINDOW_WIDTH, 
                              WINDOW_HEIGTH, 
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

    for(;;)
    {
        SDL_Event event;
        SDL_WaitEvent(&event);
        if(handle_event(&event))
        {
            break;
        }
    }

    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "ugly kid joe", "This is Ugly Kid Joe", 0);
    SDL_Quit();
    printf("Hello Something\n");
    return 0;
}

bool handle_event(SDL_Event *event)
{
    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
    SDL_Renderer *renderer = SDL_GetRenderer(window);

    static bool is_white = true;

    bool should_quit = false;
    switch(event->type)
    {
        case(SDL_QUIT):
        {
            printf("Bye Bye\n");
            should_quit = true;
        }break;

        case(SDL_WINDOWEVENT):
        {
            switch(event->window.event)
            {
                case(SDL_WINDOWEVENT_RESIZED):
                {
                }break;

                case(SDL_WINDOWEVENT_EXPOSED):
                {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
                    SDL_UpdateWindow(window, renderer);
                    
                }break;
                
                case(SDL_WINDOWEVENT_SIZE_CHANGED):
                {
                    SDL_Window *window = SDL_GetWindowFromID(event->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
                    printf("SDL_WINDOWEVENT_SIZE_CHANGED (%d, %d)\n", event->window.data1, event->window.data2);
                    SDL_ResizeTexture(renderer, event->window.data1, event->window.data2);
                }break;
            }
        }break;
    }

    return should_quit;
}


static void SDL_ResizeTexture(SDL_Renderer *renderer, int width, int height)
{
    if(pixels)
    {
        free(pixels);
    }
    if(texture)
    {
        SDL_DestroyTexture(texture);
    }

    texture = SDL_CreateTexture(renderer,
                                SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                width,
                                height);
    texture_width = width;

    /*  (Zourt): amount of bytes to allocate for buffer
        multiply by 4 since channel is ARGB8888=4bytes | 32bits
    */
    pixels = malloc(width * height * 4);
}

static void SDL_UpdateWindow(SDL_Window *Window, SDL_Renderer *renderer)
{
    SDL_UpdateTexture(texture,
                      NULL,
                      pixels,
                      texture_width * 4);

    SDL_RenderCopy(renderer,
                   texture,
                   NULL,
                   NULL
                   );

    SDL_RenderPresent(renderer);
}
