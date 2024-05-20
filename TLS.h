#pragma once

#include <string> //std::string_view

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace COM
{
    class TLS
    {
    private:
        /* SSL factory context to create SSL objects*/
        SSL_CTX *ssl_ctx = nullptr;

        /* object representing TLS connection */
        SSL *ssl = nullptr;

        /* Free the resources that have been allocated */
        void keep_clear();
    public:
        TLS(/* args */) = default;
        ~TLS() = default;

        /* create SSL factory that can produce SSL objects */
        void create_context();

        /* configure SSL factory with relevant parameters */
        void configure_context(int minVersion = TLS1_2_VERSION, const std::string_view &certPath = std::string_view{});

        /* Create an SSL object to represent the TLS connection */
        void create_object();

        /* start tls handshake*/
        bool connect(int socket_fd);

        /* helper function to recognize read/write failure*/
        int handle_io_failure(int res);

        /* get ssl "file descriptor"*/
        SSL* operator()() { return ssl;}
    };
}


