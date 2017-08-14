#ifndef _MATRIX4_H
#define _MATRIX4_H

#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#include <SDL.h>
#include <GLES3/gl3.h>
#else // iOS, OS X, Linux
#include <SDL.h>
#include "glad/glad.h"
#endif

typedef struct Matrix4 Matrix4;

typedef struct Matrix4 {
    GLfloat Elements[16];
} Matrix4;

Matrix4 *CreateMatrix4();

struct iMatrix4 {
    void (*_Dump)(Matrix4 *matrix4);
    void (*Multiply)(Matrix4 *matrix4, const Matrix4 *multiplier);
    void (*Rotate)(Matrix4 *matrix4, GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
    void (*Perspective)(Matrix4 *matrix4, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
    void (*Translate)(Matrix4 *matrix4, GLfloat x, GLfloat y, GLfloat z);
    void (*Identity)(Matrix4 *matrix4);
    void (*Transpose)(Matrix4 *matrix4);
    void (*Invert)(Matrix4 *matrix4);
    void (*LookAt)(Matrix4 *matrix4, GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ, GLfloat centerX, GLfloat centerY, GLfloat centerZ, GLfloat upX, GLfloat upY, GLfloat upZ);
    void (*Set)(Matrix4 *matrix4, Matrix4 *src);
} iMatrix4;

#endif
