/*
 * This file is part of the ART-6LoWPAN Radio Library.
 *
 * Copyright (c) 2017, Armink, <armink.ztl@gmail.com>
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
 * Function: Radio configuration.
 * Created on: 2017-10-15
 */

#ifndef _RADIO_CFG_H_
#define _RADIO_CFG_H_

/* �Ƿ����ô���ָʾ�� LED */
#define SI446X_TRANS_LED_ENABLE        1
/* ʹ�õ� SI446x �Ƿ�Ϊ B1 �汾������Ϊ C2 �汾 */
#define SI446X_USING_REV_B1            1
/* �Ƿ�����ԭʼ��ʽ����Ƶ������� */
#define RAW_RADIO_TEST                 0
/* Radio �����־������� */
#define RADIO_LOG_LVL                  ELOG_LVL_INFO

#endif /* _RADIO_CFG_H_ */
