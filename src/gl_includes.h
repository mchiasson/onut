#ifndef GL_INCLUDES_H_INCLUDED
#define GL_INCLUDES_H_INCLUDED

#if defined(__APPLE__)
    #include <onut_gl/OnutGL.h>
    #include <onut_gl/OnutGLExt.h>
    #include <SDL2/SDL.h>
#elif defined(WIN32)
    #include <onut_gl/OnutGL.h>
    #include <onut_gl/OnutGLExt.h>
    #include <gl/GLU.h>
#else
    #include <onut_gl/OnutGL.h>
    #include <onut_gl/OnutGLExt.h>
    #include <SDL2/SDL.h>
#endif

#endif
