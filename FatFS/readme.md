## description of `FatFS/`

files here are FatFS third party and configurations. to config fatfs:
- amend config macros in `ffconf.h`
- impl port functions in `diskcio.c`

overview of current config:

| macro           | value | description                                 |
| --------------- | ----- | ------------------------------------------- |
| FF_FS_MINIMIZE  | 0     | full api                                    |
| FF_USE_LFN      | 3     | filename on heap                            |
| FF_FS_RPATH     | 2     | enable chdir, getcwd,...                    |
| FF_FS_TINY      | 1     | tniy mode                                   |
| FF_FS_NORTC     | 1     | no timestamp                                |
| FF_FS_REENTRANT | 1     | io should use lock (must for multi-tasking) |
| OS_TYPE         | 3     | FreeRTOS as os                              |


> TODO: override malloc/free and test