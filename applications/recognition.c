/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: LGPL-3.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-03-29     Jianjia Ma   first implementation
 *
 * Notes:
 * This is a keyword spotting example using NNoM
 * 
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "rtthread.h"

#include "nnom.h"
#include "weights.h"

#include "mfcc.h"
#include "board.h"
#include <drv_audio_in.h>
//
rt_mutex_t mfcc_buf_mutex;

// NNoM model
nnom_model_t *model;

// 10 labels-1
//const char label_name[][10] =  {"yes", "no", "up", "down", "left", "right", "on", "off", "stop", "go", "unknow"};

// 10 labels-2
//const char label_name[][10] =  {"marvin", "sheila", "yes", "no", "left", "right", "forward", "backward", "stop", "go", "unknow"};

// 34 labels
const char label_name[][10] =  {"backward", "bed", "bird", "cat", "dog", "down", "eight","five", "follow", "forward",
                      "four", "go", "happy", "house", "learn", "left", "marvin", "nine", "no", "off", "on", "one", "right",
                      "seven", "sheila", "six", "stop", "three", "tree", "two", "up", "visual", "yes", "zero", "unknow"};

// configuration
#define SAMP_FREQ 16000
#define AUDIO_FRAME_LEN (512) //31.25ms * 16000hz = 512, // FFT (windows size must be 2 power n)

mfcc_t * mfcc;
int32_t dma_audio_buffer[AUDIO_FRAME_LEN*2];
int16_t audio_buffer_16bit[(int)(AUDIO_FRAME_LEN*1.5)]; // an easy method for 50% overlapping


//the mfcc feature for kws
#define MFCC_LEN			(63)
#define MFCC_COEFFS_FIRST	(1)		// ignore the mfcc feature before this number
#define MFCC_COEFFS_LEN 	(13)    // the total coefficient to calculate
#define MFCC_COEFFS    	    (MFCC_COEFFS_LEN-MFCC_COEFFS_FIRST)

#define MFCC_FEAT_SIZE 	(MFCC_LEN * MFCC_COEFFS)
int8_t mfcc_features[MFCC_LEN][MFCC_COEFFS];	 // ring buffer
int8_t mfcc_features_seq[MFCC_LEN][MFCC_COEFFS]; // sequencial buffer for neural network input. 
uint32_t mfcc_feat_index = 0;

// msh debugging controls
bool is_print_abs_mean = false; // to print the mean of absolute value of the mfcc_features_seq[][]
bool is_print_mfcc  = false;    // to print the raw mfcc features at each update 
void Error_Handler()
{
	rt_kprintf("error\n");
}

static TIM_HandleTypeDef s_TimerInstance = { 
    .Instance = TIM2
};
void us_timer_enable()
{
    __TIM2_CLK_ENABLE();
    s_TimerInstance.Init.Prescaler = 83;
    s_TimerInstance.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_TimerInstance.Init.Period = 0xffffffff;
    s_TimerInstance.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    s_TimerInstance.Init.RepetitionCounter = 0;
    HAL_TIM_Base_Init(&s_TimerInstance);
    HAL_TIM_Base_Start(&s_TimerInstance);
}

uint32_t us_timer_get()
{
	return __HAL_TIM_GET_COUNTER(&s_TimerInstance);
}

static int32_t abs_mean(int8_t *p, size_t size)
{
	int64_t sum = 0;
	for(size_t i = 0; i<size; i++)
	{
		if(p[i] < 0)
			sum+=-p[i];
		else
			sum += p[i];
	}
	return sum/size;
}

void thread_kws_serv(void *p)
{
	#define SaturaLH(N, L, H) (((N)<(L))?(L):(((N)>(H))?(H):(N)))
	uint32_t evt;
	int32_t *p_raw_audio;
	uint32_t time;
	static int16_t raw_in_buffer[AUDIO_FRAME_LEN];
	int offset = 0;
	rt_mailbox_t mic_mb;
	// calculate 13 coefficient, use number #2~13 coefficient. discard #1
	mfcc = mfcc_create(MFCC_COEFFS_LEN, MFCC_COEFFS_FIRST, AUDIO_FRAME_LEN, 5, 0.97f); 
	mic_mb = mic_start_record(32);
	while(1)
	{
		// wait for event and check which buffer is filled

		if(rt_mb_recv(mic_mb, (rt_ubase_t *)&p_raw_audio, RT_WAITING_FOREVER) != RT_EOK)
        {
            continue;
        }
		pcm_stereo_to_mono(p_raw_audio, 32, &raw_in_buffer[offset]);
		offset += 16;
		rt_mp_free(p_raw_audio);
		if(offset < AUDIO_FRAME_LEN)
		{
			continue;
		}
		// p_raw_audio = raw_in_buffer;
		offset = 0;
		memcpy(audio_buffer_16bit, &audio_buffer_16bit[AUDIO_FRAME_LEN], (AUDIO_FRAME_LEN/2)*sizeof(int16_t));
		memcpy(&audio_buffer_16bit[AUDIO_FRAME_LEN/2], raw_in_buffer, sizeof raw_in_buffer);
		// memory move
		// audio buffer = | 256 byte old data |   256 byte new data 1 | 256 byte new data 2 | 
		//                         ^------------------------------------------|
// 		memcpy(audio_buffer_16bit, &audio_buffer_16bit[AUDIO_FRAME_LEN], (AUDIO_FRAME_LEN/2)*sizeof(int16_t));
		
// 		// convert it to 16 bit. 
// 		// volume*4
// 		for(int i = 0; i < AUDIO_FRAME_LEN; i++)
// 		{
// 			audio_buffer_16bit[AUDIO_FRAME_LEN/2+i] = SaturaLH((p_raw_audio[i] >> 8)*1, -32768, 32767);
			
// //			if(audio_buffer_16bit[AUDIO_FRAME_LEN/2+i] == -32768 || audio_buffer_16bit[AUDIO_FRAME_LEN/2+i] == 32767)
// //				printf(">| %d\n", rt_tick_get());
// 		}
		
		// MFCC
		// do the first mfcc with half old data(256) and half new data(256)
		// then do the second mfcc with all new data(512). 
		// take mfcc buffer
		
		rt_mutex_take(mfcc_buf_mutex, RT_WAITING_FOREVER);
		for(int i=0; i<2; i++)
		{
			mfcc_compute(mfcc, &audio_buffer_16bit[i*AUDIO_FRAME_LEN/2], mfcc_features[mfcc_feat_index]);
			
			// debug only, to print mfcc data on console
			if(is_print_mfcc)
			{
				for(int i=0; i<MFCC_COEFFS; i++)
					rt_kprintf("%d ",  mfcc_features[mfcc_feat_index][i]);
				rt_kprintf("\n");
			}
			
			mfcc_feat_index++;
			if(mfcc_feat_index >= MFCC_LEN)
				mfcc_feat_index = 0;
		}
		
		// release mfcc buffer
		rt_mutex_release(mfcc_buf_mutex);
	}
}



int reco_main(void)
{
	uint32_t last_mfcc_index = 0; 
	uint32_t label;
	rt_tick_t last_time = 0;
	float prob;
	uint8_t priority = RT_THREAD_PRIORITY_MAX-2;
	
	us_timer_enable();
	
	// create thread sync 
	mfcc_buf_mutex = rt_mutex_create("mfcc_buf", RT_IPC_FLAG_FIFO);
	
	// create and compile the model 
	model = nnom_model_create();
	
	// change to lowest priority, avoid blocking shell
	rt_thread_control(rt_thread_self(), RT_THREAD_CTRL_CHANGE_PRIORITY, &priority);
	
	// create kws workers
	rt_thread_startup(rt_thread_create("kws_serv", thread_kws_serv, RT_NULL, 1024, 5, 50));
	
	
	while(1)
	{
		// mfcc wait for new data, then copy
		while(last_mfcc_index == mfcc_feat_index)
			rt_thread_delay(1);
		
		// copy mfcc ring buffer to sequance buffer. 
		rt_mutex_take(mfcc_buf_mutex, RT_WAITING_FOREVER);
		last_mfcc_index = mfcc_feat_index;
		uint32_t len_first = MFCC_FEAT_SIZE - mfcc_feat_index * MFCC_COEFFS;
		uint32_t len_second = mfcc_feat_index * MFCC_COEFFS;
		memcpy(&mfcc_features_seq[0][0], &mfcc_features[0][0] + len_second,  len_first);
		memcpy(&mfcc_features_seq[0][0] + len_first, &mfcc_features[0][0], len_second);
		rt_mutex_release(mfcc_buf_mutex);
		
		// debug only, to print the abs mean of mfcc output. use to adjust the dec bit (shifting)
		// of the mfcc computing. 
		if(is_print_abs_mean)
			rt_kprintf("abs mean:%d\n", abs_mean((int8_t*)mfcc_features_seq, MFCC_FEAT_SIZE));
		
		// ML
		memcpy(nnom_input_data, mfcc_features_seq, MFCC_FEAT_SIZE);
		nnom_predic(model, &label, &prob);
		
		// output
		//if(prob > 0.8f && label != 10 && rt_tick_get() - last_time > RT_TICK_PER_SECOND)
		if(prob > 0.5f)
		{
			last_time = rt_tick_get();
			rt_kprintf("%s : %d%%\n", (char*)&label_name[label], (int)(prob * 100));
		}
	}
}

// Msh functions

#ifdef RT_USING_FINSH
#include <finsh.h>
void nn_stat()
{
	model_stat(model);
	rt_kprintf("Total Memory cost (Network and NNoM): %d\n", nnom_mem_stat());
}
MSH_CMD_EXPORT(nn_stat, print nn model);

void kws_mfcc()
{
	is_print_mfcc = !is_print_mfcc;
}
MSH_CMD_EXPORT(kws_mfcc, print the raw mfcc values);

void kws_mean()
{
	is_print_abs_mean = !is_print_abs_mean;
}
MSH_CMD_EXPORT(kws_mean, print the abs mean value of mfcc output);

#endif















