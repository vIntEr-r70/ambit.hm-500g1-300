#include <aem/aem.h>
#include <aem/timer.h>
#include <aem/time_span.h>

#include "hw.h"
#include "worker.h"

int main(int argc, char **argv)
{
    hw::create();

    worker worker_(hw::ref());
    worker_.activate();

    return aem::run([&](int retcode)
            {
                hw::destroy();
                return retcode;
            });
}
