// Background average sample code
#include "cv.h"
#include "highgui.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "AvgBackground.h"
#include "cv_yuv_codebook.h"

//VARIABLES for CODEBOOK METHOD:
codeBook *cB;   //This will be our linear model of the image, a vector 
				//of lengh = height*width
int maxMod[CHANNELS];	//Add these (possibly negative) number onto max 
						// level when code_element determining if new pixel is foreground
int minMod[CHANNELS]; 	//Subract these (possible negative) number from min 
						//level code_element when determining if pixel is foreground
unsigned cbBounds[CHANNELS]; //Code Book bounds for learning
bool ch[CHANNELS];		//This sets what channels should be adjusted for background bounds
int nChannels = CHANNELS;
int imageLen = 0;
uchar *pColor; //YUV pointer

void help() {
	printf("\nLearn background and find foreground using simple average and average difference learning method:\n"
		"\nUSAGE: // backgroundAVG startFrameCollection# endFrameCollection# [movie filename, else from camera]\n"
		"If from AVI, then optionally add HighAvg, LowAvg, HighCB0 LowCB0 HighCB1 LowCB1 HighCB2 LowCB2\n\n"
		"INTERACTIVE PARAMETERS:\n"
		"\tESC,q,Q  - quit the program\n"
		"\th	- print this help\n"
		"\tp	- pause toggle\n"
		"\ts	- single step\n"
		"\tr	- run mode (single step off)\n"
		"=== AVG PARAMS ===\n"
		"\t-    - bump high threshold UP by 0.25\n"
		"\t=    - bump high threshold DOWN by 0.25\n"
		"\t[    - bump low threshold UP by 0.25\n"
		"\t]    - bump low threshold DOWN by 0.25\n"
		"=== CODEBOOK PARAMS ===\n"
		"\t0,1,2- only adjust channel 0 or 1 or 2 respectively\n"
		"\t3	- adjust all 3 channels at once\n"
		"\t4	- adjust only 2 and 3 at once\n"
		"\tu,i	- bump upper threshold up,down by 1\n"
		"\t,,.	- bump lower threshold up,down by 1\n"
		);
}

// backgroundAVG startFrameCollection endFrameCollection [movie filename, else from camera]
int main(int argc, char** argv)
{
 	IplImage* rawImage = 0, *yuvImage = 0; //yuvImage is for codebook method
    IplImage *ImaskAVG = 0,*ImaskAVGCC = 0;
    IplImage *ImaskCodeBook = 0,*ImaskCodeBookCC = 0;
    CvCapture* capture = 0;

	int startcapture = 1;
	int endcapture = 30;
	int c,n;

	maxMod[0] = 3;  //Set color thresholds to default values
	minMod[0] = 10;
	maxMod[1] = 1;
	minMod[1] = 1;
	maxMod[2] = 1;
	minMod[2] = 1;
	float scalehigh = HIGH_SCALE_NUM;
	float scalelow = LOW_SCALE_NUM;
	
	if(argc < 3) {
		printf("ERROR: Too few parameters\n");
		help();
	}else{
		if(argc == 3){
			printf("Capture from Camera\n");
			capture = cvCaptureFromCAM( 0 );
		}
		else {
			printf("Capture from file %s\n",argv[3]);
			capture = cvCaptureFromFile( argv[3] );
		}
		if(isdigit(argv[1][0])) { //Start from of background capture
			startcapture = atoi(argv[1]);
		}
		if(isdigit(argv[2][0])) { //End frame of background capture
			endcapture = atoi(argv[2]);
		}
		if(argc > 4){ //See if parameters are set from command line
			//FOR AVG MODEL
			if(argc >= 5){
				if(isdigit(argv[4][0])){
					scalehigh = (float)atoi(argv[4]);
				}
			}
			if(argc >= 6){
				if(isdigit(argv[5][0])){
					scalelow = (float)atoi(argv[5]);
				}
			}
			//FOR CODEBOOK MODEL, CHANNEL 0
			if(argc >= 7){
				if(isdigit(argv[6][0])){
					maxMod[0] = atoi(argv[6]);
				}
			}
			if(argc >= 8){
				if(isdigit(argv[7][0])){
					minMod[0] = atoi(argv[7]);
				}
			}
			//Channel 1
			if(argc >= 9){
				if(isdigit(argv[8][0])){
					maxMod[1] = atoi(argv[8]);
				}
			}
			if(argc >= 10){
				if(isdigit(argv[9][0])){
					minMod[1] = atoi(argv[9]);
				}
			}
			//Channel 2
			if(argc >= 11){
				if(isdigit(argv[10][0])){
					maxMod[2] = atoi(argv[10]);
				}
			}
			if(argc >= 12){
				if(isdigit(argv[11][0])){
					minMod[2] = atoi(argv[11]);
				}
			}
		}
	}

	//MAIN PROCESSING LOOP:
	bool pause = false;
	bool singlestep = false;

    if( capture )
    {
        cvNamedWindow( "Raw", 1 );
		cvNamedWindow( "AVG_ConnectComp",1);
		cvNamedWindow( "ForegroundCodeBook",1);
		cvNamedWindow( "CodeBook_ConnectComp",1);
 		cvNamedWindow( "ForegroundAVG",1);
        int i = -1;
        
        for(;;)
        {
			if(!pause){
        		if( !cvGrabFrame( capture ))
                	break;
            	rawImage = cvRetrieveFrame( capture );
				++i; //count it
			}
			if(singlestep){
				pause = true;
			}
			//First time:
			if(0 == i) {
				//AVG METHOD ALLOCATION
				AllocateImages(rawImage);
				scaleHigh(scalehigh);
				scaleLow(scalelow);
				ImaskAVG = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
				ImaskAVGCC = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
				cvSet(ImaskAVG,cvScalar(255));
				//CODEBOOK METHOD ALLOCATION:
				yuvImage = cvCloneImage(rawImage);
				ImaskCodeBook = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
				ImaskCodeBookCC = cvCreateImage( cvGetSize(rawImage), IPL_DEPTH_8U, 1 );
				cvSet(ImaskCodeBook,cvScalar(255));
				imageLen = rawImage->width*rawImage->height;
				cB = new codeBook [imageLen];
				for(int f = 0; f<imageLen; f++)
				{
 					cB[f].numEntries = 0;
				}
				for(int nc=0; nc<nChannels;nc++)
				{
					cbBounds[nc] = 10; //Learning bounds factor
				}
				ch[0] = true; //Allow threshold setting simultaneously for all channels
				ch[1] = true;
				ch[2] = true;
			}
			
       		// printf("%d ",i);
			//If we've got an rawImage and are good to go:                
        	if( rawImage )
        	{
				cvCvtColor( rawImage, yuvImage, CV_BGR2YCrCb );//YUV For codebook method
				//This is where we build our background model
				if( !pause && i >= startcapture && i < endcapture  ){
					//LEARNING THE AVERAGE AND AVG DIFF BACKGROUND
					accumulateBackground(rawImage);
					//LEARNING THE CODEBOOK BACKGROUND
					pColor = (uchar *)((yuvImage)->imageData);
					for(int c=0; c<imageLen; c++)
					{
						cvupdateCodeBook(pColor, cB[c], cbBounds, nChannels);
						pColor += 3;
					}
				}
				//When done, create the background model
				if(i == endcapture){
					createModelsfromStats();
				}
				//Find the foreground if any
				if(i >= endcapture) {
					//FIND FOREGROUND BY AVG METHOD:
					backgroundDiff(rawImage,ImaskAVG);
					cvCopy(ImaskAVG,ImaskAVGCC);
					cvconnectedComponents(ImaskAVGCC);
					//FIND FOREGROUND BY CODEBOOK METHOD
					uchar maskPixelCodeBook;
					pColor = (uchar *)((yuvImage)->imageData); //3 channel yuv image
					uchar *pMask = (uchar *)((ImaskCodeBook)->imageData); //1 channel image
					for(int c=0; c<imageLen; c++)
					{
						 maskPixelCodeBook = cvbackgroundDiff(pColor, cB[c], nChannels, minMod, maxMod);
						*pMask++ = maskPixelCodeBook;
						pColor += 3;
					}
					//This part just to visualize bounding boxes and centers if desired
					cvCopy(ImaskCodeBook,ImaskCodeBookCC);	
					cvconnectedComponents(ImaskCodeBookCC);
				}
				//Display
           		cvShowImage( "Raw", rawImage );
				cvShowImage( "AVG_ConnectComp",ImaskAVGCC);
   				cvShowImage( "ForegroundAVG",ImaskAVG);
 				cvShowImage( "ForegroundCodeBook",ImaskCodeBook);
 				cvShowImage( "CodeBook_ConnectComp",ImaskCodeBookCC);

				//USER INPUT:
	         	c = cvWaitKey(10);
				//End processing on ESC, q or Q
				if(c == 27 || c == 'q' | c == 'Q')
					break;
				//Esle check for user input
				switch(c)
				{
					case 'h':
						help();
						break;
					case 'p':
						pause ^= 1;
						break;
					case 's':
						singlestep = 1;
						pause = false;
						break;
					case 'r':
						pause = false;
						singlestep = false;
						break;
					//AVG BACKROUND PARAMS
					case '-':
						if(i > endcapture){
							scalehigh += 0.25;
							printf("AVG scalehigh=%f\n",scalehigh);
							scaleHigh(scalehigh);
						}
						break;
					case '=':
						if(i > endcapture){
							scalehigh -= 0.25;
							printf("AVG scalehigh=%f\n",scalehigh);
							scaleHigh(scalehigh);
						}
						break;
					case '[':
						if(i > endcapture){
							scalelow += 0.25;
							printf("AVG scalelow=%f\n",scalelow);
							scaleLow(scalelow);
						}
						break;
					case ']':
						if(i > endcapture){
							scalelow -= 0.25;
							printf("AVG scalelow=%f\n",scalelow);
							scaleLow(scalelow);
						}
						break;
				//CODEBOOK PARAMS
                case '0':
                        ch[0] = 1;
                        ch[1] = 0;
                        ch[2] = 0;
                        printf("CB YUV Channels active: ");
                        for(n=0; n<nChannels; n++)
                                printf("%d, ",ch[n]);
                        printf("\n");
                        break;
                case '1':
                        ch[0] = 0;
                        ch[1] = 1;
                        ch[2] = 0;
                        printf("CB YUV Channels active: ");
                        for(n=0; n<nChannels; n++)
                                printf("%d, ",ch[n]);
                        printf("\n");
                        break;
                case '2':
                        ch[0] = 0;
                        ch[1] = 0;
                        ch[2] = 1;
                        printf("CB YUV Channels active: ");
                        for(n=0; n<nChannels; n++)
                                printf("%d, ",ch[n]);
                        printf("\n");
                        break;
                case '3':
                        ch[0] = 1;
                        ch[1] = 1;
                        ch[2] = 1;
                        printf("CB YUV Channels active: ");
                        for(n=0; n<nChannels; n++)
                                printf("%d, ",ch[n]);
                        printf("\n");
                        break;
                case '4':
                        ch[0] = 0;
                        ch[1] = 1;
                        ch[2] = 1;
                        printf("CB YUV Channels active: ");
                        for(n=0; n<nChannels; n++)
                                printf("%d, ",ch[n]);
                        printf("\n");
                        break;
				case 'u': //modify max classification bounds
					for(n=0; n<nChannels; n++){
						if(ch[n])
							maxMod[n] += 1;
						printf("%.4d,",maxMod[n]);
					}
					printf(" CB High Side\n");
					break;
				case 'i': //modify max classification bounds
					for(n=0; n<nChannels; n++){
						if(ch[n])
							maxMod[n] -= 1;
						printf("%.4d,",maxMod[n]);
					}
					printf(" CB High Side\n");
					break;
				case ',': //modify min classification bounds (min bound goes lower)
					for(n=0; n<nChannels; n++){
						if(ch[n])
							minMod[n] += 1;
						printf("%.4d,",minMod[n]);
					}
					printf(" CB Low Side\n");
					break;
				case '.': //modify min classification bounds (min bound goes higher)
					for(n=0; n<nChannels; n++){
						if(ch[n])
							minMod[n] -= 1;
						printf("%.4d,",minMod[n]);
					}
					printf(" CB Low Side\n");
					break;
				}
				
            }
		}		
        cvReleaseCapture( &capture );
        cvDestroyWindow( "Raw" );
		cvDestroyWindow( "ForegroundAVG" );
		cvDestroyWindow( "AVG_ConnectComp");
		cvDestroyWindow( "ForegroundCodeBook");
		cvDestroyWindow( "CodeBook_ConnectComp");
		DeallocateImages();
		if(yuvImage) cvReleaseImage(&yuvImage);
		if(ImaskAVG) cvReleaseImage(&ImaskAVG);
		if(ImaskAVGCC) cvReleaseImage(&ImaskAVGCC);
		if(ImaskCodeBook) cvReleaseImage(&ImaskCodeBook);
		if(ImaskCodeBookCC) cvReleaseImage(&ImaskCodeBookCC);
		delete [] cB;
    }
	else{ printf("Darn\n");
	}
    return 0;
}


