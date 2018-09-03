#include "texture.h"

#include <iostream>
#include <fstream>
#include <SDL_log.h>

#ifdef _WIN32
#include <glew.h>
#include <SDL_surface.h>
#include <SDL_image.h>
#else
#include <GL/glew.h>
#include <sdl2/SDL_surface.h>
#include <sdl2/SDL_image.h>
#endif 

