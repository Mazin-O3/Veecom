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

#include "ubasic.h"
#include "interperter.h"
#include "utility.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

VariableType_t peek(VariableType_t addr);
void poke(VariableType_t addr, VariableType_t val);

static char input_buff[UBASIC_PROGRAM_LINE_WIDTH];
static int char_count;

extern char _text_end;
#define FORBIDDEN_MEM_AREA (uintptr_t)(&_text_end)

void ubasic_init(void)
{
  interperter_init(peek, poke);
  memset(input_buff, 0, sizeof(input_buff));
  char_count = 0;
  dma_nwrite(UBASIC_STARTUP_MESSAGE, sizeof(UBASIC_STARTUP_MESSAGE));
}

void ubasic_run(void)
{
  char key = toupper(read_key());
  put_char(key);

  if (key == 12)
  {
    return;
  }

  if (key == 8 && char_count > 0)
  {
    input_buff[--char_count] = '\0';
    return;
  }

  if (char_count < UBASIC_PROGRAM_LINE_WIDTH)
  {
    input_buff[char_count++] = key;
  }

  if (key == 10)
  {
    int linenum = interperter_get_line_num(input_buff);

    if (linenum == -1)
    {
      interperter_reset();
      interperter_execute();
      dma_write("READY.\n");
    }
    else if (interperter_indexed_line_empty())
    {
      interperter_remove_line(linenum);
    }
    else
    {
      interperter_add_line(linenum, input_buff, char_count);
    }

    char_count = 0;
  }
}

VariableType_t peek(VariableType_t addr)
{
  return *(volatile VariableType_t *)(addr);
}

void poke(VariableType_t addr, VariableType_t val)
{
  if (addr <= FORBIDDEN_MEM_AREA)
  {
    return;
  }

  *(volatile VariableType_t *)(addr) = val;
}