/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2014 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build MQTT enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/mqtt
 *                        http://www.aspl.es/myqtt
 */

#include <myqttd.h>

axl_bool test_common_enable_debug = axl_false;

/* default listener location */
const char * listener_host = "localhost";
const char * listener_port = "1909";

MyQttCtx * init_ctx (void)
{
	MyQttCtx * ctx;

	/* call to init the base library and close it */
	ctx = myqtt_ctx_new ();

	if (! myqtt_init_ctx (ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return NULL;
	} /* end if */

	/* enable log if requested by the user */
	if (test_common_enable_debug) {
		myqtt_log_enable (ctx, axl_true);
		myqtt_color_log_enable (ctx, axl_true);
		myqtt_log2_enable (ctx, axl_true);
	} /* end if */
		

	return ctx;
}

axl_bool  test_00 (void) {

	MyQttCtx * ctx;
	
	/* call to init the base library and close it */
	ctx = myqtt_ctx_new ();

	if (! myqtt_init_ctx (ctx)) {
		printf ("Error: unable to initialize MyQtt library..\n");
		return axl_false;
	} /* end if */

	/* now close the library */
	myqtt_exit_ctx (ctx, axl_true);
		
	return axl_true;
}


#define CHECK_TEST(name) if (run_test_name == NULL || axl_cmp (run_test_name, name))

typedef axl_bool (* MyQttTestHandler) (void);

/** 
 * @brief Helper handler that allows to execute the function provided
 * with the message associated.
 * @param function The handler to be called (test)
 * @param message The message value.
 */
int run_test (MyQttTestHandler function, const char * message) {

	printf ("--- --: starting %s\n", message);

	if (function ()) {
		printf ("%s [   OK   ]\n", message);
	} else {
		printf ("%s [ FAILED ]\n", message);
		exit (-1);
	}
	return 0;
}

/** 
 * @brief General regression test to check all features inside myqtt
 */
int main (int argc, char ** argv)
{
	char * run_test_name = NULL;

	printf ("** MyQtt: A high performance open source MQTT implementation\n");
	printf ("** Copyright (C) 2014 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Regression tests: %s \n",
		VERSION);
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     time ./test_01 [--help] [--debug] [--no-unmap] [--run-test=NAME]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./test_01 [--debug]\n**\n");
	printf ("** Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("** Available tests: test_00\n");
	printf ("**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <myqtt@lists.aspl.es> MyQtt Mailing list\n**\n");

	/* uncomment the following four lines to get debug */
	while (argc > 0) {
		if (axl_cmp (argv[argc], "--help")) 
			exit (0);
		if (axl_cmp (argv[argc], "--debug")) 
			test_common_enable_debug = axl_true;
	
		if (argv[argc] && axl_memcmp (argv[argc], "--run-test", 10)) {
			run_test_name = argv[argc] + 11;
			printf ("INFO: running test: %s\n", run_test_name);
		}
		argc--;
	} /* end if */

	/* run tests */
	CHECK_TEST("test_00")
	run_test (test_00, "Test 00: basic context initialization");

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
