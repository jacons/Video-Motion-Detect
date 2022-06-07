class ff_loader : public ff_node_t<void*,Mat> {
    private:
        VideoCapture source; // Source of video
    public:
        ff_loader(VideoCapture source): source(source) { }

        Mat* svc(void**) {
            
            Mat frame,*original;

            // We retrieve a shape of frames
            int width  = source.get(CAP_PROP_FRAME_WIDTH);
            int height = source.get(CAP_PROP_FRAME_HEIGHT);
            int totalf = source.get(CAP_PROP_FRAME_COUNT)-1;

            // Number of byte used by RGB images
            int nbytes = sizeof(unsigned char)*width*height*3;
            int c_frame = 0; // Number of frame seen

            // We send all frame of video
            // I cannot found a method to tranform a Mat to Mat pointer
            // A dummy way is to copy, but it's inefficent!
            while(source.isOpened() && c_frame<totalf) {
                
                ERROR_MSG(!source.read(frame),"Error in read frame operation")

                original = new Mat(height,width,CV_8UC3,Scalar(0,0,0)); 
                memcpy(original->data, frame.data, nbytes); 

                // Send all frame  
                ff_send_out(original);
                c_frame++;
            }
            return EOS;
        }
};

class ffa_worker : public ff_node_t<Mat,ushort> {
    private:
    int width,height;       // Shape of frame
    int dim,pixels;         // Kernel's dimention and frame's dimention
    int dx;                 // Padding and number of workers
    Mat* background,*gray;  // Background image to compare,pointer to "reusable" grayscale image
    ulong totald;           // Total pixels that are different
    float k;                // Percentage

    public:
    ffa_worker(VideoCapture source,int dx,Mat* background,float k):
        dx(dx),background(background),dim((dx+dx+1)*(dx+dx+1)),k(k) {

        this->width  = source.get(CAP_PROP_FRAME_WIDTH);
        this->height = source.get(CAP_PROP_FRAME_HEIGHT);
        this->pixels = height * width;

        // Reusable "frame" 
        this->gray = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);
    }
    void svc_end() { delete gray; }

    ushort* svc(Mat* original) {

        int i,j,z,w;     // Counters
        float r,g,b,acc; // red,gree,blue & accomulator

        // We take each RGB pixel and we tranform it into grayscale pixel
        for (i = 0; i < height; i++) {
            for (j = 0; j < width; j++){
                r = 0.2989  * original->at<Vec3b>(i, j)[2];
                g = 0.5870  * original->at<Vec3b>(i, j)[1];
                b = 0.1140  * original->at<Vec3b>(i, j)[0];
                gray->at<uchar>(i+dx, j+dx) = round(r+g+b);
            }
        }
        delete original; // we need it no more

        totald = 0;

        // foreach pixel inside the grayscale image
        for (i = 0; i < height ; i++) {
            for (j = 0; j < width ; j++) {
                acc = 0;
                // we take a average of neighboors of pixel i,j
                // oss. we use +dx 'cause of padding explained before
                for(z=-dx;z<=dx;z++)for(w=-dx;w<=dx;w++) 
                        acc += gray->at<uchar>(i+dx+z,j+dx+w);   
                // acc means total of neighboors pixel 
                totald += background->at<uchar>(i, j) - static_cast<uchar>(acc/dim) != 0;
            }
        }
        // "Differents pixels" are divided by all pixels to obtain a percentage
        // if perc > k then the frame is "different" from background
        return new ushort((((float)totald )/pixels) > k);

    }

};

class ff_detect : public ff_node_t<ushort> {
    private:
    ulong* total; // Total of frame "detected"

    public:
    ff_detect(ulong* total): total(total) {}

    ushort* svc(ushort* detected) {
        *total += *detected;
        delete detected;
        return GO_ON;
    }
};
class fastflow_a {
    private:
    VideoCapture* source;  // Source of video
    int width,height;      // Shapes of frame
    int totalf;            // Number of total frame in the video
    int dx;                // "padding" (x grayscale)
    ulong totalDiff = 0 ;  // Variable used to accomulate frame "detected"
    int f_nw;              // Gray-worker(parfor) , Convolve-worker(parfor), Farm-Worker(Farm)
    Mat* background;       // Background images used for comparisons
    float k;               // Percentage

    void cleanUp() {
        source->release();
        delete background;
        delete source;
    }
    public:
    fastflow_a(const string path,const int ksize,const float k,const int f_nw):
        f_nw(f_nw),k(k),dx(ksize/2) { 

        // checking argument
        ERROR_MSG(path == "","path error")
        ERROR_MSG(ksize < 3 || ksize%2==0,"kernel size must be >3 and odd")
        ERROR_MSG(k<= 0 || k>1,"%'of pixel must be between 0 and 1")
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

        // tranform the RGB image into gray scale
        gray = VideoDetect::static_toGray(frame,height,width,dx);

        // Apply the convolution (smoothing)
        this->background = VideoDetect::static_convolve(gray,height,width,dx);

        delete gray;
    }

    void execute_to_result() {

        ff_farm farm;  

        ff_loader loader(*source);
        ff_detect ffa_detect(&totalDiff);

        farm.add_collector(&ffa_detect);
        farm.add_emitter(&loader);

        vector<ff_node*> workers(f_nw);

        for(int i=0;i<f_nw;++i) 
            workers[i] = new ffa_worker(*source,dx,background,k);
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

        ff_loader loader(*source);
        ff_detect ffa_detect(&totalDiff);

        farm.add_collector(&ffa_detect);
        farm.add_emitter(&loader);

        vector<ff_node*> workers(f_nw);
        for(int i=0;i<f_nw;++i) 
            workers[i] = new ffa_worker(*source,dx,background,k);
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