class Sequential {

    private:
    VideoCapture* source; // Source of video
    int width,height;     // Shape of frame
    VideoDetect* vd;      // VideoDetect class contains methods used to process images (sequentially) 
    ulong totalDiff = 0 ; // Variable used to accomulate frame "detected"
    Mat* background;      // Background images used for comparisons
    int totalf,dx;        // Number of total frame in the video and "padding" (x grayscale)

    void cleanUp() {
        source->release();
        delete background;
        delete source;
        delete vd;
    }
    
    public:
    /**
    * @brief This version using a sequential approach thus the instruction is executed one a time
    * using one single flow.
    * 
    * @param path Path of video to analyze
    * @param ksize Number of pixel per side (kernel = matrix of ksize*ksize)
    * @param k % of pixels that must be different to trigger "detection"
    */
    Sequential(const string path,const int ksize,const float k) {
        
        // checking argument
        ERROR_MSG(path == "","path error")
        ERROR_MSG(ksize < 3 || ksize%2==0,"kernel size must be >3 and odd")
        ERROR_MSG(k<= 0 || k>1,"%'of pixel must be between 0 and 1")

        // VideoCapture class used to retrieve frames
        this->source = new VideoCapture(path); 

        // Check if the video is opened
        ERROR_MSG(!source->isOpened(),"Error opening video")

        // Some useful information
        this->width  = source->get(CAP_PROP_FRAME_WIDTH);
        this->height = source->get(CAP_PROP_FRAME_HEIGHT);
        this->totalf = source->get(CAP_PROP_FRAME_COUNT);
        this->dx = ksize/2;

        // We need at least 2 frame: one is the background, the other is the frame to compare
        ERROR_MSG(totalf<3,"Too short video")

        // methods like ToGray, convolve ecc..
        this->vd = new VideoDetect(width,height,k,ksize);

        // ---- First of all we retrieve the background ----

        Mat frame,*gray = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);
        this->background = new Mat(height,width,CV_8UC1,DEFAULT_IMG);
        
        // take the fist frame of the video
        ERROR_MSG(!source->read(frame),"Error in read frame operation")

        // tranform the RGB image into gray scale
        vd->toGray(frame,gray);

        // Apply the convolution (smoothing)
        vd->convolve(gray,this->background);

        // Set the first frame as backgrounds
        vd->setBackground(this->background);
        delete gray;
    }

    void execute_to_result() {
        // For each frame on the video (stating from 2° frame)

        // We add padding to ease convolution step, in the end it will be removed
        Mat frame,*gray = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);

        // We create the final image(frame) with the original dimensions
        Mat* blurred = new Mat(height,width,CV_8UC1,DEFAULT_IMG);

        for(int f=0;f<totalf-1;f++) {
            // (1° step) Take next frame of video
            ERROR_MSG(!source->read(frame),"Error in read frame operation")

            // (2° step) RGB -> Grayscale
            vd->toGray(frame,gray);

            // (3° step) Blurring 
            vd->convolve(gray,blurred);

            // (4° step) Detecting, returns 0 or 1 if "triggered" or not
            totalDiff += vd->detect(blurred);
        }
        // clean memory on heap
        delete gray;
        delete blurred;

        cout << "Total frame: " << totalf << endl;
        cout << "Total diff: " << totalDiff << endl;

        cleanUp();
        exit(0);
    }

    void execute_to_stat() {
        // For each frame on the video (stating from 2° frame)

        // We add padding to ease convolution step , in the end it will be removed
        Mat frame,*gray = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);

        // We create the final image(frame) with the original dimensions
        Mat* blurred = new Mat(height,width,CV_8UC1,DEFAULT_IMG);

        long elapsed;
        {   
            utimer u("",&elapsed);

            for(int f=0;f<totalf-1;f++) {
                // (1° step) Take next frame of video
                ERROR_MSG(!source->read(frame),"Error in read frame operation")

                // (2° step) RGB -> Grayscale
                vd->toGray(frame,gray);

                // (3° step) Blurring 
                vd->convolve(gray,blurred);

                // (4° step) Detecting, returns 0 or 1 if "triggered" or not
                totalDiff += vd->detect(blurred);
            }
        }
        cout << elapsed << endl;

        delete gray;
        delete blurred;

        cleanUp();
        exit(0);
    }

    void execute_to_stat2() {
        // For each frame on the video (stating from 2° frame)

        // We add padding to ease convolution step , in the end it will be removed
        Mat frame,*gray = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);

        // We create the final image(frame) with the original dimensions
        Mat* blurred = new Mat(height,width,CV_8UC1,DEFAULT_IMG);

        ulong tot_s1 = 0,tot_s2 = 0,tot_s3 = 0,tot_s4 = 0;

        for(int f=1;f<totalf;f++) {
            
            long elapsed;
            {   
                utimer u("",&elapsed);
                // (1° step) Take next frame of video
                ERROR_MSG(!source->read(frame),"Error in read frame operation")
            }
            tot_s1 += elapsed;
            {   
                utimer u("",&elapsed);
                // (2° step) RGB -> Grayscale
                vd->toGray(frame,gray);
            }
            tot_s2 += elapsed;
            {   
                utimer u("",&elapsed);
                // (3° step) Blurring 
                vd->convolve(gray,blurred);
            }
            tot_s3 += elapsed;            
            {   
                utimer u("",&elapsed);
                // (4° step) Detecting, returns 0 or 1 if "triggered" or not
                this->totalDiff += vd->detect(blurred);
            }
            tot_s4 += elapsed;

        }
        cout << tot_s1 << "," << tot_s2 << "," << tot_s3 << "," << tot_s4 << endl;
        
        delete gray;
        delete blurred;

        cleanUp();
        exit(0);      
    }
};
