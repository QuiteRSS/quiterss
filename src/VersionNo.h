#if defined(Q_WS_WIN)
#define PORTABLE       1
#endif

#define FILEVER         0,11,0,0
#define PRODUCTVER      FILEVER
#define STRFILEVER      "0.11.0.0\0"
#define STRDATE         "10.12.2012\0"
#define STRPRODUCTVER   "0.11.0\0"

#include "VersionRev.h"
#define FILEVER_FULL    0,11,0,HG_REVISION
#define STRFILEVER_FULL "0.11.0."#HG_REVISION"\0"

