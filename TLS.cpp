#include "TLS.h"

// #include <cstdio> //std::print
#include <filesystem> //std::filesystem::exists

namespace COM
{
    void TLS::create_context()
    {
        const SSL_METHOD *method = TLS_client_method();

        ssl_ctx = SSL_CTX_new(method);
        if (ssl_ctx == NULL) {
            perror("Unable to create SSL context");
            ERR_print_errors_fp(stderr);
            exit(EXIT_FAILURE);
        }
    }

    void TLS::configure_context(int minVersion, const std::string_view &certPath)
    {
        /* If the verification process fails, 
        the TLS/SSL handshake is immediately terminated */
        SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, nullptr);


        if (certPath == std::string_view{})
        {
            /* missing custom certificate  -> verify default location */

            /* specifies that the default locations from which CA certificates are loaded should be used. 
            There is one default directory, one default file and one default store. 
            The default CA certificates directory is called certs in the default OpenSSL directory, 
            and this is also the default store.*/
            if (!SSL_CTX_set_default_verify_paths(ssl_ctx)) {
                std::fprintf(stderr,"Failed to set the default trusted certificate store\n");
                keep_clear();
            }
        }
        else
        {
            /* custom certificate  defined*/

            if(std::filesystem::exists(certPath))
            {
                /* certificate exist */

                /* specifies the locations for ctx, at which CA certificates for verification purposes are located. 
                The certificates available via CAfile, CApath and CAstore are trusted.*/
                /* in case self-signed certificate, the client must trust it directly*/
                if (!SSL_CTX_load_verify_locations(ssl_ctx, certPath.data(), NULL)) 
                {
                    ERR_print_errors_fp(stderr);
                    keep_clear();
                }
            }
            else
            {
                /* certificate does not exist */
                std::fprintf(stderr,"Given certificate location not valid\n");
                keep_clear();
            }
        }

        /* verify if given protocol version is valid */
        if (minVersion != TLS1_3_VERSION && minVersion != TLS1_2_VERSION)
        {
            /* not allowed here protocol version */
            std::fprintf(stderr,"not allowed here protocol version the lowest possible will be used\n");
            minVersion = TLS1_2_VERSION;
        }

        /*set the minimum supported protocol versions for the ssl_ctx*/
        if (!SSL_CTX_set_min_proto_version(ssl_ctx, minVersion)) {
            std::fprintf(stderr,"Failed to set the minimum TLS protocol version\n");
            keep_clear();
        }


    }

    void TLS::create_object()
    {   
        /* creates a new SSL structure which is needed 
        to hold the data for a TLS/SSL connection */
        ssl = SSL_new(ssl_ctx);
        if (ssl == nullptr)
        {
            std::fprintf(stderr,"Failed to create the SSL object\n");
            keep_clear();
        }
    }

    bool TLS::connect(int socket_fd)
    {
        /* connect the SSL object with a file descriptor */
        if(!SSL_set_fd(ssl, socket_fd))
        {
            ERR_print_errors_fp(stderr);
            keep_clear();
        }
        int res = SSL_connect(ssl);
        if(res <= 0)
        {
            if(handle_io_failure(res) <=0)
            {
                std::printf("SSL connection to server failed\n\n");
                // ERR_print_errors_fp(stderr);
                keep_clear();
                return false;
            }

            /* non-blocking socket should get there */
        }

        std::printf("SSL connection to server successful\n\n");
        return true;
    }

    int TLS::handle_io_failure(int res)
    {
        switch (SSL_get_error(ssl, res)) {
        case SSL_ERROR_WANT_READ:
            /* Temporary failure. Wait until we can read and try again */
            return 1;

        case SSL_ERROR_WANT_WRITE:
            /* Temporary failure. Wait until we can write and try again */
            return 1;

        case SSL_ERROR_ZERO_RETURN:
            /* EOF */
            return 0;

        case SSL_ERROR_SYSCALL:
            return -1;

        case SSL_ERROR_SSL:
            /*
            * If the failure is due to a verification error we can get more
            * information about it from SSL_get_verify_result().
            */
            if (SSL_get_verify_result(ssl) != X509_V_OK)
                printf("Verify error: %s\n",
                    X509_verify_cert_error_string(SSL_get_verify_result(ssl)));
            return -1;

        default:
            return -1;
        }

    }

     void TLS::keep_clear()
    {
        if (ssl != NULL) {

            /* shuts down an active connection represented by an SSL object */
            SSL_shutdown(ssl);

            /* decrements the reference count of ssl, 
            and removes the SSL structure pointed to by ssl and 
            frees up the allocated memory if the reference count has reached 0*/
            SSL_free(ssl);
        }

        /* decrements the reference count of ctx, 
        and removes the SSL_CTX object pointed to by ctx and 
        frees up the allocated memory if the the reference count has reached 0.*/
        SSL_CTX_free(ssl_ctx);

        ssl_ctx = nullptr; 
        ssl = nullptr;
    }
}


