/**                                                                                                      
 * DARE (Direct Access REplication)
 * 
 * State machine abstraction
 *
 * Copyright (c) 2014-2015 ETH-Zurich. All rights reserved.
 * 
 * Author(s): Marius Poke <marius.poke@inf.ethz.ch>
 * 
 */
 
#ifndef DARE_SM_H
#define DARE_SM_H

/* SM types */
#define SM_NULL 1
#define SM_KVS  2
#define SM_FS   3

/* SM command - can be interpreted only by the SM */
struct sm_cmd_t {
    uint16_t    len;
    uint8_t cmd[0];
};
typedef struct sm_cmd_t sm_cmd_t;

#endif /* DARE_SM_H */
