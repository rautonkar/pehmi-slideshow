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
 * Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * File Name    : message.c
 * Version      : 1.0 <- Optional as long as history is shown below
 * Description  : This module solves all the world's problems
 **********************************************************************************************************************/
/***********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : 15.01.2007 1.00     First Release
 **********************************************************************************************************************/

/***********************************************************************************************************************
 Includes   <System Includes> , "Project Includes"
 **********************************************************************************************************************/
#include "message.h"
/***********************************************************************************************************************
 Macro definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 Exported global variables (to be accessed by other files)
 **********************************************************************************************************************/

/***********************************************************************************************************************
 Private global variables and functions
 **********************************************************************************************************************/




ssp_err_t message_display_requests_png(sf_message_event_t event,
                                        db_t type,
                                        uint32_t const fid,
                                        uint8_t * p_data,
                                        size_t * const p_length)
{
    extern void message_display_requests_png_callback(sf_message_callback_args_t * p_args);

    ssp_err_t err = SSP_SUCCESS;

    display_payload_t * p_media_payload_data;
    sf_message_acquire_cfg_t acquire_cfg = {
                                            .buffer_keep = false
    };
    err |= g_sf_message0.p_api->bufferAcquire(g_sf_message0.p_ctrl,
                                             (sf_message_header_t **) &p_media_payload_data,
                                             &acquire_cfg, TX_WAIT_FOREVER);

    memset(p_media_payload_data, 0, sizeof(*p_media_payload_data));
    p_media_payload_data->header.event_b.class_code = SF_MESSAGE_EVENT_CLASS_DISPLAY;
    p_media_payload_data->header.event_b.class_instance = 0;
    p_media_payload_data->header.event_b.code = event;
    p_media_payload_data->payload.file_type = type;
    p_media_payload_data->payload.file_id = fid;
    p_media_payload_data->payload.buffer  = p_data;
    p_media_payload_data->payload.buffer_size = p_length;

    sf_message_post_cfg_t post_cfg =
    {
     .priority = SF_MESSAGE_PRIORITY_NORMAL,
     .p_callback = message_display_requests_png_callback,
     .p_context = (void const*)tx_thread_identify(),
    };

    sf_message_post_err_t post_err =
    {
     .p_queue = NULL,
    };
    err |= g_sf_message0.p_api->post(g_sf_message0.p_ctrl,
                                    (sf_message_header_t const * const)p_media_payload_data,
                                    &post_cfg,
                                    &post_err,
                                    TX_WAIT_FOREVER);

    return err;
}

static void message_media_png_file_count_callback(sf_message_callback_args_t * p_args)
{
    SSP_PARAMETER_NOT_USED(p_args);

    return;
}


ssp_err_t publish_media_png_file_count(sf_message_event_t event, size_t const file_count)
{
    ssp_err_t err = SSP_SUCCESS;

    media_payload_t * p_media_payload_data;
    sf_message_acquire_cfg_t acquire_cfg = {
                                            .buffer_keep = false
    };
    err |= g_sf_message0.p_api->bufferAcquire(g_sf_message0.p_ctrl,
                                             (sf_message_header_t **) &p_media_payload_data,
                                             &acquire_cfg, TX_WAIT_FOREVER);

    memset(p_media_payload_data, 0, sizeof(*p_media_payload_data));
    p_media_payload_data->header.event_b.class_code = SF_MESSAGE_EVENT_CLASS_MEDIA;
    p_media_payload_data->header.event_b.class_instance = 0;
    p_media_payload_data->header.event_b.code = event;
    p_media_payload_data->payload.file_count = file_count;

    sf_message_post_cfg_t post_cfg =
    {
     .priority = SF_MESSAGE_PRIORITY_NORMAL,
     .p_callback = message_media_png_file_count_callback,
     .p_context = (void const*)tx_thread_identify(),
    };

    sf_message_post_err_t post_err =
    {
     .p_queue = NULL,
    };
    err |= g_sf_message0.p_api->post(g_sf_message0.p_ctrl,
                                    (sf_message_header_t const * const)p_media_payload_data,
                                    &post_cfg,
                                    &post_err,
                                    TX_WAIT_FOREVER);

    return err;
}
