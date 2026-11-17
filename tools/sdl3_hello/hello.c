// C2b proof: static SDL3 links and runs. Prints version and gamepad count.
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <stdio.h>

int main(void) {
    int v = SDL_GetVersion();
    printf("SDL3 version %d.%d.%d\n", SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v),
           SDL_VERSIONNUM_MICRO(v));
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    int n = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&n);
    printf("gamepads: %d\n", n);
    SDL_free(ids);
    SDL_Quit();
    return 0;
}
