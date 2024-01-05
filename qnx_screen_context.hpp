#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <screen/screen.h>
#include "color_log.hpp"
#include "global_data_define.hpp"
struct qnx_screen_context {
private:
    // Screen resolution
    int screen_width = 1920;
    int screen_height = 1080;
    int screen_size[2] = { 0 };
    int display_count = 0;
public:
    screen_context_t screen_ctx = nullptr;
    screen_display_t *screen_disps = nullptr;
    screen_display_t screen_disp = nullptr;
    static qnx_screen_context& get_instance() {
        static qnx_screen_context instance;
        return instance;
    }
    inline void set_screen_scale(int width, int height) {
        screen_width = width;
        screen_height = height;
    }
    inline int get_screen_width() const {
        return screen_width;
    }
    inline int get_screen_height() const {
        return screen_height;
    }
    int init() {
        int error = screen_create_context(&screen_ctx, SCREEN_APPLICATION_CONTEXT);
        if (error) {
            LOG_E("screen_create_context failed:%s", strerror(errno));
            return error;
        }
        // Using the number of displays returned by the query,
        // allocate enough memory to retrieve an array of pointers to screen_display_t
        error = screen_get_context_property_iv(screen_ctx, SCREEN_PROPERTY_DISPLAY_COUNT, &display_count);
        if (error) {
            LOG_E("screen_get_context_property_iv failed:%s", strerror(errno));
            return error;
        }
        LOG_I("qnx get screen count:%d", display_count);
        if (display_count < 1) {
            LOG_E("SCREEN_PROPERTY_DISPLAY_COUNT is less than 1!");
            return QNX_SCREEN_DISPLAY_COUNT_LACK;
        }
        screen_disps = (screen_display_t *)calloc(display_count, sizeof(screen_display_t));
        assert(screen_disps != nullptr);
        error = screen_get_context_property_pv(screen_ctx, SCREEN_PROPERTY_DISPLAYS, (void **)screen_disps);
        if (error) {
            LOG_E("screen_get_context_property_pv failed:%s", strerror(errno));
            return error;
        }
        int i = 0;
        // choose the correct display screen
        for (;i < display_count;i++) {
            screen_disp = screen_disps[i];
            error = screen_get_display_property_iv(screen_disp, SCREEN_PROPERTY_SIZE, screen_size);
            if (error) {
                LOG_E("screen_get_display_property_iv failed:%s\n", strerror(errno));
                return error;
            }
            LOG_I("qnx_screen_context get screen index:%d, size:%dx%d\n", i, screen_size[0], screen_size[1]);
            if ((screen_width == screen_size[0]) && (screen_height == screen_size[1])) {
                LOG_I("get correct screen display index:%d\n", i);
                break;
            }
        }
        if (i >= display_count) {
            LOG_E("no find the target screen resolution width:%d,height:%d\n", screen_width, screen_height);
            return QNX_SCREEN_DISPLAY_NOT_FOUND;
        }
        LOG_I("qnx screen context init ok!");
        return 0;
    }
private:
    qnx_screen_context() = default;
    virtual ~qnx_screen_context() {
        uninit();
    }
    void uninit() {
        if (screen_ctx != nullptr) {
            screen_destroy_context(screen_ctx);
            screen_ctx = nullptr;
        }
        if (screen_disps != nullptr) {
            free(screen_disps);
            screen_disps = nullptr;
        }
    }
};
#define QNX_SCREEN_CTX qnx_screen_context::get_instance()