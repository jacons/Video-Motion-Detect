class utimer {
  chrono::system_clock::time_point start;
  chrono::system_clock::time_point stop;
  string message; 
  using usecs = chrono::microseconds;
  using msecs = chrono::milliseconds;

  private:
    long * us_elapsed;
  
  public:

    utimer(const string m) : message(m),us_elapsed((long *)NULL)  {
      start = chrono::system_clock::now();
    }
      
    utimer(const string m, long * us) : message(m),us_elapsed(us) {
      start = chrono::system_clock::now();
    }

    ~utimer() {
      stop = chrono::system_clock::now();
      chrono::duration<double> elapsed = stop - start;
      auto musec = chrono::duration_cast<chrono::microseconds>(elapsed).count();
      
      //cout << message << " computed in " << musec << " usec "  << endl;
      if(us_elapsed != NULL) (*us_elapsed) = musec;
    }
};