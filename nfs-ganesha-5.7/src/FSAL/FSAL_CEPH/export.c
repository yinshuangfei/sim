// SPDX-License-Identifier: LGPL-3.0-or-later
/*
 * Copyright © 2012-2014, CohortFS, LLC.
 * Author: Adam C. Emerson <aemerson@linuxbox.com>
 *
 * contributeur : William Allen Simpson <bill@cohortfs.com>
 *		  Marcus Watts <mdw@cohortfs.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 * -------------
 */

/**
 * @file FSAL_CEPH/export.c
 * @author Adam C. Emerson <aemerson@linuxbox.com>
 * @author William Allen Simpson <bill@cohortfs.com>
 * @date Wed Oct 22 13:24:33 2014
 *
 * @brief Implementation of FSAL export functions for Ceph
 *
 * This file implements the Ceph specific functionality for the FSAL
 * export handle.
 */

#include <limits.h>
#include <stdint.h>
#include <sys/statvfs.h>
#include <cephfs/libcephfs.h>
#include "abstract_mem.h"
#include "fsal.h"
#include "fsal_types.h"
#include "fsal_api.h"
#include "FSAL/fsal_commonlib.h"
#include "FSAL/fsal_config.h"
#include "internal.h"
#include "statx_compat.h"
#include "sal_functions.h"
#include "nfs_core.h"
#ifdef CEPHFS_POSIX_ACL
#include "nfs_exports.h"
#endif				/* CEPHFS_POSIX_ACL */
#ifdef USE_LTTNG
#include "gsh_lttng/fsal_ceph.h"
#endif

/**
 * @brief Clean up an export
 *
 * This function cleans up an export after the last reference is
 * released.
 *
 * @param[in,out] export The export to be released
 */

static void release(struct fsal_export *export_pub)
{
	/* The private, expanded export */
	struct ceph_export *export =
			container_of(export_pub, struct ceph_export, export);
	/* Ceph mount */
	struct ceph_mount *cm = export->cm;

	deconstruct_handle(export->root);
	export->root = 0;

	fsal_detach_export(export->export.fsal, &export->export.exports);
	free_export_ops(&export->export);

	PTHREAD_RWLOCK_wrlock(&cmount_lock);

	/* Detach this export from the ceph_mount */
	glist_del(&export->cm_list);

	if (--cm->cm_refcnt == 0) {
		/* This was the final reference */

		ceph_shutdown(cm->cmount);

		ceph_mount_remove(&cm->cm_avl_mount);

		gsh_free(cm->cm_fs_name);
		gsh_free(cm->cm_mount_path);
		gsh_free(cm->cm_user_id);
		gsh_free(cm->cm_secret_key);


		gsh_free(cm);
	} else if (cm->cm_export == export) {
		/* Need to attach a different export for upcalls */
		cm->cm_export = glist_first_entry(&cm->cm_exports,
						  struct ceph_export,
						  cm_list);
	}

	export->cmount = NULL;

	PTHREAD_RWLOCK_unlock(&cmount_lock);

	gsh_free(export);
	export = NULL;
}

/**
 * @brief Return a handle corresponding to a path
 *
 * This function looks up the given path and supplies an FSAL object
 * handle.  Because the root path specified for the export is a Ceph
 * style root as supplied to mount -t ceph of ceph-fuse (of the form
 * host:/path), we check to see if the path begins with / and, if not,
 * skip until we find one.
 *
 * @param[in]  export_pub The export in which to look up the file
 * @param[in]  path       The path to look up
 * @param[out] pub_handle The created public FSAL handle
 *
 * @return FSAL status.
 */

static fsal_status_t lookup_path(struct fsal_export *export_pub,
				 const char *path,
				 struct fsal_obj_handle **pub_handle,
				 struct fsal_attrlist *attrs_out)
{
	/* The 'private' full export handle */
	struct ceph_export *export =
			container_of(export_pub, struct ceph_export, export);
	/* The 'private' full object handle */
	struct ceph_handle *handle = NULL;
	/* Inode pointer */
	struct Inode *i = NULL;
	/* FSAL status structure */
	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	/* The buffer in which to store stat info */
	struct ceph_statx stx;
	/* Return code from Ceph */
	int rc, lmp;
	/* Find the actual path in the supplied path */
	const char *realpath;

	if (*path != '/') {
		realpath = strchr(path, ':');
		if (realpath == NULL) {
			status.major = ERR_FSAL_INVAL;
			return status;
		}
		if (*(++realpath) != '/') {
			status.major = ERR_FSAL_INVAL;
			return status;
		}
	} else {
		realpath = path;
	}

	*pub_handle = NULL;

	/*
	 * sanity check: ensure that this is the right export. realpath
	 * must be a superset of the export fullpath, or the string
	 * handling will be broken.
	 */
	if (strstr(realpath, CTX_FULLPATH(op_ctx)) != realpath) {
		status.major = ERR_FSAL_SERVERFAULT;
		return status;
	}

	PTHREAD_RWLOCK_rdlock(&cmount_lock);

	lmp = strlen(export->cm->cm_mount_path);

	if (lmp == 1) {
		/* If cmount_path is "/" we need the leading '/'. */
		lmp = 0;
	}

	/* Advance past the export's fullpath */
	realpath += lmp;

	PTHREAD_RWLOCK_unlock(&cmount_lock);

	/* special case the root */
	if (realpath[1] == '\0' || realpath[0] == '\0') {
		assert(export->root);
		*pub_handle = &export->root->handle;
		return status;
	}

	rc = fsal_ceph_ll_walk(export->cmount, realpath, &i, &stx,
				!!attrs_out, &op_ctx->creds);

	if (rc < 0)
		return ceph2fsal_error(rc);

	construct_handle(&stx, i, export, &handle);

	if (attrs_out != NULL)
		ceph2fsal_attributes(&stx, attrs_out);

	*pub_handle = &handle->handle;
#ifdef USE_LTTNG
	tracepoint(fsalceph, ceph_create_handle, __func__, __LINE__,
		   &handle->handle);
#endif
	return status;
}

/**
 * @brief Decode a digested handle
 *
 * This function decodes a previously digested handle.
 *
 * @param[in]  exp_handle  Handle of the relevant fs export
 * @param[in]  in_type  The type of digest being decoded
 * @param[out] fh_desc  Address and length of key
 */
static fsal_status_t wire_to_host(struct fsal_export *exp_hdl,
				  fsal_digesttype_t in_type,
				  struct gsh_buffdesc *fh_desc,
				  int flags)
{
	struct ceph_host_handle *hhdl = fh_desc->addr;

	switch (in_type) {
		/* Digested Handles */
	case FSAL_DIGEST_NFSV3:
	case FSAL_DIGEST_NFSV4:
		/*
		 * Ganesha automatically mixes the export_id in with the
		 * filehandle and strips that out before calling this
		 * function.
		 *
		 * Note that we use a LE values in the filehandle. Earlier
		 * versions treated those values as opaque, so this allows us to
		 * maintain compatibility with legacy deployments (most of which
		 * were on LE arch).
		 */
		hhdl->chk_ino = le64toh(hhdl->chk_ino);
		hhdl->chk_snap = le64toh(hhdl->chk_snap);
		hhdl->chk_fscid = le64toh(hhdl->chk_fscid);
		fh_desc->len = sizeof(*hhdl);
		break;
	default:
		return fsalstat(ERR_FSAL_SERVERFAULT, 0);
	}

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/**
 * @brief extract "key" from a host handle
 *
 * This function extracts a "key" from a host handle.  That is, when
 * given a handle that is extracted from wire_to_host() above, this
 * method will extract the unique bits used to index the inode cache.
 *
 * @param[in]     exp_hdl Export handle
 * @param[in,out] fh_desc Buffer descriptor.  The address of the
 *                        buffer is given in @c fh_desc->buf and must
 *                        not be changed.  @c fh_desc->len is the length
 *                        of the data contained in the buffer, @c
 *                        fh_desc->len must be updated to the correct
 *                        size. In other words, the key has to be placed
 *                        at the beginning of the buffer!
 */
fsal_status_t host_to_key(struct fsal_export *exp_hdl,
			  struct gsh_buffdesc *fh_desc)
{
	struct ceph_handle_key *key = fh_desc->addr;

	/*
	 * Ganesha automatically mixes the export_id in with the actual wire
	 * filehandle and strips that out before transforming it to a host
	 * handle. This method is called on a host handle which doesn't have
	 * the export_id.
	 *
	 * Most FSALs don't factor in the export_id with the handle_key,
	 * but we want to do that for FSAL_CEPH, primarily because we
	 * want to do accesses via different exports via different cephx
	 * creds. Mix the export_id back in here.
	 */
	key->export_id = op_ctx->ctx_export->export_id;
	fh_desc->len = sizeof(*key);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

#ifndef USE_FSAL_CEPH_LOOKUP_VINO
static int ceph_ll_lookup_vino(struct ceph_mount_info *cmount, vinodeno_t vino,
			       Inode **inode)
{
	/* Check the cache first */
	*inode = ceph_ll_get_inode(cmount, vino);
	if (*inode)
		return 0;

	/*
	 * We can't look up snap inodes w/o ceph_ll_lookup_vino. Just
	 * return -ESTALE if we get one that's not already in cache.
	 */
	if (vino.snapid.val != CEPH_NOSNAP)
		return -ESTALE;

	return ceph_ll_lookup_inode(cmount, vino.ino, inode);
}
#endif /* USE_FSAL_CEPH_LOOKUP_VINO */

/**
 * @brief Create a handle object from a wire handle
 *
 * The wire handle is given in a buffer outlined by desc, which it
 * looks like we shouldn't modify.
 *
 * @param[in]  export_pub Public export
 * @param[in]  desc       Handle buffer descriptor (host handle)
 * @param[out] pub_handle The created handle
 *
 * @return FSAL status.
 */
static fsal_status_t create_handle(struct fsal_export *export_pub,
				   struct gsh_buffdesc *desc,
				   struct fsal_obj_handle **pub_handle,
				   struct fsal_attrlist *attrs_out)
{
	/* Full 'private' export structure */
	struct ceph_export *export =
			container_of(export_pub, struct ceph_export, export);
	/* FSAL status to return */
	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	/* The FSAL specific portion of the handle received by the client */
	struct ceph_host_handle *hhdl = desc->addr;
	/* Ceph return code */
	int rc = 0;
	/* Stat buffer */
	struct ceph_statx stx;
	/* Handle to be created */
	struct ceph_handle *handle = NULL;
	/* Inode pointer */
	struct Inode *i;
	vinodeno_t vi;

	*pub_handle = NULL;

	if (desc->len != sizeof(*hhdl)) {
		status.major = ERR_FSAL_INVAL;
		return status;
	}

	vi.ino.val = hhdl->chk_ino;
	vi.snapid.val = hhdl->chk_snap;

	rc = ceph_ll_lookup_vino(export->cmount, vi, &i);
	if (rc)
		return ceph2fsal_error(rc);

	rc = fsal_ceph_ll_getattr(export->cmount, i, &stx,
		attrs_out ? CEPH_STATX_ATTR_MASK : CEPH_STATX_HANDLE_MASK,
		&op_ctx->creds);
	if (rc < 0)
		return ceph2fsal_error(rc);

	construct_handle(&stx, i, export, &handle);

	if (attrs_out != NULL)
		ceph2fsal_attributes(&stx, attrs_out);

	*pub_handle = &handle->handle;
#ifdef USE_LTTNG
	tracepoint(fsalceph, ceph_create_handle, __func__, __LINE__,
		   &handle->handle);
#endif
	return status;
}

/**
 * @brief Get dynamic filesystem info
 *
 * This function returns dynamic filesystem information for the given
 * export.
 *
 * @param[in]  export_pub The public export handle
 * @param[out] info       The dynamic FS information
 *
 * @return FSAL status.
 */

static fsal_status_t get_fs_dynamic_info(struct fsal_export *export_pub,
					 struct fsal_obj_handle *obj_hdl,
					 fsal_dynamicfsinfo_t *info)
{
	/* Full 'private' export */
	struct ceph_export *export =
			container_of(export_pub, struct ceph_export, export);
	/* Return value from Ceph calls */
	int rc = 0;
	/* Filesystem stat */
	struct statvfs vfs_st;

	rc = ceph_ll_statfs(export->cmount, export->root->i, &vfs_st);

	if (rc < 0)
		return ceph2fsal_error(rc);

	memset(info, 0, sizeof(fsal_dynamicfsinfo_t));
	info->total_bytes = vfs_st.f_frsize * vfs_st.f_blocks;
	info->free_bytes = vfs_st.f_frsize * vfs_st.f_bfree;
	info->avail_bytes = vfs_st.f_frsize * vfs_st.f_bavail;
	info->total_files = vfs_st.f_files;
	info->free_files = vfs_st.f_ffree;
	info->avail_files = vfs_st.f_favail;
	info->time_delta.tv_sec = 0;
	info->time_delta.tv_nsec = FSAL_DEFAULT_TIME_DELTA_NSEC;

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

#ifdef CEPHFS_POSIX_ACL
static fsal_aclsupp_t fs_acl_support(struct fsal_export *exp_hdl)
{
	if (!op_ctx_export_has_option(EXPORT_OPTION_DISABLE_ACL))
		return fsal_acl_support(&exp_hdl->fsal->fs_info);
	else
		return 0;
}
#endif				/* CEPHFS_POSIX_ACL */

void ceph_prepare_unexport(struct fsal_export *export_pub)
{
	struct ceph_export *export =
			container_of(export_pub, struct ceph_export, export);

	/* Flush all buffers */
	ceph_sync_fs(export->cmount);

#if USE_FSAL_CEPH_ABORT_CONN
	/*
	 * If we're shutting down and are still a member of the cluster, do a
	 * hard abort on the connection to ensure that state is left intact on
	 * the MDS when we return. If we're not shutting down or aren't a
	 * member any longer then cleanly tear down the export.
	 */
	if (admin_shutdown && nfs_grace_is_member())
		ceph_abort_conn(export->cmount);
#endif
}


/**
 * @brief Function to get the fasl_obj_handle that has fsal_fd as its global fd.
 *
 * @param[in]     exp_hdl   The export in which the handle exists
 * @param[in]     fd        File descriptor in question
 * @param[out]    handle    FSAL object handle
 *
 * @return the fsal_obj_handle.
 */
void get_fsal_obj_hdl(struct fsal_export *exp_hdl,
				  struct fsal_fd *fd,
				  struct fsal_obj_handle **handle)
{
	struct ceph_fd *my_fd = NULL;
	struct ceph_handle *myself = NULL;

	my_fd = container_of(fd, struct ceph_fd, fsal_fd);
	myself = container_of(my_fd, struct ceph_handle, fd);

	*handle = &myself->handle;
}


/**
 * @brief Set operations for exports
 *
 * This function overrides operations that we've implemented, leaving
 * the rest for the default.
 *
 * @param[in,out] ops Operations vector
 */

void export_ops_init(struct export_ops *ops)
{
	ops->prepare_unexport = ceph_prepare_unexport;
	ops->release = release;
	ops->lookup_path = lookup_path;
	ops->host_to_key = host_to_key;
	ops->wire_to_host = wire_to_host;
	ops->create_handle = create_handle;
	ops->get_fs_dynamic_info = get_fs_dynamic_info;
	ops->alloc_state = ceph_alloc_state;
	ops->get_fsal_obj_hdl = get_fsal_obj_hdl;
#ifdef CEPHFS_POSIX_ACL
	ops->fs_acl_support = fs_acl_support;
#endif				/* CEPHFS_POSIX_ACL */
#ifdef CEPH_PNFS
	export_ops_pnfs(ops);
#endif				/* CEPH_PNFS */
}
