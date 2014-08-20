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
 *         C/ Antonio Suarez NÂº 10, 
 *         Edificio Alius A, Despacho 102
 *         AlcalÃ¡ de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/mqtt
 *                        http://www.aspl.es/myqtt
 */

#include <myqtt.h>
#include <stdlib.h>

/** 
 * IMPORTANT NOTE: do not include this header and any other header
 * that includes "private". They are used for internal definitions and
 * may change at any time without any notification. This header is
 * included on this file with the sole purpose of testing. 
 */
#include <myqtt-conn-private.h>
#include <myqtt-ctx-private.h>

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

	/* configure default storage location */
	myqtt_storage_set_path (ctx, ".myqtt-regression-client", 4096);

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

void test_00_b_check_and_exit (const char * test, axl_bool should_work)
{
	if (! myqtt_support_is_utf8 (test, strlen (test))) {
		if (! should_work) {
			printf ("Test 00-b: ok, detected as wrong (%s)..\n", test);
			return;
		}
		printf ("ERROR: expected to find utf8 for %s but found it is not UTF-8 as reported by myqtt_support_is_utf8\n", test);
		exit (-1);
	}
	printf ("Test 00-b: %s OK\n", test);
	return;
}

axl_bool  test_00_b (void) {

	test_00_b_check_and_exit ("aspl", axl_true);

	test_00_b_check_and_exit ("CamiÃ³n", axl_true);

	test_00_b_check_and_exit ("Ð—Ð´Ñ€Ð°Ð²Ð¾", axl_true);

	test_00_b_check_and_exit ("HÃ¦ Ã¾aÃ°", axl_true);
	
	test_00_b_check_and_exit ("€", axl_false);

	test_00_b_check_and_exit ("¿", axl_false);

	test_00_b_check_and_exit ("€¿", axl_false);

	test_00_b_check_and_exit ("€¿€", axl_false);

	test_00_b_check_and_exit ("€¿€¿", axl_false);

	test_00_b_check_and_exit ("€¿€¿€", axl_false);

	test_00_b_check_and_exit ("€¿€¿€¿", axl_false);

	test_00_b_check_and_exit ("€¿€¿€¿€", axl_false);

	test_00_b_check_and_exit ("€‚ƒ„…†‡ˆ‰Š‹ŒŽ‘’“”•–—˜™š›œžŸ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿", axl_false);

	test_00_b_check_and_exit ("À Á Â Ã Ä Å Æ Ç È É Ê Ë Ì Í Î Ï Ð Ñ Ò Ó Ô Õ Ö × Ø Ù Ú Û Ü Ý Þ ß ", axl_false);

	test_00_b_check_and_exit ("à á â ã ä å æ ç è é ê ë ì í î ï ", axl_false);

	test_00_b_check_and_exit ("ð ñ ò ó ô õ ö ÷ ", axl_false);

	test_00_b_check_and_exit ("ø ù ú û ", axl_false);

	test_00_b_check_and_exit ("ü ý ", axl_false);

	test_00_b_check_and_exit ("À", axl_false);

	test_00_b_check_and_exit ("à€", axl_false);

	test_00_b_check_and_exit ("ð€€", axl_false);

	test_00_b_check_and_exit ("ø€€€", axl_false);

	test_00_b_check_and_exit ("ü€€€€", axl_false);

	test_00_b_check_and_exit ("ß", axl_false);

	test_00_b_check_and_exit ("ï¿", axl_false);

	test_00_b_check_and_exit ("÷¿¿", axl_false);

	test_00_b_check_and_exit ("û¿¿¿", axl_false);

	test_00_b_check_and_exit ("ý¿¿¿¿", axl_false);

	test_00_b_check_and_exit ("Àà€ð€€ø€€€ü€€€€ßï¿÷¿¿û¿¿¿ý¿¿¿¿", axl_false);

	test_00_b_check_and_exit ("þ", axl_false);

	test_00_b_check_and_exit ("ÿ", axl_false);

	test_00_b_check_and_exit ("þþÿÿ", axl_false);

	test_00_b_check_and_exit ("À¯", axl_false);

	test_00_b_check_and_exit ("à€¯", axl_false);

	test_00_b_check_and_exit ("ð€€¯", axl_false);

	test_00_b_check_and_exit ("ø€€€¯", axl_false);

	test_00_b_check_and_exit ("ü€€€€¯", axl_false);

	test_00_b_check_and_exit ("Á¿", axl_false);

	test_00_b_check_and_exit ("àŸ¿", axl_false);

	test_00_b_check_and_exit ("ð¿¿", axl_false);

	test_00_b_check_and_exit ("ø‡¿¿¿", axl_false);

	test_00_b_check_and_exit ("üƒ¿¿¿¿", axl_false);
	
	test_00_b_check_and_exit ("The following five sequences should also be rejected like malformed           |\
UTF-8 sequences and should not be treated like the ASCII NUL                  |\
character.                                                                    |", axl_true);

	test_00_b_check_and_exit ("À€", axl_false);

	test_00_b_check_and_exit ("à€€", axl_false);

	test_00_b_check_and_exit ("ð€€€", axl_false);

	test_00_b_check_and_exit ("ø€€€€", axl_false);

	test_00_b_check_and_exit ("ü€€€€€", axl_false);

	test_00_b_check_and_exit ("í €", axl_false);

	test_00_b_check_and_exit ("í­¿", axl_false);

	test_00_b_check_and_exit ("í®€", axl_false);

	test_00_b_check_and_exit ("í¯¿", axl_false);

	test_00_b_check_and_exit ("í°€", axl_false);

	test_00_b_check_and_exit ("í¾€", axl_false);

	test_00_b_check_and_exit ("í¿¿", axl_false);

	test_00_b_check_and_exit ("í €í°€", axl_false);

	test_00_b_check_and_exit ("í €í¿¿", axl_false);

	test_00_b_check_and_exit ("í­¿í°€", axl_false);

	test_00_b_check_and_exit ("í­¿í¿¿", axl_false);

	test_00_b_check_and_exit ("í®€í°€", axl_false);

	test_00_b_check_and_exit ("í®€í¿¿", axl_false);

	test_00_b_check_and_exit ("í¯¿í°€", axl_false);

	test_00_b_check_and_exit ("í¯¿í¿¿", axl_false);

	test_00_b_check_and_exit ("ï¿¾", axl_false);

	test_00_b_check_and_exit ("ï¿¿", axl_false);

	test_00_b_check_and_exit ("Esto es una prueba para detectar que todo es correcto!", axl_true);

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
	printf ("Test 01: closing connection (refs: %d)..\n", myqtt_conn_ref_count (conn));
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 01: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}


axl_bool test_02 (void) {

	MyQttCtx  * ctx = init_ctx ();
	MyQttConn * conn;
	int         sub_result;
	if (! ctx)
		return axl_false;

	printf ("Test 02: creating connection..\n");

	/* now connect to the listener:
	   client_identifier -> test_02
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_02", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 02: connected without problems..\n");

	/* subscribe to a topic */
	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	/* close connection */
	printf ("Test 02: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 02: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);



	return axl_true;
}

void test_03_on_message (MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	/* push message received */
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (queue, msg);
	return;
} 

void test_03_fail (MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	printf ("ERROR: this handler shouldn't be called because this connection has no subscriptions..\n");
	return;
} 

axl_bool test_03 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConn       * conn2;
	int               sub_result;
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;

	if (! ctx)
		return axl_false;

	printf ("Test 03: creating connection..\n");

	/* now connect to the listener:
	   client_identifier -> test_03
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_03", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 03: connected without problems..\n");

	/* now connect to the listener:
	   client_identifier -> test_03
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn2 = myqtt_conn_new (ctx, "test_03-2", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 03: connected without problems (without subscriptions)..\n");

	/* subscribe to a topic */
	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	/* register on message handler */
	queue = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);
	myqtt_conn_set_on_msg (conn2, test_03_fail, NULL);

	/* publish application message */
	if (! myqtt_conn_pub (conn, "myqtt/test", "This is test message....", 24, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message, myqtt_conn_pub() failed\n");
		return axl_false;
	} /* end if */

	/* waiting for reply */
	printf ("Test 03: waiting for reply..\n");
	msg   = myqtt_async_queue_pop (queue);
	myqtt_async_queue_unref (queue);
	if (msg == NULL) {
		printf ("ERROR: expected to find message from queue, but NULL was found..\n");
		return axl_false;
	} /* end if */

	/* check content */
	if (myqtt_msg_get_app_msg_size (msg) != 24) {
		printf ("ERROR: expected payload size of 24 but found %d\n", myqtt_msg_get_app_msg_size (msg));
		return axl_false;
	} /* end if */

	if (myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
		printf ("ERROR: expected to receive PUBLISH message but found: %s\n", myqtt_msg_get_type_str (msg));
		return axl_false;
	} /* end if */

	/* check content */
	if (! axl_cmp ((const char *) myqtt_msg_get_app_msg (msg), "This is test message....")) {
		printf ("ERROR: expected to find different content..\n");
		return axl_false;
	} /* end if */

	/* release message */
	printf ("Test 03: releasing references=%d\n", myqtt_msg_ref_count (msg));
	myqtt_msg_unref (msg);
 
	/* close connection */
	printf ("Test 03: closing connection..\n");
	myqtt_conn_close (conn);
	myqtt_conn_close (conn2);

	/* release context */
	printf ("Test 03: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);



	return axl_true;
}

axl_bool test_04 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	int               sub_result;
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;

	if (! ctx)
		return axl_false;

	printf ("Test 04: creating connection..\n");

	/* now connect to the listener:
	   client_identifier -> test_03
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_03", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 04: connected without problems..\n");


	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_unsub (conn, "myqtt/test", 10)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	/* register on message handler */
	myqtt_conn_set_on_msg (conn, test_03_fail, NULL);

	/* publish application message */
	if (! myqtt_conn_pub (conn, "myqtt/test", "This is test message....", 24, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message, myqtt_conn_pub() failed\n");
		return axl_false;
	} /* end if */

	/* waiting for reply */
	printf ("Test 04: waiting for reply (we shouldn't get one, waiting 3 seconds)..\n");
	queue = myqtt_async_queue_new ();
	msg   = myqtt_async_queue_timedpop (queue, 3000000);
	myqtt_async_queue_unref (queue);
	if (msg != NULL) {
		printf ("ERROR: expected to find message from queue, but NULL was found..\n");
		return axl_false;
	} /* end if */

	/* close connection */
	printf ("Test 04: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 04: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);



	return axl_true;
}

axl_bool test_05 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	struct timeval    start;
	struct timeval    stop;
	struct timeval    diff;

	if (! ctx)
		return axl_false;

	printf ("Test 05: creating connection..\n");

	/* now connect to the listener:
	   client_identifier -> test_05
	   clean_session -> axl_true
	   keep_alive -> 30 */
	gettimeofday (&start, NULL);
	conn = myqtt_conn_new (ctx, "test_05", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */
	gettimeofday (&stop, NULL);
	myqtt_timeval_substract (&stop, &start, &diff);

	printf ("Test 05: connected without problems in %.2f ms, sending ping..\n", (double) diff.tv_usec / (double) 1000);
	gettimeofday (&start, NULL);
	if (! myqtt_conn_ping (conn, 10)) {
		printf ("ERROR: failed to send PING request or reply wasn't received after wait period..\n");
		return axl_false;
	} /* end if */
	gettimeofday (&stop, NULL);
	myqtt_timeval_substract (&stop, &start, &diff);

	printf ("Test 05: received ping reply in in %.2f ms..\n", (double)diff.tv_usec / (double) 1000);

	/* close connection */
	printf ("Test 05: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 05: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_06 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConn       * conn2;
	MyQttConn       * conn3;
	MyQttMsg        * msg;

	if (! ctx)
		return axl_false;

	printf ("Test 06: creating connection..\n");

	/* now connect to the listener:
	   client_identifier -> test_05
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_06.identifier", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* call to get client identifier */
	if (! myqtt_conn_pub (conn, "myqtt/admin/get-client-identifier", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		return axl_false;
	} /* end if */

	/* push a message to ask for clientid identifier */
	msg   = myqtt_conn_get_next (conn, 10000);

	printf ("Test 06: message reference is: %p (%s) (expecting client identifier)\n", msg, (const char *) myqtt_msg_get_app_msg (msg));
	if (! axl_cmp ("test_06.identifier", (const char *) myqtt_msg_get_app_msg (msg))) {
		printf ("ERROR: expected client identifier test_06.identifier, but found: %s\n", (const char *) myqtt_msg_get_app_msg (msg));
		return axl_false;
	} /* end if */

	/* call to release message */
	myqtt_msg_unref (msg);

	printf ("Test 06: checking that is not accepted two connections with the same client identifier..\n");

	/* now try to connect again with the same identifier */
	conn2 = myqtt_conn_new (ctx, "test_06.identifier", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: expected to find connection error for same identifier but found connection ok\n");
		return axl_false;
	} /* end if */

	printf ("Test 06: Error reported by myqtt_conn_new = %d\n", myqtt_conn_get_last_err (conn2));
	if (myqtt_conn_get_last_err (conn2) != MYQTT_CONNACK_IDENTIFIER_REJECTED) {
		printf ("ERROR: expected identifier rejected..but found different error\n");
		return axl_false;
	} /* end if */

	printf ("Test 06: OK, now check automatic client ids..\n");

	/* ok, connect with a NULL client id */
	conn3 = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn3, axl_false)) {
		printf ("ERROR: expected to find OK connection but found error\n");
		return axl_false;
	} /* end if */

	/* call to get client identifier */
	if (! myqtt_conn_pub (conn3, "myqtt/admin/get-client-identifier", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		return axl_false;
	} /* end if */

	/* push a message to ask for clientid identifier */
	msg   = myqtt_conn_get_next (conn3, 10000);
	if (msg == NULL || myqtt_msg_get_app_msg (msg) == NULL) {
		printf ("ERROR: expected to received msg defined but found NULL\n");
		return axl_false;
	} /* end if */

	printf ("Test 06: automatic id assigned is: %s (%p)\n", (const char *) myqtt_msg_get_app_msg (msg), msg);
	myqtt_msg_unref (msg);

	myqtt_conn_close (conn3);
	myqtt_conn_close (conn2);

	/* close connection */
	printf ("Test 06: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 06: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_07 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttMsg        * msg;
	MyQttConnOpts   * opts;

	if (! ctx)
		return axl_false;

	printf ("Test 07: creating connection..\n");
	
	/* now connect to the listener:
	   client_identifier -> NULL
	   clean_session -> axl_true
	   keep_alive -> 30 */
	opts = myqtt_conn_opts_new ();
	myqtt_conn_opts_set_auth (opts, "aspl", "wrong-password");
	conn = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, opts, NULL, NULL);
	if (myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected wrong LOGIN but found OK operation to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */
	myqtt_conn_close (conn);

	printf ("Test 07: login failed, this is expected, so OK\n");

	/* try again login with the right password */
	opts = myqtt_conn_opts_new ();
	myqtt_conn_opts_set_auth (opts, "aspl", "test");
	conn = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: OK LOGIN but found ERROR operation to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 07: login OK, now get auth user..\n");

	/* call to get client identifier */
	if (! myqtt_conn_pub (conn, "myqtt/admin/get-conn-user", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		return axl_false;
	} /* end if */

	/* push a message to ask for clientid identifier */
	msg   = myqtt_conn_get_next (conn, 10000);

	printf ("Test 07: auth user reported is: %p (%s)\n", msg, (const char *) myqtt_msg_get_app_msg (msg));
	if (! axl_cmp ("aspl", (const char *) myqtt_msg_get_app_msg (msg))) {
		printf ("ERROR: expected client identifier test_06.identifier, but found: %s\n", (const char *) myqtt_msg_get_app_msg (msg));
		return axl_false;
	} /* end if */

	/* call to release message */
	myqtt_msg_unref (msg);

	printf ("Test 07: checking server reports the connection is authenticated..\n");

	/* close connection */
	printf ("Test 07: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 07: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

void test_08_should_not_receive (MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	printf ("ERROR: received a message on a handler that shouldn't have been called\n");
	return;
}

void test_08_should_receive (MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{

	printf ("Test 08: received will..\n");
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (user_data, msg);
	return;
}


axl_bool test_08 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn, * conn2, * conn3;
	MyQttMsg        * msg; 
	MyQttConnOpts   * opts;
	int               sub_result;
	MyQttAsyncQueue * queue;
	

	if (! ctx)
		return axl_false;

	printf ("Test 08: creating connection..\n");
	
	/* set will message */
	opts = myqtt_conn_opts_new ();
	
	/* set will */
	myqtt_conn_opts_set_will (opts, MYQTT_QOS_2, "I lost connection", "Hey I lost connection, this is my status:....", axl_false);

	/* now connect to the listener:
	   client_identifier -> NULL
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN but found FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* now create a new connection and subscribe to the will */
	conn2 = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN but found FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	}

	if (! myqtt_conn_sub (conn2, 10, "I lost connection", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	/* now create a third connection but without subscription */
	conn3 = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable connect to MQTT server..\n");
		return axl_false;
	} /* end if */

	printf ("Test 08: 3 links connected, now close connection with will\n");
	/* set on publish handler */	
	queue  = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn2, test_08_should_receive, queue);
	myqtt_conn_set_on_msg (conn3, test_08_should_not_receive, NULL);

	/* shutdown connection */
	myqtt_conn_shutdown (conn);

	/* wait for message */
	printf ("Test 08: waiting for will... (3 seconds at most)\n");
	msg = myqtt_async_queue_timedpop (queue, 3000000);
	if (msg == NULL) {
		printf ("ERROR: expected to receive msg reference but found NULL value..\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (myqtt_msg_get_topic (msg), "I lost connection")) {
		printf ("ERROR: expected to find topic name 'I lost connection' but found: '%s'\n", myqtt_msg_get_topic (msg));
		return axl_false;
	} /* end if */

	if (! axl_cmp (myqtt_msg_get_app_msg (msg), "Hey I lost connection, this is my status:....")) {
		printf ("ERROR: expected to find topic name 'Hey I lost connection, this is my status:....' but found: '%s'\n", (const char *) myqtt_msg_get_app_msg (msg));
		return axl_false;
	} /* end if */

	/* release message and queue */
	myqtt_async_queue_unref (queue);
	myqtt_msg_unref (msg);

	/* close connection */
	printf ("Test 08: closing connections..\n");
	myqtt_conn_close (conn);
	myqtt_conn_close (conn2);
	myqtt_conn_close (conn3);

	/* release context */
	printf ("Test 08: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_09 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn, * conn2;
	MyQttMsg        * msg; 
	MyQttConnOpts   * opts;
	int               sub_result;
	MyQttAsyncQueue * queue;
	

	if (! ctx)
		return axl_false;

	printf ("Test 09: creating connections..\n");
	
	/* set will message */
	opts = myqtt_conn_opts_new ();
	
	/* set will */
	myqtt_conn_opts_set_will (opts, MYQTT_QOS_2, "I lost connection", "Hey I lost connection, this is my status:....", axl_false);

	/* now connect to the listener:
	   client_identifier -> NULL
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN but found FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* now create a new connection and subscribe to the will */
	conn2 = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN but found FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	}

	if (! myqtt_conn_sub (conn2, 10, "I lost connection", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	/* set on publish handler */	
	queue  = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn2, test_08_should_not_receive, queue);

	/* close connection */
	myqtt_conn_close (conn);

	/* wait for message */
	printf ("Test 09: we shouldn't receive a will message... (3 seconds at most)\n");
	msg = myqtt_async_queue_timedpop (queue, 3000000);
	if (msg != NULL) {
		printf ("ERROR: expected to NOT receive msg reference but found reference value..\n");
		return axl_false;
	} /* end if */

	/* release message and queue */
	myqtt_async_queue_unref (queue);

	/* close connection */
	printf ("Test 09: closing connections..\n");
	myqtt_conn_close (conn2);


	/* release context */
	printf ("Test 09: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_10 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	int               iterator;
	char            * sub_str;
	int               pos;
	if (! ctx)
		return axl_false;

	printf ("Test 10: checking numbers from file names..\n");
	if (__myqtt_storage_get_size_from_file_name (ctx, "14-16856208-1408293505-274202", &pos) != 14 || pos != 2) {
		printf ("ERROR: failed conversion: %d...\n", __myqtt_storage_get_size_from_file_name (ctx, "14-16856208-1408293505-274202", NULL));
		return axl_false;
	}
	if (__myqtt_storage_get_size_from_file_name (ctx, "1-16856208-1408293505-274202", &pos) != 1 || pos != 1) {
		printf ("ERROR: failed conversion: %d...\n", __myqtt_storage_get_size_from_file_name (ctx, "1-16856208-1408293505-274202", NULL));
		return axl_false;
	}
	if (__myqtt_storage_get_size_from_file_name (ctx, "1432969-16856208-1408293505-274202", &pos) != 1432969 || pos != 7) {
		printf ("ERROR: failed conversion: %d...\n", __myqtt_storage_get_size_from_file_name (ctx, "1432969-16856208-1408293505-274202", NULL));
		return axl_false;
	}

	printf ("Test 10: creating connection to simulate storage on this side\n");

	/* create connection to simulate storage on this side */
	conn = myqtt_conn_new (ctx, "test10@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* configure storage */
	myqtt_storage_set_path (ctx, ".myqtt-test-10", 4096);

	/* init storage for this user */
	if (! myqtt_storage_init (ctx, conn, MYQTT_STORAGE_ALL)) {
		printf ("ERROR: unable to initialize storage for the provided connection..\n");
		return axl_false;
	} /* end if */

	if (system ("find .myqtt-test-10 -type f -exec rm {} \\;"))
		printf ("WARNING: clean subscriptions failed (reported non-zero value)\n");

	/* check if the subscription exists */
	if (myqtt_storage_sub_exists (ctx, conn, "this/is/a/test")) {
		printf ("ERROR (1): expected to not find subscription but it was..\n");
		return axl_false;
	}

	/* check subscription */
	if (! myqtt_storage_sub (ctx, conn, "this/is/a/test", MYQTT_QOS_2)) {
		printf ("ERROR (2): expected to be able to register subscription, but error was found\n");
		return axl_false;
	} /* end if */

	/* check if the subscription exists */
	printf ("Test 10: checking if test/is/a/test is present in storage...\n");
	if (! myqtt_storage_sub_exists (ctx, conn, "this/is/a/test")) {
		printf ("ERROR (3): expected to find subscription but it wasn't..\n");
		return axl_false;
	}

	/* check subscription */
	if (! myqtt_storage_sub (ctx, conn, "this/is/a/test", MYQTT_QOS_1)) {
		printf ("ERROR (4): expected to be able to register subscription, but error was found\n");
		return axl_false;
	} /* end if */

	/* call to unsubscribe */
	if (! myqtt_storage_unsub (ctx, conn, "this/is/a/test")) {
		printf ("ERROR (5): expected to be able to register subscription, but error was found\n");
		return axl_false;
	} /* end if */

	/* check if the subscription exists */
	if (myqtt_storage_sub_exists (ctx, conn, "this/is/a/test")) {
		printf ("ERROR (6): expected to NOT find subscription but it was....\n");
		return axl_false;
	} /* end if */

	/* get the number of subscriptions */
	if (myqtt_storage_sub_count (ctx, conn) != 0) {
		printf ("ERROR (7): expected to receive sub count 0 but found: %d\n", myqtt_storage_sub_count (ctx, conn));
		return axl_false;
	} /* end if */

	/* do several subscriptions */
	iterator = 0;
	while (iterator < 100) {
		sub_str = axl_strdup_printf ("this/is/a/test/%d", iterator);
		myqtt_storage_sub (ctx, conn, sub_str, MYQTT_QOS_1);
		axl_free (sub_str);
		iterator++;
	} /* end while */

	/* get the number of subscriptions */
	if (myqtt_storage_sub_count (ctx, conn) != 100) {
		printf ("ERROR (8): expected to receive sub count 100 but found: %d\n", myqtt_storage_sub_count (ctx, conn));
		return axl_false;
	} /* end if */

	/* check if the subscription exists */
	if (! myqtt_storage_sub_exists (ctx, conn, "this/is/a/test/49")) {
		printf ("ERROR (9): expected to find subscription but it wasn't FOUND....\n");
		return axl_false;
	} /* end if */
	if (! myqtt_storage_sub_exists (ctx, conn, "this/is/a/test/99")) {
		printf ("ERROR (9): expected to find subscription but it wasn't FOUND....\n");
		return axl_false;
	} /* end if */
	if (myqtt_storage_sub_exists (ctx, conn, "this/is/a/test/149")) {
		printf ("ERROR (9): expected NOT to find subscription but it was FOUND....\n");
		return axl_false;
	} /* end if */

	/* close connection */
	printf ("Test 10: closing connections..\n");
	myqtt_conn_close (conn);

	/* create connection to simulate storage on this side */
	conn = myqtt_conn_new (ctx, "test10@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* now open the connection and restore subscriptions */
	printf ("Test 10: recovering connection..\n");
	if (! myqtt_storage_session_recover (ctx, conn)) {
		printf ("ERROR (10): expected to find proper session recovery but found a failure..\n");
		return axl_false;
	} /* end if */

	/* check connection state */
	printf ("Test 10: checking connection recovery..\n");
	if (axl_hash_items (conn->subs) != 100) {
		printf ("Test 10: expected to find 100 subscriptions on connection but found %d\n", axl_hash_items (conn->subs));
		return axl_false;
	}

	/* check connection state */
	printf ("Test 10: Checking context recovery..\n");
	if (axl_hash_items (ctx->subs) != 100) {
		printf ("Test 10: expected to find 100 subscriptions on context but found %d\n", axl_hash_items (ctx->subs));
		return axl_false;
	}
	
	/* close connection */
	printf ("Test 10: closing connection after recovery..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 10: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_11 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttMsg        * msg; 
	int               sub_result;
	MyQttAsyncQueue * queue;
	int               iterator;

	if (! ctx)
		return axl_false;

	printf ("Test 11: creating connections..\n");
	
	/* now connect to the listener:
	   client_identifier -> NULL
	   clean_session -> axl_false ... so we want session
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, NULL, axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN but found FAILURE operation from %s:%s..it shouldn't have worked because we are requesting session and providing NULL identifier...\n", listener_host, listener_port);
		return axl_false;
	} /* end if */
	myqtt_conn_close (conn);

	/* now try again connecting with session but providing a client identifier */
	/* client_identifier -> "test11@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test11@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_sub (conn, 10, "a/subs/1", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_sub (conn, 10, "a/subs/2", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_sub (conn, 10, "a/subs/3", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	/* now close and reconnect and check if we have those subscriptions */
	myqtt_conn_close (conn);

	/* now try again connecting with session but providing a client identifier */
	/* client_identifier -> "test11@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test11@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* set on publish handler */	
	queue  = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	/* send message to get subscriptions */
	if (! myqtt_conn_pub (conn, "myqtt/admin/get-conn-subs", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client subscriptions..\n");
		return axl_false;
	} /* end if */

	/* wait for message */
	printf ("Test 11: we have to receive subscriptions...\n");
	msg = myqtt_async_queue_timedpop (queue, 3000000);
	if (msg == NULL) {
		printf ("ERROR: expected to NOT receive msg reference but found reference value..\n");
		return axl_false;
	} /* end if */

	/* check subscriptions here */
	if (! axl_cmp (myqtt_msg_get_app_msg (msg), "0: a/subs/1\n0: a/subs/2\n0: a/subs/3\n")) {
		printf ("ERROR: expected to find a set of subscriptions that weren't found..\n");
		return axl_false;
	} /* end if */

	/* release message */
	myqtt_msg_unref (msg);

	/* close connection */
	printf ("Test 11: closing connections..\n");
	myqtt_conn_close (conn);

	printf ("Test 11: connect again and check subscriptions are recovered\n");
	conn = myqtt_conn_new (ctx, "test11@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	printf ("Test 11: publishing messages on recovered subscriptions..\n");

	/* publish messages */
	if (! myqtt_conn_pub (conn, "a/subs/1", "this is a test on sub1", 22, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client subscriptions..\n");
		return axl_false;
	} /* end if */

	/* publish messages */
	if (! myqtt_conn_pub (conn, "a/subs/2", "this is a test on sub2", 22, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client subscriptions..\n");
		return axl_false;
	} /* end if */

	/* publish messages */
	if (! myqtt_conn_pub (conn, "a/subs/3", "this is a test on sub3", 22, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client subscriptions..\n");
		return axl_false;
	} /* end if */

	iterator = 0;
	while (iterator < 3) {
		/* get message */
		msg = myqtt_async_queue_timedpop (queue, 3000000);

		printf ("Test 11: message received: %s (id: %d), topic: %s\n", myqtt_msg_get_type_str (msg), myqtt_msg_get_id (msg), myqtt_msg_get_topic (msg));

		/* check for msg 1 */
		if (axl_cmp (myqtt_msg_get_topic (msg), "a/subs/1")) {
			printf ("Test 11: received message 1..checking\n");
			if (! axl_cmp (myqtt_msg_get_app_msg (msg), "this is a test on sub1")) {
				printf ("ERROR: expected message 1..but found: %s\n", (char *) myqtt_msg_get_app_msg (msg));
				return axl_false;
			}
		} /* end if */

		/* check for msg 2 */
		if (axl_cmp (myqtt_msg_get_topic (msg), "a/subs/2")) {
			printf ("Test 11: received message 2..checking\n");
			if (! axl_cmp (myqtt_msg_get_app_msg (msg), "this is a test on sub2")) {
				printf ("ERROR: expected message 2..but found: %s\n", (char *) myqtt_msg_get_app_msg (msg));
				return axl_false;
			}
		} /* end if */

		/* check for msg 3 */
		if (axl_cmp (myqtt_msg_get_topic (msg), "a/subs/3")) {
			printf ("Test 11: received message 3..checking\n");
			if (! axl_cmp (myqtt_msg_get_app_msg (msg), "this is a test on sub3")) {
				printf ("ERROR: expected message 3..but found: %s\n", (char *) myqtt_msg_get_app_msg (msg));
				return axl_false;
			}
		} /* end if */

		myqtt_msg_unref (msg);
		iterator++;
	}

	/* close connection */
	myqtt_conn_close (conn);

	/* release message and queue */
	myqtt_async_queue_unref (queue);

	/* release context */
	printf ("Test 11: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_12 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttMsg        * msg; 
	int               sub_result;
	MyQttAsyncQueue * queue;

	if (! ctx)
		return axl_false;

	printf ("Test 12: creating connections..\n");
	
	/* now try again connecting with session but providing a client identifier */
	/* client_identifier -> "test12@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test12@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_sub (conn, 10, "a/subs/1", MYQTT_QOS_2, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d", sub_result);
		return axl_false;
	} /* end if */

	/* set on publish handler */	
	queue  = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	/* publish QoS 1 messages */
	if (! myqtt_conn_pub (conn, "a/subs/1", "This is a test message", 22, MYQTT_QOS_1, axl_false, 10)) {
		printf ("ERROR: pub message failed..\n");
		return axl_false;
	} /* end if */

	/* get message */
	printf ("Test 12: get QoS 1 message from server (limit wait to 3 seconds)\n");
	msg = myqtt_async_queue_timedpop (queue, 3000000);

	if (msg == NULL) {
		printf ("ERROR: expected to receive a message but NULL reference was found\n");
		return axl_false;
	} /* end if */

	if (! axl_cmp (myqtt_msg_get_app_msg (msg), "This is a test message")) {
		printf ("ERROR: expected to receive different message content but found: '%s'\n", (char *) myqtt_msg_get_app_msg (msg));
		return axl_false;
	} /* end if */

	/* check message quality of service */
	if (myqtt_msg_get_qos (msg) != MYQTT_QOS_1) {
		printf ("ERROR: expected to receive quality of service 1 but found: %d\n", myqtt_msg_get_qos (msg));
		return axl_false;
	} /* end if */

	/* check dup flag */
	if (myqtt_msg_get_dup_flag (msg) != axl_false) {
		printf ("ERROR: expected to find no dup flag fount it was found: %d\n", myqtt_msg_get_dup_flag (msg));
		return axl_false;
	} /* end if */

	/* release message */
	myqtt_msg_unref (msg);

	/* check pkgids on this connection */
	printf ("Test 12: checking local sending packet ids: %d..\n", axl_list_length (conn->sent_pkgids));
	if (axl_list_length (conn->sent_pkgids) != 0) {
		printf ("ERROR: expected to find sent pkgid ids list 0 but found pending %d\n", axl_list_length (conn->sent_pkgids));
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);

	/* release message and queue */
	myqtt_async_queue_unref (queue);

	/* release context */
	printf ("Test 12: releasing context..\n");
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
	printf ("** Available tests: test_00, test_00_a, test_00_b, test_01, test_02, test_03, test_04, test_05\n");
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

	CHECK_TEST("test_00_a")
	run_test (test_00_a, "Test 00-a: test remaining length generation");

	CHECK_TEST("test_00_b")
	run_test (test_00_b, "Test 00-b: test utf-8 check support");

	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: basic listener startup and client connection");

	CHECK_TEST("test_02")
	run_test (test_02, "Test 02: basic subscribe function (QOS 0)");

	CHECK_TEST("test_03")
	run_test (test_03, "Test 03: basic subscribe function (QOS 0) and publish");

	CHECK_TEST("test_04")
	run_test (test_04, "Test 04: basic unsubscribe function (QOS 0)");

	CHECK_TEST("test_05")
	run_test (test_05, "Test 05: test ping server (PINGREQ)");

	CHECK_TEST("test_06")
	run_test (test_06, "Test 06: check client identifier function");

	CHECK_TEST("test_07")
	run_test (test_07, "Test 07: check client auth (CONNECT simple auth)");

	CHECK_TEST("test_08")
	run_test (test_08, "Test 08: test will support (without auth)");

	CHECK_TEST("test_09")
	run_test (test_09, "Test 09: test will is not published with disconnect (without auth)");

	CHECK_TEST("test_10")
	run_test (test_10, "Test 10: test myqtt default storage API");

	CHECK_TEST("test_11")
	run_test (test_11, "Test 11: test sessions maintained by the server");

	CHECK_TEST("test_12")
	run_test (test_12, "Test 12: test QoS 1 messages");

	/* test reception of messages when you are disconnected (with sessions) */

	/* test will support with autentication */

	/* publish empty mesasage */

	/* test close connection after subscribing... */

	/* test reconnect (after closing) */

	/* test sending an unknown message */

	/* test connection lost notification */

	/* test automatic reconnection on connection lost */

	/* test connection lost on subscription sent */

	/* test connection lost on unsubscribe sent */

	/* test connection lost on publish sent with QOS 1 and QOS 2 */

	/* test sending header, and then message */

	/* test sending part of the header, then the rest, then the message */

	/* test sending broken subscribe (not complete) */

	/* test sending a broken unsubscribe (not complete) */

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
