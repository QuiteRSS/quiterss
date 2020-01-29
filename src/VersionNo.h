#include "VersionRev.h"

#define STRDATE           "29.01.2020\0"
#define STRPRODUCTVER     "0.19.3\0"

#define VERSION           0,19,3
#define PRODUCTVER        VERSION,0
#define FILEVER           VERSION,VCS_REVISION

#define _STRFILE_BUILD(n) #n
#define STRFILE_BUILD(n)  _STRFILE_BUILD(n)
#define STRFILEVER_FULL   STRPRODUCTVER "." STRFILE_BUILD(VCS_REVISION) "\0"
