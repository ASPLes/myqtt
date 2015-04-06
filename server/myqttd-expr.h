/* 
 *  MyQtt: A high performance open source MQTT implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation version 2.0 of the
 *  License
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301, USA.
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is GPL software 
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
