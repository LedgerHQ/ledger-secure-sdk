#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#include "unity.h"

#include "exceptions.h"

#define EXCEPTION_A 0x6A80

void setUp(void) {}

void tearDown(void) {}

void test_throw_inside_try_is_caught(void)
{
    volatile bool caught = false;

    BEGIN_TRY
    {
        TRY
        {
            THROW(EXCEPTION_A);
        }
        CATCH(EXCEPTION_A)
        {
            caught = true;
        }
        FINALLY {}
    }
    END_TRY;

    TEST_ASSERT_TRUE(caught);
}

// The throw fails the test: run it in a child process, which exits 0 only on a recorded failure
void test_throw_outside_try_fails_the_test(void)
{
    fflush(stdout);
    pid_t pid = fork();
    TEST_ASSERT_NOT_EQUAL(-1, pid);

    if (pid == 0) {
        freopen("/dev/null", "w", stdout);
        if (TEST_PROTECT()) {
            THROW(EXCEPTION_A);
        }
        _exit(Unity.CurrentTestFailed ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    int status = 0;
    TEST_ASSERT_EQUAL(pid, waitpid(pid, &status, 0));
    TEST_ASSERT_TRUE_MESSAGE(WIFEXITED(status), "THROW outside of a TRY crashed the test");
    TEST_ASSERT_EQUAL_INT(EXIT_SUCCESS, WEXITSTATUS(status));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_throw_inside_try_is_caught);
    RUN_TEST(test_throw_outside_try_fails_the_test);
    return UNITY_END();
}
