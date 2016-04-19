/*
  ==============================================================================

    Ppm.h
    Created: 16 Apr 2016 3:59:47pm
    Author:  Ashis Pati

  ==============================================================================
*/

#ifndef PPM_H_INCLUDED
#define PPM_H_INCLUDED

#include "ErrorDef.h"


class Ppm {
    
public:
    enum PpmParameter_t
    {
        AlphaAt,
        AlphaRt,
        
        numPpmParameters
    };
    
    static Error_t createInstance (Ppm*& ppm);
    static Error_t destroyInstance (Ppm*& ppm);
    
    Error_t initInstance (float sample_rate, int num_channels);
    Error_t resetInstance ();
    
    Error_t setParam (PpmParameter_t eParam, float fParamValue);
    float getParam (PpmParameter_t eParam) const;
    
    Error_t process(float **input_buffer, int number_of_frames, float* ppm_value);
    
protected:
    Ppm ();
    ~ Ppm ();
    
    
    
private:
    bool isInParamRange (PpmParameter_t eParam, float time_in_sec);
    bool _is_initialized;
    float _sample_rate;
    int _num_channels;
    float* _previous_ppm;
    float _param_range[numPpmParameters][2];
    float _params[numPpmParameters];
    float convertTimeInSecToValue(float time_in_sec);
    float convertValueToTimeInSec(float value) const;
    float findMaxOfFrame(float** input_buffer, int number_of_frames);
};


#endif  // PPM_H_INCLUDED
