## description on `types/`

`vo`: variable length string/buffer and variable length list and variant object; further behavioral descriptions are in the source files

usage of vstr_t:
- manage as a fat-pointer: `vstr_init, vstr_clear`
- manage as instance; `vstr_create, vstr_delete`
- concatenate 2 string into the first one: `vbuff_iadd(str,pbuff,bufflen)`
- ensure null-terminated: `vstr_iaddc(str,"")`
- neglect tail data: `str->len=...`
- clear but no deallocating: `str->len=0`
- ssh style usage:
  - `vbuff_iaddc`: add a byte
  - `vbuff_iaddu32`: swap endian and add 4 bytes
  - `vbuff_iaddmp`: add ssh-style mpint(multi-precision integer)