/*
 * Copyright (c) 2006, Adam Dunkels
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __INTERPERTER_H__
#define __INTERPERTER_H__

#include <stdint.h>
#include "ubasic_version.h"

#define MAX_VARNUM 26

struct for_state
{
    int line_after_for;
    int for_variable;
    int to;
};

struct line_index
{
    char text[UBASIC_PROGRAM_LINE_WIDTH];
    int line_number;
    int idx;
    int len;
};

typedef VariableType_t (*peek_func)(VariableType_t);
typedef void (*poke_func)(VariableType_t, VariableType_t);

struct interperter
{
    struct line_index program_lines[UBASIC_MAX_PROGRAM_LINES];
    struct for_state for_stack[UBASIC_MAX_FOR_STACK_DEPTH];
    VariableType_t variables[MAX_VARNUM];
    char string[UBASIC_MAX_STRINGLEN];
    char numstr[UBASIC_MAX_NUMLEN];
    int for_stack_ptr;
    int gosub_stack[UBASIC_MAX_GOSUB_STACK_DEPTH];
    int gosub_stack_ptr;
    int cur_free_lidx;
    int program_counter;
    int finished;
    int bytes_used;
    peek_func peek_function;
    poke_func poke_function;
};

void interperter_init(peek_func peek, poke_func poke);
void interperter_reset(void);
void interperter_execute(void);
int interperter_get_line_num(char *text);
int interperter_indexed_line_empty(void);
void interperter_add_line(int linenum, char *text, int len);
void interperter_remove_line(int linenum);
uint16_t interperter_bytes_free(void);

#endif