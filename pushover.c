#include <uwsgi.h>
#include <curl/curl.h>

extern struct uwsgi_server uwsgi;

struct pushover_config {
	char *token;
	char *user;
	char *message;
	char *device;
	char *title;
	char *url;
	char *url_title;
	char *priority;
	char *timestamp;
	char *sound;

	char *pushover_url;
	char *ssl_no_verify;

	char *timeout;
};

static void pushover_free(struct pushover_config *poc) {
	if (poc->token) free(poc->token);
	if (poc->user) free(poc->user);
	if (poc->message) free(poc->message);
	if (poc->device) free(poc->device);
	if (poc->title) free(poc->title);
	if (poc->url) free(poc->url);
	if (poc->url_title) free(poc->url_title);
	if (poc->priority) free(poc->priority);
	if (poc->timestamp) free(poc->timestamp);
	if (poc->sound) free(poc->sound);
	if (poc->pushover_url) free(poc->pushover_url);
	if (poc->ssl_no_verify) free(poc->ssl_no_verify);
	if (poc->timeout) free(poc->timeout);
}

#define pkv(x) #x, &poc->x

static int pushover_config_do(char *arg, struct pushover_config *poc) {
	memset(poc, 0, sizeof(struct pushover_config));

	if (uwsgi_kvlist_parse(arg, strlen(arg), ',', '=',
		pkv(token),
		pkv(user),
		pkv(message),
		pkv(device),
		pkv(title),
		pkv(url),
		pkv(url_title),
		pkv(priority),
		pkv(timestamp),
		pkv(sound),
		pkv(pushover_url),
		pkv(ssl_no_verify),
		pkv(timeout),
                        NULL)) {
		uwsgi_log("unable to parse pushover options\n");
		return -1;
	}

	if (!poc->token) {
		uwsgi_log("you need to specify a pushover token\n");
		return -1;
	}

	if (!poc->user) {
		uwsgi_log("you need to specify a pushover user key\n");
		return -1;
	}

	return 0;
}

#define pushover_form(item) if (poc->item) curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, #item, CURLFORM_COPYCONTENTS, poc->item, CURLFORM_END)
#define PUSHOVER_URL "https://api.pushover.net/1/messages.json"

static int pushover_request(struct pushover_config *poc, char *msg) {
	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr=NULL;

	pushover_form(token);
	pushover_form(user);
	if (msg) {
		curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "message", CURLFORM_COPYCONTENTS, msg, CURLFORM_END);
	}
	else {
		pushover_form(message);
	}
	pushover_form(device);
	pushover_form(title);
	pushover_form(url);
	pushover_form(url_title);
	pushover_form(priority);
	pushover_form(timestamp);
	pushover_form(sound);

	char *url = PUSHOVER_URL;	
	if (poc->pushover_url) {
		url = poc->pushover_url;
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		curl_formfree(formpost);
		return -1;
	}

	int timeout = uwsgi.socket_timeout;
	if (poc->timeout) {
		timeout = atoi(poc->timeout);
	}
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	if (poc->ssl_no_verify) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	}

	curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		uwsgi_log("[uwsgi-pushover] curl error: %s\n", curl_easy_strerror(res));
		curl_formfree(formpost);
		curl_easy_cleanup(curl);
		return -1;
	}

	curl_formfree(formpost);
        curl_easy_cleanup(curl);
	
	return 0;
}

static int pushover_hook(char *arg) {
	int ret = -1;
	struct pushover_config poc;
	if (pushover_config_do(arg, &poc)) {
		goto clear;
	}

	if (!poc.message) {
		uwsgi_log("you need to specify a pushover message for hooks\n");
		goto clear;
	}

	ret = pushover_request(&poc, NULL);
clear:
	pushover_free(&poc);
	return ret;
}

static void pushover_alarm_func(struct uwsgi_alarm_instance *uai, char *msg, size_t len) {
	struct pushover_config *poc = (struct pushover_config *) uai->data_ptr;
	char *tmp = uwsgi_concat2n(msg, len, "", 0);	
	pushover_request(poc, tmp);
	free(tmp);
}

static void pushover_alarm_init(struct uwsgi_alarm_instance *uai) {
	struct pushover_config *poc = uwsgi_calloc(sizeof(struct pushover_config));
	if (pushover_config_do(uai->arg, poc)) {
		exit(1);
        }
	uai->data_ptr = poc;
}

static void pushover_register() {
	uwsgi_register_hook("pushover", pushover_hook);
	uwsgi_register_alarm("pushover", pushover_alarm_init, pushover_alarm_func);
}

struct uwsgi_plugin pushover_plugin = {
	.name = "pushover",
	.on_load = pushover_register,
};
