/*
 * @Author: Alan Yin
 * @Date: 2024-10-29 23:19:19
 * @LastEditTime: 2024-11-01 20:36:10
 * @LastEditors: Alan Yin
 * @FilePath: /ganesha-sim/nfs-ganesha-5.7/src/FSAL/FSAL_SIM/main.c
 * @Description:
 * @// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
 * @// vim: ts=8 sw=2 smarttab
 * @Copyright (c) 2024 by Alan Yin, All Rights Reserved.
 */

#include <stdlib.h>
#include <errno.h>

// #include "fsal.h"
#include "fsal_types.h"
#include "fsal_api.h"
#include "FSAL/fsal_init.h"
#include "FSAL/fsal_commonlib.h"
#include "nfs_exports.h"

#include "utils.h"
#include "internal.h"
#include "fs.h"

static const char *module_name = "SIM";
int FSAL_ID_SIM = 12;

static pthread_mutex_t init_mtx;

/**
 * SIM global module object.
 */
struct sim_fsal_module SIMFSM = {
	.fsal = {
		.fs_info = {
			.maxfilesize = INT64_MAX,
			.maxlink = _POSIX_LINK_MAX,
			.maxnamelen = 1024,
			.maxpathlen = 1024,
			.no_trunc = true,
			.chown_restricted = false,
			.case_insensitive = false,
			.case_preserving = true,
			.link_support = false,
			.symlink_support = false,
			.lock_support = false,
			.lock_support_async_block = false,
			.named_attr = true,
			.unique_handles = true,
			.acl_support = 0,
			.cansettime = true,
			.homogenous = true,
			.supported_attrs = ATTRS_POSIX,
			.maxread = FSAL_MAXIOSIZE,
			.maxwrite = FSAL_MAXIOSIZE,
			.umask = 0,
			.rename_changes_key = true,
			.whence_is_name = true,
			.expire_time_parent = -1,
		}
	}
};

static struct config_item sim_items[] = {
	// CONF_ITEM_PATH("ceph_conf", 1, MAXPATHLEN, NULL,
	// 	sim_fsal_module, conf_path),
	// CONF_ITEM_STR("name", 1, MAXPATHLEN, NULL,
	// 	sim_fsal_module, name),
	// CONF_ITEM_STR("cluster", 1, MAXPATHLEN, NULL,
	// 	sim_fsal_module, cluster),
	// CONF_ITEM_STR("init_args", 1, MAXPATHLEN, NULL,
	// 	sim_fsal_module, init_args),
	CONF_ITEM_MODE("umask", 0,
			sim_fsal_module, fsal.fs_info.umask),
	CONFIG_EOL
};

struct config_block sim_block = {
	.dbus_interface_name = "org.ganesha.nfsd.config.fsal.sim",
	.blk_desc.name = "SIM",
	.blk_desc.type = CONFIG_BLOCK,
	.blk_desc.flags = CONFIG_UNIQUE,  /* too risky to have more */
	.blk_desc.u.blk.init = noop_conf_init,
	.blk_desc.u.blk.params = sim_items,
	.blk_desc.u.blk.commit = noop_conf_commit
};

static struct config_item export_params[] = {
	CONF_ITEM_NOOP("name"),
	CONF_MAND_STR("sim_basedir", 0, MAXDIRLEN, NULL,
		      sim_fsal_export, sim_basedir),
	CONF_MAND_STR("sim_id", 0, MAXKEYLEN, NULL,
		      sim_fsal_export, sim_id),
	CONFIG_EOL
};

static struct config_block export_param_block = {
	.dbus_interface_name = "org.ganesha.nfsd.config.fsal.sim-export%d",
	.blk_desc.name = "FSAL",
	.blk_desc.type = CONFIG_BLOCK,
	.blk_desc.u.blk.init = noop_conf_init,
	.blk_desc.u.blk.params = export_params,
	.blk_desc.u.blk.commit = noop_conf_commit
};

/**
 * @brief Initialize the configuration
 *
 * Given the root of the Ganesha configuration structure, initialize
 * the FSAL parameters.
 *
 * @param[in] module_in     The FSAL module
 * @param[in] config_struct Parsed ganesha configuration file
 * @param[out] err_type     config error processing state
 *
 * @return FSAL status.
 */
static fsal_status_t init_config(struct fsal_module *module_in,
				 config_file_t config_struct,
				 struct config_error_type *err_type)
{
	pr_entry();

	struct sim_fsal_module *myself =
		container_of(module_in, struct sim_fsal_module, fsal);

	pr_dbg("SIM module setup.");

	(void)load_config_from_parse(config_struct,
				     &sim_block,
				     myself,
				     true,
				     err_type);

	if (!config_error_is_harmless(err_type))
		return fsalstat(ERR_FSAL_INVAL, 0);

	display_fsinfo(&myself->fsal);

	return fsalstat(ERR_FSAL_NO_ERROR, 0);
}

/**
 * @brief Create a new export
 *
 * This function creates a new export in the FSAL using the supplied
 * path and options.  The function is expected to allocate its own
 * export (the full, private structure).  It must then initialize the
 * public portion like so:
 *
 * @code{.c}
 *         fsal_export_init(&private_export_handle->pub);
 * @endcode
 *
 * After doing other private initialization, it must attach the export
 * to the module, like so:
 *
 *
 * @code{.c}
 *         fsal_attach_export(fsal_hdl,
 *                            &private_export->pub.exports);
 *
 * @endcode
 *
 * And create the parent link with:
 *
 * @code{.c}
 * private_export->pub.fsal = fsal_hdl;
 * @endcode
 *
 * @note This seems like something that fsal_attach_export should
 * do. -- ACE.
 *
 * @param[in]     module_in   FSAL module
 * @param[in]     parse_node  opaque pointer to parse tree node for
 *                            export options to be passed to
 *                            load_config_from_node
 * @param[out]    err_type    config processing error reporting
 * @param[in]     up_ops      Upcall ops
 *
 * @return FSAL status.
 */
static fsal_status_t create_export(struct fsal_module *module_in,
				   void *parse_node,
				   struct config_error_type *err_type,
				   const struct fsal_up_vector *up_ops)
{
	pr_entry();

	/**
	 * 对于 .conf 文件中的每一个导出，都会自动执行 create_export 函数, 注册
	 * export 函数集, 对所有导出都执行完 create_export 函数后, 调用流程如下:
	 * (1) 调用 lookup_path 回调函数;
	 * (2) 调用 release 回调函数;
	*/
	fsal_status_t status = { ERR_FSAL_NO_ERROR, 0 };
	struct sim_fsal_export *myself = NULL;
	struct sim_fsal_handle *handle = NULL;
	struct stat st;
	int rc = 0;

	myself = gsh_calloc(1, sizeof(struct sim_fsal_export));
	fsal_export_init(&myself->export);
	sim_export_ops_init(&myself->export.exp_ops);

	/* get params for this export, if any */
	if (parse_node) {
		rc = load_config_from_node(parse_node,
					   &export_param_block,
					   myself,
					   true,
					   err_type);
		if (rc != 0) {
			pr_err("load_config_from_node error");
			status = posix2fsal_status(EINVAL);
			goto err_free;
		}
	}

	/**
	 * 解析完配置文件
	*/
	pr_dbg("sim_id:%s, sim_basedir:%s",
		myself->sim_id,
		myself->sim_basedir);

	rc = fsal_attach_export(module_in, &myself->export.exports);
	if (rc != 0) {
		pr_err("Unable to attach export for %s.", CTX_FULLPATH(op_ctx));
		status = posix2fsal_status(rc);
		goto err_free;
	}

	myself->export.fsal = module_in;
	myself->export.up_ops = up_ops;

	rc = stat(myself->sim_basedir, &st);
	if (rc != 0) {
		pr_err("stat %s error (%d:%s)", myself->sim_basedir,
			errno, strerror(errno));
		status = posix2fsal_status(rc);
		goto err_cleanup;
	}

	/* Save the export path. */
	myself->export_path = gsh_strdup(CTX_FULLPATH(op_ctx));
	op_ctx->fsal_export = &myself->export;

	/**
	 * print path in sim.conf
	 * Path = /local_mnt;
	 * */
	pr_info("SIM module export %s.", myself->export_path);

	// rc = sim_getattr(myself->sim_fs, myself->sim_fs->root_fh, &st,
	// 		 SIM_GETATTR_FLAG_NONE);
	// if (rc < 0) {
	// 	glist_del(&myself->export.exports);
	// 	status = sim2fsal_error(rc);
	// 	goto err_free;
	// }

	// rc = sim_construct_handle(myself, myself->sim_fs->root_fh, &st, &handle);
	// if (rc < 0) {
	// 	status = sim2fsal_error(rc);
	// 	goto error;
	// }

	myself->root = handle;

	pr_info("dddddddddd");

	return status;

err_cleanup:
	fsal_detach_export(module_in, &myself->export.exports);
err_free:
	free_export_ops(&myself->export);
	gsh_free(myself);

	pr_err("SIM module create export failed %d", status.major);

	return status;
}

MODULE_INIT void init(void)
{
	int rc = 0;
	struct fsal_module *myself = &SIMFSM.fsal;

	PTHREAD_MUTEX_init(&init_mtx, NULL);

	pr_info("SIM module registering.");

	/**
	 * nfs-ganesha-5.7/src/include/fsal_pnfs.h 文件中定义了 FSAL_ID
	 * FSAL_ID_RGW
	 * */
	rc = register_fsal(myself, module_name, FSAL_MAJOR_VERSION,
			   FSAL_MINOR_VERSION, FSAL_ID_SIM);
	if (rc != 0) {
		/* The register_fsal function prints its own log
		   message if it fails */
		pr_err("SIM module failed to register.");
	}

	/** Set up module operations
	 * 先执行 sim_handle_ops_init 函数,
	 * 接着执行 init_config 函数
	*/
	myself->m_ops.init_config = init_config;
	myself->m_ops.create_export = create_export;

	/* Initialize the fsal_obj_handle ops for FSAL SIM */
	sim_handle_ops_init(&SIMFSM.handle_ops);
}

MODULE_FINI void finish(void)
{
	int rc;

	pr_info("RGW module finishing.");

	rc = unregister_fsal(&SIMFSM.fsal);
	if (rc != 0) {
		pr_err("SIM: unregister_fsal failed (%d)", rc);
	}

	PTHREAD_MUTEX_destroy(&init_mtx);
}