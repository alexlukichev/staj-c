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

   STAJ library : tests

*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "staj.h"

#define assert(t,s,p) if (!(p)) { tests[t] = 0; fprintf(stderr, "%s:%d:test %d failed:%s\n", __FILE__, __LINE__, t, s); } else { tests[t] = 1; }

int tests[100];

char* ERRORS[] = {
  "Unexpected EOF",
  "Unexpected symbol",
  "Invalid escape sequence",
  "Invalid UTF-8 sequence",
  "Invalid number format"
};

char* TEST0 = "{ \"value\" : 123, \"z\" : true }";
char* TEST1 = "{ \"a\" : { \"b\" : [ 1, { \"c\" : [ 2 ] } ] } }";
char* TEST2 = "{ \"value\": \"something\", \"b\" : [ 212, 3 ] }";
char* TEST3 = "{ \"string\": \"\\tsome\\u000A\\\\thing\"  }";
char* TEST4 = "{ \"int\": 123, \"long\" : 123456789123456, \"float\" : 1.23, \"double\" : 1.23e-10, \"bool1\" : true, \"bool2\" : false }";

static inline
void print_parse_error_1(const char* buf, const int pos, const char* file, const int line) {
  char* b = strdup(buf);
  char* a = strdup(buf);
  b[pos] = 0;
  fprintf(stderr, "%s:%d:parse error:%s^%s\n", file, line, b, a+pos);
  free(b);
  free(a);
}

static inline
void print_parse_error_2(const int err, const char* file, const int line) {
  fprintf(stderr, "%s:%d:parse error:%s\n", file, line, ERRORS[err]);
}

void test0(int test) {
  staj_token_type tokens[] = {
    STAJ_BEGIN_OBJECT,
    STAJ_PROPERTY_NAME,
    STAJ_NUMBER,
    STAJ_PROPERTY_NAME,
    STAJ_BOOLEAN,
    STAJ_END_OBJECT
  };
  int ntokens = 6;
  int n = 0;
  int r;
  tests[test] = 1;
  staj_context* ctx;
  staj_parse_buffer(TEST0, &ctx);
  while (staj_has_next(ctx)) {
    r = staj_next(ctx);
    if (r != 0) {
      fprintf(stderr, "%s:%d:errno=%d\n", __FILE__, __LINE__, errno);
      if (r == STAJ_EPARSE) {
        print_parse_error_2(staj_get_parse_error(ctx), __FILE__, __LINE__);
        print_parse_error_1(TEST0, ctx->current_pos, __FILE__, __LINE__);
      }
    }
    assert(test, "staj_next != 0", r == 0);
    if (!tests[test]) goto test0_exit;
    staj_token_type t = staj_get_token(ctx);
    assert(test, "too many tokens", n < ntokens);
    if (!tests[test]) goto test0_exit;
    assert(test, "unexpected token", tokens[n++] == t);
    if (!tests[test]) goto test0_exit;
    if (t == STAJ_EOF) {
      break;
    }
  }
  assert(test, "not enough tokens", n == ntokens);
  if (!tests[test]) goto test0_exit;
  test0_exit:
    staj_release_context(ctx);
}


void test1(int test) {
  staj_token_type tokens[] = {
    STAJ_BEGIN_OBJECT,
      STAJ_PROPERTY_NAME,
      STAJ_BEGIN_OBJECT, 
        STAJ_PROPERTY_NAME,
        STAJ_BEGIN_ARRAY,
          STAJ_NUMBER,
          STAJ_BEGIN_OBJECT,
            STAJ_PROPERTY_NAME,
            STAJ_BEGIN_ARRAY, 
              STAJ_NUMBER,
            STAJ_END_ARRAY,
          STAJ_END_OBJECT,
        STAJ_END_ARRAY,
      STAJ_END_OBJECT,
    STAJ_END_OBJECT
  };
  int ntokens = 15;
  int n = 0;
  int r;
  tests[test] = 1;
  staj_context* ctx;
  staj_parse_buffer(TEST1, &ctx);
  while (staj_has_next(ctx)) {
    r = staj_next(ctx);
    if (r != 0) {
      fprintf(stderr, "%s:%d:errno=%d\n", __FILE__, __LINE__, errno);
      if (r == STAJ_EPARSE) {
        print_parse_error_2(staj_get_parse_error(ctx), __FILE__, __LINE__);
        print_parse_error_1(TEST1, ctx->current_pos, __FILE__, __LINE__);
      }
    }
    assert(test, "staj_next != 0", r == 0);
    if (!tests[test]) goto test1_exit;
    staj_token_type t = staj_get_token(ctx);
    assert(test, "too many tokens", n < ntokens);
    if (!tests[test]) goto test1_exit;
    assert(test, "unexpected token", tokens[n++] == t);
    if (!tests[test]) goto test1_exit;
    if (t == STAJ_EOF) {
      break;
    }
  }
  assert(test, "not enough tokens", n == ntokens);
  if (!tests[test]) goto test1_exit;
  test1_exit:
    staj_release_context(ctx);
}

void test2(int test) {
  staj_token_type tokens[] = {
    STAJ_BEGIN_OBJECT,
      STAJ_PROPERTY_NAME,
      STAJ_STRING,
      STAJ_PROPERTY_NAME,
      STAJ_BEGIN_ARRAY,
        STAJ_NUMBER,
        STAJ_NUMBER,
      STAJ_END_ARRAY,
    STAJ_END_OBJECT
  };
  char* text[] = {
    NULL,
    "\"value\"",
    "\"something\"",
    "\"b\"",
    NULL,
    "212",
    "3",
    NULL,
    NULL
  };
  int ntokens = 9;
  int n = 0;
  int r;
  tests[test] = 1;
  staj_context* ctx;
  staj_parse_buffer(TEST2, &ctx);
  while (staj_has_next(ctx)) {
    r = staj_next(ctx);
    if (r != 0) {
      fprintf(stderr, "%s:%d:errno=%d\n", __FILE__, __LINE__, errno);
      if (r == STAJ_EPARSE) {
        print_parse_error_2(staj_get_parse_error(ctx), __FILE__, __LINE__);
        print_parse_error_1(TEST1, ctx->current_pos, __FILE__, __LINE__);
      }
    }
    assert(test, "staj_next != 0", r == 0);
    if (!tests[test]) goto test2_exit;
    staj_token_type t = staj_get_token(ctx);
    assert(test, "too many tokens", n < ntokens);
    if (!tests[test]) goto test2_exit;
    assert(test, "unexpected token", tokens[n] == t);
    if (!tests[test]) goto test2_exit;
    if (text[n] != NULL) {
      int l = staj_get_length(ctx);
      char* buf = (char*) calloc(l+1, sizeof(char));
      r = staj_get_text(ctx, buf, l+1);
      assert(test, "r != l", r == l);
      if (r != l) {
        free(buf);
        if (!tests[test]) goto test2_exit;
      }
      if (strcmp(text[n], buf) != 0) {
        assert(test, "token value mismatch", 0);
        fprintf(stderr, "%s:%d:staj_get_length=%d\n", __FILE__, __LINE__, l);
        fprintf(stderr, "%s:%d:expected:%s:extracted:%s\n",
          __FILE__, __LINE__, text[n], buf);
        free(buf);
        if (!tests[test]) goto test2_exit;
      }
      free(buf);
    }
    n++;
    if (t == STAJ_EOF) {
      break;
    }    
  }
  assert(test, "not enough tokens", n == ntokens);
  if (!tests[test]) goto test2_exit;
  test2_exit:
    staj_release_context(ctx);
}

void test3(int test) {
  tests[test] = 1;
  staj_context* ctx;
  staj_parse_buffer(TEST3, &ctx);
  int r;
  while (staj_has_next(ctx)) {
    r = staj_next(ctx);
    if (r != 0) {
      fprintf(stderr, "%s:%d:errno=%d\n", __FILE__, __LINE__, errno);
      if (r == STAJ_EPARSE) {
        print_parse_error_2(staj_get_parse_error(ctx), __FILE__, __LINE__);
        print_parse_error_1(TEST1, ctx->current_pos, __FILE__, __LINE__);
      }
    }
    assert(test, "staj_next != 0", r == 0);
    if (!tests[test]) goto test3_exit;
    staj_token_type t = staj_get_token(ctx);
    if (t == STAJ_STRING) {
      int l = staj_get_length(ctx);
      char* buf = (char*) calloc(l+1, sizeof(char));
      r = staj_tostr(ctx, buf, l+1);
      assert(test, "staj_tostr < 0", r >= 0);
      if (!tests[test]) { free(buf); goto test3_exit; }
      assert(test, "string doesn\'t match", strcmp("\tsome\x0a\\thing", buf) == 0);
      free(buf);
      if (!tests[test]) goto test3_exit;
    }
  }
  test3_exit:
    staj_release_context(ctx);
}

void test4(int test) {
  tests[test] = 1;
  staj_context* ctx;
  staj_parse_buffer(TEST4, &ctx);
  int r;
  char pname[50];
  pname[0] = 0;
  while (staj_has_next(ctx)) {
    r = staj_next(ctx);
    if (r != 0) {
      fprintf(stderr, "%s:%d:errno=%d\n", __FILE__, __LINE__, errno);
      if (r == STAJ_EPARSE) {
        print_parse_error_2(staj_get_parse_error(ctx), __FILE__, __LINE__);
        print_parse_error_1(TEST1, ctx->current_pos, __FILE__, __LINE__);
      }
    }
    assert(test, "staj_next != 0", r == 0);
    if (!tests[test]) goto test4_exit;
    staj_token_type t = staj_get_token(ctx);
    if (t == STAJ_PROPERTY_NAME) {
      assert(test, "staj_tostr < 0", staj_tostr(ctx, pname, 50) >= 0);
      if (!tests[test]) goto test4_exit;
    }
    if (t == STAJ_NUMBER) {
      if (strcmp(pname, "int") == 0) {
        int v1;
        assert(test, "staj_toi != 0", staj_toi(ctx, &v1) == 0);
        if (!tests[test]) goto test4_exit;
        assert(test, "v1 != 123", v1 == 123);
        if (!tests[test]) goto test4_exit;
      } else
      if (strcmp(pname, "long") == 0) {
        long int v2;
        assert(test, "staj_tol != 0", staj_tol(ctx, &v2) == 0);
        if (!tests[test]) goto test4_exit;
        assert(test, "v2 != 123456789123456", v2 == 123456789123456);
        if (!tests[test]) goto test4_exit;
      } else
      if (strcmp(pname, "float") == 0) {
        float v3;
        assert(test, "staj_tof != 0", staj_tof(ctx, &v3) == 0);
        if (!tests[test]) goto test4_exit;
        assert(test, "v3 != 1.23", fabsf(v3  - 1.23) < 0.00001);
        if (!tests[test]) goto test4_exit;
      } else
      if (strcmp(pname, "double") == 0) {
        double v4;
        assert(test, "staj_tod != 0", staj_tod(ctx, &v4) == 0);
        if (!tests[test]) goto test4_exit;
        assert(test, "v4 != 1.23e-10", fabs(v4 - 1.23e-10) < 1e-15);
        if (!tests[test]) goto test4_exit;
      } else 
      if (strcmp(pname, "bool1") == 0) {
        int v5;
        assert(test, "staj_tob != 0", staj_tob(ctx, &v5) == 0);
        if (!tests[test]) goto test4_exit;
        assert(test, "v5 != true", v5 == 1);
        if (!tests[test]) goto test4_exit;
      } else
      if (strcmp(pname, "bool2") == 0) {
        int v6;
        assert(test, "staj_tob != 0", staj_tob(ctx, &v6) == 0);
        if (!tests[test]) goto test4_exit;
        assert(test, "v6 != true", v6 == 0);
        if (!tests[test]) goto test4_exit;
      }
    }
  }
  test4_exit:
    staj_release_context(ctx);
}
int main() {
  int test = 0;
  test0(test++);
  test1(test++);
  test2(test++);
  test3(test++);
  test4(test++);

  int good = 1;
  int i;
  for (i=0; i<test; i++) {
    printf("test %d: %s\n", i, tests[i] ? "pass" : "FAIL");
    if (!tests[i]) {
      good = 0;
    }
  }
  
  return (good ? 0 : 1);
}
