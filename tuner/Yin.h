#ifndef Yin_h
#define Yin_h

/// https://github.com/JorenSix/Pidato/tree/master/libraries/Yin


//#include "WProgram.h"

class Yin{
  
public: 
  //Yin();  
public:
  Yin(float sampleRate,int bufferSize);
public:
  void initialize();
  
public: 
  float getPitch(float* buffer);
public: 
  float getProbability();
  
private: 
  float parabolicInterpolation(int tauEstimate);
private: 
  int absoluteThreshold();
private: 
  void cumulativeMeanNormalizedDifference();
private: 
  void difference(float* buffer);
private:
  const double threshold;
  int bufferSize;
  const int halfBufferSize;
  const float sampleRate;
  float* yinBuffer;
  float probability;
};

#endif
