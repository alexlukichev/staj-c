StAJ-C
======

Streaming API for JSON in ANSI C

Building
========

Building should be easy on most POSIX systems, just run make in the directory. 
Overriding compilation and linking flags is done with CFLAGS and LDFLAGS parameters.

The result of the build is the object archive libstaj.a.

Using
=====

The idea of StAJ is similar to StAX: you create a stream of token over a character 
buffer (or set of buffers) and the retrieve tokens one by one using the API.
The API also provides the ability to get the textual representation of the tokens,
converting it into standard C type representation, and obtain error information.

Initialization of the context:

    #include "staj.h"
    ...
    char* buf = ... ;
    staj_context* ctx;
    staj_parse_buffer(buf, &ctx);

To initialize the context a variety of functions may be used, depending on
the input data layout. The following ones are provided out of the box:

- `staj_parse_buffer(char* buffer, staj_context** context)` - parse a null-terminated string

Walking through the tokens:

    int r;
    while (staj_has_next(ctx)) {
      r = staj_next(ctx);
      if (r != 0) {
        /* handle error */
        break;
      }
      switch (staj_get_token(ctx)) {
      ...
      }
    }

Moving in the token stream:

- `staj_has_next(staj_context* context)` - check whether there are any non-spaces 
  in the input stream remaining
- `staj_next(staj_context* context)` - move by one token. The token properties are
  obtained using additional functions. If no errors occured then the result of the
  function is 0. Otherwise the result is -1 and the error number is stored in errno.
  See [Error Handling](#error-handling).

Getting token values:

- `staj_get_length(staj_context* context)` - get the length of the literal representation
  of the token. E.g. the length of the opening curly brace will be 1 and the length
  of the string literal includes start and end quotes
- `staj_get_text(staj_context* context, char* buffer, int max_length)` - store the
  literal representation of the token in the buffer. Up to `max_length` characters
  are stored. The *literal* representation is stored, i.e. how the token
  exactly appears in the input JSON document. In case of success the function
  returns the length of the resulting string in the buffer. Otherwise -1 and sets
  errno. See [Error Handling](#error-handling).
- `staj_tostr(staj_context* context, char* buffer, int max_length)` - store the
  decoded (unescaped) value of the string or property name token in the buffer. `max_length`
  is the maximum length in the buffer. In case of success the function returns the length
  of the resulting string. Otherwise -1 and sets errno. See [Error Handling](#error-handling).
- `staj_toi(staj_context* context, int* value)` - store the int value  of the
  number token in `value`. In case of success the result is 0, otherwise -1. The error
  code is in errno. See [Error Handling](#error-handling).
- `staj_tol(staj_context* context, long int* value)` - store the long int value  of the
  number token in `value`. In case of success the result is 0, otherwise -1. The error
  code is in errno. See [Error Handling](#error-handling).
- `staj_tof(staj_context* context, float* value)` - store the float value  of the
  number token in `value`. In case of success the result is 0, otherwise -1. The error
  code is in errno. See [Error Handling](#error-handling).
- `staj_tod(staj_context* context, double* value)` - store the double precision float 
  value  of the number token in `value`. In case of success the result is 0, otherwise 
  -1. The error code is in errno. See [Error Handling](#error-handling).
- `staj_told(staj_context* context, long double* value)` - store the long double precision 
  float value  of the number token in `value`. In case of success the result is 0, 
  otherwise -1. The error code is in errno. See [Error Handling](#error-handling).
- `staj_tob(staj_context* context, int* value)` - store the boolean value of the 
  boolean token in `value`. In case of success the result is 0, 
  otherwise -1. The error code is in errno. See [Error Handling](#error-handling).

Releasing context:

    staj_release_context(ctx);

Usually release function is coupled with the context allocation function because
the structure of the memory buffers may be different for different types of
input data (e.g. single buffer vs. multiple buffers, etc.)

## Error Handling


