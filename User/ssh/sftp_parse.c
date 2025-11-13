#include "sftp_parse.h"
#include "packet_def.h"
#include "port_socket.h"
#include "portmacro.h"
#include "projdefs.h"
#include "ssh/packet.h"
#include "ssh_context.h"
#include "types/vo.h"

#include "ff.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "allocator.h"
#include "log.h"

#define DS_LEN 0
#define DS_REG 1

// singelton recv

vstr_t chnldat;
vstr_t pre_send_data;
volatile uint8_t sftp_init_done = 0;
uint8_t dpch_state;
uint32_t dpch_len;
uint32_t dpch_cnt;

void sftp_parser_init() {
  //
  vstr_init(&chnldat, 0);
  vstr_init(&pre_send_data, 0);
  sftp_init_done = 1;
  dpch_state = DS_LEN;
  dpch_len = 4;
  dpch_cnt = 0;
}

void sftp_parse(int sock, ssh_context *ctx, QueueHandle_t q,
                int *should_close) {
  if (!sftp_init_done) {
    *should_close = 0;
    return;
  }

  puts("sftp parser: waiting");
  vstr_t *recvd = recv_packet_enc(sock, ctx);
  // puts("sftp recvd:");
  // vbuff_dump(recvd);

  uint8_t opcode = recvd->buff[1];
  // printf("sftp: parse ssh opcode: %u\r\n", opcode);

  *should_close = 0;
  switch (opcode) {
  case SSH_MSG_CHANNEL_DATA: {
    vo_t *vo = payload_decode(&recvd->buff[1], recvd->len, chnldat_types, 3);
    // printf("sftp: ssh recip id:%u\r\n", vo->vlist.data[1].u32);
    const vstr_t *pstr = &vo->vlist.data[2].vstr;
    // puts("sftp: data:");
    // vbuff_dump(pstr);

    if (q) {

      // queue exist, use solo task to parse
      vstr_t vtmp;
      vstr_init(&vtmp, 0);
      vbuff_iadd(&vtmp, pstr->data, pstr->len);

      BaseType_t pass = xQueueSend(q, &vtmp, pdMS_TO_TICKS(2000));
      if (pass != pdPASS) {
        puts("sftp: data block timeout, may be malfunctioning");
        *should_close = 1;
      }

    } else {

      // queue not exist, immediate parse
      for (uint32_t i = 0; i < pstr->len; i++) {
        sftp_dispatch(sock, ctx, pstr->buff[i], should_close);
      }
    } // !q

    vo_delete(vo);
  } break;

  case SSH_MSG_CHANNEL_WINDOW_ADJUST:
    // skip
    break;

  case SSH_MSG_CHANNEL_EOF:
    *should_close = 1;
    break;

  case SSH_MSG_CHANNEL_CLOSE:
    *should_close = 1;
    break;

  case SSH_MSG_KEXINIT:
    puts("key re-exchange not impl, close");
    *should_close = 1;
    break;

  default:
    printf("sftp: warn: unknown message\r\n");
    break;
  } // switch opcode

  vstr_delete(recvd);
}

void sftp_dispatch(int sock, ssh_context *ctx, uint8_t c,
                   int *should_close) { //
  // printf("dispatch: %x\r\n", c);

  switch (dpch_state) {

  case DS_LEN: {
    vbuff_iaddc(&chnldat, c);
    dpch_cnt++;

    if (dpch_cnt == 4) {
      dpch_len = ntohl(*(uint32_t *)chnldat.buff);
      // printf("sftp packet len: %u\r\n", dpch_len);
      vstr_clear(&chnldat);

      if (dpch_len > SFTP_CHUNK) {
        puts("sftp error: pkt too big");
        *should_close = 1;
        return;
      }

      dpch_cnt = 0;
      dpch_state = dpch_len != 0 ? DS_REG : DS_LEN;
    }
  } break;

  case DS_REG: {
    vbuff_iaddc(&chnldat, c);
    dpch_cnt++;

    if (dpch_cnt >= dpch_len) {
      sftp_dispatch_spkt(sock, ctx, &chnldat, should_close);
      vstr_clear(&chnldat);
      dpch_cnt = 0, dpch_len = 4;
      dpch_state = DS_LEN;
    }
  } break;

  default:
    assert(0);
  } // switch
}

//
//
// sftp packet support

typedef struct {
  uint8_t is_dp;
  union {
    FIL *fp;
    DIR *dp;
  };
} sftp_handle;

static void sftp_send_pkt(int sock, ssh_context *ctx, const vstr_t *buff);
static void send_version(int sock, ssh_context *ctx);
static void send_status(int sock, ssh_context *ctx, uint32_t id,
                        uint32_t status, const char *msg);
static void send_handle(int sock, ssh_context *ctx, uint32_t id,
                        sftp_handle hndl);
static void send_data(int sock, ssh_context *ctx, uint32_t id, const void *buff,
                      uint32_t len);

void sftp_dispatch_spkt(int sock, ssh_context *ctx, const vstr_t *pkt,
                        int *should_close) {
  puts("dispatch sftp packet:...");
  buff_dump(pkt->data, pkt->len);
  puts("");

  if (pkt->len < 5) {
    // must have opcode:u8, id-or-version:u32
    puts("sftp: packet too small");
    *should_close = 1;
    return;
  }

  uint32_t id = ntohl(*(uint32_t *)(pkt->buff + 1));

  // common ret:
  // handle: u8 u32 str
  // data: u8 u32 str
  // name: u8 u32 u32[cnt] {str str attr} (repeat by cnt)
  // attr(packet) u8 u32 attr(local struct)
  // a minimal struct: u32(0)

  switch (pkt->buff[0]) {

  case SSH_FXP_INIT: {
    uint32_t ver = ntohl(*(uint32_t *)(pkt->buff + 1));
    printf("sftp: recv init (version=%u)\r\n", ver);
    send_version(sock, ctx);
    puts("sftp: send version (=3)");
  } break;

  case SSH_FXP_OPEN: {
    puts("FXP_OPEN");
    // in: u8 u32 str(fn) u32(pflags) attr
    // where pflags: rwa, create, trunc
    // out: handle

    uint32_t path_len = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (path_len >= 256 || pkt->len < 9 + path_len + 4) {
      printf("FXP_OPEN: path too long: %u exceeds %u\r\n", path_len, pkt->len);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "filename too long");
      return;
    }

    vstr_t path;
    vstr_init(&path, 0);
    vbuff_iadd(&path, pkt->data + 9, path_len);
    vstr_iaddpc(&path, ""); // null terminate

    uint32_t flags = ntohl(*(uint32_t *)(pkt->buff + 9 + path_len));
    flags &= 0xff; // 8bits [0..2]=rwa
    uint8_t flag2 = 0;

    // fuzzy flags
    if (flags & SSH_FXF_READ)
      flag2 |= FA_READ;
    if (flags & SSH_FXF_WRITE)
      flag2 |= FA_WRITE | FA_CREATE_ALWAYS;
    if (flags & SSH_FXF_APPEND)
      flag2 |= FA_OPEN_APPEND;
    if (flags & SSH_FXF_CREAT)
      flag2 |= FA_CREATE_ALWAYS;
    if (flags & SSH_FXF_TRUNC) {
      flag2 &= ~FA_OPEN_APPEND;
    }

    // create file
    FIL *fp = malloc(sizeof(FIL));
    FRESULT fr;
    fr = f_open(fp, path.data, flag2); // mode?

    vstr_clear(&path);
    (void)path;

    if (fr) {
      printf("FXP_OPEN: file open failed with %x\r\n", (uint32_t)fr);
      free(fp);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "fatfs fail");
      return;
    }
    printf("FXP_OPEN: handle:%u\r\n", (uint32_t)(uint64_t)fp);

    send_handle(sock, ctx, id, (sftp_handle){.is_dp = 0, .fp = fp});

    puts("FXP_OPEN: done");
  } break;

  case SSH_FXP_CLOSE: {
    puts("FXP_CLOSE");
    // in: u8 u32 str(handle)
    // out status

    uint32_t fp_size = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (fp_size != sizeof(sftp_handle)) {
      printf("FXP_CLOSE: wrong handle size %u\r\n", fp_size);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "wrong handle size");
      return;
    }

    sftp_handle p = *(sftp_handle *)(pkt->buff + 9);
    FRESULT fr;
    fr = p.is_dp ? f_closedir(p.dp) : f_close(p.fp);

    if (fr) {
      printf("FXP_CLOSE: failed %u", (uint32_t)fr);
      // do not free a possible invalid handle
      send_status(sock, ctx, id, SSH_FX_FAILURE, "fatfs fail");
      return;
    }

    puts("FXP_CLOSE: success");
    send_status(sock, ctx, id, SSH_FX_OK, "");
    free(p.is_dp ? (void *)p.dp : (void *)p.fp);

    puts("FXP_CLOSE: done");
  } break;

  case SSH_FXP_READ: {
    puts("FXP_READ");
    // in: u8 u32 str(handle) u64(offs) u32(len)
    // out: data

    uint32_t fp_size = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (fp_size != sizeof(sftp_handle)) {
      printf("FXP_READ: wrong handle size %u\r\n", fp_size);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "wrong handle size");
      return;
    }

    // puts("here");
    sftp_handle p = *(sftp_handle *)(pkt->buff + 9);
    // puts("here");
    uint64_t offs = ntohll(*(uint64_t *)(pkt->buff + 9 + fp_size));
    // puts("here");
    uint32_t len = ntohl(*(uint32_t *)(pkt->buff + 9 + fp_size + 8));
    puts("here");
    if (p.is_dp) {
      puts("FXP_READ: a dir handle");
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "not a file");
      return;
    }
    if (len > SFTP_CHUNK) {
      // printf("FXP_READ: read too long %u\r\n", len);
      // send_status(sock, ctx, id, SSH_FX_FAILURE, "read too long");
      // return;
      len = SFTP_CHUNK;
    }

    // call fatfs: seek+read
    printf("FXP_READ: seek %u\r\n", (uint32_t)offs);
    FRESULT fr;
    fr = f_lseek(p.fp, (FSIZE_t)offs);
    if (fr) {
      printf("FXP_READ: seek failed %u\r\n", fr);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "seek failed");
      return;
    }

    void *pdata = malloc(len);

    printf("FXP_READ: read %u (handle: %u)\r\n", len, (uint32_t)(uint64_t)p.fp);
    fr = f_read(p.fp, pdata, len, &len);
    printf("FXP_READ actual read: %u\r\n", len);
    if (fr) {
      printf("FXP_READ: read failed %u\r\n", fr);
      free(pdata);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "fatfs fail");
      return;
    }

    // send
    if (len) {
      send_data(sock, ctx, id, pdata, len);
    } else {
      send_status(sock, ctx, id, SSH_FX_EOF, "");
    }

    free(pdata);

    puts("FXP_READ: done");
  } break;

  case SSH_FXP_WRITE: {
    puts("FXP_WRITE");
    // in: u8 u32 str(handle) u64(offs) str(data)
    // out: status

    uint32_t fp_size = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (fp_size != sizeof(sftp_handle)) {
      printf("FXP_WRITE: wrong handle size %u\r\n", fp_size);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "wrong handle size");
      return;
    }

    sftp_handle p = *(sftp_handle *)(pkt->buff + 9);
    uint64_t offs = ntohll(*(uint64_t *)(pkt->buff + 9 + fp_size));
    uint32_t len = ntohll(*(uint64_t *)(pkt->buff + 9 + fp_size + 8));
    if (len > SFTP_CHUNK) {
      printf("FXP_WRITE: read too long %u\r\n", len);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "write too long");
      return;
    }
    if (p.is_dp) {
      puts("FXP_WRITE: a dir handle");
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "not a file");
    }

    // call fatfs: seek+write
    FRESULT fr;
    fr = f_lseek(p.fp, (FSIZE_t)offs);
    if (fr) {
      printf("FXP_READ: seek failed %u\r\n", fr);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "seek failed");
      return;
    }

    const void *pdata = pkt->buff + 9 + fp_size + 12;
    fr = f_write(p.fp, pdata, len, &len);
    if (fr) {
      printf("FXP_WRITE: failed %u\r\n", fr);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "fatfs fail");
      return;
    }

    // send
    send_status(sock, ctx, id, SSH_FX_OK, "");

    puts("FXP_WRITE: done");
  } break;

    // case SSH_FXP_LSTAT:{}break;
    // case SSH_FXP_FSTAT:{}break;
    // case SSH_FXP_SETSTAT:{}break;
    // case SSH_FXP_FSETSTAT:{}break;

  case SSH_FXP_OPENDIR: {
    puts("FXP_OPENDIR");
    // in: u8 u32 str(path)
    // out: handle

    uint32_t path_len = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (path_len >= 256 || pkt->len < 9 + path_len) {
      printf("FXP_OPENDIR: path too long: %u exceeds %u\r\n", path_len,
             pkt->len);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "filename too long");
      return;
    }

    vstr_t path;
    vstr_init(&path, 0);
    vbuff_iadd(&path, pkt->data + 9, path_len);
    vstr_iaddpc(&path, ""); // null terminate

    // create dir pointer
    DIR *dp = malloc(sizeof(DIR));
    FRESULT fr;
    fr = f_opendir(dp, path.data);

    vstr_clear(&path);
    (void)path;

    if (fr) {
      printf("FXP_OPENDIR: failed %u\r\n", fr);
      free(dp);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "opendir fail");
      return;
    }

    send_handle(sock, ctx, id, (sftp_handle){.is_dp = 1, .dp = dp});

    puts("FXP_OPENDIR: done");
  } break;

  case SSH_FXP_READDIR: {
    puts("FXP_READDIR");
    // in: u8 u32 str(handle)
    // out: name

    uint32_t dp_size = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (dp_size != sizeof(sftp_handle)) {
      printf("FXP_READDIR: wrong handle size %u\r\n", dp_size);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "wrong handle size");
      return;
    }

    sftp_handle p = *(sftp_handle *)(pkt->buff + 9);
    if (!p.is_dp) {
      puts("FXP_READDIR: not a dir");
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "not a dir");
      return;
    }

    vstr_t buff; // buff to send: u8 u32 u32 [str str attr]*
    vstr_init(&buff, 0);

    vbuff_iaddc(&buff, SSH_FXP_NAME);
    vbuff_iaddu32(&buff, id);
    vbuff_iaddu32(&buff, 0); // count

    uint32_t cnt = 0;
    while (1) {
      FILINFO inf;
      FRESULT fr = f_readdir(p.dp, &inf);
      if (inf.fname[0] == 0 || fr)
        break;

      cnt++;
      // name
      vbuff_iaddu32(&buff, strlen(inf.fname));
      vbuff_iadd(&buff, inf.fname, strlen(inf.fname));
      // long name
      char longname[65];
      snprintf(longname, 65, "0x%x %u %u %s", inf.fattrib, inf.fdate,
               (uint32_t)inf.fsize, inf.fname);
      vbuff_iaddu32(&buff, strlen(longname));
      vbuff_iadd(&buff, longname, strlen(longname));

      // attr count
      vbuff_iaddu32(&buff, 0);
    }

    uint32_t *pcnt = (uint32_t *)(buff.buff + 5);
    *pcnt = htonl(cnt);

    // if(!fr)

    if (cnt) {
      sftp_send_pkt(sock, ctx, &buff);
    } else {
      send_status(sock, ctx, id, SSH_FX_EOF, "");
    }

    vstr_clear(&buff);

    puts("FXP_READDIR: done");
  } break;

  case SSH_FXP_REMOVE: {
    puts("FXP_REMOVE");
    // in: u8 u32 str(filename)
    // out: status

    uint32_t path_len = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (path_len >= 256 || pkt->len < 9 + path_len) {
      printf("FXP_REMOVE: path too long: %u exceeds %u\r\n", path_len,
             pkt->len);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "filename too long");
      return;
    }

    vstr_t path;
    vstr_init(&path, 0);
    vbuff_iadd(&path, pkt->data + 9, path_len);
    vstr_iaddpc(&path, ""); // null terminate

    FRESULT fr;
    fr = f_unlink(path.data);

    vstr_clear(&path);
    (void)path;

    if (fr) {
      printf("FXP_REMOVE: failed %u\r\n", fr);
      send_status(sock, ctx, id, SSH_FX_FAILURE, "fatfs fail");
      return;
    }

    send_status(sock, ctx, id, SSH_FX_OK, "");

    puts("FXP_REMOVE: done");
  } break;

    // case SSH_FXP_MKDIR:{}break;
    // case SSH_FXP_RMDIR:{}break;

  case SSH_FXP_REALPATH: {
    puts("FXP_REALPATH");
    // in: u8 u32 str(path)
    // out: name u8 u32 u32(1) str str attr

    // pkt unused, dummy. return cwd
    char path[65];
    FRESULT fr = f_getcwd(path, 65);
    if (fr) {
      printf("RXP_REALPATH: getcwd failed %u\r\n", fr);
      return;
    }

    vstr_t buff;
    vstr_init(&buff, 0);
    vbuff_iaddc(&buff, SSH_FXP_NAME);
    vbuff_iaddu32(&buff, id);
    vbuff_iaddu32(&buff, 1); // nb name

    // vbuff_iaddu32(&buff, strlen(path) + 2); // strlen
    // vbuff_iadd(&buff, "0:", 2);

    vbuff_iaddu32(&buff, strlen(path));    // strlen
    vbuff_iadd(&buff, path, strlen(path)); // cstr

    vbuff_iaddu32(&buff, 0); // str long name
    // dummy attr
    vbuff_iaddu32(&buff, 5); // flags: 1|4
    vbuff_iaddu64(&buff, 0); // size
    vbuff_iaddu32(&buff, 0x8000 | 0777);

    sftp_send_pkt(sock, ctx, &buff);

    vstr_clear(&buff);

    puts("FXP_REALPATH: done");
  } break;

  case SSH_FXP_LSTAT: // fall-through
  case SSH_FXP_STAT: {
    puts("FXP_(L)STAT");
    // in: u8 u32 str(path)
    // out: attrs(u8 u32 attr)

    uint32_t path_len = ntohl(*(uint32_t *)(pkt->buff + 5));
    if (path_len >= 256 || pkt->len < 9 + path_len) {
      printf("FXP_(L)STAT: path too long: %u exceeds %u\r\n", path_len,
             pkt->len);
      send_status(sock, ctx, id, SSH_FX_NO_SUCH_FILE, "filename too long");
      return;
    }

    vstr_t path;
    vstr_init(&path, 0);
    vbuff_iadd(&path, pkt->data + 9, path_len);
    vstr_iaddpc(&path, ""); // null terminate
    printf("FXP_(L)STAT: collect path [%u]%s\r\n", path.len, path.data);

    FRESULT fr;
    FILINFO inf;

    if (path.len == 1 && path.data[0] == '/') {
      puts("FXP_(L)STAT: root path");
      fr = 0;
      inf.fsize = 0;
      inf.fattrib = AM_DIR;
    } else {
      fr = f_stat(path.data, &inf);
    }

    if (fr) {
      printf("FXP_(L)STAT: failed %u with path [len=%u]%s\r\n", fr, path.len,
             path.data);
      vstr_clear(&path);
      (void)path;

      send_status(sock, ctx, id, SSH_FX_FAILURE, "stat fail");
      return;
    }

    vstr_clear(&path);
    (void)path;

    // pressent size(u64) and perm(u32), u8 u32 u32(1|4) u64 u32

    vstr_t buff;
    vstr_init(&buff, 0);

    vbuff_iaddc(&buff, SSH_FXP_ATTRS);
    vbuff_iaddu32(&buff, id);
    vbuff_iaddu32(&buff, 5);
    vbuff_iaddu64(&buff, inf.fsize);
    // fuzzy attr
    uint32_t perm = 0;
    if (inf.fattrib & AM_DIR)
      perm |= 0x4000;
    else
      perm |= 0x8000;
    if (inf.fattrib & AM_RDO)
      perm |= 0555;
    else
      perm |= 0777;
    vbuff_iaddu32(&buff, perm);

    puts("FXP_STAT: to send:");
    buff_dump(buff.data, buff.len);

    sftp_send_pkt(sock, ctx, &buff);

    vstr_clear(&buff);

    puts("FXP_(L)STAT: done");
  } break;

    // case SSH_FXP_RENAME: {} break;
    // case SSH_FXP_READLINK:{}break;
    // case SSH_FXP_SYMLINK:{}break;

  default: {
    puts("FXP: unimpl");
    // send an error status
    send_status(sock, ctx, id, SSH_FX_OP_UNSUPPORTED, "not impl");
  } break;
  }
}

///@todo: consider size overflow
static void sftp_send_pkt(int sock, ssh_context *ctx, const vstr_t *buff) {
  vstr_t payload; // ssh payload (channel data)
  vstr_init(&payload, 0);

  vbuff_iaddc(&payload, SSH_MSG_CHANNEL_DATA); // opcode
  vbuff_iaddu32(&payload, 0);                  // recipient id
  vbuff_iaddu32(&payload, buff->len + 4);      // string-len
  vbuff_iaddu32(&payload, buff->len);          // buff(buff.len)
  vbuff_iadd(&payload, buff->data, buff->len); // buff(buff.buff)

  // puts("sftp send: payload:");
  // vbuff_dump(&payload);

  vstr_t *packet = payload2packet(&payload, 16);

  vstr_clear(&payload);
  (void)payload;

  send_packet_enc(sock, ctx, packet->data, packet->len);

  vstr_delete(packet);
}

static void send_version(int sock, ssh_context *ctx) {
  vstr_t buff;
  vstr_init(&buff, 9);

  // vbuff_iaddu32(&buff, 5);
  vbuff_iaddc(&buff, SSH_FXP_VERSION);
  vbuff_iaddu32(&buff, 3);

  sftp_send_pkt(sock, ctx, &buff);

  vstr_clear(&buff);
}

static void send_status(int sock, ssh_context *ctx, uint32_t id,
                        uint32_t status, const char *msg) {
  vstr_t buff; // u8 u32 u32 str str
  vstr_init(&buff, 0);

  vbuff_iaddc(&buff, SSH_FXP_STATUS);
  vbuff_iaddu32(&buff, id);
  vbuff_iaddu32(&buff, status);
  vbuff_iaddu32(&buff, strlen(msg));
  vbuff_iadd(&buff, msg, strlen(msg));
  vbuff_iaddu32(&buff, 0);

  // puts("Debug: sftp sending status:");
  // vbuff_dump(&buff);

  sftp_send_pkt(sock, ctx, &buff);

  vstr_clear(&buff);
}

static void send_handle(int sock, ssh_context *ctx, uint32_t id,
                        sftp_handle p) {
  vstr_t buff; // u8 u32 str
  vstr_init(&buff, 0);

  vbuff_iaddc(&buff, SSH_FXP_HANDLE);
  vbuff_iaddu32(&buff, id);
  vbuff_iaddu32(&buff, sizeof(sftp_handle));
  vbuff_iadd(&buff, (void *)&p, sizeof(sftp_handle));

  sftp_send_pkt(sock, ctx, &buff);

  vstr_clear(&buff);
}

static void send_data(int sock, ssh_context *ctx, uint32_t id, const void *data,
                      uint32_t len) {
  //
  vstr_t buff; // u8 u32 str
  vstr_init(&buff, 0);

  vbuff_iaddc(&buff, SSH_FXP_DATA);
  vbuff_iaddu32(&buff, id);
  vbuff_iaddu32(&buff, len);
  vbuff_iadd(&buff, data, len);

  sftp_send_pkt(sock, ctx, &buff);

  vstr_clear(&buff);
}