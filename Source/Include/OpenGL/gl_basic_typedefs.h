#ifndef GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#define GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS
#include <stddef.h>
#ifndef GLEXT_64_TYPES_DEFINED
	/* This code block is duplicated in glxext.h, so must be protected */
	#define GLEXT_64_TYPES_DEFINED
	/* Define int32_t, int64_t, and uint64_t types for UST/MSC */
	/* (as used in the GL_EXT_timer_query extension). */
	#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
		#include <inttypes.h>
	#elif defined(__sun__) || defined(__digital__)
		#include <inttypes.h>
		#if defined(__STDC__)
			#if defined(__arch64__) || defined(_LP64)
				typedef long int int64_t;
				typedef unsigned long int uint64_t;
			#else
				typedef long long int int64_t;
				typedef unsigned long long int uint64_t;
			#endif /* __arch64__ */
		#endif /* __STDC__ */
	#elif defined( __VMS ) || defined(__sgi)
		#include <inttypes.h>
	#elif defined(__SCO__) || defined(__USLC__)
		#include <stdint.h>
	#elif defined(__UNIXOS2__) || defined(__SOL64__)
		typedef long int int32_t;
		typedef long long int int64_t;
		typedef unsigned long long int uint64_t;
	#elif defined(_WIN32) && defined(__GNUC__)
		#include <stdint.h>
	#elif defined(_WIN32)
		typedef __int32 int32_t;
		typedef __int64 int64_t;
		typedef unsigned __int64 uint64_t;
	#else
		/* Fallback if nothing above works */
		#include <inttypes.h>
	#endif
#endif
	typedef unsigned int GLenum;
	typedef unsigned char GLboolean;
	typedef unsigned int GLbitfield;
	typedef void GLvoid;
	typedef signed char GLbyte;
	typedef short GLshort;
	typedef int GLint;
	typedef unsigned char GLubyte;
	typedef unsigned short GLushort;
	typedef unsigned int GLuint;
	typedef int GLsizei;
	typedef float GLfloat;
	typedef float GLclampf;
	typedef double GLdouble;
	typedef double GLclampd;
	typedef char GLchar;
	typedef char GLcharARB;
	#ifdef __APPLE__
		typedef void *GLhandleARB;
	#else
		typedef unsigned int GLhandleARB;
	#endif
	typedef unsigned short GLhalfARB;
	typedef unsigned short GLhalf;
	typedef GLint GLfixed;
	typedef ptrdiff_t GLintptr;
	typedef ptrdiff_t GLsizeiptr;
	typedef int64_t GLint64;
	typedef uint64_t GLuint64;
	typedef ptrdiff_t GLintptrARB;
	typedef ptrdiff_t GLsizeiptrARB;
	typedef int64_t GLint64EXT;
	typedef uint64_t GLuint64EXT;
	typedef struct __GLsync *GLsync;
#endif /*GL_LOAD_GEN_BASIC_OPENGL_TYPEDEFS*/
