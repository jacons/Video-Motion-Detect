class toGrayMap: public ff_Map<Mat> {
    
    private:
    VideoCapture* source; // Source of video
    int width,height;     // Shape of frame
    int dx,nw;            // Number of total worker and "padding" (x grayscale)

    public:
    toGrayMap(VideoCapture* source,int dx,int nw): source(source),dx(dx),nw(nw) {

        this->width  = source->get(CAP_PROP_FRAME_WIDTH);
        this->height = source->get(CAP_PROP_FRAME_HEIGHT);
    }

    Mat *svc(Mat *original) {
        // The node recieves a RGB image-> process (mapping)-> send a grayscaled frame
        Mat* gray = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);

        
        parallel_for(0,width,[&] (const long i) {
            float r,g,b;    
            for (int h = 0; h < height; h++) {
                r = 0.2989  * original->at<Vec3b>(h, i)[2];
                g = 0.5870  * original->at<Vec3b>(h, i)[1];
                b = 0.1140  * original->at<Vec3b>(h, i)[0];
                gray->at<uchar>(h+dx, i+dx) = round(r+g+b);
            }
        },nw);
        
        delete original;
        return gray;
    }    
};

class toBlurMap: public ff_Map<Mat,ushort> {
    
    private:
    VideoCapture* source; // Source of video
    int width,height;     // Shape of frame
    int dim,pixels;       // Kernel's dimention and frame's dimention
    int dx,nw;            // Padding and number of workers
    Mat* background;      // Background images used for comparisons
    float k;              // Percentage

    public:
    toBlurMap(VideoCapture* source,int dx,int nw,Mat* background,float k):
     source(source),dx(dx),nw(nw),background(background),dim( (dx+dx+1)*(dx+dx+1) ),k(k) {

        this->width  = source->get(CAP_PROP_FRAME_WIDTH);
        this->height = source->get(CAP_PROP_FRAME_HEIGHT);
        this->pixels = width*height;
    }

    ushort* svc(Mat *gray) {
        // The node recieves a grayscaled image-> process (mapping)-> send 1 or 0
        atomic<ulong> totald; // Total pixels that are different
        totald = 0 ;
        
        parallel_for(0,width,[&] (const long i) {
                int z,w;float acc;
                for (int h = 0; h < height; h++) {
                    acc = 0;
                    for(z=-dx;z<=dx;z++)for(w=-dx;w<=dx;w++) 
                            acc += gray->at<uchar>(h+dx+z,i+dx+w);

                    // Comparing with background
                    totald += background->at<uchar>(h, i) - static_cast<uchar>(acc/dim) != 0;
                }
        },nw);
        delete gray;

        // "Differents pixels" are divided by all pixels to obtain a percentage
        // if perc > k then the frame is "different" from background
        return new ushort((float)totald/pixels > k);
    }    
};

class fastflow_b {
    private:
    VideoCapture* source;  // Source of video
    int width,height;      // Shapes of frame
    int totalf;            // Number of total frame in the video
    int dx;                // "padding" (x grayscale)
    ulong totalDiff = 0 ;  // Variable used to accomulate frame "detected"
    int g_nw,c_nw,f_nw;    // Gray-worker(parfor) , Blurring-worker(parfor), Farm-Worker(Farm)
    Mat* background;       // Background images used for comparisons
    float k;               // Percentage

    void cleanUp() {
        source->release();
        delete background;
        delete source;
    }
    public:
    fastflow_b(const string path,const int ksize,const float k,const int g_nw,const int c_nw,const int f_nw):
        c_nw(c_nw),g_nw(g_nw),f_nw(f_nw),k(k),dx(ksize/2) { 

        // checking argument
        ERROR_MSG(path == "","path error")
        ERROR_MSG(ksize < 3 || ksize%2==0,"kernel size must be >3 and odd")
        ERROR_MSG(k<= 0 || k>1,"%'of pixel must be between 0 and 1")
        ERROR_MSG(g_nw<= 0,"ToGray workers must be more than 0")
        ERROR_MSG(c_nw<= 0,"ToGray workers must be more than 0")
        ERROR_MSG(f_nw<= 0,"ToGray workers must be more than 0")

        this->source = new VideoCapture(path);

        // check if the video is opened
        ERROR_MSG(!source->isOpened(),"Error opening video")

        this->width  = source->get(CAP_PROP_FRAME_WIDTH);
        this->height = source->get(CAP_PROP_FRAME_HEIGHT);
        this->totalf = source->get(CAP_PROP_FRAME_COUNT);

        // We need at least 2 frame: one is the background, the other is the frame to compare
        ERROR_MSG(totalf<3,"Too short video")
  
        // ---- First of all we retrieve the background ----

        Mat frame,*gray;
        // take the fist frame of the video
        ERROR_MSG(!source->read(frame),"Error in read frame operation")

        // Tranform the RGB image into gray scale
        gray = VideoDetect::static_toGray(frame,height,width,dx);

        // Apply the convolution (blurring)
        this->background = VideoDetect::static_convolve(gray,height,width,dx);

        delete gray;
    }

    void execute_to_result() {

        ff_farm farm;  

        // both are defined in fastflow_a.cpp
        ff_loader loader(*source);
        ff_detect detect(&totalDiff);

        farm.add_collector(&detect); // Collect the result
        farm.add_emitter(&loader);  // Send frames


        vector<ff_node*> workers(f_nw);
        for(int i=0;i<f_nw;++i) {

            // we are creating a pipiline with two stages
            ff_pipeline* pipe = new ff_pipeline;
            pipe->add_stage(new toGrayMap(source,dx,g_nw));
            pipe->add_stage(new toBlurMap(source,dx,c_nw,background,k));
            workers[i] = pipe;
        }
        farm.add_workers(move(workers));

        farm.set_scheduling_ondemand();
        farm.run_and_wait_end();
        cout << "Total frame: " << totalf << endl;
        cout << "Total diff: " << totalDiff << endl;
        cleanUp();
        exit(0);
    }

    void execute_to_stat() {

        ff_farm farm;  

        // both are defined in fastflow_a.cpp
        ff_loader loader(*source);
        ff_detect detect(&totalDiff);

        farm.add_collector(&detect);
        farm.add_emitter(&loader);


        vector<ff_node*> workers(f_nw);
        for(int i=0;i<f_nw;++i) {
            // build worker pipeline 
            ff_pipeline* pipe = new ff_pipeline;
            pipe->add_stage(new toGrayMap(source,dx,g_nw));
            pipe->add_stage(new toBlurMap(source,dx,c_nw,background,k));
            workers[i] = pipe;
        }
        farm.add_workers(move(workers));
        farm.set_scheduling_ondemand();

        long el;
        {
            utimer u("",&el);
            farm.run_and_wait_end();
        } 
        cout << el << endl;
        cleanUp();
        exit(0);
    }
};