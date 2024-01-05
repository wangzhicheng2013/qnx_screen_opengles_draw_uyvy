#pragma once
#include "qnx_screen_context.hpp"
class qnx_screen_display {
protected:
    screen_window_t win_ = { 0 };
    int display_zorder_ = QNX_SCREEN_DEFAULT_ZORDER_OF_ZONE;
    int display_size_[2] = { 0 };
    int display_pos_[2] = { 0 };
    int visible_ = 1;
public:
    int init() {
        return init_win();
    }
    void uninit() {
        unit_win();
    }
    screen_window_t& get_win() {
        return win_;
    }
protected:
    int init_win() {
        int error = screen_create_window(&win_, QNX_SCREEN_CTX.screen_ctx);
        if (error) {
            LOG_E("screen_create_window failed, error:%s", strerror(errno));
            return error;
        }
        error = set_visiable(visible_);
        if (error) {
            return error;
        }
        error = screen_set_window_property_pv(win_, SCREEN_PROPERTY_DISPLAY, (void**)&QNX_SCREEN_CTX.screen_disp);
        if (error) {
            LOG_E("screen_set_window_property_iv for SCREEN_PROPERTY_DISPLAY failed, error:%s", strerror(errno));
            return error;
        }
        static const int usage = SCREEN_USAGE_WRITE | SCREEN_USAGE_OPENGL_ES2 | SCREEN_USAGE_OPENGL_ES3;
        error =  screen_set_window_property_iv(win_, SCREEN_PROPERTY_USAGE, &usage);
        if (error) {
            LOG_E("screen_set_window_property_iv for SCREEN_PROPERTY_USAGE failed, error:%s", strerror(errno));
            return error;
        }
        // create screen window buffers
        error = screen_create_window_buffers(win_, 1);
        if (error) {
            LOG_E("screen_create_window_buffers failed, error:%s", strerror(errno));
            return error;
        }
        LOG_I("qnx screen display init win ok!");
        return 0;
    }
    void unit_win() {
        screen_destroy_window(win_);        // can call many times
    }
    int set_visiable(int visible) {         // can set visible after the win init ok
        visible_ = visible;
        int error = screen_set_window_property_iv(win_, SCREEN_PROPERTY_VISIBLE, &visible_);
        if (error) {
            LOG_E("screen_set_window_property_iv for SCREEN_PROPERTY_VISIBLE failed, error:%s", strerror(errno));
            return error;
        }
        return 0;
    }
public:
    inline void set_display_zorder(int zorder) {
        display_zorder_ = zorder;
    }
    int set_display_position(int x, int y) {
        display_pos_[0] = x;
        display_pos_[1] = y;
        LOG_I("set display position: %dx%d", display_pos_[0], display_pos_[1]);
        int error = screen_set_window_property_iv(win_, SCREEN_PROPERTY_POSITION, display_pos_);
        if (error) {
            LOG_E("screen_set_window_property_iv for SCREEN_PROPERTY_POSITION failed, error:%s", strerror(errno));
            return error;
        }
        return 0;
    }
    int set_display_size(int width, int height) {
        if ((display_size_[0] == width) && (display_size_[1] == height)) {
            LOG_D("not need to set display size %dx%d", display_size_[0], display_size_[1]);
            return 0;
        }
        display_size_[0] = width;
        display_size_[1] = height;
        LOG_D("display size: %dx%d", display_size_[0], display_size_[1]);
        int error = screen_set_window_property_iv(win_, SCREEN_PROPERTY_SIZE, display_size_);
        if (error) {
            LOG_E("screen_set_window_property_iv for SCREEN_PROPERTY_SIZE failed, error:%s", strerror(errno));
            return error;
        }
        return 0;
    }
    int set_screen_buffer_size(int size[2]) {
        int error = screen_set_window_property_iv(win_, SCREEN_PROPERTY_BUFFER_SIZE, size);
        if (error) {
            LOG_E("screen_set_window_property_iv for SCREEN_PROPERTY_BUFFER_SIZE failed, error:%s", strerror(errno));
            return error;
        }
        return 0;
    }
};