# mini_serv

A lightweight TCP chat server written in C that allows multiple clients to communicate with each other over `127.0.0.1`.

---

## Subject

**Assignment:** mini_serv  
**Expected files:** `mini_serv.c`

### Allowed functions
`write`, `close`, `select`, `socket`, `accept`, `listen`, `send`, `recv`, `bind`, `strstr`, `malloc`, `realloc`, `free`, `calloc`, `bzero`, `atoi`, `sprintf`, `strlen`, `exit`, `strcpy`, `strcat`, `memset`

---

## How It Works

- The server listens for incoming client connections on `127.0.0.1` at the port passed as argument.
- Each client that connects is assigned an incrementing ID starting from `0`.
- Clients can send messages to the server, which then broadcasts them to all other connected clients.
- The server is **non-blocking** — it uses `select()` to monitor file descriptors and never blocks on a slow or unresponsive client.

---

## Usage

### Compile
```bash
cc -Wall -Wextra -Werror -o mini_serv mini_serv.c
```

### Run
```bash
./mini_serv <port>
```

### Test with `nc`
Open multiple terminals and connect with netcat:
```bash
nc 127.0.0.1 <port>
```

---

## Behavior

### On client connect
All currently connected clients receive:
```
server: client %d just arrived
```

### On message received
The message is forwarded to all **other** clients prefixed with:
```
client %d: <message>
```

> Messages are printable characters only, always end with `\n`, and are at most **4096 characters** long.

### On client disconnect
All remaining clients receive:
```
server: client %d just left
```

---

## Error Handling

| Situation | Output (stderr) | Exit code |
|---|---|---|
| Wrong number of arguments | `Wrong number of arguments` | `1` |
| System call failure before accepting | `Fatal error` | `1` |
| Memory allocation failure | `Fatal error` | `1` |

---

## Constraints

- No `#define` preprocessor directives allowed.
- Must only listen on `127.0.0.1`.
- Must not disconnect a client just because they are slow to read.
- No memory or file descriptor leaks.
- `select()` must be called before any `send` or `recv`.

---

## Implementation Notes

- `select()` is used to multiplex all file descriptors (server + clients) in a single thread.
- A per-client message buffer accumulates partial messages until a `\n` is received, at which point the full message is broadcast.
- Write readiness (`write_fds`) is checked via `select` before sending to each client, ensuring non-blocking behavior without ever checking `EAGAIN`.