#include "rtc.h"
#include "ports.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static int rtc_update_in_progress(void) {
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static uint8_t rtc_read_register(int reg) {
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

void rtc_init(void) {
    // Basic initialization if needed, currently we just poll
}

rtc_time_t rtc_read_time(void) {
    rtc_time_t time;
    uint8_t registerB;

    // Wait until update is not in progress
    while (rtc_update_in_progress());

    time.second = rtc_read_register(0x00);
    time.minute = rtc_read_register(0x02);
    time.hour   = rtc_read_register(0x04);
    time.day    = rtc_read_register(0x07);
    time.month  = rtc_read_register(0x08);
    time.year   = rtc_read_register(0x09);

    registerB = rtc_read_register(0x0B);

    // Convert BCD to binary if necessary
    if (!(registerB & 0x04)) {
        time.second = (time.second & 0x0F) + ((time.second / 16) * 10);
        time.minute = (time.minute & 0x0F) + ((time.minute / 16) * 10);
        time.hour   = ( (time.hour & 0x0F) + (((time.hour & 0x70) / 16) * 10) ) | (time.hour & 0x80);
        time.day    = (time.day & 0x0F) + ((time.day / 16) * 10);
        time.month  = (time.month & 0x0F) + ((time.month / 16) * 10);
        time.year   = (time.year & 0x0F) + ((time.year / 16) * 10);
    }

    // Convert 12 hour format to 24 hour format if necessary, or just handle AM/PM display logic in UI
    // For simplicity, we just return raw hour and handle display logic.
    // Standard CMOS year is 2 digits (e.g. 23 for 2023). We'll assume 20xx.
    time.year += 2000; 

    return time;
}
