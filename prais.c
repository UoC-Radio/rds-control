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
 * prais.c - 	Support for Prais Coder mod. 735
 *		(and possibly 732 but haven't tested it)
 */

#include <stdint.h>	/* For sized integers */
#include <errno.h>	/* For error numbers */
#include <string.h>	/* For memset() */
#include <stdio.h>	/* For printf(...) */
#include <arpa/inet.h> 	/* For htons */
#include <termios.h>	/* For tcflush() */
#include "rds.h"
#include "prais.h"


/**********************\
* DATA FORMAT HANDLING *
\**********************/

/**
 * prais_get_frame_from_enc - Get a pending data frame from a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @data: pointer to a pre-allocated &struct prais_data_frame to fill
 */
static int
prais_get_frame_from_enc(struct rds_encoder *enc,
			struct prais_data_frame *data)
{
	unsigned char buf[PRAIS_DF_MAX_LEN];
	struct prais_message *msg = &data->msg;
	int ret = 0;
	int i = 0;
	int got_ack = 0;
	uint16_t csum_calculated = 0;
	uint16_t csum_received = 0;
	char csum_ascii[2];

	ret = rds_get_byte(enc);
	if(ret < 0)
		return -ENODATA;

	/* Got a SYN, start processing */
	if(ret == PRAIS_DL_SYN) {
		/* Skip any extra SYNs */
		while(ret == PRAIS_DL_SYN) {
			ret = rds_get_byte(enc);
		}
	} else {
		ret = -EPROTO;
		goto finished;
	}

	/* Process ACK */
	if(ret != PRAIS_DL_ACK) {
		ret = -EPROTO;
		goto finished;
	}
	
	ret = rds_get_byte(enc);
	if(ret != PRAIS_DL_SYN) {
		ret = -EPROTO;
		goto finished;
	}

	got_ack = 1;

	/*
	 * ACK only -> 0
	 * Frame following -> SYN
	 */
	ret = rds_get_byte(enc);
	if(ret == 0) {
		return 0;
	} else if(ret != PRAIS_DL_SYN) {
		ret = -EPROTO;
		goto finished;
	}

	/* Process header */

	rds_get_byte(enc); /* SYN */

	ret = rds_get_byte(enc);
	if(ret != PRAIS_DL_SOH) {
		ret = -EPROTO;
		goto finished;
	}

	/* Address field */
	ret = rds_get_byte(enc);
	if(ret < 0) {
		ret = -EPROTO;
		goto finished;
	}

	if(ret & (PRAIS_DF_NO_REPLY >> 8))
		data->no_reply = 1;
	enc->addr |= ret << 8;

	ret = rds_get_byte(enc);
	if(ret < 0) {
		ret = -EPROTO;
		goto finished;
	}

	enc->addr |= ret;

	/* Sequence number */
	ret = rds_get_byte(enc);
	if((ret < 0x30) || (ret > 0x39)) {
		ret = -EPROTO;
		goto finished;
	}

	//enc->seq = ret - 0x30;

	/*
	 * Header OK go for the message
	 * (and be a little more relaxed
	 * on the checks since we got here)
	 */
	rds_get_byte(enc); /* DLE */
	rds_get_byte(enc); /* STX */

	/* Message type */
	ret = rds_get_byte(enc);
	if((ret < PRAIS_MT_MIN_VAL) || (ret > PRAIS_MT_MAX_VAL)) {
		ret = -EPROTO;
		goto finished;
	}
	msg->type = ret;
	csum_calculated += ret;

	/* Message length */
	ret = rds_get_byte(enc);
	if((ret < 0) || (ret > PRAIS_MT_MAX_LEN)) {
		ret = -EPROTO;
		goto finished;
	}
	msg->len = ret;
	csum_calculated += ret;

	/* Message data */
	for(i = 0; i < msg->len; i++) {
		ret = rds_get_byte(enc);

		/* Escaped character */
		if(ret == PRAIS_DL_DLE) {
			ret = rds_get_byte(enc);
			csum_calculated += ret;	
		}

		if(ret < 0) {
			ret = -EPROTO;
			goto finished;
		}

		msg->data[i] = ret;

		csum_calculated += ret;
	}

	rds_get_byte(enc); /* DLE */
	rds_get_byte(enc); /* ETX */

	/* Checksum is one byte sent as 2 ASCII
	 * chars, so if checksum is e.g. 0x123
	 * 0x23 is sent as '2''3' or 0x32 0x33 */
	csum_ascii[0] = rds_get_byte(enc);
	csum_ascii[1] = rds_get_byte(enc);

	for(i = 0; i < 2; i++) {	
		if(csum_ascii[i] >= 0x41)
			csum_ascii[i] = csum_ascii[i] - 0x41 + 0xA;
		else if(csum_ascii[i] >= 0x30)
			csum_ascii[i] -= 0x30;
	}

	csum_received |= (csum_ascii[0] & 0xF) << 4;
	csum_received |= (csum_ascii[1] & 0xF);

	rds_get_byte(enc); /* SYN */

	/* If checksum doesn't fit in a byte
	 * throw away the higher bits */
	csum_calculated &= 0xFF;

	if(csum_received != csum_calculated)
		ret = -EPROTO;
	else
		ret = msg->len;

finished:
	tcflush(enc->serial_fd, TCIFLUSH);
	return ret;
}

/**
 * prais_send_frame_to_enc - Send a data frame to a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @data: pointer to the &struct prais_data_frame to send
 */
static int
prais_send_frame_to_enc(struct rds_encoder *enc,
			struct prais_data_frame *data)
{
	unsigned char buf[PRAIS_DF_MAX_LEN];
	int len = 0;
	uint8_t csum_char = 0;
	data->msg.checksum = 0;

	/* Add the no reply flag if needed */
	if(data->no_reply)
		enc->addr |= PRAIS_DF_NO_REPLY;

	/* Start header with two SYNs and SOH */
	rds_send_byte(enc, PRAIS_DL_SYN);
	rds_send_byte(enc, PRAIS_DL_SYN);
	rds_send_byte(enc, PRAIS_DL_SOH);

	/* Send address and sequence number as ASCII text */
	rds_send_byte(enc, (enc->addr & 0xFF00) >> 8);
	rds_send_byte(enc, enc->addr & 0x00FF);
	rds_send_byte(enc, enc->seq + 0x30);

	/* Send DLE and STX to mark msg start */
	rds_send_byte(enc, PRAIS_DL_DLE);
	rds_send_byte(enc, PRAIS_DL_STX);

	/* Send mesage type, length and body, in case
	 * a byte is the same as DLE, escape it by
	 * adding a DLE before it (so output 2 DLEs) */

	rds_send_byte(enc, data->msg.type);
	data->msg.checksum += data->msg.type;

	rds_send_byte(enc, data->msg.len);
	data->msg.checksum += data->msg.len;

	for(len = 0; len < data->msg.len && len < PRAIS_MT_MAX_LEN; len++) {
		if(data->msg.data[len] == 0x10) {
			rds_send_byte(enc, PRAIS_DL_DLE);
			data->msg.checksum += PRAIS_DL_DLE;
		}

		rds_send_byte(enc, data->msg.data[len]);
		data->msg.checksum += data->msg.data[len];
	}

	/* Send DLE and ETX to mark msg end */
	rds_send_byte(enc, PRAIS_DL_DLE);
	rds_send_byte(enc, PRAIS_DL_ETX);

	/* Calculate checksum: grab the LSB of checksum
	 * and send it out as ASCII text */
	csum_char = (data->msg.checksum & 0xF0) >> 4;
	if(csum_char >= 10)
		csum_char = (csum_char % 10) + 0x41;
	else
		csum_char = csum_char + 0x30;

	rds_send_byte(enc, csum_char);

	csum_char = data->msg.checksum & 0x0F;
	if(csum_char >= 10)
		csum_char = (csum_char % 10) + 0x41;
	else
		csum_char = csum_char + 0x30;

	rds_send_byte(enc, csum_char);

	/* Finaly send a SYNC to end the message */
	rds_send_byte(enc, PRAIS_DL_SYN);

	/* And increase the sequence counter */
	if(enc->seq < 9)
		enc->seq++;
	else
		enc->seq = 0; /* Reset */

	/* Prais's programm sends 1220 (4c4) ETXes
	 * when broadcasting or on satelite mode.
	 * It looks like they wanted to give some
	 * time to the device to process it since
	 * there is no feedback or something like
	 * that. It's crap, I'll leave it to 64
	 * and if you see any issues, increase it.
	 */
	if(data->no_reply)
		for(len = 0; len < 64; len++)
			rds_send_byte(enc, PRAIS_DL_ETX);
}

/**
 * prais_send_ack_to_enc - Send an ACK frame to a Prais encoder
 * @enc: pointer to &struct rds_encoder
 */
static int
prais_send_ack_to_enc(struct rds_encoder *enc)
{
	rds_send_byte(enc, PRAIS_DL_SYN);
	rds_send_byte(enc, PRAIS_DL_SYN);
	rds_send_byte(enc, PRAIS_DL_ACK);
	rds_send_byte(enc, PRAIS_DL_SYN);
	rds_send_byte(enc, 0);
	return 0;
}


/*****************\
* COMMAND HELPERS *
\*****************/

/**
 * prais_get_tamsdi - Request TAMSDI field on a Prais encoder
 *
 * Since TAMSDI field holds more than one fields
 * we want to get/set indepentently, this function
 * is used internaly, mostly to preserve TAMSDI
 * settings when chaning only TA, MS, DI or dynamic PTY
 *
 * @enc: pointer to &struct rds_encoder
 *
 * Returns: tamsdi on success or -errno
 */
static int
prais_get_tamsdi(struct rds_encoder *enc)
{
	struct prais_data_frame request;
	struct prais_message *msg = &request.msg;
	struct prais_data_frame reply;
	uint8_t tamsdi = 0;
	int ret = 0;
	memset(&request, 0, sizeof(struct prais_data_frame));

	request.no_reply = 0;

	msg->type = PRAIS_MT_TAMSDI;
	msg->len = 0;

	ret = prais_send_frame_to_enc(enc, &request);
	if(ret < 0)
		return ret;

	ret = prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	tamsdi = reply.msg.data[0];

	return tamsdi & 0x7F;	/* Ignore the msb */
}

/**
 * prais_get_rt_status - Get RT transmission status of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * Returns: 0 -> Ready to send, 1 -> Transmission pending
 */
static int
prais_get_rt_status(struct rds_encoder *enc)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		return -EOPNOTSUPP;

	msg->type = PRAIS_MT_RT;
	msg->len = 0;

	prais_send_frame_to_enc(enc, &data_frame);

	ret = prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	ret = reply.msg.data[0];

	return ret;
}

/**
 * prais_set_rt_mode - Set RT transmission state on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @mode: 0 -> Reset (?), 1 -> Send with method A, 2 -> Send with method B
 */
static int
prais_set_rt_mode(struct rds_encoder *enc, uint8_t mode)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		return -EOPNOTSUPP;

	if(mode == 0) {
		msg->type = PRAIS_MT_RT;
		msg->len = 1;
		msg->data[0] = 0;
	
		prais_send_frame_to_enc(enc, &data_frame);

		ret = prais_get_frame_from_enc(enc, &reply);
		if(ret < 0)
			return ret;

		prais_send_ack_to_enc(enc);
	} else if(mode == 1) {
		msg->type = PRAIS_MT_RT;
		msg->len = 2;
		msg->data[0] = 0;
		msg->data[1] = 0;
	
		prais_send_frame_to_enc(enc, &data_frame);

		ret = prais_get_frame_from_enc(enc, &reply);
		if(ret < 0)
			return ret;

		prais_send_ack_to_enc(enc);

		msg->type = PRAIS_MT_RT;
		msg->len = 1;
		msg->data[0] = 1;
	
		prais_send_frame_to_enc(enc, &data_frame);

		ret = prais_get_frame_from_enc(enc, &reply);
		if(ret < 0)
			return ret;

		prais_send_ack_to_enc(enc);
	} else if(mode == 2) {
		msg->type = PRAIS_MT_RT;
		msg->len = 2;
		msg->data[0] = 0;
		msg->data[1] = 0x40;
	
		prais_send_frame_to_enc(enc, &data_frame);

		ret = prais_get_frame_from_enc(enc, &reply);
		if(ret < 0)
			return ret;

		prais_send_ack_to_enc(enc);

		msg->type = PRAIS_MT_RT;
		msg->len = 1;
		msg->data[0] = 2;
	
		prais_send_frame_to_enc(enc, &data_frame);

		ret = prais_get_frame_from_enc(enc, &reply);
		if(ret < 0)
			return ret;

		prais_send_ack_to_enc(enc);
	}

	ret = reply.msg.data[0];

	return ret;
}

/**
 * prais_store - Store data on a Prais encoder (WiP)
 * @enc: pointer to &struct rds_encoder
 *
 * The vendor's utility applies most config options
 * (together with dynamic PS group messages etc) in one
 * step, instead of giving the option to tweak them one
 * by one. At the end of this step this command is issued
 * to store data on the device. It's not used here (yet)
 */
static int
prais_store(struct rds_encoder *enc)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		return -EOPNOTSUPP;

	msg->type = PRAIS_MT_STORE;
	msg->len = 0;

	prais_send_frame_to_enc(enc, &data_frame);

	ret = prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	ret = reply.msg.data[0];

	return ret;
}

/**
 * prais_reset - Reset a Prais encoder (?)
 * @enc: pointer to &struct rds_encoder
 *
 * XXX: Got this through binary analysis of the
 * vendor's application. When sent, the unit replies
 * but nothing happens (nothing is reset). Need to
 * investigate this further.
 */
static int
prais_reset(struct rds_encoder *enc)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		return -EOPNOTSUPP;

	msg->type = PRAIS_MT_RESET;
	msg->len = 0;

	prais_send_frame_to_enc(enc, &data_frame);

	ret = prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	ret = reply.msg.data[0];

	return ret;
}


/**********\
* COMMANDS *
\**********/


/**
 * prais_get_pi - Get Programme Identifier information of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @pi: pointer to &struct rds_pi to fill
 */
static int
prais_get_pi(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_pi *pi)
{
	struct prais_data_frame request;
	struct prais_message *msg = &request.msg;
	struct prais_data_frame reply;
	int ret = 0;

	/* Only one active programme is supported */
	if(dsn != 0 || psn != 0 || (enc->addr == PRAIS_DF_ADDR_BCAST))
		return -EOPNOTSUPP;

	memset(&request, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));
	memset(pi, 0, sizeof(struct rds_pi));

	msg->type = PRAIS_MT_PI;
	msg->len = 0;

	ret = prais_send_frame_to_enc(enc, &request);
	if(ret < 0)
		return ret;

	prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	pi->ccode = reply.msg.data[2] & 0xFF; /* ECC */
	pi->ccode |= ((reply.msg.data[0] & 0xF0) >> 4) << 8;
	pi->coverage = reply.msg.data[0] & 0x0F;
	pi->prn = reply.msg.data[1];

	return 0;
}

/**
 * prais_set_pi - Set Programme Identifier information on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @pi: pointer to &struct rds_pi containing the infos
 */
static int
prais_set_pi(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_pi *pi)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	uint8_t pi_country = 0;
	uint8_t pi_ecc = 0;
	int ret = 0;

	/* Only one active programme is supported */
	if(dsn != 0 || psn != 0)
		return -EOPNOTSUPP;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		data_frame.no_reply = 1;

	pi_country = (pi->ccode & 0x0F00) >> 8;
	pi_ecc = (pi->ccode & 0x00FF);
	msg->type = PRAIS_MT_PI;
	msg->len = 3;
	msg->data[0] = (pi_country << 4) | pi->coverage & 0x0F;
	msg->data[1] = pi->prn;
	msg->data[2] = pi_ecc;

	ret = prais_send_frame_to_enc(enc, &data_frame);
	if(ret < 0)
		return ret;

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**
 * prais_get_ps - Get Programme Service name of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: message group (1-2)
 * @psn: message id (0-14) (only one supported so always 0)
 * @ps: a pre-allocated 8byte char array to fill
 */
static int
prais_get_ps(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ps)
{
	struct prais_data_frame request;
	struct prais_message *msg = &request.msg;
	struct prais_data_frame reply;
	int i = 0;
	int fields = 0;
	int ret = 0;

	memset(&request, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));

	/* Only one programme is supported by the device
	 * and only one message per programme is supported by
	 * this library */
	if(psn != 0 || dsn > 2 || (enc->addr == PRAIS_DF_ADDR_BCAST))
		return -EOPNOTSUPP;

	msg->type = PRAIS_MT_PS;
	msg->len = 2;
	msg->data[0] = 1;
	msg->data[1] = psn | ((dsn == 2) ? PRAIS_PSN_INDEX_GROUP2 : 0);

	ret = prais_send_frame_to_enc(enc, &request);
	if(ret < 0)
		return ret;

	ret = prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	msg = &reply.msg;

	/* On empty message only the index field
	 * is present */
	fields = (msg->len < 3) ? 1 : 2;

	for(i = 0; i < msg->len - fields; i++)
		ps[i] = msg->data[i + fields];

	return i;
}

/**
 * prais_set_ps - Set Programme Service name on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: message group (1-2)
 * @psn: message id (0-14) (only one supported so always 0)
 * @ps: PS name to set
 */
static int
prais_set_ps(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ps)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int i = 0;
	int len = 0;
	int pslen = 0;
	int ret = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));


	if(((psn != 0) && !(enc->flags & RDS_ENCODER_FLAGS_PRAIS_HW_DYNPS)) ||
	((psn > 14) && (enc->flags & RDS_ENCODER_FLAGS_PRAIS_HW_DYNPS))
	|| (dsn > 2))
		return -EOPNOTSUPP;

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		data_frame.no_reply = 1;

	pslen = strnlen(ps, 9); /* 8 + null */

	msg->type = PRAIS_MT_PS;
	msg->len = 11;	/* Even on empty messages it's filled with spaces */

	/* 0 -> set, 1 -> request, 2 -> disable,
	 * not present on reply */
	msg->data[len++] = (pslen < 1) ? 2 : 0;

	/* Index 0 - 15, or with the flag above to
	 * refer to the 2nd group */
	msg->data[len++] = psn | ((dsn == 2) ? PRAIS_PSN_INDEX_GROUP2 : 0);


	/* Duration 0 - 100
	 * It's 0xFF when message disable is requested */
	msg->data[len++] = (pslen < 1) ? 0xFF :
			((enc->flags & RDS_ENCODER_FLAGS_PRAIS_HW_DYNPS) && (psn == 0)) ? 10 :
			(enc->flags & RDS_ENCODER_FLAGS_PRAIS_HW_DYNPS) ? 4 : 00;

	/* P.S. is 8 chars long */
	for(i = 0; i < pslen; i++) {
		/* Non-ASCII character */
		if (ps[i] < 0x20 || ps[i] > 0x7E)
			msg->data[len++] = ' ';
		else
			msg->data[len++] = ps[i];
	}


	/* If given P.S. is less than 8 chars, pad it
	 * with spaces -also for empty/disabled- */
	while(i < 8) {
		msg->data[len++] = ' ';
		i++;
	}

	ret = prais_send_frame_to_enc(enc, &data_frame);
	if(ret < 0)
		return ret;

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}

/**
 * prais_manipulator_rt - Replicates message as many times at it fits to the rt buffer
 * @msg: The message to be manipulated
 */
static void
prais_manipulator_rt(char* msg)
{
	size_t len;
	int times;
	int segment_size;
	int i;
	
	len = strlen(msg);
	segment_size = len + PRAIS_RT_SPACE_LENGTH;
	times = (RDS_RT_MSG_LEN_MAX - 1) / segment_size;
	
	/* optional */
	if(times < 2) return;

	memset(msg + len, ' ', (RDS_RT_MSG_LEN_MAX - 1) - len);
	
	for(i = 1; i < times; ++i)
	{
		memcpy(msg + i * segment_size, msg, segment_size);
	}
}

/**
 * prais_set_rt - Set RadioText message on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @rt: the &struct rds_rt to send
 *
 * XXX: WiP...
 */
static int
prais_set_rt(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_rt *rt)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;
	int i = 0;
	int c = 0;
	int j = 0;
	int k = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		return -EOPNOTSUPP;

	if(rt->buffer_config == RDS_RT_BUFF_CONFIG_APPEND)
		return -EOPNOTSUPP;
	else if(rt->buffer_config == RDS_RT_BUFF_CONFIG_FLUSH && strlen(rt->msg) == 0)
		return prais_set_rt_mode(enc, 0);
		
	enc->seq = 0;

	/* If active, disable it */
	ret = prais_get_rt_status(enc);
	if(ret < 0)
		return ret;

	/* Pending RT transmission <- WTF ? */
	if(ret == 1)
		return -EAGAIN;

	/* Set to inactive */
	ret = prais_set_rt_mode(enc, 0);
	if(ret < 0)
		return ret;
	
	/* Manipulate message */	
	prais_manipulator_rt(rt->msg);	

	for(k = 0; k < 4; k++) {
		for(i = 0; i < 16; i++) {
			msg->type = PRAIS_MT_RT;
			msg->len = 4;
			c = 4 * i;

			for(j = 0; j < 4; j++, c++) {
				/* Non-ASCII character */
				if (rt->msg[c] < 0x20 || rt->msg[c] > 0x7E) {
					msg->data[j] = ' ';
				} else
					msg->data[j] = rt->msg[c];
			}

			prais_send_frame_to_enc(enc, &data_frame);

			ret = prais_get_frame_from_enc(enc, &reply);
			if(ret < 0)
				return ret;

			prais_send_ack_to_enc(enc);

			ret = reply.msg.data[0];
		}
	}

	/* Enable RadioText */
	ret = prais_set_rt_mode(enc, 1);
	if(ret < 0)
		return ret;

	return ret;
}


/**
 * prais_get_di - Get decoder info field of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 */
static int
prais_get_di(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	int tamsdi = 0;
	int di = 0;

	if((enc->addr == PRAIS_DF_ADDR_BCAST)||
	(dsn != 0 || psn != 0))
		return -EOPNOTSUPP;

	tamsdi = prais_get_tamsdi(enc);
	if(tamsdi < 0)
		return tamsdi;

	tamsdi &= PRAIS_TAMSDI_DI_MASK;

	if(tamsdi & PRAIS_TAMSDI_DI_STEREO)
		di = RDS_DI_STEREO;
	if(tamsdi & PRAIS_TAMSDI_DI_ART_HEAD)
		di |= RDS_DI_ARTIFICIAL_HEAD;
	if(tamsdi & PRAIS_TAMSDI_DI_COMPRESSED)
		di |= RDS_DI_COMPRESSED;

	return di & 0x07;
}

/**
 * prais_set_di - Set decoder info field on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @di: RDS_DI_* status flags
 */
static int
prais_set_di(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t di)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int tamsdi = 0;
	int ret = 0;

	/* Only one active programme is supported */
	if(dsn != 0 || psn != 0)
		return -EOPNOTSUPP;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));

	if(enc->addr != PRAIS_DF_ADDR_BCAST) {
		/* Get the current TAMSDI flag so that we only mess with
		 * the TA flag and keep the rest as is	 */
		tamsdi = prais_get_tamsdi(enc);
		if(tamsdi < 0)
			return tamsdi;

		/* Reset DI field */
		tamsdi &= ~PRAIS_TAMSDI_DI_MASK;
		if(di & RDS_DI_STEREO)
			tamsdi |= PRAIS_TAMSDI_DI_STEREO;
		if(di & RDS_DI_ARTIFICIAL_HEAD)
			tamsdi |= PRAIS_TAMSDI_DI_ART_HEAD;
		if(di & RDS_DI_COMPRESSED)
			tamsdi |= PRAIS_TAMSDI_DI_COMPRESSED;
	} else
		data_frame.no_reply = 1;

	msg->type = PRAIS_MT_TAMSDI;
	msg->len = 1;
	msg->data[0] = tamsdi;

	ret = prais_send_frame_to_enc(enc, &data_frame);
	if(ret < 0)
		return ret;

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**
 * prais_get_dynpty - Get dynamic PTY indicator of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 */
static int
prais_get_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	int tamsdi = 0;

	if((enc->addr == PRAIS_DF_ADDR_BCAST)||
	(dsn != 0 || psn != 0))
		return -EOPNOTSUPP;

	tamsdi = prais_get_tamsdi(enc);
	if(tamsdi < 0)
		return tamsdi;

	return (tamsdi & PRAIS_TAMSDI_DYNPTY) ? 1 : 0;
}

/**
 * prais_set_dynpty - Set dynamic PTY indicator on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @dynpty: 1 -> Enabled, 0 -> Disabled
 */
static int
prais_set_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t dynpty)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int tamsdi = 0;
	int ret = 0;

	/* Only one active programme is supported */
	if(dsn != 0 || psn != 0)
		return -EOPNOTSUPP;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));

	if(enc->addr != PRAIS_DF_ADDR_BCAST) {
		/* Get the current TAMSDI flag so that we only mess with
		 * the DYNPTY flag and keep the rest as is */
		tamsdi = prais_get_tamsdi(enc);
		if(tamsdi < 0)
			return tamsdi;

		/* Reset DYNPTY flag */
		tamsdi &= ~PRAIS_TAMSDI_DYNPTY;
	} else
		data_frame.no_reply = 1;

	msg->type = PRAIS_MT_TAMSDI;
	msg->len = 1;
	msg->data[0] = tamsdi | ((dynpty != 0) ? PRAIS_TAMSDI_DYNPTY : 0);

	ret = prais_send_frame_to_enc(enc, &data_frame);
	if(ret < 0)
		return ret;

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**
 * prais_get_ta_tp - Get TA/TP status of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 */
static int
prais_get_ta_tp(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	int tamsdi = 0;
	int tatp = 0;

	if((enc->addr == PRAIS_DF_ADDR_BCAST)||
	(dsn != 0 || psn != 0))
		return -EOPNOTSUPP;

	tamsdi = prais_get_tamsdi(enc);
	if(tamsdi < 0)
		return tamsdi;

	tamsdi &= PRAIS_TAMSDI_TATP_MASK;

	if(tamsdi & PRAIS_TAMSDI_TA_ON)
		tatp |= RDS_TATP_TA_ON;
	if(tamsdi & PRAIS_TAMSDI_TP_ON)
		tatp |= RDS_TATP_TP_ON;

	return tatp;
}

/**
 * prais_set_ta_tp - Set TA/TP status on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @ta_tp: RDS_TATP_* status flags
 */
static int
prais_set_ta_tp(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ta_tp)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	uint8_t tamsdi = 0;
	int ret = 0;

	/* Only one active programme is supported */
	if(dsn != 0 || psn != 0)
		return -EOPNOTSUPP;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));

	if(enc->addr != PRAIS_DF_ADDR_BCAST) {
		/* Get the current TAMSDI flag so that we only mess with
		 * the TA flag and keep the rest as is */
		tamsdi = prais_get_tamsdi(enc);

		tamsdi &= ~PRAIS_TAMSDI_TATP_MASK;
	} else
		data_frame.no_reply = 1;

	if(ta_tp & RDS_TATP_TA_ON)
		tamsdi |= PRAIS_TAMSDI_TA_ON;
	if(ta_tp & RDS_TATP_TP_ON)
		tamsdi |= PRAIS_TAMSDI_TP_ON;

	msg->type = PRAIS_MT_TAMSDI;
	msg->len = 1;
	msg->data[0] = tamsdi;

	prais_send_frame_to_enc(enc, &data_frame);

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**
 * prais_get_ms - Get M/S switch status of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 */
static int
prais_get_ms(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	uint8_t tamsdi = 0;

	if((enc->addr == PRAIS_DF_ADDR_BCAST)||
	(dsn != 0 || psn != 0))
		return -EOPNOTSUPP;

	tamsdi = prais_get_tamsdi(enc);
	if(tamsdi < 0)
		return tamsdi;

	return (tamsdi & PRAIS_TAMSDI_MS_MUSIC) ? RDS_MS_MUSIC : RDS_MS_SPEECH;
}

/**
 * prais_set_ms - Set M/S switch status on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @ms: RDS_MS_* status flags
 */
static int
prais_set_ms(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ms)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	uint8_t tamsdi = 0;
	int ret = 0;

	/* Only one active programme is supported */
	if(dsn != 0 || psn != 0)
		return -EOPNOTSUPP;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));

	if(enc->addr != PRAIS_DF_ADDR_BCAST) {
		/* Get the current TAMSDI flag so that we only mess with
		 * the M/S flag and keep the rest as is */
		tamsdi = prais_get_tamsdi(enc);
		if(tamsdi < 0)
			return tamsdi;

		tamsdi &= ~PRAIS_TAMSDI_MS_MUSIC;

	} else
		data_frame.no_reply = 1;

	msg->type = PRAIS_MT_TAMSDI;
	msg->len = 1;
	msg->data[0] = tamsdi | ((ms == RDS_MS_MUSIC) ?
				PRAIS_TAMSDI_MS_MUSIC : 0);

	prais_send_frame_to_enc(enc, &data_frame);

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**
 * prais_get_pty - Get Programme Type of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 */
static int
prais_get_pty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	struct prais_data_frame request;
	struct prais_message *msg = &request.msg;
	struct prais_data_frame reply;
	int ret = 0;

	if((enc->addr == PRAIS_DF_ADDR_BCAST)||
	(dsn != 0 || psn != 0))
		return -EOPNOTSUPP;

	memset(&request, 0, sizeof(struct prais_data_frame));
	memset(&reply, 0, sizeof(struct prais_data_frame));

	msg->type = PRAIS_MT_PTY;
	msg->len = 0;

	ret = prais_send_frame_to_enc(enc, &request);
	if(ret < 0)
		return ret;

	ret = prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	ret = reply.msg.data[0] & 0xFF;

	return ret;
}

/**
 * prais_set_pty - Set Programme Type on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @dsn: 0 (only one Programme supported)
 * @psn: 0 (only one Programme supported)
 * @pty: The pty (0 - 19) to set
 */
static int
prais_set_pty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t pty)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;

	/* Only one active programme is supported */
	if(dsn != 0 || psn != 0)
		return -EOPNOTSUPP;

	/* PTY range is from 0 to 31 */
	if(pty > 31)
		return -EINVAL;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		data_frame.no_reply = 1;

	msg->type = PRAIS_MT_PTY;
	msg->len = 1;
	msg->data[0] = pty;

	prais_send_frame_to_enc(enc, &data_frame);

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**
 * prais_set_rtc - Set Real Time Clock settings on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @rtc: the &struct rds_rtc to set
 */
static int
prais_set_rtc(struct rds_encoder *enc, struct rds_rtc *rtc)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;
	uint8_t sign = 0;
	uint8_t offset = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		data_frame.no_reply = 1;

	msg->type = PRAIS_MT_RTC;
	msg->len = 6;

	/* Sanity check */
	if(rtc->month > 12 || rtc->hours > 24 ||
	rtc->minutes > 59 || rtc->seconds > 59 ||
	rtc->centiseconds > 99 || rtc->offset > 14 ||
	rtc->offset < -12)
		return -EINVAL;

	msg->data[0] = ((rtc->hours / 10) << 4) /* Hour in hex 0x00 - 0x23 */
			| (rtc->hours % 10);
	msg->data[1] = ((rtc->minutes / 10) << 4) /* Mins in hex 0x00 - 0x59 */
			| (rtc->minutes % 10);
	msg->data[2] = ((rtc->day / 10) << 4) /* Day in hex 0x01 - 0x31 */
			| (rtc->day % 10);
	msg->data[3] = ((rtc->month / 10) << 4) /* Month in hex 0x01 - 0x12 */
			| (rtc->month % 10);
	msg->data[4] = rtc->year - 1900; /* Years passed since 1900 */

	sign = (rtc->offset < 0) ? 1 : 0;
	offset = (rtc->offset * 2) & 0x1F; /* 5bits */
	offset |= (sign == 1) ? 0x20 : 0;

	msg->data[5] = offset;	/* UTC Offset in half hour incrments with sign on the
				 * sixth bit -same format as UECP- */

	prais_send_frame_to_enc(enc, &data_frame);

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**
 * prais_get_rds_on - Get the RDS output status of a Prais encoder
 * @enc: pointer to &struct rds_encoder
 */
static int
prais_get_rds_on(struct rds_encoder *enc)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		return -EOPNOTSUPP;

	msg->type = PRAIS_MT_RDSON;
	msg->len = 0;

	prais_send_frame_to_enc(enc, &data_frame);

	ret = prais_get_frame_from_enc(enc, &reply);
	if(ret < 0)
		return ret;

	prais_send_ack_to_enc(enc);

	ret = reply.msg.data[0];

	return ret;
}

/**
 * prais_set_rds_on - Set the RDS output status on a Prais encoder
 * @enc: pointer to &struct rds_encoder
 * @on: 1 -> Enable, 0 -> Disable
 */
static int
prais_set_rds_on(struct rds_encoder *enc, uint8_t on)
{
	struct prais_data_frame data_frame;
	struct prais_message *msg = &data_frame.msg;
	struct prais_data_frame reply;
	int ret = 0;

	memset(&data_frame, 0, sizeof(struct prais_data_frame));

	if(enc->addr == PRAIS_DF_ADDR_BCAST)
		data_frame.no_reply = 1;

	msg->type = PRAIS_MT_RDSON;
	msg->len = 1;
	msg->data[0] = (on != 0) ? 1 : 0;

	prais_send_frame_to_enc(enc, &data_frame);

	if(data_frame.no_reply != 1) {
		ret = prais_get_frame_from_enc(enc, &reply);
		prais_send_ack_to_enc(enc);
	}
	return ret;
}


/**************\
* ENTRY POINTS *
\**************/

int
prais_init(struct rds_encoder *enc)
{
	enc->get_pi = &prais_get_pi;
	enc->set_pi = &prais_set_pi;
	enc->get_ps = &prais_get_ps;
	enc->set_ps = &prais_set_ps;
	enc->get_di = &prais_get_di;
	enc->set_di = &prais_set_di;
	enc->get_dynpty = &prais_get_dynpty;
	enc->set_dynpty = &prais_set_dynpty;
	enc->get_rt = NULL;
	enc->set_rt = &prais_set_rt;
	enc->get_ta_tp = &prais_get_ta_tp;
	enc->set_ta_tp = &prais_set_ta_tp;
	enc->get_ms = &prais_get_ms;
	enc->set_ms = &prais_set_ms;
	enc->get_pty = &prais_get_pty;
	enc->set_pty = &prais_set_pty;
	enc->get_ptyn = NULL;
	enc->set_ptyn = NULL;
	enc->get_ct = NULL;
	enc->set_ct = NULL;
	enc->get_rtc = NULL;
	enc->set_rtc = &prais_set_rtc;
	enc->get_rds_on = &prais_get_rds_on;
	enc->set_rds_on = &prais_set_rds_on;

	return 0;
}
