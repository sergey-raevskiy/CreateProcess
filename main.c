#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>

#define NL L"\n"

#define FOR_CREATION_FLAGS(DO)           \
    DO(CREATE_BREAKAWAY_FROM_JOB)        \
    DO(CREATE_DEFAULT_ERROR_MODE)        \
    DO(CREATE_NEW_CONSOLE)               \
    DO(CREATE_NEW_PROCESS_GROUP)         \
    DO(CREATE_NO_WINDOW)                 \
    DO(CREATE_PROTECTED_PROCESS)         \
    DO(CREATE_PRESERVE_CODE_AUTHZ_LEVEL) \
    DO(CREATE_SECURE_PROCESS)            \
    DO(CREATE_SEPARATE_WOW_VDM)          \
    DO(CREATE_SHARED_WOW_VDM)            \
    DO(CREATE_SUSPENDED)                 \
    DO(CREATE_UNICODE_ENVIRONMENT)       \
    DO(DEBUG_ONLY_THIS_PROCESS)          \
    DO(DEBUG_PROCESS)                    \
    DO(DETACHED_PROCESS)                 \
    DO(EXTENDED_STARTUPINFO_PRESENT)     \
    DO(INHERIT_PARENT_AFFINITY)

static int argc;
static LPCWSTR *argv;
static int argind;

static BOOL opt_take(LPCWSTR name, LPCWSTR *arg)
{
    if (argind >= argc)
        return FALSE;

    if (name && arg && (argind + 1) >= argc)
        return FALSE;

    if (name && wcscmp(argv[argind], name) == 0)
    {
        if (arg)
        {
            *arg = argv[argind + 1];
            argind += 2;
        }
        else
        {
            argind += 1;
        }

        return TRUE;
    }
    else if (name == NULL)
    {
        *arg = argv[argind];
        argind += 1;
    }
    else
    {
        return FALSE;
    }
}

static void print_usage(FILE *f)
{
#define __STRINGIFY_FLAG(f) L"      - " #f NL

    static const LPCWSTR usage =
        L"usage: CreateProcess [OPTIONS] @[COMMAND_LINE]" NL
        L"Valid options:" NL
        L"  -f [--creation-flag] ARG" NL
        L"    Specify process creation flags. The valid values are:" NL
        FOR_CREATION_FLAGS(__STRINGIFY_FLAG)
        ;

#undef __STRINGIFY_FLAG

    fputws(usage, f);
}

static int die_usage(LPCWSTR fmt, ...)
{
    va_list ap;

    fputws(L"Invalid usage: ", stderr);
    va_start(ap, fmt);
    vfwprintf(stderr, fmt, ap);
    va_end(ap);
    fputwc(L'\n', stderr);

    print_usage(stderr);

    return EXIT_FAILURE;
}

static int die_win32(DWORD err, LPCWSTR msg)
{
    fwprintf(stderr, L"Error: %s (error code %d).\n", msg, err);
    return EXIT_FAILURE;
}

static int die(LPCWSTR fmt, ...)
{
    va_list ap;

    fputws(L"Error: ", stderr);
    va_start(ap, fmt);
    vfwprintf(stderr, fmt, ap);
    va_end(ap);
    fputws(L".", stderr);

    return EXIT_FAILURE;
}

static DWORD parse_creation_flag(LPCWSTR str)
{
#define __CHECK_FLAG(f) if (wcscmp(str, L"" #f) == 0) return f;

    FOR_CREATION_FLAGS(__CHECK_FLAG);

#undef __CHECK_FLAG

    /* Default. */
    return 0;
}

static int run(LPCWSTR proc_cmdline)
{
    DWORD creation_flags = 0;
    STARTUPINFOW psi;
    PROCESS_INFORMATION pi;
    BOOL rc;

    if (!proc_cmdline)
    {
        return die_usage(L"No command line specified for process");
    }

    while (TRUE)
    {
        LPCWSTR val;

        if (opt_take(L"-f", &val) || opt_take(L"--creation-flag", &val))
        {
            DWORD flag = parse_creation_flag(val);

            if (flag == 0)
            {
                return die_usage(L"Unrecognized creation flag '%s'", val);
            }

            creation_flags |= flag;
        }
        else if (opt_take(NULL, &val))
        {
            return die_usage(L"Unrecognized option '%s'", val);
        }
        else
        {
            break;
        }
    }

    ZeroMemory(&psi, sizeof(psi));
    psi.cb = sizeof(psi);

    rc = CreateProcessW(
        NULL,
        proc_cmdline,
        NULL,
        NULL,
        FALSE,
        creation_flags,
        NULL,
        NULL,
        &psi,
        &pi);

    if (!rc)
    {
        return die_win32(GetLastError(), L"Failed to create process");
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return EXIT_SUCCESS;
}

int wmain()
{
    LPWSTR cmdline = wcsdup(GetCommandLine());
    LPWSTR proc_cmdline;
    int rc;

    if (!cmdline)
    {
        return die(L"Failed to allocate memory");
    }

    proc_cmdline = wcschr(cmdline, L'@');

    if (proc_cmdline)
    {
        *proc_cmdline = L'\0';
        proc_cmdline++;
    }

    argv = CommandLineToArgvW(cmdline, &argc);
    if (!argv)
    {
        DWORD err = GetLastError();
        free(cmdline);

        return die_win32(err, L"Failed to parse command line");
    }

    /* Skip program name. */
    argind = 1;

    rc = run(proc_cmdline);
    LocalFree(argv);
    free(cmdline);

    return rc;
}
