uwsgi-pushover
==============

uWSGI plugin for sending pushover notifications (https://pushover.net/)

This plugins registers the "pushover" alarm and "pushover" hook.

You can use them to send push notifications to your mobile devices.

You need to register an account on pushover.net and buy their mobile app (really cheap).

Once registered you need to create an 'application' on their dashboard.

Each application has a 'token' key.

Once you put your credentials in the mobile app you will see your device in the dashboard device list.

You are now ready to configure alarms (or hooks) in your instance:

```ini
[uwsgi]
plugins = pushover
; register a 'pushme' alarm
alarm = pushme pushover:token=XXX,user=YYY
; raise alarms no more than 1 time per minute (default is 3 seconds)
alarm-freq = 60

; raise an alarm whenever uWSGI segfaults
alarm-segfault = pushme

; raise an alarm whenever /danger is hit
route = ^/danger alarm:pushme /danger has been visited !!!

; raise an alarm when the avergae response time is higher than 3000 milliseconds
metric-alarm = key=worker.0.avg_response_time,value=3000,alarm=pushme

...
```

The 'user' keyval in the pushover alarm is the destination of the message. Each user registered to pushover.net has the so called 'user-key' (the destination of the message). You can even define groups of user-keys.

To send a pushover alarm you only need to specify the "token" (read: the application key) and the "user" (the user key).

Hooks instead requires a "message" option too:

```ini
[uwsgi]
plugins = pushover

hook-post-app = pushover:token=XXX,user=YYY,message=Your app has been loaded
...
```

Available keyval options
------------------------

pushover-api related:

* token
* user
* message
* device
* title
* url
* url_title
* priority
* timestamp
* sound

check https://pushover.net/api#details for details

uwsgi related:

* pushover_url -> allows you to specify an alternative pushover url (default https://api.pushover.net/1/messages.json)
* ssl_no_verify -> if set, curl will not verify the server ssl certificate
* timeout -> set the socket timeout

Bulding and requirements
------------------------

The plugin makes use of libcurl, it is 2.0-friendly so you can easily build it with:

```sh
uwsgi --build-plugin <directory>
```
