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

   STAJ library: error codes

*/

#ifndef __STAJ_ERRORS_H
#define __STAJ_ERRORS_H 1

#define STAJ_UNEXPECTED_EOF 		0
#define STAJ_UNEXPECTED_SYMBOL		1
#define STAJ_INVALID_ESCAPE_SEQUENCE	2
#define STAJ_INVALID_UTF8_SEQUENCE	3
#define STAJ_INVALID_NUMBER_FORMAT	4

#define STAJ_EPARSE			-1
#define STAJ_ENOMEM			-2
#define STAJ_ESTACK			-3
#define STAJ_EINVAL			-4

#endif
