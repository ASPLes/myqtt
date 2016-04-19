/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
#ifndef __MYQTT_HASH_H__
#define __MYQTT_HASH_H__

#include <myqtt.h>


MyQttHash * myqtt_hash_new_full (axlHashFunc    hash_func,
				   axlEqualFunc   key_equal_func,
				   axlDestroyFunc key_destroy_func,
				   axlDestroyFunc value_destroy_func);

MyQttHash  * myqtt_hash_new      (axlHashFunc    hash_func,
				   axlEqualFunc   key_equal_func);

void         myqtt_hash_ref      (MyQttHash   * hash_table);

void         myqtt_hash_unref    (MyQttHash   * hash_table);

void         myqtt_hash_insert   (MyQttHash *hash_table,
				  axlPointer  key,
				  axlPointer  value);

void         myqtt_hash_replace  (MyQttHash *hash_table,
				   axlPointer  key,
				   axlPointer  value);

void         myqtt_hash_replace_full  (MyQttHash     * hash_table,
				       axlPointer       key,
				       axlDestroyFunc   key_destroy,
				       axlPointer       value,
				       axlDestroyFunc   value_destroy);

int          myqtt_hash_size     (MyQttHash   * hash_table);

axlPointer   myqtt_hash_lookup   (MyQttHash   * hash_table,
				  axlPointer    key);

axl_bool     myqtt_hash_exists   (MyQttHash   * hash_table,
				  axlPointer    key);

axlPointer   myqtt_hash_lookup_and_clear   (MyQttHash   *hash_table,
					    axlPointer   key);

int          myqtt_hash_lock_until_changed (MyQttHash   * hash_table,
					    long          wait_microseconds);

axl_bool     myqtt_hash_remove   (MyQttHash   * hash_table,
				  axlPointer    key);

void         myqtt_hash_destroy  (MyQttHash *hash_table);

axl_bool     myqtt_hash_delete   (MyQttHash   * hash_table,
				  axlPointer    key);

void         myqtt_hash_foreach  (MyQttHash         * hash_table,
				   axlHashForeachFunc   func,
				   axlPointer           user_data);

void         myqtt_hash_foreach2  (MyQttHash         * hash_table,
				   axlHashForeachFunc2  func,
				   axlPointer           user_data,
				   axlPointer           user_data2);

void         myqtt_hash_foreach3  (MyQttHash         * hash_table,
				    axlHashForeachFunc3  func,
				    axlPointer           user_data,
				    axlPointer           user_data2,
				    axlPointer           user_data3);

void         myqtt_hash_clear    (MyQttHash *hash_table);

#endif
