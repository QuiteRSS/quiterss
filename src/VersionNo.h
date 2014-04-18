#include "VersionRev.h"

#define STRDATE           "18.04.2014\0"
#define STRPRODUCTVER     "0.15.4\0"

#define VERSION           0,15,4
#define PRODUCTVER        VERSION,0
#define FILEVER           VERSION,VCS_REVISION

#define _STRFILE_BUILD(n) #n
#define STRFILE_BUILD(n)  _STRFILE_BUILD(n)
#define STRFILEVER_FULL   STRPRODUCTVER "." STRFILE_BUILD(VCS_REVISION) "\0"
