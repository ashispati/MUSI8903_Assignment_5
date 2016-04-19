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
			m_kiDataLength(35131),
			m_iBlockLength(171),
			m_iNumChannels(3),
			m_fSampleRate(31271)
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
			delete[] m_ppfInputTmp;
			delete[] m_pfOutputTmp;
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

	/*
	TEST_FIXTURE(PpmData, DCInput)
	{
		for (int c = 0; c < m_iNumChannels; c++)
		{
			CSynthesis::generateDc(m_ppfInputData[c], m_kiDataLength, (c + 1)*.1F);
		}

		process();

		float at_time = m_pPpm->getParam(Ppm::AlphaAt);
		int stable_state = ceil(at_time * m_fSampleRate / m_iBlockLength);
		for (int c = 0; c < m_iNumChannels; c++)
		{
			m_pfOutputTmp = &m_ppfOutputData[c][stable_state];
			CHECK_ARRAY_CLOSE(m_ppfInputData[c], m_pfOutputTmp, m_iOutputPoints-stable_state, 1e-4F);
		}
	}*/

	//TEST_FIXTURE(PpmData, SineInput)
	//{
	//	// Run PPM through a sine wave generated with a phase of pi/2. Value should be a constant.
	//	// 
	//	float amplitude = 2.0F;
	//	for (int c = 0; c < m_iNumChannels; c++)
	//	{
	//		CSynthesis::generateSine(m_ppfInputData[c], 440, m_fSampleRate, m_kiDataLength, amplitude, 0);
	//	}

	//	process();

	//	for (int c = 0; c < m_iNumChannels; c++)
	//		for (int i = 0; i < m_iOutputPoints; i++)
	//			CHECK_EQUAL(m_ppfOutputData, 20 * log10(amplitude));
	//}

	//TEST_FIXTURE(PpmData, RampInput)
	//{
	//	// Need to compensate for the attack and release. Fix that. Currently basic skeleton is implemented

	//	m_iBlockLength = 1;
	//	m_kiDataLength = 5000;

	//	for (int c = 0; c < m_iNumChannels; c++)
	//	{
	//		for (int i = 0; i < m_kiDataLength; i++)
	//		{
	//			m_ppfInputData[c][i] = c + i;
	//		}
	//	}

	//	process();

	//	for (int c = 0; c < m_iNumChannels; c++)
	//		CHECK_ARRAY_CLOSE(m_ppfInputData[c], m_ppfOutputData[c], m_kiDataLength, 1e-3);
	//}

	TEST_FIXTURE(PpmData, ZeroInput)
	{
		for (int c = 0; c < m_iNumChannels; c++)
			CVector::setZero(m_ppfInputData[c], m_kiDataLength);

		process();

		for (int c = 0; c < m_iNumChannels; c++)
			CHECK_ARRAY_CLOSE(m_ppfInputData[c], &m_ppfOutputData[c][0], m_iOutputPoints, 1e-3F);
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