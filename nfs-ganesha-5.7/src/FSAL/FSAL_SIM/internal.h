/*
 * @Author: Alan Yin
 * @Date: 2024-10-31 19:51:27
 * @LastEditTime: 2024-11-06 19:42:14
 * @LastEditors: Alan Yin
 * @FilePath: /ganesha-sim/nfs-ganesha-5.7/src/FSAL/FSAL_SIM/internal.h
 * @Description:
 * @// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
 * @// vim: ts=8 sw=2 smarttab
 * @Copyright (c) 2024 by Alan Yin, All Rights Reserved.
 */

#ifndef FSAL_SIM_INTERNAL_INTERNAL
#define FSAL_SIM_INTERNAL_INTERNAL

#include "fsal_api.h"
#include "sal_data.h"		/** gsh_calloc */
#include "fsal_convert.h"

#define SIM_GETATTR_FLAG_NONE      0x0000

#define MAXKEYLEN 32
#define MAXDIRLEN 256

struct sim_fsal_module {
	struct fsal_module fsal;
	struct fsal_obj_ops handle_ops;
	char *conf_path;
	char *name;
	char *cluster;
	char *init_args;
};

extern struct sim_fsal_module SIMFSM;

enum sim_fh_type {
	SIM_FS_TYPE_NIL = 0,
	SIM_FS_TYPE_FILE,
	SIM_FS_TYPE_DIRECTORY,
	SIM_FS_TYPE_SYMBOLIC_LINK,
};

/* content-addressable hash key */
struct sim_fh_hk {
	uint64_t bucket;
	uint64_t object;
};

struct sim_file_handle {
	/* content-addressable hash */
	struct sim_fh_hk fh_hk;
	/* private data */
	void *fh_private;
	/* file type */
	enum sim_fh_type fh_type;
};

struct sim_fs {
	// librgw_t rgw;
	void *fs_private;
	struct sim_file_handle *root_fh;
};

/**
 * SIM internal export object
 */
struct sim_fsal_export {
	struct fsal_export export;	/*< The public export object */
	struct sim_fs *sim_fs;		/*< "Opaque" fs handle */
	struct sim_fsal_handle *root;	/*< root handle */
	char *export_path;		/** 导出路径 */
	char *sim_basedir;		/** 后端根目录路径 */
	char *sim_id;
};

struct sim_fsal_handle {
	struct fsal_obj_handle handle;	/*< The public handle */
	struct sim_file_handle *sim_fh;  /*< SIM-internal file handle */
	/* XXXX remove ptr to up-ops--we can always follow export! */
	const struct fsal_up_vector *up_ops;	/*< Upcall operations */
	struct sim_fsal_export *export;	/*< The first export this handle
					 *< belongs to */
	struct fsal_share share;
	fsal_openflags_t openflags;
};

/**
 * SIM "file descriptor"
 */
struct sim_open_state {
	struct state_t gsh_open;
	fsal_openflags_t openflags;
};

fsal_status_t sim2fsal_error(const int sim_errorcode);
int sim_construct_handle(struct sim_fsal_export *export,
			 struct sim_file_handle *sim_fh,
			 struct stat *st,
			 struct sim_fsal_handle **obj);

void sim_handle_ops_init(struct fsal_obj_ops *ops);
void sim_export_ops_init(struct export_ops *ops);

#endif /** FSAL_SIM_INTERNAL_INTERNAL */
