#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include "mcached.h"
#include "uthash.h"

#define MAX_CLIENTS 100
#define QUEUE_SIZE 100
#define VERSION "C-Memcached 1.0"

typedef struct {
    char *key;
    uint8_t *value;
    size_t value_len;
    UT_hash_handle hh;
} kv_pair;

kv_pair *store = NULL;

pthread_mutex_t store_mutex = PTHREAD_MUTEX_INITIALIZER;

uint8_t *get(const char *key, size_t *len) {
    pthread_mutex_lock(&store_mutex);
    kv_pair *entry;
    HASH_FIND_STR(store, key, entry);
    uint8_t *result = NULL;
    if (entry) {
        result = malloc(entry->value_len);
        if (result) {
            memcpy(result, entry->value, entry->value_len);
            *len = entry->value_len;
        }
    }
    pthread_mutex_unlock(&store_mutex);
    return result;
}

void set(const char *key, const uint8_t *val, size_t val_len) {
    pthread_mutex_lock(&store_mutex);
    kv_pair *entry;
    HASH_FIND_STR(store, key, entry);
    if (entry) {
        free(entry->value);
        entry->value = malloc(val_len);
        if (entry->value) {
            memcpy(entry->value, val, val_len);
            entry->value_len = val_len;
        }
    } else {
        entry = malloc(sizeof(kv_pair));
        if (entry) {
            entry->key = strdup(key);
            entry->value = malloc(val_len);
            if (entry->value) {
                memcpy(entry->value, val, val_len);
                entry->value_len = val_len;
                HASH_ADD_KEYPTR(hh, store, entry->key, strlen(entry->key), entry);
            } else {
                free(entry->key);
                free(entry);
            }
        }
    }
    pthread_mutex_unlock(&store_mutex);
}

int add(const char *key, const uint8_t *val, size_t val_len) {
    pthread_mutex_lock(&store_mutex);
    kv_pair *entry;
    HASH_FIND_STR(store, key, entry);
    if (entry) {
        pthread_mutex_unlock(&store_mutex);
        return 0;
    }
    entry = malloc(sizeof(kv_pair));
    if (!entry) {
        pthread_mutex_unlock(&store_mutex);
        return 0;
    }
    entry->key = strdup(key);
    entry->value = malloc(val_len);
    if (!entry->key || !entry->value) {
        free(entry->key);
        free(entry->value);
        free(entry);
        pthread_mutex_unlock(&store_mutex);
        return 0;
    }
    memcpy(entry->value, val, val_len);
    entry->value_len = val_len;
    HASH_ADD_KEYPTR(hh, store, entry->key, strlen(entry->key), entry);
    pthread_mutex_unlock(&store_mutex);
    return 1;
}

int del(const char *key) {
    pthread_mutex_lock(&store_mutex);
    kv_pair *entry;
    HASH_FIND_STR(store, key, entry);
    if (entry) {
        HASH_DEL(store, entry);
        free(entry->key);
        free(entry->value);
        free(entry);
        pthread_mutex_unlock(&store_mutex);
        return 1;
    }
    pthread_mutex_unlock(&store_mutex);
    return 0;
}

void *handle_client(void *arg) {
    int clientfd = *(int*)arg;
    free(arg);  // Free the allocated memory for the socket pointer

    while (1) {
        memcache_req_header_t req;
        ssize_t r = recv(clientfd, &req, sizeof(req), MSG_WAITALL);
        if (r == 0 || r == -1) {
            close(clientfd);
            break;
        }

        if (req.magic != 0x80) {
            close(clientfd);
            break;
        }

        uint16_t keylen = ntohs(req.key_length);
        uint32_t bodylen = ntohl(req.total_body_length);
        uint8_t extlen = req.extras_length;

        char *body = NULL;
        if (bodylen > 0) {
            body = malloc(bodylen);
            if (!body || recv(clientfd, body, bodylen, MSG_WAITALL) != (ssize_t)bodylen) {
                free(body);
                close(clientfd);
                break;
            }
        }

        char *key = NULL;
        uint8_t *val = NULL;
        size_t vallen = 0;
        if (keylen > 0 && body) {
            key = strndup(body + extlen, keylen);
        }
        if (bodylen > keylen + extlen && body) {
            vallen = bodylen - keylen - extlen;
            val = (uint8_t *)(body + extlen + keylen);
        }

        uint16_t status = RES_OK;
        uint8_t *res = NULL;
        size_t reslen = 0;

        switch (req.opcode) {
            case CMD_GET:
                if (!key) status = RES_ERROR;
                else {
                    res = get(key, &reslen);
                    if (!res) status = RES_NOT_FOUND;
                }
                break;
            case CMD_SET:
                if (key && val) set(key, val, vallen);
                else status = RES_ERROR;
                break;
            case CMD_ADD:
                if (!(key && val && add(key, val, vallen))) status = RES_EXISTS;
                break;
            case CMD_DELETE:
                if (!(key && del(key))) status = RES_NOT_FOUND;
                break;
            case CMD_VERSION:
                res = (uint8_t *)strdup(VERSION);
                reslen = strlen(VERSION);
                break;
            case CMD_OUTPUT:
                // CMD_OUTPUT is a no-op command that should return OK
                status = RES_OK;
                break;
            default:
                status = RES_ERROR;
                break;
        }

        memcache_req_header_t resp = {0};
        resp.magic = 0x81;
        resp.opcode = req.opcode;
        resp.key_length = 0;
        resp.extras_length = 0;
        resp.data_type = 0;
        resp.vbucket_id = htons(status);
        resp.total_body_length = htonl(reslen);
        resp.opaque = req.opaque;
        resp.cas = 0;

        if (send(clientfd, &resp, sizeof(resp), 0) < 0) {
            free(res);
            free(key);
            free(body);
            close(clientfd);
            break;
        }
        
        if (res) {
            if (send(clientfd, res, reslen, 0) < 0) {
                free(res);
                free(key);
                free(body);
                close(clientfd);
                break;
            }
        }

        free(res);
        free(key);
        free(body);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <threads>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    int port = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    if (port <= 0 || num_threads <= 0) {
        fprintf(stderr, "Invalid port or number of threads\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
        perror("setsockopt");
        exit(1);
    }

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(sockfd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    pthread_t threads[num_threads];

    while (1) {
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);

        int *client_sock = malloc(sizeof(int));
        *client_sock = accept(sockfd, (struct sockaddr *)&cliaddr, &clilen);
        if (*client_sock < 0) {
            free(client_sock);
            perror("accept");
            continue;
        }

        // Find an available thread slot
        int thread_idx = -1;
        for (int i = 0; i < num_threads; i++) {
            if (threads[i] == 0) {
                thread_idx = i;
                break;
            }
        }

        if (thread_idx == -1) {
            // No available thread slots, close the connection
            close(*client_sock);
            free(client_sock);
            continue;
        }

        pthread_create(&threads[thread_idx], NULL, handle_client, client_sock);
        pthread_detach(threads[thread_idx]);
    }

    close(sockfd);
    return 0;
}
