atomic<ulong> totalDiff(0) ; // Variable used to accomulate frame "detected"

/**
 * @brief This thread handles a loader phase, infact retrieve all frame and put them into queue 
 * @param source Video capture pointer (read frame)
 * @param queue queue to insert the frame read
 */
void loader_worker(VideoCapture* source,SQueue* queue) {

    int width  = source->get(CAP_PROP_FRAME_WIDTH);
    int height = source->get(CAP_PROP_FRAME_HEIGHT);
    int totalf = source->get(CAP_PROP_FRAME_COUNT);
    int nbytes = sizeof(unsigned char)*width*height*3;

    Mat frame,*original;
    for(int f=0;f<totalf-1;f++) {

        source->read(frame);
        original = new Mat(height,width,CV_8UC3,Scalar(0,0,0));
        memcpy(original->data, frame.data, nbytes); 
        queue->push(original);
    }
    // After read all frame, exit
    queue->end();
    return;
}

/**
 * @brief This is a node of farm, it process the entire computation
 * 
 * @param source VideoCapture pointer (to retrieve some information)
 * @param queue Queue where to get frame
 * @param dx "padding" (x grayscaling)
 * @param k percentage
 * @param background background image for comparisons
 */
void complete_worker(VideoCapture* source,SQueue* queue,int dx,float k,Mat* background) {

    int width  = source->get(CAP_PROP_FRAME_WIDTH);
    int height = source->get(CAP_PROP_FRAME_HEIGHT);
    int dim    = (dx+dx+1)*(dx+dx+1); // Kerenl's dimentions
    int pixels = height*width; // Frame's dimentions

    Mat* original;
    Mat* gray = new Mat(height+dx+dx,width+dx+dx,CV_8UC1,DEFAULT_IMG);

    int i,j,z,w;     // Counters
    float r,g,b,acc; // red,gree,blue & accomulator
    ulong totald;    // pixels that are different

    while(1)  {

        original = queue->get();
        // if the pointer is null means that the worker can terminate
        if (original == nullptr) break;

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
        // totalDiff atomic variable!
        totalDiff +=  (((float)totald )/pixels) > k ;

    }
    delete gray;
}

// Second implementation
class ThreadFarm {

    private:
    VideoCapture* source;     // Source of video
    int width,height;         // Shape of frame
    int totalf;               // Number of total frame in the video
    int dx,nw;                // "padding" (x grayscale) and number of workers
    float k;                  // Percentage
    vector<thread*>* workers; // Farm of complete-workers
    Mat* background;          // Background images used for comparisons

    void cleanUp() {
        source->release();
        delete background;
        delete source;
        delete workers;
    }

    public:
    ThreadFarm(const string path,const int ksize,const float k,const int nw):
        k(k),nw(nw),dx(ksize/2) {

        // checking argument
        ERROR_MSG(path == "","path error")
        ERROR_MSG(ksize < 3 || ksize%2==0,"kernel size must be >3 and odd")
        ERROR_MSG(k<= 0 || k>1,"%'of pixel must be between 0 and 1")
        ERROR_MSG(nw<= 0,"Workers must be more than 0")

        this->source = new VideoCapture(path); 
        this->workers = new vector<thread*>(nw);

        // Check if the video is opened
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

        // Create a Shared Queue
        SQueue* q = new SQueue();

        // Start the loader that pushes into queue the frames 
        thread* loader = new thread(loader_worker,source,q);

        // Start nw worker that perform the same function
        for(int i=0;i<nw;i++) 
            (*workers)[i] = new thread(complete_worker,source,q,dx,k,background);

        // Wait until the termination
        loader->join();

        // Same for workers
        for(int i=0;i<nw;i++) {
            (*workers)[i]->join();
            delete (*workers)[i];
        }
        cout << "Total frame: " << totalf << endl;
        cout << "Total diff: " << totalDiff << endl;
        cleanUp();
        exit(0);

    }
    void execute_to_stat() {
        // As before but in this case the measure the time execution
        
        SQueue* q = new SQueue();
        long elapsed;
        {   
            utimer u("",&elapsed);
            for(int i=0;i<nw;i++) {
                (*workers)[i] = new thread(complete_worker,source,q,dx,k,background);
            }

            thread* loader = new thread(loader_worker,source,q);
            loader->join();

            for(int i=0;i<nw;i++) {
                (*workers)[i]->join();
                delete (*workers)[i];
            }
        }
        cout << elapsed << endl;

        cleanUp();
        exit(0); 
    }
};