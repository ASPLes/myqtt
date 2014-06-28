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

#include <myqtt.h>

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

axl_bool  test_00_a (void) {
 	unsigned char   buffer[7];
	int             out_position;
	MyQttCtx      * ctx;
	int             remaining_length;

	/* call to init the base library and close it */
	ctx = init_ctx ();
	
	/* check marshalling of 89 */
	printf ("Test 00-a: checking 89...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 89, &out_position)) {
		printf ("ERROR: failed to encode remaining length (89)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 1) {
		printf ("ERROR: expected out position of 1 but found %d for value 89\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 89) {
		printf ("ERROR: expected 89 value but found: %d", buffer[0]);
		return axl_false;
	} /* end if */

	/* check marshalling of 127 */
	printf ("Test 00-a: checking 127...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 127, &out_position)) {
		printf ("ERROR: failed to encode remaining length (127)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 1) {
		printf ("ERROR: expected out position of 1 but found %d for value 127\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 127) {
		printf ("ERROR: expected 127 value but found: %d\n", buffer[0]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 127) {
		printf ("ERROR: expected to have 127 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 1) {
		printf ("ERROR: expected to find out position 1 but found: %d\n", out_position);
		return axl_false;
	}


	/* check marshalling of 128 */
	printf ("Test 00-a: checking 128...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 128, &out_position)) {
		printf ("ERROR: failed to encode remaining length (128)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 2) {
		printf ("ERROR: expected out position of 2 but found %d for value 128\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0x80 || buffer[1] != 0x1) {
		printf ("ERROR: expected 128 value but found: 0x%x 0x%x\n", buffer[0], buffer[1]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 128) {
		printf ("ERROR: expected to have 128 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 2) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 139 */
	printf ("Test 00-a: checking 139...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 139, &out_position)) {
		printf ("ERROR: failed to encode remaining length (139)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 2) {
		printf ("ERROR: expected out position of 2 but found %d for value 139\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0x8b || buffer[1] != 0x1) {
		printf ("ERROR: expected 139 value but found: 0x%x 0x%x\n", buffer[0], buffer[1]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 139) {
		printf ("ERROR: expected to have 139 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 2) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 16217 */
	printf ("Test 00-a: checking 16217...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 16217, &out_position)) {
		printf ("ERROR: failed to encode remaining length (16217)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 2) {
		printf ("ERROR: expected out position of 2 but found %d for value 16217\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0xd9 || buffer[1] != 0x7e) {
		printf ("ERROR: expected 16217 value but found: 0x%x 0x%x\n", buffer[0], buffer[1]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 16217) {
		printf ("ERROR: expected to have 16217 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 2) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 16383 */
	printf ("Test 00-a: checking 16383...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 16383, &out_position)) {
		printf ("ERROR: failed to encode remaining length (16383)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 2) {
		printf ("ERROR: expected out position of 2 but found %d for value 16383\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0xff || buffer[1] != 0x7f) {
		printf ("ERROR: expected 16383 value but found: 0x%x 0x%x\n", buffer[0], buffer[1]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 16383) {
		printf ("ERROR: expected to have 16383 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 2) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 127364 */
	printf ("Test 00-a: checking 127364...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 127364, &out_position)) {
		printf ("ERROR: failed to encode remaining length (127364)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 3) {
		printf ("ERROR: expected out position of 3 but found %d for value 127364\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0x84 || buffer[1] != 0xe3 || buffer[2] != 0x7) {
		printf ("ERROR: expected 127364 value but found: 0x%x 0x%x 0x%x\n", buffer[0], buffer[1], buffer[2]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 127364) {
		printf ("ERROR: expected to have 127364 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 3) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 2097151 */
	printf ("Test 00-a: checking 2097151...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 2097151, &out_position)) {
		printf ("ERROR: failed to encode remaining length (2097151)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 3) {
		printf ("ERROR: expected out position of 3 but found %d for value 2097151\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0xFF || buffer[1] != 0xFF || buffer[2] != 0x7F) {
		printf ("ERROR: expected 2097151 value but found: 0x%x 0x%x 0x%x\n", buffer[0], buffer[1], buffer[2]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 2097151) {
		printf ("ERROR: expected to have 2097151 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 3) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 5678945 */
	printf ("Test 00-a: checking 5678945...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 5678945, &out_position)) {
		printf ("ERROR: failed to encode remaining length (5678945)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 4) {
		printf ("ERROR: expected out position of 4 but found %d for value 5678945\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0xe1 || buffer[1] != 0xce || buffer[2] != 0xda || buffer[3] != 0x2) {
		printf ("ERROR: expected 5678945 value but found: 0x%x 0x%x 0x%x 0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 5678945) {
		printf ("ERROR: expected to have 5678945 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 4) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 268435455 */
	printf ("Test 00-a: checking 268435455...\n");
	out_position = 0;
	if (! myqtt_msg_encode_remaining_length (ctx, buffer, 268435455, &out_position)) {
		printf ("ERROR: failed to encode remaining length (268435455)..\n");
		return axl_false;
	} /* end if */

	if (out_position != 4) {
		printf ("ERROR: expected out position of 4 but found %d for value 268435455\n", out_position);
		return axl_false;
	}
	if (buffer[0] != 0xff || buffer[1] != 0xff || buffer[2] != 0xff || buffer[3] != 0x7f) {
		printf ("ERROR: expected 268435455 value but found: 0x%x 0x%x 0x%x 0x%x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
		return axl_false;
	} /* end if */

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != 268435455) {
		printf ("ERROR: expected to have 268435455 but found: %d\n", remaining_length);
		return axl_false;
	}
	if (out_position != 4) {
		printf ("ERROR: expected to find out position 2 but found: %d\n", out_position);
		return axl_false;
	}

	/* check marshalling of 1268435455 */
	printf ("Test 00-a: checking 1268435455...\n");
	out_position = 0;
	if (myqtt_msg_encode_remaining_length (ctx, buffer, 1268435455, &out_position)) {
		printf ("ERROR: expected error while encoding bigger number ..\n");
		return axl_false;
	} /* end if */


	buffer[0] = 0xff;
	buffer[1] = 0xff;
	buffer[2] = 0xff;
	buffer[3] = 0xff;
	buffer[5] = 0xff;

	/* get remaining length */
	remaining_length = myqtt_msg_decode_remaining_length (ctx, buffer, &out_position);
	if (remaining_length != -1) {
		printf ("ERROR: expected to have -1 but found: %d\n", remaining_length);
		return axl_false;
	}

	/* now close the library */
	myqtt_exit_ctx (ctx, axl_true);
	
	return axl_true;
}

axl_bool test_01 (void) {

	MyQttCtx  * ctx = init_ctx ();
	MyQttConn * conn;
	if (! ctx)
		return axl_false;

	printf ("Test 01: creating connection..\n");

	/* now connect to the listener:
	   client_identifier -> test_01
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_01", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 01: connected without problems..\n");

	/* do some checkings */

	/* close connection */
	printf ("Test 01: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 01: releasing context..\n");
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
	printf ("** Available tests: test_00, test_00_a, test_01\n");
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
	run_test (test_00, "Test 00: generic API function checks");

	/* run tests */
	CHECK_TEST("test_00_a")
	run_test (test_00_a, "Test 00-a: test remaining length generation");

	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: basic listener startup and client connection");

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
