
#include <iostream>
#include <ctime>

#include "MUSI8903Config.h"

#include "AudioFileIf.h"
#include "Vibrato.h"

using std::cout;
using std::endl;

// local function declarations
void    showClInfo ();

/////////////////////////////////////////////////////////////////////////////////
// main function
int main(int argc, char* argv[])
{
	std::string             sInputFilePath,                 //!< file paths
							sOutputFilePath,
							sInput2TxtPath;

    static const int        kBlockSize				= 1024;

    clock_t                 time					= 0;

	float                   **ppfInputAudioData		= 0,
							**ppfOutputAudioData	= 0,
							mod_freq				= 0.F,
							mod_amp_secs			= 0.F,
							delay_width_secs		= 0.1F;

    CAudioFileIf            *phAudioFile			= 0;
	Vibrato					*vibrato				= 0;
	Error_t					error_check;
    std::ofstream           infile, outfile;
    CAudioFileIf::FileSpec_t stFileSpec;
    outfile.precision(15);

    showClInfo ();

    //////////////////////////////////////////////////////////////////////////////
    // parse command line arguments
	try
	{
		switch (argc)
		{
		case 1: cout << "Too few arguments. Enter Filename." << endl;
			exit(0);
			break;
		case 2: sInputFilePath = argv[1];
			break;
		case 3: sInputFilePath = argv[1];
			mod_freq = stof(argv[2]);
			break;
		case 4: sInputFilePath = argv[1];
			mod_freq = stof(argv[2]);
			mod_amp_secs = stof(argv[3]);
			break;
		default: cout << "Too many parameters. Check what you're entering." << endl;
			exit(0);
		}
	}
	catch (exception &exc)
	{
		cerr << "Invalid arguments passed. Please use the correct formatting for running the program." << endl;
	}

    //////////////////////////////////////////////////////////////////////////////
    // open the input wave file
    CAudioFileIf::create(phAudioFile);
    phAudioFile->openFile(sInputFilePath, CAudioFileIf::kFileRead);
    if (!phAudioFile->isOpen())
    {
        cout << "Wave file open error!";
        return -1;
    }
    phAudioFile->getFileSpec(stFileSpec);

	sOutputFilePath = sInputFilePath + "output.txt";
	sInput2TxtPath = sInputFilePath + "input.txt";

    //////////////////////////////////////////////////////////////////////////////
    // open the output text file
    outfile.open (sOutputFilePath.c_str(), std::ios::out);
    if (!outfile.is_open())
    {
        cout << "Text file open error!";
        return -1;
    }
	//REMOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOVE THIS AFTER MATLAB CHECK IS DONE
	infile.open(sInput2TxtPath.c_str(), std::ios::out);
	if (!infile.is_open())
	{
		cout << "Text file open error!";
		return -1;
	}

    //////////////////////////////////////////////////////////////////////////////
    // allocate memory
	ppfInputAudioData = new float*[stFileSpec.iNumChannels];
	ppfOutputAudioData = new float*[stFileSpec.iNumChannels];
    
	for (int i = 0; i < stFileSpec.iNumChannels; i++)
	{
		ppfInputAudioData[i] = new float[kBlockSize];
		ppfOutputAudioData[i] = new float[kBlockSize];
	}
    time                    = clock();
    //////////////////////////////////////////////////////////////////////////////
	// do processing
    
    /*
    float** tempInp = new float*[1];
    tempInp[0] = new float[10000];
    for (int i = 0; i < 10000; i++) {
        tempInp[0][i] = 0;
    }
    tempInp[0][0] = 1;
    float** tempOut = new float*[1];
    tempOut[0] = new float[10000];
    */
    
	error_check = Vibrato::create(vibrato, stFileSpec.fSampleRateInHz, stFileSpec.iNumChannels);
    if (error_check == kUnknownError) {
		cerr << "Runtime error. Memory issues." << endl;
        return -1;
    }
	
	/*Here delay_width_secs is selected by the developer to set the maximum delay length that he wants the
	user to be able to choose for the vibrato modulation amplitude.*/
	error_check = vibrato->init(mod_freq, delay_width_secs, mod_amp_secs);
	if (error_check == kFunctionInvalidArgsError) {
		cerr << "Invalid parameters: One or more parameters is out of bounds. Please check your parameters." << endl;
        return -1;
    }
    /*
    vibrato->process(tempInp, tempOut, 10000);
    for (int i = 0; i < 10000; i++) {
        
        for (int j = 0; j < 1; j++)
        {
            outfile << tempOut[j][i] << " ";
            infile << tempInp[j][i] << " ";
        }
        outfile << endl;
        infile << endl;
    }
    */
    
	cout << "Processing....." << endl;
	while (!phAudioFile->isEof())
	{
		long long iNumFrames = kBlockSize;
		phAudioFile->readData(ppfInputAudioData, iNumFrames);
		vibrato->process(ppfInputAudioData, ppfOutputAudioData, iNumFrames);
		for (int i = 0; i < iNumFrames; i++)
		{
			for (int j = 0; j < stFileSpec.iNumChannels; j++)
			{
				outfile << ppfOutputAudioData[j][i] << " ";
				infile << ppfInputAudioData[j][i] << " ";
			}
			outfile << endl;
			infile << endl;
		}
	}
    
    cout << "Processing done in: \t"    << (clock()-time)*1.F/CLOCKS_PER_SEC << " seconds." << endl;
    
    //////////////////////////////////////////////////////////////////////////////
    // clean-up
    CAudioFileIf::destroy(phAudioFile);
	Vibrato::destroy(vibrato);
    outfile.close();
	infile.close();

	for (int i = 0; i < stFileSpec.iNumChannels; i++)
	{
		delete[] ppfInputAudioData[i];
		delete[] ppfOutputAudioData[i];
	}
    delete [] ppfInputAudioData;
	delete[] ppfOutputAudioData;
    ppfInputAudioData = 0;
	ppfOutputAudioData = 0;
    
    /*
    delete[] tempInp[0];
    delete[] tempInp;
    delete[] tempOut[0];
    delete[] tempOut;
    */
     
    return 1;
}


void     showClInfo()
{
    cout << "GTCMT MUSI8903" << endl;
    cout << "(c) 2016 by Siddharth and Ashis" << endl;
    cout  << endl;

    return;
}

