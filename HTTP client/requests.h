// Copyright Craciunoiu Cezar 324CA
#ifndef _REQUESTS_
#define _REQUESTS_

#define AUTH_TYPE "Bearer" // Tip de autorizare (doar unul necesar in tema)

// Construieste un request HTTP generalizat: GET/POST
char *compute_any_request(const char *host, const char *request, const char *url,
                          const char *url_params, const char *form_data, 
                          const char *type, const char **cookies, 
                          const int nrCookies, const char *authorizeToken);

// Construieste un mesaj HTTP de tip GET
char *compute_get_request(const char *host, const char *url, 
                          const char *url_params, const char **cookie, 
                          const int nrCookies, const char *authorizeToken);

// Construieste un mesaj de tip POST
char *compute_post_request(const char *host, const char *url, 
                           const char *form_data, const char *type, 
                           const char **cookies, const int nrCookies, 
                           const char *authorizeToken);

#endif
