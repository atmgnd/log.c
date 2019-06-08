# log.c
A simple logging library implemented in C99


## Usage
**[log.c](src/log.c?raw=1)** and **[log.h](src/log.h?raw=1)** should be dropped
into an existing project and compiled along with it.

#### log_set_level(int level)
The current logging level can be set by using the `log_set_level()` function.
All logs below the given level will be ignored. By default the level is
`LOG_TRACE`, such that nothing is ignored.


```
2019-05-03 15:31:41.898 TRACE [16727]: Hello world
```

#### log_log(int level, const char *fmt, ...)
print log in printf like way

### log_register_cmd(const char *command, lsh_fn fn);
register a command to telnet console

### log_server_loop(int port);
start the main telnet loop

## License
This library is free software; you can redistribute it and/or modify it under
the terms of the MIT license. See [LICENSE](LICENSE) for details.
