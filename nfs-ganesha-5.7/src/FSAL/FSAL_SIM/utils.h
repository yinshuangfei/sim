/*
 * @Author: Alan Yin
 * @Date: 2024-10-31 19:25:44
 * @LastEditTime: 2024-12-17 22:20:34
 * @LastEditors: Alan Yin
 * @FilePath: /ganesha-sim/nfs-ganesha-5.7/src/FSAL/FSAL_SIM/utils.h
 * @Description:
 * @// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
 * @// vim: ts=8 sw=2 smarttab
 * @Copyright (c) 2024 by Alan Yin, All Rights Reserved.
 */
/*
 * @Author: Alan Yin
 * @Date: 2024-10-31 19:25:44
 * @LastEditTime: 2024-11-06 19:37:15
 * @LastEditors: Alan Yin
 * @FilePath: /ganesha-sim/nfs-ganesha-5.7/src/FSAL/FSAL_SIM/utils.h
 * @Description:
 * @// -*- mode:C; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
 * @// vim: ts=8 sw=2 smarttab
 * @Copyright (c) 2024 by Alan Yin, All Rights Reserved.
 */

#ifndef YSF_UTILS_H
#define YSF_UTILS_H

#include <stdio.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))


/**
 * 错误输出： LogCrit
 * 信息输出： LogInfo
 *
 * echo -e "\e[31m红色文本\e[0m"
 * echo -e "\e[32m绿色文本\e[0m"
 * echo -e "\e[33m黄色文本\e[0m"
 * echo -e "\e[34m蓝色文本\e[0m"
 * echo -e "\e[35m紫色文本\e[0m"
 * echo -e "\e[36m青色文本\e[0m"
 * echo -e "\e[37m白色文本\e[0m"
*/
#define ENTRY_PREFIX	"\e[35m[ENTRY -----> TEST]\e[0m"
#define DEBUG_PREFIX	"\e[34m[DEBUG -----> TEST]\e[0m"
#define INFO_PREFIX	"\e[32m[INFO  -----> TEST]\e[0m"
#define WARN_PREFIX	"\e[33m[WARN  -----> TEST]\e[0m"
#define ERR_PREFIX	"\e[31m[ERR   -----> TEST]\e[0m"

#define pr_entry()							\
	do {								\
		LogDebug(COMPONENT_FSAL, ENTRY_PREFIX			\
			" ========>>> [   %-20s   ] <<<========",	\
			__FUNCTION__);					\
		fprintf(stdout, ENTRY_PREFIX				\
			" ========>>> [   %-20s   ] <<<========\n",	\
			__FUNCTION__);					\
	} while (0)

#define pr_dbg(format, ...)						\
	do {								\
		LogDebug(COMPONENT_FSAL, DEBUG_PREFIX" (%s:%d): "format,\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
		fprintf(stdout, DEBUG_PREFIX" (%s:%d): "format"\n",	\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
	} while (0)

#define pr_info(format, ...)						\
	do {								\
		LogInfo(COMPONENT_FSAL, INFO_PREFIX" (%s:%d): "format,	\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
		fprintf(stdout, INFO_PREFIX" (%s:%d): "format"\n",	\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
	} while (0)

#define pr_warn(format, ...)						\
	do {								\
		LogWarn(COMPONENT_FSAL, WARN_PREFIX" (%s:%d): "format,	\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
		fprintf(stdout, WARN_PREFIX" (%s:%d): "format"\n",	\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
	} while (0)

#define pr_err(format, ...)						\
	do {								\
		LogCrit(COMPONENT_FSAL, ERR_PREFIX" (%s:%d): "format,	\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
		fprintf(stdout, ERR_PREFIX" (%s:%d): "format"\n",	\
			__FUNCTION__, __LINE__, ##__VA_ARGS__);		\
	} while (0)

#define SAFE_FREE(ptr)							\
	if (ptr) {							\
		free(ptr);						\
		ptr = NULL;						\
	}

#endif /** YSF_UTILS_H */