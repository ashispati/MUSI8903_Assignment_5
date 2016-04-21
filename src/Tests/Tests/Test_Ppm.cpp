#include "MUSI8903Config.h"

#ifdef WITH_TESTS
#include <cassert>
#include <algorithm>
#include <cstdio>

#include "UnitTest++.h"

#include "Synthesis.h"
#include "Vector.h"

#include "Ppm.h"

SUITE(Ppm)
{
	struct PpmData
	{
		PpmData() :
			m_pPpm(0),
			m_ppfInputData(0),
			m_ppfOutputData(0),
			m_kiDataLength(234930),
			m_iBlockLength(1024),
			m_iNumChannels(3),
			m_fSampleRate(44100)
		{
			Ppm::createInstance(m_pPpm);
			m_pPpm->initInstance(m_fSampleRate, m_iNumChannels);
			m_ppfInputData = new float*[m_iNumChannels];
			m_ppfOutputData = new float*[m_iNumChannels];
			m_ppfInputTmp = new float*[m_iNumChannels];
			m_pfOutputTmp = new float[m_iNumChannels];
			m_iOutputPoints = static_cast<int>(ceil(m_kiDataLength / m_iBlockLength));
			for (int i = 0; i < m_iNumChannels; i++)
			{
				m_ppfInputData[i] = new float[m_kiDataLength];
				CSynthesis::generateSine(m_ppfInputData[i], 800, m_fSampleRate, m_kiDataLength, .6F, 0);
				m_ppfOutputData[i] = new float[static_cast<int>(m_iOutputPoints)];
				CVector::setZero(m_ppfOutputData[i], static_cast<int>(m_iOutputPoints));
			}
		}

		~PpmData()
		{
			for (int i = 0; i < m_iNumChannels; i++)
			{
				delete[] m_ppfOutputData[i];
				delete[] m_ppfInputData[i];
			}
			delete[] m_ppfOutputData;
			delete[] m_ppfInputData;

			Ppm::destroyInstance(m_pPpm);
		}

		void process()
		{
			int iNumFramesRemaining = m_kiDataLength;
			int iBlockIdx = 0;
			while (iNumFramesRemaining > 0)
			{
				int iNumFrames = std::min(iNumFramesRemaining, m_iBlockLength);

				for (int c = 0; c < m_iNumChannels; c++)
				{
					m_ppfInputTmp[c] = &m_ppfInputData[c][m_kiDataLength - iNumFramesRemaining];
				}
				m_pPpm->process(m_ppfInputTmp, iNumFrames, m_pfOutputTmp);
				for (int c = 0; c < m_iNumChannels; c++)
				{
					m_ppfOutputData[c][iBlockIdx] = m_pfOutputTmp[c];
				}
				iNumFramesRemaining -= iNumFrames;
				iBlockIdx++;
			}
		}

		Ppm *m_pPpm;
		float **m_ppfInputData,
			**m_ppfOutputData,
			**m_ppfInputTmp,
			*m_pfOutputTmp;
		int m_kiDataLength;
		int m_iBlockLength;
		int m_iNumChannels;
		float m_fSampleRate;
		int m_iOutputPoints;

	};

	
	TEST_FIXTURE(PpmData, DCInput)
	{
		for (int c = 0; c < m_iNumChannels; c++)
		{
			CSynthesis::generateDc(m_ppfInputData[c], m_kiDataLength, (c + 1)*.1F);
		}

		process();

		float at_time = m_pPpm->getParam(Ppm::AlphaAt);
		int steady_state = ceil(at_time * m_fSampleRate / m_iBlockLength);
		for (int c = 0; c < m_iNumChannels; c++)
		{
			m_pfOutputTmp = &m_ppfOutputData[c][steady_state];
			CHECK_ARRAY_CLOSE(m_ppfInputData[c], m_pfOutputTmp, m_iOutputPoints-steady_state, 1e-4F);
		}
	}
    /*
	TEST_FIXTURE(PpmData, SineInput)
	{
		// Defunct. Need to fix
		float amplitude = 2.0F;
		for (int c = 0; c < m_iNumChannels; c++)
		{
			CSynthesis::generateSine(m_ppfInputData[c], 440, m_fSampleRate, m_kiDataLength, amplitude, 0);
		}

		process();

		for (int c = 0; c < m_iNumChannels; c++)
			for (int i = 0; i < m_iOutputPoints; i++)
				CHECK_EQUAL(m_ppfOutputData, 20 * log10(amplitude));
	}
    */

	TEST_FIXTURE(PpmData, RampInput)
	{
		for (int c = 0; c < m_iNumChannels; c++)
		{
			for (int i = 0; i < m_kiDataLength; i++)
			{
				m_ppfInputData[c][i] = c + i;
			}
		}

		process();

		float at_time = m_pPpm->getParam(Ppm::AlphaAt);
		int steady_state = ceil(at_time * m_fSampleRate / m_iBlockLength);
		for (int c = 0; c < m_iNumChannels; c++)
		{
			float input_diff = m_ppfInputData[c][1] - m_ppfInputData[c][0];
			for (int i = steady_state; i < m_iOutputPoints - 2; i++)
			{
				CHECK_CLOSE(input_diff*m_iBlockLength, m_ppfOutputData[c][i + 1] - m_ppfOutputData[c][i], 1e0);
			}
		}
	}
    
    TEST_FIXTURE(PpmData, ZeroInput)
    {
        for (int c = 0; c < m_iNumChannels; c++)
            CVector::setZero(m_ppfInputData[c], m_kiDataLength);
        
        process();
        
        for (int c = 0; c < m_iNumChannels; c++) {
            CHECK_ARRAY_CLOSE(m_ppfInputData[c], &m_ppfOutputData[c][0], m_iOutputPoints, 1e-3F);
        }
    }

	TEST_FIXTURE(PpmData, DecayWithAlphaRT)
	{
		for (int c = 0; c < m_iNumChannels; c++)
		{
			for (int i = 0; i < floor(m_kiDataLength/2); i++)
			{
				m_ppfInputData[c][i] = 1;
			}
			for (int i = ceil(m_kiDataLength / 2); i < m_kiDataLength; i++)
			{
				m_ppfInputData[c][i] = 0;
			}
		}

		process();

		float at_time = m_pPpm->getParam(Ppm::AlphaAt);
		int steady_state = ceil(at_time * m_fSampleRate / m_iBlockLength);
		float ratio = exp(-2.2*m_iBlockLength / (m_fSampleRate*m_pPpm->getParam(Ppm::AlphaRt)));
		for (int c = 0; c < m_iNumChannels; c++)
		{
			for (int i = ceil(m_iOutputPoints / 2) + steady_state + 1; i < m_iOutputPoints; i++)
			{
				CHECK_CLOSE(ratio, m_ppfOutputData[c][i]/m_ppfOutputData[c][i-1], 1e-3);
			}
		}
	}
    
    TEST_FIXTURE(PpmData, DecayWithZeroDecayTime)
    {
        for (int c = 0; c < m_iNumChannels; c++)
        {
            for (int i = 0; i < floor(m_kiDataLength/2); i++)
            {
                m_ppfInputData[c][i] = 1;
            }
            for (int i = ceil(m_kiDataLength / 2); i < m_kiDataLength; i++)
            {
                m_ppfInputData[c][i] = 0;
            }
        }
        
        m_pPpm->setParam(Ppm::AlphaRt, 0);
        
        process();
        
        float at_time = m_pPpm->getParam(Ppm::AlphaAt);
        int steady_state = ceil(at_time * m_fSampleRate / m_iBlockLength);
        for (int c = 0; c < m_iNumChannels; c++) {
            for (int i = ceil(m_iOutputPoints / 2) + steady_state + 1; i < m_iOutputPoints; i++) {
                CHECK_CLOSE(0, m_ppfOutputData[c][i], 1e-3F);
            }
        }
    }
    
    TEST_FIXTURE(PpmData, AttackWithZeroAttackTime)
    {
        for (int c = 0; c < m_iNumChannels; c++)
        {
            for (int i = 0; i < m_kiDataLength; i++)
            {
                m_ppfInputData[c][i] = c + i;
            }
        }
        
        m_pPpm->setParam(Ppm::AlphaAt, 0);
        m_pPpm->setParam(Ppm::AlphaRt, 1.5);
        
        process();
        
        for (int c = 0; c < m_iNumChannels; c++) {
            for (int i = 0; i < m_iOutputPoints; i++) {
                CHECK_CLOSE(m_ppfInputData[c][m_iBlockLength*(i+1)-1], m_ppfOutputData[c][i], 1e-3F);
            }
        }

    }

	TEST_FIXTURE(PpmData, ParamRange)
	{
		CHECK_EQUAL(Error_t::kFunctionInvalidArgsError, m_pPpm->setParam(Ppm::AlphaAt, -1));
		CHECK_EQUAL(Error_t::kFunctionInvalidArgsError, m_pPpm->setParam(Ppm::AlphaRt, -1));

		CHECK_EQUAL(Error_t::kFunctionInvalidArgsError, m_pPpm->setParam(Ppm::AlphaAt, 1));
		CHECK_EQUAL(Error_t::kFunctionInvalidArgsError, m_pPpm->setParam(Ppm::AlphaRt, 2.5));
	}

}

#endif //WITH_TESTS