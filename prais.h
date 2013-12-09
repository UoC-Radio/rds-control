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
 * prais.h - 	Support for Prais Coder mod. 735
 *		(and possibly 732 but haven't tested it)
 */

/******************************\
* DATA LINK CONTROL CHARACTERS *
\******************************/

/* These are common ASCII control characters
 * used for signaling on the serial port */
#define PRAIS_DL_SYN	0x16	/* SYNchronize */
#define PRAIS_DL_SOH	0x01	/* Start Of Header */
#define PRAIS_DL_ACK	0x06	/* ACK */
#define PRAIS_DL_DLE	0x10	/* Data Link Escape */
#define PRAIS_DL_STX	0x02	/* Start of TeXt */
#define PRAIS_DL_ETX	0x03	/* End of TeXt */


/*****************\
* MESSAGE FORMATS *
\*****************/

/* A Message for Prais Coder mod. 735 */
struct prais_message {
	uint8_t type;		/* Message type code */
	uint8_t len;		/* Message len */
	uint8_t data[40];	/* Message */
	uint16_t checksum;	/* The sum of type and data fields
				 * cut to one byte (ANDed with 0xFF)
				 * and transmitted as two ascii chars */
};

/* Message types */
#define PRAIS_MT_RESET		0x00	/* Unit reset */
#define PRAIS_MT_RDSON		0x01	/* RDS Enable */
#define PRAIS_MT_LOCK_STATUS	0x02	/* RDS Lock status request */
#define PRAIS_MT_TAMSDI		0x03	/* Traffic Anouncement, M/S switch and Decoder Info */
#define PRAIS_MT_UNITMSG	0x04	/* Unit Message (?) */
#define PRAIS_MT_AF		0x05	/* Alternative Frequencies */
#define PRAIS_MT_STORE		0x06	/* Store data on device */
#define PRAIS_MT_SERIAL_NUM	0x08	/* Unit serial number request / set unit address */
#define PRAIS_MT_PI		0x09	/* Programme Identifier */
#define PRAIS_MT_PS		0x0A	/* Programme Service Name */
#define PRAIS_MT_RTC		0x0B	/* Real Time Clock */
#define PRAIS_MT_PTY		0x0C	/* Programme Type */
#define PRAIS_MT_TDC		0x0D	/* Traffic Data Channel data -unsupported- */
#define PRAIS_MT_IH		0x0E	/* In House data -unsupported- */
#define PRAIS_MT_RT		0x0F	/* RadioText */

#define PRAIS_MT_MIN_VAL	PRAIS_MT_RESET
#define PRAIS_MT_MAX_VAL	PRAIS_MT_RT

#define PRAIS_MT_MAX_LEN	40

/* A data frame for Prais Coder mod. 735 */
struct prais_data_frame {
	uint8_t no_reply;		/* Set the no reply flag */
	struct prais_message msg;
};

/* The no reply flag is set on the high byte
 * of the address field */
#define PRAIS_DF_NO_REPLY	0x8000

/* Broadcast address */
#define PRAIS_DF_ADDR_BCAST	0xFFFF

/* Sequence number is 0 - 9 ASCII */
#define PRAIS_DF_SEQ_MIN	0x30
#define PRAIS_DF_SEQ_MAX	0x39

#define PRAIS_DF_MAX_LEN	48

/**********\
* COMMANDS *
\**********/

/**
 * DOC: Hardware capabilities (Prais Coder mod. 735)
 *
 * NOTE: Only one programme is supported by the hw.
 *
 * In all commands that accept DSN and PSN, except the PS command,
 * any non-zero value for DSN and PSN will result an error. 
 *
 * NOTE: Unit has the ability to handle dynamic PS on hw.
 *
 * Only one programme is supported by the device
 * but 2 message groups are provided, each one for
 * handling dynamic PS (for this single programme)
 * on hw. Each message group contains 15 messages and
 * a number of retransmissions for each message (up to 100),
 * forming a message queue to be transmitted. User can
 * change the active message group by using a physical switch
 * on the unit's front panel.
 *
 * In general we don't support dynamic PS on hw because it's
 * not flexible and it breaks abstraction with other devices
 * (e.g. we need an extra input for number of retransmissions),
 * so we always set a single message per message group. To enable
 * dynamic PS hw support, set the RDS_ENCODER_FLAGS_PRAIS_HW_DYNPS
 * and use dsn as the message group and psn as the message index on
 * that message group. Since there is no way to provide number of
 * retransmissions, we set the number of retransmissions to 10 for
 * the first group entry (e.g. the station's name) and 4 for all the
 * rest.
 *
 * Even when not using this feature, dsn can still be used to set an
 * alternative PS on group 2 so that the user can still use the physical
 * switch, e.g. to switch to a backup PS. PSN however still needs to be 0.
 */

/* Flags for the T.A., M/S, DI field */
#define PRAIS_TAMSDI_TA_ON		0x02
#define PRAIS_TAMSDI_TP_ON		0x04
#define PRAIS_TAMSDI_MS_MUSIC		0x40
#define PRAIS_TAMSDI_DI_STEREO		0x01
#define PRAIS_TAMSDI_DI_ART_HEAD	0x08
#define PRAIS_TAMSDI_DI_COMPRESSED	0x10
#define PRAIS_TAMSDI_DYNPTY		0x20
#define PRAIS_TAMSDI_TATP_ON		0x80	/* Got this on reply after
						 * requesting TAMSDI with
						 * both TA/TP flags on.
						 */
#define PRAIS_TAMSDI_TATP_MASK		(PRAIS_TAMSDI_TA_ON |\
					PRAIS_TAMSDI_TP_ON)

#define PRAIS_TAMSDI_DI_MASK		(PRAIS_TAMSDI_DI_STEREO |\
					PRAIS_TAMSDI_DI_ART_HEAD |\
					PRAIS_TAMSDI_DI_COMPRESSED)

/* Programme Service */
/* Flag to add to the index field to indicate message group 2 */
#define PRAIS_PSN_INDEX_GROUP2		0x80


/************\
* PROTOTYPES *
\************/

int prais_init(struct rds_encoder *enc);
