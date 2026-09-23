CreateProcess
=

A command line wrapper that gives full-control over `CreateProcess()` WinAPI function.

This is a low level tool with minimum implications.

Usage
-

Basic usage:

```
CreateProcess [OPTIONS] @[COMMAND_LINE]
```

The `[OPTIONS]` is parsed until `@` symbol is found. The rest of command line is passed to the `CreateProcess()` as `lpCommandLine` parameter.

Exampes:

```
CreateProcess -f CREATE_SUSPENDED --prompt-resume @notepad.exe C:\FileToOpen.txt
```

To start process as different user:

```
CreateProcess --with-logon ^
    --domain CONTOSO ^
    --username Jack ^
    --password pAssw0rd ^
    --logon-flag LOGON_WITH_PROFILE ^
    @^
    cmd.exe
```

Environment variables
-

When creating a process with custom environment, you must explicitely start a
new environment block with `--environment` option.

Environment variables (`-e` or `--environment-variable`) must be specified after
that option, otherwise the error will occur.

To create a process with unicode environment block, you must specify `CREATE_UNICODE_ENVIRONMENT` flag explicitely. Otherwise the ANSI environment block will be used.

Examples:

To start a process with altered environment block of parent process:

```
CreateProcess --environment=copy -e PATH=<custom path> -e TEMP=C:\Temp -f CREATE_UNICODE_ENVIRONMENT @cmd.exe
```

To start a process with completelly custom environment block:

```
CreateProcess --environment=empty -e TEMP=C:\Temp -f CREATE_UNICODE_ENVIRONMENT @cmd.exe
```

In this case, only `TEMP` variable will be passed to created process.
