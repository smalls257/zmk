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
