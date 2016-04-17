/***************************************************************************
 * LPRng - An Extended Print Spooler System
 *
 * Copyright 1988-2003, Patrick Powell, San Diego, CA
 *     papowell@lprng.com
 * See LICENSE for conditions of use.
 * $Id: krb5_auth.h,v 1.4 2005/04/14 20:05:20 papowell Exp $
 ***************************************************************************/



#ifndef _KRB5_AUTH_H
#define _KRB5_AUTH_H 1

/* PROTOTYPES */
int server_krb5_status( int sock, char *err, int errlen, char *file, int use_crypt_transfer );
int client_krb5_auth( char *keytabfile, char *service, char *host,
	char *server_principal,
	int options, char *life, char *renew_time,
	int sock, char *err, int errlen, char *file, int use_crypt_transfer );
int remote_principal_krb5( char *service, char *host, char *err, int errlen );
char *krb4_err_str( int err );
int Send_krb4_auth( struct job *job, int *sock,
	int connect_timeout, char *errmsg, int errlen,
	struct security *security, struct line_list *info );
int Receive_k4auth( int *sock, char *input );
int Krb5_receive_work( int *sock,
	int transfer_timeout,
	char *user, char *jobsize, int from_server, char *authtype,
	struct line_list *info,
	char *errmsg, int errlen,
	struct line_list *header_info,
	struct security *security, char *tempfile, int use_crypt_transfer );
int Krb5_receive( int *sock,
	int transfer_timeout,
	char *user, char *jobsize, int from_server, char *authtype,
	struct line_list *info,
	char *errmsg, int errlen,
	struct line_list *header_info,
	struct security *security, char *tempfile );
int Krb5_receive_nocrypt( int *sock,
	int transfer_timeout,
	char *user, char *jobsize, int from_server, char *authtype,
	struct line_list *info,
	char *errmsg, int errlen,
	struct line_list *header_info,
	struct security *security, char *tempfile );
int Krb5_send_work( int *sock,
	int transfer_timeout,
	char *tempfile,
	char *error, int errlen,
	struct security *security, struct line_list *info, int use_crypt_transfer );
int Krb5_send( int *sock,
	int transfer_timeout,
	char *tempfile,
	char *error, int errlen,
	struct security *security, struct line_list *info );
int Krb5_send_nocrypt( int *sock,
	int transfer_timeout,
	char *tempfile,
	char *error, int errlen,
	struct security *security, struct line_list *info );
int GetKerberosUser_from_gCCacheStr( char *buffer, int bufferlen );

#endif
