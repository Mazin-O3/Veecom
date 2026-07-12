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

#include "interperter.h"
#include "tokenizer.h"
#include "utility.h"
#include <string.h>
#include <stdlib.h>

VariableType_t expr(void);

static struct interperter self;

void interperter_init(peek_func peek, poke_func poke)
{
    interperter_reset();

    for (int i = 0; i < UBASIC_MAX_PROGRAM_LINES; ++i)
    {
        self.program_lines[i].idx = -1;
    }

    self.peek_function = peek;
    self.poke_function = poke;

    self.bytes_used = 0;
    self.cur_free_lidx = 0;
}

void interperter_reset(void)
{
    self.for_stack_ptr = 0;
    self.gosub_stack_ptr = 0;
    self.program_counter = 0;
    self.finished = 0;
}

int interperter_get_line_num(char *text)
{
    tokenizer_init(text);

    if (tokenizer_token() == TOKENIZER_NUMBER)
    {
        return tokenizer_num();
    }

    return -1;
}

int interperter_indexed_line_empty()
{
    tokenizer_next();
    return tokenizer_token() == TOKENIZER_CR;
}

void set_variable(int varnum, VariableType_t value)
{
    if (varnum >= 0 && varnum <= MAX_VARNUM)
    {
        self.variables[varnum] = value;
    }
}

VariableType_t get_variable(int varnum)
{
    if (varnum >= 0 && varnum <= MAX_VARNUM)
    {
        return self.variables[varnum];
    }

    return 0;
}

void accept(int token)
{
    if (token != tokenizer_token())
    {
        // tokenizer_error_dma_write();
        return;
    }

    tokenizer_next();
}

int varfactor(void)
{
    int r = get_variable(tokenizer_variable_num());
    accept(TOKENIZER_VARIABLE);
    return r;
}

int factor(void)
{
    int r;

    switch (tokenizer_token())
    {
    case TOKENIZER_NUMBER:
        r = tokenizer_num();
        accept(TOKENIZER_NUMBER);
        break;
    case TOKENIZER_LEFTPAREN:
        accept(TOKENIZER_LEFTPAREN);
        r = expr();
        accept(TOKENIZER_RIGHTPAREN);
        break;
    default:
        r = varfactor();
        break;
    }

    return r;
}

int term(void)
{
    int f1 = factor();
    int op = tokenizer_token();

    while (op == TOKENIZER_ASTR ||
           op == TOKENIZER_SLASH ||
           op == TOKENIZER_MOD)
    {
        tokenizer_next();
        int f2 = factor();

        switch (op)
        {
        case TOKENIZER_ASTR:
            f1 = f1 * f2;
            break;
        case TOKENIZER_SLASH:
            f1 = f1 / f2;
            break;
        case TOKENIZER_MOD:
            f1 = f1 % f2;
            break;
        }
        op = tokenizer_token();
    }

    return f1;
}

VariableType_t expr(void)
{
    int t1 = term();
    int op = tokenizer_token();

    while (op == TOKENIZER_PLUS ||
           op == TOKENIZER_MINUS ||
           op == TOKENIZER_AND ||
           op == TOKENIZER_OR)
    {
        tokenizer_next();
        int t2 = term();

        switch (op)
        {
        case TOKENIZER_PLUS:
            t1 = t1 + t2;
            break;
        case TOKENIZER_MINUS:
            t1 = t1 - t2;
            break;
        case TOKENIZER_AND:
            t1 = t1 & t2;
            break;
        case TOKENIZER_OR:
            t1 = t1 | t2;
            break;
        }
        op = tokenizer_token();
    }

    return t1;
}

int relation(void)
{
    int r1 = expr();
    int op = tokenizer_token();

    while (op == TOKENIZER_LT ||
           op == TOKENIZER_GT ||
           op == TOKENIZER_EQ)
    {
        tokenizer_next();
        int r2 = expr();

        switch (op)
        {
        case TOKENIZER_LT:
            r1 = r1 < r2;
            break;
        case TOKENIZER_GT:
            r1 = r1 > r2;
            break;
        case TOKENIZER_EQ:
            r1 = r1 == r2;
            break;
        }
        op = tokenizer_token();
    }

    return r1;
}

struct line_index index_find(int linenum)
{
    struct line_index find = {.idx = -1};

    for (int i = 0; i < self.cur_free_lidx + 1; ++i)
    {
        if (self.program_lines[i].line_number == linenum)
        {
            find = self.program_lines[i];
            break;
        }
    }

    return find;
}

void index_insert(struct line_index lidx)
{
    for (int i = 0; i < self.cur_free_lidx + 1; ++i)
    {
        if (self.program_lines[i].idx == -1)
        {
            self.program_lines[i] = lidx;
            return;
        }

        if (self.program_lines[i].line_number > lidx.line_number)
        {
            struct line_index temp = self.program_lines[i];
            temp.idx = lidx.idx;
            self.program_lines[i] = lidx;
            self.program_lines[i].idx = i;
            lidx = temp;
        }
    }
}

void interperter_add_line(int linenum, char *text, int len)
{
    if (self.cur_free_lidx >= UBASIC_MAX_PROGRAM_LINES)
    {
        return;
    }

    int idx = index_find(linenum).idx;

    if (idx != -1)
    {
        memcpy(self.program_lines[idx].text, text, len);
        return;
    }

    struct line_index new_lidx = {
        .line_number = linenum,
        .idx = self.cur_free_lidx++,
        .len = len,
    };

    memcpy(new_lidx.text, text, len);
    index_insert(new_lidx);
    self.bytes_used += len;
}

void goto_statement(void)
{
    accept(TOKENIZER_GOTO);
    self.program_counter = index_find(tokenizer_num()).idx;
}

void print_statement(void)
{
    accept(TOKENIZER_PRINT);

    do
    {
        if (tokenizer_token() == TOKENIZER_STRING)
        {
            int strlen = tokenizer_string(self.string, UBASIC_MAX_STRINGLEN);
            dma_nwrite(self.string, strlen);
            tokenizer_next();
        }
        else if (tokenizer_token() == TOKENIZER_COMMA)
        {
            put_char(' ');
            tokenizer_next();
        }
        else if (tokenizer_token() == TOKENIZER_SEMICOLON)
        {
            tokenizer_next();
        }
        else
        {
            VariableType_t e = expr();
            itoa(e, self.numstr, 10);
            dma_nwrite(self.numstr, UBASIC_MAX_NUMLEN);
        }
    } while (tokenizer_token() != TOKENIZER_CR &&
             tokenizer_token() != TOKENIZER_ENDOFINPUT);

    put_char('\n');
    tokenizer_next();
}

void if_statement(void)
{
    int r;

    accept(TOKENIZER_IF);

    r = relation();

    accept(TOKENIZER_THEN);

    if (r)
    {
        interperter_execute();
    }

    accept(TOKENIZER_CR);
}

void let_statement(void)
{
    int var = tokenizer_variable_num();

    accept(TOKENIZER_VARIABLE);
    accept(TOKENIZER_EQ);
    set_variable(var, expr());
    accept(TOKENIZER_CR);
}

void gosub_statement(void)
{
    int linenum;

    accept(TOKENIZER_GOSUB);
    linenum = tokenizer_num();

    accept(TOKENIZER_NUMBER);
    accept(TOKENIZER_CR);

    if (self.gosub_stack_ptr < UBASIC_MAX_GOSUB_STACK_DEPTH)
    {
        self.gosub_stack[self.gosub_stack_ptr] = self.program_counter;
        self.gosub_stack_ptr++;
        self.program_counter = index_find(linenum).idx;
    }
}

void return_statement(void)
{
    accept(TOKENIZER_RETURN);

    if (self.gosub_stack_ptr > 0)
    {
        self.gosub_stack_ptr--;
        self.program_counter = self.gosub_stack[self.gosub_stack_ptr];
    }
}

void next_statement(void)
{
    int var;

    accept(TOKENIZER_NEXT);
    var = tokenizer_variable_num();
    accept(TOKENIZER_VARIABLE);

    if (self.for_stack_ptr > 0 && var == self.for_stack[self.for_stack_ptr - 1].for_variable)
    {
        set_variable(var, get_variable(var) + 1);

        if (get_variable(var) <= self.for_stack[self.for_stack_ptr - 1].to)
        {
            self.program_counter = self.for_stack[self.for_stack_ptr - 1].line_after_for;
        }
        else
        {
            self.for_stack_ptr--;
            accept(TOKENIZER_CR);
        }
    }
    else
    {
        accept(TOKENIZER_CR);
    }
}

void for_statement(void)
{
    int for_variable, to;

    accept(TOKENIZER_FOR);
    for_variable = tokenizer_variable_num();

    accept(TOKENIZER_VARIABLE);
    accept(TOKENIZER_EQ);

    set_variable(for_variable, expr());
    accept(TOKENIZER_TO);

    to = expr();
    accept(TOKENIZER_CR);

    if (self.for_stack_ptr < UBASIC_MAX_FOR_STACK_DEPTH)
    {
        self.for_stack[self.for_stack_ptr].line_after_for = self.program_counter;
        self.for_stack[self.for_stack_ptr].for_variable = for_variable;
        self.for_stack[self.for_stack_ptr].to = to;
        self.for_stack_ptr++;
    }
}

void peek_statement(void)
{
    VariableType_t peek_addr;
    VariableType_t var;

    accept(TOKENIZER_PEEK);
    peek_addr = expr();

    accept(TOKENIZER_COMMA);
    var = tokenizer_variable_num();

    accept(TOKENIZER_VARIABLE);
    accept(TOKENIZER_CR);

    set_variable(var, self.peek_function(peek_addr));
}

void poke_statement(void)
{
    VariableType_t poke_addr;
    VariableType_t value;

    accept(TOKENIZER_POKE);
    poke_addr = expr();

    accept(TOKENIZER_COMMA);
    value = expr();

    accept(TOKENIZER_CR);

    self.poke_function(poke_addr, value);
}

void end_statement(void)
{
    accept(TOKENIZER_END);
    self.finished = 1;
}

void new_statement(void)
{
    accept(TOKENIZER_NEW);
    accept(TOKENIZER_CR);

    for (int i = 0; i < self.cur_free_lidx; ++i)
    {
        self.program_lines[i].idx = -1;
    }

    self.cur_free_lidx = 0;
    self.bytes_used = 0;
}

void run_statement(void)
{
    accept(TOKENIZER_RUN);
    accept(TOKENIZER_CR);

    if (self.bytes_used == 0)
    {
        return;
    }

    interperter_reset();

    while (self.program_counter < self.cur_free_lidx)
    {
        if (self.finished)
        {
            return;
        }

        struct line_index lidx = self.program_lines[self.program_counter++];

        if (lidx.idx == -1)
        {
            continue;
        }

        tokenizer_init(lidx.text);
        accept(TOKENIZER_NUMBER);
        interperter_execute();
    }
}

void list_statement(void)
{
    if (self.bytes_used == 0)
    {
        put_char('\n');
        return;
    }

    accept(TOKENIZER_LIST);

    int start = 0;
    int end = self.cur_free_lidx - 1;

    switch (tokenizer_token())
    {
    case TOKENIZER_NUMBER:
        start = index_find(tokenizer_num()).idx;
        tokenizer_next();

        if (tokenizer_token() == TOKENIZER_CR)
        {
            end = start;
        }
        else if (tokenizer_token() == TOKENIZER_MINUS)
        {
            tokenizer_next();

            if (tokenizer_token() == TOKENIZER_NUMBER)
            {
                end = index_find(tokenizer_num()).idx;
            }
        }
        break;
    case TOKENIZER_MINUS:
        tokenizer_next();
        if (tokenizer_token() == TOKENIZER_NUMBER)
        {
            end = index_find(tokenizer_num()).idx;
        }
        break;
    default:
        break;
    }

    if (start == -1 || end == -1)
    {
        put_char('\n');
        return;
    }

    for (int i = start; i <= end; ++i)
    {
        if (self.program_lines[i].idx == -1)
        {
            continue;
        }

        dma_nwrite(self.program_lines[i].text, self.program_lines[i].len);
    }
}

void fre_statement(void)
{
    accept(TOKENIZER_FRE);
    accept(TOKENIZER_CR);

    sprintf(self.string, "%d uBASIC BYTES FREE\n");
    dma_write(self.string);
}

void interperter_remove_line(int linenum)
{
    int idx = index_find(linenum).idx;

    if (idx == -1)
    {
        return;
    }

    self.program_lines[idx].idx = -1;
    self.bytes_used -= self.program_lines[idx].len;
}

uint16_t interperter_bytes_free(void)
{
    return UBASIC_FREE_BYTES - self.bytes_used;
}

void interperter_execute(void)
{
    int token;

    token = tokenizer_token();

    switch (token)
    {
    case TOKENIZER_PRINT:
        print_statement();
        break;
    case TOKENIZER_IF:
        if_statement();
        break;
    case TOKENIZER_GOTO:
        goto_statement();
        break;
    case TOKENIZER_GOSUB:
        gosub_statement();
        break;
    case TOKENIZER_RETURN:
        return_statement();
        break;
    case TOKENIZER_FOR:
        for_statement();
        break;
    case TOKENIZER_PEEK:
        peek_statement();
        break;
    case TOKENIZER_POKE:
        poke_statement();
        break;
    case TOKENIZER_NEXT:
        next_statement();
        break;
    case TOKENIZER_END:
        end_statement();
        break;
    case TOKENIZER_LET:
        accept(TOKENIZER_LET);
        /* Fall through. */
    case TOKENIZER_VARIABLE:
        let_statement();
        break;
    case TOKENIZER_NEW:
        new_statement();
        break;
    case TOKENIZER_RUN:
        run_statement();
        break;
    case TOKENIZER_LIST:
        list_statement();
        break;
    case TOKENIZER_FRE:
        fre_statement();
        break;
    default:
        self.finished = 1;
        break;
    }
}