
#include "sdk_default_config.h"
#include "ci_log.h"
#include "ci130x_uart.h"
#include "ci_log_config.h"
#include "ci130x_gpio.h"
#include "ci130x_dpmu.h"
#include "ci130x_timer.h"
#include "FreeRTOS.h"
#include "system_msg_deal.h"
#include "baudrate_calibrate.h"

#if UART_BAUDRATE_CALIBRATE

#if 0

volatile uint32_t timer_array_count = 0; //数组下标
volatile uint32_t *timer_array = NULL;

//串口波特率校准处理函数
void uart_calibrate_irq_handle(void)
{
    uint32_t temp = 0;
    uint8_t level_status = 0;
   // mprintf("GPIO_USRT_Calibrate_Irq_Handle is called\r\n");
    timer_get_count(UART_CALIBRATE_TIMER, (unsigned int *)(&temp));
    timer_stop(UART_CALIBRATE_TIMER);
    timer_set_count(UART_CALIBRATE_TIMER, get_apb_clk());  //定时器重置超时为1S
    timer_start(UART_CALIBRATE_TIMER);
    level_status = gpio_get_input_level_single(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
    if (1 == level_status)
    {
        gpio_irq_trigger_config(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN, down_edges_trigger);
    }
    else
    {
        gpio_irq_trigger_config(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN, up_edges_trigger);
    }
    if(temp != 0)
    {
       timer_array[timer_array_count] = temp;
       timer_array_count++;
    }
    
}


void baudrate_calibrate_io_irq_proc()
{
    // for (int i = 0; i < 8; i++)
    {
        if (gpio_get_irq_mask_status_single(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN))
        {
            gpio_clear_irq_single(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
            if(timer_array_count < UART_CALIBRATE_ARRAY_SIZE)
            {
               uart_calibrate_irq_handle();
            }
            //ci_loginfo(LOG_GPIO_DRIVER,"GPIO%d Pin%d IRQ\n", gpio_port_index, i);
        }
    }
}


//串口自动校准初始化函数
void uart_calibrate_init(void)
{
    //初始化串口波特率校准定时器
    timer_init_t uart_init_t;
    scu_set_device_gate(UART_CALIBRATE_TIMER_BASE, ENABLE);
    scu_set_device_reset(UART_CALIBRATE_TIMER_BASE);
    scu_set_device_reset_release(UART_CALIBRATE_TIMER_BASE);
    uart_init_t.count = get_apb_clk();  //1s
    uart_init_t.div = timer_clk_div_0;
    uart_init_t.width = timer_iqr_width_f;
    uart_init_t.mode = timer_count_mode_single;
    timer_init(UART_CALIBRATE_TIMER, uart_init_t);
    timer_start(UART_CALIBRATE_TIMER);
    //初始化串口波特校准GPIO
      /* IR receive IO */
    gpio_irq_mask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
    scu_set_device_gate(UART_CALIBRATE_IO_BASE, ENABLE);
    dpmu_set_io_pull(UART_CALIBRATE_IO_PAD, DPMU_IO_PULL_DISABLE);
    dpmu_set_io_reuse(UART_CALIBRATE_IO_PAD, FIRST_FUNCTION); /*gpio function*/
    dpmu_set_io_direction(UART_CALIBRATE_IO_PAD, DPMU_IO_DIRECTION_INPUT);
    dpmu_set_io_slew_rate(UART_CALIBRATE_IO_PIN,DPMU_IO_SLEW_RATE_FAST);
    dpmu_set_io_driver_strength(UART_CALIBRATE_IO_PIN,DPMU_IO_DRIVER_STRENGTH_3);

    gpio_set_input_mode(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
    gpio_irq_unmask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
    gpio_irq_trigger_config(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN, both_edges_trigger);
    gpio_clear_irq_single(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
    eclic_irq_enable(UART_CALIBRATE_IRQ);
    gpio_irq_mask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
    // while(timer_array == NULL)
    // {
    //     timer_array = pvPortMalloc(UART_CALIBRATE_ARRAY_SIZE * sizeof(uint32_t));
    // }
}

//串口波特率自适应计算
void uart_calibrate_calc(void)
{
    uint32_t buffer[UART_CALIBRATE_ARRAY_SIZE];
    float ftemp = 0.0;
    int32_t baud_rate_modulus = 0;
    volatile int timeout_count = 0;

    timer_array = buffer;
    timer_array_count = 0;
    uart_calibrate_init();
    // 设置IO中断处理函数
    int32_t old_vector = __eclic_irq_get_vector(UART_CALIBRATE_IRQ);
    __eclic_irq_set_vector(UART_CALIBRATE_IRQ, baudrate_calibrate_io_irq_proc);
    //wait baud_rate_modulus
    while (timeout_count < 200)
    {
        //mprintf("The calculated baud rate is %d\r\n",baud_rate_modulus);
        if((int32_t)ftemp!= 0)
        {
            // mprintf("The calculated baud rate is %d\r\n",p );
            gpio_irq_mask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
            break;
        }
        if(timer_array_count > 15) 
        {
            gpio_irq_mask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
            // for(int8_t i = 0; i < timer_array_count; i++)
            // {
            //     mprintf("c:%d, val:%ld", i, timer_array[i]);
            //     if(i < timer_array_count - 1 )
            //     {
            //         mprintf(" abv:%d, tov:%ld",(get_apb_clk()- timer_array[i]), get_apb_clk());
            //     }
            //     mprintf("\r\n\n");
            // }
            uint32_t array_total_val = 0;
            uint32_t calc_temp_count = 0;
            uint32_t average_tick_val = 0;
            for(int i = 0 ; i < timer_array_count; i++)   //取均值
            {
                    if((get_apb_clk() - timer_array[i] <  get_apb_clk()/UART_CALIBRATE_FIX_BAUDRATE*(1 + UART_CALIBRATE_MAX_ERR_PROPORITION)) && \
                       (get_apb_clk() - timer_array[i] >  get_apb_clk()/UART_CALIBRATE_FIX_BAUDRATE*(1 - UART_CALIBRATE_MAX_ERR_PROPORITION)))
                    {
                        array_total_val += timer_array[i];
                        calc_temp_count++;
                    }
            }
            if(calc_temp_count < 8)  //正确count值小于8 bit,清缓存重新计算
            {
                timer_array_count = 0;
                memset(timer_array, 0, 64);
                gpio_irq_unmask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
                continue;
            }
            mprintf("array_total_val = %ld, calc_temp_count = %d\r\n", array_total_val, calc_temp_count);    
            average_tick_val = array_total_val/calc_temp_count;    
            mprintf("average_tick_val = %ld\r\n", average_tick_val);
            ftemp = (float)((float)get_apb_clk()/(float)(get_apb_clk() - average_tick_val) );
            mprintf("\nftemp:%ld, ftemp*1000:%ld \n", (uint32_t)ftemp, (uint32_t)ftemp*1000);
            
            extern float g_freq_correct_factor;
            float factor = ftemp/UART_CALIBRATE_FIX_BAUDRATE;
            if (factor > 0.92 && factor < 1.08)
            {
                g_freq_correct_factor = factor;
            }
            
            gpio_irq_mask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);

            #if CONFIG_CI_LOG_UART    //日志串口
            UartPollingSenddone(CONFIG_CI_LOG_UART_PORT);            
            UARTPollingConfig((UART_TypeDef*)CONFIG_CI_LOG_UART_PORT, UART_BaudRate921600);
            #endif
            #if 0
            baud_rate_modulus = (int32_t)(ftemp - UART_CALIBRATE_FIX_BAUDRATE);
            mprintf("baud_rate_modulus =  %ld, error rate = %d\r\n",baud_rate_modulus, baud_rate_modulus*1000/UART_CALIBRATE_FIX_BAUDRATE);
            // 921600/9600*baud_rate_modulus +921600
            // mprintf("921600:%ld %ld\r\n", 921600/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus + 921600, ((921600*1000)/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus)/1000 +921600);
            // mprintf("600000:%ld %ld\r\n", 600000/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus + 600000, ((600000*1000)/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus)/1000 +600000);
            // mprintf("230400:%ld %ld\r\n", 230400/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus + 230400, ((230400*1000)/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus)/1000 +230400);
            // mprintf("115200:%ld %ld\r\n", 115200/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus + 115200, ((115200*1000)/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus)/1000 +115200);
            // mprintf("654321:%ld %ld\r\n", 654321/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus + 654321, ((654321*1000)/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus)/1000 +654321);
            // mprintf("2000000:%ld %ld\r\n", 2000000/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus + 2000000, ((2000000*1000)/UART_CALIBRATE_FIX_BAUDRATE*baud_rate_modulus)/1000 +2000000);
            #if MSG_COM_USE_UART_EN   //协议串口
            flag = 0;
            timeout = 100000;
            do{
                flag = UART_FLAGSTAT(UART_PROTOCOL_NUMBER, UART_TXFE);
                timeout--;
            }while((!flag) && timeout > 0);
                UARTPollingConfig((UART_TypeDef*)UART_PROTOCOL_NUMBER, ((UART_PROTOCOL_BAUDRATE*1000)/9600*baud_rate_modulus)/1000 +UART_PROTOCOL_BAUDRATE);
            #endif
            // vPortFree(timer_array);
            #endif
        }
        else
        {
            gpio_irq_unmask(UART_CALIBRATE_IO_BASE, UART_CALIBRATE_IO_PIN);
        }
        //mprintf("Wait baud rate (%d)\r\n", timer_array_count); 
        vTaskDelay(10);
        timeout_count++;
    }
    __eclic_irq_set_vector(UART_CALIBRATE_IRQ, old_vector);     // 恢复中断向量
}


#else


// static const uint8_t sync_req[] = {'B', 'C'};

static const float factors[] = {
    0.90, 0.92, 0.940, 0.960, 0.980, 1.0, 1.02, 1.04, 1.06, 1.08, 1.10 };
// static const float factors[] = {1.0, 1.015, 0.955, 0.97, 0.985, 1.0, 1.015, 1.03, 1.045};


// #define SYNC_REQ_LENGTH     (sizeof(sync_req) / sizeof(sync_req[0]))
#define FACTOR_NUM      (sizeof(factors)/sizeof(factors[0]))

#ifndef BAUD_CALIBRATE_MAX_WAIT_TIME
#define BAUD_CALIBRATE_MAX_WAIT_TIME        60
#endif

typedef enum{
    BC_IDLE,
    BC_START,
    BC_STOP,
    BC_SEND_REQ,
    BC_WAIT_ACK,
    BC_CHANGE_BAUDRATE,
    BC_CALC,
}status_t;

typedef struct {
    status_t status;
    send_sync_req_func_t send_sync_req_func;
    int32_t factor_index;
    UART_TypeDef *UARTx; 
    uint32_t std_baudrate;
    uint32_t idle_count;
    uint8_t test_flag[FACTOR_NUM];
    uint8_t ack_flag;
    uint8_t ack_index;
    uint8_t old_baudrate_index;
    uint8_t frist_sync;
}bc_info_t;

volatile static bc_info_t bc_info;
static void task_baudrate_calibrate(void *p_arg);


void baudrate_calibrate_init(UART_TypeDef *UARTx, uint32_t baudrate, send_sync_req_func_t send_sync_req_func)
{
    bc_info.status = BC_IDLE;
    bc_info.std_baudrate = baudrate;
    bc_info.UARTx = UARTx;    
    bc_info.ack_index = 0;
    bc_info.idle_count = 0;
    bc_info.old_baudrate_index = sizeof(factors)/sizeof(factors[0])/2;
    bc_info.send_sync_req_func = send_sync_req_func;
    bc_info.frist_sync = 1;    
    xTaskCreate(task_baudrate_calibrate, "baudrate_sync", 196, NULL, 4, NULL);
}

void baudrate_calibrate_start(void)
{
    if (bc_info.status == BC_IDLE)
    {
        bc_info.status = BC_START;
    }
}

void baudrate_calibrate_stop(void)
{
    bc_info.status = BC_STOP;
    while(bc_info.status != BC_IDLE)
    {
        vTaskDelay(pdMS_TO_TICKS(2));
    }
    if (BAUDRATE_SYNC_PERIOD > 1000)
    {
        bc_info.idle_count = (BAUDRATE_SYNC_PERIOD-1000)/10;
    }
}

void baudrate_calibrate_wait_finish(void)
{
    while(bc_info.status != BC_IDLE)
    {
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

// 语音协议模块收到同步ACK的时候，通过调用此接口告知已收到ACK。
void baudrate_calibrate_set_ack()
{
    bc_info.ack_flag = 1;
}


void task_baudrate_calibrate(void *p_arg)
{
    int wait_count = 0;
    while(1)
    {
        switch (bc_info.status)
        {
        case BC_START:
            if (bc_info.status != BC_STOP)
            {
                bc_info.ack_index = 0;
                bc_info.factor_index = -1;
                bc_info.status = BC_SEND_REQ;
                mprintf("send @ index:%d\n", bc_info.old_baudrate_index);
            }
            break;
        case BC_STOP:
            bc_info.idle_count = 0;
            bc_info.status = BC_IDLE;
            break;
        case BC_SEND_REQ:
            bc_info.ack_flag = 0;
            bc_info.ack_index = 0;
            if (bc_info.send_sync_req_func)
            {
                bc_info.send_sync_req_func();
            }

            //发送成功，转入等待ACK的状态
            if (bc_info.status != BC_STOP)
            {
                bc_info.status = BC_WAIT_ACK;
                wait_count = 0;                
            }            
            break;
        case BC_WAIT_ACK:
            if (bc_info.ack_flag)
            {
                // 收到回应
                if (bc_info.factor_index == -1)
                {
                    // 当前波特率正确，不用调整。
                    bc_info.status = BC_STOP;
                    if (bc_info.frist_sync)
                    {
                        bc_info.frist_sync = 0;
                        sys_power_on_hook();
                    }
                    mprintf("keep baudrate %d\n", (uint32_t)(bc_info.std_baudrate*factors[bc_info.old_baudrate_index]));
                }
                else
                {
                    bc_info.test_flag[bc_info.factor_index] = 1;
                    if (bc_info.factor_index < FACTOR_NUM)
                    {
                        bc_info.status = BC_CHANGE_BAUDRATE;
                    }
                    else
                    {
                        bc_info.status = BC_CALC;
                    }
                }
            }
            else
            {
                if (wait_count < BAUD_CALIBRATE_MAX_WAIT_TIME/6)
                {
                    vTaskDelay(pdMS_TO_TICKS(6));
                    wait_count++;
                }
                else
                {
                    if (bc_info.factor_index < (int32_t)FACTOR_NUM)
                    {
                        // if (bc_info.factor_index >= 0)
                        {
                            bc_info.test_flag[bc_info.factor_index] = 0;
                            bc_info.status = BC_CHANGE_BAUDRATE;        // 等待ACK超时，执行切换波特率操作
                        }
                    }
                    else
                    {
                        bc_info.status = BC_CALC;
                    }
                }
            }
            break;
        case BC_CHANGE_BAUDRATE:
            bc_info.factor_index++;
            if (bc_info.factor_index < FACTOR_NUM)
            {
                UARTInterruptConfig(bc_info.UARTx, bc_info.std_baudrate*factors[bc_info.factor_index]);
                bc_info.status = BC_SEND_REQ;
                mprintf("send @ index:%d\n", bc_info.factor_index);
            }
            else
            {
                bc_info.status = BC_CALC;
            }
            break;
        case BC_CALC:
        {
            int low = 0;
            int high = FACTOR_NUM - 1;
            while(low < high)
            {
                if (bc_info.test_flag[low] == 0)
                {
                    low++;
                }
                else
                {
                    if(bc_info.test_flag[high] == 0)
                    {
                        high--;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if (low < high || (low == high && bc_info.test_flag[low]))
            {
                UARTInterruptConfig(bc_info.UARTx, bc_info.std_baudrate*factors[(low+high)/2]);
                bc_info.old_baudrate_index = (low+high)/2;
                mprintf("change baudrate to %d\n", (uint32_t)(bc_info.std_baudrate*factors[(low+high)/2]));
            }
            else
            {
                UARTInterruptConfig(bc_info.UARTx, bc_info.std_baudrate*factors[bc_info.old_baudrate_index]);
                mprintf("keep baudrate %d\n", (uint32_t)(bc_info.std_baudrate*factors[bc_info.old_baudrate_index]));
            }
            if (bc_info.frist_sync)
            {
                bc_info.frist_sync = 0;
                sys_power_on_hook();
            }
            bc_info.status = BC_STOP;
            break;
        }
        case BC_IDLE:
        default:
            vTaskDelay(pdMS_TO_TICKS(10));
            if (++bc_info.idle_count > BAUDRATE_SYNC_PERIOD/10)
            {
                bc_info.idle_count = 0;
                if (get_wakeup_state() == SYS_STATE_UNWAKEUP)
                {
                    bc_info.status = BC_START;
                }
            }
            break;
        }
    }
}

#endif

#endif

