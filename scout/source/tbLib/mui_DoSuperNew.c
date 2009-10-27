#include <exec/ports.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <libraries/mui.h>
#include <proto/muimaster.h>
#include <proto/intuition.h>
#include <clib/alib_protos.h>
#include "tblib.h"
#include "SDI_compiler.h"

/* /// "DoSuperNew()" */
#if defined(__AROS__)
IPTR DoSuperNew(struct IClass *cl, Object *obj, IPTR tag1, ...)
{
  AROS_SLOWSTACKTAGS_PRE(tag1)
  retval = DoSuperMethod(cl, obj, OM_NEW, AROS_SLOWSTACKTAGS_ARG(tag1));
  AROS_SLOWSTACKTAGS_POST
}
#else
ULONG VARARGS68K STDARGS DoSuperNew( struct IClass *cl,
                                     Object *obj, ... )
{
    ULONG rc;
    VA_LIST args;

    VA_START(args, obj);
    rc = DoSuperMethod(cl, obj, OM_NEW, VA_ARG(args, ULONG), NULL);
    VA_END(args);

    return rc;
}
#endif

/* \\\ */
