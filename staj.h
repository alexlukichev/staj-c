/*

   Copyright 2013 (c) Alexander Lukichev

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   ===

   Declarations of STAJ library functions

*/

#ifndef __STAJ_C_H
#define __STAJ_C_H 1

#include "staj_errors.h"

#define STAJ_MAX_CONTEXT_STACK 1024

typedef enum {
  STAJ_BEGIN_OBJECT,
  STAJ_BEGIN_ARRAY,
  STAJ_PROPERTY_NAME,
  STAJ_NUMBER,
  STAJ_STRING,
  STAJ_BOOLEAN,
  STAJ_NULL,
  STAJ_END_OBJECT,
  STAJ_END_ARRAY,
  STAJ_EOF
} staj_token_type;

typedef enum {
  STAJ_CTX_START_DOCUMENT,
  STAJ_CTX_END_DOCUMENT,
  STAJ_CTX_ARRAY_ITEM,
  STAJ_CTX_ARRAY_ITEM_ARRAY_END,
  STAJ_CTX_PROPERTY_NAME,
  STAJ_CTX_PROPERTY_VALUE,
  STAJ_CTX_PROPERTY_NAME_OBJECT_END
} staj_context_type;

typedef struct {
  char* buffer;
  int start;
  int length;
} staj_interval;

typedef struct {
  /*
   * Get next buffer that contains the remainder of the input stream.
   * If result is 0 then buffer was returned successfully. Non-zero
   * result indicates error.
   *
   * *len=0 indicates EOF
   *
   * ctx is passed to the function. This is opaque to StAJ but may
   * help to establish the right context to next_buffer
   */
  int (*next_buffer)(void* ctx, int* len, char** buf);
  void* ctx;
  staj_context_type context;
  int max_buffers;
  int* buffer_lengths;
  char** buffers;
  int current_buffer;
  int current_pos;
  staj_token_type token;
  int _errno;
  int start_buffer;
  int start_pos;
  int end_buffer;
  int end_pos;
  unsigned int context_stack[STAJ_MAX_CONTEXT_STACK];
  int curr_context_stack_ptr;
  int parse_error;
} staj_context;

int staj_has_next(staj_context*);
int staj_next(staj_context*);
int staj_get_token(staj_context*);
int staj_get_length(staj_context*);
int staj_get_text(staj_context*, char*, int);
int staj_get_parse_error(staj_context*);

int staj_tostr(staj_context*, char*, int);
int staj_toi(staj_context*, int*);
int staj_tol(staj_context*, long int*);
// int staj_toll(staj_context*, long long int*);
int staj_tof(staj_context*, float*);
int staj_tod(staj_context*, double*);
int staj_told(staj_context*, long double*);
int staj_tob(staj_context*, int*);

int staj_parse_buffer(char*, staj_context**);
int staj_release_context(staj_context*);

#endif
