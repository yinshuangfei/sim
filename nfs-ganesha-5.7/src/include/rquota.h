// SPDX-License-Identifier: unknown license...
/*
 * The content of this file is a mix of rpcgen-generated
 * and hand-edited program text.  It is not automatically
 * generated by, e.g., build processes.
 *
 * This file is under version control.
 */

#ifndef _RQUOTA_H_RPCGEN
#define _RQUOTA_H_RPCGEN

#include "gsh_rpc.h"
#include "extended_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RQ_PATHLEN 1024

	struct sq_dqblk {
		u_int rq_bhardlimit;
		u_int rq_bsoftlimit;
		u_int rq_curblocks;
		u_int rq_fhardlimit;
		u_int rq_fsoftlimit;
		u_int rq_curfiles;
		u_int rq_btimeleft;
		u_int rq_ftimeleft;
	};
	typedef struct sq_dqblk sq_dqblk;

	struct getquota_args {
		char *gqa_pathp;
		int gqa_uid;
	};
	typedef struct getquota_args getquota_args;

	struct setquota_args {
		int sqa_qcmd;
		char *sqa_pathp;
		int sqa_id;
		sq_dqblk sqa_dqblk;
	};
	typedef struct setquota_args setquota_args;

	struct ext_getquota_args {
		char *gqa_pathp;
		int gqa_type;
		int gqa_id;
	};
	typedef struct ext_getquota_args ext_getquota_args;

	struct ext_setquota_args {
		int sqa_qcmd;
		char *sqa_pathp;
		int sqa_id;
		int sqa_type;
		sq_dqblk sqa_dqblk;
	};
	typedef struct ext_setquota_args ext_setquota_args;

	struct rquota {
		int rq_bsize;
		bool_t rq_active;
		u_int rq_bhardlimit;
		u_int rq_bsoftlimit;
		u_int rq_curblocks;
		u_int rq_fhardlimit;
		u_int rq_fsoftlimit;
		u_int rq_curfiles;
		u_int rq_btimeleft;
		u_int rq_ftimeleft;
	};
	typedef struct rquota rquota;

	enum qr_status {
		Q_OK = 1,
		Q_NOQUOTA = 2,
		Q_EPERM = 3,
	};
	typedef enum qr_status qr_status;

	struct getquota_rslt {
		qr_status status;
		union {
			rquota gqr_rquota;
		} getquota_rslt_u;
	};
	typedef struct getquota_rslt getquota_rslt;

	struct setquota_rslt {
		qr_status status;
		union {
			rquota sqr_rquota;
		} setquota_rslt_u;
	};
	typedef struct setquota_rslt setquota_rslt;

#define RQUOTAPROG 100011
#define RQUOTAVERS 1

#if defined(__STDC__) || defined(__cplusplus)
#define RQUOTAPROC_GETQUOTA 1
	extern getquota_rslt *rquotaproc_getquota_1(getquota_args *, CLIENT *);
	extern getquota_rslt *rquotaproc_getquota_1_svc(getquota_args *,
							struct svc_req *);
#define RQUOTAPROC_GETACTIVEQUOTA 2
	extern getquota_rslt *rquotaproc_getactivequota_1(getquota_args *,
							  CLIENT *);
	extern getquota_rslt *rquotaproc_getactivequota_1_svc(getquota_args *,
							      struct svc_req *);
#define RQUOTAPROC_SETQUOTA 3
	extern setquota_rslt *rquotaproc_setquota_1(setquota_args *, CLIENT *);
	extern setquota_rslt *rquotaproc_setquota_1_svc(setquota_args *,
							struct svc_req *);
#define RQUOTAPROC_SETACTIVEQUOTA 4
	extern setquota_rslt *rquotaproc_setactivequota_1(setquota_args *,
							  CLIENT *);
	extern setquota_rslt *rquotaproc_setactivequota_1_svc(setquota_args *,
							      struct svc_req *);
	extern int rquotaprog_1_freeresult(SVCXPRT *, xdrproc_t, void *);
	extern char *check_handle_lead_slash(char *, char *, size_t);

#else				/* K&R C */
#define RQUOTAPROC_GETQUOTA 1
	extern getquota_rslt *rquotaproc_getquota_1();
	extern getquota_rslt *rquotaproc_getquota_1_svc();
#define RQUOTAPROC_GETACTIVEQUOTA 2
	extern getquota_rslt *rquotaproc_getactivequota_1();
	extern getquota_rslt *rquotaproc_getactivequota_1_svc();
#define RQUOTAPROC_SETQUOTA 3
	extern setquota_rslt *rquotaproc_setquota_1();
	extern setquota_rslt *rquotaproc_setquota_1_svc();
#define RQUOTAPROC_SETACTIVEQUOTA 4
	extern setquota_rslt *rquotaproc_setactivequota_1();
	extern setquota_rslt *rquotaproc_setactivequota_1_svc();
	extern int rquotaprog_1_freeresult();
	extern char *check_handle_lead_slash();
#endif				/* K&R C */

/* Number of rquota commands. */
#define RQUOTA_NB_COMMAND (RQUOTAPROC_SETACTIVEQUOTA + 1)

#define EXT_RQUOTAVERS 2

/* the xdr functions */

#if defined(__STDC__) || defined(__cplusplus)
	extern bool xdr_sq_dqblk(XDR *, sq_dqblk *);
	extern bool xdr_getquota_args(XDR *, getquota_args *);
	extern bool xdr_setquota_args(XDR *, setquota_args *);
	extern bool xdr_ext_getquota_args(XDR *, ext_getquota_args *);
	extern bool xdr_ext_setquota_args(XDR *, ext_setquota_args *);
	extern bool xdr_rquota(XDR *, rquota *);
	extern bool xdr_qr_status(XDR *, qr_status *);
	extern bool xdr_getquota_rslt(XDR *, getquota_rslt *);
	extern bool xdr_setquota_rslt(XDR *, setquota_rslt *);

#else				/* K&R C */
	extern bool_t xdr_sq_dqblk();
	extern bool_t xdr_getquota_args();
	extern bool_t xdr_setquota_args();
	extern bool_t xdr_ext_getquota_args();
	extern bool_t xdr_ext_setquota_args();
	extern bool_t xdr_rquota();
	extern bool_t xdr_qr_status();
	extern bool_t xdr_getquota_rslt();
	extern bool_t xdr_setquota_rslt();

#endif				/* K&R C */

#ifdef __cplusplus
}
#endif
#endif				/* !_RQUOTA_H_RPCGEN */
