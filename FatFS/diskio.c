/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h" /* Basic definitions of FatFs */

#include "diskio.h" /* Declarations FatFs MAI */

#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_sd.h"

extern SD_HandleTypeDef m_sdh;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

static int is_card_present() { return 1; }

DSTATUS disk_status(BYTE pdrv) {
  if (pdrv != 0)
    return STA_NOINIT;
  if (!is_card_present())
    return STA_NODISK;
  if (m_sdh.State == HAL_SD_STATE_RESET)
    return STA_NOINIT;
  return 0;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS
disk_initialize(BYTE pdrv) {
  if (pdrv != 0)
    return STA_NOINIT;
  if (!is_card_present())
    return STA_NODISK;

  if (m_sdh.State == HAL_SD_STATE_RESET) {
    if (HAL_SD_Init(&m_sdh) != HAL_OK) {
      return STA_NOINIT;
    }
  }
  return 0;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv,  /* Physical drive nmuber to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
) {

  if (pdrv != 0 || count == 0)
    return RES_PARERR;
  if (!is_card_present())
    return RES_NOTRDY;

  if (HAL_SD_ReadBlocks(&m_sdh,                                          //
                        (void *)buff, (uint32_t)sector, (uint32_t)count, //
                        HAL_MAX_DELAY                                    //
                        ) != HAL_OK) {
    return RES_ERROR;
  }

  /* wait for transfer to complete */
  uint32_t tout = HAL_GetTick() + 5000;
  while (HAL_SD_GetCardState(&m_sdh) != HAL_SD_CARD_TRANSFER) {
    if (HAL_GetTick() > tout)
      return RES_ERROR;
  }
  return RES_OK;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   LBA_t sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
) {
  if (pdrv != 0 || count == 0)
    return RES_PARERR;
  if (!is_card_present())
    return RES_NOTRDY;

  if (HAL_SD_WriteBlocks(&m_sdh, //
                         (void *)buff, (uint32_t)sector,
                         (uint32_t)count, //
                         HAL_MAX_DELAY    //
                         ) != HAL_OK) {
    return RES_ERROR;
  }
  uint32_t tout = HAL_GetTick() + 5000;
  while (HAL_SD_GetCardState(&m_sdh) != HAL_SD_CARD_TRANSFER) {
    if (HAL_GetTick() > tout)
      return RES_ERROR;
  }
  return RES_OK;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
) {
  if (pdrv != 0)
    return RES_PARERR;
  if (!is_card_present())
    return RES_NOTRDY;

  DRESULT res = RES_ERROR;
  HAL_SD_CardInfoTypeDef CardInfo;

  switch (cmd) {
  case CTRL_SYNC:
    if (HAL_SD_GetCardState(&m_sdh) == HAL_SD_CARD_TRANSFER)
      res = RES_OK;
    break;
  case GET_SECTOR_COUNT:
    if (HAL_SD_GetCardInfo(&m_sdh, &CardInfo) == HAL_OK) {
      *(DWORD *)buff = CardInfo.LogBlockNbr;
      res = RES_OK;
    }
    break;
  case GET_SECTOR_SIZE:
    if (HAL_SD_GetCardInfo(&m_sdh, &CardInfo) == HAL_OK) {
      *(WORD *)buff = CardInfo.LogBlockSize;
      res = RES_OK;
    }
    break;
  case GET_BLOCK_SIZE:
    if (HAL_SD_GetCardInfo(&m_sdh, &CardInfo) == HAL_OK) {
      *(DWORD *)buff = 1; /* or CardInfo.LogBlockSize / 512 if available */
      res = RES_OK;
    }
    break;
  default:
    res = RES_PARERR;
  }
  return res;
}
