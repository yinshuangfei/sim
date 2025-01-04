/*
 * @Author: Alan Yin
 * @Date: 2024-11-06 20:04:42
 * @LastEditTime: 2024-12-17 22:27:18
 * @LastEditors: Alan Yin
 * @FilePath: /ganesha-sim/nfs-ganesha-5.7/src/FSAL/FSAL_SIM/fs.h
 * @Description:
 * @// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
 * @// vim: ts=8 sw=2 smarttab
 * @Copyright (c) 2024 by Alan Yin, All Rights Reserved.
 */

#ifndef SIM_FILESYSTEM_H
#define SIM_FILESYSTEM_H

#include <sys/stat.h>
#include <sys/types.h>
#include <stdint.h>
// #include <stdbool.h>

// #include "os/linux/fsal_handle_syscalls.h"
#include "internal.h"

#ifdef __cplusplus
extern "C" {
#endif

int sim_getattr(struct sim_fs *rgw_fs, struct sim_file_handle *fh,
		struct stat *st, uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif /** SIM_FILESYSTEM_H */