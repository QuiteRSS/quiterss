#include "VersionRev.h"

#if defined(Q_WS_WIN)
#define PORTABLE       1
#endif

#define STRDATE           "10.12.2012\0"
#define STRPRODUCTVER     "0.11.0\0"

#define VERSION           0,11,0
#define PRODUCTVER        VERSION,0
#define FILEVER           VERSION,HG_REVISION

#define _STRFILE_BUILD(n) #n
#define STRFILE_BUILD(n)  _STRFILE_BUILD(n)
#define STRFILEVER_FULL   STRPRODUCTVER "." STRFILE_BUILD(HG_REVISION) "\0"
