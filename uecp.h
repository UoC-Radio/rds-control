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
 * uecp.h -	Support for UECP compatible encoders
 *		(untested - WiP)
 */

/*****************\
* MESSAGE FORMATS *
\*****************/

/* A UECP message */
struct uecp_message {
	uint8_t mec;		/* Message element code */
	/* The fields below are optional, depending
	 * on mec */
	uint8_t	dsn;		/* Data Set Number */
	uint8_t psn;		/* Program Service Number */
	uint8_t mel_len;	/* Message element length */
	uint8_t mel_data[254];	/* Message element data */
};

/* Message can be 255 bytes long (0xFF) */
#define UECP_MSG_LEN_MAX	255
/* Message element can be 254 bytes long when only mec
 * is defined */
#define UECP_MSG_MEL_LEN_MAX	UECP_MSG_LEN_MAX - 1
/* Since 255 is not a valid number for MEL, we can use
 * 0xFF to indicate that the field is not present */
#define UECP_MSG_MEL_NA		0xFF

/* MEC range: 0, FE and FF are reserved */
#define UECP_MSG_MEC_MIN	0x01
#define UECP_MSG_MEC_MAX	0xFD

/* Message Element Codes */
#define UECP_MEC_PI		0x01 /* Program Identification */
#define UECP_MEC_PS		0x02 /* Program Service name */
#define UECP_MEC_PIN		0x06 /* Program Item Number */
#define UECP_MEC_DI_PTYI	0x04 /* Decoder Information / Dynamic PTY Indicator */
#define UECP_MEC_TA_TP		0x03 /* Traffic Anouncement / Traffic Programme */
#define UECP_MEC_MS		0x05 /* Music / Speech switch */
#define UECP_MEC_PTY		0x07 /* Programme Type */
#define UECP_MEC_PTYN		0x3E /* Programme Type Name */
#define UECP_MEC_RT		0x0A /* RadioText */
#define UECP_MEC_RTC		0x0D /* Real Time Clock */
#define UECP_MEC_CT		0x19 /* Enable RTC (group 4 version A) transmits */
#define UECP_MEC_DSN_SELECT	0x1C /* Set active Data Set */
#define UECP_MEC_PSN_ENABLE	0x0B /* Enable / disable a specific PSN */
#define UECP_MEC_RDSON		0x1E /* Enable / disable the RDS output signal */
#define UECP_MEC_SET_SITE_ADDR	0x23 /* Add a site address to the encoder */
#define UECP_MEC_SET_ENC_ADDR	0x27 /* Add an encoder address to the encoder */
#define UECP_MEC_MSG_REQUEST	0x17 /* Request data from the encoder */
#define UECP_MEC_MSG_ACK	0x18 /* ACK of a received message */
#define UECP_MEC_SET_COMM_MODE	0x2C /* Set communication mode for the encoder */

/* DSN allows us to target a data set or set of data sets
 * within an encoder. Current data set is the RDS's output */
#define UECP_MSG_DSN_CURRENT_SET	0x00
#define UECP_MSG_DSN_MIN		1
#define UECP_MSG_DSN_MAX		0xFD
#define UECP_MSG_DSN_ALL_OTHER_SETS	0xFE
#define UECP_MSG_DSN_ALL_SETS		0xFF

/* PSN allows us to target a service within a data set
 * zero marks the main service in the data set and 
 * 1 - 255 mark EON services -services on other networks- */
#define UECP_MSG_PSN_MAIN		0x00
#define UECP_MSG_PSN_MIN		1
#define UECP_MSG_PSN_MAX		0xFF


/* A UECP data frame */

/* NOTE: This frame encapsulates the message.
 * Before it's transmitted it needs
 * to pass through "byte-stuffing" so that the
 * reserved start and stop bytes are not
 * present in the payload */
struct uecp_data_frame {
	uint16_t 		addr; 		/* Remote address */
	uint8_t			seq;		/* Sequence number */
	uint8_t 		msg_len;	/* Message Length -can be zero- */
	struct	uecp_message	msg;		/* Message */
	uint16_t		crc;		/* CRC using CCIT polynomial */
};

#define UECP_DF_START_BYTE	0xFE
#define UECP_DF_STOP_BYTE	0xFF

/* Encoder's site address, an encoder
 * can have multiple site addresses
 * to accept messages e.g. at a site
 * or regional, or country level */
#define UECP_SITE_ADDRESS_MIN 	0
#define UECP_SITE_ADDRESS_MAX 	1023
#define UECP_SITE_ADDRESS_BCAST	0 /* All sites */

/* Encoder's address, this one identifies
 * a specific encoder at that site, again
 * an encoder can have multiple addresses */
#define UECP_ENC_ADDRES_MIN 	0
#define UECP_ENC_ADDRESS_MAX 	63
#define UECP_ENC_ADDRESS_BCAST	0 /* All encoders */

/* addr field holds both site and encoder
 * addresses */
#define UECP_DF_SITE_ADDR_MASK	0xFFC0
#define UECP_DF_SITE_ADDR_SHIFT	6
#define UECP_DF_ENC_ADDR_MASK	0x3F
#define UECP_DF_ENC_ADDR_SHIFT	0

/* When sequence number is not used
 * seq field needs to be 0 */
#define UECP_DF_SEQ_DISABLED	0

/* max message length + data frame fields + start + stop */
#define UECP_DF_MAX_LEN		UECP_MSG_LEN_MAX + 6 + 2


/**********\
* COMMANDS *
\**********/

/* Flags for the Decoder Information and Dynamic PTY
 * indicator field */
#define UECP_DI_DYNPTY_STEREO		1	/* Stereo recording */
#define UECP_DI_DYNPTY_ARTIFICIAL_HEAD	2	/* Dummy head recording (Sound Model Indicator) */
#define UECP_DI_DYNPTY_COMPRESSED	4	/* Compressed transmission */
#define UECP_DI_DYNPTY_DYNAMIC_PTY	8	/* Dynamic PTY or EON */
#define UECP_DI_DYNPTY_DI_MASK		0x7

/* Traffic Anouncement / Traffic Programme field flags */
#define UECP_TATP_OFF			0x0
#define UECP_TATP_TA_ON			0x1
#define UECP_TATP_TP_ON			0x2

/* Real Time Clock */
#define UECP_RTC_OFFSET_MASK		0x3F
#define UECP_RTC_OFFSET_UNCHANGED	0xFF


/************\
* PROTOTYPES *
\************/

int uecp_init(struct rds_encoder *enc);
