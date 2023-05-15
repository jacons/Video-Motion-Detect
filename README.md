# Video-Motion-Detect

Video motion detect (AV)
Simple motion detection may be implemented by subtracting video frames from some background images. We require to implement a parallel video motion detector processing video such that:
1. The first frame of the video is taken as “background picture”
2. Each frame is turned to greyscale, smoothed, and subtracted to the greyscale smoothed background picture
3. Motion detect flag will be true in case more the k% of the pixels differ
4. The output of the program is represented by the number of frames with motion detected
OpenCV may be used to open video file and read frames only (greyscale and smoothing should be programmed explicitly. Converting an image to greyscale can be achieved by substituting each (R,G,B) pixel with a grey pixel with a grey value which is the average of the R, G and B values. Smoothing requires pixel is obtained “averaging” its value with the value of the surrounding pixels. This is achieved by usually multiplying the 3x3 pixel neighborhood by one of the matrixes
and subsequently taking as the new value of the pixel the central value in the resulting matrix (H1 computes the average, the other Hi computes differently weighted averages.
