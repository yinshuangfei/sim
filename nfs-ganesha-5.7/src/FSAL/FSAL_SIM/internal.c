#include "fsal_convert.h"
#include "FSAL/fsal_commonlib.h"

#include "internal.h"

/**
 * @brief FSAL status from SIM error
 *
 * This function returns a fsal_status_t with the FSAL error as the
 * major, and the posix error as minor.	 (SIM's error codes are just
 * negative signed versions of POSIX error codes.)
 *
 * @param[in] sim_errorcode SIM error (negative Posix)
 *
 * @return FSAL status.
 */
fsal_status_t sim2fsal_error(const int sim_errorcode)
{
	fsal_status_t status;

	status.minor = -sim_errorcode;

	switch (-sim_errorcode) {

	case 0:
		status.major = ERR_FSAL_NO_ERROR;
		break;

	case EPERM:
		status.major = ERR_FSAL_PERM;
		break;

	case ENOENT:
		status.major = ERR_FSAL_NOENT;
		break;

	case ECONNREFUSED:
	case ECONNABORTED:
	case ECONNRESET:
	case EIO:
	case ENFILE:
	case EMFILE:
	case EPIPE:
		status.major = ERR_FSAL_IO;
		break;

	case ENODEV:
	case ENXIO:
		status.major = ERR_FSAL_NXIO;
		break;

	case EBADF:
		status.major = ERR_FSAL_NOT_OPENED;
		break;

	case ENOMEM:
		status.major = ERR_FSAL_NOMEM;
		break;

	case EACCES:
		status.major = ERR_FSAL_ACCESS;
		break;

	case EFAULT:
		status.major = ERR_FSAL_FAULT;
		break;

	case EEXIST:
		status.major = ERR_FSAL_EXIST;
		break;

	case EXDEV:
		status.major = ERR_FSAL_XDEV;
		break;

	case ENOTDIR:
		status.major = ERR_FSAL_NOTDIR;
		break;

	case EISDIR:
		status.major = ERR_FSAL_ISDIR;
		break;

	case EINVAL:
		status.major = ERR_FSAL_INVAL;
		break;

	case EFBIG:
		status.major = ERR_FSAL_FBIG;
		break;

	case ENOSPC:
		status.major = ERR_FSAL_NOSPC;
		break;

	case EMLINK:
		status.major = ERR_FSAL_MLINK;
		break;

	case EDQUOT:
		status.major = ERR_FSAL_DQUOT;
		break;

	case ENAMETOOLONG:
		status.major = ERR_FSAL_NAMETOOLONG;
		break;

	case ENOTEMPTY:
		status.major = ERR_FSAL_NOTEMPTY;
		break;

	case ESTALE:
		status.major = ERR_FSAL_STALE;
		break;

	case EAGAIN:
	case EBUSY:
		status.major = ERR_FSAL_DELAY;
		break;

	default:
		status.major = ERR_FSAL_SERVERFAULT;
		break;
	}

	return status;
}

/**
 * @brief Construct a new filehandle
 *
 * This function constructs a new SIM FSAL object handle and attaches
 * it to the export.  After this call the attributes have been filled
 * in and the handle is up-to-date and usable.
 *
 * @param[in]  export The export on which the object lives
 * @param[in]  sim_fh Concise representation of the object name,
 *                    in SIM notation
 * @param[inout] st   Object attributes
 * @param[out] obj    Object created
 *
 * @return 0 on success, negative error codes on failure.
 */

int sim_construct_handle(struct sim_fsal_export *export,
			 struct sim_file_handle *sim_fh,
			 struct stat *st,
			 struct sim_fsal_handle **obj)
{
	/* Pointer to the handle under construction */
	struct sim_fsal_handle *constructing = NULL;
	*obj = NULL;

	constructing = gsh_calloc(1, sizeof(struct sim_fsal_handle));
	constructing->sim_fh = sim_fh;
	constructing->up_ops = export->export.up_ops; /* XXXX going away */

	fsal_obj_handle_init(&constructing->handle, &export->export,
			     posix2fsal_type(st->st_mode));
	constructing->handle.obj_ops = &SIMFSM.handle_ops;
	constructing->handle.fsid = posix2fsal_fsid(st->st_dev);
	constructing->handle.fileid = st->st_ino;

	constructing->export = export;

	*obj = constructing;

	return 0;
}

// void deconstruct_handle(struct rgw_handle *obj)
// {
// 	fsal_obj_handle_fini(&obj->handle);
// 	gsh_free(obj);
// }
