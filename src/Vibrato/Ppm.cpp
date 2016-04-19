/*
  ==============================================================================

    Ppm.cpp
    Created: 16 Apr 2016 3:59:47pm
    Author:  Ashis Pati

  ==============================================================================
*/

#include "Ppm.h"
#include <cmath>
#include <iostream>

Ppm::Ppm () :
_is_initialized(false),
_sample_rate(44100),
_num_channels(0)
{
    // this never hurts
    this->resetInstance ();
}


Ppm::~Ppm ()
{
    this->resetInstance ();
}

Error_t Ppm::createInstance (Ppm*& ppm)
{
    ppm = new Ppm ();
    
    if (!ppm)
        return kUnknownError;
    
    return kNoError;
}

Error_t Ppm::destroyInstance (Ppm*& ppm)
{
    if (!ppm)
        return kUnknownError;
    
    ppm->resetInstance ();
    
    delete ppm;
    ppm = 0;
    
    return kNoError;
}

Error_t Ppm::initInstance (float sample_rate, int num_channels)
{
    // set parameters
    _sample_rate = sample_rate;
    _num_channels = num_channels;
    _previous_ppm = new float [_num_channels];
    for (int i = 0; i < _num_channels; i++) {
        _previous_ppm[i] = 0;
    }
    
    // set parameter ranges
    _param_range[AlphaAt][0] = 0;
    _param_range[AlphaAt][1] = 0.02;
    _param_range[AlphaRt][0] = 0;
    _param_range[AlphaRt][1] = 2.0;
    
    
    //set default values
    _params[AlphaAt] = convertTimeInSecToValue(0.01);
    _params[AlphaRt] = convertTimeInSecToValue(1.5);
    
    _is_initialized    = true;
    
    return kNoError;
}

Error_t Ppm::resetInstance ()
{
    for (int i = 0; i < numPpmParameters; i++)
        setParam((PpmParameter_t)i, 0);
    
    _num_channels      = 0;
    _is_initialized    = false;
    
    return kNoError;
}

Error_t Ppm::setParam( PpmParameter_t eParam, float time_in_sec )
{
    if (!_is_initialized)
        return kNotInitializedError;
    if (!isInParamRange(eParam, time_in_sec))
        return kFunctionInvalidArgsError;
    
    _params[eParam] = convertTimeInSecToValue(time_in_sec);
    
    return kNoError;
}

float Ppm::getParam( PpmParameter_t eParam ) const
{
    if (!_is_initialized) {
        std::cout << "PPM is not initialized " << std::endl;
        return -1;
    }
    
    return convertValueToTimeInSec(_params[eParam]);
}


Error_t Ppm::process (float **input_buffer, int number_of_frames, float *ppm_value_max) {
    if (!input_buffer || number_of_frames < 0)
        return kFunctionInvalidArgsError;
    
    if (!_is_initialized) {
        return kNotInitializedError;
    }
    
    float current_sample = 0;
    
    for (int i = 0; i < number_of_frames; i++)
    {
        for (int c = 0; c < _num_channels; c++)
        {
            float ppm_value = 0;
            current_sample = fabsf(input_buffer[c][i]);
            
            // check for state
            //release state
            if (_previous_ppm[c] > current_sample) {
                ppm_value = (1 - _params[AlphaRt]) * _previous_ppm[c];
            }
            // attack state
            else {
                ppm_value = _params[AlphaAt] * current_sample + (1 - _params[AlphaAt]) * _previous_ppm[c];
            }
            // update max ppm value
            if (ppm_value > _previous_ppm[c]) {
                ppm_value_max[c] = ppm_value;
            }
            _previous_ppm[c] = ppm_value;
        }
        
    }
    return kNoError;
}

bool Ppm::isInParamRange( PpmParameter_t eParam, float time_in_sec )
{
    if (time_in_sec < _param_range[eParam][0] || time_in_sec > _param_range[eParam][1])
    {
        return false;
    }
    else
    {
        return true;
    }
}


float Ppm::convertTimeInSecToValue(float time_in_sec) {
    return 1 - expf(-2.2 / (_sample_rate * time_in_sec));
}

float Ppm::convertValueToTimeInSec(float value) const{
    return -2.2 / (_sample_rate * logf(1 - value));
}

float Ppm::findMaxOfFrame(float **input_buffer, int number_of_frames) {
    float max = 0;
    for (int i = 0; i < number_of_frames; i++)
    {
        float dummy = 0;
        for (int c = 0; c < _num_channels; c++)
        {
            dummy += input_buffer[c][i];
        }
        if (dummy >= max) {
            max = dummy;
        }
    }
    return max;
}

