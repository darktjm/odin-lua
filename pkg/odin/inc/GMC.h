#ifndef GMC_HEADER
#define GMC_HEADER

typedef int boolean;
#ifndef FALSE
#define			FALSE 0
#endif
#if (FALSE != 0)
#define			FALSE 0
#endif
#ifndef TRUE
#define			TRUE 1
#endif
#if (TRUE != 1)
#define			TRUE 1
#endif

#ifndef ERROR
#define			ERROR 0
#endif
#if (ERROR != 0)
#define			ERROR 0
#endif

#ifndef NIL
#define			NIL 0
#endif
#if (NIL != 0)
#define			NIL 0
#endif

#include "System.hh"
#include "Type.hh"
#include "Var.hh"
#include "Func.hh"
#include "Macro.h"

#endif
