#include "../Lib/BMBase.h"
#include <string.h>
#include <strings.h>

int TickUT();
int EvPoolUT();
int BufferUT();

int HasSth(int argc, const char* argv[], const char* sth)
{
    return
        ((argc > 1) &&
        (0 == strcasecmp(argv[1], sth)));
}

#define DO_TICKUT(_argc, _argv) HasSth(_argc, _argv, "tick")
#define DO_SVR(_argc, _argv) HasSth(_argc, _argv, "svr")
#define DO_CLI(_argc, _argv) HasSth(_argc, _argv, "cli")


int main(int argc, const char* argv[])
{
    int err = EXIT_SUCCESS;
    do {
        if (DO_TICKUT(argc, argv) &&
            (EXIT_SUCCESS != (err = TickUT())))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in TickUT() = %d", err);
        }
        /*
        else if (DO_SVR(argc, argv) && EXIT_SUCCESS !=(err = Svr()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in Svr() = %d", err);
        }
        else if (DO_CLI(argc, argv) && EXIT_SUCCESS !=(err = Cli()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in Cli() = %d", err);
        }
        */
        // if it did one of special tests, it terminates the program.
        if (
            DO_TICKUT(argc, argv) || 
            DO_SVR(argc, argv) || 
            DO_CLI(argc,argv)) break;

        // beginning of general tests
        if (EXIT_SUCCESS != (err = EvPoolUT()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in EvPoolUT() = %d", err);
        }
        if (EXIT_SUCCESS != (err = BufferUT()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in BufferUT() = %d", err);
        }
    } while (0);
    BMEND_FUNC(__FILE__, __FUNCTION__, __LINE__, err);
    return err;
}