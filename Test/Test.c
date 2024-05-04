#include "../Lib/BMBase.h"
#include <string.h>
#include <strings.h>

int TickUT();
int STickUT();
int EvPoolUT();
int BufferUT();
int CommUT();
int RingBufferUT();
int CommThUT();
int EvQPoolUT();
int LockNUnlockUT();

int HasSth(int argc, const char* argv[], const char* sth)
{
    return
        ((argc > 1) &&
        (0 == strcasecmp(argv[1], sth)));
}

#define DO_TICKUT(_argc, _argv) HasSth(_argc, _argv, "tick")
#define DO_STICKUT(_argc, _argv) HasSth(_argc, _argv, "stick")
#define DO_SVR(_argc, _argv) HasSth(_argc, _argv, "svr")
#define DO_CLI(_argc, _argv) HasSth(_argc, _argv, "cli")
#define DO_COMM(_argc, _argv) HasSth(_argc, _argv, "comm")
#define DO_COMMTH(_argc, _argv) HasSth(_argc, _argv, "commth")
#define DO_LOCKNUNLOCK(_argc, _argv) HasSth(_argc, _argv, "lock")


int main(int argc, const char* argv[])
{
    int err = EXIT_SUCCESS;
    do {
        BMBASELOCK_INIT;
        if (DO_TICKUT(argc, argv) &&
            (EXIT_SUCCESS != (err = TickUT())))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in TickUT() = %d", err);
        }
        else if (DO_STICKUT(argc, argv) && 
            (EXIT_SUCCESS != (err = STickUT())))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in STickUT() = %d", err);
        }
        /*
        else if (DO_SVR(argc, argv) && EXIT_SUCCESS !=(err = Svr()))
        {
            BMERR_LOGBREAKEX("Fail in Svr() = %d", err);
        }
        else if (DO_CLI(argc, argv) && EXIT_SUCCESS !=(err = Cli()))
        {
            BMERR_LOGBREAKEX("Fail in Cli() = %d", err);
        }
        */
        else if (DO_COMM(argc, argv) && EXIT_SUCCESS != (err = CommUT()))
        {
            BMERR_LOGBREAKEX("Fail in Comm() = %d", err);
        }
        else if (DO_COMMTH(argc, argv) && EXIT_SUCCESS != (err = CommThUT()))
        {
            BMERR_LOGBREAKEX("Fail in CommTh() = %d", err);
        }
        else if (DO_LOCKNUNLOCK(argc, argv) && EXIT_SUCCESS != (err = LockNUnlockUT()))
        {
            BMERR_LOGBREAKEX("Fail in LockNUnlockUT() = %d", err);
        }
        // if it did one of special tests, it terminates the program.
        if (DO_TICKUT(argc, argv) ||
            DO_STICKUT(argc, argv) ||
            DO_COMM(argc, argv) ||
            DO_COMMTH(argc, argv) ||
            DO_LOCKNUNLOCK(argc, argv))
        {
            break;
        }

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
        if (EXIT_SUCCESS != (err = RingBufferUT()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in RingBufferUT() = %d", err);
        }
        if (EXIT_SUCCESS != (err = EvQPoolUT()))
        {
            BMERR_LOGBREAK(__FILE__, __FUNCTION__, __LINE__,
                "Fail in EvQPoolUT() = %d", err);
        }
    } while (0);
    BMEND_FUNCEX(err);
    BMBASELOCK_DESTROY;
    return err;
}