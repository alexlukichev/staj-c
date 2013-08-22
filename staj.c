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

   Implementation of STAJ library functions

*/
#include "staj.h"
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

static inline
void init_errno(staj_context* context) {
  if (context->_errno != 0) {
    errno = context->_errno;
  }
}

static inline
void set_parse_error(staj_context* context, int error) {
  context->_errno = STAJ_EPARSE;
  context->parse_error = error;
}

#define UINT_BITS (sizeof(unsigned int)*CHAR_BIT)

static inline
int push_context(staj_context* context, int c) {
  int next_uint = (context->curr_context_stack_ptr+1)/UINT_BITS;
  if (next_uint >= STAJ_MAX_CONTEXT_STACK) {
    context->_errno = STAJ_ENOMEM;
    return -1;
  }
  context->curr_context_stack_ptr ++;
  if (c) {
    context->context_stack[context->curr_context_stack_ptr / UINT_BITS] |=
      1 << (context->curr_context_stack_ptr % UINT_BITS);
  } else {
    context->context_stack[context->curr_context_stack_ptr / UINT_BITS] &=
      ~(1 << (context->curr_context_stack_ptr % UINT_BITS));
  }
  return 0;
}

static inline
int pop_context(staj_context* context, int* c) {
  if (context->curr_context_stack_ptr == -1) {
    context->_errno = STAJ_ESTACK;
    return -1;
  }
  *c = (context->context_stack[context->curr_context_stack_ptr / UINT_BITS] & (1 << (context->curr_context_stack_ptr % UINT_BITS))) != 0;
  context->curr_context_stack_ptr --;
  return 0;
}

static inline
int peek_context(staj_context* context, int* c) {
  if (context->curr_context_stack_ptr == -1) {
    *c = -1;
    return 0;
  }
  *c = (context->context_stack[context->curr_context_stack_ptr / UINT_BITS] & (1 << (context->curr_context_stack_ptr % UINT_BITS))) != 0;
  return 0;
}

static inline
int get_char(staj_context* context, char* c) {
  if (context->current_buffer == -1) {
    return -1;
  } else {
    if (context->buffer_lengths[context->current_buffer] == 0) {
      *c = 0;
      return 0;
    } else {
      *c = context->buffers[context->current_buffer][context->current_pos];
      return 0;
    }
  }
}

static inline
int next_char(staj_context* context, char* c) {
  if (context->_errno != 0) {
    return -1;
  }

  if (context->current_buffer == -1) {
    if (context->current_buffer >= context->max_buffers - 1) {
      context->_errno = STAJ_ENOMEM;
      return -1;
    }
    context->current_buffer ++;
    if (context->next_buffer(
                context->ctx,
                &(context->buffer_lengths[context->current_buffer]),
                &(context->buffers[context->current_buffer])) != 0) {
      return -1;
    }
    context->current_pos = -1;
  }

  if (context->buffer_lengths[context->current_buffer] == 0) {
    *c = 0;
    return 0;
  }

  if (context->current_pos >= context->buffer_lengths[context->current_buffer]-1) {
    if (context->current_buffer >= context->max_buffers - 1) {
      context->_errno = STAJ_ENOMEM;
      return -1;
    }
    context->current_buffer ++;
    if (context->next_buffer(
                context->ctx,
                &(context->buffer_lengths[context->current_buffer]),
                &(context->buffers[context->current_buffer])) != 0) {
      return -1;
    }
    context->current_pos = -1;
  }

  if (context->buffer_lengths[context->current_buffer] == 0) {
    *c = 0;
    return 0;
  }

  *c = context->buffers[context->current_buffer][++(context->current_pos)];
  return 0;
}

static inline
int is_whitespace(char c) {
  return c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D;
}

static inline
int skip_whitespace(staj_context* context, char* c) {
  if (get_char(context, c) != 0) {
    if (next_char(context, c) != 0) {
      return -1;
    }
  }
  while (is_whitespace(*c)) {
    if (next_char(context, c) != 0) {
      return -1;
    }
  }
  return 0;
}

int staj_has_next(staj_context* context) {
  char c;
  if (skip_whitespace(context, &c) != 0) {
    return -1;
  }
  return c != 0;
}

int staj_next(staj_context* context) {
  if (context->_errno != 0) {
    init_errno(context);
    return -1;
  }

  char c;

  int t;
  int in_number = 0;
  int zero = 0;

  if (skip_whitespace(context, &c) != 0) {
    init_errno(context);
    return -1;
  }
  switch (c) {
  case 0: {
    if (context->context != STAJ_CTX_END_DOCUMENT) {
      set_parse_error(context, STAJ_UNEXPECTED_EOF);
      init_errno(context);
      return -1;
    }
    
    context->token = STAJ_EOF;
  } return 0;
  case 0x7B: { /* begin_object */
    if (context->context != STAJ_CTX_START_DOCUMENT &&
        context->context != STAJ_CTX_ARRAY_ITEM &&
        context->context != STAJ_CTX_ARRAY_ITEM_ARRAY_END &&
        context->context != STAJ_CTX_PROPERTY_VALUE) {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }
       
    context->token = STAJ_BEGIN_OBJECT;
    context->start_buffer = context->current_buffer;
    context->start_pos = context->current_pos;
    context->end_buffer = context->current_buffer;
    context->end_pos = context->current_pos;
    context->context = STAJ_CTX_PROPERTY_NAME_OBJECT_END;
    if (push_context(context, 1) != 0) {
      init_errno(context);
      return -1;
    }
    if (next_char(context, &c) != 0) {
      init_errno(context);
      return -1;
    }
  } return 0;
  case 0x5B: { /* begin_array */
    if (context->context != STAJ_CTX_START_DOCUMENT &&
        context->context != STAJ_CTX_ARRAY_ITEM &&
        context->context != STAJ_CTX_ARRAY_ITEM_ARRAY_END &&
        context->context != STAJ_CTX_PROPERTY_VALUE) {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }
       
    context->token = STAJ_BEGIN_ARRAY;
    context->start_buffer = context->current_buffer;
    context->start_pos = context->current_pos;
    context->end_buffer = context->current_buffer;
    context->end_pos = context->current_pos;
    context->context = STAJ_CTX_ARRAY_ITEM_ARRAY_END;
    if (push_context(context, 0) != 0) {
      init_errno(context);
      return -1;
    }
    if (next_char(context, &c) != 0) {
      init_errno(context);
      return -1;
    }
  } return 0;
  case 0x7D: { /* end_object */
    if (context->context != STAJ_CTX_PROPERTY_NAME_OBJECT_END) {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }
    
    context->token = STAJ_END_OBJECT;
    context->start_buffer = context->current_buffer;
    context->start_pos = context->current_pos;
    context->end_buffer = context->current_buffer;
    context->end_pos = context->current_pos;
    if (pop_context(context, &t) != 0) {
      init_errno(context);
      return -1;
    }
    if (peek_context(context, &t) != 0) {
      init_errno(context);
      return -1;
    }
    if (t == 0) {
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) {
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_ARRAY_ITEM;
      } else
      if (c == 0x5D) {
        context->context = STAJ_CTX_ARRAY_ITEM_ARRAY_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }  
    } else
    if (t == 1) {
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) {
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_PROPERTY_NAME;
      } else
      if (c == 0x7D) {
        context->context = STAJ_CTX_PROPERTY_NAME_OBJECT_END;
      } else 
      if (c == 0) {
        context->context = STAJ_CTX_END_DOCUMENT;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else {
      context->context = STAJ_CTX_END_DOCUMENT;
      next_char(context, &c);
    }
  } return 0;
  case 0x5D: { /* end_array */
    if (context->context != STAJ_CTX_ARRAY_ITEM_ARRAY_END) {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }
    
    context->token = STAJ_END_ARRAY;
    context->start_buffer = context->current_buffer;
    context->start_pos = context->current_pos;
    context->end_buffer = context->current_buffer;
    context->end_pos = context->current_pos;
    if (pop_context(context, &t) != 0) {
      init_errno(context);
      return -1;
    }
    if (peek_context(context, &t) != 0) {
      init_errno(context);
      return -1;
    }
    if (t == 0) {
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) {
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_ARRAY_ITEM;
      } else
      if (c == 0x5D) {
        context->context = STAJ_CTX_ARRAY_ITEM_ARRAY_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }  
    } else
    if (t == 1) {
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) {
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_PROPERTY_NAME;
      } else
      if (c == 0x7D) {
        context->context = STAJ_CTX_PROPERTY_NAME_OBJECT_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else {
      context->context = STAJ_CTX_END_DOCUMENT;
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
    }
  }  return 0;
  case 0x22: { /* start string */
    if (context->context != STAJ_CTX_PROPERTY_NAME &&
        context->context != STAJ_CTX_PROPERTY_NAME_OBJECT_END &&
        context->context != STAJ_CTX_PROPERTY_VALUE &&
        context->context != STAJ_CTX_ARRAY_ITEM &&
        context->context != STAJ_CTX_ARRAY_ITEM_ARRAY_END) {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }

    context->start_buffer = context->current_buffer;
    context->start_pos = context->current_pos;    

    int r;
    while ((r = next_char(context, &c)) == 0) {
      if (c == 0x22) { /* quote */
        context->end_buffer = context->current_buffer;
        context->end_pos = context->current_pos;
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        break;
      } 
      if (c == 0x5C) { /* escape */
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        if (c == 0x22 || c == 0x5C || c == 0x2F ||
            c == 0x62 || c == 0x66 || c == 0x6E ||
            c == 0x72 || c == 0x74) {
          if (next_char(context, &c) != 0) {
            init_errno(context);
            return -1;
          }
        } else
        if (c == 0x75) { /* \uXXXX */
          int i;
          for (i=0; i<4; i++) {
            if (next_char(context, &c) != 0) {
              init_errno(context);
              return -1;
            }
            if ((c >= '0' && c <= '9') ||
                (c >= 'a' && c <= 'f') ||
                (c >= 'A' && c <= 'F')) {
            } else {
              set_parse_error(context, STAJ_INVALID_ESCAPE_SEQUENCE);
              init_errno(context);
              return -1;
            }
          }
        } else {
          set_parse_error(context, STAJ_INVALID_ESCAPE_SEQUENCE);
          init_errno(context);
          return -1;
        }
      } else
      if ((c == 0x20) || 
          (c == 0x21) || 
          ((c >= 0x23) && (c <= 0x5B)) ||
          ((c >= 0x5D) && (c <= 0x7F))) {
        // continue
      } else
      if ((c >= 0xC0) && (c <= 0xDF)) {
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        if ((c >= 0x80) && (c <= 0xBF)) {
        } else {
          set_parse_error(context, STAJ_INVALID_UTF8_SEQUENCE);
          init_errno(context);
          return -1;
        }
      } else
      if ((c >= 0xE0) && (c <= 0xEF)) {
        int i;
        for (i=0; i<2; i++) {
          if (next_char(context, &c) != 0) {
            init_errno(context);
            return -1;
          }
          if ((c >= 0x80) && (c <= 0xBF)) {
          } else {
            set_parse_error(context, STAJ_INVALID_UTF8_SEQUENCE);
            init_errno(context);
            return -1;
          }
        }
      } else
      if ((c >= 0xF0) && (c <= 0xF0)) {
        int i;
        for (i=0; i<3; i++) {
          if (next_char(context, &c) != 0) {
            init_errno(context);
            return -1;
          }
          if ((c >= 0x80) && (c <= 0xBF)) {
          } else {
            set_parse_error(context, STAJ_INVALID_UTF8_SEQUENCE);
            init_errno(context);
            return -1;
          }
        }
      } else 
      if (c == 0) {
        set_parse_error(context, STAJ_UNEXPECTED_EOF);
        init_errno(context);
        return -1;
      } else {
        set_parse_error(context, STAJ_INVALID_UTF8_SEQUENCE);
        init_errno(context);
        return -1;
      }
    } // while

    if (r != 0) {
      init_errno(context);
      return -1;
    }

    if (context->context == STAJ_CTX_PROPERTY_NAME ||
        context->context == STAJ_CTX_PROPERTY_NAME_OBJECT_END) {
      context->token = STAJ_PROPERTY_NAME;

      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x3A) {
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_PROPERTY_VALUE;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else
    if (context->context == STAJ_CTX_PROPERTY_VALUE) {
      context->token = STAJ_STRING;

      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) { 
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_PROPERTY_NAME;
      } else 
      if (c == 0x7D) {
        context->context = STAJ_CTX_PROPERTY_NAME_OBJECT_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else
    if (context->context == STAJ_CTX_ARRAY_ITEM ||
        context->context == STAJ_CTX_ARRAY_ITEM_ARRAY_END) {
      context->token = STAJ_STRING;

      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) { 
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_ARRAY_ITEM;
      } else 
      if (c == 0x5D) {
        context->context = STAJ_CTX_ARRAY_ITEM_ARRAY_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }

  } return 0;
  case 'n':
  case 'f':
  case 't': {
    char* word;
    int wlen = 0;
    switch (c) {
    case 'n':
      word = "null";
      wlen = 4;
      context->token = STAJ_NULL;
      break;
    case 'f':
      word = "false";
      wlen = 5;
      context->token = STAJ_BOOLEAN;
      break;
    case 't':
      word = "true";
      wlen = 4;
      context->token = STAJ_BOOLEAN;
      break;
    }
    context->start_buffer = context->current_buffer;
    context->start_pos = context->current_pos;
    int i;
    for (i=1; i<wlen; i++) {
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c != word[i]) {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    }
    context->end_buffer = context->current_buffer;
    context->end_pos = context->current_pos;
    if (next_char(context, &c) != 0) {
      init_errno(context);
      return -1;
    }

    
    if (context->context == STAJ_CTX_PROPERTY_VALUE) {
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) { 
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_PROPERTY_NAME;
      } else 
      if (c == 0x7D) {
        context->context = STAJ_CTX_PROPERTY_NAME_OBJECT_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else
    if (context->context == STAJ_CTX_ARRAY_ITEM ||
        context->context == STAJ_CTX_ARRAY_ITEM_ARRAY_END) {
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) { 
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_ARRAY_ITEM;
      } else 
      if (c == 0x5D) {
        context->context = STAJ_CTX_ARRAY_ITEM_ARRAY_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }

  } return 0;
  case '-': {
    if (context->context == STAJ_CTX_ARRAY_ITEM ||
        context->context == STAJ_CTX_ARRAY_ITEM_ARRAY_END ||
        context->context == STAJ_CTX_PROPERTY_VALUE) {
    } else {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }
    context->token = STAJ_NUMBER;
    context->start_buffer = context->current_buffer;
    context->start_pos = context->current_pos;
    if (next_char(context, &c) != 0) {
      init_errno(context);
      return -1;
    }
    if (c >= '0' && c <= '9') {
      in_number = 1;
    } else {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }
    // no break
  }
  case '0': {
    if (!in_number) {
      if (context->context == STAJ_CTX_ARRAY_ITEM ||
          context->context == STAJ_CTX_ARRAY_ITEM_ARRAY_END ||
          context->context == STAJ_CTX_PROPERTY_VALUE) {
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
      context->token = STAJ_NUMBER;
      context->start_buffer = context->current_buffer;
      context->start_pos = context->current_pos;
      in_number = 1;
    }
    if (c == '0') {
      context->end_buffer = context->current_buffer;
      context->end_pos = context->current_pos;
      zero = 1;
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
    }
    // no break
  }
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9': {
    if (!in_number) {
      if (context->context == STAJ_CTX_ARRAY_ITEM ||
          context->context == STAJ_CTX_ARRAY_ITEM_ARRAY_END ||
          context->context == STAJ_CTX_PROPERTY_VALUE) {
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
      context->token = STAJ_NUMBER;
      context->start_buffer = context->current_buffer;
      context->start_pos = context->current_pos;
      in_number = 1;
    }
    context->end_buffer = context->current_buffer;
    context->end_pos = context->current_pos;
    int r;
    while ((r = next_char(context, &c)) == 0) {
      if (c >= '0' && c <= '9') {
        if (zero) {
          set_parse_error(context, STAJ_INVALID_NUMBER_FORMAT);
          init_errno(context);
          return -1;
        } else {
          context->end_buffer = context->current_buffer;
          context->end_pos = context->current_pos;
        }
      } else {
        break;
      }
    }
    if (r != 0) {
      init_errno(context);
      return -1;
    }
    if (c == 0x2E) {
      while ((r = next_char(context, &c)) == 0) {
        if (c >= '0' && c <= '9') {
          context->end_buffer = context->current_buffer;
          context->end_pos = context->current_pos;
        } else {
          break;
        }
      }
      if (r != 0) {
        init_errno(context);
        return -1;
      }
    }
    if (c == 0x65 || c == 0x45) {
      if (next_char(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2D || c == 0x2B) {
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
      }
      if (c >= '0' && c <= '9') {
        while ((r = next_char(context, &c)) == 0) {
          if (c >= '0' && c <= '9') {
            context->end_buffer = context->current_buffer;
            context->end_pos = context->current_pos;
          } else {
            break;
          }
        }
        if (r != 0) {
          init_errno(context);
          return -1;
        }
      } else {
        set_parse_error(context, STAJ_INVALID_NUMBER_FORMAT);
        init_errno(context);
        return -1;
      }
    }

    if (context->context == STAJ_CTX_PROPERTY_VALUE) {
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) { 
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_PROPERTY_NAME;
      } else 
      if (c == 0x7D) {
        context->context = STAJ_CTX_PROPERTY_NAME_OBJECT_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else
    if (context->context == STAJ_CTX_ARRAY_ITEM ||
        context->context == STAJ_CTX_ARRAY_ITEM_ARRAY_END) {
      if (skip_whitespace(context, &c) != 0) {
        init_errno(context);
        return -1;
      }
      if (c == 0x2C) { 
        if (next_char(context, &c) != 0) {
          init_errno(context);
          return -1;
        }
        context->context = STAJ_CTX_ARRAY_ITEM;
      } else 
      if (c == 0x5D) {
        context->context = STAJ_CTX_ARRAY_ITEM_ARRAY_END;
      } else {
        set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
        init_errno(context);
        return -1;
      }
    } else {
      set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
      init_errno(context);
      return -1;
    }

    } return 0;
  default: {
    set_parse_error(context, STAJ_UNEXPECTED_SYMBOL);
    init_errno(context);
  } return -1;
  }
}
/*
 * staj_get_token
 *
 * Get current token
 *
 * context - StAJ context
 *
 * returns token type
 * in case of error return -1 and sets errno to:
 * STAJ_ENOTOKEN - need to call next() at least once
 * STAJ_ENOMEM - not enough memory allocated during initialization
 * STAJ_EPARSE - parse error
 */
int staj_get_token(staj_context* context) {
  return context->token;
}

int staj_get_length(staj_context* context) {
  int i;
  int l;
  if (context->start_buffer == context->end_buffer) {
    l = context->end_pos - context->start_pos + 1;
  } else {
    l = context->buffer_lengths[context->start_buffer] - context->start_pos;  
    for (i=context->start_buffer+1; i<context->end_buffer; i++) {
      l += context->buffer_lengths[i];
    }
    l += context->end_pos + 1;
  }
  return l;
}

static inline
int min(int a, int b) {
  return (a < b ? a : b);
}

int staj_get_text(staj_context* context, char* buffer, int max) {
  int l;
  if (context->start_buffer == context->end_buffer) {
    l = context->end_pos - context->start_pos + 1;
    strncpy(buffer, context->buffers[context->start_buffer] + context->start_pos, min(l, max));
  } else {
    l = 0;
    int t = context->buffer_lengths[context->start_buffer] - context->start_pos;
    strncpy(buffer+l, 
            context->buffers[context->start_buffer]+context->start_pos, 
            min(max-l, t));
    l += t;
    int i;
    for (i=context->start_buffer+1; i<context->end_buffer; i++) {
      t = context->buffer_lengths[i];
      strncpy(buffer+l,
              context->buffers[i],
              min(max-l, t));
      l += t;
    }
    t = context->end_pos + 1;
    strncpy(buffer+l,
            context->buffers[context->end_buffer],
            min(max-l, t));
    l += t;
  }

  if (l < max) {
    buffer[l] = 0;
  }

  return l;
}

struct __staj_parse_buffer_ctx {
  char* buf;
  int len;
  int rem;
};

static
int __staj_parse_buffer_next_chunk(void* ctx, int* len, char** buf) {
  struct __staj_parse_buffer_ctx* c = (struct __staj_parse_buffer_ctx*) ctx;
  if (c->rem > 0) {
    *buf = c->buf + (c->len - c->rem);
    *len = c->rem;
    c->rem -= c->rem;
  } else {
    *len = 0;
  }
  return 0;
}

int staj_parse_buffer(char* buffer, staj_context** _ctx) {
  int l = strlen(buffer);
  staj_context* ctx = (staj_context*) calloc(1, sizeof(staj_context));
  ctx->next_buffer = &__staj_parse_buffer_next_chunk;
  struct __staj_parse_buffer_ctx* __ctx = (struct __staj_parse_buffer_ctx*) calloc(1, sizeof(struct __staj_parse_buffer_ctx));
  ctx->ctx = __ctx;
  __ctx->buf = buffer;
  __ctx->len = l;
  __ctx->rem = l;
  ctx->max_buffers = 2;
  ctx->buffer_lengths = (int*) calloc(ctx->max_buffers, sizeof(int));
  ctx->buffers = (char**) calloc(ctx->max_buffers, sizeof(char*));
  ctx->current_buffer = -1;
  ctx->curr_context_stack_ptr = -1;
  *_ctx = ctx;
  return 0;
}

int staj_release_context(staj_context* ctx) {
  free(ctx->buffer_lengths);
  free(ctx->buffers);
  free(ctx->ctx);
  free(ctx);
  return 0;
}

int staj_get_parse_error(staj_context* ctx) {
  return ctx->parse_error;
}

int staj_tostr(staj_context* ctx, char* buf, int max) {
  int l = staj_get_text(ctx, buf, max);
  int i = 0;
  int r = 0;
  while (i<l && buf[i] != 0) {
    char c = buf[i++];
    if (c == 0x22) {
    } else
    if (c == 0x5C) {
      if (i >= l) {
        ctx->_errno = STAJ_INVALID_ESCAPE_SEQUENCE;
        init_errno(ctx);
        return -1;
      }
      c = buf[i++];
      if (c == 0x22 || c == 0x5C || c == 0x2F) {
        buf[r++] = c;
      } else
      if (c == 0x62) {
        buf[r++] = 0x08;
      } else
      if (c == 0x66) {
        buf[r++] = 0x0C;
      } else
      if (c == 0x6E) {
        buf[r++] = 0x0A;
      } else
      if (c == 0x72) {
        buf[r++] = 0x0D;
      } else
      if (c == 0x74) {
        buf[r++] = 0x09;
      } else
      if (c == 0x75) {
        char xbuf[5];
        if (l - i < 4) {
          ctx->_errno = STAJ_INVALID_ESCAPE_SEQUENCE;
          init_errno(ctx);
          return -1;
        } else {
          xbuf[0] = buf[i++];
          xbuf[1] = buf[i++];
          xbuf[2] = buf[i++];
          xbuf[3] = buf[i++];
          xbuf[4] = 0;
          char* eptr;
          long int n = strtol(xbuf, &eptr, 16);
          if (*eptr != 0) {
            ctx->_errno = STAJ_INVALID_ESCAPE_SEQUENCE;
            init_errno(ctx);
            return -1;
          }
          if (n >= 0 && n <= 0x7F) {
            buf[r++] = (char) n;
          } else
          if (n >= 0x80 && n <= 0x07FF) {
            buf[r++] = (char) (0xC0 | (n >> 6));
            buf[r++] = (char) (0x80 | (n & 0x3F));
          } else
          if (n >= 0x0800 && n <= 0xFFFF) {
            buf[r++] = (char) (0xE0 | (n >> 12));
            buf[r++] = (char) (0x80 | ((n >> 6) & 0x3F));
            buf[r++] = (char) (0x80 | (n & 0x3F));
          } else {
            ctx->_errno = STAJ_INVALID_ESCAPE_SEQUENCE;
            init_errno(ctx);
            return -1;
          }
        }
      } else {
        ctx->_errno = STAJ_INVALID_ESCAPE_SEQUENCE;
        init_errno(ctx);
        return -1;
      }
    } else {
      // it is safe since every byte in non-ASCII UTF-8 has higher bit set
      buf[r++] = c;
    }
  }

  if (r < max) {
    buf[r] = 0;
  }

  return r;
}

int staj_toi(staj_context* ctx, int* v) {
  long int l;
  if (staj_tol(ctx, &l) != 0) {
    return -1;
  }
  if (l > INT_MAX || l < INT_MIN) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    return -1;
  }
  *v = (int) l;
  return 0;
}

int staj_tol(staj_context* ctx, long int* v) {
  int l = staj_get_length(ctx);
  char* buf = (char*) calloc(l+1, sizeof(char));
  int r = staj_get_text(ctx, buf, l+1);
  if (r < 0) {
    free(buf);
    return -1;
  }
  if (r == 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  char* eptr;
  *v = strtol(buf, &eptr, 10);
  if (*eptr != 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  free(buf);
  return 0;
}

int staj_tof(staj_context* ctx, float* v) {
  int l = staj_get_length(ctx);
  char* buf = (char*) calloc(l+1, sizeof(char));
  int r = staj_get_text(ctx, buf, l+1);
  if (r < 0) {
    free(buf);
    return -1;
  }
  if (r == 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  char* eptr;
  *v = strtof(buf, &eptr);
  if (*eptr != 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  free(buf);
  return 0;
}


int staj_tod(staj_context* ctx, double* v) {
  int l = staj_get_length(ctx);
  char* buf = (char*) calloc(l+1, sizeof(char));
  int r = staj_get_text(ctx, buf, l+1);
  if (r < 0) {
    free(buf);
    return -1;
  }
  if (r == 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  char* eptr;
  *v = strtod(buf, &eptr);
  if (*eptr != 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  free(buf);
  return 0;
}

int staj_told(staj_context* ctx, long double* v) {
  int l = staj_get_length(ctx);
  char* buf = (char*) calloc(l+1, sizeof(char));
  int r = staj_get_text(ctx, buf, l+1);
  if (r < 0) {
    free(buf);
    return -1;
  }
  if (r == 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  char* eptr;
  *v = strtold(buf, &eptr);
  if (*eptr != 0) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    free(buf);
    return -1;
  }
  free(buf);
  return 0;
}

int staj_tob(staj_context* ctx, int* v) {
  int l = staj_get_length(ctx);
  if (l != 4 && l != 5) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    return -1;
  }
  char buf[6] = { 0, 0, 0, 0, 0, 0 };
  int r = staj_get_text(ctx, buf, 6);
  if (r < 0) {
    return -1;
  }
  if (r >= 6) {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    return -1;
  }
  if (strncmp(buf, "true", 6) == 0) {
    *v = 1;
  } else
  if (strncmp(buf, "false", 6) == 0) {
    *v = 0;
  } else {
    ctx->_errno = STAJ_EINVAL;
    init_errno(ctx);
    return -1;
  }
  return 0;
}
