#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/APT.h>
#include <ctr/GSP.h>
#include <ctr/GX.h>
#include <ctr/HID.h>
#include <ctr/FS.h>
#include <ctr/svc.h>
#include "costable.h"
#define BUTTON_A 1
#define BUTTON_B 2
#define BUTTON_UP 64
#define BUTTON_LEFT 32
#define BUTTON_DOWN 128
#define BUTTON_RIGHT 16
#define BUTTON_X 1024
#define BUTTON_Y 2048
#define BUTTON_L1 512
#define BUTTON_R1 256
#define BUTTON_START 8
#define BUTTON_SELECT 4
u8* gspHeap;
u32* gxCmdBuf;

u8 currentBuffer;
u8* topLeftFramebuffers[2];

Handle gspEvent, gspSharedMemHandle;

void gspGpuInit()
{
	gspInit();

	GSPGPU_AcquireRight(NULL, 0x0);
	GSPGPU_SetLcdForceBlack(NULL, 0x0);

	//set subscreen to blue
	u32 regData=0x01FF0000;
	GSPGPU_WriteHWRegs(NULL, 0x202A04, &regData, 4);

	//grab main left screen framebuffer addresses
	GSPGPU_ReadHWRegs(NULL, 0x400468, (u32*)&topLeftFramebuffers, 8);

	//convert PA to VA (assuming FB in VRAM)
	topLeftFramebuffers[0]+=0x7000000;
	topLeftFramebuffers[1]+=0x7000000;

	//setup our gsp shared mem section
	u8 threadID;
	svc_createEvent(&gspEvent, 0x0);
	GSPGPU_RegisterInterruptRelayQueue(NULL, gspEvent, 0x1, &gspSharedMemHandle, &threadID);
	svc_mapMemoryBlock(gspSharedMemHandle, 0x10002000, 0x3, 0x10000000);

	//map GSP heap
	svc_controlMemory((u32*)&gspHeap, 0x0, 0x0, 0x2000000, 0x10003, 0x3);

	//wait until we can write stuff to it
	svc_waitSynchronization1(gspEvent, 0x55bcb0);

	//GSP shared mem : 0x2779F000
	gxCmdBuf=(u32*)(0x10002000+0x800+threadID*0x200);

	currentBuffer=0;
}

void gspGpuExit()
{
	GSPGPU_UnregisterInterruptRelayQueue(NULL);

	//unmap GSP shared mem
	svc_unmapMemoryBlock(gspSharedMemHandle, 0x10002000);
	svc_closeHandle(gspSharedMemHandle);
	svc_closeHandle(gspEvent);

	gspExit();

	//free GSP heap
	svc_controlMemory((u32*)&gspHeap, (u32)gspHeap, 0x0, 0x2000000, MEMOP_FREE, 0x0);
}
void draw_pixel(int x, int y, int blue, int green, int red) {
    /* don't fail on attempts to draw outside the screen. */
     u8* bufAdr=&gspHeap[0x46500*currentBuffer];
    if ((x>=400) || (x<0)) {return;}
    if ((y>=240) || (y<0)) {return;}
    u32 v=(y+x*240)*3;
    bufAdr[v]=blue;
    bufAdr[v+1]=green;
    bufAdr[v+2]=red;
}
void draw_fillsquare(int x1, int y1, int x2, int y2, int blue, int green, int red ){
    int i;
    int j;
    for(i=x1;i<x2;i++){
        for(j=y1;j<y2;j++){
            draw_pixel(i,j,blue,green,red);
        }
    }
}
/*
int divide(int n,int d){
    int q = 0;
    int r = n;
    while(r > d+1){
        q = q+1;
        r = r-d;
    }
    return 1;
}
*/
/*void draw_line(int x1, int y1, int m, int blue, int green, int red){
    int b = y1-m*x1;
    int x;
    int y;
    for(x=x1;x<x2;x++){
        y = m*x+b;
        draw_pixel( x,y,blue,green,red);
    }
}
*/
void draw_line_hor(int x1, int x2, int y, int blue, int green, int red){
    int x;
    for(x=x1;x<x2;x++){
        draw_pixel(x,y,blue,green,red);
    }
}
void draw_line_ver(int y1, int y2, int x, int blue, int green, int red){
    int y;
    for(y=y1;y<y2;y++){
        draw_pixel(x,y,blue,green,red);
        }
}
void swapBuffers()
{
	u32 regData;
	GSPGPU_ReadHWRegs(NULL, 0x400478, &regData, 4);
	regData^=1;
	currentBuffer=regData&1;
	GSPGPU_WriteHWRegs(NULL, 0x400478, &regData, 4);
}

void copyBuffer()
{
	//copy topleft FB
	u8 copiedBuffer=currentBuffer^1;
	u8* bufAdr=&gspHeap[0x46500*copiedBuffer];
	GSPGPU_FlushDataCache(NULL, bufAdr, 0x46500);

	GX_RequestDma(gxCmdBuf, (u32*)bufAdr, (u32*)topLeftFramebuffers[copiedBuffer], 0x46500);
}

s32 pcCos(u16 v)
{
	return costable[v&0x1FF];
}

void renderEffect()
{
	u8* bufAdr=&gspHeap[0x46500*currentBuffer];

	if(!currentBuffer)return;
	int i, j;
	for(i=1;i<400;i++)
	{
		for(j=1;j<240;j++)
		{
			u32 v=(j+i*240)*3;
			bufAdr[v]=(pcCos(i)+4096)/32;
			bufAdr[v+1]=0x00;
			bufAdr[v+2]=0xFF*currentBuffer;
		}
	}
}
void init_map(){
int i;
draw_fillsquare(0,0,400,15,135,135,135);//draws the roadlike?
draw_fillsquare(0,15,400,135,84,84,84); //draws the road :D
draw_fillsquare(0,135,400,150,135,135,135);//Draws another roadlike
draw_fillsquare(0,150,400,240,255,0,0); //draws the puddle :D
for(i=2;i<9;i++){
    draw_line_hor(0,400,15*i,255,255,255);
    }

}
void draw_frog(int x, int y){
    draw_fillsquare(15*x,15*y,15*x+15,15*y+15,0,255,0);
}
void draw_car(int posx , int y){
    draw_fillsquare(15*posx,y,15*posx +15, y+15,0,0,255);
}
void draw_logs(int posx , int y){
    draw_fillsquare(posx*15,y,15*posx +45,y+15,66,49,0);
    }
int main()
{
	initSrv();

	aptInit(APPID_APPLICATION);

	gspGpuInit();

	hidInit(NULL);

	Handle fsuHandle;
	srv_getServiceHandle(NULL, &fsuHandle, "fs:USER");
	FSUSER_Initialize(fsuHandle);

	aptSetupEventHandler();
    init_map();
    int i;
    int j;
    int p=4;
    int d=0;
    int q;
    int frogx = 0;
    int frogy = 0;
    int carx[6][9];
    int logx[5][6];

    // Cant use rand.
    //for(d=1;d<6;d++){
    //    while(p>4){
    //        p = rand();
    //    }
    //    carx[d]=26+p;

     for(d=0;d<8;d++){
        carx[0][d]=26;
        if(d<5){
            logx[0][d]=26;
        }
        d++;
    }
    for(d=1;d<8;d++){
        if(d<5){
            logx[0][d]=0;
        }
        carx[0][d]=0;
        d++;
    }
    for(q=0;q<8;q++){

        for(d=1; d<6; d++){
            if(q<5){
                if(d<5){
                    logx[d][q]=26+p+6;
                }
            }
            carx[d][q]=26+p +6;
            p=p+6;
        }
        p=0;
        q++;
    }
    p= 0;
    for(q=1;q<8;q++){

        for(d=1;d<5;d++){
            if(q<5){
                if(d<5){
                    logx[d][q] = p-6;
                }
            }
            carx[d][q]=p-6;
            p=p-6;
        }
        p=0;
        q++;
     }

	while(!aptGetStatus()){
		u32 PAD=hidSharedMem[7];
		if (PAD == BUTTON_UP){
            frogy+= 1;
            }
        else if (PAD == BUTTON_DOWN){
            frogy+= -1;
            }
        else if (PAD == BUTTON_LEFT){
            frogx+= -1;
            }
        else if (PAD == BUTTON_RIGHT){
            frogx+=1;
            }

		u32 regData=PAD|0x01000000;
		init_map();
        for(i=0;i<6;i++){
            for(j=0;j<8;j++){
                draw_car(carx[i][j],15+j*15);
                if(i<5){
                    if(j<5){
                        draw_logs(logx[i][j],150+j*15);
                        }
                }
            }
        }
        draw_frog(frogx,frogy);
		//Checks Colition
        for(i=0;i<6;i++){
            for(j=0;j<8;j++){
                if(frogy==j+1){
                    if(frogx==carx[i][j]){
                        frogy =0;
                        frogx =0;
                    }
                }
            }
        }
        //Checks if frog in poodle
         if(frogy >9){
            for(i=0;i<5;i++){
                for(j=0;j<5;j++){
                    if(frogy==j+10){
                        if(frogx==logx[i][j] || frogx==logx[i][j]+1 || frogx==logx[i][j]+2){
                                frogx= frogx -1;
                        }
                        else{
                            frogx =0;
                            frogy =0;
                            }
                    }
                }
            }
        }
        //reinitialize the cars :D
        // Cant use fucking rand
        //for(p=0;p<6;p++){
        //    if(carx[p]==0){
        //        d=5;
        //        while(d>4){
        //            d = rand();
        //        }
        //        carx[p]= 26+d;
        //     }
        //    else {
        //    carx[p]=carx[p]-1;
        //    }
        //}
        for(i=0;i<6;i++){
            for(j=0;j<9;j=j+2){
                if(i<5){
                    if(j<5){
                        if(logx[i][j]==0){
                            logx[i][j]=26;
                        }
                        else{
                        logx[i][j]=logx[i][j]-1;
                        }
                    }
                }
                if(carx[i][j]==0){
                    carx[i][j] =26;
                }
                else{
                    carx[i][j]=carx[i][j]-1;
                }
            }
        }
        for(i=0;i<6;i++){
            for(j=1;j<9;j=j+2){
                if(i<5){
                    if(j<5){
                        if(logx[i][j]==26){
                            logx[i][j]=0;
                        }
                        else{
                        logx[i][j]=logx[i][j]+1;
                        }
                    }
                }
                if(carx[i][j]==26){
                    carx[i][j]=0;
                    }
                else{
                    carx[i][j]= carx[i][j]+1;
                }
            }
        }
		copyBuffer();
		swapBuffers();
		GSPGPU_WriteHWRegs(NULL, 0x202A04, &regData, 4);
        svc_sleepThread(220000000);


    }

	svc_closeHandle(fsuHandle);
	hidExit();
	gspGpuInit();
	aptExit();
	svc_exitProcess();
	return 0;
}
