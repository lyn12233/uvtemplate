///@file port_errno.h
#pragma once

enum {
  _err_again = 1,
  _err_badf,
  _err_inval,
  _err_io,
  _err_notconn,
  _err_notsock,
  _err_unreachable,
};

#define EAGAIN (-(int)_err_again)
#define EBADF (-(int)_err_badf)
#define EINVAL (-(int)_err_inval)
#define EIO (-(int)_err_io)
#define ENOTCONN (-(int)_err_notconn)
#define ENOTSOCK (-(int)_err_notsock)
#define EUNREACHABLE (-(int)_err_unreachable)
