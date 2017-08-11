#define GL_GLEXT_PROTOTYPES
#define EGL_EGLEXT_PROTOTYPES

#define _GNU_SOURCE

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <GLES3/gl3.h>

#include "matrix4.h"

void Matrix4Init(Matrix4 *matrix4);

static void
_internal_sincos (double a, double *s, double *c)
{
    *s = sin (a);
    *c = cos (a);
}

Matrix4 *
CreateMatrix4()
{
    Matrix4 *matrix4 = calloc(1, sizeof(Matrix4));
    
    Matrix4Init(matrix4);
    
    return matrix4;
}

void
DestroyMatrix4(Matrix4 *matrix4)
{
    free(matrix4);
}

void
iMatrix4_Dump(Matrix4 *matrix4)
{
    for (int i = 0; i < 16; i += 4) {
        printf("%.9f[%d] %.9f[%d] %.9f[%d] %.9f[%d]\n",
               matrix4->Elements[i + 0], i + 0,
               matrix4->Elements[i + 1], i + 1,
               matrix4->Elements[i + 2], i + 2,
               matrix4->Elements[i + 3], i + 3
               );
    }
}

/**
 * Multiplies two 4x4 matrices.
 *
 * The result is stored in matrix m.
 *
 * @param m the first matrix to multiply
 * @param n the second matrix to multiply
 */
static void
iMatrix4Multiply(Matrix4 *matrix4, const Matrix4 *multiplier)
{
    GLfloat *freeMe = NULL;
    
    if (matrix4 == multiplier) {
        freeMe = calloc(1, sizeof(matrix4->Elements));
        
        memcpy(freeMe, matrix4->Elements, sizeof(matrix4->Elements));
    }
    
    GLfloat *e = matrix4->Elements;
    const GLfloat *a = matrix4->Elements;
    const GLfloat *b = freeMe ? freeMe : multiplier->Elements;
    
    // printf("/=\\\n");
    // iMatrix4._Dump(matrix4);
    // printf("-=-\n");
    // iMatrix4._Dump(multiplier);
    // printf("\\=/\n");
    
    for (int i = 0; i < 4; i++) {
        GLfloat ai0, ai1, ai2, ai3;
        
        ai0=a[i];  ai1=a[i+4];  ai2=a[i+8];  ai3=a[i+12];
        
        e[i]    = ai0 * b[0]  + ai1 * b[1]  + ai2 * b[2]  + ai3 * b[3];
        e[i+4]  = ai0 * b[4]  + ai1 * b[5]  + ai2 * b[6]  + ai3 * b[7];
        e[i+8]  = ai0 * b[8]  + ai1 * b[9]  + ai2 * b[10] + ai3 * b[11];
        e[i+12] = ai0 * b[12] + ai1 * b[13] + ai2 * b[14] + ai3 * b[15];
    }
    
    // iMatrix4._Dump(matrix4);
    
    free(freeMe);
}

/**
 * Rotates a 4x4 matrix.
 *
 * @param[in,out] m the matrix to rotate
 * @param angle the angle to rotate
 * @param x the x component of the direction to rotate to
 * @param y the y component of the direction to rotate to
 * @param z the z component of the direction to rotate to
 */
static void
iMatrix4Rotate(Matrix4 *matrix4, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    double s, c;
    // GLfloat *m = matrix4->Elements;
    
    _internal_sincos(angle, &s, &c);
    GLfloat r[16] = {
        x * x * (1 - c) + c,     y * x * (1 - c) + z * s, x * z * (1 - c) - y * s, 0,
        x * y * (1 - c) - z * s, y * y * (1 - c) + c,     y * z * (1 - c) + x * s, 0,
        x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, z * z * (1 - c) + c,     0,
        0, 0, 0, 1
    };
    
    Matrix4 *rotation = CreateMatrix4();
    memcpy(rotation->Elements, r, sizeof(r));
    
    iMatrix4.Multiply(matrix4, rotation);
}

static void
iMatrix4Set(Matrix4 *matrix4, Matrix4 *src)
{
    if (src == matrix4) {
        return;
    }
    
    GLfloat *s = src->Elements;
    GLfloat *d = matrix4->Elements;
    
    for (int i = 0; i < 16; ++i) {
        d[i] = s[i];
    }
    // memcpy(matrix4->Elements, src->Elements, sizeof(src->Elements));
}

/**
 * Calculate a perspective projection transformation.
 *
 * @param m the matrix to save the transformation in
 * @param fovy the field of view in the y direction
 * @param aspect the view aspect ratio
 * @param zNear the near clipping plane
 * @param zFar the far clipping plane
 */
static void
iMatrix4Perspective(Matrix4 *matrix4, GLfloat fovy, GLfloat aspect, GLfloat near, GLfloat far)
{
    double sine, cosine, cotangent, rd;
    GLfloat *m = matrix4->Elements;
    
    GLfloat radians = fovy / 2 * M_PI / 180;
    
    _internal_sincos(radians, &sine, &cosine);
    cotangent = cosine / sine;
    rd = 1.0 / (far - near);
    
    m[0]  = cotangent / aspect;
    m[1]  = 0;
    m[2]  = 0;
    m[3]  = 0;
    
    m[4]  = 0;
    m[5]  = cotangent;
    m[6]  = 0;
    m[7]  = 0;
    
    m[8]  = 0;
    m[9]  = 0;
    m[10] = -(far + near) * rd;
    m[11] = -1;
    
    m[12] = 0;
    m[13] = 0;
    m[14] = -2.0 * near * far * rd;
    m[15] = 0;
}

/**
 * Translates a 4x4 matrix.
 *
 * @param[in,out] m the matrix to translate
 * @param x the x component of the direction to translate to
 * @param y the y component of the direction to translate to
 * @param z the z component of the direction to translate to
 */
static void
iMatrix4Translate(Matrix4 *matrix4, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat *e = matrix4->Elements;
    
    e[12] += e[0] * x + e[4] * y + e[8]  * z;
    e[13] += e[1] * x + e[5] * y + e[9]  * z;
    e[14] += e[2] * x + e[6] * y + e[10] * z;
    e[15] += e[3] * x + e[7] * y + e[11] * z;
}

/**
 * Creates an identity 4x4 matrix.
 *
 * @param m the matrix make an identity matrix
 */
static void
iMatrix4Identity(Matrix4 *matrix4)
{
    GLfloat *m = matrix4->Elements;
    
    m[0] = 1.0;
    m[1] = 0.0;
    m[2] = 0.0;
    m[3] = 0.0;
    
    m[4] = 0.0;
    m[5] = 1.0;
    m[6] = 0.0;
    m[7] = 0.0;
    
    m[8] = 0.0;
    m[9] = 0.0;
    m[10] = 1.0;
    m[11] = 0.0;
    
    m[12] = 0.0;
    m[13] = 0.0;
    m[14] = 0.0;
    m[15] = 1.0;
}

/**
 * Transposes a 4x4 matrix.
 *
 * @param m the matrix to transpose
 */
static void
iMatrix4Transpose(Matrix4 *matrix4)
{
    GLfloat *m = matrix4->Elements;
    
    GLfloat t[16] = {
        m[0], m[4], m[8],  m[12],
        m[1], m[5], m[9],  m[13],
        m[2], m[6], m[10], m[14],
        m[3], m[7], m[11], m[15]};
    
    memcpy(matrix4->Elements, t, sizeof(t));
}

/**
 * Inverts a 4x4 matrix.
 *
 * This function can currently handle only pure translation-rotation matrices.
 * Read http://www.gamedev.net/community/forums/topic.asp?topic_id=425118
 * for an explanation.
 */
static void
iMatrix4Invert(Matrix4 *matrix4)
{
    GLfloat *m = matrix4->Elements;
    
    Matrix4 *tmpMatrix4 = CreateMatrix4();
    GLfloat *t = tmpMatrix4->Elements;
    
    iMatrix4.Identity(tmpMatrix4);
    
    // Extract and invert the translation part 't'. The inverse of a
    // translation matrix can be calculated by negating the translation
    // coordinates.
    t[12] = -m[12]; t[13] = -m[13]; t[14] = -m[14];
    
    // Invert the rotation part 'r'. The inverse of a rotation matrix is
    // equal to its transpose.
    m[12] = m[13] = m[14] = 0;
    iMatrix4.Transpose(matrix4);
    
    // inv(m) = inv(r) * inv(t)
    iMatrix4.Multiply(matrix4, tmpMatrix4);
}

static void
iMatrix4LookAt(
               Matrix4 *matrix4,
               GLfloat eyeX, GLfloat eyeY, GLfloat eyeZ,
               GLfloat centerX, GLfloat centerY, GLfloat centerZ,
               GLfloat upX, GLfloat upY, GLfloat upZ)
{
    Matrix4 *tmpMatrix4 = CreateMatrix4();
    iMatrix4.Identity(tmpMatrix4);
    
    GLfloat fx, fy, fz, rlf, sx, sy, sz, rls, ux, uy, uz;
    
    fx = centerX - eyeX;
    fy = centerY - eyeY;
    fz = centerZ - eyeZ;
    
    // Normalize f.
    rlf = 1.0 / sqrtf(fx*fx + fy*fy + fz*fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;
    
    // Calculate cross product of f and up.
    sx = fy * upZ - fz * upY;
    sy = fz * upX - fx * upZ;
    sz = fx * upY - fy * upX;
    
    // Normalize s.
    rls = 1.0 / sqrtf(sx*sx + sy*sy + sz*sz);
    sx *= rls;
    sy *= rls;
    sz *= rls;
    
    // Calculate cross product of s and f.
    ux = sy * fz - sz * fy;
    uy = sz * fx - sx * fz;
    uz = sx * fy - sy * fx;
    
    GLfloat *e = tmpMatrix4->Elements;
    e[0] = sx;
    e[1] = ux;
    e[2] = -fx;
    e[3] = 0;
    
    e[4] = sy;
    e[5] = uy;
    e[6] = -fy;
    e[7] = 0;
    
    e[8] = sz;
    e[9] = uz;
    e[10] = -fz;
    e[11] = 0;
    
    e[12] = 0;
    e[13] = 0;
    e[14] = 0;
    e[15] = 1;
    
    iMatrix4.Translate(tmpMatrix4, -eyeX, -eyeY, -eyeZ);
    
    iMatrix4.Multiply(matrix4, tmpMatrix4);
};

void
Matrix4Init(Matrix4 *matrix4)
{
    iMatrix4._Dump = iMatrix4_Dump;
    iMatrix4.Multiply = iMatrix4Multiply;
    iMatrix4.Rotate = iMatrix4Rotate;
    iMatrix4.Set = iMatrix4Set;
    iMatrix4.Perspective = iMatrix4Perspective;
    iMatrix4.Translate = iMatrix4Translate;
    iMatrix4.Identity = iMatrix4Identity;
    iMatrix4.Transpose = iMatrix4Transpose;
    iMatrix4.Invert = iMatrix4Invert;
    iMatrix4.LookAt = iMatrix4LookAt;
}

