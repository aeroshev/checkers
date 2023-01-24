/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

/*il_printf.h"


int main()
{
    init_platform();

    print("Hello World\n\r");

    cleanup_platform();
    return 0;
}
*/

#include <stdio.h>
#include "xparameters.h"
#include "platform.h"
#include "xil_printf.h"
#include "xaxidma.h"
#include "xscugic.h"
#include "xil_exception.h"

#define DMA_DEV_ID XPAR_AXIDMA_0_DEVICE_ID
#define INTC_DEVICE_ID XPAR_SCUGIC_SINGLE_DEVICE_ID
#define FRAME_SIZE 2*480*641
#define FRAME_PARTS 40
#define FRAME_PART_SIZE FRAME_SIZE/FRAME_PARTS
#define INTC XScuGic
#define INTC_HANDLER XScuGic_InterruptHandler
#define TX_INTR_ID XPAR_FABRIC_AXI_DMA_0_MM2S_INTROUT_INTR
#define VGA_INTR_ID 62
#define FRAME_0_BASE (XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x00100000)
#define FRAME_1_BASE (XPAR_PS7_DDR_0_S_AXI_BASEADDR + 0x00500000)

#define WHITE 0xFFFFFF
#define BLACK 0x000000
#define RED 0xFF0000
#define BLUE 0x0000FF


const int VGA_WEIGHT = 640;
const int VGA_HEIGHT = 480;

const int BOARD_SIZE = 8;

const int CELL_PIXELS_X = VGA_HEIGHT / BOARD_SIZE;
const int CELL_PIXELS_Y = VGA_WEIGHT / BOARD_SIZE;

const int CHECKER_R_X = CELL_PIXELS_X / 2;
const int CHECKER_R_Y = CELL_PIXELS_Y / 2;

struct checker {
    int color;
    int num;
    int pos_x;
    int pos_y;
} typedef checker;

XAxiDma AxiDma; /* Instance of the XAxiDma */
INTC Intc; /* Instance of the Interrupt Controller */

/* ���������� ����������, ������������ � ����������� ���������� */
int frame_part = 0;
int new_frame_ready = 0;
u8 corent_frame = 1;

int SetupIntrSystem(INTC * IntcInstancePtr, XAxiDma * AxiDmaPtr, u16 TxIntrId, u16 RxIntrId);
void TxIntrHandler(void *Callback);
void Data_reqwestIntrHandler(void *Callback);
void render_checker(u16 *, int, int);
void render_map(u16 *, int (*)[BOARD_SIZE]);
void move_left(int *, int *, int, int (*)[BOARD_SIZE]);
void move_right(int *, int *, int, int (*)[BOARD_SIZE]);

int main()
{
	int 	Status;
	int 	Index;
	u16* 	Frame_Base_Addr;
	u16 	*frame0 = (u16*)FRAME_0_BASE,
			*frame1 = (u16*)FRAME_1_BASE;


	init_platform();
	xil_printf("Start Checkers\n\r");

	XAxiDma_Config *CfgPtr;

	CfgPtr = XAxiDma_LookupConfig(DMA_DEV_ID);

	if (!CfgPtr) {
		xil_printf("No config found for %d\r\n", DMA_DEV_ID);
		return XST_FAILURE;
	}

	Status = XAxiDma_CfgInitialize(&AxiDma, CfgPtr);
	if (Status != XST_SUCCESS) {
		xil_printf("Initialization failed %d\r\n", Status);
		return XST_FAILURE;
	}
	if(XAxiDma_HasSg(&AxiDma)){
		xil_printf("Device configured as SG mode \r\n");
		return XST_FAILURE;
	}

	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DMA_TO_DEVICE);

	/*��������� ���������� �����*/
	for(Index = 0; Index < FRAME_SIZE/2; Index++){
		frame0[Index] = frame1[Index] = 0x0;
	}
	frame0[0] = frame1[0] = 0x2000;

	Xil_DCacheFlushRange((UINTPTR)frame0, FRAME_SIZE);
	Xil_DCacheFlushRange((UINTPTR)frame1, FRAME_SIZE);

	Status = SetupIntrSystem(&Intc, &AxiDma, TX_INTR_ID, VGA_INTR_ID);

	if (Status != XST_SUCCESS) {
		xil_printf("Failed intr setup\r\n");
		return XST_FAILURE;
	}
	/* Disable all interrupts before setup */
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrDisable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	/* Enable all interrupts */
	XAxiDma_IntrEnable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrEnable(&AxiDma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

	xil_printf("Init game\n\r");
	int game[BOARD_SIZE][BOARD_SIZE];
	for(int i = 0; i < BOARD_SIZE; i++) {
		if (i % 2 == 0) {
			game[0][i] = 1;
			game[1][i] = 0;
			game[BOARD_SIZE - 1][i] = 2;
			game[BOARD_SIZE - 2][i] = 0;
		} else {
			game[0][i] = 0;
			game[1][i] = 1;
			game[BOARD_SIZE - 1][i] = 0;
			game[BOARD_SIZE - 2][i] = 2;
		}
		for (int j = 2; j < BOARD_SIZE - 2; j++) {
			game[j][i] = 0;
		}
	}
	char key;
    int coord_x = 1;
    int coord_y = 1;

	xil_printf("Run\n\r");
	while(1){

		Frame_Base_Addr = !corent_frame ? frame1 : frame0 ;

		render_map(Frame_Base_Addr, game);


		Frame_Base_Addr[0] = Frame_Base_Addr[0]|0x2000;
		new_frame_ready = 1;
		while(new_frame_ready);
		Xil_DCacheFlushRange((UINTPTR)Frame_Base_Addr, FRAME_SIZE);


        key = inbyte();
        xil_printf("Echo: %s\n\r", &key);
        switch (key)
        {
            case 'l':
                move_left(&coord_x, &coord_y, 1, game);
                break;r
            case 'r':
                move_right(&coord_x, &coord_y, 1, game);
                break;
        }

	}
	cleanup_platform();
	return 0;
}

int SetupIntrSystem(INTC * IntcInstancePtr,
			XAxiDma * AxiDmaPtr, u16 TxIntrId, u16 Data_reqwestIntrId)
{
	int Status;
	XScuGic_Config *IntcConfig;

	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig,
				IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XScuGic_SetPriorityTriggerType(IntcInstancePtr, TxIntrId, 0xA0, 0x3);
	XScuGic_SetPriorityTriggerType(IntcInstancePtr, Data_reqwestIntrId, 0xA0, 0x0);

	Status = XScuGic_Connect(IntcInstancePtr, TxIntrId,
				(Xil_InterruptHandler)TxIntrHandler,
				AxiDmaPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}
	Status = XScuGic_Connect(IntcInstancePtr, Data_reqwestIntrId,
				(Xil_InterruptHandler)Data_reqwestIntrHandler,
				AxiDmaPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	XScuGic_Enable(IntcInstancePtr, TxIntrId);
	XScuGic_Enable(IntcInstancePtr, Data_reqwestIntrId);

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)INTC_HANDLER,
				(void *)IntcInstancePtr);

	Xil_ExceptionEnable();
	return 0;
}
void TxIntrHandler(void *Callback)
{
	XScuGic_Enable(&Intc, VGA_INTR_ID);
	XAxiDma *AxiDmaInst = (XAxiDma *)Callback;
	u32 IrqStatus = XAxiDma_IntrGetIrq(AxiDmaInst, XAXIDMA_DMA_TO_DEVICE);
	XAxiDma_IntrAckIrq(AxiDmaInst, IrqStatus, XAXIDMA_DMA_TO_DEVICE);
	if (frame_part == 0 && new_frame_ready == 1){
		corent_frame = !corent_frame;
		new_frame_ready = 0;
	}
}
void Data_reqwestIntrHandler(void *Callback)
{
	u8* Frame_Base_Addr = corent_frame ? (u8 *)FRAME_1_BASE : (u8 *)FRAME_0_BASE;

	XAxiDma_SimpleTransfer( &AxiDma,
			(UINTPTR) (Frame_Base_Addr + frame_part * FRAME_PART_SIZE),
			FRAME_PART_SIZE,
			XAXIDMA_DMA_TO_DEVICE);
	XScuGic_Disable(&Intc, VGA_INTR_ID);
	frame_part = (frame_part + 1)%FRAME_PARTS;
}

void move_left(int *pos_x, int *pos_y, int side, int game[BOARD_SIZE][BOARD_SIZE]) {
    if (side == 1) {
        // Upper half
        if (*pos_x + 1 < BOARD_SIZE && *pos_y - 1 >= 0) {
            game[*pos_x][*pos_y] = 0;
            ++*pos_x;
            --*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    } else {
        if (pos_x - 1 >= 0 && pos_y - 1 >= 0) {
            game[*pos_x][*pos_y] = 0;
            --*pos_x;
            --*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    }
}

void move_right(int *pos_x, int *pos_y, int side, int game[BOARD_SIZE][BOARD_SIZE]) {
    if (side == 1) {
        if (*pos_x + 1 < BOARD_SIZE && *pos_y + 1 < BOARD_SIZE) {
            game[*pos_x][*pos_y] = 0;
            ++*pos_x;
            ++*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    } else {
        if (*pos_x - 1 >= 0 && *pos_y + 1 < BOARD_SIZE) {
            game[*pos_x][*pos_y] = 0;
            --*pos_x;
            ++*pos_y;
            game[*pos_x][*pos_y] = side;
        }
    }
}

void render_checker(u16 *Frame, int pos_x, int pos_y) {
	int start_x = (CELL_PIXELS_X * pos_x);
	int start_y = (CELL_PIXELS_Y * pos_y);

    int center_x = start_x + CELL_PIXELS_X / 2;
    int center_y = start_y + CELL_PIXELS_Y / 2;

    int Index;
    int cicrle_x, circle_y;
    for (int x = 0; x < CELL_PIXELS_X; x++) {
    	for (int y = 0; y < CELL_PIXELS_Y; y++) {
    		Index = VGA_WEIGHT * (start_x + x) + (start_y + y);
    		cicrle_x = start_x + x - center_x;
    		circle_y = start_y + y - center_y;
        	if ((cicrle_x * cicrle_x + circle_y * circle_y) < 400) {
        		Frame[Index] = 0xF;
        	}
    	}
    }
}

void render_map(u16 *Frame, int game[BOARD_SIZE][BOARD_SIZE]) {
    // Render board
    int accum_x = 0, accum_y = 0;
    int pos_x = 0, pos_y = 0;
    for (int i = 0; i < VGA_HEIGHT; i++) {
        accum_x++;
        if (accum_x == CELL_PIXELS_X) {
            accum_x = 0;
            pos_x++;
        }
        pos_y = 0;
        for (int j = 0; j < VGA_WEIGHT; j++) {
            accum_y++;
            if (accum_y == CELL_PIXELS_Y) {
                accum_y = 0;
                pos_y++;
            }

            // Render cells
            if (pos_x % 2 == 0) {
                if (pos_y % 2 == 0) {
                	Frame[i * VGA_WEIGHT + j] = 0xFFFF;
                } else {
                	Frame[i * VGA_WEIGHT + j] = 0x0;
                }
            } else {
                if (pos_y % 2 == 1) {
                	Frame[i * VGA_WEIGHT + j] = 0xFFFF;
                } else {
                	Frame[i * VGA_WEIGHT + j] = 0x0;
                }
            }

            if (game[pos_x][pos_y] == 1) {
                render_checker(Frame, pos_x, pos_y);
            }
            if (game[pos_x][pos_y] == 2) {
                render_checker(Frame, pos_x, pos_y);
            }
        }
    }
}
