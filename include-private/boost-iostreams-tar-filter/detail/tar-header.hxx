#pragma once

/**
 * @struct TarHeader
 * @brief Representation of a POSIX/USTAR TAR header block (512 bytes).
 *
 * The struct layout matches the on-disk TAR header format. Fields are
 * fixed-size character arrays and may be NUL-terminated or filled according to
 * the TAR spec.
 *
 * Note: The struct is packed to guarantee the exact 512-byte layout.
 */
struct __attribute__((packed)) TarHeader {
  char name[100];     /**< @brief File name (may be NUL-terminated). */
  char mode[8];       /**< @brief File mode (octal ASCII). */
  char uid[8];        /**< @brief Owner user ID (octal ASCII). */
  char gid[8];        /**< @brief Owner group ID (octal ASCII). */
  char size[12];      /**< @brief File size (octal ASCII or base-256 binary). */
  char mtime[12];     /**< @brief Modification time (octal ASCII). */
  char chksum[8];     /**< @brief Header checksum field (octal ASCII). */
  char typeflag[1];   /**< @brief Type flag ('0' regular file, '5' directory,
                         etc.). */
  char linkname[100]; /**< @brief Name of linked file for symlinks. */
  char magic[6];      /**< @brief UStar magic ("ustar\0"). */
  char version[2];    /**< @brief UStar version ("00"). */
  char uname[32];     /**< @brief Owner user name. */
  char gname[32];     /**< @brief Owner group name. */
  char devmajor[8];   /**< @brief Device major number for special files. */
  char devminor[8];   /**< @brief Device minor number for special files. */
  char prefix[155];   /**< @brief Prefix for long file names. */
  char padding[12];   /**< @brief Padding to make the header 512 bytes. */
};

static_assert(sizeof(TarHeader) == 512, "TarHeader must be 512 bytes");