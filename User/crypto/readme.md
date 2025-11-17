## description on `crypto/`

cryptography functions for ssh

most sources are from openssh(further take roots in openbsd or supercop)

- `byte_order.h` pseudo header, indicating little endian
- `chacha`: chacha20 cipher(encryption algorithm) standard impl, from openssh
- `crypto_api.h`: export header for this dir
- `ed25519.c`: ed25519 key(host key signature algorithm) standard impl, from openssh
- `ed25519-4...`: self-defined ed25519, complying to rfc8032, trading speed for size
- `hash.c`: wrapper for sha256 and sha512, amended, from openssh
- `off_t.h`: pseudo header providing off_t
- `poly1305`: mac(message authentication code algorithm) standard impl, from openssh
- `sha2`: sha256 and sha512 impl
- `smul_curve25519_ref.c`: kex(key exchange algorithm) core function standard impl, from openssh