#include <dnswire/reader.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <err.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <ctype.h>
#include <signal.h>

#define	SYSLOG_NAMES
#include <syslog.h>

#include "b64.h"
#include "sensor.h"

static struct sensor_handle *g_sh = NULL;

static const char* printable_ip_address(const uint8_t* data, size_t len)
{
    static char buf[INET6_ADDRSTRLEN];

    buf[0] = 0;
    if (len == 4) {
        inet_ntop(AF_INET, data, buf, sizeof(buf));
    } else if (len == 16) {
        inet_ntop(AF_INET6, data, buf, sizeof(buf));
    }

    return buf;
}

static char* print_dnstap(const struct dnstap* d)
{
    static char src[4096];
    static char dst[8192];
    char *enc;
    size_t enc_len;
    u_int8_t is_query;

    dst[0] = '\0';

    if (dnstap_type(*d) == DNSTAP_TYPE_MESSAGE && dnstap_has_message(*d)) {
        if (dnstap_message_has_socket_family(*d)) {
            if(dnstap_message_socket_family(*d) != DNSTAP_SOCKET_FAMILY_INET)
                goto err;
        }
        if (dnstap_message_has_socket_protocol(*d)) {
            if(dnstap_message_socket_protocol(*d) != DNSTAP_SOCKET_PROTOCOL_UDP)
                goto err;
        }

        sprintf(src, "%s %d", 
                    DNSTAP_MESSAGE_TYPE_STRING[dnstap_message_type(*d)],
                    dnstap_message_type(*d));
        strcat(dst, src);

        is_query = dnstap_message_type(*d) % 2;

        /*if (is_query && dnstap_message_has_query_time_sec(*d) 
                && dnstap_message_has_query_time_nsec(*d)) {
            sprintf(src, " %lu %d", 
                        dnstap_message_query_time_sec(*d), 
                        dnstap_message_query_time_nsec(*d));
            strcat(dst, src);
        }

        if (!is_query && dnstap_message_has_response_time_sec(*d) 
                && dnstap_message_has_response_time_nsec(*d)) {
            sprintf(src, " %lu %d", 
                        dnstap_message_response_time_sec(*d), 
                        dnstap_message_response_time_nsec(*d));
            strcat(dst, src);
        }*/

        if (dnstap_message_has_query_address(*d)) {
            sprintf(src, " %s", 
                        printable_ip_address(dnstap_message_query_address(*d),
                                dnstap_message_query_address_length(*d)));
            strcat(dst, src);
        }

        if (dnstap_message_has_query_port(*d)) {
            sprintf(src, " %u", dnstap_message_query_port(*d));
            strcat(dst, src);
        }

        if (dnstap_message_has_response_address(*d)) {
            sprintf(src, " %s", 
                    printable_ip_address(dnstap_message_response_address(*d), 
                                dnstap_message_response_address_length(*d)));
            strcat(dst, src);
        }

        if (dnstap_message_has_response_port(*d)) {
            sprintf(src, " %u", dnstap_message_response_port(*d));
            strcat(dst, src);
        }

        if (is_query && dnstap_message_has_query_time_sec(*d)
                && dnstap_message_has_query_message(*d)) {
            sprintf(src, " %lu", dnstap_message_query_message_length(*d));
            strcat(dst, src);

            enc = b64_encode(dnstap_message_query_message(*d),
                                dnstap_message_query_message_length(*d),
                                &enc_len);

            sprintf(src, " %lu ", enc_len);
            strcat(dst, src);
            strcat(dst, enc);
            free(enc);
        }
        
        if (!is_query && dnstap_message_has_response_time_sec(*d)
                && dnstap_message_has_response_message(*d)) {
            sprintf(src, " %lu", dnstap_message_response_message_length(*d));
            strcat(dst, src);

            enc = b64_encode(dnstap_message_response_message(*d),
                                dnstap_message_response_message_length(*d),
                                &enc_len);

            sprintf(src, " %lu ", enc_len);
            strcat(dst, src);
            strcat(dst, enc);
            free(enc);
        }
    }

    return dst;
err:
    return NULL;
}

static int decode(const char *name, const CODE *codetab)
{
	register const CODE *c;

	if (name == NULL || *name == '\0')
		return -1;
	if (isdigit(*name)) {
		int num;
		char *end = NULL;

		errno = 0;
		num = strtol(name, &end, 10);
		if (errno || name == end || (end && *end))
			return -1;
		for (c = codetab; c->c_name; c++)
			if (num == c->c_val)
				return num;
		return -1;
	}
	for (c = codetab; c->c_name; c++)
		if (!strcasecmp(name, c->c_name))
			return (c->c_val);

	return -1;
}

static int pencode(const char *s)
{
	int facility, level;
	char *separator;

	assert(s);

	separator = strchr(s, '.');
	if (separator) {
		*separator = '\0';
		facility = decode(s, facilitynames);
		if (facility < 0)
			errx(EXIT_FAILURE, ("unknown facility name: %s"), s);
		s = ++separator;
	} else
		facility = LOG_USER;
	level = decode(s, prioritynames);
	if (level < 0)
		errx(EXIT_FAILURE, ("unknown priority name: %s"), s);
	if (facility == LOG_KERN)
		facility = LOG_USER;	/* kern is forbidden */
	return ((level & LOG_PRIMASK) | (facility & LOG_FACMASK));
}

void usage(char *argv[])
{
    fprintf(stderr, "usage: %s <-u unix socket path> <-p facility.priority> " \
                    "[-l facility.priority for log] [-d]\n",
            argv[0]);

	exit(1);
}

int main(int argc, char* argv[])
{
    int pri;
    char *log;
	int ch;
    char *sun_path = NULL;
    char *fac_pri = NULL;
    char *log_fac_pri = "local6.debug";
    int dn = 0;

	while ((ch = getopt(argc, argv, "u:p:dl:")) != -1) {
		switch (ch) {
		case 'd':
			dn = 1;
			break;
		case 'u':
			sun_path = optarg;
			break;
		case 'p':
			fac_pri = optarg;
			break;
		case 'l':
			log_fac_pri = optarg;
			break;
		default:
            usage(argv);
		}
	}
    if (!sun_path || !fac_pri) {
        usage(argv);
    }

    g_sh = (struct sensor_handle *) calloc(1, sizeof(struct sensor_handle));
    if (g_sh == NULL){
        fprintf(stderr, "Failed to calloc g_sh\n");
        return 1;
    }

    pri = pencode(fac_pri);
    g_sh->log_pri = pencode(log_fac_pri);

    gethostname(g_sh->hostname, sizeof(g_sh->hostname));

    struct dnswire_reader reader;
    if (dnswire_reader_init(&reader) != dnswire_ok) {
        fprintf(stderr, "Unable to initialize dnswire reader\n");
        return 1;
    }
    if (dnswire_reader_allow_bidirectional(&reader, true) != dnswire_ok) {
        fprintf(stderr, "Unable to set dnswire reader to bidirectional mode\n");
        return 1;
    }

	if (dn && daemon(1, 0) == -1)
		err(1, "unable to daemonize");

	signal(SIGPIPE, SIG_IGN);

    sensor_log_init(g_sh);

    /******************************************/
    int reader_sockfd;
    struct sockaddr_un path;
    int ret;

    memset(&path, 0, sizeof(struct sockaddr_un));
    path.sun_family = AF_UNIX;
    strncpy(path.sun_path, sun_path, sizeof(path.sun_path) - 1);

    /* Remove a previously bound socket existing on the filesystem. */	
    ret = remove(path.sun_path);
    if (ret != 0 && errno != ENOENT) {
        fprintf(stderr, "Failed to remove existing socket path %s\n", 
                    path.sun_path);
		return false;
    }
    reader_sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (reader_sockfd == -1) {
        fprintf(stderr, "socket() failed: %s\n", strerror(errno));
        return 1;
    }
    printf("socket\n");

    if (bind(reader_sockfd, (struct sockaddr*)&path, sizeof(struct sockaddr_un))) {
        fprintf(stderr, "bind() failed: %s\n", strerror(errno));
        close(reader_sockfd);
        return 1;
    }
    printf("bind\n");

    if (listen(reader_sockfd, 1)) {
        fprintf(stderr, "listen() failed: %s\n", strerror(errno));
        close(reader_sockfd);
        return 1;
    }
    printf("listen\n");

    int clifd = accept(reader_sockfd, 0, 0);
    if (clifd < 0) {
        fprintf(stderr, "accept() failed: %s\n", strerror(errno));
        close(reader_sockfd);
        return 1;
    }
    printf("accept\n");

    /******/
    int done = 0;

    printf("receiving...\n");
    while (!done) {
        switch (dnswire_reader_read(&reader, clifd)) {
        case dnswire_have_dnstap:
            g_sh->count_input++;
            log = print_dnstap(dnswire_reader_dnstap(reader));
            if (log) {
                g_sh->count_output++;
                syslog(pri, "%s", log);
                if (!dn)
                    printf("%s\n", log);
            }
            break;
        case dnswire_again:
        case dnswire_need_more:
            /*
             * This indicates that we need to call the reader again as it
             * will only do one pass in a non-blocking fasion.
             */
            break;
        case dnswire_endofdata:
            /*
             * The stream was stopped from the sender side, we're done!
             */
            printf("stopped\n");
            done = 1;
            break;
        default:
            fprintf(stderr, "dnswire_reader_read() error\n");
            done = 1;
        }
    }

    dnswire_reader_destroy(reader);
    shutdown(clifd, SHUT_RDWR);
    close(clifd);
    close(reader_sockfd);

    return 0;
}
