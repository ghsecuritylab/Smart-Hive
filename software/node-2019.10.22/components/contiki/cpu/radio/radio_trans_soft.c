/*
 * This file is part of the ART-6LoWPAN Radio Library.
 *
 * Copyright (c) 2017, Armink, <armink.ztl@gmail.com>
 * Copyright (c) 2017, xidongxu, <xi_dongxu@126.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Radio transition software process. It provide API to contiki radio driver layer.
 * Created on: 2017-09-15
 */

#include "radio_cfg.h"

#define LOG_TAG    "radio_trans"
#define LOG_LVL    RADIO_LOG_LVL

#include <elog.h>
#include "radio_hal.h"
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include "Si446x/si446x_api_lib.h"
#include <lib/list.h>
#include <board.h>

/* ���ͻ�������С */
#ifndef RADIO_TRANS_TX_FIFO_SIZE
#define RADIO_TRANS_TX_FIFO_SIZE                 (2*1024)
#endif
/* ���ջ�������С */
#ifndef RADIO_TRANS_RX_FIFO_SIZE
#define RADIO_TRANS_RX_FIFO_SIZE                 (2*1024)
#endif
/* �ѽ��ձ��������������󳤶� */
#ifndef RADIO_TRANS_RX_PKGS_LIST_MAX_LEN
#define RADIO_TRANS_RX_PKGS_LIST_MAX_LEN         32
#endif
/* ��Ƶ���ݴ����У�鷽ʽ �� ��ע�⡿��������֤ SI4463 ��Ӳ�� CRC ���ɿ������Ա�����ʹ�������� CRC У�� */
#ifndef RADIO_TRANS_CRC_BIT
#define RADIO_TRANS_CRC_BIT                      16
#endif
/* ��Ƶ���� filed1 Ĭ�ϲ�ʹ��λ��ת */
#ifndef RADIO_FILED1_BIT_ORDER_REVERSE
#define RADIO_FILED1_BIT_ORDER_REVERSE           0
#endif
/* �������ݳ�ʱ */
#ifndef RADIO_SEND_TIMEOUT
    #if RF_CORE_CONF_DATARATE_20K
        #define RADIO_SEND_TIMEOUT               (RT_TICK_PER_SECOND/5)
    #elif RF_CORE_CONF_DATARATE_10K
        #define RADIO_SEND_TIMEOUT               (RT_TICK_PER_SECOND/3)
    #elif RF_CORE_CONF_DATARATE_4800
        #define RADIO_SEND_TIMEOUT               (RT_TICK_PER_SECOND/2)
    #elif RF_CORE_CONF_DATARATE_2400
        #define RADIO_SEND_TIMEOUT               (RT_TICK_PER_SECOND/1)
    #endif /* RF_CORE_CONF_DATARATE_20K */
#endif /* RADIO_SEND_TIMEOUT */
/* �������ݳ�ʱ */
#ifndef RADIO_RECV_TIMEOUT
    #if RF_CORE_CONF_DATARATE_20K
        #define RADIO_RECV_TIMEOUT               (RT_TICK_PER_SECOND/5)
    #elif RF_CORE_CONF_DATARATE_10K
        #define RADIO_RECV_TIMEOUT               (RT_TICK_PER_SECOND/3)
    #elif RF_CORE_CONF_DATARATE_4800
        #define RADIO_RECV_TIMEOUT               (RT_TICK_PER_SECOND/2)
    #elif RF_CORE_CONF_DATARATE_2400
        #define RADIO_RECV_TIMEOUT               (RT_TICK_PER_SECOND/1)
    #endif /* RF_CORE_CONF_DATARATE_20K */
#endif /* RADIO_RECV_TIMEOUT */
/* ����ĳ��ȣ�IEEE802.15.4 ���֡���� 127�� */
#ifndef RADIO_PKT_MAX_LEN
#define RADIO_PKT_MAX_LEN                        127
#endif
/* ������������ֵ�������ô����������Ƶģ��������³�ʼ�� */
#define CMD_ERR_THRESHOLD_FOR_REINIT             10

/* ������������������¼� */
enum {
    RADIO_TX_FINISH    = (1L << 0), /* ������� */
    RADIO_TX_TIMEOUT   = (1L << 1), /* ���ͳ�ʱ */
    RADIO_RX_FINISH    = (1L << 2), /* ������� */
    RADIO_RX_TIMEOUT   = (1L << 3), /* ���ճ�ʱ */
    RADIO_RX_CRC_ERR   = (1L << 4)  /* ���ճ��� */
};

/* 802.15.4g ��Ƶ�շ����ݵĻ�����λ������ */
struct radio_pkt {
    struct radio_pkt *next;
    int rssi;
    size_t len;
    uint8_t *data;
};
typedef struct radio_pkt *radio_pkt_t;

/* ����ͳ����Ϣ */
struct trans_stat {
    /* ����ɹ��������� */
    size_t success_pkt_num;
    /* ����ʧ�ܱ������� */
    size_t failed_pkt_num;
    /* �ܼƴ���ɹ�������������λ���ֽ� */
    size_t total_byte;
};
typedef struct trans_stat *trans_stat_t;

/* �շ����ݳ�ʱʱ�� ��λ������ */
#define TRANS_TIMEOUT               (10*1000)

/* SI446x packet header length for 802.15.4g mode */
#define SI446X_PHR_LENGTH            2
/* CRC У�����ݳ��ȣ���λ���ֽ� */
#if RADIO_TRANS_CRC_BIT == 16
#define SI446X_CRC_LENGTH            2
#elif RADIO_TRANS_CRC_BIT == 32
#define SI446X_CRC_LENGTH            4
#endif /* RADIO_TRANS_CRC_BIT == 16 */

/* ���������� */
static rt_mutex_t trans_locker = NULL;
/* �ж�֪ͨ�¼� */
static rt_sem_t isr_notice = NULL;
/* �������֪ͨ�¼� */
static rt_event_t radio_event = NULL;
/* si446x ���� FIFO �ϴη��͵����ݴ�С */
static size_t si446x_tx_fifo_last_size = 0;
/* ���� FIFO �� si446x FIFO ��ʣ��ģ��ȴ�ͨ����Ƶ���͵����ݴ�С */
static size_t tx_reamin_size = 0;
/* �˴δ����ܼ���Ҫ���͵����ݴ�С */
static size_t tx_total_size = 0;
/* �������ݱ����� payload �ĳ��� */
static size_t rx_payload_length = 0;
/* �������ݱ����� filed2 �ĳ��� */
static size_t rx_filed2_length = 0;
/* ���ջ��������Ѿ����յ������ݴ�С */
static size_t rx_buf_recived_size = 0;
/* �������ݻ����� */
static struct rt_ringbuffer tx_fifo = { 0 };
/* �������ݻ����� */
static struct rt_ringbuffer rx_fifo = { 0 };
/* ����ѽ��յ��������ݵ����� @see radio_pkt_t */
LIST(rx_pkts);
/* ��ǰ�Ƿ����ڷ������� */
static bool is_sending = false;
/* ��ǰ�Ƿ����ڽ������� */
static bool is_recving = false;
/* ��Ƶ�ж��źŴ����߳� */
static rt_thread_t radio_isr = NULL;
/* �ȴ���������¼���Э�� */
struct process *wait_recved_process = NULL;
/* ��ʼ����ʱ�䡢��ʼ����ʱ�� */
static time_t recv_start, send_start;
/* ���ճ�ʱ���ƶ�ʱ�� */
static rt_timer_t recv_timer;
/* ���ͳ�ʱ���ƶ�ʱ�� */
static rt_timer_t send_timer;
/* ������ɹ��Ӻ��� */
static void (*recved_hook)(size_t recv_len);
/* �շ�ͳ����Ϣ */
static struct trans_stat send_stat = { 0 }, recv_stat = { 0 };
/* ���������� */
static size_t cmd_err_count = 0;

static int send_timeout_cnt = 0;

extern const tRadioConfiguration *pRadioConfiguration;
extern void si446x_pa_enabled(bool enabled);
extern void si446x_trans_led_ctrl(bool rx_on, bool tx_on);
static void radio_isr_entry(void* parameter);
static uint16_t calc_crc16(const char *data, size_t size);

#if RADIO_FILED1_BIT_ORDER_REVERSE
static uint8_t bit_order_reverse(uint8_t byte_to_reverse) {
    byte_to_reverse = (byte_to_reverse & 0xF0) >> 4 | (byte_to_reverse & 0x0F) << 4;
    byte_to_reverse = (byte_to_reverse & 0xCC) >> 2 | (byte_to_reverse & 0x33) << 2;
    byte_to_reverse = (byte_to_reverse & 0xAA) >> 1 | (byte_to_reverse & 0x55) << 1;
    return byte_to_reverse;
}
#endif

static uint16_t get_payload_length_from_phr(uint8_t* phr) {
    uint16_t len = (phr[0] & 0x03) * 256 + phr[1] - SI446X_CRC_LENGTH;

    return len > RADIO_PKT_MAX_LEN ? RADIO_PKT_MAX_LEN : len;
}

static void set_field1(char *field1, bool dw, uint16_t field2_len) {
    field1[0] = 0;
    field1[1] = 0;
    /* ���� crc �ĳ��� */
    field2_len += SI446X_CRC_LENGTH;
#if RADIO_FILED1_BIT_ORDER_REVERSE
    field1[0] |= bit_order_reverse(field2_len >> 8);
#else
    field1[0] |= field2_len >> 8;
#endif /* RADIO_FILED1_BIT_ORDER_REVERSE */

#if RADIO_IEEE_802_15_4_PACKET
#if RADIO_TRANS_CRC_BIT == 16
    /* CRC16 */
    field1[0] |= 0x08;
#elif RADIO_TRANS_CRC_BIT == 32
    /* CRC32 */
    field1[0] |= 0x00;
#endif /* RADIO_TRANS_CRC_BIT == 16 */
    if (dw) {
        field1[0] |= 0x10;
    }
#endif /* RADIO_IEEE_802_15_4_PACKET */

#if RADIO_FILED1_BIT_ORDER_REVERSE
    field1[1] = bit_order_reverse(field2_len);
#else
    field1[1] = field2_len;
#endif /* RADIO_FILED1_BIT_ORDER_REVERSE */
}

static void trans_fifo_clear(struct rt_ringbuffer *fifo) {
    rt_ringbuffer_reset(fifo);
}

static void print_hex(const uint8_t *packet, size_t size, uint8_t log_lvl) {
    size_t i;
    char *hex;

    assert(packet);

    if (!size) {
        return;
    }

    hex = rt_malloc(size * 5 + 1);
    if (hex) {
        for (i = 0; i < size; i++) {
            snprintf(hex + 5 * i, 6, " 0x%02X", *(packet + i));
        }
        switch (log_lvl) {
        case ELOG_LVL_ASSERT: {
            log_a("package: %s", hex + 1);
            break;
        }
        case ELOG_LVL_ERROR: {
            log_e("package: %s", hex + 1);
            break;
        }
        case ELOG_LVL_WARN: {
            log_w("package: %s", hex + 1);
            break;
        }
        case ELOG_LVL_INFO: {
            log_i("package: %s", hex + 1);
            break;
        }
        case ELOG_LVL_DEBUG: {
            log_d("package: %s", hex + 1);
            break;
        }
        case ELOG_LVL_VERBOSE: {
            log_v("package: %s", hex + 1);
            break;
        }
        }
        rt_free(hex);
    } else {
        log_w("ע�⣺�ڴ治��!");
    }
}

/**
 * ��Ӳ�� FIFO ��ȡ�����յ�����
 *
 * @param recv_is_finish �����Ƿ����
 *
 * @return ���յ����ҿ�����ɵ�����
 */
static size_t recv_from_hw_fifo(bool recv_is_finish) {
    static uint8_t buffer[SI466X_FIFO_SIZE] = { 0 };
    size_t copy_size = 0;
    uint8_t phr[SI446X_PHR_LENGTH], hw_fifo_count = 0;
    static bool phr_is_detected = false;

    /* ��ȡ SI446X Ӳ�� FIFO ��Ϣ */
    radio_cmd_reply_lock();
    si446x_fifo_info(0);
    hw_fifo_count = Si446xCmd.FIFO_INFO.RX_FIFO_COUNT;
    radio_cmd_reply_unlock();

    if (hw_fifo_count > SI466X_FIFO_SIZE) {
        hw_fifo_count = SI466X_FIFO_SIZE;
        log_w("radio hardware FIFO size has some error.");
    }
    /* �Ƿ��Ѿ���⵽�� PHR */
    if (!phr_is_detected) {
        /* ����֮ǰ���ܲ������ѽ������� */
        trans_fifo_clear(&rx_fifo);
        if (hw_fifo_count >= sizeof(phr)) {
            phr_is_detected = true;
            /* ȡ�� PHR */
            si446x_read_rx_fifo(sizeof(phr), phr);
            /* ���� payload length */
            rx_payload_length = get_payload_length_from_phr(phr);
            /* ���� filed2 length */
            rx_filed2_length = rx_payload_length + SI446X_CRC_LENGTH;
            if (sizeof(phr) + rx_filed2_length <= hw_fifo_count) {
                copy_size = rx_filed2_length;
            } else {
                copy_size = hw_fifo_count - sizeof(phr);
            }
            log_v("get PHR([0]:0x%02X, [1]:0x%02X) payload length is %d bytes", phr[0], phr[1], rx_payload_length);
        }
    } else {
        copy_size = hw_fifo_count;
        if ((rx_buf_recived_size + copy_size) >= rx_filed2_length) {
            copy_size = rx_filed2_length - rx_buf_recived_size;
        }
    }
    if (copy_size) {
        /* ��Ӳ�� FIFO ��ȡ������ */
        si446x_read_rx_fifo(copy_size, buffer);
        /* ��ӡ���յ������� */
        print_hex(buffer, copy_size, ELOG_LVL_VERBOSE);
        rx_buf_recived_size += copy_size;
        /* ���ݷ������ FIFO �� */
        rt_ringbuffer_put_force(&rx_fifo, (rt_uint8_t *) buffer, copy_size);
    }
    /* ������ɣ���ԭ״̬ */
    if (recv_is_finish) {
        phr_is_detected = false;
    }

    return copy_size;
}

static void trans_lock(void) {
    rt_mutex_take(trans_locker, RT_WAITING_FOREVER);
}

static void trans_unlock(void) {
    rt_mutex_release(trans_locker);
}

static void transmit_timeout(void *ptr) {
    trans_lock();
    if (is_sending) {
        log_w("radio send timeout.");
        send_stat.failed_pkt_num ++;
        /* �ر���Ƶģ��PA */
        si446x_pa_enabled(false);
        /* �������� */
        radio_start_rx(pRadioConfiguration->Radio_ChannelNumber, 0u);
        is_sending = false;
        rt_event_send(radio_event, RADIO_TX_TIMEOUT);
    } else {
        log_w("radio receive timeout.");
        /* Reset FIFO */
        si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);
        rx_buf_recived_size = 0;
        rx_payload_length = 0;
        recv_stat.failed_pkt_num ++;
        /* �������� */
        radio_start_rx(pRadioConfiguration->Radio_ChannelNumber, 0u);
        is_recving = false;
        rt_event_send(radio_event, RADIO_RX_TIMEOUT);
    }
    si446x_trans_led_ctrl(false, false);
    trans_unlock();
}

#if RAW_RADIO_TEST
static void radio_test_entry(void* parameter) {
    extern bool si446x_test_key_press(void);
    extern rt_err_t radio_pkt_send_sync(const uint8_t *buf, size_t size);
    uint8_t radio_data[] = "Hello ART-6LoWPAN!";

    while (true) {
        if (si446x_test_key_press()) {
            /* ���� 18+1 �ֽ����� */
            radio_pkt_send_sync(radio_data, sizeof(radio_data));
        }
        /* 10ms ��� */
        rt_thread_delay(rt_tick_from_millisecond(10));
    }
}
#endif /* RAW_RADIO_TEST */

/**
 * ��Ƶ������������ֳ�ʼ��
 */
void radio_transmit_software_init(void) {
    static bool init_ok = false;

    if (init_ok) {
        return;
    }
    /* ���������� */
    trans_locker = rt_mutex_create("trans_lock", RT_IPC_FLAG_FIFO);
    /* ��������ɹ������������ʾ������ */
    assert(trans_locker);

    /* ��ʼ���������ݻ����� */
    rt_uint8_t * tx_buf = (rt_uint8_t *) rt_malloc(RADIO_TRANS_TX_FIFO_SIZE * sizeof(rt_uint8_t));
    /* ��������ɹ������������ʾ������ */
    assert(tx_buf);
    rt_ringbuffer_init(&tx_fifo, tx_buf, RADIO_TRANS_TX_FIFO_SIZE);

    /* ��ʼ���������ݻ����� */
    rt_uint8_t * rx_buf = (rt_uint8_t *) rt_malloc(RADIO_TRANS_RX_FIFO_SIZE * sizeof(rt_uint8_t));
    /* ��������ɹ������������ʾ������ */
    assert(rx_buf);
    rt_ringbuffer_init(&rx_fifo, rx_buf, RADIO_TRANS_RX_FIFO_SIZE);

    /* ��ʼ���������ݵ����� */
    list_init(rx_pkts);

    isr_notice = rt_sem_create("radio_isr", 0, RT_IPC_FLAG_FIFO);
    /* ��������ɹ������������ʾ������ */
    assert(isr_notice);

    radio_event = rt_event_create("radio", RT_IPC_FLAG_FIFO);
    /* ��������ɹ������������ʾ������ */
    assert(radio_event);

    recv_timer = rt_timer_create("radio_recv", transmit_timeout, NULL, RADIO_RECV_TIMEOUT,
            RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    /* ��������ɹ������������ʾ������ */
    assert(recv_timer);

    send_timer = rt_timer_create("radio_send", transmit_timeout, NULL, RADIO_RECV_TIMEOUT,
            RT_TIMER_FLAG_ONE_SHOT | RT_TIMER_FLAG_SOFT_TIMER);
    /* ��������ɹ������������ʾ������ */
    assert(send_timer);

    /* ��ʼ���жϴ����߳� */
    radio_isr = rt_thread_create("radio_isr", radio_isr_entry, NULL, 2048, 3, 20);
    /* ��������ɹ������������ʾ������ */
    assert(radio_isr);
    rt_thread_startup(radio_isr);

#if RAW_RADIO_TEST
    /* ��ʼ����Ƶ�����߳� */
    rt_thread_t radio_test_thread = rt_thread_create("radio_test", radio_test_entry, NULL, 1024, 15, 50);
    if (radio_test_thread) {
        rt_thread_startup(radio_test_thread);
    }
#endif /* RAW_RADIO_TEST */

    init_ok = true;
}

/**
 * ֪ͨ�Ѳ�����Ƶ�ж�
 */
void radio_notice_irq(void) {
    if (isr_notice) {
        rt_sem_release(isr_notice);
    }
}

bool radio_is_sending(void) {
    return is_sending;
}

bool radio_is_recving(void) {
    return is_recving;
}

/**
 * ��Ƶ���ݱ��ķ��ͣ��� 802.15.4g ����Ϊ��λ��@note �˷���Ϊ����������
 *
 * @param buf ���ͱ��Ļ�����
 * @param size �ñ������ݵĴ�С
 *
 * @return ��� RT_EOK: �ɹ���-RT_EBUSY����ǰ���ڷ������ݣ����Ժ�
 */
rt_err_t radio_pkt_send(const uint8_t *buf, size_t size) {
    static uint8_t buffer[SI466X_FIFO_SIZE] = { 0 };
    char field1[SI446X_PHR_LENGTH] = { 0 };
    uint32_t crc = 0;
    rt_uint8_t field2_crc[SI446X_CRC_LENGTH] = { 0 };
    uint8_t filed2_size_high, filed2_size_low;
    rt_size_t filed2_len;
    rt_tick_t timeout = RADIO_SEND_TIMEOUT;
    rt_err_t result = RT_EOK;

    assert(buf);
    rt_enter_critical();
    trans_lock();

    if (size > RADIO_PKT_MAX_LEN) {
        log_w("send size must <= %d", RADIO_PKT_MAX_LEN);
        result = -RT_ERROR;
        goto __exit;
    }

    if (is_sending) {
        log_w("radio is sending, please wait and retry.");
        result = -RT_EBUSY;
        goto __exit;
    }
    is_sending = true;

    /* ��շ������ FIFO */
    trans_fifo_clear(&tx_fifo);
    /* ���� filed1 */
    set_field1(field1, true, size);
    /* �� field1 �� filed2 �ֱ����������� FIFO */
    rt_ringbuffer_put_force(&tx_fifo, (rt_uint8_t *) field1, sizeof(field1));
    rt_ringbuffer_put_force(&tx_fifo, (rt_uint8_t *) buf, size);

    /* ���㲢����CRC */
#if RADIO_TRANS_CRC_BIT == 16
    crc = calc_crc16((const char *) buf, size);
    field2_crc[0] = crc >> 8;
    field2_crc[1] = crc & 0xFF;
#elif RADIO_TRANS_CRC_BIT == 32
    #error "not supported!"
#endif
    /* �� filed2_crc ����������� FIFO */
    rt_ringbuffer_put_force(&tx_fifo, field2_crc, sizeof(field2_crc));

    /* ���¼�����������ݳ��� */
    size += SI446X_PHR_LENGTH + SI446X_CRC_LENGTH;
    tx_reamin_size = size;
    tx_total_size = size;

    if (size > SI466X_FIFO_SIZE) {
        si446x_tx_fifo_last_size = SI466X_FIFO_SIZE;
    } else {
        si446x_tx_fifo_last_size = size;
    }

    log_v("send data %d byte, PHR[0]:0x%02X, PHR[1]:0x%02X, CRC:0x%08X,", size, field1[0], field1[1], crc);
    /* ��ӡ filed2 �е� payload */
    print_hex(buf, size - SI446X_PHR_LENGTH - SI446X_CRC_LENGTH, ELOG_LVL_VERBOSE);

    send_start = clock();
    si446x_trans_led_ctrl(false, true);

    /* ��շ���Ӳ�� FIFO */
    si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT);

    /* pkt �ܳ��ȼ�ȥ filed1 �ĳ���*/
    filed2_len = rt_ringbuffer_data_len(&tx_fifo) - RADIO_PH_FILE1_LEN;
    filed2_size_high = filed2_len >> 8;
    filed2_size_low = filed2_len;
    /* ���� field2 ���� */
    si446x_set_property(SI446X_PROP_GRP_ID_PKT, 2, SI446X_PROP_PKT_FIELD_2_LENGTH_FIELD_2_LENGTH_12_8_INDEX,
            filed2_size_high, filed2_size_low);
    /* ȡ����һ�οɷ��͵����� */
    rt_ringbuffer_get(&tx_fifo, (rt_uint8_t *) buffer, si446x_tx_fifo_last_size);
    /* ����Ƶģ��PA */
    si446x_pa_enabled(true);
    /* �������� */
    radio_start_tx_variable_packet_multifield(pRadioConfiguration->Radio_ChannelNumber, buffer,
            si446x_tx_fifo_last_size);
    /* �������䳬ʱ��ʱ�� */
    rt_timer_control(send_timer, RT_TIMER_CTRL_SET_TIME, (void *) &timeout);
    rt_timer_start(send_timer);

__exit:
    trans_unlock();
    rt_exit_critical();
    return result;
}

/**
 * ��Ƶ���ݱ��Ĳ���ͬ����ʽ���ͣ��� 802.15.4g ����Ϊ��λ��@note �˷���Ϊ��������
 *
 * @param buf ���ͱ��Ļ�����
 * @param size �ñ������ݵĴ�С
 *
 * @return ��� RT_EOK: �ɹ���-RT_EBUSY����ǰ���ڷ������ݣ����Ժ�-RT_ETIMEOUT �������ݳ�ʱ
 */
rt_err_t radio_pkt_send_sync(const uint8_t *buf, size_t size) {
    rt_err_t result = RT_EOK;
    rt_uint32_t recved;

    if ((result = radio_pkt_send(buf, size)) == RT_EOK) {
        /* ����Ѵ����¼� */
        rt_event_control(radio_event, RT_IPC_CMD_RESET, NULL);
        if (rt_event_recv(radio_event, RADIO_TX_FINISH | RADIO_TX_TIMEOUT, RT_EVENT_FLAG_CLEAR | RT_EVENT_FLAG_OR,
                RT_WAITING_FOREVER, &recved) == RT_EOK) {
            if (recved & RADIO_TX_TIMEOUT) {
                send_timeout_cnt++;
                if(send_timeout_cnt == 10)
                {
                  NVIC_SystemReset();
                }
                result = -RT_ETIMEOUT;
            }
            if (recved & RADIO_TX_FINISH) {
                send_timeout_cnt = 0;
                result = RT_EOK;
            }
        } else {
            result = -RT_ETIMEOUT;
        }
    }

    return result;
}

/**
 * ���ý�����ɹ��Ӻ���
 *
 * @param hook ���Ӻ����������ڽ������ʱִ��
 */
void radio_recved_set_hook(void (*hook)(size_t recv_len)) {
    recved_hook = hook;
}

/**
 * �Ƿ��ѽ��յ�����
 */
bool radio_has_recved_pkt(void) {
    bool has_pkt = false;

    trans_lock();
    if (list_length(rx_pkts)) {
        has_pkt = true;
    }
    trans_unlock();

    return has_pkt;
}

/**
 * ��Ƶ���ݱ��Ľ��գ��� 802.15.4g ����Ϊ��λ��@note �˷���Ϊ����������
 *
 * @param buf ���ձ��Ļ�����
 * @param size �������Ĵ�С
 * @param len �ñ������ݵĳ���
 * @param rssi ���ոñ�������ʱ���ź�ǿ�ȣ���λ dbm
 *
 * @return ��� RT_EOK: �ɹ���-RT_ERROR��û������
 */
rt_err_t radio_pkt_get_recved(uint8_t* buf, size_t size, size_t *len, int *rssi) {
    rt_err_t result = RT_EOK;

    assert(buf);
    assert(len);
    trans_lock();
    if (list_length(rx_pkts)) {
        radio_pkt_t pkt = list_pop(rx_pkts);
        if (pkt) {
            if (pkt->len > size) {
                *len = size;
            } else {
                *len = pkt->len;
            }
            memcpy(buf, pkt->data, *len);
            rt_free(pkt);
            *rssi = pkt->rssi;
        }
    } else {
        result = -RT_ERROR;
    }
    trans_unlock();

    return result;
}

/**
 * ����������ɵĴ�����
 */
static void radio_recive_finished(int rssi) {
    uint32_t crc = 0, crc_recv = 0;
    bool has_crc_error = false;

    /* ������������ */
    radio_start_rx(pRadioConfiguration->Radio_ChannelNumber, 0u);
    si446x_trans_led_ctrl(false, false);
    log_d("RADIO_RX_FINISH: cost %3ld ms, total size %3d, rssi: %3ddBm, payload is %3d bytes", clock() - recv_start,
            SI446X_PHR_LENGTH + rx_filed2_length, rssi, rx_payload_length);
    /* �����յ��ı������ݣ����ñ��������� */
    if (list_length(rx_pkts) < RADIO_TRANS_RX_PKGS_LIST_MAX_LEN) {
        radio_pkt_t pkt = (radio_pkt_t)rt_malloc(RT_ALIGN(sizeof(struct radio_pkt), sizeof(size_t)) + rx_payload_length);
        if (pkt) {
            pkt->data = (uint8_t *) pkt + RT_ALIGN(sizeof(struct radio_pkt), sizeof(size_t));
            pkt->len = rx_payload_length;
            pkt->rssi = rssi;
            rt_ringbuffer_get(&rx_fifo, pkt->data, rx_payload_length);
            /* ������յ��� CRC ���� */
            for (uint8_t i = 0, temp = 0; i < SI446X_CRC_LENGTH; i++) {
                rt_ringbuffer_getchar(&rx_fifo, &temp);
                crc_recv <<= 8;
                crc_recv |= temp;
            }
            /* ���� CRC */
            //rt_kprintf("crc_recv:%o",crc_recv);
            crc = calc_crc16((const char *) pkt->data, rx_payload_length);
            //rt_kprintf("crc:%o",crc);
            if (crc == crc_recv) {
                /* ��ӵ��ѽ��յ������������� */
                list_add(rx_pkts, pkt);
                recv_stat.success_pkt_num ++;
                recv_stat.total_byte += pkt->len;
            } else {
                log_w("CRC ERROR!");
                recv_stat.failed_pkt_num ++;
                /* �ͷ��ڴ� */
                rt_free(pkt);
            }
        } else {
            log_w("Warning: No memory!");
        }
    } else {
        log_w("received package list is full.");
    }
    /* ֹͣ���ն�ʱ�� */
    rt_timer_stop(recv_timer);
    rx_buf_recived_size = 0;
    rx_payload_length = 0;
    is_recving = false;
    if (recved_hook) {
        recved_hook(rx_payload_length);
    }
    /* ֪ͨ�����߳� */
    if (!has_crc_error) {
        rt_event_send(radio_event, RADIO_RX_FINISH);
    } else {
        rt_event_send(radio_event, RADIO_RX_CRC_ERR);
    }
}

/**
 * ��Ƶ�ж��źŴ����߳�
 */
static void radio_isr_entry(void* parameter) {
    static uint8_t buffer[SI466X_FIFO_SIZE] = { 0 };
    static size_t get_size = 0;
    static size_t copy_size = 0;
    static size_t last_tansed_size = 0;
    static uint8_t hw_fifo_space;
    static int latch_rssi;
    static uint8_t ph_int, modem_int, chip_int;

    while (true) {
        rt_sem_take(isr_notice, RT_WAITING_FOREVER);
        trans_lock();

        radio_cmd_reply_lock();
        /* ��ȡ�ж�״̬ */
        si446x_get_int_status(0, 0, 0);
        ph_int = Si446xCmd.GET_INT_STATUS.PH_PEND;
        modem_int = Si446xCmd.GET_INT_STATUS.MODEM_PEND;
        chip_int = Si446xCmd.GET_INT_STATUS.CHIP_PEND;
        radio_cmd_reply_unlock();

        //rt_kprintf("chip_int:%x\n",chip_int);
        /* �鿴�жϹ���״̬��Ϣ */
        log_v("irq pend int: 0x%02X, ph: 0x%02X, modem: 0x%02X, chip: 0x%02X", Si446xCmd.GET_INT_STATUS.INT_PEND, ph_int, modem_int, chip_int);
        /* ��⵽ͬ���� */
        if (modem_int & SI446X_CMD_GET_INT_STATUS_REP_MODEM_PEND_SYNC_DETECT_PEND_BIT) {
            is_recving = true;
            recv_start = clock();
            si446x_trans_led_ctrl(true, false);
            /* �������ճ�ʱ��ʱ�� */
            rt_tick_t timeout = RADIO_RECV_TIMEOUT;
            rt_timer_control(recv_timer, RT_TIMER_CTRL_SET_TIME, (void *) &timeout);
            rt_timer_start(recv_timer);
            latch_rssi = radio_get_latch_rssi();
        }
        /* ��������жϲ��� */
        if (ph_int & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_SENT_PEND_BIT) {
            if (tx_reamin_size <= si446x_tx_fifo_last_size) {
                tx_reamin_size = 0;
            } else {
                tx_reamin_size -= si446x_tx_fifo_last_size;
                log_w("remain %d bytes not send", tx_reamin_size);
            }
            /* ֹͣ���Ͷ�ʱ�� */
            rt_timer_stop(send_timer);
            send_stat.success_pkt_num ++;
            send_stat.total_byte += tx_total_size - SI446X_PHR_LENGTH - SI446X_CRC_LENGTH;
            si446x_trans_led_ctrl(false, false);
            log_d("RADIO_TX_FINISH: cost %3ld ms, total size %3d", clock() - send_start, tx_total_size);
            /* �ر���Ƶģ��PA */
            si446x_pa_enabled(false);
            /* �������� */
            radio_start_rx(pRadioConfiguration->Radio_ChannelNumber, 0u);
            is_sending = false;
            rt_event_send(radio_event, RADIO_TX_FINISH);
            /* ���ͻ������������жϲ��� */
        } else if ((ph_int & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_TX_FIFO_ALMOST_EMPTY_PEND_BIT) && is_sending) {
            /* ��ȡ SI446X Ӳ�� FIFO ��Ϣ */
            radio_cmd_reply_lock();
            si446x_fifo_info(0);
            hw_fifo_space = Si446xCmd.FIFO_INFO.TX_FIFO_SPACE;
            radio_cmd_reply_unlock();

            if (hw_fifo_space > SI466X_FIFO_SIZE) {
                hw_fifo_space = SI466X_FIFO_SIZE;
                log_w("radio hardware FIFO size has some error.");
            }
            /* �����ϴ��Ѵ��䣨���ͣ������ݳ��� */
            if (si446x_tx_fifo_last_size >= SI466X_FIFO_SIZE - hw_fifo_space) {
                last_tansed_size = si446x_tx_fifo_last_size - (SI466X_FIFO_SIZE - hw_fifo_space);
            } else {
                log_e("si446x_tx_fifo_last_size(%d) or hw_fifo_count(%d) has some error.", si446x_tx_fifo_last_size,
                        hw_fifo_space);
                last_tansed_size = 0;
            }
            /* ���¼���ʣ����Ҫ������Ƶ���͵����� */
            if (tx_reamin_size >= last_tansed_size) {
                tx_reamin_size -= last_tansed_size;
            } else {
                log_e("tx_reamin_size(%d) or last_tansed_size(%d) has some error.", tx_reamin_size, last_tansed_size);
                tx_reamin_size = 0;
            }
            /* ������� FIFO �д�ŵ����ݳ��� */
            copy_size = rt_ringbuffer_data_len(&tx_fifo);
            /* �Ա�ʣ����Ҫ����Ӳ�� FIFO �е����ݳ��ȣ��Ƿ���� si446x FIFO ��ʣ��ռ� */
            if (copy_size > hw_fifo_space) {
                /* si446x ���� FIFO ��ȫ������� */
                copy_size = hw_fifo_space;
                si446x_tx_fifo_last_size = SI466X_FIFO_SIZE;
            } else {
                /* ��ʣ��������� FIFO ��δ���͵�ȫ���������� */
                si446x_tx_fifo_last_size = (SI466X_FIFO_SIZE - hw_fifo_space) + copy_size;
            }
            /* �� ��� ���� FIFO ��ȡ�������͵����� */
            get_size = rt_ringbuffer_get(&tx_fifo, (rt_uint8_t *) buffer, copy_size);
            if (get_size != copy_size) {
                log_e("software FIFO has some error, copy_size(%d) != get_size(%d)", copy_size, get_size);
            }
            if (get_size) {
                /* �������� */
                si446x_write_tx_fifo(get_size, buffer);
            }
            log_v("RADIO_TX_ALMOST_EMPTY: last packet sent %d bytes, add %d bytes to FIFO, remain size %d bytes",
                    last_tansed_size, get_size, tx_reamin_size);
            /* ��������жϲ��� */
        } else if (ph_int & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_PACKET_RX_PEND_BIT) {
            /* ��Ӳ�� FIFO ��ȡ�����յ����� */
            copy_size = recv_from_hw_fifo(true);
            /* ������ɴ��� */
            radio_recive_finished(latch_rssi);
            /* ���ջ������������жϲ��� */
        } else if (ph_int & SI446X_CMD_GET_INT_STATUS_REP_PH_PEND_RX_FIFO_ALMOST_FULL_PEND_BIT) {
            /* ��Ӳ�� FIFO ��ȡ�����յ����� */
            copy_size = recv_from_hw_fifo(false);
            log_v("RADIO_RX_ALMOST_FULL: current packet received %d bytes, total size %d bytes", copy_size,
                    rx_buf_recived_size);
            /* ���� filed2 ��������Ϊ������ɣ�ǿ��ֹͣ���ݽ��� */
            if (rx_buf_recived_size >= rx_filed2_length) {
                recv_from_hw_fifo(true);
                /* ������ɴ��� */
                radio_recive_finished(latch_rssi);
            }
        }
        /* ������� */
        if (chip_int & SI446X_CMD_GET_CHIP_STATUS_REP_CHIP_PEND_CMD_ERROR_PEND_BIT) {
            /* ����ԭ״̬ */
            recv_from_hw_fifo(true);
            rx_buf_recived_size = 0;
            rx_payload_length = 0;
            /* �ر���Ƶģ��PA */
            si446x_pa_enabled(false);
            si446x_trans_led_ctrl(false, false);
            log_w("RADIO_CMD_ERROR");
            /* Reset FIFO */
            si446x_fifo_info(SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT | SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT);
            trans_fifo_clear(&rx_fifo);
            /* command error count ++ */
            cmd_err_count ++;
            if (cmd_err_count > CMD_ERR_THRESHOLD_FOR_REINIT) {
                cmd_err_count = 0;
                log_e("Radio command error count > %d, reinitialize radio.", CMD_ERR_THRESHOLD_FOR_REINIT);
                //radio_hal_init();
                NVIC_SystemReset();
            } else {
                /* ������������ */
                radio_start_rx(pRadioConfiguration->Radio_ChannelNumber, 0u);
            }
            is_sending = false;
            is_recving = false;
        }
        trans_unlock();
    }
}

/**
 * ����CRC16
 *
 * @param data ����У��ֵ������
 * @param size У�����ݴ�С
 *
 * @return CRC16���
 */
static uint16_t calc_crc16(const char *data, size_t size) {
    static const uint8_t auc_crc_hi[] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
        0x00, 0xC1, 0x81, 0x40
    };

    static const uint8_t auc_crc_lo[] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7,
        0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E,
        0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9,
        0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC,
        0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32,
        0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D,
        0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38,
        0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF,
        0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1,
        0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4,
        0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB,
        0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA,
        0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0,
        0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97,
        0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E,
        0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89,
        0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83,
        0x41, 0x81, 0x80, 0x40
    };
    uint8_t crc_hi = 0xFF;
    uint8_t crc_lo = 0xFF;
    size_t index;

    while (size--) {
        index = crc_lo ^ *(data++);
        crc_lo = (uint8_t) (crc_hi ^ auc_crc_hi[index]);
        crc_hi = auc_crc_lo[index];
    }
    return (uint16_t) (crc_hi << 8 | crc_lo);
}

#if defined(RT_USING_FINSH) && defined(FINSH_USING_MSH)
#include <finsh.h>
#include <net/netstack.h>
#include <lib/random.h>

static void si4463(uint8_t argc, char **argv) {

    enum {
        CMD_SEND_INDEX,
        CMD_RECV_INDEX,
        CMD_STAT_INDEX,
        CMD_PROP_INDEX,
        CMD_TEST_INDEX,
    };

    size_t i = 0;

    const char* help_info[] = {
            [CMD_SEND_INDEX]     = "si4463 send <length>            - send some data with length",
            [CMD_RECV_INDEX]     = "si4463 recv                     - receive some data",
            [CMD_STAT_INDEX]     = "si4463 stat                     - show radio transmit statistics",
            [CMD_PROP_INDEX]     = "si4463 prop                     - get or set SI4463 property",
            [CMD_TEST_INDEX]     = "si4463 test <times>             - SI4463 send data test",
    };

    if (argc < 2) {
        rt_kprintf("Usage:\n");
        for (i = 0; i < sizeof(help_info) / sizeof(char*); i++) {
            rt_kprintf("%s\n", help_info[i]);
        }
        rt_kprintf("\n");
    } else {
        const char *operator = argv[1];

        if (!strcmp(operator, "send")) {
            if (argc < 3) {
                rt_kprintf("Usage: %s.\n", help_info[CMD_SEND_INDEX]);
                return;
            }
            size_t len = atoi(argv[2]);
            uint8_t *str = (uint8_t *) rt_malloc(len);
            memset(str, len, len);
            radio_pkt_send(str, len);
            rt_free(str);
        } else if (!strcmp(operator, "recv")) {
            size_t len;
            int rssi;
            uint8_t *str = (uint8_t *) rt_calloc(1, RADIO_TRANS_RX_FIFO_SIZE);
            if (radio_pkt_get_recved(str, RADIO_TRANS_RX_FIFO_SIZE, &len, &rssi) == RT_EOK) {
                rt_kprintf("received %d bytes package, rssi %ddBm\n", len, rssi);
                print_hex(str, len, ELOG_LVL_INFO);
            } else {
                rt_kprintf("no data, please retry.\n");
            }
            rt_free(str);
        } else if (!strcmp(operator, "stat")) {
            float recv_err_per = 0.0f;
            float send_err_per = 0.0f;
            int rssi = radio_get_latch_rssi();

            if (recv_stat.success_pkt_num + recv_stat.failed_pkt_num) {
                recv_err_per = 100.0f * (float) recv_stat.failed_pkt_num / (float) (recv_stat.success_pkt_num + recv_stat.failed_pkt_num);
            }
            if (send_stat.success_pkt_num + send_stat.failed_pkt_num) {
                send_err_per = 100.0f * (float) send_stat.failed_pkt_num / (float) (send_stat.success_pkt_num + send_stat.failed_pkt_num);
            }

            rt_kprintf("RX packets:%ld errors:%ld (%d.%02d%%) last rssi:%ddBm\n", recv_stat.success_pkt_num,
                    recv_stat.failed_pkt_num, (uint8_t) recv_err_per, (int) (recv_err_per * 100.0f) % 100, rssi);
            rt_kprintf("TX packets:%ld errors:%ld (%d.%02d%%)\n", send_stat.success_pkt_num, send_stat.failed_pkt_num,
                    (uint8_t) send_err_per, (int) (send_err_per * 100.0f) % 100);
            rt_kprintf("RX bytes:%ld (%ld.%01dKiB) TX bytes:%ld (%ld.%01dKiB)\n\n", recv_stat.total_byte,
                    recv_stat.total_byte / 1024, recv_stat.total_byte % 1024 / 100, send_stat.total_byte,
                    send_stat.total_byte / 1024, send_stat.total_byte % 1024 / 100);
        } else if (!strcmp(operator, "prop")) {
//            si446x_set_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_CONFIG1, atoi(argv[2]));
            si446x_get_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_CONFIG1);
            rt_kprintf("SI446X_PROP_GRP_INDEX_PKT_CONFIG1:0x%02X\n", Si446xCmd.GET_PROPERTY.DATA[0]);
//            si446x_set_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_CONFIG2, 0x00);
            si446x_get_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_CONFIG2);
            rt_kprintf("SI446X_PROP_GRP_INDEX_PKT_CONFIG2:0x%02X\n", Si446xCmd.GET_PROPERTY.DATA[0]);
//            si446x_set_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_CRC_CONFIG, atoi(argv[2]));
            si446x_get_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_CRC_CONFIG);
            rt_kprintf("SI446X_PROP_GRP_INDEX_PKT_CRC_CONFIG:0x%02X\n", Si446xCmd.GET_PROPERTY.DATA[0]);
            si446x_get_property(SI446X_PROP_GRP_ID_PKT, 4, SI446X_PROP_GRP_INDEX_PKT_CRC_SEED);
            rt_kprintf("SI446X_PROP_GRP_INDEX_PKT_CRC_SEED:0x%02X%02X%02X%02X\n", Si446xCmd.GET_PROPERTY.DATA[0], Si446xCmd.GET_PROPERTY.DATA[1], Si446xCmd.GET_PROPERTY.DATA[2], Si446xCmd.GET_PROPERTY.DATA[3]);
//            si446x_set_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_FIELD_2_CRC_CONFIG, 0x15);
            si446x_get_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_FIELD_2_CRC_CONFIG);
            rt_kprintf("SI446X_PROP_GRP_INDEX_PKT_FIELD_2_CRC_CONFIG:0x%02X\n", Si446xCmd.GET_PROPERTY.DATA[0]);
//            si446x_set_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_RX_FIELD_2_CRC_CONFIG, 0x15);
            si446x_get_property(SI446X_PROP_GRP_ID_PKT, 1, SI446X_PROP_GRP_INDEX_PKT_RX_FIELD_2_CRC_CONFIG);
            rt_kprintf("SI446X_PROP_GRP_INDEX_PKT_RX_FIELD_2_CRC_CONFIG:0x%02X\n", Si446xCmd.GET_PROPERTY.DATA[0]);
        } else if (!strcmp(operator, "test")) {
            if (argc < 3) {
                rt_kprintf("Usage: %s.\n", help_info[CMD_TEST_INDEX]);
                return;
            }
#define RADIO_TEST_RETRY_TIMES         20
            size_t times = atoi(argv[2]);
            while (times--) {
                /* ������������� */
                srand(rt_tick_get());
                /* ��ȡ 1-126 ֮��������� */
                size_t len = rand() % 126 + 1;
                uint8_t retry = 0;
                uint8_t *str = (uint8_t *) rt_malloc(len);
                if (str) {
                    memset(str, len, len);
                    while(!NETSTACK_RADIO.channel_clear() && retry ++ < RADIO_TEST_RETRY_TIMES) {
                        rt_thread_delay(random_rand() % 10);
                    }
                    if (retry < RADIO_TEST_RETRY_TIMES) {
                        radio_pkt_send_sync(str, len);
                    }
                    rt_free(str);
#if !RAW_RADIO_TEST
                    rt_kprintf("remaining times: %9ld \r", times);
#endif
                    /* ��ȡ 0-19 ����֮������ʱ�� */
                    size_t time = random_rand() % 20 + 0;
                    rt_thread_delay(rt_tick_from_millisecond(time));
                } else {
                    rt_kprintf("no memery to use!\r\n");
                }
            }
            /* ��ʾ��� */
            rt_kprintf("test finished��please use <si4463 stat> view test results...\n");
        } else {
            rt_kprintf("Usage:\n");
            for (i = 0; i < sizeof(help_info) / sizeof(char*); i++) {
                rt_kprintf("%s\n", help_info[i]);
            }
            rt_kprintf("\n");
        }
    }
}
MSH_CMD_EXPORT(si4463, si4463 test. More information by run 'si4463')
#endif
