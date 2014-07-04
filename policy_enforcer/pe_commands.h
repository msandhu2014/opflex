/*
* Copyright (c) 2014 Cisco Systems, Inc. and others.
*               All rights reserved.
*
* This program and the accompanying materials are made available under
* the terms of the Eclipse Public License v1.0 which accompanies this
* distribution, and is available at
* http://www.eclipse.org/legal/epl-v10.html
*/

#ifndef PE_COMMANDS_H
#define PE_COMMANDS_H 1

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* data */

/* enums for ovs commands */
typedef enum {
    CREATE_BRIDGE,
    CREATE_BROADCAST_DOMAIN,
    CREATE_HOST_PORT,
    CREATE_ROUTER,
    ADD_BD_MEMBER,
    ADD_FLOW_ARP,
    ADD_FLOW_SELECT_SEPG,
    ADD_FLOW_SELECT_DEPG_L2,
    ADD_FLOW_SELECT_DEPG_L3,
    ADD_FLOW_POLICY_DENY,
    ADD_FLOW_POLICY_DROP,
    ADD_FLOW_POLICY_PERMIT,
    ADD_FLOW_POLICY_PERMIT_BI,
    COMMIT_FLOWS
} pe_ovs_command_t;

/* linked list types for pe_translate and pe_commands
 * TODO: this should have an enum for the command
 */
typedef struct _pe_command_node pe_command_node_t;
struct _pe_command_node {
    uint32_t group_id;        /* ID of this linked list */
    uint32_t cmd_id;          /* Ordered ID of this command */
    uint32_t **dependency;    /* Array of cmd_id's on which this command
                               * depends - e.g., if one in the list of
                               * dependencies fail, then they all do */
    pe_command_node_t *next;     /* next pe_command_node in the linked list */
    pe_command_node_t *previous; /* previous pe_command_node in the linked
                                  * list */
    pe_ovs_command_t cmd;     /* command to execute */
    uint32_t nr_args;         /* number of args to command */
    void **v_args;            /* array of args to command */
};


/* prototypes */
uint32_t **pe_command_list_processor(pe_command_node_t *, uint32_t);

#endif //PE_COMMANDS_H