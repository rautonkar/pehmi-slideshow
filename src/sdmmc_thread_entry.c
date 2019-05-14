/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
* applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
* this software. By using this software, you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2017 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/

/******************************************************************************
 * This project demonstrates the typical use of the Segment LCD APIs. The
 * project main thread entry initializes the Segment LCD HAL module. The
 * counter counts up and down and its current value is displayed on the screen.
 * Counting up starts with decreased contrast and the countdown runs with
 * increased contrast.
 */
#include <stdio.h>
#include <inttypes.h>
#include <ctype.h>
#include "sdmmc_thread.h"

#include "message.h"

#ifdef SEMIHOSTING
#include "semihosting_cfg.h"
#endif

#ifdef SEMIHOSTING
#ifdef __GNUC__
extern void initialise_monitor_handles (void);
#endif
#endif

/* Pseudo-random number generator - seed */
#define RANDOM_SEED 983

/* Pseudo-random number generator - multiplier */
#define RANDOM_MUL 48271

/* Pseudo-random number generator - modulo divider */
#define RANDOM_MOD 2147483647

/* Max. number of bytes occupied by an integer converted to a string */
#define MAX_BYTES 10

/* ASCII code for '0' */
#define ASCII_ZERO_CODE 48

#define DIMENSION_OF(x)     sizeof(x)/sizeof(x[0])

#define STORAGE_EVENT_CLOSE     (1U)
#define STORAGE_EVENT_OPEN      (2U)

/* USB Device Storage Class - Sense/ASC/ASCQ combinations */
#define UX_MEDIA_WRITE_PROTECTED_MEDIA                              (0x00002707UL)
#define UX_MEDIA_LOGIC_DRIVE_NOT_READY_INITIALIZATION_REQUIRED      (0x00020402UL)
#define UX_MEDIA_UNRECOVERED_READ_ERROR                             (0x00001103UL)
#define UX_MEDIA_WRITE_FAULT                                        (0x00000303UL)
#define UX_MEDIA_UNKNOWN_ERROR                                      (0x00FFFF02UL)

/* File structure */
static FX_FILE g_file;

/* Converts an integer given into a string in decimal system.*/
void int_to_str(CHAR * p_result, uint8_t * p_result_size, uint32_t number);

/* Converts a string in decimal system into an integer */
void str_to_int(uint32_t * p_result, CHAR const * p_str, uint8_t p_str_size);

/* Handles a file system error.*/
void handle_error(UINT const res);

/* Writes a number into the given file.*/
void file_write_number(FX_FILE * p_file, CHAR * p_file_name, uint32_t number);

/* Reads a number from the given file.*/
size_t file_read_number(FX_FILE * p_file, CHAR * p_file_name, uint8_t * read_buffer, size_t read_size);

/* Selects a pseudo-random number and stores in the given pointer.*/
void get_random(int32_t * p_rng);

void get_random(int32_t * p_rng)
{
    *p_rng *= RANDOM_MUL;
    if (*p_rng < 0)
    {
        *p_rng += RANDOM_MOD;
    }
}

void int_to_str(CHAR * p_result, uint8_t * p_result_size, uint32_t number)
{
    CHAR digit;
    *p_result_size = 0u;
    CHAR * p_org_res = p_result;

    /* Convert the integer starting from the least significant digit */
    while(number > 0u)
    {
        digit = (CHAR)(number % 10u);
        *p_result = (CHAR)(digit + (uint8_t)ASCII_ZERO_CODE);
        ++p_result;
        number /= 10u;
        ++*p_result_size;
    }

    /* Reverse the string so it starts with the most significant digit */
    CHAR tmp;
    uint8_t j;
    for(uint8_t i=0;i<(*p_result_size/2u);++i)
    {
        j = (uint8_t)(*p_result_size - (i + 1u));
        tmp = p_org_res[i];
        p_org_res[i] = p_org_res[j];
        p_org_res[j] = tmp;
    }
}

void str_to_int(uint32_t * p_result, CHAR const * p_str, uint8_t p_str_size)
{
    uint8_t digit;
    *p_result = 0;

    /* Convert the string */
    while(p_str_size--)
    {
        digit = (uint8_t)(*p_str - (CHAR)ASCII_ZERO_CODE);
        *p_result *= 10u;
        *p_result += digit;
        ++p_str;
    }
}

uint32_t fileX_error_count = 0;

void handle_error(UINT const res)
{
#ifdef SEMIHOSTING
    printf("Error occurred, code: %x\r\n", res);
#else
    SSP_PARAMETER_NOT_USED(res);
#endif

    fileX_error_count++;
}

void file_write_number(FX_FILE * p_file, CHAR * p_file_name, uint32_t number)
{
    UINT res;
    CHAR intstr[MAX_BYTES];
    uint8_t size;

    /* Convert a number into a string */
    int_to_str(intstr, &size, number);

    /* Open the file for write */
    res = fx_file_open(&g_fx_media0, p_file, p_file_name, FX_OPEN_FOR_WRITE);

    if ((uint8_t)FX_SUCCESS != res)
    {
        handle_error(res);
    }

    /* Clear file contents */
    res = fx_file_truncate(p_file, 0u);

    if ((uint8_t)FX_SUCCESS != res)
    {
        handle_error(res);
    }

    /* Write the converted number to the opened file */
    res = fx_file_write(p_file, intstr, size);

    if ((uint8_t)FX_SUCCESS != res)
    {
        handle_error(res);
    }

    /* Close the file */
    res = fx_file_close(p_file);

    if ((uint8_t)FX_SUCCESS != res)
    {
        handle_error(res);
    }
    
    /* Flush data to physical media */
    res = fx_media_flush(&g_fx_media0);

    if ((uint8_t)FX_SUCCESS != res)
    {
        handle_error(res);
    }
}

size_t file_read_number(FX_FILE * p_file, CHAR * p_file_name, uint8_t * read_buffer, size_t read_size)
{
    UINT res;
    ULONG result = 0;

    /* Open the file for read */
    res = fx_file_open(&g_fx_media0, p_file, p_file_name, FX_OPEN_FOR_READ);

    if ((uint8_t)FX_SUCCESS != res)
    {
        handle_error(res);
        return 0;
    }

    /* Read the file */
    res = fx_file_read(p_file, read_buffer, read_size, &result);

    if (FX_SUCCESS != res)
    {
        handle_error(res);
        return 0;
    }


    /* Close the file */
    res = fx_file_close(p_file);

    if (FX_SUCCESS != res)
    {
        handle_error(res);
        return 0;
    }

    return result;
}

/* SDMMC Thread entry function */

static CHAR return_entry_name[FX_MAX_LONG_NAME_LEN] = {0};

typedef struct st_viewable_media_entry
{
    FX_FILE file_ptr;
    CHAR file_name[FX_MAX_LONG_NAME_LEN];
}viewable_media_entry_t;

static viewable_media_entry_t slides[32];
static viewable_media_entry_t certificates[10];
static const viewable_media_entry_t * db[] =
{
 [DB_DEMO_SLIDES] = &slides,
 [DB_DEMO_SLIDES] = &certificates,
};

FX_LOCAL_PATH local_path;

static size_t file_count(FX_MEDIA * p_fx_media)
{
    UINT res = FX_SUCCESS;

    /* Directory name */
    CHAR * dir_name;

    res = fx_directory_local_path_get( p_fx_media, &dir_name);
    res = fx_directory_name_test( p_fx_media, "slides" );

    if(FX_SUCCESS==res)
    {
        res = fx_directory_local_path_set( p_fx_media, &local_path, "\\slides");
        res = fx_directory_local_path_get( p_fx_media, &dir_name);
        size_t itr = 0;
        for( ; itr < DIMENSION_OF(slides) && (FX_SUCCESS == res); )
        {
            res = fx_directory_next_entry_find( p_fx_media, &return_entry_name[0] );
            res = (res == FX_SUCCESS) ? fx_directory_name_test( p_fx_media, &return_entry_name[0] ): res ;

            if( FX_NOT_DIRECTORY == res )
            {
                size_t len = strnlen(return_entry_name, DIMENSION_OF(return_entry_name));
                if(toupper((int)return_entry_name[len-1]) == 'G' &&
                        toupper((int)return_entry_name[len-2]) == 'N' &&
                        toupper((int)return_entry_name[len-3]) == 'P' )
                {
                    strncat(&slides[itr].file_name[0], dir_name, DIMENSION_OF(slides[itr].file_name));
                    strncat(&slides[itr].file_name[0], "\\", DIMENSION_OF(slides[itr].file_name));
                    strncat(&slides[itr].file_name[0], return_entry_name, DIMENSION_OF(slides[itr].file_name));
                    itr++;
                }
                res = FX_SUCCESS;
            }
        }

        return itr;
    }

    return 0;
}

void sdmmc_thread_entry(void)
{
    uint32_t result;
    uint32_t attributes_ptr = 0;
    uint32_t storage_event_flag = STORAGE_EVENT_OPEN;
    UINT tx_err = TX_SUCCESS;
    sf_message_header_t * p_message = NULL;
    size_t * buffer_size = NULL;

    ssp_err_t status = SSP_SUCCESS;


    /* Initialize pseudo-random number generator */
    int32_t rng = RANDOM_SEED;

#ifdef SEMIHOSTING
#ifdef __GNUC__
    /* If semihosting is enabled, the function initialise_monitor_handles() must be called before using printf() */
    initialise_monitor_handles();
#endif
    /* Get available media space */
    ULONG space_available;
    res = fx_media_space_available(&g_fx_media0, &space_available);

    if((uint8_t)FX_SUCCESS == res)
    {
        printf("Available space: %lu bytes\r\n", space_available);
    }
    else
    {
        printf("Cannot get available space\r\n");
    }
#endif

#ifdef SEMIHOSTING
    printf("Creating directory ");
    printf(dir_name);
    printf("\r\n");
#endif


    while(1)
    {
        if(TX_SUCCESS == tx_err)
        {
            if(storage_event_flag & STORAGE_EVENT_OPEN)
            {
                /* Open FileX */
                extern uint8_t g_media_memory_g_fx_media0[512];
                uint32_t fx_ret_val = fx_media_open (&g_fx_media0, (CHAR *) "g_fx_media0", SF_EL_FX_BlockDriver, &g_sf_el_fx0_cfg,
                                            g_media_memory_g_fx_media0, sizeof(g_media_memory_g_fx_media0));
                if (fx_ret_val != FX_SUCCESS)
                {
                    g_fx_media0_err_callback ((void *) &g_fx_media0, &fx_ret_val);
                }

                storage_event_flag &= ~STORAGE_EVENT_OPEN;

                status = publish_media_png_file_count(SF_MESSAGE_EVENT_NEW_DATA, file_count(&g_fx_media0));
            }
            else if(storage_event_flag & STORAGE_EVENT_CLOSE)
            {
                /* Close FileX */
                UINT fx_err = fx_media_abort(&g_fx_media0);
                fx_err |= fx_media_flush(&g_fx_media0);
                fx_err |= fx_media_close(&g_fx_media0);

                storage_event_flag &= ~STORAGE_EVENT_CLOSE;

                status = publish_media_png_file_count(SF_MESSAGE_EVENT_NEW_DATA, 0);
            }
        }

        tx_err = tx_event_flags_get(&g_storage_event_flags,
                                            (STORAGE_EVENT_OPEN|STORAGE_EVENT_CLOSE),
                                            (UINT)TX_OR_CLEAR,
                                            &storage_event_flag,
                                            1);

        status = g_sf_message0.p_api->pend(g_sf_message0.p_ctrl,
                                           &sdmmc_thread_message_queue,
                                           (sf_message_header_t **) &p_message,
                                           1);

        if( SSP_SUCCESS == status )
        {
            switch(p_message->event_b.class_code)
            {
                case SF_MESSAGE_EVENT_CLASS_DISPLAY:
                {
                    switch (p_message->event_b.code)
                    {
                        case SF_MESSAGE_EVENT_NEW_DATA:
                        {
                            db_t ft = ((display_payload_t * )p_message)->payload.file_type;
                            uint32_t id = ((display_payload_t * )p_message)->payload.file_id < DIMENSION_OF(slides) ? ((display_payload_t * )p_message)->payload.file_id : DIMENSION_OF(slides)-1;
                            uint8_t * p_buffer = ((display_payload_t * )p_message)->payload.buffer;
                            buffer_size = ((display_payload_t * )p_message)->payload.buffer_size;
                            if(id < DIMENSION_OF(slides))
                            {
                                *buffer_size = file_read_number(&slides[id].file_ptr, &slides[id].file_name[0], p_buffer, *buffer_size);
                            }
                        }
                        default:
                            break;
                    }
                }
                break;

                default:
                    break;
            }

            /** Message is processed, so release buffer. */
            sf_message_release_option_t release_status = (*buffer_size) > 0 ? SF_MESSAGE_RELEASE_OPTION_ACK:SF_MESSAGE_RELEASE_OPTION_NAK;
            status = g_sf_message0.p_api->bufferRelease(g_sf_message0.p_ctrl, (sf_message_header_t *) p_message, release_status);

            if (status)
            {
                /** TODO: Handle error. */
            }
        }

        tx_thread_sleep(1);
    }
}



VOID ux_device_msc_media_activate (VOID * arg)
{
    extern TX_THREAD sdmmc_thread;
    UINT tx_err = tx_event_flags_set(&g_storage_event_flags, STORAGE_EVENT_CLOSE, TX_OR);

    tx_err |= tx_thread_suspend(&sdmmc_thread);
    SSP_PARAMETER_NOT_USED(tx_err);
    SSP_PARAMETER_NOT_USED(arg);
}

VOID ux_device_msc_media_deactivate (VOID * arg)
{
    extern TX_THREAD sdmmc_thread;

    NVIC_SystemReset();

    UINT tx_err = tx_event_flags_set(&g_storage_event_flags, STORAGE_EVENT_OPEN, TX_OR);


    SSP_PARAMETER_NOT_USED(tx_err);
    SSP_PARAMETER_NOT_USED(arg);
}

/* USBX Mass Storage Class Media Read User Callback Function */
UINT ux_device_msc_media_read (VOID *storage,
                                ULONG lun,
                                UCHAR *data_pointer,
                                ULONG number_blocks,
                                ULONG lba,
                                ULONG *media_status)
{
    SSP_PARAMETER_NOT_USED(storage);
    SSP_PARAMETER_NOT_USED(lun);

    ssp_err_t ssp_err = g_sf_block_media_sdmmc0.p_api->read(g_sf_block_media_sdmmc0.p_ctrl, data_pointer, lba, number_blocks);

    UINT ux_err = (ssp_err == SSP_SUCCESS) ? UX_SUCCESS:UX_ERROR;

    switch(ssp_err)
    {
        case SSP_ERR_READ_FAILED:
            *media_status = UX_MEDIA_UNRECOVERED_READ_ERROR;
            break;

        case SSP_ERR_NOT_OPEN:
            *media_status = UX_MEDIA_LOGIC_DRIVE_NOT_READY_INITIALIZATION_REQUIRED;
            break;

        case SSP_ERR_WRITE_FAILED:
            *media_status = UX_MEDIA_WRITE_FAULT;
            break;

        case SSP_ERR_INTERNAL:
            *media_status = UX_MEDIA_UNKNOWN_ERROR;
            break;

        default:
            break;
    }

    return ux_err;
}

/* USBX Mass Storage Class Media Write User Callback Function */
UINT ux_device_msc_media_write (VOID *storage,
                                ULONG lun,
                                UCHAR *data_pointer,
                                ULONG number_blocks,
                                ULONG lba,
                                ULONG *media_status)
{
    ssp_err_t ssp_err = SSP_SUCCESS;

    /* Write protection */
    bool sdmmc_wp = true;

    ssp_err = g_sf_block_media_sdmmc0.p_api->ioctl(g_sf_block_media_sdmmc0.p_ctrl, SSP_COMMAND_GET_WRITE_PROTECTED, &sdmmc_wp);

    if (!sdmmc_wp)
    {
        ssp_err = g_sf_block_media_sdmmc0.p_api->write(g_sf_block_media_sdmmc0.p_ctrl, data_pointer, lba, number_blocks);
    }
    else
    {
        *media_status = UX_MEDIA_WRITE_PROTECTED_MEDIA;
        return (UX_ERROR);
    }

    UINT ux_err = (ssp_err == SSP_SUCCESS) ? UX_SUCCESS:UX_ERROR;

    switch(ssp_err)
    {
        case SSP_ERR_READ_FAILED:
            *media_status = UX_MEDIA_UNRECOVERED_READ_ERROR;
            break;

        case SSP_ERR_NOT_OPEN:
            *media_status = UX_MEDIA_LOGIC_DRIVE_NOT_READY_INITIALIZATION_REQUIRED;
            break;

        case SSP_ERR_WRITE_FAILED:
            *media_status = UX_MEDIA_WRITE_FAULT;
            break;

        case SSP_ERR_INTERNAL:
            *media_status = UX_MEDIA_UNKNOWN_ERROR;
            break;

        default:
            break;
    }

    return ux_err;

    SSP_PARAMETER_NOT_USED(storage);
    SSP_PARAMETER_NOT_USED(lun);
}
/* USBX Mass Storage Class Media Status User Callback Function */
UINT ux_device_msc_media_status(VOID *storage, ULONG lun, ULONG media_id, ULONG *media_status)
{
    SSP_PARAMETER_NOT_USED(storage);
    SSP_PARAMETER_NOT_USED(lun);
    SSP_PARAMETER_NOT_USED(media_id);
    SSP_PARAMETER_NOT_USED(media_status);

    return (UX_SUCCESS);
}

