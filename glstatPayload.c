//
// glstatPayload.c
// Copyright (c) 2014 Matthew Henry.
// MIT licensed - refer to LICENSE.txt for details.
//

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <OpenGL/OpenGL.h>
#include <stdio.h>
#include <unistd.h>
#include "mach_override/mach_override.h"

FILE *fin, *fout, *ferr;
CGLError (*CGLFlushDrawableReentry)(CGLContextObj);
mach_timebase_info_data_t timebaseInfo;
double startTime;

double getTime(void)
{
    uint64_t t = mach_absolute_time();
    return (t * timebaseInfo.numer / timebaseInfo.denom)/1e9;
}

CGLError CGLFlushDrawableOverride(CGLContextObj ctx)
{
    fprintf(fout, "%p %.6f\n", (void *)ctx, getTime() - startTime);
    return CGLFlushDrawableReentry(ctx);
}

void payload_main(int in, int out, int err)
{
    fin = fdopen(in, "r");
    fout = fdopen(out, "w");
    ferr = fdopen(out, "w");

    setvbuf(fin, NULL, _IONBF, 0);
    setvbuf(fout, NULL, _IONBF, 0);
    setvbuf(ferr, NULL, _IONBF, 0);

    mach_timebase_info(&timebaseInfo);
    startTime = getTime();

    if (mach_override_ptr((void *)CGLFlushDrawable, 
        (void *)CGLFlushDrawableOverride, (void **)&CGLFlushDrawableReentry))
    {
        fprintf(ferr, "glstat: failed to override CGLFlushDrawable\n");
        return;
    }
}
