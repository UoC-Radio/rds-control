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

#include <stdint.h>	/* For sized integers */
#include <errno.h>	/* For error numbers */
#include <stdlib.h>	/* For malloc/free */
#include <string.h>	/* For memset() */
#include <stdio.h>	/* For printf(...) */
#include <termios.h>	/* For serial port stuff */
#include <arpa/inet.h> 	/* For htons */
#include <fcntl.h>	/* For O_* macros */
#include <unistd.h>	/* For read/write etc */
#include <poll.h>	/* For poll() */
#include "rds.h"
#include "rds_ccodes.h"
#include "uecp.h"
#include "prais.h"


/***************\
* I/O FUNCTIONS *
\***************/

static int
rds_open_serial(const unsigned char* port)
{
	int fd = 0;
	int ret = 0;
	struct termios tty;
	memset(&tty, 0, sizeof(struct termios));


	fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
	if(fd < 0)
		return -EIO;

	ret = tcgetattr(fd, &tty);
	if(ret < 0)
		return -EIO;

	/* Set baud rate to 9600 */
	cfsetispeed(&tty, B9600);
	cfsetospeed(&tty, B9600);

	/* Set 8bit characters */
	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;

	/* Disable parity bits */
	tty.c_cflag &= ~(PARENB | PARODD);

	/* Disable XON/XOFF */
	tty.c_iflag &= ~(IXON | IXOFF | IXANY);

	/* Don't do any CR <-> NL or uppercase <-> lowercase
	 * translation and don't strip any bits */
	tty.c_iflag &= ~(INLCR | ICRNL | IUCLC | ISTRIP);
	tty.c_oflag &= ~(ONLCR | OCRNL | OLCUC);

	/* Disable echo, canonical processing,
	 * implementation-defined processing etc */
	tty.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

	/* Don't ignore breaks or parity errors etc */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | IGNCR);

	/* Set ony 1 stop bit */
	tty.c_cflag &= ~CSTOPB;

	/* Enable the receiver and ignore
	 * modem control lines */
	tty.c_cflag |= CLOCAL | CREAD;

	/* Disable output processing */
	tty.c_oflag &= ~OPOST;

	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 25;

	ret = tcsetattr(fd, TCSANOW, &tty);
	if(ret < 0)
		return -EIO;

	return fd;
}

static int
rds_close_serial(struct rds_encoder *enc)
{
	return close(enc->serial_fd);
}

int
rds_get_byte(struct rds_encoder *enc)
{
	struct pollfd fds;
	uint8_t in_byte = 0;
	int ret = 0;

	memset(&fds, 0, sizeof(struct pollfd));

	fds.fd = enc->serial_fd;
	fds.events = POLLIN|POLLPRI;
	ret = poll(&fds,1, 1000);	/* 1 second timeout */
	if(ret <= 0)
		return -ETIME;

	ret = read(enc->serial_fd, &in_byte, 1);
	if(ret < 0)
		return ret;

	//printf("In: 0x%02X (%c)\n", in_byte, in_byte);
	return in_byte;
}

int
rds_send_byte(struct rds_encoder *enc, char byte)
{
	struct pollfd fds;
	char out_byte = byte;
	int ret = 0;

	memset(&fds, 0, sizeof(struct pollfd));

	fds.fd = enc->serial_fd;
	fds.events = POLLOUT;
	ret = poll(&fds,1,1000); /* 1 second timeout */
	if(ret <= 0)
		return -ETIME;

	ret = write(enc->serial_fd, &out_byte, 1);
	if(ret < 0)
		return ret;

	//printf("Out: 0x%02X (%c)\n", out_byte, out_byte);

	tcdrain(enc->serial_fd);
	return ret;
}


/**********\
* COMMANDS *
\**********/

/**
 * rds_get_pi -	Get Programme Identifier information
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @pi: pointer to &struct rds_pi to fill
 */
int
rds_get_pi(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_pi *pi)
{
	if(enc->get_pi == NULL)
		return -EOPNOTSUPP;

	return enc->get_pi(enc, dsn, psn, pi);
}

/**
 * rds_set_pi -	Set Programme Identifier information
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @pi: pointer to &struct rds_pi containing the infos
 */
int
rds_set_pi(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_pi *pi)
{
	if(enc->set_pi == NULL)
		return -EOPNOTSUPP;

	return enc->set_pi(enc, dsn, psn, pi);
}

/**
 * rds_get_ps -	Get Programme Service name
 * @enc: pointer to &struct rds_encoder
 * @dsn: message group (1-2)
 * @psn: message id (0-14) (only one supported so always 0)
 * @ps: a pre-allocated 8byte char array to fill
 */
int
rds_get_ps(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ps)
{
	if(enc->get_ps == NULL)
		return -EOPNOTSUPP;

	return enc->get_ps(enc, dsn, psn, ps);
}

/**
 * rds_set_ps -	Set Programme Service name
 * @enc: pointer to &struct rds_encoder
 * @dsn: message group (1-2)
 * @psn: message id (0-14) (only one supported so always 0)
 * @ps: PS name to set
 */
int
rds_set_ps(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ps)
{
	if(enc->set_ps == NULL)
		return -EOPNOTSUPP;

	return enc->set_ps(enc, dsn, psn, ps);
}

/**
 * rds_get_rt -	Get RadioText message
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @rt: the &struct rds_rt to fill
 */
int
rds_get_rt(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_rt *rt)
{
	if(enc->get_rt == NULL)
		return -EOPNOTSUPP;

	return enc->get_rt(enc, dsn, psn, rt);
}

/**
 * rds_set_rt -	Set RadioText message
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @rt: the &struct rds_rt to send
 */
int
rds_set_rt(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, struct rds_rt *rt)
{
	if(enc->set_rt == NULL)
		return -EOPNOTSUPP;

	return enc->set_rt(enc, dsn, psn, rt);
}

/**
 * rds_get_di - Get decoder info field
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 */
int
rds_get_di(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	if(enc->get_di == NULL)
		return -EOPNOTSUPP;

	return enc->get_di(enc, dsn, psn);
}

/**
 * rds_set_di - Set decoder info field
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @di: RDS_DI_* flags
 */
int
rds_set_di(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t di)
{
	if(enc->set_di == NULL)
		return -EOPNOTSUPP;

	return enc->set_di(enc, dsn, psn, di);
}

/**
 * rds_get_dynpty - Get dynamic PTY indicator
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 */
int
rds_get_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	if(enc->get_dynpty == NULL)
		return -EOPNOTSUPP;

	return enc->get_dynpty(enc, dsn, psn);
}

/**
 * rds_set_dynpty - Set dynamic PTY indicator
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @dynpty: 1 -> Enabled, 0 -> Disabled
 */
int
rds_set_dynpty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t dynpty)
{
	if(enc->set_dynpty == NULL)
		return -EOPNOTSUPP;

	return enc->set_dynpty(enc, dsn, psn, dynpty);
}

/**
 * rds_set_ta_tp - Get TA/TP status
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 */
int
rds_get_ta_tp(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	if(enc->get_ta_tp == NULL)
		return -EOPNOTSUPP;

	return enc->get_ta_tp(enc, dsn, psn);
}

/**
 * rds_set_ta_tp - Set TA/TP status
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @ta_tp: RDS_TATP_* status flags
 */
int
rds_set_ta_tp(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ta_tp)
{
	if(enc->set_ta_tp == NULL)
		return -EOPNOTSUPP;

	return enc->set_ta_tp(enc, dsn, psn, ta_tp);
}

/**
 * rds_get_ms - Get M/S switch status
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 */
int
rds_get_ms(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	if(enc->get_ms == NULL)
		return -EOPNOTSUPP;

	return enc->get_ms(enc, dsn, psn);
}

/**
 * rds_set_ms - Set M/S switch status
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @ms: RDS_MS_* status flags
 */
int
rds_set_ms(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t ms)
{
	if(enc->set_ms == NULL)
		return -EOPNOTSUPP;

	return enc->set_ms(enc, dsn, psn, ms);
}

/**
 * rds_get_pty - Get Programme Type
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 */
int
rds_get_pty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn)
{
	if(enc->get_pty == NULL)
		return -EOPNOTSUPP;

	return enc->get_pty(enc, dsn, psn);
}

/**
 * rds_set_pty - Set Programme Type
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @pty: The pty (0 - 19) to set
 */
int
rds_set_pty(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, uint8_t pty)
{
	if(enc->set_pty == NULL)
		return -EOPNOTSUPP;

	return enc->set_pty(enc, dsn, psn, pty);
}

/**
 * rds_get_ptyn - Get Programme Type Name
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @ptyn: pre-allocated PTY name string to fill
 */
int
rds_get_ptyn(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ptyn)
{
	if(enc->get_ptyn == NULL)
		return -EOPNOTSUPP;

	return enc->get_ptyn(enc, dsn, psn, ptyn);
}

/**
 * rds_set_ptyn - Set Programme Type Name
 * @enc: pointer to &struct rds_encoder
 * @dsn: Data Segment Number
 * @psn: Programme Service Number
 * @ptyn: PTY name string to set
 */
int
rds_set_ptyn(struct rds_encoder *enc, uint8_t dsn, uint8_t psn, char* ptyn)
{
	if(enc->set_ptyn == NULL)
		return -EOPNOTSUPP;

	return enc->set_ptyn(enc, dsn, psn, ptyn);
}

/**
 * rds_get_ct - Get transmission status of RTC
 * @enc: pointer to &struct rds_encoder
 */
int
rds_get_ct(struct rds_encoder *enc)
{
	if(enc->get_ct == NULL)
		return -EOPNOTSUPP;

	return enc->get_ct(enc);
}

/**
 * rds_set_ct - Enable/disable transmission of RTC
 * @enc: pointer to &struct rds_encoder
 * @ct: 0 -> disable, 1 -> enable
 */
int
rds_set_ct(struct rds_encoder *enc, uint8_t ct)
{
	if(enc->set_ct == NULL)
		return -EOPNOTSUPP;

	return enc->set_ct(enc, ct);
}

/**
 * rds_get_rtc - Set Real Time Clock settings on the device
 * @enc: pointer to &struct rds_encoder
 * @rtc: the &struct rds_rtc to fill
 */
int
rds_get_rtc(struct rds_encoder *enc, struct rds_rtc *rtc)
{
	if(enc->get_rtc == NULL)
		return -EOPNOTSUPP;

	return enc->get_rtc(enc, rtc);
}

/**
 * rds_set_rtc - Set Real Time Clock settings on the device
 * @enc: pointer to &struct rds_encoder
 * @rtc: the &struct rds_rtc to send
 */
int
rds_set_rtc(struct rds_encoder *enc, struct rds_rtc *rtc)
{
	if(enc->set_rtc == NULL)
		return -EOPNOTSUPP;

	return enc->set_rtc(enc, rtc);
}

/**
 * rds_get_rds_on - Get the encoder's RDS output status
 * @enc: pointer to &struct rds_encoder
 */
int
rds_get_rds_on(struct rds_encoder *enc)
{
	if(enc->get_rds_on == NULL)
		return -EOPNOTSUPP;

	return enc->get_rds_on(enc);
}

/**
 * rds_set_rds_on - Set encoder's RDS output status
 * @enc: pointer to &struct rds_encoder
 * @on: 1 -> Enable, 0 -> Disable
 */
int
rds_set_rds_on(struct rds_encoder *enc, uint8_t on)
{
	if(enc->set_rds_on == NULL)
		return -EOPNOTSUPP;

	return enc->set_rds_on(enc, on);
}


/*************\
* INIT / EXIT *
\*************/

struct rds_encoder *
rds_init(uint8_t type, uint16_t site_addr, uint16_t enc_addr,
					const unsigned char* port)
{
	struct rds_encoder *enc = NULL;
	uint16_t site_addr_le = 0;
	uint16_t enc_addr_le = 0;
	int fd = 0;

	/* Fix endianess if needed */
	if(ntohs(1) == 1) {
		site_addr_le = (site_addr & 0x00FF) << 8 |
				(site_addr & 0xFF00) >> 8;

		enc_addr_le = (enc_addr & 0x00FF) << 8 |
				(enc_addr & 0xFF00) >> 8;
	} else {
		site_addr_le = site_addr;
		enc_addr_le = enc_addr;
	}

	enc = malloc(sizeof(struct rds_encoder));
	if(!enc)
		return NULL;
	memset(enc, 0, sizeof(struct rds_encoder));

	switch(type){
		case RDS_ENCODER_TYPE_PRAIS:
			enc->addr = enc_addr_le;
			prais_init(enc);
			break;
		case RDS_ENCODER_TYPE_UECP:
			enc->addr |= (site_addr_le & 0x3FF) << 6;
			enc->addr |= enc_addr_le & 0x3F;
			uecp_init(enc);
			break;
		default:
			return NULL;
	}

	fd = rds_open_serial(port);
	if(fd < 0)
		return NULL;
	enc->serial_fd = fd;

	return enc;
}

int
rds_exit(struct rds_encoder *enc)
{
	rds_close_serial(enc);
	free(enc);
	return 0;
}
