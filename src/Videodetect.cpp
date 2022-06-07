class VideoDetect {
    protected:
    
        const int width,height; // Shape of frames
        const int dim;          // Total kernel's pixels 
        const int ksize;        // Number of kernel's pixel per side (square kernel)
        const long pixels;      // Total frame's pixels
        const int dx;           // (ksize-1)/2
        const float k;          // % of pixels that must be different to trigger "detection"
        Mat* background;        // Background image used to comparisons

    public:
        VideoDetect(const int width,const int height,const float k,const int ksize):
            width(width),height(height),k(k),ksize(ksize),dim(ksize*ksize),
            dx(ksize/2),pixels(width * height),background(nullptr) { }

        void setBackground(Mat* background) {
            this->background = background;
        }

        /**
         * @brief Tranform the multi-channel RGB image into single-channel, for each pixel we make a 
         * linear combination in order to produce a grayscale pixel.
         * 
         * @param src Original image
         * @param dest Pointer to destination (Grayscaled)
         */
        void toGray(const Mat src,Mat* dest) {
            float r,g,b;
            int i,j;

            // We take each RGB pixel and we tranform it into grayscale pixel
            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){
                    r = 0.2989  * src.at<Vec3b>(i, j)[2];
                    g = 0.5870  * src.at<Vec3b>(i, j)[1];
                    b = 0.1140  * src.at<Vec3b>(i, j)[0];
                    dest->at<uchar>(i+dx, j+dx) = round(r+g+b);
                }
            }
        }

        /**
         * @brief We apply the convolution to the image, the covolution is performed passing a matrix/kernel
            which is formed by all one (the result is to take a average of neighboors pixel)  to the grayscale image. 
            More kernel size is bigger more the computation is slower.
         * 
         * @param src pointer to grayscale image
         * @param src pointer to blurred image
         */
        void convolve(const Mat* src,Mat* dest) {
        
            int i,j,z,w; 
            float acc;

            // Foreach pixel on grayscale image
            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    acc = 0;
                    // We take a average of neighboors of pixel i,j
                    // oss. we use +dx 'cause of padding explained before
                    for(z=-dx;z<=dx;z++)for(w=-dx;w<=dx;w++) 
                            acc += src->at<uchar>(i+dx+z,j+dx+w);
                    // acc means total of neighboors pixel, all is dived by kernel's dimentions
                    dest->at<uchar>(i, j) = acc/dim;
                }
            }
        }

        /**
         * @brief This method apply the convolution as before but at the same time perform the detection,
         * the avantages of using this method is to avoid the "blurred matrix" allocation.
         * @param src Pointer to grayscale image
         * @return ushort 1 if the moviment is detected
         */
        ushort convolveDetect(const Mat* src) {
        
            int i,j,z,w;
            float acc;
            ulong totald = 0;

            // Foreach pixel inside the grayscale image
            for (i = 0; i < height ; i++) {
                for (j = 0; j < width ; j++) {
                    acc = 0;
                    // We take a average of neighboors of pixel i,j
                    // oss. we use +dx 'cause of padding explained before
                    for(z=-dx;z<=dx;z++)for(w=-dx;w<=dx;w++) 
                            acc += src->at<uchar>(i+dx+z,j+dx+w);

                    // We are comparing with to background
                    totald += background->at<uchar>(i, j) - static_cast<uchar>(acc/dim) != 0;
                }
            }

            // "Differents pixels" are divided by all pixels to obtain a percentage
            float perc = (((float)totald)/pixels);   
            // if perc > k then the frame is "different" from background
            return  perc > k ;        
        }

        /**
         * @brief We compare the source and the background in order to detect the "movement"
         * @param s frame to compare
         * @return ushort returns 1 if triggered 0 otherwise
         */
        ushort detect(const Mat* src) {

            int i,j;
            ulong acc = 0;

            // We compare each pixels to perform the difference
            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){
                    // If they are different, we increase the counter
                    acc += background->at<uchar>(i, j) != src->at<uchar>(i, j) ;
                }
            }

            // "Differents pixels" are divided by all pixels to obtain a percentage
            float perc = (((float)acc)/pixels);
            //cout << perc << endl;   
            // if perc > k then the frame is "different" from background
            return  perc > k ;       
        }

        // ------------- STATIC UTILS -------------
        /**
         * @brief The mechanism is equal to the previous one, but this requires more arguments
         * 
         * @param src Pointer to frame that we have to process
         * @param height Number of rows of frame
         * @param width Number of cols of frame
         * @param dx "padding" to add
         * @return Mat* pointer to grayscale image
         */
        static Mat* static_toGray(Mat src,int height,int width,int dx) {

            Mat* grey = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);
            float r,g,b;
            int i,j;

            for (i = 0; i < height; i++) {
                for (j = 0; j < width; j++){

                    r = 0.2989  * src.at<Vec3b>(i, j)[2];
                    g = 0.5870  * src.at<Vec3b>(i, j)[1];
                    b = 0.1140  * src.at<Vec3b>(i, j)[0];
                    grey->at<uchar>(i+dx, j+dx) = round(r+g+b);

                }
            }
            return grey;
        }
        /**
         * @brief The mechanism is equal to the previous one, but this requires more arguments
         * 
         * @param src Pointer to frame that we have to process
         * @param height Number of rows of frame
         * @param width Number of cols of frame
         * @param dx "padding" to add
         * @return Mat* pointer to blurred image 
         */
        static Mat* static_convolve(Mat* src,int height,int width,int dx) {
            
            Mat* blurred = new Mat(height,width,CV_8UC1,DEFAULT_IMG);
            int i,j,z,w,dim = (dx+dx+1)*(dx+dx+1);
            float acc;

            for (i = 0; i <height ; i++) {
                for (j = 0; j < width ; j++) {
                    acc = 0;
                    for(z=-dx;z<=dx;z++)for(w=-dx;w<=dx;w++) 
                        acc += src->at<uchar>(i+dx+z,j+dx+w);

                    blurred->at<uchar>(i, j) = acc/dim;
                }
            }
            return blurred;
        }

        // This method was used to debug
        static void GetFirstSteps(const string path,const int ksize) {
            
            ERROR_MSG(path == "","path error")

            VideoCapture source(path); 

            // check if the video is opened
            ERROR_MSG(!source.isOpened(),"Error opening video")

            int totalf = source.get(CAP_PROP_FRAME_COUNT);
            int width  = source.get(CAP_PROP_FRAME_WIDTH);
            int height = source.get(CAP_PROP_FRAME_HEIGHT);
            int nbytes = sizeof(unsigned char)*width*height*3;
            int dx = ksize/2;

            Mat frame,*gray,*blurred;
            // take the fist frame of the video
            source.read(frame);
            imwrite("Original.jpg",frame);
            // tranform the RGB image into gray scale
            gray = VideoDetect::static_toGray(frame,height,width,dx);
            imwrite("Grayscale.jpg",*gray);
            // Apply the convolution (smoothing)
            blurred = VideoDetect::static_convolve(gray,height,width,dx);
            imwrite("blurred.jpg",*blurred);

            delete gray;
            delete blurred;
        }

};