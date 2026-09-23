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

#define FOR_LOGON_FLAGS(DO)       \
    DO(LOGON_WITH_PROFILE)        \
    DO(LOGON_NETCREDENTIALS_ONLY)

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

    fputws(
        L"Usage: CreateProcess [OPTIONS] @[COMMAND_LINE]" NL
        L"" NL
        L"The first '@' terminates CreateProcess options." NL
        L"Everything following it is passed verbatim as the child process command line." NL
        L"" NL
        L"Valid options:" NL
        L"" NL
        L"  --with-logon" NL
        L"    Create process with using logon credentials (call the CreateProcessWithLogon() function)." NL
        L"" NL
        L"  --username ARG, --domain ARG, --password ARG" NL
        L"    Specify username, domain and password for logon." NL
        L"" NL
        L"  -l [--logon-flag] ARG" NL
        L"    Specify process logon flags. The valid values are:" NL
        FOR_LOGON_FLAGS(__STRINGIFY_FLAG)
        L"" NL
        L"  -f [--creation-flag] ARG" NL
        L"    Specify process creation flags. The valid values are:" NL
        FOR_CREATION_FLAGS(__STRINGIFY_FLAG),
        f);

#undef __STRINGIFY_FLAG
}

static int die_usage(LPCWSTR fmt, ...)
{
    va_list ap;

    fputws(L"Invalid usage: ", stderr);
    va_start(ap, fmt);
    vfwprintf(stderr, fmt, ap);
    va_end(ap);
    fputws(L"\n\n", stderr);

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

#define __CHECK_FLAG(f) if (wcscmp(str, L"" #f) == 0) return f;

static DWORD parse_creation_flag(LPCWSTR str)
{
    FOR_CREATION_FLAGS(__CHECK_FLAG);

    /* Default. */
    return 0;
}

static DWORD parse_logon_flag(LPCWSTR str)
{
    FOR_LOGON_FLAGS(__CHECK_FLAG);

    /* Default. */
    return 0;
}

#undef __CHECK_FLAG

static int run(LPCWSTR proc_cmdline)
{
    BOOL with_logon = FALSE;
    LPCWSTR username = NULL;
    LPCWSTR domain = NULL;
    LPCWSTR password = NULL;
    DWORD logon_flags = 0;
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

        if (opt_take(L"--with-logon", NULL))
        {
            with_logon = TRUE;
        }
        else if (opt_take(L"--username", &val))
        {
            username = val;
        }
        else if (opt_take(L"--domain", &val))
        {
            domain = val;
        }
        else if (opt_take(L"--password", &val))
        {
            password = val;
        }
        else if (opt_take(L"-l", &val) || opt_take(L"--logon-flag", &val))
        {
            DWORD flag = parse_logon_flag(val);

            if (flag == 0)
            {
                return die_usage(L"Unrecognized logon flag '%s'", val);
            }

            logon_flags |= flag;
        }
        else if (opt_take(L"-f", &val) || opt_take(L"--creation-flag", &val))
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
            return die_usage(L"Option '%s' is unrecognized or requires an argument.", val);
        }
        else
        {
            break;
        }
    }

    ZeroMemory(&psi, sizeof(psi));
    psi.cb = sizeof(psi);

    if (with_logon)
    {
        rc = CreateProcessWithLogonW(
            username,
            domain,
            password,
            logon_flags,
            NULL,
            proc_cmdline,
            creation_flags,
            NULL,
            NULL,
            &psi,
            &pi);
    }
    else
    {
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
    }

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
