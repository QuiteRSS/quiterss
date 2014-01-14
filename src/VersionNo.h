#include "VersionRev.h"

#if defined(Q_OS_WIN)
#define PORTABLE       1
#endif

#define STRDATE           "14.01.2014\0"
#define STRPRODUCTVER     "0.14.3\0"

#define VERSION           0,14,3
#define PRODUCTVER        VERSION,0
#define FILEVER           VERSION,VCS_REVISION

#define _STRFILE_BUILD(n) #n
#define STRFILE_BUILD(n)  _STRFILE_BUILD(n)
#define STRFILEVER_FULL   STRPRODUCTVER "." STRFILE_BUILD(VCS_REVISION) "\0"
