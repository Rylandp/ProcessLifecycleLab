# proclab

A C11 Linux process-lifecycle lab demonstrating `fork()`, `execl()`, `/proc/<pid>/stat` zombie observation, and `waitpid()` child reaping.

## How It Works

- **`proclab` (`proclab.c`)**: Parses a child count `N` (`0`–`20`), spawns `N` child processes via `fork()`, and replaces each child image with `./proclab-worker` using `execl()` with expected exit code `10 + index`. Before calling `waitpid()`, it polls `/proc/<pid>/stat` (up to `2000 ms` at `5 ms` intervals using `CLOCK_MONOTONIC` and `nanosleep()`) to observe each child as a `proclab-worker` process in state `Z` (zombie), then reaps children via `waitpid()` and reports summary counts.
- **`proclab-worker` (`proclab-worker.c`)**: Validates a single integer `<exit_code>` (`0`–`255`) and exits immediately using `_exit()`.

## Requirements

- **Linux** with `/proc` filesystem access
- **`gcc`** supporting `-std=c11` (`-Wall -Wextra -Wpedantic -Werror`) and `make`

## Building and Running

### Build

```bash
make
```

### Run

```bash
./proclab <N>
```

Example (spawn and reap 4 worker processes):

```bash
./proclab 4
```

### Clean Build Artifacts

```bash
make clean
```

## Output Format

`proclab` disables stdout buffering (`_IONBF`) for deterministic output and emits structured status lines:

- `SPAWN index=<i > pid=<pid> exit=<10+i>` — Child forked and launched
- `ZOMBIE index=<i> pid=<pid> name=proclab-worker state=Z` — Child observed in zombie state via `/proc/<pid>/stat`
- `REAP index=<i> pid=<pid> exit=<code>` — Child reaped and exit code verified
- `DONE spawned=<n> reaped=<m>` — Final summary line

## Project Structure

```text
.
├── Makefile          # Builds proclab and proclab-worker with strict C11 warnings-as-errors
├── proclab.c         # Parent process spawner, /proc zombie detector, and waitpid reaper
├── proclab-worker.c  # Worker executable that validates and returns an exit code via _exit()
└── .gitignore        # Ignores compiled proclab and proclab-worker binaries
```[cite: 8, 9, 10, 11]
```
