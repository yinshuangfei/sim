/*
 * @Author: Alan Yin
 * @Date: 2024-11-01 21:10:29
 * @LastEditTime: 2024-11-01 21:10:40
 * @LastEditors: Alan Yin
 * @FilePath: /ganesha-sim/nfs-ganesha-5.7/src/FSAL/FSAL_SIM/export.c
 * @Description:
 * @// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
 * @// vim: ts=8 sw=2 smarttab
 * @Copyright (c) 2024 by Alan Yin, All Rights Reserved.
 */

#include "fsal_api.h"
#include "FSAL/fsal_commonlib.h"
// #include "os/linux/fsal_handle_syscalls.h"

#include "internal.h"
#include "utils.h"

/**
 * @brief Finalize an export
 *
 * This function is called as part of cleanup when the last reference to
 * an export is released and it is no longer part of the list.  It
 * should clean up all private resources and destroy the object.
 *
 * @param[in] exp_hdl The export to release.
 */
static void release(struct fsal_export *export_pub)
{
	/** 该函数在初始化导出的时候被框架调用, 调用栈如下：
	 *
	 *     main()->
	 *     nfs_start()->
	 *     nfs_Init()->
	 *     exports_pkginit()->
	 *     export_revert()->
	 *     release_op_context()->
	 *     clear_op_context_export_impl()->
	 *     _put_gsh_export()->
	 *     free_export_resources()->
	 *     mdcache_exp_release()->
	 *     sub_export->exp_ops.release()
	 */

	pr_entry();

	struct sim_fsal_export *export =
		container_of(export_pub, struct sim_fsal_export, export);

	pr_info("path: %s", export->export_path);
}

/**
 * @brief Look up a path
 *
 * This function looks up a path within the export, it is now exclusively
 * used to get a handle for the root directory of the export.
 *
 * NOTE: This method will eventually be replaced by a method that simply
 *       requests the root obj_handle be instantiated. The single caller
 *       doesn't request attributes (nor did the two callers that were removed
 *       in favor of calling fsal_lookup_path).
 *
 * The caller will set the request_mask in attrs_out to indicate the attributes
 * of interest. ATTR_ACL SHOULD NOT be requested and need not be provided. If
 * not all the requested attributes can be provided, this method MUST return
 * an error unless the ATTR_RDATTR_ERR bit was set in the request_mask.
 *
 * Since this method instantiates a new fsal_obj_handle, it will be forced
 * to fetch at least some attributes in order to even know what the object
 * type is (as well as it's fileid and fsid). For this reason, the operation
 * as a whole can be expected to fail if the attributes were not able to be
 * fetched.
 *
 * @param[in]     exp_hdl   The export in which to look up
 * @param[in]     path      The path to look up
 * @param[out]    handle    The object found
 * @param[in,out] attrs_out Optional attributes for newly created object
 *
 * @note On success, @a handle has been ref'd
 *
 * @return FSAL status.
 */
static fsal_status_t lookup_path(struct fsal_export *export_pub,
				 const char *path,
				 struct fsal_obj_handle **pub_handle,
				 struct fsal_attrlist *attrs_out)
{
	/** 该函数在初始化导出的时候被框架调用, 调用栈如下：
	 *
	 *     main()->
	 *     nfs_start()->
	 *     nfs_Init()->
	 *     exports_pkginit()->
	 *     foreach_gsh_export()->
	 *     init_export_cb()->
	 *     init_export_root()->
	 *     export->fsal_export->exp_ops.lookup_path()
	 */

	pr_entry();

	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	struct sim_fsal_handle *handle = NULL;
	struct stat st;
	struct sim_file_handle *sim_fh = NULL;
	struct sim_fsal_export *export =
		container_of(export_pub, struct sim_fsal_export, export);
	// int rc = 0;

	pr_dbg("<export> path:%s", path);

	// name_to_handle_at(parent_fd, path, handle, &mount_id, flags);
	// rc = name_to_handle_at(parent_fd, path, handle, &mount_id, lookup->flags);
	// if(0 != rslt){
	// 	__show_fd_path(parent_fd, "parent_fd");
	// 	xprintf("<s><failed><looup> (1.1) <path=%s><%s>(err:%d):%s\n", lookup->path, path, errno, strerror(errno));
	// 	rslt = errno;
	// 	goto DONE;
	// }

	*pub_handle = NULL;

	if (strcmp(path, "/") && !strcmp((path + strlen(path) - 1), "/")) {
		status.major = ERR_FSAL_INVAL;
		return status;
	}

	if (export->sim_fs) {
		sim_fh = export->sim_fs->root_fh;
	} else {
		pr_err("<export> export->fs is NULL, abort");
		status.major = ERR_FSAL_INVAL;
		return status;
	}

	(void)sim_construct_handle(export, sim_fh, &st, &handle);

	*pub_handle = &handle->handle;

	if (attrs_out != NULL) {
		posix2fsal_attributes_all(&st, attrs_out);
	}

	return status;
}

/**
 * @brief Convert a wire handle to a host handle
 *
 * This function extracts a host handle from a wire handle.  That
 * is, when given a handle as passed to a client, this method will
 * extract the handle to create objects.
 *
 * @param[in]     exp_hdl Export handle
 * @param[in]     in_type Protocol through which buffer was received.
 * @param[in,out] fh_desc Buffer descriptor.  The address of the
 *                        buffer is given in @c fh_desc->buf and must
 *                        not be changed.  @c fh_desc->len is the
 *                        length of the data contained in the buffer,
 *                        @c fh_desc->len must be updated to the correct
 *                        host handle size.
 * @param[in]     flags   Flags to describe the wire handle. Example, if
 *                        the handle is a big endian handle.
 *
 * @return FSAL type.
 */
static fsal_status_t wire_to_host(struct fsal_export *exp_hdl,
				  fsal_digesttype_t in_type,
				  struct gsh_buffdesc *fh_desc,
				  int flags)
{
	pr_entry();

	switch (in_type) {
		/* Digested Handles */
	case FSAL_DIGEST_NFSV3:
	case FSAL_DIGEST_NFSV4:
		/* wire handles */
		fh_desc->len = sizeof(struct sim_fh_hk);
		break;
	default:
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/**
 * @brief Create a FSAL object handle from a host handle
 *
 * This function creates a FSAL object handle from a host handle
 * (when an object is no longer in cache but the client still remembers
 * the handle).
 *
 * The caller will set the request_mask in attrs_out to indicate the attributes
 * of interest. ATTR_ACL SHOULD NOT be requested and need not be provided. If
 * not all the requested attributes can be provided, this method MUST return
 * an error unless the ATTR_RDATTR_ERR bit was set in the request_mask.
 *
 * Since this method instantiates a new fsal_obj_handle, it will be forced
 * to fetch at least some attributes in order to even know what the object
 * type is (as well as it's fileid and fsid). For this reason, the operation
 * as a whole can be expected to fail if the attributes were not able to be
 * fetched.
 *
 * @param[in]     exp_hdl   The export in which to create the handle
 * @param[in]     hdl_desc  Buffer descriptor for the host handle
 * @param[out]    handle    FSAL object handle
 * @param[in,out] attrs_out Optional attributes for newly created object
 *
 * @note On success, @a handle has been ref'd
 *
 * @return FSAL status.
 */
static fsal_status_t create_handle(struct fsal_export *export_pub,
				   struct gsh_buffdesc *desc,
				   struct fsal_obj_handle **pub_handle,
				   struct fsal_attrlist *attrs_out)
{
	pr_entry();

	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	return status;
}

/**
 * @brief Get filesystem statistics
 *
 * This function gets information on inodes and space in use and free
 * for a filesystem.  See @c fsal_dynamicinfo_t for details of what to
 * fill out.
 *
 * @param[in]  exp_hdl Export handle to interrogate
 * @param[in]  obj_hdl Directory
 * @param[out] info    Buffer to fill with information
 *
 * @retval FSAL status.
 */
static fsal_status_t get_fs_dynamic_info(struct fsal_export *export_pub,
					 struct fsal_obj_handle *obj_hdl,
					 fsal_dynamicfsinfo_t *info)
{
	pr_entry();

	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	return status;
}

struct state_t *rgw_alloc_state(struct fsal_export *exp_hdl,
				enum state_type state_type,
				struct state_t *related_state)
{
	pr_entry();

	return init_state(gsh_calloc(1, sizeof(struct sim_open_state)),
			  NULL, state_type, related_state);
}

/**
 * @brief Set operations for exports
 *
 * This function overrides operations that we've implemented, leaving
 * the rest for the default.
 *
 * @param[in,out] ops Operations vector
 */
void sim_export_ops_init(struct export_ops *ops)
{
	ops->release = release;
	ops->lookup_path = lookup_path;
	ops->wire_to_host = wire_to_host;
	ops->create_handle = create_handle;
	ops->get_fs_dynamic_info = get_fs_dynamic_info;
	ops->alloc_state = rgw_alloc_state;

	// ops->release = release;
	// ops->lookup_path = vfs_lookup_path;
	// ops->wire_to_host = wire_to_host;
	// ops->create_handle = vfs_create_handle;
	// ops->get_fs_dynamic_info = get_dynamic_info;
	// ops->get_quota = get_quota;
	// ops->set_quota = set_quota;
	// ops->alloc_state = vfs_alloc_state;
	// ops->get_fsal_obj_hdl = get_fsal_obj_hdl;
}