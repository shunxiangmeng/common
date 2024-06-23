/*
  Copyright (c) 2009 Dave Gamble
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef __AYJSON__H__
#define __AYJSON__H__

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ayJSON Types: */
#define ayJSON_False 0
#define ayJSON_True 1
#define ayJSON_NULL 2
#define ayJSON_Number 3
#define ayJSON_String 4
#define ayJSON_Array 5
#define ayJSON_Object 6
	
#define ayJSON_IsReference 256

/* The ayJSON structure: */
typedef struct ayJSON {
	struct ayJSON *next,*prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
	struct ayJSON *child;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */

	int type;					/* The type of the item, as above. */

	char *valuestring;			/* The item's string, if type==ayJSON_String */
	int valueint;				/* The item's number, if type==ayJSON_Number */
	double valuedouble;			/* The item's number, if type==ayJSON_Number */

	char *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} ayJSON;

typedef struct ayJSON_Hooks {
      void *(*malloc_fn) (size_t sz);
      //void *(*malloc_fn)(uint32 sz);
      void (*free_fn)(void *ptr);
} ayJSON_Hooks;

/* Supply malloc, realloc and free functions to ayJSON */
void ayJSON_InitHooks(ayJSON_Hooks* hooks);


/* Supply a block of JSON, and this returns a ayJSON object you can interrogate. Call ayJSON_Delete when finished. */
ayJSON *ayJSON_Parse(const char *value);
/* Render a ayJSON entity to text for transfer/storage. Free the char* when finished. */
char  *ayJSON_Print(ayJSON *item);
/* Render a ayJSON entity to text for transfer/storage without any formatting. Free the char* when finished. */
char  *ayJSON_PrintUnformatted(ayJSON *item);
/* Delete a ayJSON entity and all subentities. */
void   ayJSON_Delete(ayJSON *c);

/* Returns the number of items in an array (or object). */
int	  ayJSON_GetArraySize(ayJSON *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
ayJSON *ayJSON_GetArrayItem(ayJSON *array,int item);
/* Get item "string" from object. Case insensitive. */
ayJSON *ayJSON_GetObjectItem(ayJSON *object,const char *string);

/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when ayJSON_Parse() returns 0. 0 when ayJSON_Parse() succeeds. */
const char *ayJSON_GetErrorPtr();
	
/* These calls create a ayJSON item of the appropriate type. */
ayJSON *ayJSON_CreateNull();
ayJSON *ayJSON_CreateTrue();
ayJSON *ayJSON_CreateFalse();
ayJSON *ayJSON_CreateBool(int b);
ayJSON *ayJSON_CreateNumber(double num);
ayJSON *ayJSON_CreateString(const char *string);
ayJSON *ayJSON_CreateArray();
ayJSON *ayJSON_CreateObject();

/* These utilities create an Array of count items. */
ayJSON *ayJSON_CreateIntArray(int *numbers,int count);
ayJSON *ayJSON_CreateFloatArray(float *numbers,int count);
ayJSON *ayJSON_CreateDoubleArray(double *numbers,int count);
ayJSON *ayJSON_CreateStringArray(const char **strings,int count);

/* Append item to the specified array/object. */
void ayJSON_AddItemToArray(ayJSON *array, ayJSON *item);
void	ayJSON_AddItemToObject(ayJSON *object,const char *string,ayJSON *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing ayJSON to a new ayJSON, but don't want to corrupt your existing ayJSON. */
void ayJSON_AddItemReferenceToArray(ayJSON *array, ayJSON *item);
void	ayJSON_AddItemReferenceToObject(ayJSON *object,const char *string,ayJSON *item);

/* Remove/Detatch items from Arrays/Objects. */
ayJSON *ayJSON_DetachItemFromArray(ayJSON *array,int which);
void   ayJSON_DeleteItemFromArray(ayJSON *array,int which);
ayJSON *ayJSON_DetachItemFromObject(ayJSON *object,const char *string);
void   ayJSON_DeleteItemFromObject(ayJSON *object,const char *string);
	
/* Update array items. */
void ayJSON_ReplaceItemInArray(ayJSON *array,int which,ayJSON *newitem);
void ayJSON_ReplaceItemInObject(ayJSON *object,const char *string,ayJSON *newitem);

#define ayJSON_AddNullToObject(object,name)	ayJSON_AddItemToObject(object, name, ayJSON_CreateNull())
#define ayJSON_AddTrueToObject(object,name)	ayJSON_AddItemToObject(object, name, ayJSON_CreateTrue())
#define ayJSON_AddFalseToObject(object,name)	ayJSON_AddItemToObject(object, name, ayJSON_CreateFalse())
#define ayJSON_AddNumberToObject(object,name,n)	ayJSON_AddItemToObject(object, name, ayJSON_CreateNumber(n))
#define ayJSON_AddStringToObject(object,name,s)	ayJSON_AddItemToObject(object, name, ayJSON_CreateString(s))

#ifdef __cplusplus
}
#endif

#endif
