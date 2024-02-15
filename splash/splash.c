/*
Splash by Bryce Bejlovec and Owen Sapp

Simplistic water ripple simulation 'toy' utilizing Adafruit 2.4" display
Requires ADS7846 touch screen and ADXL345 accelerometer

Framebuffer Modification based on: https://gist.github.com/FredEckert/3425429:

retrieved from:
Testing the Linux Framebuffer for Qtopia Core (qt4-x11-4.2.2)

http://cep.xor.aps.anl.gov/software/qt4-x11-4.2.2/qtopiacore-testingframebuffer.html
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <signal.h>
#include <math.h>
#include <linux/input.h>

FILE* xAxis;
FILE* yAxis;
FILE* zAxis;

int keepgoing = 1;

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
int x = 0, y = 0;
long int location = 0;

int oldxAxis = 0;
int curxAxis = 0;
int newxAxis = 0;
int oldyAxis = 0;
int curyAxis = 0;
int newyAxis = 0;

int touchfd = 0;
fd_set touchrd;
struct input_event touchev[5];
struct timeval touchtimer;


void signal_handler(int sig)
{
	printf( "\nCtrl-C pressed, cleaning up and exiting...\n" );
	keepgoing = 0;
}

int readAxis(FILE* axis){
    char buffer[7];
    fread(buffer,sizeof(char),6,axis);
    int retval = atoi(buffer);
    fseek(axis, 0, SEEK_SET);
    return retval;
}

void parseInput(signed char motionVec[][60][2]){
    FD_ZERO(&touchrd);
    FD_SET(touchfd,&touchrd);
    touchtimer.tv_sec = 0;
    touchtimer.tv_usec = 100;
    select(touchfd+1, &touchrd,NULL,NULL,&touchtimer);

    
    if(touchtimer.tv_usec == 0){
        return;
    }
    int x = 0;
    int y = 0;
    int pressure = 0;
    int size = read(touchfd,touchev,sizeof(touchev));
    for(int i = 0; i < size/sizeof(struct input_event);i++){
        switch(touchev[i].code){
            case ABS_Y:
                x = (touchev[i].value-350)/45;
            break;
            case ABS_X:
                y = (touchev[i].value-250)/60;
            break;
            case ABS_PRESSURE:
                pressure = touchev[i].value;
            break;
            default:
            continue;
        }
    }
    if(x < 0)
        x = 0;
    else if(x > 79)
        x = 79;
    if(y < 1)
        y = 0;
    else if(y > 59)
        y = 59;
    y = 59 - y;
    motionVec[x][y][0] += pressure;
    motionVec[x][y][1] += pressure;
    motionVec[x][y][0] += pressure;
    motionVec[x][y][1] += pressure;
    return;
}

int formatPixel(char* pixel){
    int r = pixel[0]>>3;
    int g = pixel[1]>>2;
    int b = pixel[2]>>3;
    return r << 11 | g << 5 | b;
}

void writeFrame(unsigned char frame[][60][3]){
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 320; x++) {
            // Figure out where in memory to put the pixel
            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                       (y+vinfo.yoffset) * finfo.line_length;
            // unsigned short int t =  //r<<8 | g << 4 | b;
            *((unsigned short int*)(fbp + location)) = formatPixel(frame[x>>2][y>>2]);
        }
    }
    // usleep(100000);
}

void iterateVec(signed char motionVec[][60][2],signed char prevVec[][60][2]){
    signed char curVec[82][62][2] = { [0][0][0] = 0 };
    for(signed int i = 0; i < 80; i++){
        for(signed int j = 0; j < 60; j++){
            curVec[i+1][j+1][0] = motionVec[i][j][0];
            curVec[i+1][j+1][1] = motionVec[i][j][1];
        }
    }
    for(signed int i = 0; i < 80; i++){
        for(signed int j = 0; j < 60; j++){
            // signed int xmax = (i != 79);
            // signed int xmin = (i != 0);
            // signed int ymax = (j != 59);
            // signed int ymin = (j != 0);
            signed int flow[2];
            flow[0] = 0;
            flow[1] = 0;

            // flow[0] += curVec[i + xmax][j][0]*(-1 + 2*xmax);
            // flow[0] += curVec[i - xmin][j][0]*(-1 + 2*xmin);
            // flow[0] += curVec[i][j + ymax][0]*(-1 + 2*ymax);
            // flow[0] += curVec[i][j - ymin][0]*(-1 + 2*ymin);
            flow[0] += curVec[i + 2][j + 1][0];
            flow[0] += curVec[i][j + 1][0];
            flow[0] += curVec[i + 1][j + 2][0]>>1;
            flow[0] += curVec[i + 1][j][0]>>1;

            // flow[1] += curVec[i + xmax][j][1]*(-1 + 2*xmax);
            // flow[1] += curVec[i - xmin][j][1]*(-1 + 2*xmin);
            // flow[1] += curVec[i][j + ymax][1]*(-1 + 2*ymax);
            // flow[1] += curVec[i][j - ymin][1]*(-1 + 2*ymin);
            flow[1] += curVec[i + 2][j + 1][1]>>1;
            flow[1] += curVec[i][j + 1][1] >> 1;
            flow[1] += curVec[i + 1][j + 2][1];
            flow[1] += curVec[i + 1][j][1];

            flow[0] = flow[0]/3;
            flow[1] = flow[1]/3;

            if(abs(flow[0]) < 10 && abs(prevVec[i][j][0]) > 100){
                prevVec[i][j][0] = prevVec[i][j][0]>>1;
            }
            if(abs(flow[1]) < 10 && abs(prevVec[i][j][1]) > 100){
                prevVec[i][j][1] = prevVec[i][j][1]>>1;
            }

            motionVec[i][j][0] =  ((flow[0]*0.5 + curVec[i + 1][j + 1][0])/0.8 - prevVec[i][j][0]);
            motionVec[i][j][1] =  ((flow[1]*0.5 + curVec[i + 1][j + 1][1])/0.8 - prevVec[i][j][1]);
            // motionVec[i][j][0] = motionVec[i][j][0] - motionVec[i][j][0]/10;
            // motionVec[i][j][1] = motionVec[i][j][1] - motionVec[i][j][1]/10;
        }
    }
    for(signed int i = 0; i < 80; i++){
        for(signed int j = 0; j < 60; j++){
            prevVec[i][j][0] = curVec[i + 1][j + 1][0];
            prevVec[i][j][1] = curVec[i + 1][j + 1][1];
        }
    }
}

void setFrame(unsigned char frame[][60][3], float grad[][60], unsigned char* base, float* baseRatio, signed char motionVec[][60][2],signed char prevVec[][60][2]){
    parseInput(motionVec);
    newyAxis = readAxis(yAxis);
    newxAxis = readAxis(xAxis);
    
    signed int force[2];
    
    force[0] = (newyAxis - ((curyAxis + oldyAxis)>>2));
    force[1] = (newxAxis - ((curxAxis + oldxAxis)>>2));

    // printf("%d\n",force[0]>>3);

    signed int mx = newyAxis>>3;
    signed int my = newxAxis>>3;
    if(force[0]*force[0] > 2500){
        signed int xval = (40 * ((force[0] < 0) - (force[0]> 0))) - (force[0] < 0);
        for(int i = 0;i < 60;i++){
            motionVec[40+xval][i][0] = motionVec[40+xval][i][0] - (force[0]>>1);
        }
    }
    if(force[1]*force[1] > 2500){
        signed int yval = (30 * ((force[1] < 0) - (force[1]> 0))) - (force[1] < 0);
        for(int i = 0;i < 80;i++){
            motionVec[i][30 + yval][1] = motionVec[i][30 + yval][1] + (force[1]>>1);
        }

    }
    iterateVec(motionVec,prevVec);

    for(signed int i = 0; i < 80; i++){
        for(signed int j = 0; j < 60; j++){
            grad[i][j] = (float)((-(i-40)*mx - (j-30)*my)>>3);
            signed char caustic = (motionVec[i][j][0]+motionVec[i][j][1])>>2;
            
            frame[i][j][0] = (unsigned char)((float)(base[0]) + (grad[i][j] * baseRatio[0]) + caustic);
            frame[i][j][1] = (unsigned char)((float)(base[1]) + (grad[i][j] * baseRatio[1]) + caustic);
            frame[i][j][2] = (unsigned char)((float)(base[2]) + (grad[i][j] * baseRatio[2]) + caustic);
        }
    }
    oldxAxis = curxAxis;
    oldyAxis = curyAxis;
    curxAxis = newxAxis;
    curyAxis = newyAxis;
}


int main (int argc, char *argv[])
{
    unsigned char (*frame)[80][60][3] = calloc(1, sizeof(*frame));
    signed char (*motionVec)[80][60][2] = calloc(1, sizeof(*motionVec));
    signed char (*prevVec)[80][60][2] = calloc(1, sizeof(*motionVec));
    float (*grad)[80][60] = calloc(1, sizeof(*grad));
    
    unsigned char base[3];
    float baseTotal = 0;
    float baseRatio[3];

    // Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);
    
    xAxis = fopen("/sys/class/i2c-adapter/i2c-2/2-0053/iio:device0/in_accel_x_raw","r");
    yAxis = fopen("/sys/class/i2c-adapter/i2c-2/2-0053/iio:device0/in_accel_y_raw","r");
    zAxis = fopen("/sys/class/i2c-adapter/i2c-2/2-0053/iio:device0/in_accel_z_raw","r");

    if(xAxis == NULL){
        printf("Gyroscope Not Detected\n");
        return 1;
    }

    if ((touchfd = open("/dev/input/event1", O_RDONLY)) < 0) {
		// perror("evtest");
		// if (errno == EACCES && getuid() != 0)
		// 	fprintf(stderr, "You do not have access to %s. Try "
		// 			"running as root instead.\n",
		// 			filename);
		// goto error;
        printf("Touchscreen Not Detected\n");
	}


    if(argc == 4) {     // get RGB color
        base[0] = atoi(argv[1]);
        base[1] = atoi(argv[2]);
        base[2] = atoi(argv[3]);
    }else{
        base[0] = 60;
        base[1] = 130;
        base[2] = 170;
    }
    baseTotal = (float)(base[0]+base[1]+base[2]);
    baseRatio[0] = ((float)base[0])/baseTotal;
    baseRatio[1] = ((float)base[1])/baseTotal;
    baseRatio[2] = ((float)base[2])/baseTotal;

    printf("R: %d, G: %d, B: %d\n", base[0], base[1], base[2]);

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
    printf("Offset: %dx%d, line_length: %d\n", vinfo.xoffset, vinfo.yoffset, finfo.line_length);
    
    if (vinfo.bits_per_pixel != 16) {
        printf("Can't handle %d bpp, can only do 16.\n", vinfo.bits_per_pixel);
        exit(5);
    }

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");
    while(keepgoing == 1){
        setFrame(frame[0],grad[0],base,baseRatio,motionVec[0],prevVec[0]);
        writeFrame(frame[0]);
    }
    
    munmap(fbp, screensize);
    close(fbfd);
    fclose(xAxis);
    fclose(yAxis);
    fclose(zAxis);
    close(touchfd);
    return 0;
}
