
#ifndef WIN32 // only on Linux Platform

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#include "ay_sdk.h"

/*扫描提取wifi加密类型*/
void  Iwlist_encrypt_type_thread(int args,const char *ssid,const char *pwd)
{
    char  buf_cmd[1024]; 
    char  buf_path[256]; 
    char  *ptemp = NULL;
    int   len, i;
    FILE  *fhd = NULL;
    uint8  encrypto;

    WIFI_CallBack p_callbk_wifi = ay_psdk->cbfuncs.pWifiCbfc;
    st_ay_wifi_ctrl *pwifi = &ay_psdk->wifi;

    if (pwifi->is_request_encrypt == 0 )
    {
	DEBUGF("no encrypt request\n");					
	(*p_callbk_wifi)((char*)ssid, (char *)pwd, Ulk802_11AuthModeAuto);	
	return;
    }
    for ( i = 0 ; i < args ; i++ )
    {
	if ( strlen(pwifi->wireless_name ) == 0 )
	{
	    snprintf(buf_cmd,sizeof(buf_cmd),"iwlist ra0 scan|grep %s -A 15 > %s/wifilist", ssid, ay_psdk->devinfo.rw_path);// 
	}
	else
	{
	    snprintf(buf_cmd,sizeof(buf_cmd),"iwlist %s scan|grep %s -A 15 > %s/wifilist", pwifi->wireless_name,  ssid, ay_psdk->devinfo.rw_path);// 
	}
	system(buf_cmd);

	sleep(1);
	snprintf(buf_path,sizeof(buf_path),"%s/wifilist",ay_psdk->devinfo.rw_path);
	fhd = fopen(buf_path, "r");
	if ( fhd != NULL )
	{
	    len = fread(buf_cmd, 1, 1024, fhd);
	    if(len > 0)
	    {
		ptemp = buf_cmd;

		encrypto = Ulk802_11AuthModeAuto;

		if (strstr(ptemp , "Encryption key:off") != NULL)
		{
		    encrypto = Ulk802_11AuthModeOpen;
		}
		else if ( strstr(ptemp , "Encryption key:on")  &&
			strstr(ptemp , "WPA") == NULL)
		{
		    encrypto = Ulk802_11AuthModeWEPOPEN;
		}																									
		else if ( strstr(ptemp , "WPA1") &&strstr(ptemp , "WPA2")  && strstr(ptemp , "PSK"))
		{
		    if(strstr(ptemp , "CCMP") && strstr(ptemp , "TKIP"))
		    {
			encrypto = Ulk802_11AuthModeWPA1PSKWPA2PSKTKIPAES;
		    }
		    else if ( strstr(ptemp , "CCMP") )
		    {
			encrypto = Ulk802_11AuthModeWPA1PSKWPA2PSKAES; 
		    }
		    else if ( strstr(ptemp , "TKIP") )
		    {
			encrypto = Ulk802_11AuthModeWPA1PSKWPA2PSKTKIP;
		    }	
		    else if ( strstr(ptemp , "EAP") )
		    {
			encrypto = Ulk802_11AuthModeWPA1EAPWPA2EAP;
		    }						
		}					
		else if ( strstr(ptemp , "WPA2") != NULL && strstr(ptemp , "PSK"))
		{
		    if(strstr(ptemp , "CCMP") && strstr(ptemp , "TKIP"))
		    {
			encrypto = Ulk802_11AuthModeWPA2PSKTKIPAES;
		    }
		    else if ( strstr(ptemp , "CCMP") )
		    {
			encrypto = Ulk802_11AuthModeWPA2PSKAES; 
		    }
		    else if ( strstr(ptemp , "TKIP") )
		    {
			encrypto = Ulk802_11AuthModeWPA2PSKTKIP;
		    }
		    else if ( strstr(ptemp , "EAP") )
		    {
			encrypto = Ulk802_11AuthModeWPA2EAP;
		    }
		}
		else if ( strstr(ptemp , "WPA")  && strstr(ptemp , "PSK"))
		{
		    if(strstr(ptemp , "CCMP") && strstr(ptemp , "TKIP"))
		    {
			encrypto = Ulk802_11AuthModeWPAPSKTKIPAES;
		    }
		    else if ( strstr(ptemp , "CCMP") )
		    {
			encrypto = Ulk802_11AuthModeWPAPSKAES; 
		    }
		    else if ( strstr(ptemp , "TKIP") )
		    {
			encrypto = Ulk802_11AuthModeWPAPSKTKIP;
		    }
		    else if ( strstr(ptemp , "EAP") )
		    {
			encrypto = Ulk802_11AuthModeWPAEAP;
		    }
		}

		if ( Ulk802_11AuthModeAuto != encrypto )
		{
		    DEBUGF("parse encrypt %s,%s, %d\n",(char*)ssid, (char *)pwd, encrypto);	
		    (*p_callbk_wifi)((char*)ssid, (char *)pwd, encrypto);	
		    break;
		}
		else
		{
		    DEBUGF("no search %s \n", ssid);
		}
		pwifi->ulk_encrypto = encrypto;
	    }
	    fclose(fhd);
	}
    }
}

#ifdef AY_ENABLE_QRDECODE

#include "zbar.h"
#include "jpeglib.h"

struct jpeg_decompress_struct cinfo;
static zbar_image_scanner_t *scanner = NULL;
static zbar_image_t *image = NULL;
static void *raw = NULL;//全局图片原生数据

struct my_error_mgr
{
    struct jpeg_error_mgr pub;	/* "public" fields */
    jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */
    METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
    /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
    my_error_ptr myerr = (my_error_ptr) cinfo->err;

    /* Always display the message. */
    /* We could postpone this until after returning, if we chose. */
    (*cinfo->err->output_message) (cinfo);

    /* Return control to the setjmp point */
    longjmp(myerr->setjmp_buffer, 1);
}

static int Decode_JPEG_data (int *width, int *height, char *pic_data, int datalen)
{
    /* This struct contains the JPEG decompression parameters and pointers to
     * working space (which is allocated as needed by the JPEG library).
     */

    /* We use our private extension JPEG error handler.
     * Note that this struct must live as long as the main JPEG parameter
     * struct, to avoid dangling-pointer problems.
     */
    struct my_error_mgr jerr;

    /* More stuff */

    JSAMPARRAY buffer;		/* Output row buffer */
    int row_stride;		/* physical row width in output buffer */

    /* In this example we want to open the input file before doing anything else,
     * so that the setjmp() error recovery below can assume the file is open.
     * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
     * requires it in order to read binary files.
     */


    /* Step 1: allocate and initialize JPEG decompression object */

    /* We set up the normal JPEG error routines, then override error_exit. */
    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    /* Establish the setjmp return context for my_error_exit to use. */
    if (setjmp(jerr.setjmp_buffer))
    {
	/* If we get here, the JPEG code has signaled an error.
	 * We need to clean up the JPEG object, close the input file, and return.
	 */
	jpeg_destroy_decompress(&cinfo);
	//        fclose(infile);
	return 0;
    }
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress(&cinfo);

    /* Step 2: specify data source (eg, a file) */

    //    jpeg_stdio_src(&cinfo, infile);

    jpeg_stdio_buffer_src(&cinfo, (UINT8*)pic_data, datalen);

    /* Step 3: read file parameters with jpeg_read_header() */

    (void) jpeg_read_header(&cinfo, TRUE);
    /* We can ignore the return value from jpeg_read_header since
     *   (a) suspension is not possible with the stdio data source, and
     *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
     * See libjpeg.doc for more info.
     */

    /* Step 4: set parameters for decompression */

    /* In this example, we don't need to change any of the defaults set by
     * jpeg_read_header(), so we do nothing here.
     */
    /* Step 5: Start decompressor */
    (void) jpeg_start_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* We may need to do some setup of our own at this point before reading
     * the data.  After jpeg_start_decompress() we have the correct scaled
     * output image dimensions available, as well as the output colormap
     * if we asked for color quantization.
     * In this example, we need to make an output work buffer of the right size.
     */
    /* JSAMPLEs per row in output buffer */
    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)
	((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    /* Step 6: while (scan lines remain to be read) */
    /*           jpeg_read_scanlines(...); */

    /* Here we use the library's state variable cinfo.output_scanline as the
     * loop counter, so that we don't have to keep track ourselves.
     */
    *width = cinfo.output_width;
    *height = cinfo.output_height;

    raw = malloc((*width) * (*height));
    int   ioffset = 0;
    int 	t; 
    JSAMPLE  tmp;

    //printf("width=%d, height=%d\n",cinfo.output_width, cinfo.output_height);
    //	sleep(2);
    while (cinfo.output_scanline < cinfo.output_height)
    {
	/* jpeg_read_scanlines expects an array of pointers to scanlines.
	 * Here the array is only one element long, but you could ask for
	 * more than one scanline at a time if that's more convenient.
	 */
	(void) jpeg_read_scanlines(&cinfo, buffer, 1);
	/* Assume put_scanline_someplace wants a pointer and sample count. */
	//put_scanline_someplace(buffer[0], row_stride);

	//把rgb的jpeg的数据转换为灰度图像数据压缩成单个字节的
	for(  t = 0; t < *width*3; t+=3)
	{           
	    //			tmp = (buffer[0][t]*30 + buffer[0][t+1]*59 + buffer[0][t+2]*11 + 50) / 100;
	    tmp = buffer[0][t];
	    memcpy(((char *)raw + ioffset), (char *)&tmp, 1);
	    ioffset += 1;
	}
    }

    /* Step 7: Finish decompression */


    (void) jpeg_finish_decompress(&cinfo);
    /* We can ignore the return value since suspension is not possible
     * with the stdio data source.
     */

    /* Step 8: Release JPEG decompression object */

    /* This is an important step since it will release a good deal of memory. */
    jpeg_destroy_decompress(&cinfo);

    /* After finish_decompress, we can close the input file.
     * Here we postpone it until after no more JPEG errors are possible,
     * so as to simplify the setjmp error logic above.  (Actually, I don't
     * think that jpeg_destroy can do an error exit, but why assume anything...)
     */
    //    fclose(infile);

    /* At this point you may want to check to see whether any corrupt-data
     * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
     */
    /* And we're done! */
    return 1;
}

#define PREFIX_SSID "S:"
#define PREFIX_CODE "P:"
#define SEPARATOR_1 "\r\n"
#define SEPARATOR_2 "\n"
int Ulu_SDK_Push_ScreenShot(Picture_info *pic_info)
{
    /* obtain image data */
    int width = 0, height = 0;
    char  *ptemp1, *ptemp2, *psep;
    char  tmp_buf[128];
    char   ssid[33];
    char   pwd[33];

    /* create a reader */
    scanner = zbar_image_scanner_create();
    /* configure the reader */
    zbar_image_scanner_set_config(scanner, 0, ZBAR_CFG_ENABLE, 1);

    Decode_JPEG_data( &width, &height, (char*)pic_info->pic_data, pic_info->pic_data_len);

    /* wrap image data */
    image = zbar_image_create();
    zbar_image_set_format(image, *(int*)"Y800");
    zbar_image_set_size(image, width, height);	
    zbar_image_set_data(image, raw, width * height, zbar_image_free_data);//自动释放内存

    /* scan the image for barcodes */
    zbar_scan_image(scanner, image);

    /* extract results */
    const zbar_symbol_t *symbol = zbar_image_first_symbol(image);
    for(; symbol; symbol = zbar_symbol_next(symbol))
    {
	/* do something useful with results */
	zbar_symbol_type_t typ = zbar_symbol_get_type(symbol);
	const char *data = zbar_symbol_get_data(symbol);
	DEBUGF("decoded %s symbol \"%s\"\n", zbar_get_symbol_name(typ), data);

	ptemp1 = (char *)data;
	if ( strstr(ptemp1, PREFIX_SSID) != NULL)
	{
		psep = SEPARATOR_1;
	    ptemp2 = strstr(ptemp1, psep);
		if(ptemp2==NULL) {
			psep = SEPARATOR_2;
			ptemp2 = strstr(ptemp1, psep);
		}
	    if ( ptemp2 != NULL )
	    {
		memset(tmp_buf, 0, sizeof(tmp_buf));
		memcpy(tmp_buf, ptemp1 + strlen(PREFIX_SSID), ptemp2 - ptemp1 - strlen(PREFIX_SSID));

		memset(ssid, 0, sizeof(ssid));
		strncpy(ssid, tmp_buf,sizeof(ssid)-1);
		DEBUGF("ssid:%s\n",  ssid);

		//----------继续解析密码------------------
		ptemp1 = ptemp2 + strlen(psep);
		if ( strstr(ptemp1, PREFIX_CODE) != NULL)
		{
		    ptemp2 = strstr(ptemp1, SEPARATOR_1);
		    if ( ptemp2 != NULL )
		    {
			memset(tmp_buf, 0, sizeof(tmp_buf));				
			memcpy(tmp_buf, ptemp1 + strlen(PREFIX_CODE), ptemp2 - ptemp1 - strlen(PREFIX_CODE));
			DEBUGF("pass:%s\n", tmp_buf);
		    }
		    else
		    {
			strcpy(tmp_buf, ptemp1 + strlen(PREFIX_CODE));
			DEBUGF("pass:%s\n", tmp_buf);
		    }
		    memset(pwd, 0, sizeof(pwd));
		    strncpy(pwd, tmp_buf,sizeof(pwd)-1);
		    //扫描加密类型并回调
		    Iwlist_encrypt_type_thread(3,ssid,pwd);
		}
	    }
	}
    }

    //DEBUGF("decode finish\n");

    /* clean up */
    zbar_image_destroy(image);
    zbar_image_scanner_destroy(scanner);

    return 0;
}
#else
int Ulu_SDK_Push_ScreenShot(Picture_info *pic_info)
{
    return -1;
}
#endif

#endif

