## description on `ssh/`

singleton ssh session impl

- `acpt_loop`: all-in-one accepted-scoket loop
- `packet_def`: static definitions of specific packet structs, also statically constructed message structs
- `packet`: packet recv, recv-encrypted, send-encrypted, adding paddings, decode to a vo_t or encode from a vo_t
- `sx_xxx`: ssh protocol impl(singleton), refer to openssh source code and rfc's
- `sftp_parse`: example sftp packet reading routine. no permission check, singleton, no file-descriptor middle layers(directly use FatFS), incomplete option support
- `sftp_task`: unused, incomplete
- `ssh_context`: context of a ssh session, used to generate hash, maintain session id , maintain sequence id,...

functionality supports:
- [x] ssh alg: kex=curve25519-sha256, cipher=chacha20-poly1305, key=ssh-ed25519, mac=< implicit >
- [x] ssh userauth: none
- [x] ssh channel request: subsystem-sftp
- [ ] ssh key re-exchange
- [ ] ssh proper disconnect
- [x] sftp cmd: realpath(dummy), rm, (l)stat(partially dummy), put, get, open, write(partially dummy), read, opendir, readdir

configs:
| macro                    | value | description                                                                                                                                                                                                 |
| ------------------------ | ----- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| SSH_DEFAULT_PACKET_ALIGN | 8     | mac size in the case of poly1305, necessary to decide padding                                                                                                                                               |
| SFTP_CHUNK               | 400   | read/write bytes per once; used to avoid heap overflow; for large write, it returns sftp success message after the first 400 bytes of the sftp packet, making to following write (continue_write) in silent |
