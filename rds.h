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

/**********\
* COMMANDS *
\**********/

/* Decoder Info flags */
#define RDS_DI_STEREO			0x1	/* Stereo recording */
#define RDS_DI_ARTIFICIAL_HEAD		0x2	/* Dummy head recording (Sound Model Indicator) */
#define RDS_DI_COMPRESSED		0x4	/* Compressed transmission */

/* Traffic Anouncement / Traffic Programme field flags */
#define RDS_TATP_OFF			0x0
#define RDS_TATP_TA_ON			0x1
#define RDS_TATP_TP_ON			0x2

/* Music / Speech switch settings */
#define RDS_MS_DEFAULT			0x1
#define RDS_MS_MUSIC			0x1
#define RDS_MS_SPEECH			0x2


/* Programme Idendifier */
struct rds_pi {
	uint16_t ccode;		/* Country code and extended country code
				 * as defined inside rds_ccodes.h */
	uint8_t coverage;	/* Coverage class */
	uint8_t prn;		/* Programme Reference Number */
};

/* Coverage classes */
#define RDS_PI_COVERAGE_LOCAL		0x0
#define RDS_PI_COVERAGE_INTERNATIONAL	0x1
#define RDS_PI_COVERAGE_NATIONAL	0x2
#define RDS_PI_COVERAGE_SUPRA_REGIONAL	0x3
#define RDS_PI_COVERAGE_REGIONAL1	0x4
#define RDS_PI_COVERAGE_REGIONAL2	0x5
#define RDS_PI_COVERAGE_REGIONAL3	0x6
#define RDS_PI_COVERAGE_REGIONAL4	0x7
#define RDS_PI_COVERAGE_REGIONAL5	0x8
#define RDS_PI_COVERAGE_REGIONAL6	0x9
#define RDS_PI_COVERAGE_REGIONAL7	0xA
#define RDS_PI_COVERAGE_REGIONAL8	0xB
#define RDS_PI_COVERAGE_REGIONAL9	0xC
#define RDS_PI_COVERAGE_REGIONAL10	0xD
#define RDS_PI_COVERAGE_REGIONAL11	0xE
#define RDS_PI_COVERAGE_REGIONAL12	0xF


/* A struct to hold RadioText data and config */
struct rds_rt {
	uint8_t ab_flag;		/* Use method A or B */
	uint8_t retransmissions;	/* Number of retransmissions, max 15 */
	uint8_t buffer_config;		/* Buffer config flags (see below) */
	uint8_t msg[65];		/* Message buffer, max 65 including NULL terminator */
};

/* RadioText */
#define RDS_RT_MSG_LEN_MAX		65
#define RDS_RT_BUFF_CONFIG_FLUSH	0x0	/* Flush buffer and send provided message */
#define RDS_RT_BUFF_CONFIG_APPEND	0x2	/* Append provided message to the buffer and enqueue it */
#define RDS_RT_METHOD_A			0x1
#define RDS_RT_METHOD_B			0x2

/* A struct to hold RTC data */
struct rds_rtc {
	uint16_t year;
	uint8_t month;
	uint8_t	day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t centiseconds;
	int8_t offset;
};


/*************\
* MAIN HANDLE *
\*************/

/* An encoder */
struct rds_encoder {
	uint8_t type;			/* Encoder type */
	uint16_t flags;			/* Encoder specific flags */
	uint16_t addr;			/* Encoder's address */
	int serial_fd;			/* File descriptor of the opened serial port */
	uint8_t seq;			/* Sequence number of last packet */
	uint8_t	rt_num;			/* Number of radiotext buffers */

	/* Device specific methods, used internaly */
	int (*get_pi)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							struct rds_pi *pi);
	int (*set_pi)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							struct rds_pi *pi);
	int (*get_ps)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							char* ps);
	int (*set_ps)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							char* ps);
	int (*get_rt)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							struct rds_rt *rt);
	int (*set_rt)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							struct rds_rt *rt);
	int (*get_di)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);
	int (*set_di)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							uint8_t di);
	int (*get_dynpty)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);
	int (*set_dynpty)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							uint8_t dynpty);
	int (*get_ta_tp)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);
	int (*set_ta_tp)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							uint8_t ta_tp);
	int (*get_ms)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);
	int (*set_ms)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							uint8_t ms);
	int (*get_pty)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);
	int (*set_pty)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							uint8_t pty);
	int (*get_ptyn)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							char* ptyn);
	int (*set_ptyn)(struct rds_encoder *enc, uint8_t dsn, uint8_t psn,
							char* ptyn);
	int (*get_ct)(struct rds_encoder *enc);
	int (*set_ct)(struct rds_encoder *enc, uint8_t ct);
	int (*get_rtc)(struct rds_encoder *enc, struct rds_rtc *rtc);
	int (*set_rtc)(struct rds_encoder *enc, struct rds_rtc *rtc);
	int (*get_rds_on)(struct rds_encoder *enc);
	int (*set_rds_on)(struct rds_encoder *enc, uint8_t on);
};

/* Type */
#define RDS_ENCODER_TYPE_UECP	0x01
#define RDS_ENCODER_TYPE_PRAIS	0x02

/* Flags */
#define RDS_ENCODER_FLAGS_PRAIS_HW_DYNPS	0x01	/* Support dynamic PS on hardware for
							 * Prais Coder mod. 735 */

/************\
* PROTOTYPES *
\************/

/* Input / Output -used internaly- */
int
rds_get_byte(struct rds_encoder *enc);

int
rds_send_byte(struct rds_encoder *enc, char byte);


/* Commands */

int
rds_get_pi(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_pi *pi);

int
rds_set_pi(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_pi *pi);

int
rds_get_ps(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ps);

int
rds_set_ps(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ps);

int
rds_get_rt(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_rt *rt);

int
rds_set_rt(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_rt *rt);

int
rds_get_di(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);

int
rds_set_di(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t di);

int
rds_get_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);

int
rds_set_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t dynpty);

int
rds_get_ta_tp(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);

int
rds_set_ta_tp(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ta_tp);

int
rds_get_ms(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);

int
rds_set_ms(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ms);

int
rds_get_pty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn);

int
rds_set_pty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t pty);

int
rds_get_ptyn(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ptyn);

int
rds_set_ptyn(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ptyn);

int
rds_get_ct(struct rds_encoder *enc);

int
rds_set_ct(struct rds_encoder *enc, uint8_t ct);

int
rds_get_rtc(struct rds_encoder *enc, struct rds_rtc *rtc);

int
rds_set_rtc(struct rds_encoder *enc, struct rds_rtc *rtc);

int
rds_get_rds_on(struct rds_encoder *enc);

int
rds_set_rds_on(struct rds_encoder *enc, uint8_t on);


/* Init / Exit */
struct rds_encoder *
rds_init(uint8_t type, uint16_t site_addr, uint16_t enc_addr,
				const unsigned char* port);

int
rds_exit(struct rds_encoder *enc);


