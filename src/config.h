/* CONFIG_H_ */
#ifndef CONFIG_H_
#define CONFIG_H_

#define SAMPLE_BUFFER_LENGTH 1024
#define NUM_TAPS 32

#define USE_ADC_DATA
#define INSTRUCTION_BENCH

#define ENABLE_GRAPH_NORMALIZATION 1U

#define GRAPH_NORMALIZATION_FACTOR 100.0f

#define FFT_UNFILTERED_MULTIPLIER (1.0f/100.0f)
#define FFT_FILTERED_MULTIPLIER (1.0f/1000.0f)
#define REALTIME_UNFILTERED_MULTIPLIER 4.0f
#define REALTIME_FILTERED_MULTIPLIER 5.0f


#define FFT_UNFILTERED_OFFSET 0
#define FFT_FILTERED_OFFSET 0
#define REALTIME_UNFILTERED_OFFSET 0
#define REALTIME_FILTERED_OFFSET 330.0f

#define DATA_REFRESH_PERIOD 33U

#endif /* CONFIG_H_ */
