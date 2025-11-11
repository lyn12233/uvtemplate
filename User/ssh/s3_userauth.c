#include "s3_userauth.h"
#include "packet.h"
#include "types/vo.h"

#include "log.h"

void respond_auth_service(int sock, ssh_context *ctx, int *done) {
  vstr_t *recvd = recv_packet_enc(sock, ctx);
  puts("expect ssh_msg_service_request:");
  vbuff_dump(recvd);
  *done = 0;
}

void respond_userauth(int sock, ssh_context *ctx, int *done, int *half);