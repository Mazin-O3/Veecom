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

#include "tokenizer.h"
#include "ubasic_config.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static char const *ptr, *nextptr;

struct keyword_token
{
  char *keyword;
  int token;
};

static int current_token = TOKENIZER_ERROR;

static const struct keyword_token keywords[] = {
    {"LET", TOKENIZER_LET},
    {"PRINT", TOKENIZER_PRINT},
    {"IF", TOKENIZER_IF},
    {"THEN", TOKENIZER_THEN},
    {"FOR", TOKENIZER_FOR},
    {"TO", TOKENIZER_TO},
    {"NEXT", TOKENIZER_NEXT},
    {"GOTO", TOKENIZER_GOTO},
    {"GOSUB", TOKENIZER_GOSUB},
    {"RETURN", TOKENIZER_RETURN},
    {"REM", TOKENIZER_REM},
    {"PEEK", TOKENIZER_PEEK},
    {"POKE", TOKENIZER_POKE},
    {"END", TOKENIZER_END},
    {"NEW", TOKENIZER_NEW},
    {"RUN", TOKENIZER_RUN},
    {"LIST", TOKENIZER_LIST},
    {"FRE", TOKENIZER_FRE},
    {NULL, TOKENIZER_ERROR},
};

static int singlechar(void)
{
  switch (*ptr)
  {
  case '\n':
    return TOKENIZER_CR;
  case ',':
    return TOKENIZER_COMMA;
  case ';':
    return TOKENIZER_SEMICOLON;
  case '+':
    return TOKENIZER_PLUS;
  case '-':
    return TOKENIZER_MINUS;
  case '&':
    return TOKENIZER_AND;
  case '|':
    return TOKENIZER_OR;
  case '*':
    return TOKENIZER_ASTR;
  case '/':
    return TOKENIZER_SLASH;
  case '%':
    return TOKENIZER_MOD;
  case '(':
    return TOKENIZER_LEFTPAREN;
  case ')':
    return TOKENIZER_RIGHTPAREN;
  case '<':
    return TOKENIZER_LT;
  case '>':
    return TOKENIZER_GT;
  case '=':
    return TOKENIZER_EQ;
  }

  return 0;
}

static int get_next_token(void)
{
  struct keyword_token const *kt;
  int i = 0;
  
  if (*ptr == 0)
  {
    return TOKENIZER_ENDOFINPUT;
  }

  if (isdigit(ptr[i++]))
  {
    for (; i < UBASIC_MAX_NUMLEN; ++i)
    {
      if (!isdigit(ptr[i]))
      {
        nextptr = ptr + i;
        return TOKENIZER_NUMBER;
      }
    }

    // Number is too long
    return TOKENIZER_ERROR;
  }

  if (singlechar())
  {
    nextptr = ptr + 1;
    return singlechar();
  }

  if (*ptr == '"')
  {
    nextptr = ptr;
    do
    {
      ++nextptr;
    } while (*nextptr != '"');

    ++nextptr;
    return TOKENIZER_STRING;
  }

  for (kt = keywords; kt->keyword != NULL; ++kt)
  {
    if (strncmp(ptr, kt->keyword, strlen(kt->keyword)) == 0)
    {
      nextptr = ptr + strlen(kt->keyword);
      return kt->token;
    }
  }

  if (*ptr >= 'A' && *ptr <= 'Z')
  {
    nextptr = ptr + 1;
    return TOKENIZER_VARIABLE;
  }

  return TOKENIZER_ERROR;
}

void tokenizer_goto(const char *program)
{
  ptr = program;
  current_token = get_next_token();
}

void tokenizer_init(const char *program)
{
  tokenizer_goto(program);
  current_token = get_next_token();
}

int tokenizer_token(void)
{
  return current_token;
}

void tokenizer_next(void)
{

  if (tokenizer_finished())
  {
    return;
  }

  ptr = nextptr;

  while (*ptr == ' ')
  {
    ++ptr;
  }

  current_token = get_next_token();

  if (current_token == TOKENIZER_REM)
  {
    while (!(*nextptr == '\n' || tokenizer_finished()))
    {
      ++nextptr;
    }

    if (*nextptr == '\n')
    {
      ++nextptr;
    }
    tokenizer_next();
  }
}

VariableType_t tokenizer_num(void)
{
  return atoi(ptr);
}

int tokenizer_string(char *dest, int len)
{
  char *string_end;
  int string_len;

  string_end = strchr(ptr + 1, '"');

  if (string_end == NULL)
  {
    return 0;
  }

  string_len = string_end - ptr - 1;

  if (len < string_len)
  {
    string_len = len;
  }

  memcpy(dest, ptr + 1, string_len);

  dest[string_len] = '\0';

  return string_len;
}

void tokenizer_error_print(void)
{
  // DEBUG_PRINTF("tokenizer_error_print: '%s'\n", ptr);
}

int tokenizer_finished(void)
{
  return *ptr == 0 || current_token == TOKENIZER_ENDOFINPUT;
}

int tokenizer_variable_num(void)
{
  return *ptr - 'A';
}

char const *tokenizer_pos(void)
{
  return ptr;
}
