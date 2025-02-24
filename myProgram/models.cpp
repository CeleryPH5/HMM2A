

#include <cstdlib>
#include <ctime>
#include <cstring>
#include <iostream>
#include <fstream>
#include <conio.h>	

#include "models.h"

using namespace std;





//******************************************************************************
//******************************************************************************
//
//  Parameter setting for the storage capacity
//
//******************************************************************************
//******************************************************************************

//The maximum capacity (maximum number of characters allowed) 
//		for the storing the vocabulary set
#define VocabularyStorageLimit 20000

//The maximum capacity (maximum number of characters allowed) 
//		for storing the corrupted words during spelling recognition
#define NoisyWordsStorageLimit 15000





//******************************************************************************
//******************************************************************************
//
//  Parameter settings (in global variables) for the Spelling model
//
//******************************************************************************
//******************************************************************************
double prSpRepeat = 0.2;
//The probability of repeating the current cognitive state again as the next state
//we make it 0.2 initially, but you can try different values to see the effects.

double prSpMoveOn = 0.8;
//The probability of moving from the current cognitive state to other states
//	as the next state
//we make it 0.8 initially, but you can try different values to see the effects.

//********************************************************
//Note that prSpRepeat + prSpMoveon should always equal 1
//********************************************************

double spDegenerateTransitionDistancePower = 2;
//The likelihood of moving from the cognitive state of typing some character in a word 
//to the next cognitive state is proportion to the inverse of 
//(spDegenerateTransitionDistancePower) to the 
//(the distance between the current state to the next state)th power.
//In the setting of the original spelling model in the project,
//we make it 2, but you can try different values to see the effects.

double spDegenerateInitialStateDistancePower = 2;
//The likelihood of some character in a word as the initial cognitive state
//is proportion to the inverse of 
//(spDegenerateInitialStateDistancePower) to the 
//(the position of the character in the word)th power.
//In the setting of the original spelling model in the project,
// spDegenerateInitialStateDistancePower and spDegenerateTransitionDistancePower
//have the same value, but you can make them different to see the effects
//By default, we make it 2, but you can try different values to see the effects.


void setParametersSpellingModel()
{
	cout << endl
		<< "Reset the parameters of the spelling model:" << endl << endl;

	cout << "Reset P_moveOn, the probability of moving on" << endl
		<< "   from the current cognitive state to other states:" << endl
		<< "P_moveOn = ";
	cin >> prSpMoveOn;

	prSpRepeat = 1 - prSpMoveOn;
	cout << endl
		<< "Automatically reset P_repeat to 1-P_moveOn" << endl
		<< "P_repeat = " << prSpRepeat << endl;

	cout << endl
		<< "Do you want to change the deg_kb? (y or n): ";

	char option;
	cin >> option;
	
	if (option == 'y')
	{
		cout << "Reset deg_sp, the power coefficient governing the probability of " << endl
			<< "   skipping from the current cognitive state to other states:" << endl
			<< "deg_sp = ";

		cin >> spDegenerateTransitionDistancePower;

		spDegenerateInitialStateDistancePower = spDegenerateTransitionDistancePower;
	}
}

void displayParametersSpellingModel()
{
	cout << endl
		<< "Parameter values of the spelling model:" << endl
		<< "   P_repeat  = " << prSpRepeat << endl
		<< "   P_moveOn  = " << prSpMoveOn << endl
		<< "   deg_sp = " << spDegenerateTransitionDistancePower << endl << endl;
}

//******************************************************************************
//******************************************************************************
//
//  Parameter settings (in global variables) for the keyboard model
//
//******************************************************************************
//******************************************************************************

double prKbHit = 0.6;
//The probability that you want to type a character and you do successfully make it
//In the setting of the original keyboard model in the project,
//we make it 0.9, but you can try different values to see the effects.

double prKbMiss = 0.4;
//The sum of probabilities that you want to type a character but end in touching 
//a different character.
//we make it 0.1, but you can try different values to see the effects.

//*******************************************************
//Note that prKbHit + prKbMiss should always equal 1
//*******************************************************

//In the setting of the original keyboard model in the project,
//we make it 0.2, but you can try different values to see the effects.


double kbDegenerateDistancePower = 2;
//The likelihood you want to type a character but end in touching a different character
//is proportion to the inverse of 
//(kbDegenerateDistancePower) raised to the (distance between them) th power
//In the setting of the original keyboard model in the project,
//we make it 2, but you can try different constants to see the effects.


void setParametersKbModel()
{
	cout << endl
		<< "Reset the parameters of the keyboard model:" << endl << endl;

	cout << "Reset P_hit, the probability of hitting" << endl
		<< "   the right character wanted on the keyboard:" << endl
		<< "P_hit = ";
	cin >> prKbHit;

	prKbMiss = 1 - prKbHit;
	cout << endl
		<< "Automatically reset P_miss to 1-P_hit" << endl
		<< "P_miss = " << prKbMiss << endl;

	cout << endl
		<< "Do you want to change the deg_kb? (y or n): ";

	char option;
	cin >> option;

	if (option == 'y')
	{
		cout << "Reset deg_kb, the power coefficient governing the probability of " << endl
			<< "   skipping from the current cognitive state to other states:" << endl
			<< "deg_kb = ";

		cin >> kbDegenerateDistancePower;
	}
}

void displayParametersKbModel()
{
	cout << endl
		<< "Parameter values of the keyboard model:" << endl
		<< "   P_hit  = " << prKbHit << endl
		<< "   P_miss = " << prKbMiss << endl
		<< "   deg_kb = " << kbDegenerateDistancePower << endl << endl;
}


//******************************************************************************
//******************************************************************************
//
//  Trace flag and the alphabet table
//
//******************************************************************************
//******************************************************************************
bool traceON = true;   // output tracing messages



/************************************************************************/
//Calculate and return the probability of charGenerated actually typed
//given charOfTheState as the underlying cognitive state.
/************************************************************************/

double prCharGivenCharOfState(char charGenerated, char charOfTheState)
{   // KEYBOARD STATE
	// CharGenerated = What we actually touched (typed)
	// CharOfTheState = What we want to type in our mind (our cognitive state)

	int distanceASCII = abs(charOfTheState - charGenerated);

	if (distanceASCII > 13)		//Determine the right distance since the 1D keyboard is a ring
		distanceASCII = 26 - distanceASCII;

	if (distanceASCII == 0)		//In case, CharGenerated == CharOfTheState
		return prKbHit;	//	we hit the exactly right key and the probability is prKbHit
	else
	{
		//first calculate the sum of ratios of probabilities of
		//	hitting the other 25 keys different from charOfTheState.
		//Note that these keys are 1 to 25 characters clockwise away from charOfTheState 
		double sumOfRatios = 0;
		for (int i = 1; i < 26; i++)
		{
			int distance;

			//Determine the real distance since the 1D keyboard is a ring
			//	need to check both clockwise and counter-clockwie
			if (i <= 13)
				distance = i;
			else
				distance = 26 - i;

			double ratio;
			ratio = pow(kbDegenerateDistancePower, -distance);
			sumOfRatios += ratio;
		}

		double currentRatio;
		currentRatio = pow(kbDegenerateDistancePower, -distanceASCII);

		return prKbMiss * (currentRatio / sumOfRatios);
	}

}



/************************************************************************/
//The following function implement part of the spelling model for
// spelling a word with sizeOfTable characters,
//This function
//(i) calculates for each cognitive state excluding the special states I and F,
//     the probability of that cognitive state being the first cognitive state
//     after the transition out of the special state I, and
//(ii) stores these probabilities in a prTable array of sizeOfTable elements.
//Note that the value of the parameter sizeOfTable should be
//     exactly the number characters in the word.
/************************************************************************/

void getPrTableForPossibleInitialStates(double prTable[], int sizeOfTable)
{
	//It is a word of sizeOfTable characters:
	//     i.e. the number of character states is sizeOfTable.
	//     let's index these characters from 0 to sizeOfTable-1.
	//

	//First calculate the sum of ratios of probabilities of
	//     going from the special I state into these character states.
	//This allows you to calculate the scaling factor to determine the probabilities.


	//Second, for each character state calculate the probability
	//     transitioning from the special I state into the character state.

	//**************************************************
	//Replace the following with your own implementation
	//**************************************************
	prTable[sizeOfTable];
	double sum_exponantialDegrade = 0;
	for (int i = 1; i <= sizeOfTable; i++) // initial the sum of exponantial degrade to 0
		sum_exponantialDegrade += pow((1.0 / spDegenerateInitialStateDistancePower), i); // the sum of exponantial degrades
	const double scalingConstant = 1 / sum_exponantialDegrade; // const x which is 1 divided by the sum of degrades
	for (int j = 0; j < sizeOfTable; j++)
		prTable[j] = scalingConstant * (pow((1.0 / spDegenerateInitialStateDistancePower), j + 1)); // const x multiply by degrade
}




/************************************************************************/
//The following function implement part of the spelling model for
// spelling a word with sizeOfTable-1 characters,
// i.e. sizeOfTable should be 1 + the number characters in the word.
//This function
//(i) calculates for each actual cognitive state for a word
//     (excluding the special I state but including the special F state),
//     the probability of that cognitive state being the next cognitive state
//     given currentState as the index of the current state in the word, and
//(ii) stores these probabilities in a transitionPrTable array of sizeOfTable elements.
//Note that the value of the parameter sizeOfTable should be
//     1 + the number characters in the word.
/************************************************************************/

void getPrTableForPossibleNextStates
(double transitionPrTable[], int sizeOfTable, int currentState)
{
	//We are working on a word of sizeOfTable-1 characters:
	//     i.e. the number of character states is sizeOfTable-1.
	//Index these character states from 0 to sizeOfTable-2 respectively, while
	//     the index of the special final state F is sizeOfTable-1.
	//currentState is the index of the current state in the word

	//First calculate the sum of ratios of probabilities of
	//     going from the current state into the other down-stream states down in word
	//     including all the down-stream character states and the
	//     special F final state.
	//This allows you to calculate the scaling factor to determine the probabilities.

	//Second, for each state (excluding the special I state)
	//     calculate the probability of
	//     transitioning from the current state into the character state
	//     and store the probability into the table.


	//**************************************************
	//Replace the following with your own implementation
	//**************************************************
	double sum_exponantialDegrade = 0;
	double exponentialDegrade = 0;
	int distance;
	transitionPrTable[sizeOfTable];
	for (int i = 0; i < sizeOfTable; i++)
	{
		distance = i - currentState; // the distance from the current state
		sum_exponantialDegrade += (distance > 0) ? pow((1.0 / spDegenerateTransitionDistancePower), i) : 0; // the sum of  degrade of all the charecters behind the current state
	}
	const double scalingConstant = prSpMoveOn / sum_exponantialDegrade; // const x which is possibility to move on divided by sum of degrade
	for (int j = 0; j < sizeOfTable; j++)
	{
		if (j == currentState)
			transitionPrTable[j] = prSpRepeat;// the possibility to repeat
		else
		{
			distance = j - currentState;
			exponentialDegrade = (distance > 0) ? pow((1.0 / spDegenerateTransitionDistancePower), j) : 0;
			transitionPrTable[j] = scalingConstant * exponentialDegrade; // the possibility of going on to the charecters after the current state
		}
	}
}



/************************************************************************/
//  Programming #2 
//
//  List below are function prototypes of functions given  or required 
//		to be implemented in Programming #2 
//
/************************************************************************/

/************************************************************************/
//Given the probabilities (of sizeOfTable elements) stored in prTable,
//	try to randomly take a sample out of sizeOfTable elements
//	according to the probabilities of sizeOfTable elements;
//Return the index of the non-deterministically sampled element.
/************************************************************************/



int take1SampleFrom1PrSpace(double prTable[], int sizeOfTable) {
    // example probability table: {0.8, 0.1, 0.1} [HIS] (Numbers were picked arbitrarily)
    // 0.8 represents the probability of selecting the first character,
    // 0.1 for the second, and 0.1 for the third. 
    
    int idx;
    double prSum = 0;
    
    // Step 1: Sum all the probabilities to ensure they add up to ~1
    for (idx = 0; idx < sizeOfTable; idx++) {
        prSum += prTable[idx];
    }
    
    // Check if the sum of the probabilities is close to 1
    if (prSum < 0.999 || prSum > 1.001) {
        cout << "Error in probability sampling" << endl
             << "Sum of probabilities: " << prSum << endl;
    }

    // Step 2: Create cumulative probability intervals
    // This defines the ranges where each element can be selected
    double prAccum = 0;
    double* prIntervals = new double[sizeOfTable];
    
    for (idx = 0; idx < sizeOfTable; idx++) {
        prAccum += prTable[idx];  // Accumulate probabilities
        prIntervals[idx] = prAccum;  // Store cumulative sum
    }
    
    // At this point:
    // prIntervals[0] = [0,0.8]
    // prIntervals[1] = [0.8,0.9]
    // prIntervals[2] = [0.9,1.0]
    
    // Step 3: Generate a random number between 0 and 1
    int randVal = rand() % 1001;  // Random integer between 0 and 1000
    double tempVal = randVal / 1000.0;  // Convert to a float between 0.0 and 1.0
    
    // Step 4: Check which interval this random number falls into
	// So if it is like 0.76 then it will fall in the range of prIntervals[0] which is [0,0.8]
    bool taken = false;
    for (idx = 0; idx < sizeOfTable && !taken; idx++) {
        // If the random value falls within the current interval
        if (tempVal <= prIntervals[idx]) {
            delete[] prIntervals;
            return idx;  // Return the index of the selected item
        }
    }

    // Edge case: If no match found, return the last index
    delete[] prIntervals;
    return sizeOfTable - 1;
}



/************************************************************************/
//
//Given the character to type (charToType) 
//	(assuming that the 1D keyboard of 26 keys is used),
//	(assuming that prTable[] for storing 26 prbabilities),
//	calculate pr(charGenerated = 'a' | charToType),
//	calculate pr(charGenerated = 'b' | charToType), 
//	...
//	calculate pr(charGenerated = 'z' | charToType), and
//	store these probabilities in prTable.
/************************************************************************/
void getKeyboardProbabilityTable(char charToType, double prTable[]) {
	// Suppose we want to type in c
	int tableSize = 26;
	char map[] = "abcdefghijklmnopqrstuvwxyz";
	
	double totalProb = 0;

	for (int i = 0; i < tableSize; i++) {
		prTable[i] = prCharGivenCharOfState(map[i], charToType);
		totalProb += prTable[i];
	}
	/*
		This creates the probability of typing a character (ie. G), given you were trying to type another character (ie. C)
 		If we only have a,b,c and the target is 'c' then it may look like this
   		- prCharGivenCharOfState('a', 'c') → 0.2
		- prCharGivenCharOfState('b', 'c') → 0.3
		- prCharGivenCharOfState('c', 'c') → 0.5
 	
		Which symbolizes 
  		- prTable[0] = 0.2 (probability of mistyping 'a' when trying to type 'c')
		- prTable[1] = 0.3 (probability of mistyping 'b' when trying to type 'c')
		- prTable[2] = 0.5 (probability of correctly typing 'c')
  
  	*/

	for (int i = 0; i < tableSize; i++) {
		prTable[i] /= totalProb;
	}

	//This function just normalizes it to be == 1, there would be no change in our baby example of 
	/*

  		- prTable[0] = 0.2 (probability of mistyping 'a' when trying to type 'c')
		- prTable[1] = 0.3 (probability of mistyping 'b' when trying to type 'c')
		- prTable[2] = 0.5 (probability of correctly typing 'c')

  		Since it sums to 1, but in the case that it doesn't, we need to adjust it.
 
 	*/
}

/************************************************************************/
//Simulate the keyboard model:
//Given the charToTye, simulate what character may actually
//	be typed and return it as the result.
/************************************************************************/
char typeOneChar(char charToType) {
	char alphabet[] = "abcdefghijklmnopqrstuvwxyz";
	double prTable[26];
	// This gets the probability table from the second function
	getKeyboardProbabilityTable(charToType, prTable);
	// This returns a random letter typed from the probability table
	return alphabet[take1SampleFrom1PrSpace(prTable, 26)];
}



/************************************************************************/
//Simulate the combination of the spelling model and the keyboard model:
//Given a word stored in the word array, simulate what may actually
//	be typed and store the result in the output array.
//maxOutput specifies the capacity limit of the output array, by default it is 100.
//When traceON is true (by default it is false), extra outputs are provided as traces.
/************************************************************************/
void typeOneWord(char word[], char output[], bool traceON, int maxOutput)
{ 
	int wordLen = strlen(word);
	int currentState = 0; 
	int outputIndex = 0;
	output[outputIndex++] = '\0'; // Null-terminate the output array

	// Transition probability table
	double* transitionPrTable = new double[wordLen + 1];

	while (currentState < wordLen && outputIndex < maxOutput - 1) {
		getPrTableForPossibleNextStates(transitionPrTable, wordLen + 1, currentState);

		// Simulate the next state based on transition probabilities
		double randVal = (double)rand() / RAND_MAX;
		double cumulative = 0.0;
		int nextState = currentState;

		for (int i = 0; i <= wordLen; i++) {
			cumulative += transitionPrTable[i];
			if (randVal <= cumulative) {
				nextState = i;
				break;
			}
		}

		if (nextState == currentState) { //looper to go through each character
			continue;
		}
		else if (nextState == wordLen) {
			break;
		}
		else {
			output[outputIndex++] = word[nextState];
			if (traceON) {
				cout << "State " << currentState << " to state " << nextState
					<< " (typed '" << word[nextState] << "')" << endl;
			}
			currentState = nextState;
		}
	}

	output[outputIndex] = '\0'; // Null-terminate the output array

	if (traceON) {
		cout << "Final output: " << output << endl;
	}

	delete[] transitionPrTable;
}// end of the function
/*******************************************************************/
