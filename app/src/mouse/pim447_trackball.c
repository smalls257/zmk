/*
 * Copyright (c) 2021 Cedric VINCENT
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <zmk/mouse.h>


#include <drivers/i2c.h>

#include <drivers/sensor.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(PIM447, CONFIG_SENSOR_LOG_LEVEL);

/*
 * It feels more natural and more confortable to square the speed
 * reported by the PIM447 trackball.
 */
static int16_t convert_speed(int32_t value)
{
    bool negative = false;

    if (value < 0) {
        negative = true;
        value = -value;
    }

    if (value > 11) {
        value = 11;
    }

    value = value * value;

    if (negative) {
        value = -value;
    }

    return value;
}

static void thread_code(void *p1, void *p2, void *p3)
{
    const struct device *dev;
    int result;

    /* PIM447 trackball initialization. */
     const char *label = DT_INST_BUS_LABEL(0);
    dev = device_get_binding(label);
    if (dev == NULL) {
        LOG_ERR("Cannot get PIM447_TRACKBALL device");
        return;
    }

    /* Event loop. */

    bool button_press_sent   = false;
    bool button_release_sent = false;

    while (true) {
        struct sensor_value pos_dx, pos_dy, pos_dz;
        bool send_report = false;

        result = sensor_sample_fetch(dev);
        if (result < 0) {
            LOG_ERR("Failed to fetch PIM447_TRACKBALL sample");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DX, &pos_dx);
        if (result < 0) {
            LOG_ERR("Failed to get PIM447_TRACKBALL pos_dx channel value");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DY, &pos_dy);
        if (result < 0) {
            LOG_ERR("Failed to get PIM447_TRACKBALL pos_dy channel value");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DZ, &pos_dz);
        if (result < 0) {
            LOG_ERR("Failed to get PIM447_TRACKBALL pos_dz channel value");
            return;
        }

        if (pos_dx.val1 != 0 || pos_dy.val1 != 0) {
            zmk_hid_mouse_movement_update(convert_speed(pos_dx.val1),
                                          convert_speed(pos_dy.val1));
            send_report = true;
        }

        if (pos_dz.val1 == 0x80 && button_press_sent == false) {
            zmk_hid_mouse_button_press(0);
            button_press_sent   = true;
            button_release_sent = false;
            send_report = true;
        } else if (pos_dz.val1 == 0x01 && button_release_sent == false) {
            zmk_hid_mouse_button_release(0);
            button_press_sent   = false;
            button_release_sent = true;
            send_report = true;
        }

        if (send_report) {
            zmk_endpoints_send_mouse_report();
            zmk_hid_mouse_clear();
        }

        k_sleep(K_MSEC(10));
    }
}

#define STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
static struct k_thread thread;

int zmk_pim447_trackball_init()
{
    k_thread_create(&thread, thread_stack, STACK_SIZE, thread_code,
                    NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
    return 0;
}

SYS_INIT(zmk_pim447_trackball_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
