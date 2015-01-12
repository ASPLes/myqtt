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


#ifndef __MYQTTD_EXPR_H__
#define __MYQTTD_EXPR_H__

#include <myqttd.h>

/** 
 * \addtogroup myqttd_expr
 * @{
 */

/** 
 * @brief Regular expression type definition provided by the \ref
 * myqttd_expr module. This type is used to represent an regular
 * expression that can be used to match strings.
 */ 
typedef struct _MyQttdExpr MyQttdExpr;

MyQttdExpr * myqttd_expr_compile (MyQttdCtx     * ctx, 
					  const char    * expression, 
					  const char    * error_msg);

axl_bool         myqttd_expr_match   (MyQttdExpr * expr, 
					  const char     * subject);

/** 
 * @brief Alias definition associated to myqttd_expr_get_expression.
 * @param expr The MyQttdExpr where the associated string expression is being queried.
 * @return A reference to the string expression.
 */
#define __MYQTTD_EXP_STR__(expr) myqttd_expr_get_expression(expr)
const char     * myqttd_expr_get_expression (MyQttdExpr * expr);

void             myqttd_expr_free    (MyQttdExpr * expr);

#endif /* __MYQTTD_EXPR_H__ */

/** 
 * @}
 */
