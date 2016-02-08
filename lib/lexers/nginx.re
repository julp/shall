#include <stdlib.h>
#include <string.h>

#include "tokens.h"
#include "lexer.h"
#include "utils.h"

/**
 * Spec/language reference:
 * - none
 *
 * Nginx implementation:
 * - https://raw.githubusercontent.com/nginx/nginx/master/src/core/ngx_conf_file.c
 */

enum {
    STATE(INITIAL),
    STATE(IN_DIRECTIVE),
    STATE(IN_SINGLE_STRING),
    STATE(IN_DOUBLE_STRING),
};

static const named_element_t directives[] = {
    NE("accept_mutex"),
    NE("accept_mutex_delay"),
    NE("access_log"),
    NE("add_after_body"),
    NE("add_before_body"),
    NE("add_header"),
    NE("addition_types"),
    NE("aio"),
    NE("alias"),
    NE("allow"),
    NE("ancient_browser"),
    NE("ancient_browser_value"),
    NE("auth_basic"),
    NE("auth_basic_user_file"),
    NE("auth_http"),
    NE("auth_http_header"),
    NE("auth_http_pass_client_cert"),
    NE("auth_http_timeout"),
    NE("auth_request"),
    NE("auth_request_set"),
    NE("autoindex"),
    NE("autoindex_exact_size"),
    NE("autoindex_format"),
    NE("autoindex_localtime"),
    NE("break"),
    NE("charset"),
    NE("charset_map"),
    NE("charset_types"),
    NE("chunked_transfer_encoding"),
    NE("client_body_buffer_size"),
    NE("client_body_in_file_only"),
    NE("client_body_in_single_buffer"),
    NE("client_body_temp_path"),
    NE("client_body_timeout"),
    NE("client_header_buffer_size"),
    NE("client_header_timeout"),
    NE("client_max_body_size"),
    NE("connection_pool_size"),
    NE("create_full_put_path"),
    NE("daemon"),
    NE("dav_access"),
    NE("dav_methods"),
    NE("debug_connection"),
    NE("debug_points"),
    NE("default_type"),
    NE("deny"),
    NE("directio"),
    NE("directio_alignment"),
    NE("disable_symlinks"),
    NE("empty_gif"),
    NE("env"),
    NE("error_log"),
    NE("error_page"),
    NE("etag"),
    NE("events"),
    NE("expires"),
    NE("f4f"),
    NE("f4f_buffer_size"),
    NE("fastcgi_bind"),
    NE("fastcgi_buffering"),
    NE("fastcgi_buffers"),
    NE("fastcgi_buffer_size"),
    NE("fastcgi_busy_buffers_size"),
    NE("fastcgi_cache"),
    NE("fastcgi_cache_bypass"),
    NE("fastcgi_cache_key"),
    NE("fastcgi_cache_lock"),
    NE("fastcgi_cache_lock_age"),
    NE("fastcgi_cache_lock_timeout"),
    NE("fastcgi_cache_methods"),
    NE("fastcgi_cache_min_uses"),
    NE("fastcgi_cache_path"),
    NE("fastcgi_cache_purge"),
    NE("fastcgi_cache_revalidate"),
    NE("fastcgi_cache_use_stale"),
    NE("fastcgi_cache_valid"),
    NE("fastcgi_catch_stderr"),
    NE("fastcgi_connect_timeout"),
    NE("fastcgi_force_ranges"),
    NE("fastcgi_hide_header"),
    NE("fastcgi_ignore_client_abort"),
    NE("fastcgi_ignore_headers"),
    NE("fastcgi_index"),
    NE("fastcgi_intercept_errors"),
    NE("fastcgi_keep_conn"),
    NE("fastcgi_limit_rate"),
    NE("fastcgi_max_temp_file_size"),
    NE("fastcgi_next_upstream"),
    NE("fastcgi_next_upstream_timeout"),
    NE("fastcgi_next_upstream_tries"),
    NE("fastcgi_no_cache"),
    NE("fastcgi_param"),
    NE("fastcgi_pass"),
    NE("fastcgi_pass_header"),
    NE("fastcgi_pass_request_body"),
    NE("fastcgi_pass_request_headers"),
    NE("fastcgi_read_timeout"),
    NE("fastcgi_request_buffering"),
    NE("fastcgi_send_lowat"),
    NE("fastcgi_send_timeout"),
    NE("fastcgi_split_path_info"),
    NE("fastcgi_store"),
    NE("fastcgi_store_access"),
    NE("fastcgi_temp_file_write_size"),
    NE("fastcgi_temp_path"),
    NE("flv"),
    NE("geo"),
    NE("geoip_city"),
    NE("geoip_country"),
    NE("geoip_org"),
    NE("geoip_proxy"),
    NE("geoip_proxy_recursive"),
    NE("gunzip"),
    NE("gunzip_buffers"),
    NE("gzip"),
    NE("gzip_buffers"),
    NE("gzip_comp_level"),
    NE("gzip_disable"),
    NE("gzip_http_version"),
    NE("gzip_min_length"),
    NE("gzip_proxied"),
    NE("gzip_static"),
    NE("gzip_types"),
    NE("gzip_vary"),
    NE("hash"),
    NE("health_check"),
    NE("health_check_timeout"),
    NE("hls"),
    NE("hls_buffers"),
    NE("hls_forward_args"),
    NE("hls_fragment"),
    NE("hls_mp4_buffer_size"),
    NE("hls_mp4_max_buffer_size"),
    NE("http"),
    NE("http2_chunk_size"),
    NE("http2_idle_timeout"),
    NE("http2_max_concurrent_streams"),
    NE("http2_max_field_size"),
    NE("http2_max_header_size"),
    NE("http2_recv_buffer_size"),
    NE("http2_recv_timeout"),
    NE("if"),
    NE("if_modified_since"),
    NE("ignore_invalid_headers"),
    NE("image_filter"),
    NE("image_filter_buffer"),
    NE("image_filter_interlace"),
    NE("image_filter_jpeg_quality"),
    NE("image_filter_sharpen"),
    NE("image_filter_transparency"),
    NE("imap_auth"),
    NE("imap_capabilities"),
    NE("imap_client_buffer"),
    NE("include"),
    NE("index"),
    NE("internal"),
    NE("ip_hash"),
    NE("keepalive"),
    NE("keepalive_disable"),
    NE("keepalive_requests"),
    NE("keepalive_timeout"),
    NE("large_client_header_buffers"),
    NE("least_conn"),
    NE("least_time"),
    NE("limit_conn"),
    NE("limit_conn_log_level"),
    NE("limit_conn_status"),
    NE("limit_conn_zone"),
    NE("limit_except"),
    NE("limit_rate"),
    NE("limit_rate_after"),
    NE("limit_req"),
    NE("limit_req_log_level"),
    NE("limit_req_status"),
    NE("limit_req_zone"),
    NE("limit_zone"),
    NE("lingering_close"),
    NE("lingering_time"),
    NE("lingering_timeout"),
    NE("listen"),
    NE("location"),
    NE("lock_file"),
    NE("log_format"),
    NE("log_not_found"),
    NE("log_subrequest"),
    NE("mail"),
    NE("map"),
    NE("map_hash_bucket_size"),
    NE("map_hash_max_size"),
    NE("master_process"),
    NE("match"),
    NE("max_ranges"),
    NE("memcached_bind"),
    NE("memcached_buffer_size"),
    NE("memcached_connect_timeout"),
    NE("memcached_force_ranges"),
    NE("memcached_gzip_flag"),
    NE("memcached_next_upstream"),
    NE("memcached_next_upstream_timeout"),
    NE("memcached_next_upstream_tries"),
    NE("memcached_pass"),
    NE("memcached_read_timeout"),
    NE("memcached_send_timeout"),
    NE("merge_slashes"),
    NE("min_delete_depth"),
    NE("modern_browser"),
    NE("modern_browser_value"),
    NE("mp4"),
    NE("mp4_buffer_size"),
    NE("mp4_limit_rate"),
    NE("mp4_limit_rate_after"),
    NE("mp4_max_buffer_size"),
    NE("msie_padding"),
    NE("msie_refresh"),
    NE("multi_accept"),
    NE("ntlm"),
    NE("open_file_cache"),
    NE("open_file_cache_errors"),
    NE("open_file_cache_min_uses"),
    NE("open_file_cache_valid"),
    NE("open_log_file_cache"),
    NE("output_buffers"),
    NE("override_charset"),
    NE("pcre_jit"),
    NE("perl"),
    NE("perl_modules"),
    NE("perl_require"),
    NE("perl_set"),
    NE("pid"),
    NE("pop3_auth"),
    NE("pop3_capabilities"),
    NE("port_in_redirect"),
    NE("postpone_output"),
    NE("protocol"),
    NE("proxy_bind"),
    NE("proxy_buffer"),
    NE("proxy_buffering"),
    NE("proxy_buffers"),
    NE("proxy_buffer_size"),
    NE("proxy_busy_buffers_size"),
    NE("proxy_cache"),
    NE("proxy_cache_bypass"),
    NE("proxy_cache_convert_head"),
    NE("proxy_cache_key"),
    NE("proxy_cache_lock"),
    NE("proxy_cache_lock_age"),
    NE("proxy_cache_lock_timeout"),
    NE("proxy_cache_methods"),
    NE("proxy_cache_min_uses"),
    NE("proxy_cache_path"),
    NE("proxy_cache_purge"),
    NE("proxy_cache_revalidate"),
    NE("proxy_cache_use_stale"),
    NE("proxy_cache_valid"),
    NE("proxy_connect_timeout"),
    NE("proxy_cookie_domain"),
    NE("proxy_cookie_path"),
    NE("proxy_download_rate"),
    NE("proxy_force_ranges"),
    NE("proxy_headers_hash_bucket_size"),
    NE("proxy_headers_hash_max_size"),
    NE("proxy_hide_header"),
    NE("proxy_http_version"),
    NE("proxy_ignore_client_abort"),
    NE("proxy_ignore_headers"),
    NE("proxy_intercept_errors"),
    NE("proxy_limit_rate"),
    NE("proxy_max_temp_file_size"),
    NE("proxy_method"),
    NE("proxy_next_upstream"),
    NE("proxy_next_upstream_timeout"),
    NE("proxy_next_upstream_tries"),
    NE("proxy_no_cache"),
    NE("proxy_pass"),
    NE("proxy_pass_error_message"),
    NE("proxy_pass_header"),
    NE("proxy_pass_request_body"),
    NE("proxy_pass_request_headers"),
    NE("proxy_protocol"),
    NE("proxy_read_timeout"),
    NE("proxy_redirect"),
    NE("proxy_request_buffering"),
    NE("proxy_send_lowat"),
    NE("proxy_send_timeout"),
    NE("proxy_set_body"),
    NE("proxy_set_header"),
    NE("proxy_ssl"),
    NE("proxy_ssl_certificate"),
    NE("proxy_ssl_certificate_key"),
    NE("proxy_ssl_ciphers"),
    NE("proxy_ssl_crl"),
    NE("proxy_ssl_name"),
    NE("proxy_ssl_password_file"),
    NE("proxy_ssl_protocols"),
    NE("proxy_ssl_server_name"),
    NE("proxy_ssl_session_reuse"),
    NE("proxy_ssl_trusted_certificate"),
    NE("proxy_ssl_verify"),
    NE("proxy_ssl_verify_depth"),
    NE("proxy_store"),
    NE("proxy_store_access"),
    NE("proxy_temp_file_write_size"),
    NE("proxy_temp_path"),
    NE("proxy_timeout"),
    NE("proxy_upload_rate"),
    NE("queue"),
    NE("random_index"),
    NE("read_ahead"),
    NE("real_ip_header"),
    NE("real_ip_recursive"),
    NE("recursive_error_pages"),
    NE("referer_hash_bucket_size"),
    NE("referer_hash_max_size"),
    NE("request_pool_size"),
    NE("reset_timedout_connection"),
    NE("resolver"),
    NE("resolver_timeout"),
    NE("return"),
    NE("rewrite"),
    NE("rewrite_log"),
    NE("root"),
    NE("satisfy"),
    NE("scgi_bind"),
    NE("scgi_buffering"),
    NE("scgi_buffers"),
    NE("scgi_buffer_size"),
    NE("scgi_busy_buffers_size"),
    NE("scgi_cache"),
    NE("scgi_cache_bypass"),
    NE("scgi_cache_key"),
    NE("scgi_cache_lock"),
    NE("scgi_cache_lock_age"),
    NE("scgi_cache_lock_timeout"),
    NE("scgi_cache_methods"),
    NE("scgi_cache_min_uses"),
    NE("scgi_cache_path"),
    NE("scgi_cache_purge"),
    NE("scgi_cache_revalidate"),
    NE("scgi_cache_use_stale"),
    NE("scgi_cache_valid"),
    NE("scgi_connect_timeout"),
    NE("scgi_force_ranges"),
    NE("scgi_hide_header"),
    NE("scgi_ignore_client_abort"),
    NE("scgi_ignore_headers"),
    NE("scgi_intercept_errors"),
    NE("scgi_limit_rate"),
    NE("scgi_max_temp_file_size"),
    NE("scgi_next_upstream"),
    NE("scgi_next_upstream_timeout"),
    NE("scgi_next_upstream_tries"),
    NE("scgi_no_cache"),
    NE("scgi_param"),
    NE("scgi_pass"),
    NE("scgi_pass_header"),
    NE("scgi_pass_request_body"),
    NE("scgi_pass_request_headers"),
    NE("scgi_read_timeout"),
    NE("scgi_request_buffering"),
    NE("scgi_send_timeout"),
    NE("scgi_store"),
    NE("scgi_store_access"),
    NE("scgi_temp_file_write_size"),
    NE("scgi_temp_path"),
    NE("secure_link"),
    NE("secure_link_md5"),
    NE("secure_link_secret"),
    NE("sendfile"),
    NE("sendfile_max_chunk"),
    NE("send_lowat"),
    NE("send_timeout"),
    NE("server"),
    NE("server_name"),
    NE("server_name_in_redirect"),
    NE("server_names_hash_bucket_size"),
    NE("server_names_hash_max_size"),
    NE("server_tokens"),
    NE("session_log"),
    NE("session_log_format"),
    NE("session_log_zone"),
    NE("set"),
    NE("set_real_ip_from"),
    NE("slice"),
    NE("smtp_auth"),
    NE("smtp_capabilities"),
    NE("source_charset"),
    NE("spdy_chunk_size"),
    NE("spdy_headers_comp"),
    NE("split_clients"),
    NE("ssi"),
    NE("ssi_last_modified"),
    NE("ssi_min_file_chunk"),
    NE("ssi_silent_errors"),
    NE("ssi_types"),
    NE("ssi_value_length"),
    NE("ssl"),
    NE("ssl_buffer_size"),
    NE("ssl_certificate"),
    NE("ssl_certificate_key"),
    NE("ssl_ciphers"),
    NE("ssl_client_certificate"),
    NE("ssl_crl"),
    NE("ssl_dhparam"),
    NE("ssl_ecdh_curve"),
    NE("ssl_engine"),
    NE("ssl_handshake_timeout"),
    NE("ssl_password_file"),
    NE("ssl_prefer_server_ciphers"),
    NE("ssl_protocols"),
    NE("ssl_session_cache"),
    NE("ssl_session_ticket_key"),
    NE("ssl_session_tickets"),
    NE("ssl_session_timeout"),
    NE("ssl_stapling"),
    NE("ssl_stapling_file"),
    NE("ssl_stapling_responder"),
    NE("ssl_stapling_verify"),
    NE("ssl_trusted_certificate"),
    NE("ssl_verify_client"),
    NE("ssl_verify_depth"),
    NE("starttls"),
    NE("state"),
    NE("status"),
    NE("status_format"),
    NE("status_zone"),
    NE("sticky"),
    NE("sticky_cookie_insert"),
    NE("stream"),
    NE("stub_status"),
    NE("sub_filter"),
    NE("sub_filter_last_modified"),
    NE("sub_filter_once"),
    NE("sub_filter_types"),
    NE("tcp_nodelay"),
    NE("tcp_nopush"),
    NE("thread_pool"),
    NE("timeout"),
    NE("timer_resolution"),
    NE("try_files"),
    NE("types"),
    NE("types_hash_bucket_size"),
    NE("types_hash_max_size"),
    NE("underscores_in_headers"),
    NE("uninitialized_variable_warn"),
    NE("upstream"),
    NE("upstream_conf"),
    NE("use"),
    NE("user"),
    NE("userid"),
    NE("userid_domain"),
    NE("userid_expires"),
    NE("userid_mark"),
    NE("userid_name"),
    NE("userid_p3p"),
    NE("userid_path"),
    NE("userid_service"),
    NE("uwsgi_bind"),
    NE("uwsgi_buffering"),
    NE("uwsgi_buffers"),
    NE("uwsgi_buffer_size"),
    NE("uwsgi_busy_buffers_size"),
    NE("uwsgi_cache"),
    NE("uwsgi_cache_bypass"),
    NE("uwsgi_cache_key"),
    NE("uwsgi_cache_lock"),
    NE("uwsgi_cache_lock_age"),
    NE("uwsgi_cache_lock_timeout"),
    NE("uwsgi_cache_methods"),
    NE("uwsgi_cache_min_uses"),
    NE("uwsgi_cache_path"),
    NE("uwsgi_cache_purge"),
    NE("uwsgi_cache_revalidate"),
    NE("uwsgi_cache_use_stale"),
    NE("uwsgi_cache_valid"),
    NE("uwsgi_connect_timeout"),
    NE("uwsgi_force_ranges"),
    NE("uwsgi_hide_header"),
    NE("uwsgi_ignore_client_abort"),
    NE("uwsgi_ignore_headers"),
    NE("uwsgi_intercept_errors"),
    NE("uwsgi_limit_rate"),
    NE("uwsgi_max_temp_file_size"),
    NE("uwsgi_modifier1"),
    NE("uwsgi_modifier2"),
    NE("uwsgi_next_upstream"),
    NE("uwsgi_next_upstream_timeout"),
    NE("uwsgi_next_upstream_tries"),
    NE("uwsgi_no_cache"),
    NE("uwsgi_param"),
    NE("uwsgi_pass"),
    NE("uwsgi_pass_header"),
    NE("uwsgi_pass_request_body"),
    NE("uwsgi_pass_request_headers"),
    NE("uwsgi_read_timeout"),
    NE("uwsgi_request_buffering"),
    NE("uwsgi_send_timeout"),
    NE("uwsgi_ssl_certificate"),
    NE("uwsgi_ssl_certificate_key"),
    NE("uwsgi_ssl_ciphers"),
    NE("uwsgi_ssl_crl"),
    NE("uwsgi_ssl_name"),
    NE("uwsgi_ssl_password_file"),
    NE("uwsgi_ssl_protocols"),
    NE("uwsgi_ssl_server_name"),
    NE("uwsgi_ssl_session_reuse"),
    NE("uwsgi_ssl_trusted_certificate"),
    NE("uwsgi_ssl_verify"),
    NE("uwsgi_ssl_verify_depth"),
    NE("uwsgi_store"),
    NE("uwsgi_store_access"),
    NE("uwsgi_temp_file_write_size"),
    NE("uwsgi_temp_path"),
    NE("valid_referers"),
    NE("variables_hash_bucket_size"),
    NE("variables_hash_max_size"),
    NE("worker_aio_requests"),
    NE("worker_connections"),
    NE("worker_cpu_affinity"),
    NE("worker_priority"),
    NE("worker_processes"),
    NE("worker_rlimit_core"),
    NE("worker_rlimit_nofile"),
    NE("working_directory"),
    NE("xclient"),
    NE("xml_entities"),
    NE("xslt_last_modified"),
    NE("xslt_param"),
    NE("xslt_string_param"),
    NE("xslt_stylesheet"),
    NE("xslt_types"),
    NE("zone"),
};

typedef struct {
    const char *name;
    size_t name_len;
    bool starts_with;
} nginx_named_element_t;

#define VAR(s) \
    { s, STR_LEN(s), false },

#define PREF(s) \
    { s, STR_LEN(s), true },

static const nginx_named_element_t variables[] = {
    VAR("$ancient_browser")
    PREF("$arg_")
    VAR("$args")
    VAR("$binary_remote_addr")
    VAR("$body_bytes_sent")
    VAR("$bytes_sent")
    VAR("$connection")
    VAR("$connection_requests")
    VAR("$connections_active")
    VAR("$connections_reading")
    VAR("$connections_waiting")
    VAR("$connections_writing")
    VAR("$content_length")
    VAR("$content_type")
    PREF("$cookie_")
    VAR("$date_gmt")
    VAR("$date_local")
    VAR("$document_root")
    VAR("$document_uri")
    VAR("$fastcgi_path_info")
    VAR("$fastcgi_script_name")
    VAR("$geoip_area_code")
    VAR("$geoip_city")
    VAR("$geoip_city_continent_code")
    VAR("$geoip_city_country_code")
    VAR("$geoip_city_country_code3")
    VAR("$geoip_city_country_name")
    VAR("$geoip_country_code")
    VAR("$geoip_country_code3")
    VAR("$geoip_country_name")
    VAR("$geoip_dma_code")
    VAR("$geoip_latitude")
    VAR("$geoip_longitude")
    VAR("$geoip_org")
    VAR("$geoip_postal_code")
    VAR("$geoip_region")
    VAR("$geoip_region_name")
    VAR("$gzip_ratio")
    VAR("$host")
    VAR("$hostname")
    PREF("$http_")
    VAR("$http2")
    VAR("$https")
    VAR("$invalid_referer")
    VAR("$is_args")
    VAR("$limit_rate")
    VAR("$memcached_key")
    VAR("$modern_browser")
    VAR("$msec")
    VAR("$msie")
    VAR("$nginx_version")
    VAR("$pid")
    VAR("$pipe")
    VAR("$proxy_add_x_forwarded_for")
    VAR("$proxy_host")
    VAR("$proxy_port")
    VAR("$proxy_protocol_addr")
    VAR("$query_string")
    VAR("$realip_remote_addr")
    VAR("$realpath_root")
    VAR("$remote_addr")
    VAR("$remote_port")
    VAR("$remote_user")
    VAR("$request")
    VAR("$request_body")
    VAR("$request_body_file")
    VAR("$request_completion")
    VAR("$request_filename")
    VAR("$request_length")
    VAR("$request_method")
    VAR("$request_time")
    VAR("$request_uri")
    VAR("$scheme")
    VAR("$secure_link")
    VAR("$secure_link_expires")
    PREF("$sent_http_")
    VAR("$server_addr")
    VAR("$server_name")
    VAR("$server_port")
    VAR("$server_protocol")
    VAR("$session_log_binary_id")
    VAR("$session_log_id")
    VAR("$slice_range")
    VAR("$spdy")
    VAR("$spdy_request_priority")
    VAR("$ssl_cipher")
    VAR("$ssl_client_cert")
    VAR("$ssl_client_fingerprint")
    VAR("$ssl_client_i_dn")
    VAR("$ssl_client_raw_cert")
    VAR("$ssl_client_s_dn")
    VAR("$ssl_client_serial")
    VAR("$ssl_client_verify")
    VAR("$ssl_protocol")
    VAR("$ssl_server_name")
    VAR("$ssl_session_id")
    VAR("$ssl_session_reused")
    VAR("$status")
    VAR("$tcpinfo_rcv_space")
    VAR("$tcpinfo_rtt")
    VAR("$tcpinfo_rttvar")
    VAR("$tcpinfo_snd_cwnd")
    VAR("$time_iso8601")
    VAR("$time_local")
    VAR("$uid_got")
    VAR("$uid_reset")
    VAR("$uid_set")
    VAR("$upstream_addr")
    VAR("$upstream_cache_status")
    VAR("$upstream_connect_time")
    PREF("$upstream_cookie_")
    VAR("$upstream_header_time")
    PREF("$upstream_http_")
    VAR("$upstream_response_length")
    VAR("$upstream_response_time")
    VAR("$upstream_status")
    VAR("$uri")
};

static int nginx_named_elements_cmp(const void *a, const void *b)
{
    const nginx_named_element_t *na, *nb;

    na = (const nginx_named_element_t *) a; /* key */
    nb = (const nginx_named_element_t *) b;
    if (nb->starts_with) {
        return strncmp_l(na->name, na->name_len, nb->name, nb->name_len, nb->name_len);
    } else {
        return strcmp_l(na->name, na->name_len, nb->name, nb->name_len);
    }
}

/**
 * TODO:
 * - gérer les LITERAL_DURATION et LITERAL_SIZE (cf http://nginx.org/en/docs/syntax.html)
 * - une variable ne sera pas interpolée dans une expression régulière
 * - pour l'interpolation, lever toute ambiguité notamment, le nom d'une variable peut être entourée d'accolades
 **/
static int nginxlex(YYLEX_ARGS)
{
    (void) ctxt;
    (void) data;
    (void) options;
    while (YYCURSOR < YYLIMIT) {
        YYTEXT = YYCURSOR;
/*!re2c
re2c:yyfill:check = 0;
re2c:yyfill:enable = 0;

LNUM = [0-9]+;
SPACE = [ \n\r\t];

<INITIAL> SPACE+ {
    TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> "\\"[\n] {
    TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> [\n] {
    BEGIN(INITIAL);
    TOKEN(IGNORABLE);
}

<IN_DIRECTIVE> [ \n\r\t]+ {
    TOKEN(IGNORABLE);
}

<INITIAL> [#] {
    while (YYCURSOR < YYLIMIT) {
        switch (*YYCURSOR++) {
            case '\r':
                if ('\n' == *YYCURSOR) {
                    YYCURSOR++;
                }
                /* fall through */
            case '\n':
                //YYLINENO++;
                break;
            default:
                continue;
        }
        break;
    }
    TOKEN(COMMENT_SINGLE);
}

<INITIAL> [;{}] {
    TOKEN(PUNCTUATION);
}

<IN_DIRECTIVE> [;{] {
    BEGIN(INITIAL);
    TOKEN(PUNCTUATION);
}

<INITIAL> [^$#;{} \r\n\t][^;{} \n\r\t]* {
    int type;
    named_element_t *match, key = { (char *) YYTEXT, YYLENG };

    type = TEXT;
    BEGIN(IN_DIRECTIVE);
    if (NULL != (match = bsearch(&key, directives, ARRAY_SIZE(directives), sizeof(directives[0]), named_elements_cmp))) {
        type = KEYWORD_BUILTIN;
    }
    TOKEN(type);
}

<IN_DIRECTIVE> "$"[^;{}() \n\r\t]+ {
    int type;
    nginx_named_element_t *match, key = { (char *) YYTEXT, YYLENG, false };

    type = NAME_VARIABLE;
    if (NULL != (match = bsearch(&key, variables, ARRAY_SIZE(variables), sizeof(variables[0]), nginx_named_elements_cmp))) {
        type = NAME_BUILTIN;
    }
    TOKEN(type);
}

<IN_DIRECTIVE> [^$#;{}() \n\r\t]+ {
    TOKEN(STRING_SINGLE);
}

<IN_DIRECTIVE> LNUM {
    TOKEN(NUMBER_INTEGER);
}

<INITIAL,IN_DIRECTIVE> '"' {
    BEGIN(IN_DOUBLE_STRING);
    TOKEN(STRING_SINGLE);
}

<INITIAL,IN_DIRECTIVE> '\'' {
    BEGIN(IN_SINGLE_STRING);
    TOKEN(STRING_SINGLE);
}

<IN_DOUBLE_STRING,IN_SINGLE_STRING> "\\" [trn] {
    TOKEN(ESCAPED_CHAR);
}

<IN_DOUBLE_STRING> "\\" [\\"] {
    TOKEN(ESCAPED_CHAR);
}

<IN_SINGLE_STRING> "\\" [\\'] {
    TOKEN(ESCAPED_CHAR);
}

<*> [^] {
    TOKEN(IGNORABLE);
}
*/
    }
    DONE();
}

LexerImplementation nginx_lexer = {
    "Nginx",
    "Lexer for Nginx configuration files",
    (const char * const []) { "nginxconf", NULL },
    (const char * const []) { "nginx.conf", NULL },
    (const char * const []) { "text/x-nginx-conf", NULL },
    NULL, // interpreters
    NULL, // analyse
    NULL, // init
    nginxlex,
    NULL, // finalyze
    sizeof(LexerData),
    NULL, // options
    NULL // dependencies
};
