## description on `StdPort/`

std-style header provisions. only `allocator.h, log.h, port_errno.h, port_unistd.h` is currently in use.

- `allocator.h`: overridding malloc/free provided by microlib with freertos port-malloc and port-free
- `log.h`: overridding printf/puts provided by microlib with self-defined printf/puts
- `port_errno.h`: `<errno.h>` reimpl
- `port_unistd.h`: `<unistd.h>` reimpl, several types and functions, namely endian-swap(e.g. htonl)