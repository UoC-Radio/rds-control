/*
 * Copyright (C) 2013 Nick Kossifidis
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * uecp.c -	Support for UECP compatible encoders
 *		(untested - WiP)
 */

/**
 * TODO:
 *	* Set comm mode to bidirectional to support get methods
 *	* Implement get methods + ACK
 *	* Test it on a device
 */

#include <stdint.h>	/* For sized integers */
#include <errno.h>	/* For error numbers */
#include <string.h>	/* For memset() */
#include <stdio.h>	/* For printf(...) */
#include <arpa/inet.h> 	/* For ntohs */
#include "rds.h"
#include "uecp.h"


/******************\
* HELPER FUNCTIONS *
\******************/

/**
 * uecp_crc16_ccit - Calculate the CRC of a UECP data frame
 * @data: the buffer containing the data frame
 * @len: the buffer's length
 */
static uint16_t
uecp_crc16_ccitt(unsigned char* data, uint8_t len)
{
	int i = 0;
	uint16_t crc = 0xFFFF;

	for (i=0; i < len; i++)
	{
		crc = (unsigned char)(crc >> 8) | (crc << 8);
		crc ^= data[i];
		crc ^= (unsigned char)(crc & 0xff) >> 4;
		crc ^= (crc << 8) << 4;
		crc ^= ((crc & 0xff) << 4) << 1;
	}

	return ((crc ^= 0xFFFF) & 0xFFFF);
}

/**
 * uecp_is_global_mec -	Check for UECP Message Elements that don't relate
 * 			to a specific Data Set / Programme number
 * @mec: The Message Element Code to check
 */
static int
uecp_is_global_mec(uint8_t mec)
{
	switch(mec) {
	case UECP_MEC_RTC:
	case UECP_MEC_CT:
	case UECP_MEC_DSN_SELECT:
	case UECP_MEC_RDSON:
	case UECP_MEC_SET_SITE_ADDR:
	case UECP_MEC_SET_ENC_ADDR:
	case UECP_MEC_SET_COMM_MODE:
		return 1;
	default:
		return 0;
	}
}

/**
 * uecp_data_frame_to_buf - Write out the UECP data frame on a buffer
 * @data: pointer to &uecp_data_frame to parse
 * @buf: pre-allocated buffer to fill
 */
static int
uecp_data_frame_to_buf(struct uecp_data_frame *data,
				unsigned char *buf)
{
	uint8_t msg_fields = 0;
	uint8_t len = 0;
	int i = 0;
	uint16_t crc = 0;
	struct uecp_message *msg = &data->msg;

	buf[len++] = (data->addr & 0xFF00) >> 8;
	buf[len++] = (data->addr & 0xFF);
	buf[len++] = data->seq;

	/* Empty message
	 * XXX: Needs further research */
	if(data->msg_len == 0) {
		buf[len++] = data->msg_len;
		goto finalize;
	}

	buf[len++] = data->msg_len;
	buf[len++] = msg->mec;

	/* Do we have DSN/PSN ? */
	if(!uecp_is_global_mec(msg->mec)) {
		buf[len++] = msg->dsn;
		buf[len++] = msg->psn;
		msg_fields = 3; /* mec + dsn + psn */
	} else
		msg_fields = 1; /* mec only */

	/* Should we add mel_len ? */
	if(msg->mel_len != UECP_MSG_MEL_NA)
		buf[len++] = msg->mel_len;

	/* Add message data */
	for(i = 0; i < data->msg_len - msg_fields; i++)
		buf[len++] = msg->mel_data[i];

finalize:
	crc = uecp_crc16_ccitt(buf, len);

	buf[len++] = (crc & 0xFF00) >> 8;
	buf[len++] = (crc & 0xFF);

	/* Put crc on data frame struct just
	 * in case we need it later on */
	data->crc = crc;

	return len;
}

/**
 * uecp_send_frame_to_enc - Send a UECP data frame to encoder
 * @enc: pointer to &struct rds_encoder
 * @data_frame: pointer to the &struct uecp_data_frame to send
 */
static int
uecp_send_frame_to_enc(struct rds_encoder *enc,
			struct uecp_data_frame *data_frame)
{
	int ret = 0;
	int i = 0;
	int fd = 0;
	unsigned char buf[UECP_DF_MAX_LEN];

	data_frame->addr = enc->addr;
	data_frame->seq = 0;

	ret = uecp_data_frame_to_buf(data_frame, buf);
	if(ret < 0)
		return ret;


	rds_send_byte(enc, UECP_DF_START_BYTE);

	/* Perform byte-stuffing on output */
	for(i = 0; i < ret && i < UECP_DF_MAX_LEN; i++) {

		if(buf[i] == 0xFD) {
			rds_send_byte(enc, 0xFD);
			rds_send_byte(enc, 0x00);
		} else if(buf[i] == 0xFE) {
			rds_send_byte(enc, 0xFD);
			rds_send_byte(enc, 0x01);
		} else if(buf[i] == 0xFF) {
			rds_send_byte(enc, 0xFD);
			rds_send_byte(enc, 0x02);
		} else
			rds_send_byte(enc, buf[i]);
	}

	rds_send_byte(enc, UECP_DF_STOP_BYTE);

	return ret;
}


/*****************\
* COMMAND HELPERS *
\*****************/

/**
 * uecp_get_di_dynpty -	Get DI/PTYI field through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 *
 * DI and DYNPTY flags are on the same field on UECP
 * so in order to preserve DI when setting DYNPTYI or
 * the opposite, we need to first get the full field,
 * using this function.
 *
 * XXX: WiP... For now it always returns 0
 */
static int
uecp_get_di_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	return 0;
}


/**********\
* COMMANDS *
\**********/


/**
 * uecp_set_pi - Set Programme Identifier information through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @pi: pointer to &struct rds_pi containing the infos
 */
static int
uecp_set_pi(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_pi *pi)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;
	uint16_t pi_val = 0;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	pi_val |= pi->prn & 0xFF;
	pi_val |= (pi->coverage & 0xF) << 8;
	pi_val |= (pi->ccode & 0x0F00) << 4;

	msg->mec = UECP_MEC_PI;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;
	msg->mel_data[0] = (pi_val & 0xFF00) >> 8;
	msg->mel_data[1] = pi_val & 0x00FF;

	/* Don't include mel_len */
	data_frame.msg_len = 3 + 2;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_ps - Set Programme Service name through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: message group (1-2)
 * @psn: message id (0-14) (only one supported so always 0)
 * @ps: PS name to set
 */
static int
uecp_set_ps(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ps)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;
	int i = 0;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_PS;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;

	/* P.S. is 8 chars long -fixed- */
	for(i = 0; i < 8; i++) {
		/* Reached NULL */
		if(ps[i] == '\0')
			break;

		/* Non-ASCII character */
		else if (ps[i] < 0x20 || ps[i] > 0x7E)
			msg->mel_data[i] = ' ';
		else
			msg->mel_data[i] = ps[i];
	}


	/* If given P.S. is less than 8 chars, pad it
	 * with spaces */
	while(i < 8) {
		msg->mel_data[i++] = ' ';
	}

	/* Don't include mel_len */
	data_frame.msg_len = 3 + 8;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_rt - Set RadioText message through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @rt: the &struct rds_rt to send
 */
static int
uecp_set_rt(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_rt *rt)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;
	int i = 0;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_RT;
	msg->dsn = dsn;
	msg->psn = psn;

	/* RadioText can be 64bytes long */
	msg->mel_data[0] |= (rt->ab_flag & RDS_RT_METHOD_B) ? 1 : 0;
	msg->mel_data[0] |= (rt->retransmissions & 0xF) << 1;
	msg->mel_data[0] |= (rt->buffer_config & 0x3) << 5;

	/* The RT message can be empty, in this case
	 * mel_len will be 1 and bufer_config should be
	 * RDS_RT_BUFF_CONFIG_FLUSH */
	for(i = 1; i < 65; i++) {
		/* Reached NULL */
		if(rt->msg[i] == '\0')
			break;

		/* Non-ASCII character */
		else if (rt->msg[i] < 0x20 || rt->msg[i] > 0x7E)
			msg->mel_data[i] = ' ';
		else
			msg->mel_data[i] = rt->msg[i];
	}

	msg->mel_len = i;

	data_frame.msg_len = 4 + i;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_di - Set decoder info field through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @di: RDS_DI_* flags
 */
static int
uecp_set_di(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t di)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;
	uint8_t di_dynpty = 0;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	/* Get current DI/DYNPTY flag so that we only change
	 * the DI part */
	di_dynpty = uecp_get_di_dynpty(enc, dsn, psn);

	msg->mec = UECP_MEC_DI_PTYI;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;

	/* Note: UECP_DI_DYNPTY_DI_* flags match RDS_DI_* flags */
	msg->mel_data[0] = (di_dynpty & UECP_DI_DYNPTY_DYNAMIC_PTY) |
					(di & UECP_DI_DYNPTY_DI_MASK);

	/* Don't include mel_len */
	data_frame.msg_len = 3 + 1;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_dynpty - Set dynamic PTY indicator through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @dynpty: 1 -> Enabled, 0 -> Disabled
 */
static int
uecp_set_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t dynpty)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;
	uint8_t di_dynpty = 0;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	/* Get current DI/DYNPTY flag so that we only change
	 * the DYNPTY part */
	di_dynpty = uecp_get_di_dynpty(enc, dsn, psn);

	msg->mec = UECP_MEC_DI_PTYI;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;
	msg->mel_data[0] = (di_dynpty & UECP_DI_DYNPTY_DI_MASK) |
				((dynpty != 0) ? UECP_DI_DYNPTY_DYNAMIC_PTY : 0);

	/* Don't include mel_len */
	data_frame.msg_len = 3 + 1;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_ta_tp - Set TA/TP status through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @ta_tp: RDS_TATP_* status flags
 */
static int
uecp_set_ta_tp(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ta_tp)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_TA_TP;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;
	/* Note: UECP_TATP_* flags match RDS_TATP_* flags */
	msg->mel_data[0] = ta_tp & 0x3;
	
	/* Don't include mel_len */
	data_frame.msg_len = 3 + 1;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_ms - Set M/S switch status through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @ms: RDS_MS_* status flags
 */
static int
uecp_set_ms(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ms)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_MS;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;
	msg->mel_data[0] = (ms == RDS_MS_MUSIC) ? 1 : 0;
	
	/* Don't include mel_len */
	data_frame.msg_len = 3 + 1;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_pty - Set Programme Type through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @pty: The pty (0 - 19) to set
 */
static int
uecp_set_pty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t pty)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_PTY;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;
	msg->mel_data[0] = pty & 0x1F;
	
	/* Don't include mel_len */
	data_frame.msg_len = 3 + 1;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_ptyn - Set Programme Type Name through UECP
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @ptyn: PTY name string
 */
static int
uecp_set_ptyn(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ptyn)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;
	int i = 0;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_PTYN;
	msg->dsn = dsn;
	msg->psn = psn;
	msg->mel_len = UECP_MSG_MEL_NA;

	/* PTY Name is 8 chars long -fixed- */
	for(i = 0; i < 8; i++) {
		/* Reached NULL */
		if(ptyn[i] == '\0')
			break;

		/* Non-ASCII character */
		else if (ptyn[i] < 0x20 || ptyn[i] > 0x7E)
			msg->mel_data[i] = ' ';
		else
			msg->mel_data[i] = ptyn[i];
	}


	/* If given PTY Name is less than 8 chars, pad it
	 * with spaces */
	while(i < 8) {
		msg->mel_data[i++] = ' ';
	}

	/* Don't include mel_len */
	data_frame.msg_len = 3 + 8;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_ct - Enable/disable transmission of RTC through UECP
 * @enc: pointer to &struct rds_encoder
 * @ct: 0 -> disable, 1 -> enable
 */
static int
uecp_set_ct(struct rds_encoder *enc, uint8_t ct)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_CT;
	msg->mel_len = UECP_MSG_MEL_NA;
	msg->mel_data[0] = (ct & 0x01) ? 1 : 0;
	
	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_rtc - Set Real Time Clock settings on the device through UECP
 * @enc: pointer to &struct rds_encoder
 * @rtc: the &struct rds_rtc to set
 */
static int
uecp_set_rtc(struct rds_encoder *enc, struct rds_rtc *rtc)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;
	int sign = 0;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_RTC;
	msg->mel_len = UECP_MSG_MEL_NA;

	/* Sanity check */
	if(rtc->month > 12 || rtc->hours > 24 ||
	rtc->minutes > 59 || rtc->seconds > 59 ||
	rtc->centiseconds > 99 || rtc->offset > 14 ||
	rtc->offset < -12)
		return -EINVAL;

	msg->mel_data[0] = rtc->year % 100;	/* Last 2 digits of Year in hex */
	msg->mel_data[1] = rtc->month;
	msg->mel_data[2] = rtc->hours;
	msg->mel_data[3] = rtc->minutes;
	msg->mel_data[4] = rtc->seconds;
	msg->mel_data[5] = rtc->centiseconds;

	sign = (rtc->offset & 0x80) >> 2; /* Sign is on the 6th bit */
	rtc->offset = rtc->offset * 2;	/* Offset is in half-hour increments
					 * so max value is 28 */

	msg->mel_data[6] = (rtc->offset | sign) & 0x3F;

	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**
 * uecp_set_rds_on - Set encoder's RDS output status through UECP
 * @enc: pointer to &struct rds_encoder
 * @on: 1 -> Enable, 0 -> Disable
 */
static int
uecp_set_rds_on(struct rds_encoder *enc, uint8_t on)
{
	struct uecp_data_frame data_frame;
	struct uecp_message *msg = &data_frame.msg;

	memset(&data_frame, 0, sizeof(struct uecp_data_frame));

	msg->mec = UECP_MEC_RDSON;
	msg->mel_len = UECP_MSG_MEL_NA;
	msg->mel_data[0] = (on & 0x01) ? 1 : 0;
	
	uecp_send_frame_to_enc(enc, &data_frame);

	return 0;
}


/**************\
* ENTRY POINTS *
\**************/

int
uecp_init(struct rds_encoder *enc)
{
	enc->get_pi = NULL;
	enc->set_pi = &uecp_set_pi;
	enc->get_ps = NULL;
	enc->set_ps = &uecp_set_ps;
	enc->get_di = NULL;
	enc->set_di = &uecp_set_di;
	enc->get_dynpty = NULL;
	enc->set_dynpty = &uecp_set_dynpty;
	enc->get_rt = NULL;
	enc->set_rt = &uecp_set_rt;
	enc->get_ta_tp = NULL;
	enc->set_ta_tp = &uecp_set_ta_tp;
	enc->get_ms = NULL;
	enc->set_ms = &uecp_set_ms;
	enc->get_pty = NULL;
	enc->set_pty = &uecp_set_pty;
	enc->get_ptyn = NULL;
	enc->set_ptyn = &uecp_set_ptyn;
	enc->get_ct = NULL;
	enc->set_ct = &uecp_set_ct;
	enc->get_rtc = NULL;
	enc->set_rtc = &uecp_set_rtc;
	enc->get_rds_on = NULL;
	enc->set_rds_on = &uecp_set_rds_on;
	return 0;
}
