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
#include <stdio.h>

#if defined(ENABLE_TLS_SUPPORT)
#include <myqtt-tls.h>
#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
#include <myqtt-web-socket.h>
#endif

#if defined(ENABLE_MOSQUITTO)
#include <mosquitto.h>
#endif

typedef enum {
	TEST_FILES = 1,
	TEST_DIRS  = 2
} RegTestMyQttCountTypes;

int test_count_in_dir (const char * directory, RegTestMyQttCountTypes  count_type) {
	char * command;
	char   buffer[20];
	FILE * handle;
	int    iterator;

	if (count_type == TEST_FILES)
		command = axl_strdup_printf ("find %s -type f | wc -l > test_count_in_dir.res", directory);
	else if (count_type == TEST_DIRS)
		command = axl_strdup_printf ("find %s -type f | wc -l > test_count_in_dir.res", directory);
	else {
		printf ("ERROR: count type provided is wrong: %d\n", count_type);
		exit (-1);
	} /* end if */

	/* run command */
	if (system (command) != 0) {
		printf ("ERROR: system(\"%s\") command filed\n", command);
		exit (-1);
	} /* end if */

	/* release command */
	axl_free (command);

	/* open result */
	memset (buffer, 0, 20);
	
	handle = fopen ("test_count_in_dir.res", "r");
	if (handle == NULL) {
		printf ("ERROR: failed to open test_count_in_dir.res...NULL pointer received...\n");
		return axl_false;
	} /* end if */

	if (fread (buffer, 1, 20, handle) <= 0) {
		printf ("ERROR: expected to receive content but found 0 or less bytes read..\n");
		return axl_false;
	} /* end if */

	fclose (handle);
	
	iterator = 0;
	while (buffer[iterator] != 0) {
		if (buffer[iterator] == '\n')
			buffer[iterator] = 0;
		iterator++;
	}

	return myqtt_support_strtod (buffer, NULL);
}

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
const char * listener_host     = "localhost";
const char * listener_port     = "1909";
const char * listener_tls_port = "1910";
const char * listener_websocket_port = "1918";
const char * listener_websocket_s_port = "1919";

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

axl_bool test_00_c (void) {

	/* 1 */
	if (system ("rm -rf test-00-c") != 0)
		printf ("Test 00-c: no need to remove old directory..\n");

	if (myqtt_mkdir ("test-00-c/prueba/value/test", 0700)) {
		printf ("ERROR: failed to create directory (1)..\n");
		return axl_false;
	}
	if (! myqtt_support_file_test ("test-00-c/prueba/value/test", FILE_EXISTS | FILE_IS_DIR))
		printf ("ERROR: expected to find a directory (1)..\n");

	/* 2 */
	if (system ("rm -rf test-00-c") != 0)
		printf ("Test 00-c: no need to remove old directory..\n");

	if (myqtt_mkdir ("test-00-c", 0700)) {
		printf ("ERROR: failed to create directory (2)..\n");
		return axl_false;
	}
	if (! myqtt_support_file_test ("test-00-c", FILE_EXISTS | FILE_IS_DIR))
		printf ("ERROR: expected to find a directory (1)..\n");

	/* 3 */
	if (system ("rm -rf test-00-c") != 0)
		printf ("Test 00-c: no need to remove old directory..\n");

	if (myqtt_mkdir ("test-00-c/prueba", 0700)) {
		printf ("ERROR: failed to create directory (3)..\n");
		return axl_false;
	}
	if (! myqtt_support_file_test ("test-00-c/prueba", FILE_EXISTS | FILE_IS_DIR))
		printf ("ERROR: expected to find a directory (3)..\n");

	/* 4 */
	if (myqtt_mkdir ("test-00-c/prueba/value/test", 0700)) {
		printf ("ERROR: failed to create directory (1)..\n");
		return axl_false;
	}
	if (! myqtt_support_file_test ("test-00-c/prueba/value/test", FILE_EXISTS | FILE_IS_DIR))
		printf ("ERROR: expected to find a directory (1)..\n");
	
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
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
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

axl_bool test_02_a (void) {

	MyQttCtx  * ctx = init_ctx ();
	MyQttConn * conn;
	int         sub_result;
	if (! ctx)
		return axl_false;

	printf ("Test 02-a: creating connection..\n");

	/* now connect to the listener:
	   client_identifier -> test_02
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_02", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 02-a: connected without problems..\n");

	/* subscribe to a topic */
	if (! myqtt_conn_sub (conn, 10, "myqtt/+", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* subscribe to a topic */
	if (! myqtt_conn_sub (conn, 10, "test/#", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* close connection */
	printf ("Test 02-a: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 02-a: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);



	return axl_true;
}

void test_03_on_message (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	/* push message received */
	myqtt_msg_ref (msg);
	myqtt_async_queue_push (queue, msg);
	return;
} 

void test_03_fail (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	printf ("ERROR: this handler shouldn't be called because this connection has no subscriptions..\n");
	printf ("ERROR: received message with topic=%s and content=%s\n", myqtt_msg_get_topic (msg), (const char *) myqtt_msg_get_app_msg (msg));
	exit (-1);
	return;
} 

axl_bool test_03_common (int wildcard) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConn       * conn2;
	int               sub_result;
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;
	const char      * label;

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
	label = "myqtt/test";
	if (wildcard == 1)
		label = "myqtt/+";
	else if (wildcard == 2)
		label = "myqtt/#";

	printf ("Test 03: subscribing to %s\n", label);
	if (! myqtt_conn_sub (conn, 10, label, 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* register on message handler */
	queue = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);
	myqtt_conn_set_on_msg (conn2, test_03_fail, NULL);

	/* publish application message (the message sent here is
	 * bigger than 24, this is on purpose) */
	if (! myqtt_conn_pub (conn, "myqtt/test", "This is test message........", 24, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message, myqtt_conn_pub() failed\n");
		return axl_false;
	} /* end if */

	/* waiting for reply */
	printf ("Test 03: waiting for reply..\n");
	msg   = myqtt_async_queue_pop (queue);
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

	myqtt_async_queue_unref (queue);

	/* release context */
	printf ("Test 03: releasing context refs=%d..\n", myqtt_ctx_ref_count (ctx));
	myqtt_exit_ctx (ctx, axl_true);



	return axl_true;
}

axl_bool test_03 (void) {
	/* wildcard = 0 */
	return test_03_common (0);
}

axl_bool test_03a (void) {
	/* wildcard = 1 -> + */
	return test_03_common (1);
}

axl_bool test_03b (void) {
	/* wildcard = 2 -> # */
	return test_03_common (2);
}

axl_bool test_04_common (int wildcard) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	int               sub_result;
	MyQttAsyncQueue * queue;
	MyQttMsg        * msg;
	const char      * sub_topic;
	const char      * test_label;

	if (! ctx)
		return axl_false;

	test_label = "04";
	if (wildcard == 1)
		test_label = "04-a";
	if (wildcard == 2)
		test_label = "04-b";
	

	printf ("Test %s: creating connection..\n", test_label);

	/* now connect to the listener:
	   client_identifier -> test_03
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_03", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test %s: connected without problems..\n", test_label);

	sub_topic = "myqtt/test";
	if (wildcard == 1)
		sub_topic = "myqtt/+";
	if (wildcard == 2)
		sub_topic = "myqtt/#";

	if (! myqtt_conn_sub (conn, 10, sub_topic, 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_unsub (conn, sub_topic, 10)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
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
	printf ("Test %s: waiting for reply (we shouldn't get one, waiting 3 seconds)..\n", test_label);
	queue = myqtt_async_queue_new ();
	msg   = myqtt_async_queue_timedpop (queue, 3000000);
	myqtt_async_queue_unref (queue);
	if (msg != NULL) {
		printf ("ERROR: expected to find message from queue, but NULL was found..\n");
		return axl_false;
	} /* end if */

	/* close connection */
	printf ("Test %s: closing connection..\n", test_label);
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test %s: releasing context..\n", test_label);
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_04 (void) {
	return test_04_common (0);
}

axl_bool test_04a (void) {
	return test_04_common (1);
}

axl_bool test_04b (void) {
	return test_04_common (2);
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
	   client_identifier -> test_06.identifier
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

void test_08_should_not_receive (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
{
	printf ("ERROR: received a message on a handler that shouldn't have been called\n");
	return;
}

void test_08_should_receive (MyQttCtx * ctx, MyQttConn * conn, MyQttMsg * msg, axlPointer user_data)
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
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
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
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
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
	
	int               packet_id;
	int               qos;
	int               size;


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

	__myqtt_storage_get_values_from_file_name (ctx, "10-47-2-1438039660-798316", &packet_id, &size, &qos);
	printf ("Test 10: values received from test packet_id=%d, size=%d, qos=%d\n",
		packet_id, size, qos);
	if (packet_id != 10 || size != 47 || qos != 2) {
		printf ("ERROR: expected diffferent values for packet_id or size or qos (1)\n");
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

	/* call to load local storage first (before an incoming connection) */
	myqtt_storage_load (ctx);

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
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_sub (conn, 10, "a/subs/2", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_sub (conn, 10, "a/subs/3", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
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

axl_bool test_12_common (int wildcard) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttMsg        * msg; 
	int               sub_result;
	MyQttAsyncQueue * queue;
	const char      * sub_topic;
	const char      * test_label = "12";

	if (! ctx)
		return axl_false;

	if (wildcard == 0)
		test_label = "12";
	else if (wildcard == 1)
		test_label = "12-a";
	else if (wildcard == 2)
		test_label = "12-b";

	printf ("Test %s: creating connections..\n", test_label);
	
	/* now try again connecting with session but providing a client identifier */
	/* client_identifier -> "test12@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test12@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	sub_topic = "a/subs/1";
	if (wildcard == 1)
		sub_topic = "a/subs/+";
	if (wildcard == 2)
		sub_topic = "a/subs/#";
	
	printf ("Test %s: subscribing to topic: %s\n", test_label, sub_topic);
	if (! myqtt_conn_sub (conn, 10, sub_topic, MYQTT_QOS_2, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
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
	printf ("Test %s: get QoS 1 message from server (limit wait to 3 seconds)\n", test_label);
	msg = myqtt_async_queue_timedpop (queue, 3000000);

	if (msg == NULL) {
		printf ("ERROR: expected to receive a message but NULL reference was found\n");
		return axl_false;
	} /* end if */
	
#define test_12_check_msg "This is a test message"

	if (! axl_cmp (myqtt_msg_get_app_msg (msg), test_12_check_msg)) {
		printf ("ERROR (12.1): expected to receive different message content ('%s') but found: '%s'\n", 
			test_12_check_msg,
			(char *) myqtt_msg_get_app_msg (msg));
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
	printf ("Test %s: checking local sending packet ids: %d..\n", test_label, axl_list_length (conn->sent_pkgids));
	if (axl_list_length (conn->sent_pkgids) != 0) {
		printf ("ERROR: expected to find sent pkgid ids list 0 but found pending %d\n", axl_list_length (conn->sent_pkgids));
		return axl_false;
	} /* end if */


	/* unsubscribe */
	if (! myqtt_conn_unsub (conn, sub_topic, 10)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);

	/* release message and queue */
	myqtt_async_queue_unref (queue);

	/* release context */
	printf ("Test %s: releasing context..\n", test_label);
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_12 (void){
	return test_12_common (0);
}

axl_bool test_12a (void){
	return test_12_common (1);
}

axl_bool test_12b (void){
	return test_12_common (2);
}

axl_bool test_13_common (int wildcard) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttMsg        * msg; 
	int               sub_result;
	MyQttAsyncQueue * queue;
	
	const char      * sub_topic;
	const char      * test_label;

	sub_topic = "a/subs/1";
	test_label = "13";
	if (wildcard == 1) {
		sub_topic = "a/subs/+";
		test_label = "13-a";
	}
	if (wildcard == 2) {
		sub_topic = "a/subs/#";
		test_label = "13-b";
	}
		

	if (! ctx)
		return axl_false;

	printf ("Test %s: creating connections..\n", test_label);
	
	/* now try again connecting with session but providing a client identifier */
	/* client_identifier -> "test13@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test13@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected FAILURE but found LOGIN operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_sub (conn, 10, sub_topic, MYQTT_QOS_2, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* set on publish handler */	
	queue  = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	/* publish QoS 1 messages */
	if (! myqtt_conn_pub (conn, "a/subs/1", "This is a test message", 22, MYQTT_QOS_2, axl_false, 10)) {
		printf ("ERROR: pub message failed..\n");
		return axl_false;
	} /* end if */

	/* get message */
	printf ("Test %s: get QoS 2 message from server (limit wait to 3 seconds)\n", test_label);
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
	if (myqtt_msg_get_qos (msg) != MYQTT_QOS_2) {
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
	printf ("Test %s: checking local sending packet ids: %d..\n", test_label, axl_list_length (conn->sent_pkgids));
	if (axl_list_length (conn->sent_pkgids) != 0) {
		printf ("ERROR: expected to find sent pkgid ids list 0 but found pending %d\n", axl_list_length (conn->sent_pkgids));
		return axl_false;
	} /* end if */

	/* unsubscribe */
	if (! myqtt_conn_unsub (conn, sub_topic, 10)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);

	/* release message and queue */
	myqtt_async_queue_unref (queue);

	/* release context */
	printf ("Test %s: releasing context..\n", test_label);
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_13 (void) {
	return test_13_common (0);
}

axl_bool test_13a (void) {
	return test_13_common (1);
}

axl_bool test_13b (void) {
	return test_13_common (2);
}

axl_bool test_14_common (int wildcard) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConn       * conn2;
	MyQttMsg        * msg; 
	int               iterator;
	int               sub_result;
	MyQttAsyncQueue * queue; 
	int               value;
	const char      * msg_str;
	
	const char      * sub_topic = NULL;
	const char      * test_label = "14";

	if (wildcard == 1) {
		sub_topic = "test/message/+";
		test_label = "14-a";
	}
	if (wildcard == 2) {
		sub_topic = "test/message/#";
		test_label = "14-b";
	}


	if (! ctx)
		return axl_false;

	printf ("Test %s: queueing 3 messages (offline PUB)..\n", test_label);
	if (system ("find .myqtt-regression-client/test14@identifier.com/msgs/ -type f -exec rm {} \\;") != 0)
		printf ("WARNING: failed to clean messages from local storage..\n");

	if (! myqtt_conn_offline_pub (ctx, "test14@identifier.com", "test/message/1", "This is a test message 1 for offline publication..", 50, MYQTT_QOS_0, axl_false)) {
		printf ("ERROR: failed to publish offline message..\n");
		return axl_false;
	} /* end if */

	if (! myqtt_conn_offline_pub (ctx, "test14@identifier.com", "test/message/2", "This is a test message 2 for offline publication..", 50, MYQTT_QOS_0, axl_false)) {
		printf ("ERROR: failed to publish offline message..\n");
		return axl_false;
	} /* end if */

	if (! myqtt_conn_offline_pub (ctx, "test14@identifier.com", "test/message/3", "This is a test message 3 for offline publication..", 50, MYQTT_QOS_0, axl_false)) {
		printf ("ERROR: failed to publish offline message..\n");
		return axl_false;
	} /* end if */

	/* get number of queued messages */
	if (myqtt_storage_queued_messages_offline (ctx, "test14@identifier.com") != 3) {
		printf ("ERROR: expected to find 3 queued messages (waiting to be sent on next connection..) but found: %d..\n", 
			myqtt_storage_queued_messages_offline (ctx, "test14@identifier.com"));
		return axl_false;
	}

	/* get quota used */
	value = myqtt_storage_queued_messages_quota_offline (ctx, "test14@identifier.com");
	if (value != 204) {
		printf ("ERROR: expected to find different quota size, but found: %d\n", value);
		return axl_false;
	} /* end if */

	/* now connect with a different client id and subscribe to previous messages */
	/* client_identifier -> "test13@identifier.com", clean_session -> axl_false */
	printf ("Test %s: now connect with a second connection that expects to receive those queued messages for the other identifier..\n", test_label);
	conn = myqtt_conn_new (ctx, "test14-2@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN  but found LOGIN FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* subscribe to the topics referenced before */
	printf ("Test %s: subscribing to the topics (%s)..\n", test_label, sub_topic ? sub_topic : "test/message/1,2,3");
	switch (wildcard) {
	case 0:
		if (! myqtt_conn_sub (conn, 10, "test/message/1", MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		
		if (! myqtt_conn_sub (conn, 10, "test/message/2", MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		
		if (! myqtt_conn_sub (conn, 10, "test/message/3", MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		break;
	case 1:
	case 2:
		if (! myqtt_conn_sub (conn, 10, sub_topic, MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		break;
	}
		

	/* set on publish handler */	
	queue  = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	/* now connect with the client id with queued messages */
	printf ("Test %s: now connect with the initial connection that has queued messages..\n", test_label);
	conn2 = myqtt_conn_new (ctx, "test14@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: expected LOGIN  but found LOGIN FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* check we receive those messages that were queued */ 
	iterator = 0;
	while (iterator < 3) {
		/* next message on queue */
		printf ("Test %s: waiting for messages to arrive (at most 3 seconds waiting before failing..\n", test_label);
		msg = myqtt_async_queue_timedpop (queue, 3000000);
		if (msg == NULL || myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
			printf ("ERROR: received NULL message or wrong type..\n");
			return axl_false;
		} /* end if */

		/* more checks here */
		if (axl_cmp (myqtt_msg_get_topic (msg), "test/message/1")) {
			msg_str = "This is a test message 1 for offline publication..";
			if (! axl_cmp (myqtt_msg_get_app_msg (msg), msg_str)) {
				printf ("ERROR: 14.1, expected different test but found (1): %s\n", (char *) myqtt_msg_get_app_msg (msg));
				printf ("       message expected: %s\n", msg_str);
				return axl_false;
			} /* end if */
		} else if (axl_cmp (myqtt_msg_get_topic (msg), "test/message/2")) {
			msg_str = "This is a test message 2 for offline publication..";
			if (! axl_cmp (myqtt_msg_get_app_msg (msg), msg_str)) {
				printf ("ERROR: 14.2, expected different test but found (2): %s\n", (char *) myqtt_msg_get_app_msg (msg));
				printf ("       message expected: %s\n", msg_str);
				return axl_false;
			} /* end if */
		} else if (axl_cmp (myqtt_msg_get_topic (msg), "test/message/3")) {
			msg_str = "This is a test message 3 for offline publication..";
			if (! axl_cmp (myqtt_msg_get_app_msg (msg), msg_str)) {
				printf ("ERROR: 14.3, expected different test but found (3): %s\n", (char *) myqtt_msg_get_app_msg (msg));
				printf ("       message expected: %s\n", msg_str);
				return axl_false;
			} /* end if */
		} else {
			printf ("ERROR: expected different content at the topic but found: %s\n", myqtt_msg_get_topic (msg));
			return axl_false;
		} /* end if */

		if (myqtt_msg_get_app_msg_size (msg) != 50) {
			printf ("ERROR: expected different content size but found: %d\n", myqtt_msg_get_app_msg_size (msg));
			return axl_false;
		} /* end if */

		/* release message */
		myqtt_msg_unref (msg);

		/* next iterator */
		iterator++;
	} /* end while */

	/* now check pending messages for test14@identifier.com */
	if (myqtt_storage_queued_messages (ctx, conn) != 0) {
		printf ("ERROR: expected to not find any queued message but found: %d\n", myqtt_storage_queued_messages (ctx, conn));
		return axl_false;
	} /* end if */

	/* close both connections */
	myqtt_conn_close (conn2);



	switch (wildcard) {
	case 0:
		/* unsubscribe */
		if (! myqtt_conn_unsub (conn, "test/message/1", 10)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */

		if (! myqtt_conn_unsub (conn, "test/message/2", 10)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */

		if (! myqtt_conn_unsub (conn, "test/message/3", 10)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */



		break;
	case 1:
	case 2:
		/* unsubscribe */
		if (! myqtt_conn_unsub (conn, sub_topic, 10)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		break;
	}

	myqtt_conn_close (conn);

	/* release queue */
	myqtt_async_queue_unref (queue);
	
	/* release context */
	printf ("Test %s: releasing context..\n", test_label);
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_14 (void) {
	return test_14_common (0);
}

axl_bool test_14b (void) {
	return test_14_common (1);
}

axl_bool test_14c (void) {
	return test_14_common (2);
}

axl_bool test_14a_subs (MyQttCtx * ctx, const char * client_id)
{
	/* store some subscriptions */
	if (! myqtt_storage_sub_offline (ctx, client_id, "test/sub/1", MYQTT_QOS_0))
		return axl_false;
	if (! myqtt_storage_sub_offline (ctx, client_id, "test/sub/2", MYQTT_QOS_1))
		return axl_false;
	if (! myqtt_storage_sub_offline (ctx, client_id, "test/sub/3", MYQTT_QOS_2))
		return axl_false;

	return axl_true; /* return ok */
}

axl_bool test_14a (void) {
	MyQttCtx        * ctx = init_ctx ();
	int               entries;
	axlHashCursor   * cursor;

	/* configure path */
	myqtt_storage_set_path (ctx, ".myqtt-listener-test14a", 4096);

	/* init storage for 5 clients */
	myqtt_storage_init_offline (ctx, "test14aclient1", MYQTT_STORAGE_ALL);
	myqtt_storage_init_offline (ctx, "test14aclient2", MYQTT_STORAGE_ALL);
	myqtt_storage_init_offline (ctx, "test14aclient3", MYQTT_STORAGE_ALL);
	myqtt_storage_init_offline (ctx, "test14aclient4", MYQTT_STORAGE_ALL);
	myqtt_storage_init_offline (ctx, "test14aclient5", MYQTT_STORAGE_ALL);

	if (! test_14a_subs (ctx, "test14aclient1"))
		return axl_false;
	if (! test_14a_subs (ctx, "test14aclient2"))
		return axl_false;
	if (! test_14a_subs (ctx, "test14aclient3"))
		return axl_false;
	if (! test_14a_subs (ctx, "test14aclient4"))
		return axl_false;
	if (! test_14a_subs (ctx, "test14aclient5"))
		return axl_false;

	/* load storage */
	entries = myqtt_storage_load (ctx);
	printf ("Test 14a: loaded %d subscriptions..\n", entries);
	if (entries != 15) {
		printf ("ERROR: expected to find 15 subscriptions but found %d\n", entries);
		return axl_false;
	} /* end if */

	if (axl_hash_items (ctx->offline_subs) != 3) {
		printf ("ERROR: expected to find 3 subscriptions inside offline subs..\n");
		return axl_false;
	}

	if (axl_hash_items (ctx->subs) != 0) {
		printf ("ERROR: expected to find 0 subscriptions inside online subs..\n");
		return axl_false;
	}

	/* iterarate hashes */
	cursor = axl_hash_cursor_new (ctx->offline_subs);
	while (axl_hash_cursor_has_item (cursor)) {

		if (axl_cmp (axl_hash_cursor_get_key (cursor), "test/sub/1")) {
			if (axl_hash_items (axl_hash_cursor_get_value (cursor)) != 5) {
				printf ("Expected to find 5 items inside hash directed by subscription: %s but found (%d)\n", 
					(char *) axl_hash_cursor_get_key (cursor), axl_hash_items (axl_hash_cursor_get_value (cursor)));
				return axl_false;
			}
		} else if (axl_cmp (axl_hash_cursor_get_key (cursor), "test/sub/2")) {
			if (axl_hash_items (axl_hash_cursor_get_value (cursor)) != 5) {
				printf ("Expected to find 5 items inside hash directed by subscription: %s but found (%d)\n", 
					(char *) axl_hash_cursor_get_key (cursor), axl_hash_items (axl_hash_cursor_get_value (cursor)));
				return axl_false;
			}
			
		} else if (axl_cmp (axl_hash_cursor_get_key (cursor), "test/sub/3")) {
			if (axl_hash_items (axl_hash_cursor_get_value (cursor)) != 5) {
				printf ("Expected to find 5 items inside hash directed by subscription: %s but found (%d)\n", 
					(char *) axl_hash_cursor_get_key (cursor), axl_hash_items (axl_hash_cursor_get_value (cursor)));
				return axl_false;
			}
			
		} else {
			printf ("ERROR: unexpected subscription found: %s\n", (char *) axl_hash_cursor_get_key (cursor));
			return axl_false;
		}


		/* next hash cursor */
		axl_hash_cursor_next (cursor);
	} /* end if */
	axl_hash_cursor_free (cursor);

	/* release context */
	printf ("Test 14a: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);
	
	return axl_true;
}
axl_bool __test_15_check (MyQttMsg * msg, int * count_qos0, int * count_qos1, int * count_qos2)
{
	if (msg == NULL || myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
		printf ("ERROR: empty reference received or wrong type: msg=%p (%s)..\n",
			msg, msg ? myqtt_msg_get_type_str (msg) : "UNKNOWN");
		return axl_false;
	}

	if (myqtt_msg_get_qos (msg) == MYQTT_QOS_0)
		(*count_qos0)++;
	if (myqtt_msg_get_qos (msg) == MYQTT_QOS_1)
		(*count_qos1)++;
	if (myqtt_msg_get_qos (msg) == MYQTT_QOS_2)
		(*count_qos2)++;

	/* more checks here */
	printf ("Test 15: received msg with topic: %s\n", myqtt_msg_get_topic (msg));

	/* release message */
	myqtt_msg_unref (msg);
	return axl_true;
}


axl_bool test_15_common (int wildcard) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttMsg        * msg; 
	int               iterator;
	int               sub_result;
	MyQttAsyncQueue * queue; 
	char            * ref;
	int               count_qos0 = 0, count_qos1 = 0, count_qos2 = 0;
	const char      * sub_topic  = NULL;
	const char      * test_label = "15";

	if (! ctx)
		return axl_false;

	switch (wildcard) {
	case 1:
		sub_topic  = "test/message/+";
		test_label = "15-a";
		break;
	case 2:
		sub_topic  = "test/message/#";
		test_label = "15-b";
		break;
	}
	

	printf ("Test %s: connecting with session, subscribe and disconnect..\n", test_label);

	/* client_identifier -> "test15@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test15@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN  but found LOGIN FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* subscribe to the topics referenced before */
	switch (wildcard) {
	case 0:
		printf ("Test %s: subscribing to the topics..\n", test_label);
		if (! myqtt_conn_sub (conn, 10, "test/message/1", MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		
		if (! myqtt_conn_sub (conn, 10, "test/message/2", MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		
		if (! myqtt_conn_sub (conn, 10, "test/message/3", MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		break;
	case 1:
	case 2:
		printf ("Test %s: subscribing to the topic %s..\n", test_label, sub_topic);
		if (! myqtt_conn_sub (conn, 10, sub_topic, MYQTT_QOS_2, &sub_result)) {
			printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
			return axl_false;
		} /* end if */
		break;
	}

	printf ("Test %s: close connection..\n", test_label);
	myqtt_conn_close (conn);

	printf ("Test %s: connect again..\n", test_label);

	/* now connect with a different connection and send messages
	 * to previous topics */
	conn = myqtt_conn_new (ctx, NULL, axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN but found LOGIN FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test %s: publishing messages..\n", test_label);
	/* publish messages */
	iterator = 0;
	while (iterator < 10) {

		/* next message on queue */
		ref = axl_strdup_printf ("This is a test message: %d..", iterator);
		if (! myqtt_conn_pub (conn, "test/message/1", ref, strlen (ref), MYQTT_QOS_0, axl_false, 10)) {
			printf ("ERROR: unable to publish messages on qos 0..\n");
			return axl_false;
		} /* end if */
		axl_free (ref);

		ref = axl_strdup_printf ("This is a test message: %d..", iterator);
		if (! myqtt_conn_pub (conn, "test/message/2", ref, strlen (ref), MYQTT_QOS_1, axl_false, 10)) {
			printf ("ERROR: unable to publish messages on qos 1..\n");
			return axl_false;
		} /* end if */
		axl_free (ref);

		ref = axl_strdup_printf ("This is a test message: %d..", iterator);
		if (! myqtt_conn_pub (conn, "test/message/3", ref, strlen (ref), MYQTT_QOS_2, axl_false, 10)) {
			printf ("ERROR: unable to publish messages on qos 2..\n");
			return axl_false;
		} /* end if */
		axl_free (ref);

		/* next iterator */
		iterator++;
	} /* end while */

	/* set on publish handler */	
	printf ("Test %s: getting queued messages..\n", test_label);
	queue  = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	/* send message to get subscriptions */
	if (! myqtt_conn_pub (conn, "myqtt/admin/get-queued-msgs", "test15@identifier.com", 21, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client subscriptions..\n");
		return axl_false;
	} /* end if */

	/* get message */
	msg = myqtt_async_queue_timedpop (queue, 3000000);
	if (msg == NULL || myqtt_msg_get_type (msg) != MYQTT_PUBLISH) {
		printf ("ERROR: expected publish message but found null or wrong type..\n");
		return axl_false;
	}

	printf ("Test %s: stored messages for test15@identifier.com are: %s\n", test_label, (const char *) myqtt_msg_get_app_msg (msg));
	if (! axl_cmp ("30", (const char *) myqtt_msg_get_app_msg (msg))) {
		printf ("ERROR: expected to find 30 messages waiting but found: %s\n", (const char *) myqtt_msg_get_app_msg (msg));
		return axl_false;
	} /* end if */

	myqtt_msg_unref (msg);

	/* close connection */
	myqtt_conn_close (conn);

	printf ("Test %s: reconnecting again with initial connection (2 seconds simulated wait)..\n", test_label);
	myqtt_async_queue_timedpop (queue, 2000000);

	/* now seting on message received */
	/* set on publish handler */	
	myqtt_ctx_set_on_msg (ctx, test_03_on_message, queue);

	/* now connect again with the initial client identifier */
	/* client_identifier -> "test15@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test15@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN  but found LOGIN FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	iterator = 0;
	while (iterator < 10) {
		/* get first message */
		msg = myqtt_async_queue_timedpop (queue, 3000000);
		if (! __test_15_check (msg, &count_qos0, &count_qos1, &count_qos2))
			return axl_false;

		/* get first message */
		msg = myqtt_async_queue_timedpop (queue, 3000000);
		if (! __test_15_check (msg, &count_qos0, &count_qos1, &count_qos2))
			return axl_false;

		/* get first message */
		msg = myqtt_async_queue_timedpop (queue, 3000000);
		if (! __test_15_check (msg, &count_qos0, &count_qos1, &count_qos2))
			return axl_false;

		/* next position */
		iterator++;
	} /* end if */

	if ((count_qos2 + count_qos1 + count_qos0) != 30) {
		printf ("ERROR: expected to find a total count of messages received of 30 but found something different..\n");
		return axl_false;
	} /* end if */

	if (count_qos0 != 10) {
		printf ("ERROR: expected to receive 10 messages with QoS 0 but received a different count..\n");
		return axl_false;
	}

	if (count_qos1 != 10) {
		printf ("ERROR: expected to receive 10 messages with QoS 1 but received a different count..\n");
		return axl_false;
	}

	if (count_qos2 != 10) {
		printf ("ERROR: expected to receive 10 messages with QoS 2 but received a different count..\n");
		return axl_false;
	}

	printf ("Test %s: all messages received OK, now closing connection..\n", test_label);

	/* close connection */
	myqtt_conn_close (conn);

	printf ("Test %s: connection closed, now connect again to check we don't receive any message..\n", test_label);

	/* client_identifier -> "test15@identifier.com", clean_session -> axl_false */
	conn = myqtt_conn_new (ctx, "test15@identifier.com", axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected LOGIN  but found LOGIN FAILURE operation from %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* ensure we don't get more messages because we didn't publish anything new */
	printf ("Test %s: checking no more messages are published (3 seconds)..\n", test_label);
	msg = myqtt_async_queue_timedpop (queue, 3000000);
	if (msg != NULL) {
		printf ("ERROR (15.1): we shouldn't have received any message but we did!\nMessage received, topic='%s'\nmsg='%s'\nsize='%d'\nqos='%d'\n",
			myqtt_msg_get_topic (msg),
			(const char *) myqtt_msg_get_app_msg (msg),
			myqtt_msg_get_app_msg_size (msg),
			myqtt_msg_get_qos (msg));
		return axl_false;
	} /* end if */

	/* subscribe to the topics referenced before */
	switch (wildcard) {
	case 0:
		if (! myqtt_conn_unsub (conn, "test/message/1", 10)) {
			printf ("ERROR: unable to unsubscribe, myqtt_conn_unsub () failed\n");
			return axl_false;
		} /* end if */
		
		if (! myqtt_conn_unsub (conn, "test/message/2", 10)) {
			printf ("ERROR: unable to unsubscribe, myqtt_conn_unsub () failed\n");
			return axl_false;
		} /* end if */
		
		if (! myqtt_conn_unsub (conn, "test/message/3", 10)) {
			printf ("ERROR: unable to unsubscribe, myqtt_conn_unsub () failed\n");
			return axl_false;
		} /* end if */
		break;
	case 1:
	case 2:
		if (! myqtt_conn_unsub (conn, sub_topic, 10)) {
			printf ("ERROR: unable to unsubscribe, myqtt_conn_unsub () failed\n");
			return axl_false;
		} /* end if */
		break;
	}

	/* close connection */
	myqtt_conn_close (conn);

	/* release queue */
	myqtt_async_queue_unref (queue);
	
	/* release context */
	printf ("Test %s: releasing context..\n", test_label);
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_15 (void) {
	return test_15_common (0);
}

axl_bool test_15a (void) {
	return test_15_common (1);
}

axl_bool test_15b (void) {
	return test_15_common (2);
}

axl_bool test_16 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttQos          qos;
	unsigned char   * app_msg;
	int               app_size;
	axlList         * list;
	axlListCursor   * cursor;
	const char      * topic;

	if (! ctx)
		return axl_false;

	printf ("Test 16: checking message retention\n");

	/* set context path */
	myqtt_storage_set_path (ctx, "myqtt-test-16", 128);
	if (myqtt_support_file_test ("myqtt-test-16", FILE_EXISTS)) {
		if (system ("find myqtt-test-16 -type f -exec rm {} \\;") != 0) {
			printf ("ERROR: expected to cleanup myqtt-test-16 directory..but system() call failed..\n");
			return axl_false;
		} /* end if */
	} /* end if */

	/* init storage */
	myqtt_storage_init_offline (ctx, "test16@myqtt", MYQTT_STORAGE_ALL);

	/* store a message but several times to ensure it remains the last */
	if (! myqtt_storage_retain_msg_set (ctx, "this/is/a/test", MYQTT_QOS_2, (axlPointer) "This is a application message test that will act as retained message..", 70)) {
		printf ("ERROR (1): failed to queue retained message..\n");
		return axl_false;
	} /* end if */

	if (! myqtt_storage_retain_msg_set (ctx, "this/is/a/test", MYQTT_QOS_1, (axlPointer) "This is a application message test that will act as retained message (2)..", 74)) {
		printf ("ERROR (2): failed to queue retained message..\n");
		return axl_false;
	} /* end if */

	if (! myqtt_storage_retain_msg_set (ctx, "this/is/a/test", MYQTT_QOS_2, (axlPointer) "This is a application message test that will act as retained message (3)..", 74)) {
		printf ("ERROR (3): failed to queue retained message..\n");
		return axl_false;
	} /* end if */

	/* count number of files in test directory */
	if (test_count_in_dir ("myqtt-test-16", TEST_FILES) != 2) {
		printf ("ERROR (4): expected to find %d files but found: %d\n", 2, test_count_in_dir ("myqtt-test-16", TEST_FILES));
		return axl_false;
	} /* end if */

	/* save a different message */
	if (! myqtt_storage_retain_msg_set (ctx, "this/is/a/test/b", MYQTT_QOS_2, (axlPointer) "This is a application message test that will act as retained message (4)..", 74)) {
		printf ("ERROR (5): failed to queue retained message..\n");
		return axl_false;
	} /* end if */

	/* get the list of retained topics */
	list = myqtt_storage_get_retained_topics (ctx, "#");
	if (list == NULL || axl_list_length (list) == 0) {
		printf ("ERROR (5.1): expected to find a list of retained messages but found NULL or empty..\n");
		return axl_false;
	} /* end if */

	if (axl_list_length (list) != 2) {
		printf ("ERROR (5.2): expected to find 2 elements into the list but found: %d\n", axl_list_length (list));
		return axl_false;
	}

	/* create the cursor */
	cursor = axl_list_cursor_new (list);
	while (axl_list_cursor_has_item (cursor)) {
		/* get the topic */
		topic = (const char *) axl_list_cursor_get (cursor);
		printf ("Test --: retained topic found: %s\n", topic);
		if (! axl_cmp (topic, "this/is/a/test") &&
		    ! axl_cmp (topic, "this/is/a/test/b")) {
			printf ("ERROR: found topic that is not expected (tgk34): %s\n", topic);
			return axl_false;
		} /* end if */

		/* next position */
		axl_list_cursor_next (cursor);
	}
	axl_list_cursor_free (cursor);
	axl_list_free (list);

	/* count number of files in test directory */
	if (test_count_in_dir ("myqtt-test-16", TEST_FILES) != 4) {
		printf ("ERROR (6): expected to find %d files but found: %d\n", 4, test_count_in_dir ("myqtt-test-16", TEST_FILES));
		return axl_false;
	} /* end if */

	/* now release a retained message that is not found now */
	myqtt_storage_retain_msg_release (ctx, "test/value/different");

	/* count number of files in test directory */
	if (test_count_in_dir ("myqtt-test-16", TEST_FILES) != 4) {
		printf ("ERROR (7): expected to find %d files but found: %d\n", 4, test_count_in_dir ("myqtt-test-16", TEST_FILES));
		return axl_false;
	} /* end if */

	/* now recover retained messages */
	printf ("Test 16: calling to recover retained message for subscription: this/is/a/test/b\n");
	if (! myqtt_storage_retain_msg_recover (ctx, "this/is/a/test/b", &qos, &app_msg, &app_size)) {
		printf ("ERROR (8): failed to recover message that should be there..\n");
		return axl_false;
 	} /* end if */
	
	if (app_size != 74) {
		printf ("ERROR (9): expected to find 74 as size...but found: %d\n", app_size);
		return axl_false;
	} /* end if */

	if (! axl_cmp ((const char *) app_msg, "This is a application message test that will act as retained message (4)..")) {
		printf ("ERROR (10): expected different content but found: '%s'\n", app_msg);
		return axl_false;
	} /* end if */

	if (qos != MYQTT_QOS_2) {
		printf ("ERROR (11): expected to find qos 2 but found: %d\n", qos);
		return axl_false;
	} /* end if */

	axl_free (app_msg);

	printf ("Test 16: testing message release from storage..\n");
	/* now release a retained message that is not found now */
	myqtt_storage_retain_msg_release (ctx, "this/is/a/test/b");

	/* count number of files in test directory */
	if (test_count_in_dir ("myqtt-test-16", TEST_FILES) != 2) {
		printf ("ERROR (12): expected to find %d files but found: %d\n", 2, test_count_in_dir ("myqtt-test-16", TEST_FILES));
		return axl_false;
	} /* end if */

	/* now release a retained message that is not found now */
	printf ("Test 16: testing message release from storage (2)..\n");
	myqtt_storage_retain_msg_release (ctx, "this/is/a/test");

	/* count number of files in test directory */
	if (test_count_in_dir ("myqtt-test-16", TEST_FILES) != 0) {
		printf ("ERROR (13): expected to find %d files but found: %d\n", 0, test_count_in_dir ("myqtt-test-16", TEST_FILES));
		return axl_false;
	} /* end if */

	/* now recover retained messages */
	printf ("Test 16: calling to recover retained message for subscription: this/is/a/test/b\n");
	if (myqtt_storage_retain_msg_recover (ctx, "this/is/a/test/b", &qos, &app_msg, &app_size)) {
		printf ("ERROR (14): we shouldn't have received a message recovered but we did!..\n");
		return axl_false;
 	} /* end if */

	/* release context */
	printf ("Test 16: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_17_publish_and_check (MyQttConn * conn, MyQttAsyncQueue * queue, axl_bool should_be_received, int wildcard, const char * sub_topic, const char * test_label)
{
	int        sub_result;
	MyQttMsg * msg;
	

	if (! myqtt_conn_sub (conn, 14, sub_topic, 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* get message (ensure we don't receive anything) */
	printf ("Test %s: now wait for message (for 3 seconds)\n", test_label);
	msg   = myqtt_async_queue_timedpop (queue, 3000000);

	/* it should not be received */
	if (! should_be_received && msg == NULL)
		return axl_true;

	/* it should be received */
	if (msg == NULL) {
		printf ("ERROR: we should  have received a message after subscription (should_be_received=%d)...BUT null was received..\n", should_be_received);
		return axl_false;
	} /* end if */

	/* check message received */
	if (! axl_cmp (myqtt_msg_get_topic (msg), "this/is/a/test")) {
		printf ("ERROR (1): expected to find a different topic but found: %s\n", myqtt_msg_get_topic (msg));
		return axl_false;
	} /* end if */

	if (! axl_cmp (myqtt_msg_get_app_msg (msg), "This is a retained message for future subscribers..")) {
		printf ("ERROR (2): expected to find a different app msg but found: %s\n", (const char *) myqtt_msg_get_app_msg (msg));
		return axl_false;
	} /* end if */

	if (myqtt_msg_get_app_msg_size (msg) != 51) {
		printf ("ERROR (3): expected to receive 51 as app msg size but found: %d\n", myqtt_msg_get_app_msg_size (msg));
		return axl_false;
	} /* end if */

	if (! myqtt_conn_unsub (conn, sub_topic, 14)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_unsub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* release message */
	myqtt_msg_unref (msg);

	return axl_true;
}

axl_bool test_17_common (int wildcard) {

	MyQttCtx        * ctx = init_ctx ();

	MyQttAsyncQueue * queue;
	int               sub_result;
	MyQttMsg        * msg;
	MyQttConn       * conn;

	const char      * sub_topic  = NULL;
	const char      * test_label = NULL;

	switch (wildcard) {
	case 0:
		sub_topic  = "this/is/a/test";
		test_label = "17";
		break;
	case 1:
		sub_topic  = "this/is/a/+";
		test_label = "17-a";
		break;
	case 2:
		sub_topic  = "this/is/a/#";
		test_label = "17-b";
		break;
	}

	if (! ctx)
		return axl_false;

	printf ("Test %s: checking message retention\n", test_label);

	/* do a simple connection */
	conn = myqtt_conn_new (ctx, NULL, axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test %s: connection created..\n", test_label);

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test", "This is a retained message for future subscribers..", 51, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	printf ("Test %s: message published..\n", test_label);

	/* close connection */
	myqtt_conn_close (conn);

	/* now connect again and subscribe to a different topic */
	queue = myqtt_async_queue_new ();

	/* do a simple connection */
	conn = myqtt_conn_new (ctx, NULL, axl_false, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* set on message received */
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	printf ("Test %s: sending new subscription..\n", test_label);

	/* subscribe to a topic without retained message */
	if (! myqtt_conn_sub (conn, 10, "myqtt/test", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* get message (ensure we don't receive anything) */
	printf ("Test %s: ensuring we don't get a message as a consequence of this subscription (3 seconds)..\n", test_label);
	msg   = myqtt_async_queue_timedpop (queue, 3000000);
	if (msg != NULL) {
		printf ("ERROR: we shouldn't have received a message after subscription... but we did! topic: [%s]\n", myqtt_msg_get_topic (msg));
		return axl_false;
	} /* end if */

	/* now subscribe to a topic with a retained message */
	printf ("Test %s: subscribe and check we receive the message..\n", test_label);
	if (! test_17_publish_and_check (conn, queue, axl_true, wildcard, sub_topic, test_label))
		return axl_false;

	/* now subscribe to a topic with a retained message */
	printf ("Test %s: subscribe with the same connection but without receiving message..\n", test_label);
	if (! test_17_publish_and_check (conn, queue, axl_false, wildcard, sub_topic, test_label))
		return axl_false;

	/* close connection and release message */
	myqtt_conn_close (conn);

	/* release queue */
	myqtt_async_queue_unref (queue);

	/* release context */
	printf ("Test %s: releasing context..\n", test_label);
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_17 (void) {
	return test_17_common (0);
}

axl_bool test_17a (void) {
	return test_17_common (1);
}

axl_bool test_17b (void) {
	return test_17_common (2);
}

#if defined(ENABLE_TLS_SUPPORT)
axl_bool test_18 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConnOpts   * opts;

	if (! ctx)
		return axl_false;

	printf ("Test 18: checking TLS support\n");

	/* disable verification */
	opts = myqtt_conn_opts_new ();
	myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

	/* do a simple connection */
	conn = myqtt_tls_conn_new (ctx, NULL, axl_false, 30, listener_host, listener_tls_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	printf ("Test 18: pushing messages\n");

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/18", "Test message 1", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/18", "Test message 2", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	printf ("Test 18: checking connection\n");

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 18: closing connection..\n");
	if (! myqtt_tls_is_on (conn)) {
		printf ("ERROR: expected to find TLS enabled connection but found it isn't\n");
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 18: releasing context\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_19 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConnOpts   * opts;

	if (! ctx)
		return axl_false;

	printf ("Test 19: (step 1) checking TLS support (server side verified certificate with common CA)\n");

	/* disable verification */
	opts = myqtt_conn_opts_new ();

	myqtt_tls_opts_set_ssl_certs (opts, 
				      /* certificate */
				      "client.pem",
				      /* private key */
				      "client.pem",
				      NULL,
				      /* ca certificate */
				      "root.pem");

	/* setup a host name just to ensure handlers
	   __certificate_handler at myqtt-regression-listener.c does
	   not find a certificate for this test */
	myqtt_tls_opts_set_server_name (opts, "test19.skipsni.localhost");

	/* do a simple connection */
	conn = myqtt_tls_conn_new (ctx, NULL, axl_false, 30, listener_host, "1911", opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	printf ("Test 19: pushing messages\n");

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/19", "Test message 1", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/19", "Test message 2", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	printf ("Test 19: checking connection\n");

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 19: closing connection..\n");
	if (! myqtt_tls_is_on (conn)) {
		printf ("ERROR: expected to find TLS enabled connection but found it isn't\n");
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);

	/*** STEP 2 ***/
	printf ("Test 19: (step 2) attempting to connect without providing certificates\n");

	/* disable verification */
	opts = myqtt_conn_opts_new ();
	myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

	conn = myqtt_tls_conn_new (ctx, NULL, axl_false, 30, listener_host, "1911", opts, NULL, NULL);
	if (myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being NOT able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);

	/*** STEP 3 ***/
	printf ("Test 19: (step 3) attempting to connect without providing certificates\n");

	/* disable verification */
	opts = myqtt_conn_opts_new ();

	myqtt_tls_opts_set_ssl_certs (opts, 
				      /* certificate */
				      "test-certificate.crt",
				      /* private key */
				      "test-private.key",
				      NULL,
				      /* ca certificate */
				      "root.pem");

	conn = myqtt_tls_conn_new (ctx, NULL, axl_false, 30, listener_host, "1911", opts, NULL, NULL);
	if (myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being NOT able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	/* close connection */
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 19: releasing context\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_19a (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConn       * conn2;
	MyQttConnOpts   * opts;
	MyQttMsg        * msg;
	char            * fingerprint;
	const char      * ref;
	MyQttAsyncQueue * queue;

	if (! ctx)
		return axl_false;

	printf ("Test 19-a: checking TLS support (SNI indication)\n");

	/* create queue */
	queue = myqtt_async_queue_new ();

	/* disable verification */
	opts = myqtt_conn_opts_new ();
	myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

	/* do a simple connection */
	conn = myqtt_tls_conn_new (ctx, NULL, axl_false, 30, listener_host, listener_tls_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* get finger print */
	fingerprint = myqtt_tls_get_peer_ssl_digest (conn, MYQTT_SHA1);
	printf ("Test 19-a: ssl certificate fingerprint found: %s\n", fingerprint);

	if (!axl_cmp (fingerprint, "5B:94:2B:0C:25:DA:DA:22:B3:D7:54:52:1D:D1:94:63:A0:E7:E2:A4")) {
		printf ("ERROR: expected different finger print...\n");
		return axl_false;
	}
	axl_free (fingerprint);

	printf ("Test 19-a: closing connection..\n");
	if (! myqtt_tls_is_on (conn)) {
		printf ("ERROR: expected to find TLS enabled connection but found it isn't\n");
		return axl_false;
	} /* end if */

	printf ("Test 19-a: getting announced SNI value at the server side\n");
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	/* call to get client identifier */
	if (! myqtt_conn_pub (conn, "myqtt/admin/get-server-name", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		return axl_false;
	} /* end if */

	/* push a message to ask for clientid identifier */
	msg   = myqtt_async_queue_timedpop (queue, 10000000);
	if (msg == NULL) {
		printf ("ERROR: expected to find message reply...but nothing was found..\n");
		return axl_false;
	} /* end if */

	printf ("Test 19-a: checking serverName...\n");
	if (! axl_cmp (myqtt_msg_get_app_msg (msg), "localhost")) {
		printf ("ERROR: expected localhost, but found: %s\n", (const char*) myqtt_msg_get_app_msg (msg));
		return axl_false;
	}
	printf ("Test 19-a: found announced serverName: %s\n", (const char*) myqtt_msg_get_app_msg (msg));

	/* release message */
	myqtt_msg_unref (msg);

	/* disable verification */
	opts = myqtt_conn_opts_new ();
	myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

	printf ("Test 19-a: now connect requesting serverName: test19a.localhost\n");
	myqtt_tls_opts_set_server_name (opts, "test19a.localhost");

	/* do a simple connection */
	conn2 = myqtt_tls_conn_new (ctx, NULL, axl_false, 30, listener_host, listener_tls_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	/* get finger print */
	fingerprint = myqtt_tls_get_peer_ssl_digest (conn2, MYQTT_SHA1);
	printf ("Test 19-a: ssl certificate fingerprint found: %s\n", fingerprint);

	ref = "EE:A8:D6:4B:25:C0:E7:24:6E:F5:2D:59:B2:DF:47:22:7D:E6:FC:BC";
	if (!axl_cmp (fingerprint, ref)) {
		printf ("ERROR: expected different finger: %s\n", ref);
		printf ("ERROR:              but received: %s\n", fingerprint);
		return axl_false;
	}
	axl_free (fingerprint); 

	if (! myqtt_conn_is_ok (conn2, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 19-a: checking connection has tls on..\n");
	if (! myqtt_tls_is_on (conn2)) {
		printf ("ERROR: expected to find TLS enabled connection but found it isn't\n");
		return axl_false;
	} /* end if */

	myqtt_conn_set_on_msg (conn2, test_03_on_message, queue);

	/* call to get client identifier */
	if (! myqtt_conn_pub (conn2, "myqtt/admin/get-server-name", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		return axl_false;
	} /* end if */

	/* push a message to ask for clientid identifier */
	msg   = myqtt_async_queue_timedpop (queue, 10000000);
	if (msg == NULL) {
		printf ("ERROR: expected to find message reply...but nothing was found..\n");
		return axl_false;
	} /* end if */

	printf ("Test 19-a: checking serverName...\n");
	if (! axl_cmp (myqtt_msg_get_app_msg (msg), "test19a.localhost")) {
		printf ("ERROR: expected localhost, but found: %s\n", (const char*) myqtt_msg_get_app_msg (msg));
		return axl_false;
	}
	printf ("Test 19-a: found announced serverName: %s\n", (const char*) myqtt_msg_get_app_msg (msg));

	/* release message */
	myqtt_msg_unref (msg);

	/* release queue */
	myqtt_async_queue_unref (queue);

	/* close connection */
	myqtt_conn_close (conn);
	myqtt_conn_close (conn2);

	/* release context */
	printf ("Test 19-a: releasing context\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
axl_bool test_20 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	noPollConn      * nopoll_conn;
	noPollCtx       * nopoll_ctx;

	if (! ctx)
		return axl_false;

	printf ("Test 20: checking WebSocket ws:// support\n");

	/* create first a noPoll connection, for that we need to
	   create a context */
	nopoll_ctx   = nopoll_ctx_new ();
	nopoll_conn  = nopoll_conn_new (nopoll_ctx, listener_host, listener_websocket_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	printf ("Test 20: created WS connection, now starting MQTT session on top of it..\n");

	/* now create MQTT connection using already working noPoll
	   connection */
	/* nopoll_log_enable (nopoll_ctx, axl_true);
	   nopoll_log_color_enable (nopoll_ctx, axl_true); */
	conn = myqtt_web_socket_conn_new (ctx, NULL, axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_websocket_port);
		return axl_false;
	} /* end if */

	printf ("Test 20: pushing messages\n");

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/19", "Test message 1", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/19", "Test message 2", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	printf ("Test 20: checking connection\n");

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 20: closing connection..\n");

	/* close connection (this already closes the provided
	   connection) */
	myqtt_conn_close (conn);

	/* release context (this already closes provided noPollCtx (nopoll_ctx) */
	printf ("Test 20: releasing context\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_20b (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	noPollConn      * nopoll_conn;
	noPollCtx       * nopoll_ctx;
	noPollConnOpts  * opts;
	MyQttMsg        * msg;

	if (! ctx)
		return axl_false;

	printf ("Test 20-b: checking WebSocket wss://%s:%s support\n", listener_host, listener_websocket_s_port);

	/* create first a noPoll connection, for that we need to
	   create a context */
	nopoll_ctx   = nopoll_ctx_new ();

	if (test_common_enable_debug) {
		nopoll_log_enable (nopoll_ctx, nopoll_true);
		nopoll_log_color_enable (nopoll_ctx, nopoll_true);
	} /* end if */

	opts = nopoll_conn_opts_new ();
	nopoll_conn_opts_ssl_peer_verify (opts, nopoll_false);

	nopoll_conn  = nopoll_conn_tls_new (nopoll_ctx, opts, listener_host, listener_websocket_s_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return nopoll_false;
	} /* end if */

	printf ("Test 20-b: created WSS (TLS) connection, now starting MQTT session on top of it (%s:%s socket %d)..\n",
		nopoll_conn_host (nopoll_conn), nopoll_conn_port (nopoll_conn), nopoll_conn_socket (nopoll_conn));

	/* now create MQTT connection using already working noPoll
	   connection */
	/* nopoll_log_enable (nopoll_ctx, axl_true);
	   nopoll_log_color_enable (nopoll_ctx, axl_true); */
	conn = myqtt_web_socket_conn_new (ctx, NULL, axl_false, 30, nopoll_conn, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_websocket_port);
		return axl_false;
	} /* end if */

	printf ("Test 20-b: pushing messages\n");

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/19", "Test message 1", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	/* publish a message with retention */
	if (! myqtt_conn_pub (conn, "this/is/a/test/19", "Test message 2", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: unable to publish retained message.. myqtt_conn_pub () failed..\n");
		return axl_false;
	} /* end if */

	printf ("Test 20-b: checking connection\n");

	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	/* call to get client identifier */
	if (! myqtt_conn_pub (conn, "myqtt/admin/get-tls-status", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		return axl_false;
	} /* end if */

	/* push a message to ask for clientid identifier */
	msg   = myqtt_conn_get_next (conn, 10000);
	if (msg == NULL) {
		printf ("ERROR: expected to find message reply...but nothing was found..\n");
		return axl_false;
	} /* end if */

	printf ("Test 20-b: remote tls status...\n");
	if (! axl_cmp (myqtt_msg_get_app_msg (msg), "tls-on-bro!")) {
		printf ("ERROR: expected tls-on-bro!, but found: %s\n", (const char*) myqtt_msg_get_app_msg (msg));
		return axl_false;
	}

	printf ("Test 20-b: found announced TLS: %s\n", (const char*) myqtt_msg_get_app_msg (msg));

	/* release message */
	myqtt_msg_unref (msg);

	printf ("Test 20-b: closing connection..\n");

	/* close connection (this already closes the provided
	   connection) */
	myqtt_conn_close (conn);

	/* release context (this already closes provided noPollCtx (nopoll_ctx) */
	printf ("Test 20-b: releasing context\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}
#endif

axl_bool test_21 (void) {

	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;

	struct timeval    stop;
	struct timeval    start;
	struct timeval    diff;

	if (! ctx)
		return axl_false;

	printf ("Test 21: checking connection close after PUBLISH..\n");

	/* create first a noPoll connection, for that we need to
	   create a context */
	conn = myqtt_conn_new (ctx, "test_01", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_websocket_port);
		return axl_false;
	} /* end if */

	printf ("Test 21: pushing messages\n");

	/* publish a message */
	printf ("Test 21: testing (QOS2)...\n");
	gettimeofday (&start, NULL);
	if (myqtt_conn_pub (conn, "close/conn", "Test message 1", 14, MYQTT_QOS_2, axl_true, 10)) {
		printf ("ERROR: it shouldn't work, but it did .. myqtt_conn_pub () worked..\n");
		return axl_false;
	} /* end if */
	gettimeofday (&stop, NULL);
	myqtt_timeval_substract (&stop, &start, &diff);

	printf ("Test 21: wait for failing pub was %d,%.2f ms..\n", 
		(int) diff.tv_sec, 
		(double) ((double)diff.tv_usec / (double) 1000));
	if (diff.tv_sec > 2) {
		printf ("ERROR: expected to find no more of 2 seconds of wait but found %d\n", (int) diff.tv_sec);
		return axl_false;
	} /* end if */

	/* publish a message */
	printf ("Test 21: testing (QOS1)...\n");
	gettimeofday (&start, NULL);
	if (myqtt_conn_pub (conn, "close/conn", "Test message 1", 14, MYQTT_QOS_1, axl_true, 10)) {
		printf ("ERROR: it shouldn't work, but it did .. myqtt_conn_pub () worked..\n");
		return axl_false;
	} /* end if */
	gettimeofday (&stop, NULL);
	myqtt_timeval_substract (&stop, &start, &diff);

	printf ("Test 21: wait for failing pub was %d,%.2f ms..\n", 
		(int) diff.tv_sec, 
		(double) ((double)diff.tv_usec / (double) 1000));
	if (diff.tv_sec > 2) {
		printf ("ERROR: expected to find no more of 2 seconds of wait but found %d\n", (int) diff.tv_sec);
		return axl_false;
	} /* end if */

	/* publish a message */
	printf ("Test 21: testing (QOS0)...\n");
	gettimeofday (&start, NULL);
	if (myqtt_conn_pub (conn, "close/conn", "Test message 1", 14, MYQTT_QOS_0, axl_true, 10)) {
		printf ("ERROR: it shouldn't work, but it did .. myqtt_conn_pub () worked..\n");
		return axl_false;
	} /* end if */
	gettimeofday (&stop, NULL);
	myqtt_timeval_substract (&stop, &start, &diff);

	printf ("Test 21: wait for failing pub was %d,%.2f ms..\n", 
		(int) diff.tv_sec, 
		(double) ((double)diff.tv_usec / (double) 1000));
	if (diff.tv_sec > 2) {
		printf ("ERROR: expected to find no more of 2 seconds of wait but found %d\n", (int) diff.tv_sec);
		return axl_false;
	} /* end if */

	/* close connection (this already closes the provided
	   connection) */
	myqtt_conn_close (conn);

	/* release context (this already closes provided noPollCtx (nopoll_ctx) */
	printf ("Test 21: releasing context\n");
	myqtt_exit_ctx (ctx, axl_true);

	return axl_true;
}

void test_22_reconnected (MyQttConn * conn, axlPointer user_data)
{
	MyQttAsyncQueue * queue = user_data;

	printf ("Test 22: reconnect detected (and finished)...\n");
	myqtt_async_queue_push (queue, INT_TO_PTR (4));
	return;
}

axl_bool test_22_close_recover_and_send (MyQttConn * conn, MyQttAsyncQueue * queue, const char * label)
{

	MYQTT_SOCKET   _sock;
	MyQttMsg     * msg;

	/* close the socket to simulate a connection failure */
	_sock = myqtt_conn_get_socket (conn);
	printf ("Test 22: %s: closing socket=%d...\n", label, _sock);
	myqtt_close_socket (myqtt_conn_get_socket (conn));

	/* wait for reconnect */
	printf ("Test 22: %s: waiting reconnect to complete (10 seconds at most)..\n", label);
	myqtt_async_queue_timedpop (queue, 10000000);
	
	/* check connection */
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected to find connection is OK state, but found it broken..\n");
		return axl_false;
	} /* end if */

	if (myqtt_conn_get_socket (conn) <= 0) {
		printf ("ERROR: myqtt connection socket %d seems to be not right\n", myqtt_conn_get_socket (conn));
		return axl_false;
	} /* end if */

	printf ("Test 22: %s: close it again socket=%d...\n", label, _sock);
	myqtt_close_socket (myqtt_conn_get_socket (conn));

	/* wait for reconnect */
	printf ("Test 22: %s: waiting reconnect to complete (10 seconds at most) for second time..\n", label);
	myqtt_async_queue_timedpop (queue, 10000000);
	
	/* check connection */
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected to find connection is OK state, but found it broken..\n");
		return axl_false;
	} /* end if */

	if (myqtt_conn_get_socket (conn) <= 0) {
		printf ("ERROR: myqtt connection socket %d seems to be not right\n", myqtt_conn_get_socket (conn));
		return axl_false;
	} /* end if */

	/* send some data here */
	printf ("Test 22: %s: sending some data..\n", label);

	/* set on message received */
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);
	
	/* call to get client identifier */
	if (! myqtt_conn_pub (conn, "myqtt/admin/get-server-name", "", 0, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message to get client identifier..\n");
		return axl_false;
	} /* end if */

	/* push a message to ask for clientid identifier */
	msg   = myqtt_async_queue_timedpop (queue, 10000000);
	if (msg == NULL) {
		printf ("ERROR: expected to find message reply...but nothing was found..\n");
		return axl_false;
	} /* end if */

	printf ("Test 22: %s: data received, connection is working: %s\n", label, (const char *) myqtt_msg_get_app_msg (msg));
	myqtt_msg_unref (msg);

	return axl_true;
}

#if defined(ENABLE_WEBSOCKET_SUPPORT)
axlPointer test_22_create_websocket (MyQttCtx * ctx, MyQttConn * conn, axlPointer user_data, axlPointer user_data2, axlPointer user_data3)
{
	noPollCtx  * nopoll_ctx;
	noPollConn * nopoll_conn;

	const char * _listener_host           = user_data;
	const char * _listener_websocket_port = user_data2;

	printf ("Test --: called init session setup pointer to create websocket\n");
	/* IMPORTANT: do not create the context with nopoll_ctx but
	   with myqtt_web_socket_get_ctx () to ensure no leak
	   happens */
	nopoll_ctx   = 	myqtt_web_socket_get_ctx (ctx);
	nopoll_conn  = nopoll_conn_new (nopoll_ctx, _listener_host, _listener_websocket_port, NULL, NULL, NULL, NULL);
	if (! nopoll_conn_is_ok (nopoll_conn)) {
		printf ("ERROR: failed to connect remote host through WebSocket..\n");
		return NULL;
	} /* end if */

	/* report connection */
	return nopoll_conn;
}
#endif /* defined(ENABLE_WEBSOCKET_SUPPORT) */

axl_bool test_22 (void)
{
	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttConnOpts   * opts;
	MyQttAsyncQueue * queue;

	if (! ctx)
		return axl_false;

	/* create queue */
	queue = myqtt_async_queue_new ();

	printf ("Test 22: Connect to the server and force disconnect to let the library reconnect..\n");

	opts = myqtt_conn_opts_new ();
	myqtt_conn_opts_set_reconnect (opts, axl_true);

	/* create first a noPoll connection, for that we need to
	   create a context */
	conn = myqtt_conn_new (ctx, "test_01", axl_true, 30, listener_host, listener_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_websocket_port);
		return axl_false;
	} /* end if */

	/* configure on reconnect */
	myqtt_conn_set_on_reconnect (conn, test_22_reconnected, queue);

	if (! test_22_close_recover_and_send (conn, queue, "plain-mqtt"))
		return axl_false;

	/* close connection (this already closes the provided
	   connection) */
	myqtt_conn_close (conn);

#if defined(ENABLE_TLS_SUPPORT)
	printf ("Test 22: creating TLS connection, now starting MQTT session on top of it (to check reconnection)..\n");

	opts = myqtt_conn_opts_new ();
	myqtt_conn_opts_set_reconnect (opts, axl_true);
	myqtt_tls_opts_ssl_peer_verify (opts, axl_false);

	/* do a simple connection */
	conn = myqtt_tls_conn_new (ctx, NULL, axl_false, 30, listener_host, listener_tls_port, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s..\n", listener_host, listener_tls_port);
		return axl_false;
	} /* end if */

	/* configure on reconnect */
	myqtt_conn_set_on_reconnect (conn, test_22_reconnected, queue);

	if (! test_22_close_recover_and_send (conn, queue, "mqtt-tls"))
		return axl_false;

	/* close connection (this already closes the provided connection) */
	myqtt_conn_close (conn);
	
#endif /* defined(ENABLE_WEBSOCKET_SUPPORT) */

#if defined(ENABLE_WEBSOCKET_SUPPORT)
	/* create first a noPoll connection, for that we need to
	   create a context */
	printf ("Test 22: creating WebSocket connection, now starting MQTT session on top of it (to check reconnection)..\n");

	opts = myqtt_conn_opts_new ();
	myqtt_conn_opts_set_reconnect (opts, axl_true);
	myqtt_conn_opts_set_init_session_setup_ptr (opts, test_22_create_websocket, (axlPointer) listener_host, (axlPointer) listener_websocket_port, NULL);

	/* now create MQTT connection using already working noPoll
	   connection */
	/* nopoll_log_enable (nopoll_ctx, axl_true);
	   nopoll_log_color_enable (nopoll_ctx, axl_true); */
	conn = myqtt_web_socket_conn_new (ctx, NULL, axl_false, 30, NULL, opts, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: expected being able to connect to %s:%s.. (myqtt_web_socket_conn_new)\n", listener_host, listener_websocket_port);
		return axl_false;
	} /* end if */

	/* configure on reconnect */
	myqtt_conn_set_on_reconnect (conn, test_22_reconnected, queue);

	if (! test_22_close_recover_and_send (conn, queue, "mqtt-ws"))
		return axl_false;

	/* close connection (this already closes the provided
	   connection) */
	myqtt_conn_close (conn);
	

#endif /* defined(ENABLE_WEBSOCKET_SUPPORT) */

	/* release queue */
	myqtt_async_queue_unref (queue);

	/* release context (this already closes provided noPollCtx (nopoll_ctx) */
	printf ("Test 21: releasing context\n");
	myqtt_exit_ctx (ctx, axl_true);


	return axl_true;
}

axl_bool test_23 (void)
{
	MyQttCtx        * ctx = init_ctx ();
	MyQttConn       * conn;
	MyQttAsyncQueue * queue;
	int               sub_result;
	MyQttMsg        * msg;

	if (! ctx)
		return axl_false;

	printf ("Test 23: creating connection (2 seconds delay because MYQTT_CONNACK_DEFERRED)..\n");

	/* now connect to the listener:
	   client_identifier -> test_03
	   clean_session -> axl_true
	   keep_alive -> 30 */
	conn = myqtt_conn_new (ctx, "test_23", axl_true, 30, listener_host, listener_port, NULL, NULL, NULL);
	if (! myqtt_conn_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to connect to %s:%s..\n", listener_host, listener_port);
		return axl_false;
	} /* end if */

	printf ("Test 23: connected without problems..\n");

	/* subscribe to a topic */
	if (! myqtt_conn_sub (conn, 10, "myqtt/test/deferred", 0, &sub_result)) {
		printf ("ERROR: unable to subscribe, myqtt_conn_sub () failed, sub_result=%d\n", sub_result);
		return axl_false;
	} /* end if */

	/* register on message handler */
	queue = myqtt_async_queue_new ();
	myqtt_conn_set_on_msg (conn, test_03_on_message, queue);

	/* publish application message (the message sent here is
	 * bigger than 24, this is on purpose) */
	if (! myqtt_conn_pub (conn, "myqtt/test/deferred", "This is test message........", 24, MYQTT_QOS_0, axl_false, 0)) {
		printf ("ERROR: unable to publish message, myqtt_conn_pub() failed\n");
		return axl_false;
	} /* end if */


	/* waiting for reply */
	printf ("Test 23: waiting for reply..\n");
	msg   = myqtt_async_queue_pop (queue);
	myqtt_async_queue_unref (queue);
	if (msg == NULL) {
		printf ("ERROR: expected to find message from queue, but NULL was found..\n");
		return axl_false;
	} /* end if */

	/* release message */
	printf ("Test 23: releasing references=%d\n", myqtt_msg_ref_count (msg));
	myqtt_msg_unref (msg);
 
	/* close connection */
	printf ("Test 23: closing connection..\n");
	myqtt_conn_close (conn);

	/* release context */
	printf ("Test 23: releasing context..\n");
	myqtt_exit_ctx (ctx, axl_true);
	
	return axl_true;
}

void wrong_sub (const char * topic_filter, axl_bool should_fail) {
	axl_bool is_wrong = myqtt_reader_is_wrong_topic (topic_filter);

	if (! is_wrong && should_fail) {
		printf ("ERROR: expected to find notified as wrong topic [%s] but it wasn't\n", topic_filter);
		exit (-1);
	}

	if (is_wrong && ! should_fail) {
		printf ("ERROR: expected proper topic declatation [%s] but it was found considered wrong\n",
			topic_filter);
		exit (-1);
	}

	return;
}


void match_topic (const char * topic, const char * filter, axl_bool should_match) {

	axl_bool match;

	wrong_sub (filter, axl_false);

	/* call match */
	match = myqtt_reader_topic_filter_match (topic, filter);
	if (should_match && match) {
		printf ("Test --: [%s] matches with [%s]\n", topic, filter);
		return;
	}

	if (should_match && ! match) {
		printf ("ERROR: expected topic [%s] to be matched by [%s] but it was not\n",
			topic, filter);
		exit (-1);
	}

	if (! should_match && match) {
		printf ("ERROR: expected topic [%s] to NOT be matched by [%s] but it was\n",
			topic, filter);
		exit (-1);
	}

	printf ("Test --: [%s] NOT matches with [%s]\n", topic, filter);
	return;
}

axl_bool test_00_d (void)
{

	/* set 0 */
	match_topic ("hola", "hola", axl_true);

	match_topic ("hola/", "hola/", axl_true);

	match_topic ("/", "/", axl_true);

	/* set 1 */
	match_topic ("sensors/my-pc/temperature/disk-0", "sensors/+/temperature/+", axl_true);

	match_topic ("sensors/my-pc/temperature/disk-0", "sensors/+/temperature", axl_false);

	match_topic ("sensors/my-pc/temperature/disk-0", "+/temperature/+", axl_false);
	

	/* set 2 */
	match_topic ("a/b/c/d", "+/b/c/d", axl_true);

	match_topic ("a/b/c/d", "a/+/c/d", axl_true);

	match_topic ("a/b/c/d", "a/+/+/d", axl_true);

	match_topic ("a/b/c/d", "+/+/+/+", axl_true);

	
	/* set 3 */
	match_topic ("a/b/c/d", "a/b/c", axl_false);

	match_topic ("a/b/c/d", "b/+/c/d", axl_false);

	match_topic ("a/b/c/d", "+/+/+", axl_false);


	/* set 4 */
	match_topic ("a/b/c/d", "#", axl_true);

	match_topic ("a/b/c/d", "a/#", axl_true);

	match_topic ("a/b/c/d", "a/b/#", axl_true);

	match_topic ("a/b/c/d", "a/b/c/#", axl_true);

	match_topic ("a/b/c/d", "+/b/c/#", axl_true);

	

	/* set 5 */
	match_topic ("a//topic", "a/+/topic", axl_true);

	match_topic ("a//topic", "a/+/topic", axl_true);

	match_topic ("/a/topic", "+/a/topic", axl_true);

	match_topic ("/a/topic", "#", axl_true);

	match_topic ("/a/topic", "/#", axl_true);

	match_topic ("value/a/topic", "/#", axl_false);


	/* set  6 */

	match_topic ("a/topic/", "a/topic/+", axl_true);

	match_topic ("a/topic/", "a/topic/#", axl_true);


	
	/* set 7 */

	match_topic ("sport/tennis/player1", "sport/tennis/player1/#", axl_true);

	match_topic ("sport/tennis/player1/ranking", "sport/tennis/player1/#", axl_true);

	match_topic ("sport/tennis/player1/score/wimbledon", "sport/tennis/player1/#", axl_true);


	/* set 8 */

	match_topic ("sport", "sport/#", axl_true);


	/* set 9 */

	match_topic ("sport/tennis/player1", "sport/tennis/+", axl_true);

	match_topic ("sport/tennis/player2", "sport/tennis/+", axl_true);

	match_topic ("sport/tennis/player1/ranking", "sport/tennis/+", axl_false);

	
	/* set 10 */

	match_topic ("sport", "sport/+", axl_false);

	match_topic ("sport/", "sport/+", axl_true);


	/* set 11 */


	match_topic ("/finance", "+/+", axl_true);

	match_topic ("/finance", "/+", axl_true);

	match_topic ("/finance", "+", axl_false);


	/* set 12 */

	match_topic ("$SYS", "#", axl_false);

	match_topic ("$SYS", "+", axl_false);

	match_topic ("$SYS/monitor/Clients", "+/monitor/Clients", axl_false);

	match_topic ("$SYS/", "$SYS/#", axl_true);

	match_topic ("$SYS/monitor/Clients", "$SYS/monitor/+", axl_true);


	/* set 13 */

	match_topic ("ACCOUNTS", "Accounts", axl_false);

	match_topic ("finance", "/finance", axl_false);


	/* not valid subscriptions */
	
	wrong_sub ("sport/tennis#", axl_true);

	wrong_sub ("sport/tennis/#/ranking", axl_true);

	wrong_sub ("sport+", axl_true);

	return axl_true;
}

#if defined(ENABLE_MOSQUITTO)
void test_mosquitto_queue_message (struct mosquitto * mosq, void * _queue, const struct mosquitto_message * msg)
{
	struct mosquitto_message * dst = axl_new (struct mosquitto_message, 1);

	mosquitto_message_copy (dst, msg);

	/* copy message received */
	myqtt_async_queue_push (_queue, dst);
	return;
}

void test_mosquitto_on_subscribe (struct mosquitto * mosq, void * _queue, int mid, int qos_count, const int * granted_qos)
{
	myqtt_async_queue_push (_queue, INT_TO_PTR (granted_qos[0] + 10));
	return;
}

axl_bool test_mosquitto_01 (void)
{
	const char * client_id = "test01";
	int port = 1909;
	int keepalive          = 60;
	axl_bool clean_session = axl_true;
	struct mosquitto *mosq = NULL;
	int                      err_value;
	MyQttAsyncQueue * queue;
	struct mosquitto_message * msg;
	int granted_qos;
	int iterator;

	/* new queue */
	queue = myqtt_async_queue_new ();

	printf ("Test mosquitto-01: creating mosq reference..\n");
	mosq = mosquitto_new (client_id, clean_session, queue);
	if (! mosq) {
		printf ("Test mosquitto-01: Error: Out of memory, errno=%d (ENOMEM=%d, EINVAL=%d)\n",
			errno, errno == ENOMEM, errno == EINVAL);
		return axl_false;
	} /* end if */

	/* mosquitto_message_callback_set (mosq, my_message_callback); */

	printf ("Test mosquitto-01: connectin with clean_session=%d\n", clean_session);
	if (mosquitto_connect (mosq, listener_host, port, keepalive)){
		printf ("Test mosquitto-01: Unable to connect.\n");
		return axl_false;
	}
	printf ("Test mosquitto-01: Connected without problems..\n");

	/* configure on message */
	mosquitto_subscribe_callback_set (mosq, test_mosquitto_on_subscribe);

	/* subscribe to the topic */
	printf ("Test mosquitto-01: Subscribing to myqtt/mosquitto/test..\n");
	err_value = mosquitto_subscribe (mosq, NULL, "myqtt/mosquitto/test", 2);
	if (err_value != MOSQ_ERR_SUCCESS) {
		printf ("ERROR: failed to subscribe, error was: %d\n", err_value);
		return axl_false;
	} /* end if */

	/* wait for a reply */
	while (axl_true) {
		mosquitto_loop (mosq, -1, 1);
		granted_qos = PTR_TO_INT (myqtt_async_queue_timedpop (queue, 10000));
		if (granted_qos > 0) {
			printf ("Test mosquitto-01: received granted QoS: %d\n", granted_qos - 10);
			break;
		}
	} /* end while */

	/* configure on message */
	mosquitto_message_callback_set (mosq, test_mosquitto_queue_message);

	printf ("Test mosquitto-01: Publishing a message..\n");
	err_value = mosquitto_publish (mosq, NULL, "myqtt/mosquitto/test", 39, "This is a test from mosquitto library..", 0, axl_false);
	if (err_value != MOSQ_ERR_SUCCESS) {
		printf ("ERROR: failed to publish message...err_value=%d\n", err_value);
		return axl_false;
	}

	/* wait to receive message */
	printf ("Test mosquitto-01: Waiting to receive published message.....\n");
	iterator = 0;
	while (axl_true) {
		mosquitto_loop (mosq, -1, 1);
		msg = myqtt_async_queue_timedpop (queue, 10000);
		if (msg)
			break;

		if (msg == NULL && iterator == 3) {
			printf ("ERROR: received NULL message when expected a valid message..\n");
			return axl_false;
		}
		iterator++;
	} /* end if */

	/* check content */
	if (! axl_memcmp (msg->payload, "This is a test from mosquitto library..", 39)) {
		printf ("ERROR: expected to receive different content...\n");
		return axl_false;
	} /* end if */

	/* check topic name */
	if (! axl_cmp (msg->topic, "myqtt/mosquitto/test")) {
		printf ("ERROR: expected to have a particular topic but found something different..\n");
		return axl_false;
	} /* end if */

	if (msg->qos != 0) {
		printf ("ERROR: expected to have a quality of service of 0 but found: %d\n", msg->qos);
		return axl_false;
	}

	/* free message */
	mosquitto_message_free (&msg);

	/* now call to close */
	printf ("Test mosquitto-01: Disconnected without problems..\n");
	err_value = mosquitto_disconnect (mosq);
	if (err_value != MOSQ_ERR_SUCCESS) {
		printf ("ERROR: failed to disconnect, error was: err_value=%d\n", 
			err_value);
		return axl_false;
	}
	
	mosquitto_destroy (mosq);

	/* release queue */
	myqtt_async_queue_unref (queue);

	return axl_true;
}

void __mosquitto_log (struct mosquitto * mosq, void * user_data, int level, const char * message)
{
	const char * label = "UNKNOWN";
	switch (level) {
	case MOSQ_LOG_INFO:
		label = "INFO";
		break;
	case MOSQ_LOG_NOTICE:
		label = "NOTICE";
		break;
	case MOSQ_LOG_WARNING:
		label = "WARNING";
		break;
	case MOSQ_LOG_ERR:
		label = "ERR";
		break;
	case MOSQ_LOG_DEBUG:
		label = "DEBUG";
		break;
	}

	printf ("%s: %s\n", label, message);
	return;
}

axl_bool test_mosquitto_02 (void)
{
	const char * client_id = "test01";
	int port = 1909;
	int keepalive          = 60;
	axl_bool clean_session = axl_true;
	struct mosquitto *mosq = NULL;
	int                      err_value;
	MyQttAsyncQueue * queue;
	struct mosquitto_message * msg;
	int granted_qos;
	int iterator;

	/* new queue */
	queue = myqtt_async_queue_new ();

	printf ("Test mosquitto-02: creating mosq reference..\n");
	mosq = mosquitto_new (client_id, clean_session, queue);
	if (! mosq) {
		printf ("Test mosquitto-02: Error: Out of memory, errno=%d (ENOMEM=%d, EINVAL=%d)\n",
			errno, errno == ENOMEM, errno == EINVAL);
		return axl_false;
	} /* end if */

	/* configure log */
	mosquitto_log_callback_set (mosq, __mosquitto_log);

	/* mosquitto_message_callback_set (mosq, my_message_callback); */

	printf ("Test mosquitto-02: connectin with clean_session=%d\n", clean_session);
	if (mosquitto_connect (mosq, listener_host, port, keepalive)){
		printf ("Test mosquitto-01: Unable to connect.\n");
		return axl_false;
	}
	printf ("Test mosquitto-02: Connected without problems..\n");

	/* configure on message */
	mosquitto_subscribe_callback_set (mosq, test_mosquitto_on_subscribe);

	/* subscribe to the topic */
	printf ("Test mosquitto-02: Subscribing to myqtt/mosquitto/test..\n");
	err_value = mosquitto_subscribe (mosq, NULL, "myqtt/mosquitto/test", 2);
	if (err_value != MOSQ_ERR_SUCCESS) {
		printf ("ERROR: failed to subscribe, error was: %d\n", err_value);
		return axl_false;
	} /* end if */

	/* wait for a reply */
	while (axl_true) {
		mosquitto_loop (mosq, -1, 1);
		granted_qos = PTR_TO_INT (myqtt_async_queue_timedpop (queue, 10000));
		if (granted_qos > 0) {
			printf ("Test mosquitto-01: received granted QoS: %d\n", granted_qos - 10);
			break;
		}
	} /* end while */

	if ((granted_qos - 10) != 2) {
		printf ("ERROR: expected granted QoS of 2 but found: %d\n", granted_qos);
		return axl_false;
	} /* end if */

	/* configure on message */
	mosquitto_message_callback_set (mosq, test_mosquitto_queue_message);

	printf ("Test mosquitto-02: Publishing a message with QoS 2..\n");
	err_value = mosquitto_publish (mosq, NULL, "myqtt/mosquitto/test", 47, "This is a test from mosquitto library (QoS 2)..", 2, axl_false);
	if (err_value != MOSQ_ERR_SUCCESS) {
		printf ("ERROR: failed to publish message...err_value=%d\n", err_value);
		return axl_false;
	}

	/* wait to receive message */
	printf ("Test mosquitto-02: Waiting to receive published message (QoS 2).....\n");
	iterator = 0;
	while (axl_true) {
		mosquitto_loop (mosq, -1, 1);
		msg = myqtt_async_queue_timedpop (queue, 10000);
		if (msg)
			break;

		if (msg == NULL && iterator == 20) {
			printf ("ERROR: received NULL message when expected a valid message iterator=%d..\n", iterator);
			return axl_false;
		}
		iterator++;
	} /* end if */

	/* check content */
	if (! axl_memcmp (msg->payload, "This is a test from mosquitto library (QoS 2)..", 47)) {
		printf ("ERROR: expected to receive different content...\n");
		return axl_false;
	} /* end if */

	/* check topic name */
	if (! axl_cmp (msg->topic, "myqtt/mosquitto/test")) {
		printf ("ERROR: expected to have a particular topic but found something different..\n");
		return axl_false;
	} /* end if */

	if (msg->qos != 2) {
		printf ("ERROR: expected to have a quality of service of 0 but found: %d\n", msg->qos);
		return axl_false;
	}

	/* free message */
	mosquitto_message_free (&msg);

	mosquitto_loop (mosq, -1, 1);
	mosquitto_loop (mosq, -1, 1);
	mosquitto_loop (mosq, -1, 1);

	/* now call to close */
	printf ("Test mosquitto-02: Disconnected without problems..\n");
	err_value = mosquitto_disconnect (mosq);
	if (err_value != MOSQ_ERR_SUCCESS) {
		printf ("ERROR: failed to disconnect, error was: err_value=%d\n", 
			err_value);
		return axl_false;
	}
	
	mosquitto_destroy (mosq);

	/* release queue */
	myqtt_async_queue_unref (queue);

	return axl_true;
}

#endif

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
	printf ("**     time ./myqtt-regression-client [--help] [--debug] [--no-unmap] [--run-test=NAME]\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --show-reachable=yes --error-limit=no ./myqtt-regression-client [--debug]\n**\n");
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

	CHECK_TEST("test_00_c")
	run_test (test_00_c, "Test 00-c: check myqtt_mkdir ()");

	/* check wildcard subscription */
	CHECK_TEST("test_00_d")
	run_test (test_00_d, "Test 00-d: wildcard pattern matching"); 

	CHECK_TEST("test_01")
	run_test (test_01, "Test 01: basic listener startup and client connection");

	CHECK_TEST("test_02")
	run_test (test_02, "Test 02: basic subscribe function (QOS 0)");

	CHECK_TEST("test_02a")
	run_test (test_02_a, "Test 02-a: basic subscribe function with wild-cards (QOS 0)");

	CHECK_TEST("test_03")
	run_test (test_03, "Test 03: basic subscribe function (QOS 0) and publish");

	CHECK_TEST("test_03a")
	run_test (test_03a, "Test 03-a: basic subscribe function (QOS 0) and publish (wildcard +)");

	CHECK_TEST("test_03b")
	run_test (test_03b, "Test 03-a: basic subscribe function (QOS 0) and publish (wildcard #)");

	CHECK_TEST("test_04")
	run_test (test_04, "Test 04: basic unsubscribe function (QOS 0)");

	CHECK_TEST("test_04a")
	run_test (test_04a, "Test 04-a: basic unsubscribe function (QOS 0), wildcard +");

	CHECK_TEST("test_04b")
	run_test (test_04b, "Test 04-b: basic unsubscribe function (QOS 0), wildcard #");

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

	CHECK_TEST("test_12a")
	run_test (test_12a, "Test 12-a: test QoS 1 messages (wildcard +)");

	CHECK_TEST("test_12b")
	run_test (test_12b, "Test 12-b: test QoS 1 messages (wildcard #)");

	CHECK_TEST("test_13")
	run_test (test_13, "Test 13: test QoS 2 messages");

	CHECK_TEST("test_13a")
	run_test (test_13a, "Test 13-a: test QoS 2 messages");

	CHECK_TEST("test_13b")
	run_test (test_13b, "Test 13-b: test QoS 2 messages");

	CHECK_TEST("test_14")
	run_test (test_14, "Test 14: offline PUB test messages queued to be sent on next connection (client)");  

	CHECK_TEST("test_14a")
	run_test (test_14a, "Test 14a: checking server side subscription loading on startup");  

	CHECK_TEST("test_14b")
	run_test (test_14b, "Test 14: offline PUB test messages queued to be sent on next connection (client), wildcard +");  

	CHECK_TEST("test_14c")
	run_test (test_14c, "Test 14: offline PUB test messages queued to be sent on next connection (client), wildcard #");  

	CHECK_TEST("test_15")
	run_test (test_15, "Test 15: test reception of messages when you are disconnected (with sessions)"); 

	CHECK_TEST("test_15a")
	run_test (test_15a, "Test 15-a: test reception of messages when you are disconnected (with sessions), wildcard +"); 

	CHECK_TEST("test_15b")
	run_test (test_15b, "Test 15-a: test reception of messages when you are disconnected (with sessions), wildcard #"); 
	
	CHECK_TEST("test_16")
	run_test (test_16, "Test 16: check message retention"); 

	CHECK_TEST("test_17")
	run_test (test_17, "Test 17: check message retention"); 

	CHECK_TEST("test_17a")
	run_test (test_17a, "Test 17-a: check message retention (wildcard +)"); 

	CHECK_TEST("test_17b")
	run_test (test_17b, "Test 17-b: check message retention (wildcard #)"); 

#if defined(ENABLE_TLS_SUPPORT)
	CHECK_TEST("test_18")
	run_test (test_18, "Test 18: check TLS support"); 

	CHECK_TEST("test_19")
	run_test (test_19, "Test 19: check TLS support (server side certificate auth: common CA)"); 

	/* test SNI indication */
	CHECK_TEST("test_19a")
	run_test (test_19a, "Test 19-a: check TLS SNI support (announcing a different server name and check different certificate expected)"); 
#endif

#if defined(ENABLE_WEBSOCKET_SUPPORT)
	CHECK_TEST("test_20")
	run_test (test_20, "Test 20: check WebSocket support (basic MQTT over ws://)"); 

	CHECK_TEST("test_20b")
	run_test (test_20b, "Test 20-b: check WebSocket support (basic MQTT over wss://)"); 
#endif

	/* test close connection after publish... */
	CHECK_TEST("test_21")
	run_test (test_21, "Test 21: connection close after PUBLISH (qos 0, qos 1 and qos 2)"); 

	/* test close connection after publish... */
	CHECK_TEST("test_22")
	run_test (test_22, "Test 22: test reconnect support"); 

	/* check on connect deferred */
	CHECK_TEST("test_23")
	run_test (test_23, "Test 23: check on connect deferred"); 

#if defined(ENABLE_MOSQUITTO)
	/* call to enable mosquitto library globally */
	mosquitto_lib_init();

	CHECK_TEST("test_mosquitto_01")
	run_test (test_mosquitto_01, "Test mosquitto-01: check basic connect, subscribe and PUBLISH QoS 0"); 

	CHECK_TEST("test_mosquitto_02")
	run_test (test_mosquitto_02, "Test mosquitto-01: check PUBLISH QoS 2"); 

	/* finish globally mosquitto */
	mosquitto_lib_cleanup();
#endif

	/* support for message retention when subscribed with a wild
	   card topic filter that matches different topic names */

	/* test will support with autentication */

	/* publish empty mesasage */

	/* test reconnect (after closing) */

	/* test sending an unknown message */

	/* test connection lost notification */

	/* test automatic reconnection on connection lost */

	/* test connection lost on subscription sent */

	/* test connection lost on unsubscribe sent */

	/* test sending header, and then message */

	/* test sending part of the header, then the rest, then the message */

	/* test sending broken subscribe (not complete) */

	/* test sending a broken unsubscribe (not complete) */

	printf ("All tests passed OK!\n");

	/* terminate */
	return 0;
	
} /* end main */
