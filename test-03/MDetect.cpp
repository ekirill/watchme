//
//  Created by Cedric Verstraeten on 18/02/14.
//  Copyright (c) 2014 Cedric Verstraeten. All rights reserved.
//

#include <iostream>
#include <fstream>

#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <time.h>
#include <dirent.h>
#include <sstream>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std;
using namespace cv;

// Check if the directory exists, if not create it
// This function will create a new directory if the image is the first
// image taken for a specific day
inline void directoryExistsOrCreate(const char* pzPath)
{
    DIR *pDir;
    // directory doesn't exists -> create it
    if ( pzPath == NULL || (pDir = opendir (pzPath)) == NULL)
        mkdir(pzPath, 0777);
    // if directory exists we opened it and we
    // have to close the directory again.
    else if(pDir != NULL)
        (void) closedir (pDir);
}

// When motion is detected we write the image to disk
//    - Check if the directory exists where the image will be stored.
//    - Build the directory and image names.
int incr = 0;
inline bool saveImg(Mat image, const string DIRECTORY, const string EXTENSION, const char * DIR_FORMAT, const char * FILE_FORMAT)
{
    stringstream ss;
    time_t seconds;
    struct tm * timeinfo;
    char TIME[80];
    time (&seconds);
    // Get the current time
    timeinfo = localtime (&seconds);

    // Create name for the date directory
    strftime (TIME,80,DIR_FORMAT,timeinfo);
    ss.str("");
    ss << DIRECTORY << TIME;
    directoryExistsOrCreate(ss.str().c_str());
    ss << "/cropped";
    directoryExistsOrCreate(ss.str().c_str());

    // Create name for the image
    strftime (TIME,80,FILE_FORMAT,timeinfo);
    ss.str("");
    if(incr < 100) incr++; // quick fix for when delay < 1s && > 10ms, (when delay <= 10ms, images are overwritten)
    else incr = 0;
    ss << DIRECTORY << TIME << static_cast<int>(incr) << EXTENSION;
    return imwrite(ss.str().c_str(), image);
}

// Check if there is motion in the result matrix
// count the number of changes and return.
inline int detectMotion(const Mat & motion, Mat & result,
                 int x_start, int x_stop, int y_start, int y_stop,
                 int max_deviation)
{
    // calculate the standard deviation
    Scalar mean, stddev;
    meanStdDev(motion, mean, stddev);
    // if not to much changes then the motion is real (neglect agressive snow, temporary sunlight)
    if(stddev[0] < max_deviation)
    {
        int number_of_changes = 0;
        int min_x = motion.cols, max_x = 0;
        int min_y = motion.rows, max_y = 0;
        // loop over image and detect changes
        for(int j = y_start; j < y_stop; j+=2){ // height
            for(int i = x_start; i < x_stop; i+=2){ // width
                // check if at pixel (j,i) intensity is equal to 255
                // this means that the pixel is different in the sequence
                // of images (prev_frame, current_frame, next_frame)
                if(motion.at<int>(j,i) == 255)
                {
                    number_of_changes++;
                    if(min_x>i) min_x = i;
                    if(max_x<i) max_x = i;
                    if(min_y>j) min_y = j;
                    if(max_y<j) max_y = j;
                }
            }
        }
        return number_of_changes;
    }
    return 0;
}

bool hasMotion(Mat & prev_frame, Mat & next_frame, Mat & current_frame) {
    // Erode kernel
    Mat kernel_ero = getStructuringElement(MORPH_RECT, Size(2,2));

    // If more than 'there_is_motion' pixels are changed, we say there is motion
    // and store an image on disk
    int there_is_motion = 5;

    // Detect motion in window
    int x_start = 10, x_stop = current_frame.cols-11;
    int y_start = 10, y_stop = current_frame.rows-11;

    // Maximum deviation of the image, the higher the value, the more motion is allowed
    int max_deviation = 20;

    Mat d1, d2, motion;
    cvtColor(next_frame, next_frame, CV_RGB2GRAY);

    // Calc differences between the images and do AND-operation
    // threshold image, low differences are ignored (ex. contrast change due to sunlight)
    absdiff(prev_frame, next_frame, d1);
    absdiff(next_frame, current_frame, d2);
    bitwise_and(d1, d2, motion);
    threshold(motion, motion, 35, 255, CV_THRESH_BINARY);
    erode(motion, motion, kernel_ero);

    int number_of_changes = detectMotion(
        motion, next_frame, x_start, x_stop, y_start, y_stop, max_deviation
    );

    return (number_of_changes>=there_is_motion);
}

int main (int argc, char * const argv[])
{
    const int DELAY = 250; // in mseconds, take a picture every 1/2 second

    // Set up camera
    VideoCapture camera;
    camera.open(0);

    // Take images and convert them to gray
    Mat frame, result, prev_frame, next_frame, current_frame;
    int i = 0;
    while (i < 3) {
        camera >> frame;
        if( frame.empty() ) continue;
        if ( i == 0 ) {
            prev_frame = result = frame;
        } else if (i == 1) {
            current_frame = frame;
        } else {
            next_frame = frame;
        }
        i++;
    }
    cvtColor(current_frame, current_frame, CV_RGB2GRAY);
    cvtColor(prev_frame, prev_frame, CV_RGB2GRAY);
    cvtColor(next_frame, next_frame, CV_RGB2GRAY);

    int number_of_sequence = 0;

    // All settings have been set, now go in endless loop and
    // take as many pictures you want..
    while (true){
        // Take a new image
        prev_frame = current_frame;
        current_frame = next_frame;
        camera >> next_frame;
        if( next_frame.empty() ) continue;

        if (hasMotion(prev_frame, next_frame, current_frame)) {
            if(number_of_sequence>0){ 
                cout << "MOTION!\n";
            }
            number_of_sequence++;
        } else {
            number_of_sequence = 0;
            cvWaitKey (DELAY);
        }

    }
    return 0;    
}
