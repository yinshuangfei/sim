/* SPDX-License-Identifier: LGPL-3.0-or-later */
/*
 * vim:noexpandtab:shiftwidth=8:tabstop=8:
 *
 * Copyright (C) 2012, The Linux Box Corporation
 * Contributor: Matt Benjamin <matt@linuxbox.com>
 *
 * Some portions Copyright CEA/DAM/DIF  (2008)
 * contributeur: Philippe DENIEL   philippe.deniel@cea.fr
 *               Thomas LEIBOVICI  thomas.leibovici@cea.fr
 *
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
 * @addtogroup FSAL_MDCACHE
 * @{
 */

#ifndef MDCACHE_LRU_H
#define MDCACHE_LRU_H

#include "config.h"
#include "log.h"
#include "mdcache_int.h"

/**
 * @file mdcache_lru.h
 * @author Matt Benjamin
 * @brief Constant-time MDCACHE cache management implementation
 *
 * @section DESCRIPTION
 *
 * This module implements a constant-time cache management strategy
 * based on LRU.  Some ideas are taken from 2Q [Johnson and Shasha 1994]
 * and MQ [Zhou, Chen, Li 2004].  In this system, cache management does
 * interact with cache entry lifecycle.  Also, the cache size high- and
 * low- water mark management is maintained, but executes asynchronously
 * to avoid inline request delay.  Cache management operations execute in
 * constant time, as expected with LRU (and MQ).
 *
 * Cache entries in use by a currently-active protocol request (or other
 * operation) have a positive refcount, and threfore should not be present
 * at the cold end of an lru queue if the cache is well-sized.
 *
 * Cache entries with lock and open state are not eligible for collection
 * under ordinary circumstances, so are kept on a separate lru_pinned
 * list to retain constant time.
 *
 */

struct lru_state {
	uint64_t entries_hiwat;
	uint64_t entries_used;
	uint32_t entries_release_size;
	uint64_t chunks_hiwat;
	uint64_t chunks_lowat;
	uint64_t chunks_used;
	uint32_t per_lane_work;
	time_t prev_time;	/* previous time the gc thread was run. */
};

extern struct lru_state lru_state;

/** Cache entries pool */
extern pool_t *mdcache_entry_pool;

/**
 * Reference type Flags for functions in the LRU package
 */

#define LRU_ACTIVE_REF 0x0004
#define LRU_PROMOTE 0x0008
#define LRU_FLAG_SENTINEL 0x0001
#define LRU_TEMP_REF 0x0002

/**
 * The minimum reference count for a cache entry not being recycled.
 */

#define LRU_SENTINEL_REFCOUNT  1

/**
 * The number of lanes comprising a logical queue.  This must be
 * prime.
 */
#define LRU_N_Q_LANES  17

fsal_status_t mdcache_lru_pkginit(void);
fsal_status_t mdcache_lru_pkgshutdown(void);

mdcache_entry_t *mdcache_lru_get(struct fsal_obj_handle *sub_handle,
				 uint32_t flags);
void mdcache_lru_insert_active(mdcache_entry_t *entry);

#define mdcache_lru_ref(e, f) _mdcache_lru_ref(e, f, __func__, __LINE__)

/**
 *
 * @brief Get a logical reference to a cache entry
 *
 * @param[in] entry   Cache entry being returned
 * @param[in] flags   Set of flags to specify type of reference
 */
void _mdcache_lru_ref(mdcache_entry_t *entry, uint32_t flags,
		      const char *func, int line);

/* XXX */
void mdcache_lru_kill(mdcache_entry_t *entry);
void mdcache_lru_cleanup_push(mdcache_entry_t *entry);
void mdcache_lru_cleanup_try_push(mdcache_entry_t *entry);

size_t mdcache_lru_release_entries(int32_t want_release);

#define mdcache_lru_unref(e, f) _mdcache_lru_unref(e, f, __func__, __LINE__)

/**
 *
 * @brief Release logical reference to a cache entry
 *
 * This function releases a logical reference to a cache entry
 * acquired by a previous mdcache handle op (such as lookup, create, etc.)
 *
 * The result is typically to decrement the reference count on entry,
 * but additional side effects include LRU adjustment, movement
 * to/from the protected LRU partition, or recycling if the caller has
 * raced an operation which made entry unreachable (and this current
 * caller has the last reference).  Caller MUST NOT make further
 * accesses to the memory pointed to by entry.
 *
 * @param[in] entry Cache entry being returned
 * @param[in] flags Flag to indicate what kind of reference is to be released
 */
bool _mdcache_lru_unref(mdcache_entry_t *entry, uint32_t flags,
			const char *func, int line);

void mdcache_lru_kill_for_shutdown(mdcache_entry_t *entry);

#define mdcache_lru_ref_chunk(chunk) \
	_mdcache_lru_ref_chunk(chunk, __func__, __LINE__)
void _mdcache_lru_ref_chunk(struct dir_chunk *chunk, const char *func,
			    int line);
#define mdcache_lru_unref_chunk(chunk) \
	_mdcache_lru_unref_chunk(chunk, __func__, __LINE__)
void _mdcache_lru_unref_chunk(struct dir_chunk *chunk, const char *func,
			      int line);
struct dir_chunk *mdcache_get_chunk(mdcache_entry_t *parent,
				    struct dir_chunk *prev_chunk,
				    fsal_cookie_t whence);
void lru_bump_chunk(struct dir_chunk *chunk);

void mdc_lru_map_dirent(mdcache_dir_entry_t *dirent);
fsal_cookie_t *mdc_lru_unmap_dirent(uint64_t ck);
fsal_status_t dirmap_lru_init(struct mdcache_fsal_export *exp);
void dirmap_lru_stop(struct mdcache_fsal_export *exp);
#endif				/* MDCACHE_LRU_H */
/** @} */
