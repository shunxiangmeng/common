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

/* ayJSON */
/* JSON parser in C. */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cJSON.h"

#ifdef WIN32
	#pragma warning(disable:4996)
#endif

static const char *ep;

const char *ayJSON_GetErrorPtr() {return ep;}

static int ayJSON_strcasecmp(const char *s1,const char *s2)
{
	if (!s1) return (s1==s2)?0:1;if (!s2) return 1;
	for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0)	return 0;
	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

static void *(*ayJSON_malloc)(size_t sz) = malloc;
static void (*ayJSON_free)(void *ptr) = free;

static char* ayJSON_strdup(const char* str)
{
      size_t len;
      char* copy;

      len = strlen(str) + 1;
      if (!(copy = (char*)ayJSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
}

void ayJSON_InitHooks(ayJSON_Hooks* hooks)
{
    if (!hooks) { /* Reset hooks */
        ayJSON_malloc = malloc;
        ayJSON_free = free;
        return;
    }

	ayJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
	ayJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
}

/* Internal constructor. */
static ayJSON *ayJSON_New_Item()
{
	ayJSON* node = (ayJSON*)ayJSON_malloc(sizeof(ayJSON));
	if (node) memset(node,0,sizeof(ayJSON));
	return node;
}

/* Delete a ayJSON structure. */
void ayJSON_Delete(ayJSON *c)
{
	ayJSON *next;
//	printf("del q\n");
	while (c)
	{
		next=c->next;
		if (!(c->type&ayJSON_IsReference) && c->child) 
			{
			ayJSON_Delete(c->child);
			}
		if (!(c->type&ayJSON_IsReference) && c->valuestring) 
			{
			ayJSON_free(c->valuestring);
			}
		if (c->string) 
			{
			ayJSON_free(c->string);
			}
		ayJSON_free(c);
		c=next;
	}
//	printf("del h\n");
}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(ayJSON *item,const char *num)
{
	double n=0,sign=1,scale=0;int subscale=0,signsubscale=1;

	/* Could use sscanf for this? */
	if (*num=='-') sign=-1,num++;	/* Has sign? */
	if (*num=='0') num++;			/* is zero */
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
	if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
	if (*num=='e' || *num=='E')		/* Exponent? */
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
	
	item->valuedouble=n;
	item->valueint=(int)n;
	item->type=ayJSON_Number;
	return num;
}

/* Render the number nicely from the given item into a string. */
static char *print_number(ayJSON *item)
{
	char *str;
	double d=item->valuedouble;
	if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		str=(char*)ayJSON_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
		if (str) sprintf(str,"%d",item->valueint);
	}
	else
	{
		str=(char*)ayJSON_malloc(64);	/* This is a nice tradeoff. */
		if (str)
		{
			if (fabs(floor(d)-d)<=DBL_EPSILON)			sprintf(str,"%.0f",d);
			else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)	sprintf(str,"%e",d);
			else										sprintf(str,"%f",d);
		}
	}
	return str;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(ayJSON *item,const char *str)
{
	const char *ptr=str+1;char *ptr2;char *out;int len=0;unsigned uc,uc2;
	if (*str!='\"') {ep=str;return 0;}	/* not a string! */
	
	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */
	
	out=(char*)ayJSON_malloc(len+1);	/* This is how long we need for the string, roughly. */
	if (!out) return 0;
	
	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
				case 'b': *ptr2++='\b';	break;
				case 'f': *ptr2++='\f';	break;
				case 'n': *ptr2++='\n';	break;
				case 'r': *ptr2++='\r';	break;
				case 't': *ptr2++='\t';	break;
				case 'u':	 /* transcode utf16 to utf8. */
					sscanf(ptr+1,"%4x",&uc);ptr+=4;	/* get the unicode char. */

					if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	// check for invalid.

					if (uc>=0xD800 && uc<=0xDBFF)	// UTF16 surrogate pairs.
					{
						if (ptr[1]!='\\' || ptr[2]!='u')	break;	// missing second-half of surrogate.
						sscanf(ptr+3,"%4x",&uc2);ptr+=6;
						if (uc2<0xDC00 || uc2>0xDFFF)		break;	// invalid second-half of surrogate.
						uc=0x10000 | ((uc&0x3FF)<<10) | (uc2&0x3FF);
					}

					len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
					
					switch (len) {
						case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 1: *--ptr2 =(uc | firstByteMark[len]);
					}
					ptr2+=len;
					break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;
	item->valuestring=out;
	item->type=ayJSON_String;
	return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str)
{
	const char *ptr;char *ptr2,*out;int len=0;unsigned char token;
	
	if (!str) return ayJSON_strdup("");
	ptr=str;while ((token=*ptr) && ++len) {if (strchr("\"\\\b\f\n\r\t",token)) len++; else if (token<32) len+=5;ptr++;}
	
	out=(char*)ayJSON_malloc(len+3);
	if (!out) return 0;

	ptr2=out;ptr=str;
	*ptr2++='\"';
	while (*ptr)
	{
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
		else
		{
			*ptr2++='\\';
			switch (token=*ptr++)
			{
				case '\\':	*ptr2++='\\';	break;
				case '\"':	*ptr2++='\"';	break;
				case '\b':	*ptr2++='b';	break;
				case '\f':	*ptr2++='f';	break;
				case '\n':	*ptr2++='n';	break;
				case '\r':	*ptr2++='r';	break;
				case '\t':	*ptr2++='t';	break;
				default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* escape and print */
			}
		}
	}
	*ptr2++='\"';*ptr2++=0;
	return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
static char *print_string(ayJSON *item)	{return print_string_ptr(item->valuestring);}

/* Predeclare these prototypes. */
static const char *parse_value(ayJSON *item,const char *value);
static char *print_value(ayJSON *item,int depth,int fmt);
static const char *parse_array(ayJSON *item,const char *value);
static char *print_array(ayJSON *item,int depth,int fmt);
static const char *parse_object(ayJSON *item,const char *value);
static char *print_object(ayJSON *item,int depth,int fmt);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) 
{
	while (in && *in && (unsigned char)*in<=32) in++; 
	return in;
}

/* Parse an object - create a new root, and populate. */
ayJSON *ayJSON_Parse(const char *value)
{
	ayJSON *c=ayJSON_New_Item();
	ep=0;
	if (!c) return 0;       /* memory fail */

	if (!parse_value(c,skip(value))) {ayJSON_Delete(c);return 0;}
	return c;
}

/* Render a ayJSON item/entity/structure to text. */
char *ayJSON_Print(ayJSON *item)				{return print_value(item,0,1);}
char *ayJSON_PrintUnformatted(ayJSON *item)	{return print_value(item,0,0);}

/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(ayJSON *item, const char *value)
{
	if (!value)						return 0;	/* Fail on null. */
	if (!strncmp(value,"null",4))	{ item->type=ayJSON_NULL;  return value+4; }
	if (!strncmp(value,"false",5))	{ item->type=ayJSON_False; return value+5; }
	if (!strncmp(value,"true",4))	{ item->type=ayJSON_True; item->valueint=1;	return value+4; }
	if (*value=='\"')				{ return parse_string(item,value); }
	if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
	if (*value=='[')				{ return parse_array(item,value); }
	if (*value=='{')				{ return parse_object(item,value); }

	ep=value;return 0;	/* failure. */
}

/* Render a value to text. */
static char *print_value(ayJSON *item,int depth,int fmt)
{
	char *out=0;
	if (!item) return 0;
	switch ((item->type)&255)
	{
		case ayJSON_NULL:	out=ayJSON_strdup("null");	break;
		case ayJSON_False:	out=ayJSON_strdup("false");break;
		case ayJSON_True:	out=ayJSON_strdup("true"); break;
		case ayJSON_Number:	out=print_number(item);break;
		case ayJSON_String:	out=print_string(item);break;
		case ayJSON_Array:	out=print_array(item,depth,fmt);break;
		case ayJSON_Object:	out=print_object(item,depth,fmt);break;
	}
	return out;
}

/* Build an array from input text. */
static const char *parse_array(ayJSON *item,const char *value)
{
	ayJSON *child;
	if (*value!='[')	{ep=value;return 0;}	/* not an array! */

	item->type=ayJSON_Array;
	value=skip(value+1);
	if (*value==']') return value+1;	/* empty array. */

	item->child=child=ayJSON_New_Item();
	if (!item->child) return 0;		 /* memory fail */
	value=skip(parse_value(child,skip(value)));	/* skip any spacing, get the value. */
	if (!value) return 0;

	while (*value==',')
	{
		ayJSON *new_item;
		if (!(new_item=ayJSON_New_Item())) return 0; 	/* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_value(child,skip(value+1)));
		if (!value) return 0;	/* memory fail */
	}

	if (*value==']') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */
}

/* Render an array to text */
static char *print_array(ayJSON *item,int depth,int fmt)
{
	char **entries;
	char *out=0,*ptr,*ret;int len=5;
	ayJSON *child=item->child;
	int numentries=0,i=0,fail=0;
	
	/* How many entries in the array? */
	while (child) numentries++,child=child->next;
	/* Allocate an array to hold the values for each */
	entries=(char**)ayJSON_malloc(numentries*sizeof(char*));
	if (!entries) return 0;
	memset(entries,0,numentries*sizeof(char*));
	/* Retrieve all the results: */
	child=item->child;
	while (child && !fail)
	{
		ret=print_value(child,depth+1,fmt);
		entries[i++]=ret;
		if (ret) len+=strlen(ret)+2+(fmt?1:0); else fail=1;
		child=child->next;
	}
	
	/* If we didn't fail, try to malloc the output string */
	if (!fail) out=(char*)ayJSON_malloc(len);
	/* If that fails, we fail. */
	if (!out) fail=1;

	/* Handle failure. */
	if (fail)
	{
		for (i=0;i<numentries;i++) if (entries[i]) ayJSON_free(entries[i]);
		ayJSON_free(entries);
		return 0;
	}
	
	/* Compose the output array. */
	*out='[';
	ptr=out+1;*ptr=0;
	for (i=0;i<numentries;i++)
	{
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
		ayJSON_free(entries[i]);
	}
	ayJSON_free(entries);
	*ptr++=']';*ptr++=0;
	return out;	
}

/* Build an object from the text. */
static const char *parse_object(ayJSON *item,const char *value)
{
	ayJSON *child;
	if (*value!='{')	{ep=value;return 0;}	/* not an object! */
	
	item->type=ayJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	/* empty array. */
	
	item->child=child=ayJSON_New_Item();
	if (!item->child) return 0;
	value=skip(parse_string(child,skip(value)));
	if (!value) return 0;
	child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') {ep=value;return 0;}	/* fail! */
	value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
	if (!value) return 0;
	
	while (*value==',')
	{
		ayJSON *new_item;
		if (!(new_item=ayJSON_New_Item()))	return 0; /* memory fail */
		child->next=new_item;new_item->prev=child;child=new_item;
		value=skip(parse_string(child,skip(value+1)));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') {ep=value;return 0;}	/* fail! */
		value=skip(parse_value(child,skip(value+1)));	/* skip any spacing, get the value. */
		if (!value) return 0;
	}
	
	if (*value=='}') return value+1;	/* end of array */
	ep=value;return 0;	/* malformed. */
}

/* Render an object to text. */
static char *print_object(ayJSON *item,int depth,int fmt)
{
	char **entries=0,**names=0;
	char *out=0,*ptr,*ret,*str;int len=7,i=0,j;
	ayJSON *child=item->child;
	int numentries=0,fail=0;
	/* Count the number of entries. */
	while (child) numentries++,child=child->next;
	/* Allocate space for the names and the objects */
	entries=(char**)ayJSON_malloc(numentries*sizeof(char*));
	if (!entries) return 0;
	names=(char**)ayJSON_malloc(numentries*sizeof(char*));
	if (!names) {ayJSON_free(entries);return 0;}
	memset(entries,0,sizeof(char*)*numentries);
	memset(names,0,sizeof(char*)*numentries);

	/* Collect all the results into our arrays: */
	child=item->child;depth++;if (fmt) len+=depth;
	while (child)
	{
		names[i]=str=print_string_ptr(child->string);
		entries[i++]=ret=print_value(child,depth,fmt);
		if (str && ret) len+=strlen(ret)+strlen(str)+2+(fmt?2+depth:0); else fail=1;
		child=child->next;
	}
	
	/* Try to allocate the output string */
	//printf("%s->%d len = %d\n",__func__,__LINE__,len);
	if (!fail) out=(char*)ayJSON_malloc(len);
	if (!out) fail=1;

	/* Handle failure */
	if (fail)
	{
		for (i=0;i<numentries;i++) {if (names[i]) ayJSON_free(names[i]);if (entries[i]) ayJSON_free(entries[i]);}
		ayJSON_free(names);ayJSON_free(entries);
		return 0;
	}
	
	/* Compose the output: */
	//printf("%s->%d out = %p\n",__func__,__LINE__,out);
	*out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;
	for (i=0;i<numentries;i++)
	{
		if (fmt) for (j=0;j<depth;j++) *ptr++='\t';
		strcpy(ptr,names[i]);ptr+=strlen(names[i]);
		*ptr++=':';if (fmt) *ptr++='\t';
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		if (i!=numentries-1) *ptr++=',';
		if (fmt) *ptr++='\n';*ptr=0;
		ayJSON_free(names[i]);ayJSON_free(entries[i]);
	}
	
	ayJSON_free(names);ayJSON_free(entries);
	if (fmt) for (i=0;i<depth-1;i++) *ptr++='\t';
	*ptr++='}';*ptr++=0;
	//printf("%s->%d end = %p,size = %d,length = %d\n",__func__,__LINE__,ptr,ptr-out,strlen(out));
	return out;	
}

/* Get Array size/item / object item. */
int    ayJSON_GetArraySize(ayJSON *array)							{ayJSON *c=array->child;int i=0;while(c)i++,c=c->next;return i;}
ayJSON *ayJSON_GetArrayItem(ayJSON *array, int item)				{ayJSON *c=array->child;  while (c && item>0) item--,c=c->next; return c;}
ayJSON *ayJSON_GetObjectItem(ayJSON *object,const char *string)	
{
	ayJSON *c=object->child; 
	while (c && ayJSON_strcasecmp(c->string,string)) 
		c=c->next;
	return c;
}

/* Utility for array list handling. */
static void suffix_object(ayJSON *prev,ayJSON *item) 
{
	prev->next=item;
	item->prev=prev;
}
/* Utility for handling references. */
static ayJSON *create_reference(ayJSON *item) {ayJSON *ref=ayJSON_New_Item();if (!ref) return 0;memcpy(ref,item,sizeof(ayJSON));ref->string=0;ref->type|=ayJSON_IsReference;ref->next=ref->prev=0;return ref;}

/* Add item to array/object. */
void   ayJSON_AddItemToArray(ayJSON *array, ayJSON *item)						
{
	ayJSON *c=array->child;
	if (!item) 
		return; 
	if (!c) 
	{
		array->child=item;
	} 
	else 
	{
		while (c && c->next)
			c=c->next; 
		suffix_object(c,item);
	}
}
void   ayJSON_AddItemToObject(ayJSON *object,const char *string,ayJSON *item)	
{
	if (!item) 
		return; 
	if (item->string) 
		ayJSON_free(item->string);
	item->string=ayJSON_strdup(string);
	ayJSON_AddItemToArray(object,item);
}
void	ayJSON_AddItemReferenceToArray(ayJSON *array, ayJSON *item)						{ayJSON_AddItemToArray(array,create_reference(item));}
void	ayJSON_AddItemReferenceToObject(ayJSON *object,const char *string,ayJSON *item)	{ayJSON_AddItemToObject(object,string,create_reference(item));}

ayJSON *ayJSON_DetachItemFromArray(ayJSON *array,int which)			{ayJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return 0;
	if (c->prev) c->prev->next=c->next;if (c->next) c->next->prev=c->prev;if (c==array->child) array->child=c->next;c->prev=c->next=0;return c;}
void   ayJSON_DeleteItemFromArray(ayJSON *array,int which)			{ayJSON_Delete(ayJSON_DetachItemFromArray(array,which));}
ayJSON *ayJSON_DetachItemFromObject(ayJSON *object,const char *string) {int i=0;ayJSON *c=object->child;while (c && ayJSON_strcasecmp(c->string,string)) i++,c=c->next;if (c) return ayJSON_DetachItemFromArray(object,i);return 0;}
void   ayJSON_DeleteItemFromObject(ayJSON *object,const char *string) {ayJSON_Delete(ayJSON_DetachItemFromObject(object,string));}

/* Replace array/object items with new ones. */
void   ayJSON_ReplaceItemInArray(ayJSON *array,int which,ayJSON *newitem)		{ayJSON *c=array->child;while (c && which>0) c=c->next,which--;if (!c) return;
	newitem->next=c->next;newitem->prev=c->prev;if (newitem->next) newitem->next->prev=newitem;
	if (c==array->child) array->child=newitem; else newitem->prev->next=newitem;c->next=c->prev=0;ayJSON_Delete(c);}
void   ayJSON_ReplaceItemInObject(ayJSON *object,const char *string,ayJSON *newitem){int i=0;ayJSON *c=object->child;while(c && ayJSON_strcasecmp(c->string,string))i++,c=c->next;if(c){newitem->string=ayJSON_strdup(string);ayJSON_ReplaceItemInArray(object,i,newitem);}}

/* Create basic types: */
ayJSON *ayJSON_CreateNull()						{ayJSON *item=ayJSON_New_Item();if(item)item->type=ayJSON_NULL;return item;}
ayJSON *ayJSON_CreateTrue()						{ayJSON *item=ayJSON_New_Item();if(item)item->type=ayJSON_True;return item;}
ayJSON *ayJSON_CreateFalse()						{ayJSON *item=ayJSON_New_Item();if(item)item->type=ayJSON_False;return item;}
ayJSON *ayJSON_CreateBool(int b)					{ayJSON *item=ayJSON_New_Item();if(item)item->type=b?ayJSON_True:ayJSON_False;return item;}
ayJSON *ayJSON_CreateNumber(double num)			{ayJSON *item=ayJSON_New_Item();if(item){item->type=ayJSON_Number;item->valuedouble=num;item->valueint=(int)num;}return item;}
ayJSON *ayJSON_CreateString(const char *string)	
{
	ayJSON *item=ayJSON_New_Item();
	if(item)
	{
		item->type=ayJSON_String;
		item->valuestring=ayJSON_strdup(string);
	}
	return item;
}
ayJSON *ayJSON_CreateArray()						{ayJSON *item=ayJSON_New_Item();if(item)item->type=ayJSON_Array;return item;}
ayJSON *ayJSON_CreateObject()						
{
	ayJSON *item=ayJSON_New_Item();
	if(item)
		item->type=ayJSON_Object;
	return item;
}

/* Create Arrays: */
ayJSON *ayJSON_CreateIntArray(int *numbers,int count)				{int i;ayJSON *n=0,*p=0,*a=ayJSON_CreateArray();for(i=0;a && i<count;i++){n=ayJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
ayJSON *ayJSON_CreateFloatArray(float *numbers,int count)			{int i;ayJSON *n=0,*p=0,*a=ayJSON_CreateArray();for(i=0;a && i<count;i++){n=ayJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
ayJSON *ayJSON_CreateDoubleArray(double *numbers,int count)		{int i;ayJSON *n=0,*p=0,*a=ayJSON_CreateArray();for(i=0;a && i<count;i++){n=ayJSON_CreateNumber(numbers[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}
ayJSON *ayJSON_CreateStringArray(const char **strings,int count)	{int i;ayJSON *n=0,*p=0,*a=ayJSON_CreateArray();for(i=0;a && i<count;i++){n=ayJSON_CreateString(strings[i]);if(!i)a->child=n;else suffix_object(p,n);p=n;}return a;}

