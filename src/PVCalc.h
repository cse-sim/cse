// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// pvwatts.h -- defns and decls for PVWATTS interface
///////////////////////////////////////////////////////////////////////////////

#if !defined( _PVCALC_H)
#define _PVCALC_H

#define SSC_LOG 0
#define SSC_NOTICE 1
#define SSC_WARNING 2
#define SSC_ERROR 3

typedef void* ssc_data_t;
typedef float ssc_number_t;
typedef int ssc_bool_t;
typedef void* ssc_module_t;
typedef void* ssc_handler_t;

///////////////////////////////////////////////////////////////////////////////
// class XPVWATTS: interface to PVWATTS DLL (photovoltaic array module)
///////////////////////////////////////////////////////////////////////////////
#include "xmodule.h"


class XPVWATTS : public XMODULE
{
typedef ssc_data_t (*ssc_data_create_fn)();
typedef void (*ssc_data_set_number_fn)(ssc_data_t p_data, const char* name, ssc_number_t value);
typedef ssc_module_t (*ssc_module_create_fn)(const char* name);
typedef ssc_bool_t (*ssc_module_exec_simple_fn)(const char * name, ssc_data_t p_data);
typedef ssc_bool_t (*ssc_data_get_number_fn)(ssc_data_t p_data, const char * name, ssc_number_t * value);
typedef void (*ssc_data_free_fn)(ssc_data_t p_data);
typedef void (*ssc_module_free_fn)(ssc_module_t p_mod);
typedef ssc_bool_t (*ssc_module_exec_with_handler_fn)(
	ssc_module_t p_mod,
	ssc_data_t p_data,
	ssc_bool_t (*pf_handler)(
		ssc_module_t,
		ssc_handler_t,
		int action,
		float f0,
		float f1,
		const char *s0,
		const char *s1,
		void *user_data),
	void *pf_user_data);

private:
	ssc_data_create_fn ssc_data_create;
	ssc_data_set_number_fn ssc_data_set_number;
	ssc_module_create_fn ssc_module_create;
	ssc_module_exec_simple_fn ssc_module_exec_simple;
	ssc_data_get_number_fn ssc_data_get_number;
	ssc_data_free_fn ssc_data_free;
	ssc_module_free_fn ssc_module_free;
	ssc_module_exec_with_handler_fn ssc_module_exec_with_handler;
	ssc_module_t pv_mod;

public:
	XPVWATTS( const char* moduleName);
	XPVWATTS(){};
	~XPVWATTS();

	RC xp_Setup(const char* moduleName);
	RC xp_CalcAC(PVARRAY *pvArr);
	void xp_InitData(PVARRAY *pvArr);
	void xp_ClearData(PVARRAY *pvArr);

};

#endif	// _PVCALC_H
