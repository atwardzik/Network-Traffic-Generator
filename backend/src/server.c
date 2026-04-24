#include "server.h"

#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>

static pthread_mutex_t mut;
static pthread_cond_t cond;
static unsigned int active_connections = 0;

static unsigned int counter;
constexpr int MAX_ACTIVE_CONNECTIONS = 10;

static const char *http_header = "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %u\r\n"
        "Connection: close\r\n\r\n";

const char *server_tokens[] = {"${{TIME}}", "${{TEST}}"};

static char *read_html(const char *filename) {
        const int fd = open(filename, O_RDONLY);
        if (fd < 0) {
                return nullptr;
        }

        struct stat st;
        fstat(fd, &st);
        char *buffer = malloc(st.st_size);
        if (!buffer) {
                return nullptr;
        }
        if (read(fd, buffer, st.st_size) <= 0) {
                free(buffer);
                return nullptr;
        }

        return buffer;
}

static int replace_in_html(char **html, const char *token, const char *value) {
        unsigned int index = 0;
        const unsigned int html_length = strlen(*html);
        const unsigned int buffer_length = html_length;
        do {
                char *substring = strstr(*html, token);
                if (!substring) {
                        return 0;
                }
                index = (uintptr_t) substring - (uintptr_t) *html;

                if (html_length - strlen(token) + strlen(value) >= buffer_length) {
                        char *new_html = realloc(*html, buffer_length + strlen(value) + 1);
                        if (!new_html) {
                                return -ENOMEM;
                        }
                        *html = new_html;
                }

                const unsigned int tail_index = index + strlen(token);
                char *buffer_tail = malloc(buffer_length - tail_index);
                if (!buffer_tail) {
                        return -ENOMEM;
                }
                strcpy(buffer_tail, *html + tail_index);
                strcpy(*html + index, value);
                strcpy(*html + strlen(*html), buffer_tail);

                free(buffer_tail);
        } while (index < html_length);

        return 0;
}

static int replace_server_token(char **html, const char *token) {
        if (strcmp(token, "${{TIME}}") == 0) {
                struct timeval tv;
                gettimeofday(&tv, nullptr);

                const struct tm *time = localtime(&tv.tv_sec);

                char datetime[80];

                strftime(datetime, sizeof(datetime), "%a %d %b %Y %H:%M:%S", time);

                replace_in_html(html, token, datetime);
        }

        return 0;
}

static int replace_server_tokens(char **html) {
        constexpr int tokens_count = sizeof(server_tokens) / sizeof(*server_tokens);
        for (size_t i = 0; i < tokens_count; ++i) {
                replace_server_token(html, server_tokens[i]);
        }

        return 0;
}

static void *manage_connection(void *arg) {
        const int sockfd = (int) (intptr_t) arg;
        int ret = 0;

        printf("Connection %u accepted\n", counter);

        char *website = read_html("index.html");
        if (!website) {
                ret = -ENOENT;
                goto conn_close;
        }
        replace_server_tokens(&website);
        int html_length = strlen(website);

        int http_response_length = strlen(http_header) - 2 + snprintf(nullptr, 0, "%u", html_length) +
                                   strlen(website) + 1;

        char *http_response = malloc(http_response_length);
        if (!http_response) {
                ret = -ENOMEM;
                goto conn_close;
        }
        snprintf(http_response, http_response_length, http_header, strlen(website));
        strlcat(http_response, website, http_response_length);

        write(sockfd, http_response, strlen(http_response));
        free(http_response);

conn_close:
        free(website);
        close(sockfd);

        pthread_mutex_lock(&mut);
        active_connections -= 1;
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mut);

        return (void *) (intptr_t) ret;
}

static int event_loop(const int sockfd) {
        pthread_mutex_init(&mut, nullptr);
        pthread_cond_init(&cond, nullptr);

        while (1) {
                struct sockaddr destination = {};
                socklen_t dest_len = sizeof(destination);

                int destfd;
                if ((destfd = accept(sockfd, &destination, &dest_len)) < 0) {
                        dprintf(2, "Error while trying to accept connection\n");
                        continue;
                }


                pthread_mutex_lock(&mut);
                while (active_connections >= MAX_ACTIVE_CONNECTIONS) {
                        pthread_cond_wait(&cond, &mut);
                }

                pthread_t thread;
                pthread_create(&thread, nullptr, manage_connection, (void *) (intptr_t) destfd);
                pthread_detach(thread);

                counter += 1;
                active_connections += 1;

                pthread_mutex_unlock(&mut);
        }

        return 0;
}


int run_http_server(const uint16_t port) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                dprintf(2, "Error while trying to open socket\n");
                return EXIT_FAILURE;
        }

        int opt = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
                dprintf(2, "Error while trying to set socket option\n");
                return EXIT_FAILURE;
        }

        struct sockaddr_in source = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(port)};
        if (bind(sockfd, (struct sockaddr *) &source, sizeof(source)) < 0) {
                dprintf(2, "Error while trying to bind\n");
                return EXIT_FAILURE;
        }


        if (listen(sockfd, SOMAXCONN) < 0) {
                dprintf(2, "Error while trying to listen on port: %i\n", source.sin_port);
                return EXIT_FAILURE;
        }

        event_loop(sockfd);

        close(sockfd);
        return 0;
}
