#include "opencv2/opencv.hpp"

using namespace cv;

int main(int argc, char** argv)
{
    VideoCapture cap;
    // open the default camera, use something different from 0 otherwise;
    // Check VideoCapture documentation.
    if (!cap.open(0))
        return 0;

    int frame_width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    int frame_height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    VideoWriter outputVideo(
        "out.avi",
        CV_FOURCC('M', 'J', 'P', 'G'),
        12, Size(frame_width, frame_height),
        true
    );
    
    for(;;) {
          Mat frame;
          cap >> frame;
          if( frame.empty() ) continue; // end of video stream
          
          outputVideo << frame;
          if( waitKey(1) == 27 ) break; // stop capturing by pressing ESC 
    }

    // cap.close();
    return 0;
}
