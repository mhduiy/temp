// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "service-sdbus.h"

#ifdef SDBUS_SERVICE

#  include "common/common.h"
#  include "common/log.h"
#  include "manager.h"

#  include <stdarg.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
#  include <threads.h>

const char *PASSKEY_SERVICE_DBUS_NAME = "com.deepin.Passkey";
const char *PASSKEY_SERVICE_DBUS_PATH = "/com/deepin/Passkey";
const char *PASSKEY_SERVICE_DBUS_INTERFACE = "com.deepin.Passkey";
const char *PASSKEY_SERVICE_DBUS_MANAGER_INTERFACE = "com.deepin.Passkey.Manager";

static int dpk_service_error_return(sd_bus_message *m, sd_bus_error *retError)
{
    int ret = -1;
    if (m != NULL && retError != NULL) {
        ret = sd_bus_reply_method_error(m, retError);
        sd_bus_error_free(retError);
    }
    return ret;
}

static void dpk_service_error_set(sd_bus_error *retError, const char *format, ...)
{
    if (retError == NULL || format == NULL) {
        return;
    }
    char buf[BUFSIZE] = { 0 };
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, BUFSIZE, format, ap);
    va_end(ap);

    sd_bus_error_set(retError, "com.deepin.daemon.Authenticate.Passkey.Error", buf);
}

int dpk_service_filter(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    UNUSED_VALUE(userdata);
    UNUSED_VALUE(ret_error);
    UNUSED_VALUE(m);
    return 0;
}

int abc()
{
    printf("123123123\n");
}

int method_hello(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    UNUSED_VALUE(userdata);
    UNUSED_VALUE(ret_error);
    // printf("In function thread id = %d\n", pthread_self());
    // printf("Hello There!kthread ID - %d\n", gettid());
    printf("Hello There! thread pthread ID - %lu\n", thrd_current());
    printf("Hello There! thread sleep\n");
    thrd_sleep(&(struct timespec){ .tv_sec = 3 }, NULL);
    printf("Hello There! thread sleep end\n");
    // dpk_manager_test();

    thrd_t thr;
    thrd_create(&thr, abc, NULL);
    thrd_detach(thr);

    sd_bus_emit_signal(sd_bus_message_get_bus(m), PASSKEY_SERVICE_DBUS_PATH, PASSKEY_SERVICE_DBUS_MANAGER_INTERFACE, "aaa", "s", "aabbcc");
    const char *string = "World";
    return sd_bus_reply_method_return(m, "s", string);
    // return dpk_service_error_return(m, ret_error);
}

int method_multiply(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    UNUSED_VALUE(userdata);
    UNUSED_VALUE(ret_error);
    int64_t x = 0, y = 0;
    // int r;

    // // /* Read the parameters */
    // r = sd_bus_message_read(m, "xx", &x, &y);
    // if (r < 0) {
    //     LOG(LOG_WARNING,  "Failed to parse parameters: %s\n", strerror(-r));
    //     return r;
    // }

    // /* Reply with the response */
    return sd_bus_reply_method_return(m, "x", x * y);
}

int method_divide(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
{
    UNUSED_VALUE(userdata);
    UNUSED_VALUE(ret_error);
    // std::cout << "method_divide called." << std::endl;
    int64_t x = 0, y = 0;

    // /* Read the parameters */
    // r = sd_bus_message_read(m, "xx", &x, &y);
    // if (r < 0) {
    //     LOG(LOG_WARNING,  "Failed to parse parameters: %s\n", strerror(-r));
    //     return r;
    // }

    // /* Return an error on division by zero */
    // if (y == 0) {
    //     sd_bus_error_set_const(ret_error, "org.deepin.demo.sdbus.test.error.DivisionByZero", "Sorry, can't allow division by zero.");
    //     return -EINVAL;
    // }

    return sd_bus_reply_method_return(m, "x", x / y);
}

static int dpk_service_get_pin_status(sd_bus_message *m, void *userData, sd_bus_error *retError)
{
    UNUSED_VALUE(userData);
    bool success = false;
    int status = 0;
    LOG(LOG_INFO, "service get pin status");

    if (!dpk_manager_get_pin_status(&status)) {
        dpk_service_error_set(retError, "get pin status error.");
        goto end;
    }
    int support = 0;
    int hasPin = 0;
    if (status & 0x10) {
        support = 1;
    }
    if (status & 0x1) {
        hasPin = 1;
    }

    success = true;
end:
    if (!success) {
        return dpk_service_error_return(m, retError);
    }
    return sd_bus_reply_method_return(m, "xx", support, hasPin);
}

static int dpk_service_set_pin(sd_bus_message *m, void *userData, sd_bus_error *retError)
{
    UNUSED_VALUE(userData);
    bool success = false;
    char *pin = NULL;
    char *oldPin = NULL;
    int r;

    // /* Read the parameters */
    r = sd_bus_message_read(m, "ss", &oldPin, &pin);
    if (r < 0) {
        dpk_service_error_set(retError, "Failed to parse parameters: %s", strerror(-r));
        goto end;
    }

    LOG(LOG_INFO, "set pin: %s, oldpin: %s", pin, oldPin);

    if (strlen(oldPin) == 0) {
        LOG(LOG_INFO, "set pin %s", pin);
        dpk_manager_set_pin(pin, NULL);
    } else {
        LOG(LOG_INFO, "change pin %s, oldpin %s", pin, oldPin);
        dpk_manager_set_pin(pin, oldPin);
    }

    success = true;

end:
    if (!success) {
        return dpk_service_error_return(m, retError);
    }
    return sd_bus_reply_method_return(m, "");
}

static int dpk_service_reset(sd_bus_message *m, void *userData, sd_bus_error *retError)
{
    UNUSED_VALUE(userData);
    bool success = false;

    LOG(LOG_INFO, "service reset");

    dpk_manager_reset(); // TODO 鉴权？

    success = true;

    // end:
    if (!success) {
        return dpk_service_error_return(m, retError);
    }
    return sd_bus_reply_method_return(m, "");
}

static int dpk_service_make_cred(sd_bus_message *m, void *userData, sd_bus_error *retError)
{
    UNUSED_VALUE(userData);
    bool success = false;
    char *userName = NULL;
    char *pin = NULL;
    int ret;

    // /* Read the parameters */
    ret = sd_bus_message_read(m, "ss", &userName, &pin);
    if (ret < 0) {
        dpk_service_error_set(retError, "Failed to parse parameters: %s", strerror(-ret));
        goto end;
    }

    LOG(LOG_INFO, "service make cred by %s/%s", sd_bus_message_get_sender(m), userName);

    if (!dpk_manager_make_cred(userName, pin)) {
        dpk_service_error_set(retError, "Failed to makr cred.");
        goto end;
    }

    success = true;

end:
    if (!success) {
        return dpk_service_error_return(m, retError);
    }
    return sd_bus_reply_method_return(m, "");
}

static int dpk_service_get_assert(sd_bus_message *m, void *userData, sd_bus_error *retError)
{
    UNUSED_VALUE(userData);
    bool success = false;
    char *userName = NULL;
    char *pin = NULL;
    int ret;

    /* Read the parameters */
    ret = sd_bus_message_read(m, "ss", &userName, &pin);
    if (ret < 0) {
        dpk_service_error_set(retError, "Failed to parse parameters: %s", strerror(-ret));
        goto end;
    }

    LOG(LOG_INFO, "service get assert by %s/%s", sd_bus_message_get_sender(m), userName);
    if (!dpk_manager_get_assertion(userName, pin)) {
        dpk_service_error_set(retError, "Failed to get assert.");
        goto end;
    }
    success = true;

end:
    if (!success) {
        return dpk_service_error_return(m, retError);
    }
    return sd_bus_reply_method_return(m, "");
}

static int dpk_service_get_valid_cred_count(sd_bus_message *m, void *userData, sd_bus_error *retError)
{
    UNUSED_VALUE(userData);
    bool success = false;
    char *userName = NULL;
    int ret;

    /* Read the parameters */
    ret = sd_bus_message_read(m, "s", &userName);
    if (ret < 0) {
        dpk_service_error_set(retError, "Failed to parse parameters: %s", strerror(-ret));
        goto end;
    }

    LOG(LOG_INFO, "service get assert by %s/%s", sd_bus_message_get_sender(m), userName);
    unsigned validCredCount = 0;
    dpk_manager_get_valid_cred_count(userName, NULL, &validCredCount);

    success = true;

end:
    if (!success) {
        return dpk_service_error_return(m, retError);
    }
    return sd_bus_reply_method_return(m, "x", validCredCount);
}

const sd_bus_vtable SERVICE_AUTH_VTABLE[] = { SD_BUS_VTABLE_START(0),
                                              SD_BUS_METHOD("Hello", "", "s", method_hello, SD_BUS_VTABLE_UNPRIVILEGED),
                                              SD_BUS_METHOD("Multiply", "xx", "x", method_multiply, SD_BUS_VTABLE_UNPRIVILEGED),
                                              SD_BUS_METHOD("Divide", "xx", "x", method_divide, SD_BUS_VTABLE_UNPRIVILEGED),
                                              SD_BUS_VTABLE_END };

const sd_bus_vtable SERVICE_DEV_MANAGER_VTABLE[] = { SD_BUS_VTABLE_START(0),
                                                     SD_BUS_METHOD("Hello", "", "s", method_hello, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("Multiply", "xx", "x", method_multiply, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("Divide", "xx", "x", method_divide, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("GetPinStatus", "", "xx", dpk_service_get_pin_status, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("SetPin", "ss", "", dpk_service_set_pin, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("Reset", "", "", dpk_service_reset, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("MakeCredential", "ss", "", dpk_service_make_cred, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("GetAssertion", "ss", "", dpk_service_get_assert, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_METHOD("GetValidCredCount", "s", "x", dpk_service_get_valid_cred_count, SD_BUS_VTABLE_UNPRIVILEGED),
                                                     SD_BUS_SIGNAL("aaa", "s", SD_BUS_VTABLE_DEPRECATED),
                                                     SD_BUS_VTABLE_END };

bool dpk_service_start()
{
    bool success = false;
    sd_bus *bus = NULL;
    sd_bus_slot *slot = NULL;

    if (sd_bus_open_system(&bus) < 0) {
        LOG(LOG_ERR, "sd_bus_open_user error");
        goto end;
    }
    const char *unique;
    if (sd_bus_get_unique_name(bus, &unique) < 0) {
        LOG(LOG_ERR, "sd_bus_get_unique_name error");
        goto end;
    }

    if (sd_bus_request_name(bus, PASSKEY_SERVICE_DBUS_NAME, 0) < 0) {
        LOG(LOG_ERR, "sd_bus_request_name error");
        goto end;
    }

    if (sd_bus_add_filter(bus, &slot, dpk_service_filter, NULL) < 0) {
        LOG(LOG_ERR, "sd_bus_add_filter error");
        goto end;
    }

    if (sd_bus_add_object_vtable(bus, &slot, PASSKEY_SERVICE_DBUS_PATH, PASSKEY_SERVICE_DBUS_INTERFACE, SERVICE_AUTH_VTABLE, NULL) < 0) {
        LOG(LOG_ERR, "sd_bus_add_object_vtable error");
        goto end;
    }

    if (sd_bus_add_object_vtable(bus, &slot, PASSKEY_SERVICE_DBUS_PATH, PASSKEY_SERVICE_DBUS_MANAGER_INTERFACE, SERVICE_DEV_MANAGER_VTABLE, NULL) < 0) {
        LOG(LOG_ERR, "sd_bus_add_object_vtable error");
        goto end;
    }

    // bool quit = false; // TODO timeout, 并发问题
    // while (!quit) {
    //     sd_bus_message *m = NULL;
    //     int r = sd_bus_process(bus, &m);
    //     // qInfo() << "[ServiceSDBus]sd_bus_process finish and result=" << r;
    //     if (r < 0) {
    //         // qWarning() << "[sd-bus hook]Failed to process requests: %m";
    //         break;
    //     }
    //     if (r == 0) {
    //         /* Wait for the next request to process */
    //         r = sd_bus_wait(bus, UINT64_MAX);
    //         if (r < 0) {
    //             // qWarning() << "[ServiceSDBus]Failed to wait: %m";
    //             break;
    //         }
    //         continue;
    //     }
    //     if (!m) {
    //         continue;
    //     }
    //     // qInfo() << "[ServiceSDBus]sd_bus_process Get msg=" << sd_bus_message_get_member(m);
    //     sd_bus_message_unref(m);
    // }

    int retCode = 0;
    sd_event *event = NULL;
    int ret = sd_event_default(&event);
    // if (ret < 0)
    //     return log_error_errno(r, "Failed to get event loop: %m");
    sd_bus_attach_event(bus, event, SD_EVENT_PRIORITY_NORMAL);
    // sd_bus_start(bus);
    sd_event_loop(event);
    sd_event_get_exit_code(event, &retCode);
    // if (retCode > 0)
    //     break;

    // sd_event_get_tid

end:
    // TODO delete object

    return success;
}

#endif