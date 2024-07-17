#include "fft_thread.h"
#include "config.h"
#include "SEGGER_RTT/SEGGER_RTT.h"
#include "lv_data_viewer.h"


/* Application error */
#define APP_ASSERT(a)   \
{                       \
    if ((a))            \
    {                   \
        /* Break */     \
        __BKPT(0);      \
    }                   \
}

#define APP_PRINT(fn_, ...)      SEGGER_RTT_printf (SEGGER_INDEX,(fn_), ##__VA_ARGS__);
#define SEGGER_INDEX            (0)

extern lv_chart_series_t* series_fft_unfiltered; //unfiltered freq domain
extern lv_chart_series_t* series_fft_filtered; //filtered freq domain
extern lv_chart_series_t* series_analog_unfiltered; //unfiltered time domain
extern lv_chart_series_t* series_analog_filtered; //filtered time domain

extern lv_obj_t* chart_time;
extern lv_obj_t* chart_freq;

int16_t real_fft_mag_f32(float32_t* input, float32_t* output, float32_t* magnitude);
void fir_f32(float32_t* input, float32_t* output);

extern float32_t real_input_signal_f32[SAMPLE_BUFFER_LENGTH];
extern float32_t test_input_signal_f32[SAMPLE_BUFFER_LENGTH];
extern float32_t unfiltered_FFT_output_f32[SAMPLE_BUFFER_LENGTH];
extern float32_t unfiltered_FFT_mag_f32[SAMPLE_BUFFER_LENGTH/2];
extern float32_t filtered_output_f32[SAMPLE_BUFFER_LENGTH];
extern float32_t filtered_FFT_output_f32[SAMPLE_BUFFER_LENGTH];
extern float32_t filtered_FFT_mag_f32[SAMPLE_BUFFER_LENGTH/2];
extern float32_t temp_buffer[SAMPLE_BUFFER_LENGTH];
extern uint32_t raw_adc_buffer[SAMPLE_BUFFER_LENGTH];

extern float32_t real_input_signal_copy_f32[SAMPLE_BUFFER_LENGTH];
extern float32_t filtered_output_copy_f32[SAMPLE_BUFFER_LENGTH];

static volatile uint16_t buffer_index = 0;
static volatile bool buffer_full = false;
static const float32_t vref = 3.3f;

const float32_t fir_coeffs_f32[NUM_TAPS] = {
    -0.00063047f, -0.00181857f, -0.00256194f, -0.00158749f, 0.00236951f, 0.00833250f, 0.01180361f, 0.00675930f,
    -0.00917451f, -0.02973091f, -0.03981645f, -0.02230165f, 0.03102797f, 0.11114350f, 0.19245540f, 0.24373020f,
    0.24373020f, 0.19245540f, 0.11114350f, 0.03102797f, -0.02230165f, -0.03981645f, -0.02973091f, -0.00917451f,
    0.00675930f, 0.01180361f, 0.00833250f, 0.00236951f, -0.00158749f, -0.00256194f, -0.00181857f, -0.00063047f
};





void g_timer1_cb(timer_callback_args_t *p_args)
{
    FSP_PARAMETER_NOT_USED(p_args);

    R_ADC_ScanStart(&g_adc0_ctrl);
}

void g_adc0_cb(adc_callback_args_t *p_args)
{
    if(ADC_EVENT_SCAN_COMPLETE == p_args->event) {
        uint32_t data;
        fsp_err_t err = R_ADC_Read32(&g_adc0_ctrl, ADC_CHANNEL_0, &data);

        if (err != FSP_SUCCESS) {
            APP_PRINT("ADC ERR CODE: %d\n", err);
        }

        if(buffer_index < SAMPLE_BUFFER_LENGTH) {
            raw_adc_buffer[buffer_index++] = data;

            if(buffer_index >= SAMPLE_BUFFER_LENGTH) {
                buffer_full = true;
            }
        }
    }
}


/* FFT Thread entry function */
/* pvParameters contains TaskHandle_t */
void fft_thread_entry(void *pvParameters)
{
    FSP_PARAMETER_NOT_USED (pvParameters);

    /* TODO: add your own code here */

    #if BSP_TZ_SECURE_BUILD

    /* Enter non-secure code */
    R_BSP_NonSecureEnter();
    #endif
    int16_t fft_status = ARM_MATH_SUCCESS;
    int16_t filtered_fft_status = ARM_MATH_SUCCESS;

    fsp_err_t err;

    #ifdef USE_ADC_DATA
    err = R_ADC_Open(&g_adc0_ctrl, &g_adc0_cfg);
    APP_ASSERT(err);
    err = R_ADC_ScanCfg(&g_adc0_ctrl, &g_adc0_channel_cfg);
    APP_ASSERT(err);
    err = R_ADC_ScanStart(&g_adc0_ctrl);
    APP_ASSERT(err);

    err = R_GPT_Open(&g_timer1_ctrl, &g_timer1_cfg);
    APP_ASSERT(err);
    err = R_GPT_Start(&g_timer1_ctrl);
    APP_ASSERT(err);
    #else
    buffer_full = true;
    #endif

    vTaskDelay (250);

    volatile uint16_t my_index;
    float32_t sample;

    while (1)
    {
        if(buffer_full)
        {
            #ifdef USE_ADC_DATA
            // Process ADC data buffer
            for (my_index = 0; my_index < SAMPLE_BUFFER_LENGTH; my_index++)
            {
                sample = ((float32_t)raw_adc_buffer[my_index] / 4095.0f) * vref;
//                real_input_signal_f32[my_index] = sample - (vref / 2.0f);
                real_input_signal_f32[my_index] = sample*(100.0f/vref); //scale the value to 0-100
            }

            __disable_irq();

            // Reset buffer index and flag
            buffer_index = 0;
            buffer_full = false;

            __enable_irq();
            #endif

            #ifdef INSTRUCTION_BENCH
            // Instruction bench
            uint32_t ts_0 = 0;
            uint32_t ts_fft = 0;
            uint32_t ts_filter = 0;
            uint32_t ts_fft2 = 0;
            uint32_t fft_cycle = 0;
            uint32_t filter_cycle = 0;
            uint32_t fft2_cycle = 0;
            uint32_t total_cycle = 0;

            ts_0 = DWT->CYCCNT;

            R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_12, BSP_IO_LEVEL_HIGH);
            #endif

            // FFT modifies the source data so need to copy to temp buffer
            #ifdef USE_ADC_DATA
            memcpy(real_input_signal_copy_f32, real_input_signal_f32, SAMPLE_BUFFER_LENGTH);
            #else
            memcpy(temp_buffer, test_input_signal_f32, SAMPLE_BUFFER_LENGTH);
            #endif

            // Generate unfiltered FFT from input signal
            fft_status = real_fft_mag_f32(real_input_signal_f32, unfiltered_FFT_output_f32, unfiltered_FFT_mag_f32);
            if (fft_status != ARM_MATH_SUCCESS) {
                APP_PRINT("Error in FFT computation: %d\n", fft_status);
            }
            #ifdef INSTRUCTION_BENCH
            ts_fft = DWT->CYCCNT;
            #endif

            // Filter input signal
            fir_f32(real_input_signal_f32, filtered_output_f32);
            #ifdef INSTRUCTION_BENCH
            ts_filter = DWT->CYCCNT;
            #endif

            // FFT modifies the source data so need to copy to temp buffer
            #ifdef USE_ADC_DATA
            memcpy(filtered_output_copy_f32, filtered_output_f32, SAMPLE_BUFFER_LENGTH);
            #else
            memcpy(temp_buffer, test_input_signal_f32, SAMPLE_BUFFER_LENGTH);
            #endif

            // FFT filtered signal
            filtered_fft_status = real_fft_mag_f32(filtered_output_f32, filtered_FFT_output_f32, filtered_FFT_mag_f32);
            if (filtered_fft_status != ARM_MATH_SUCCESS) {
                APP_PRINT("Error in filtered FFT computation: %d\n", filtered_fft_status);
            }
            #ifdef INSTRUCTION_BENCH
            ts_fft2 = DWT->CYCCNT;

            R_IOPORT_PinWrite(&g_ioport_ctrl, BSP_IO_PORT_08_PIN_12, BSP_IO_LEVEL_LOW);

            fft_cycle = ts_fft > ts_0? ts_fft - ts_0 : ~(ts_0 - ts_fft);
            filter_cycle = ts_filter > ts_fft? ts_filter - ts_fft : ~(ts_fft - ts_filter);
            fft2_cycle = ts_fft2 > ts_filter? ts_fft2 - ts_filter : ~(ts_filter - ts_fft2);
            total_cycle = ts_fft2 > ts_0? ts_fft2 - ts_0 : ~(ts_0 - ts_fft2);

            APP_PRINT("FFT input signal clock cycle: %d\n", fft_cycle);
            APP_PRINT("Filter input signal clock cycle: %d\n", filter_cycle);
            APP_PRINT("FFT filtered signal clock cycle: %d\n", fft2_cycle);
            APP_PRINT("Total clock cycle: %d\n", total_cycle);
            #endif

            //process and copy over the analog data
            for( my_index = 0; my_index < SAMPLE_BUFFER_LENGTH; my_index++ )
            {
                series_analog_unfiltered->y_points[my_index] = (int32_t)(REALTIME_UNFILTERED_MULTIPLIER*real_input_signal_copy_f32[my_index]);
                series_analog_filtered->y_points[my_index] = (int32_t)(REALTIME_FILTERED_MULTIPLIER*(filtered_output_copy_f32[my_index] + 330));
            }

            //process and copy over the fft data
            for( my_index = 0; my_index < SAMPLE_BUFFER_LENGTH/2; my_index++ )
            {
                series_fft_unfiltered->y_points[my_index] = (int32_t)(FFT_UNFILTERED_MULTIPLIER*unfiltered_FFT_mag_f32[my_index]);
                series_fft_filtered->y_points[my_index] = (int32_t)(FFT_FILTERED_MULTIPLIER*filtered_FFT_mag_f32[my_index]);
            }

            APP_PRINT("Place holder\n", total_cycle);
            vTaskDelay (1);
        }
        else
        {
            vTaskDelay (33); //run every 33ms for 30 updates per second.
        }
    }
}


int16_t real_fft_mag_f32(float32_t* input, float32_t* output, float32_t* magnitude)
{
    uint16_t fftSize = 1024;
    uint8_t ifftFlag = 0;
    arm_rfft_fast_instance_f32 varInstRfft32;
    arm_status status;

    status = ARM_MATH_SUCCESS;

    status = arm_rfft_fast_init_f32(&varInstRfft32, fftSize);

    /* Process the data through the CFFT/CIFFT module */
    arm_rfft_fast_f32(&varInstRfft32, input, output, ifftFlag);
    arm_cmplx_mag_f32(output, magnitude, fftSize/2);

    return status;
}

void fir_f32(float32_t* input, float32_t* output)
{
    uint32_t blockSize = 32;
    uint32_t numBlocks = SAMPLE_BUFFER_LENGTH/blockSize;
    float32_t firStateF32[NUM_TAPS + 2 * blockSize - 1];
    arm_fir_instance_f32 firInstance;

    /* Call FIR init function to initialize the instance structure. */
    arm_fir_init_f32(&firInstance, NUM_TAPS, (float32_t *)&fir_coeffs_f32[0], &firStateF32[0], blockSize);

    for(uint32_t i = 0; i < numBlocks; i++)
    {
      arm_fir_f32(&firInstance, input + (i * blockSize), output + (i * blockSize), blockSize);
    }
}
