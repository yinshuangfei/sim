/*
 * @Author: Alan Yin
 * @Date: 2024-11-06 20:04:29
 * @LastEditTime: 2024-12-17 21:34:11
 * @LastEditors: Alan Yin
 * @FilePath: /ganesha-sim/nfs-ganesha-5.7/src/FSAL/FSAL_SIM/fs.c
 * @Description:
 * @// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
 * @// vim: ts=8 sw=2 smarttab
 * @Copyright (c) 2024 by Alan Yin, All Rights Reserved.
 */
#include "fs.h"
#include "utils.h"

int sim_getattr(struct sim_fs *rgw_fs, struct sim_file_handle *fh,
		struct stat *st, uint32_t flags)
{
	pr_entry();

	int rc = 0;
	return rc;
}