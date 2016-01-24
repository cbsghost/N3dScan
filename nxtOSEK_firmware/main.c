#include "kernel.h"
#include "kernel_id.h"
#include "ecrobot_interface.h"
#include "ecrobot_base.h"
#include "DistNx_API.h"
#include "smile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* OSEK declarations */
DeclareCounter(SysTimerCnt);
DeclareTask(TaskInteraction);
DeclareTask(TaskGUI);
DeclareTask(TaskBluetooth);
DeclareTask(TaskSnanner);

/*
 * a wav file can be accessed by using following macros:
 * E.g lego_mindstorms_nxt.wav
 * EXTERNAL_WAV_DATA(file name without extension); <- This is external declarations
 * WAV_DATA_START(file name without extension)     <- start address of a wav file
 * WAV_DATA_END(file name without extension)       <- end address of a wav file
 * WAV_DATA_SIZE(file name without extension)      <- size of a wav file 
 */
EXTERNAL_WAV_DATA(boot_snd);

//EXTERNAL_BMP_DATA(proc_circle);
//EXTERNAL_BMP_DATA(proc_circle_highlight);
//EXTERNAL_BMP_DATA(icon_done);
//EXTERNAL_BMP_DATA(icon_failed);
//EXTERNAL_BMP_DATA(icon_bt_wait);
//EXTERNAL_BMP_DATA(icon_bt_conn);

#define QUEUE_BUF 600
#define BT_PIN_CODE "0277343562"
const char nxt_ver[] = "N3dScan=v0.0.1";

typedef struct _N3dData {
	short int count_xy;
	short int count_z;
	short int dist;
} N3dData;
N3dData queue[QUEUE_BUF];

typedef struct _N3dParam {
	short int z_offset;
	short int z_prec;
	unsigned char dist_raw;
} N3dParam;
N3dParam scan_param;

char bt_done_flag;
char cancel_flag;
unsigned char status;
char start_scan;
char scan_state;
short int scan_progress;
short int obj_height;
short int latest_queue_count;

short int shutdown_count = 0;

void ecrobot_device_initialize(void)
{
	static char init = 0;
	if (!init) {
		show_splash_screen();
		status = 0x02;
		scan_state = 0;
		start_scan = 0;
		cancel_flag = 0;
		ecrobot_sound_wav(WAV_DATA_START(boot_snd), 
			(U32)WAV_DATA_SIZE(boot_snd), -1, 50);
		DistNx_Init(NXT_PORT_S1, DIST_MODEL_SHORT);
		
		nxt_motor_set_speed(NXT_PORT_A, 80, 0);
		
		nxt_motor_set_speed(NXT_PORT_B, -100, 1);
		nxt_motor_set_speed(NXT_PORT_C, -80, 1);
		while (!ecrobot_get_touch_sensor(NXT_PORT_S3));
		nxt_motor_set_speed(NXT_PORT_B, 0, 1);
		nxt_motor_set_speed(NXT_PORT_C, 60, 1);
		nxt_motor_set_count(NXT_PORT_B, 0);
		nxt_motor_set_speed(NXT_PORT_B, 60, 1);
		while (nxt_motor_get_count(NXT_PORT_B) < 630);
		while (DistNx_Distance(NXT_PORT_S1) < 80) {
			if (nxt_motor_get_count(NXT_PORT_B) >= 1000) {
				break;
			}
		}
		nxt_motor_set_speed(NXT_PORT_B, 0, 1);
		nxt_motor_set_speed(NXT_PORT_C, 0, 1);
		
		char bt_name[16];
		unsigned char bt_addr[7];
		ecrobot_get_bt_device_address(bt_addr);
		sprintf(bt_name, "N3dScan-%02X%02X%02X", bt_addr[3], bt_addr[4], bt_addr[5]);
		ecrobot_set_bt_device_name(bt_name);
		//ecrobot_init_bt_slave(BT_PIN_CODE);
		
		DistNx_Term(NXT_PORT_S1);
		nxt_motor_set_speed(NXT_PORT_A, 0, 0);
		status = 0x00;
		init = 1;
	}
}

void ecrobot_device_terminate()
{
	static char term = 0;
	if (!term) {
		DistNx_Init(NXT_PORT_S1, DIST_MODEL_SHORT);
		nxt_motor_set_speed(NXT_PORT_A, 100, 0);
		ecrobot_term_bt_connection();
		
		nxt_motor_set_speed(NXT_PORT_B, -80, 1);
		nxt_motor_set_speed(NXT_PORT_C, -80, 1);
		while (!ecrobot_get_touch_sensor(NXT_PORT_S3));
		nxt_motor_set_speed(NXT_PORT_B, 0, 1);
		nxt_motor_set_speed(NXT_PORT_C, 60, 1);
		nxt_motor_set_count(NXT_PORT_B, 0);
		nxt_motor_set_speed(NXT_PORT_B, 60, 1);
		while (nxt_motor_get_count(NXT_PORT_B) < 630);
		while (DistNx_Distance(NXT_PORT_S1) < 80) {
			if (nxt_motor_get_count(NXT_PORT_B) >= 1000) {
				break;
			}
		}
		nxt_motor_set_speed(NXT_PORT_B, 0, 1);
		nxt_motor_set_speed(NXT_PORT_C, 0, 1);
		
		nxt_motor_set_speed(NXT_PORT_A, 0, 0);
		DistNx_Term(NXT_PORT_S1);
		term = 1;
	}
	
}

/* LEJOS OSEK hook to be invoked from an ISR in category 2 */
void user_1ms_isr_type2(void)
{
	//SignalCounter(SysTimerCnt);
	StatusType ercd;

	ercd = SignalCounter(SysTimerCnt); /* Increment OSEK Alarm Counter */ 
	if (ercd != E_OK) {
		ShutdownOS(ercd);
	}
}


TASK(TaskInteraction)
{
	unsigned char btn_counter = 0;
	unsigned char buttons;
	buttons = ecrobot_get_button_state();

	if (start_scan) {
		if (buttons & BTN_GRAY_RECT) {
			cancel_flag = 1;
			start_scan = 0;
		}
	} else {
		while (buttons & BTN_RIGHT) {
			ecrobot_sound_tone(262, 50, 70);
			nxt_motor_set_speed(NXT_PORT_C, -70, 1);
			nxt_motor_set_speed(NXT_PORT_A, 70, 0);
			shutdown_count = 0;
			buttons = ecrobot_get_button_state();
		}
		nxt_motor_set_speed(NXT_PORT_A, 0, 0);
		
		while (buttons & BTN_LEFT) {
			ecrobot_sound_tone(262, 50, 70);
			nxt_motor_set_speed(NXT_PORT_C, 70, 1);
			nxt_motor_set_speed(NXT_PORT_A, 70, 0);
			shutdown_count = 0;
			buttons = ecrobot_get_button_state();
		}
		nxt_motor_set_speed(NXT_PORT_A, 0, 0);
		
		nxt_motor_set_speed(NXT_PORT_C, 0, 1);
		
		if (buttons & BTN_ORANGE_RECT) {
			shutdown_count = 0;
			systick_wait_ms(200);
			while ((buttons & BTN_ORANGE_RECT) && (buttons & BTN_GRAY_RECT)) {
				btn_counter ++;
				systick_wait_ms(50);
				if (btn_counter >= 96) {
					ecrobot_sound_tone(1000, 1000, 100);
					systick_wait_ms(1000);
					buttons = ecrobot_get_button_state();
					if ((buttons & BTN_ORANGE_RECT) && (buttons & BTN_GRAY_RECT)) {
						exec_NXT_BIOS();
					}
				}
				buttons = ecrobot_get_button_state();
			}
		}
		
		while (buttons & BTN_GRAY_RECT) {
			shutdown_count = 0;
			btn_counter ++;
			systick_wait_ms(50);
			if (btn_counter >= 40) {
				ecrobot_shutdown_NXT();
			}
			buttons = ecrobot_get_button_state();
		}
	}
	
	TerminateTask();
}

TASK(TaskGUI)
{
	static char bd_name[16];
	static unsigned char title_line_data[100];
	//static unsigned char proc_circle_data[8], proc_circle_highlight_data[8];
	static char init = 0, ani_count = 0;
	//static unsigned char icon_data[200];
	
	if (!init) {
		ecrobot_get_bt_device_name(bd_name);
		memset(title_line_data, 0x01, sizeof(title_line_data));
		//ecrobot_bmp2lcd(BMP_DATA_START(proc_circle), proc_circle_data, 8, 8);
		//ecrobot_bmp2lcd(BMP_DATA_START(proc_circle_highlight), proc_circle_highlight_data, 8, 8);
		
		init = 1;
	}
	//memset(title_line_data, 0x01, sizeof(title_line_data));
	display_clear(0);
	display_goto_xy(1, 0);
	display_string("#");
	display_goto_xy(2, 0);
	display_string(bd_name);
	display_bitmap_copy(title_line_data, 100, 1, 0, 1);
	
	if (start_scan) {
		nxt_motor_set_speed(NXT_PORT_A, 50, 0);
		display_goto_xy(1, 7);
		display_string("Scanning...");
		if ((status & 0x08) == 0x00) {
			display_goto_xy(12, 7);
			display_int(scan_progress, 3);
			display_goto_xy(15, 7);
			display_string("%");
		}
		/*if (ani_count == 0) {
			display_bitmap_copy(proc_circle_data, 8, 1, 32, 4);
			display_bitmap_copy(proc_circle_data, 8, 1, 46, 4);
			display_bitmap_copy(proc_circle_data, 8, 1, 60, 4);
		} else if (ani_count == 1) {
			display_bitmap_copy(proc_circle_highlight_data, 8, 1, 32, 4);
			display_bitmap_copy(proc_circle_data, 8, 1, 46, 4);
			display_bitmap_copy(proc_circle_data, 8, 1, 60, 4);
		} else if (ani_count == 2) {
			display_bitmap_copy(proc_circle_data, 8, 1, 32, 4);
			display_bitmap_copy(proc_circle_highlight_data, 8, 1, 46, 4);
			display_bitmap_copy(proc_circle_data, 8, 1, 60, 4);
		} else if (ani_count == 3) {
			display_bitmap_copy(proc_circle_data, 8, 1, 32, 4);
			display_bitmap_copy(proc_circle_data, 8, 1, 46, 4);
			display_bitmap_copy(proc_circle_highlight_data, 8, 1, 60, 4);
		}
		ani_count ++;
		if (ani_count >= 4) {
			ani_count = 0;
		}*/
		ani_count ++;
		if (ani_count > 1) {
			ani_count = 0;
		}
	} else {
		ani_count = 0;
		if (cancel_flag == 1) {
			nxt_motor_set_speed(NXT_PORT_A, 100, 0);
			display_goto_xy(2, 7);
			display_string("User canceled");
		} else if (cancel_flag == 2) {
			nxt_motor_set_speed(NXT_PORT_A, 100, 0);
			display_goto_xy(0, 7);
		} else {
			nxt_motor_set_speed(NXT_PORT_A, 0, 0);
			if (ecrobot_get_bt_status() != BT_STREAM) {
				display_goto_xy(0, 7);
				display_string("Waiting for host");
			} else {
				display_goto_xy(1, 7);
				display_string("Host connected");
			}
		}
		//systick_wait_ms(10);
		
	}
	if (!ani_count) {
		display_bitmap_copy(smile_data, 40, 5, 30, 2);
	}
	
	display_update();
	TerminateTask();
}

TASK(TaskBluetooth)
{
	static unsigned char send_buf[32];
	short int ret_c, seq;
	unsigned char rx_err = 0;
	
	if (ecrobot_get_bt_status() != BT_STREAM){
		if (start_scan) {
			start_scan = 0;
			cancel_flag = 2;
			rx_err = 1;
		}
		ecrobot_init_bt_slave(BT_PIN_CODE);
	} else {
		ret_c = ecrobot_read_bt(send_buf, 0, 1);
		if (ret_c == 1) {
			if (send_buf[0] == 0x00 || send_buf[0] == 0xFF) {
				rx_err = 1;
			} else {
				if (ecrobot_read_bt(&(send_buf[1]), 0, send_buf[0]) != send_buf[0] - 1) {
					rx_err = 1;
				} else {
					switch (send_buf[1]) {
					case 0x01: // info and version
						if (send_buf[0] != 2) {
							rx_err = 1;
						} else {
							strcpy((char *)(&(send_buf[2])), nxt_ver);
							send_buf[0] = strlen((char *)(&(send_buf[2]))) + 3;
						}
						break;
						
					case 0x02: // status
						if (send_buf[0] != 2) {
							rx_err = 1;
						} else {
							send_buf[2] = status;
							memcpy(&(send_buf[3]), &(scan_param.z_offset), 2);
							memcpy(&(send_buf[5]), &(scan_param.z_prec), 2);
							memcpy(&(send_buf[7]), &(scan_param.dist_raw), 1);
							send_buf[0] = 8;
						}
						break;
						
					case 0x10: // start scan
						if (send_buf[0] != 7) {
							rx_err = 1;
						} else {
							memcpy(&(scan_param.z_offset), &(send_buf[2]), 2);
							memcpy(&(scan_param.z_prec), &(send_buf[4]), 2);
							memcpy(&(scan_param.dist_raw), &(send_buf[6]), 1);
							start_scan = 1;
							
							send_buf[2] = 0x01; // ok
							send_buf[0] = 3;
						}
						break;
						
					case 0x20: //cancel scan
						if (send_buf[0] != 2) {
							rx_err = 1;
						} else {
							cancel_flag = 1;
							start_scan = 0;
							
							send_buf[2] = 0x01; // ok
							send_buf[0] = 3;
						}
						break;
						
					case 0x30: // get scan data
						if (send_buf[0] != 4) {
							rx_err = 1;
						} else {
							
							memcpy(&seq, &(send_buf[2]), 2);
							if (seq >= latest_queue_count) {
								bt_done_flag = 1;
								send_buf[4] = (status | 0x10);
								send_buf[0] = 5;
							} else if (seq == latest_queue_count - 1) {
								bt_done_flag = 1;
								send_buf[4] = status;
								memcpy(&(send_buf[5]), &(queue[seq].count_xy), 2);
								memcpy(&(send_buf[7]), &(queue[seq].count_z), 2);
								memcpy(&(send_buf[9]), &(queue[seq].dist), 2);
								send_buf[0] = 11;
							} else {
								send_buf[4] = status;
								memcpy(&(send_buf[5]), &(queue[seq].count_xy), 2);
								memcpy(&(send_buf[7]), &(queue[seq].count_z), 2);
								memcpy(&(send_buf[9]), &(queue[seq].dist), 2);
								send_buf[0] = 11;
							}
						}
						break;
						
					case 0x31: // get object height
						if (send_buf[0] != 2) {
							rx_err = 1;
						} else {
							send_buf[2] = status;
							if (start_scan) {
								memcpy(&(send_buf[3]), &obj_height, 2);
							} else {
								memset(&(send_buf[3]), 0x00, 2);
							}
							send_buf[0] = 5;
						}
						break;
						
					default:
						rx_err = 1;
					}
					
				}
			}
			if (rx_err) {
				send_buf[1] = 0xFF;
				send_buf[0] = 2;
			}
			ecrobot_send_bt(send_buf, 0, send_buf[0]);
		}
		
	}
	
	if (rx_err) {
		ecrobot_read_bt(send_buf, 0, 32);
	}
	
	TerminateTask();
}

TASK(TaskScanner)
{
	static short int xy_speed;
	static unsigned short queue_idx;
	static short int z_count, xy_cur_count, xy_last_count, xy_start_point, xy_null_count;	
	
	if (start_scan == 0 && scan_state != 0) {
		nxt_motor_set_speed(NXT_PORT_B, -100, 1);
		nxt_motor_set_speed(NXT_PORT_C, 0, 1);
		while (!ecrobot_get_touch_sensor(NXT_PORT_S3));
		nxt_motor_set_speed(NXT_PORT_B, 0, 1);
		nxt_motor_set_count(NXT_PORT_B, 0);
		nxt_motor_set_speed(NXT_PORT_B, 60, 1);
		while (nxt_motor_get_count(NXT_PORT_B) < 630);
		while (DistNx_Distance(NXT_PORT_S1) < 80);
		nxt_motor_set_speed(NXT_PORT_B, 0, 1);
		DistNx_Term(NXT_PORT_S1);
		cancel_flag = 0;
		scan_state = 0;
	}
	
	switch (scan_state) {
	case 0:
		status = (status & 0xF0);
		if (start_scan) {
			scan_state ++;
		}
		break;
		
	case 1:
		status = (status & 0xF0) | 0x0B;
		DistNx_Init(NXT_PORT_S1, DIST_MODEL_SHORT);
		xy_speed = 80;
		xy_cur_count = 0;
		xy_last_count = 0;
		xy_start_point = 0;
		xy_null_count = 0;
		queue_idx = 0;
		latest_queue_count = 0;
		scan_progress = 0;
		scan_state ++;
		//break;
		
	case 2:
		status = (status & 0xF0) | 0x0B;
		
		nxt_motor_set_speed(NXT_PORT_B, 100, 1);
		nxt_motor_set_speed(NXT_PORT_C, xy_speed, 1);
		if (ecrobot_get_touch_sensor(NXT_PORT_S2)) {
			xy_speed *= 10;
			nxt_motor_set_speed(NXT_PORT_B, 0, 1);
			nxt_motor_set_count(NXT_PORT_B, 0);
			nxt_motor_set_count(NXT_PORT_C, 0);
			DistNx_Distance(NXT_PORT_S1);
			scan_state ++;
		}
		break;
		
	case 3:
		status = (status & 0xF0) | 0x0B;
		nxt_motor_set_speed(NXT_PORT_B, -100, 1);
		nxt_motor_set_speed(NXT_PORT_C, xy_speed/10, 1);
		
		xy_cur_count = nxt_motor_get_count(NXT_PORT_C);
		nxt_motor_set_count(NXT_PORT_C, 0);
		if (xy_cur_count < 4) {
			xy_speed ++;
		} else if (xy_cur_count > 5) {
			xy_speed --;
		}
		
		if (DistNx_Distance(NXT_PORT_S1) < 210) {
			nxt_motor_set_count(NXT_PORT_B, 0);
			scan_state ++;
		}
		break;
		
	case 4:
		status = (status & 0xF0) | 0x0B;
		nxt_motor_set_speed(NXT_PORT_B, -100, 1);
		nxt_motor_set_speed(NXT_PORT_C, xy_speed/10, 1);
		
		xy_cur_count = nxt_motor_get_count(NXT_PORT_C);
		nxt_motor_set_count(NXT_PORT_C, 0);
		if (xy_cur_count < 4) {
			xy_speed ++;
		} else if (xy_cur_count > 5) {
			xy_speed --;
		}
		
		if (ecrobot_get_touch_sensor(NXT_PORT_S3)) {
			xy_speed /= 10;
			nxt_motor_set_speed(NXT_PORT_B, 0, 1);
			obj_height = nxt_motor_get_count(NXT_PORT_B) * -1;
			nxt_motor_set_count(NXT_PORT_B, 0);
			scan_state ++;
		}
		break;
		
	case 5:
		status = (status & 0xF0) | 0x0B;
		nxt_motor_set_speed(NXT_PORT_B, 60, 1);
		nxt_motor_set_speed(NXT_PORT_C, xy_speed, 1);
		if (nxt_motor_get_count(NXT_PORT_B) >= 630) {
			scan_state ++;
		}
		break;
		
	case 6:
		status = (status & 0xF0) | 0x0B;
		nxt_motor_set_speed(NXT_PORT_B, 60, 1);
		if (DistNx_Distance(NXT_PORT_S1) >= 80) {
			obj_height -= nxt_motor_get_count(NXT_PORT_B);
			if (obj_height < 1) {
				obj_height = 1;
			}
			nxt_motor_set_count(NXT_PORT_B, 0);
			scan_state ++;
		}
		break;
		
	case 7:
		status = (status & 0xF0) | 0x03;
		nxt_motor_set_speed(NXT_PORT_B, 60, 1);
		if (nxt_motor_get_count(NXT_PORT_B) >= scan_param.z_offset) {
			nxt_motor_set_speed(NXT_PORT_B, 0, 1);
			z_count = nxt_motor_get_count(NXT_PORT_B);
			//obj_height -= z_count;
			nxt_motor_set_count(NXT_PORT_C, 0);
			scan_progress = 0;
			DistNx_Distance(NXT_PORT_S1);
			scan_state ++;
		}
		break;
		
	case 8:
		status = (status & 0xF0) | 0x01;
		nxt_motor_set_speed(NXT_PORT_C, xy_speed, 1);
		
		if (scan_param.dist_raw) {
			queue[queue_idx].dist = DistNx_Voltage(NXT_PORT_S1);
			if (queue[queue_idx].dist < 550) {
				xy_null_count ++;
			}
		} else {
			queue[queue_idx].dist = DistNx_Distance(NXT_PORT_S1);
			if (queue[queue_idx].dist > 165) {
				xy_null_count ++;
			}
		}
		queue[queue_idx].count_xy = nxt_motor_get_count(NXT_PORT_C);
		
		queue[queue_idx].count_z = z_count;
		
		if (queue_idx > 0 && (latest_queue_count >= QUEUE_BUF || queue[queue_idx].count_xy >= xy_start_point + 1800)) {
			if (xy_null_count >= latest_queue_count - 11 || ecrobot_get_touch_sensor(NXT_PORT_S2)) {
				start_scan = 0;
			}
			xy_null_count = 0;
			scan_state ++;
		}
		
		if (queue_idx == 0 || (queue[queue_idx].count_xy > queue[queue_idx - 1].count_xy)) {
			latest_queue_count ++;
			queue_idx ++;
			bt_done_flag = 0;
		}
		
		break;
		
	case 9:
		status = (status & 0xF0) | 0x03;
		nxt_motor_set_speed(NXT_PORT_B, 60, 1);
		if (nxt_motor_get_count(NXT_PORT_B) >= scan_param.z_prec + z_count) {
			nxt_motor_set_speed(NXT_PORT_B, 0, 1);
			scan_progress = z_count * 100 / obj_height;
			if (scan_progress > 100) {
				scan_progress = 100;
			}
			scan_state ++;
		}
		break;
		
	case 10:
		status = (status & 0xF0) | 0x03;
		if (bt_done_flag){
			z_count = nxt_motor_get_count(NXT_PORT_B);
			latest_queue_count = 0;
			queue_idx = 0;
			scan_state -= 2;
			if (ecrobot_get_touch_sensor(NXT_PORT_S3)) {
				start_scan = 0;
			}
			xy_start_point = nxt_motor_get_count(NXT_PORT_C);
			while (xy_start_point >= 1800) {
				xy_start_point -= 1800;
				nxt_motor_set_count(NXT_PORT_C, xy_start_point);
			}
			DistNx_Distance(NXT_PORT_S1);
		}
		break;
		
	default:
		status = (status & 0xF0) | 0x07;
		start_scan = 0;
	}
	TerminateTask();
}

TASK(TaskPowerManagement)
{
	
	if (start_scan) {
		shutdown_count = 0;
	} else {
		shutdown_count ++;
	}
	if (shutdown_count > 1500) {
		ecrobot_shutdown_NXT();
	}
	TerminateTask();
}
