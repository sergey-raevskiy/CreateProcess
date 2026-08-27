CreateProcess
=

A command line wrapper that gives full-control over `CreateProcess()` WinAPI function.

Usage
-

Basic usage:

```
CreateProcess [OPTIONS] [COMMAND_LINE]
```

The [OPTIONS] is parsed until they are prefixed with `-`, `--` or `/`. The rest of original command line is passed to `CreateProcess()`.
