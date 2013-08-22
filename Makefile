#   Copyright 2013 (c) Alexander Lukichev
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   ===
#  
#   STAJ library: Makefile
#

CFLAGS=-Wall -O3

LIBSTAJ_SRC=staj.c
LIBSTAJ_OBJ=$(LIBSTAJ_SRC:.c=.o)

TEST_SRC=test_staj.c
TEST_OBJ=$(TEST_SRC:.c=.o)

LIBSTAJ=libstaj.a
TESTEXEC=_test

.PHONY: test clean

all: $(LIBSTAJ) test

test: $(TESTEXEC)
	./$(TESTEXEC)

clean:
	$(RM) -f $(LIBSTAJ_OBJ) $(TEST_OBJ) $(LIBSTAJ) $(TESTEXEC) no-such-file

.c.o:
	$(CC) $(CFLAGS) $< -c -o $@

$(LIBSTAJ): $(LIBSTAJ_OBJ)
	$(AR) rcs $@ $(LIBSTAJ_OBJ)

$(TESTEXEC): $(TEST_OBJ) $(LIBSTAJ)
	$(CC) $(LDFLAGS) $(TEST_OBJ) $(LIBSTAJ) -o $@
