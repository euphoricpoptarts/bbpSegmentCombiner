// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <math.h>
#include <iostream>

#define uint64 unsigned long long

const uint64 baseSystem = 1024;
const char * completionPathFormat = "completed/segmented%dDigit%lluSegment%dBase1024Complete.dat";

struct sJ {
	double s[7] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
};

void sJAdd(sJ* addend, const sJ* augend) {
	for (int i = 0; i < 7; i++) {
		addend->s[i] += augend->s[i];
		if (addend->s[i] >= 1.0) addend->s[i] -= 1.0;
	}
}

uint64 finalizeDigit(sJ input, uint64 n) {
	double reducer = 1.0;

	//unfortunately 64 is not a power of 16, so if n is < 2
	//then division is unavoidable
	//this division must occur before any modulus are taken
	if (n == 0) reducer /= 64.0;
	else if (n == 1) reducer /= 4.0;

	//logic relating to 1024 not being a power of 16 and having to divide by 64
	int loopLimit = (2 * n - 3) % 5;
	if (n < 2) n = 0;
	else n = (2 * n - 3) / 5;

	double trash = 0.0;
	double *s = input.s;
	for (int i = 0; i < 7; i++) s[i] *= reducer;

	if (n < 16000) {
		for (int i = 0; i < 5; i++) {
			n++;
			double sign = 1.0;
			double nD = (double)n;
			if (n & 1) sign = -1.0;
			reducer /= (double)baseSystem;
			s[0] += sign * reducer / (4.0 * nD + 1.0);
			s[1] += sign * reducer / (4.0 * nD + 3.0);
			s[2] += sign * reducer / (10.0 * nD + 1.0);
			s[3] += sign * reducer / (10.0 * nD + 3.0);
			s[4] += sign * reducer / (10.0 * nD + 5.0);
			s[5] += sign * reducer / (10.0 * nD + 7.0);
			s[6] += sign * reducer / (10.0 * nD + 9.0);
		}
	}

	//multiply sJs by coefficients from Bellard's formula and then find their fractional parts
	double coeffs[7] = { -32.0, -1.0, 256.0, -64.0, -4.0, -4.0, 1.0 };
	for (int i = 0; i < 7; i++) {
		s[i] = modf(coeffs[i] * s[i], &trash);
		if (s[i] < 0.0) s[i]++;
	}

	double hexDigit = 0.0;
	for (int i = 0; i < 7; i++) hexDigit += s[i];
	hexDigit = modf(hexDigit, &trash);
	if (hexDigit < 0) hexDigit++;

	//16^n is divided by 64 and then combined into chunks of 1024^m
	//where m is = (2n - 3)/5
	//because 5 may not evenly divide this, the remaining 4^((2n - 3)%5)
	//must be multiplied into the formula at the end
	for (int i = 0; i < loopLimit; i++) hexDigit *= 4.0;
	hexDigit = modf(hexDigit, &trash);

	//shift left by 8 hex digits
	for (int i = 0; i < 12; i++) hexDigit *= 16.0;
	printf("hexDigit = %.8f\n", hexDigit);
	return (uint64)hexDigit;
}

int combineFromFiles(uint64 * n, double * totalTime, sJ * resultTotal) {

	uint64 hexDigitPosition;
	int totalSegments;
	std::cout << "Input hexDigit (1-indexed) with completed segments:" << std::endl;
	std::cin >> hexDigitPosition;
	std::cout << "Input number of total segments:" << std::endl;
	std::cin >> totalSegments;
	//subtract 1 to convert to 0-indexed
	hexDigitPosition--;

	*n = hexDigitPosition;

	uint64 sumEnd = 0;

	//convert from number of digits in base16 to base1024
	//because of the 1/64 in formula, we must subtract log16(64) which is 1.5, so carrying the 2 * (digitPosition - 1.5) = 2 * digitPosition - 3
	//this is because division messes up with respect to modulus, so use the 16^digitPosition to absorb it
	if (hexDigitPosition < 2) sumEnd = 0;
	else sumEnd = ((2LLU * hexDigitPosition) - 3LLU) / 5LLU;

	for (int i = 0; i < totalSegments; i++) {
		char buffer[256];
		snprintf(buffer, sizeof(buffer), completionPathFormat, totalSegments, sumEnd, i);
		FILE * cacheF;
		fopen_s(&cacheF, buffer, "r");

		if (cacheF == NULL) {
			std::cout << "Could not open " << buffer << "!" << std::endl;
			std::cout << "Please make sure all segments are complete!" << std::endl;
			return 1;
		}

		int readLines = 0;

		sJ segmentResult;
		double segmentTime;

		readLines += fscanf_s(cacheF, "%la", &segmentTime);
		for (int i = 0; i < 7; i++) readLines += fscanf_s(cacheF, "%la", &segmentResult.s[i]);
		fclose(cacheF);
		//8 lines of data should have been read, 1 time and 7 data points
		if (readLines != 8) {
			std::cout << "Data reading failed!" << std::endl;
			return 1;
		}
		sJAdd(resultTotal, &segmentResult);
		*totalTime += segmentTime;
	}
	return 0;
}

int main()
{
	sJ result;
	double totalTime = 0;
	uint64 hexDigitPosition = 0;

	if (combineFromFiles(&hexDigitPosition, &totalTime, &result)) {
		return 1;
	}

	uint64 hexDigit = finalizeDigit(result, hexDigitPosition);

	printf("pi at hexadecimal digit %llu is %012llX\n",
		hexDigitPosition + 1, hexDigit);

	printf("Total GPU time for all segments: %.8f seconds\n", totalTime);

    return 0;
}