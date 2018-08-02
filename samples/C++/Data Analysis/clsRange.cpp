#include "stdafx_seq2text.h"
#include "clsRange.hpp"
#include "algUtils.hpp"
#include <memory>
#include <algorithm>
#include <vector>
#include <tuple>

namespace s2t{
namespace impl{

using namespace std;
ClassifierRange::ClassifierRange()
{
}
ctext_t __stdcall ClassifierRange::name() const
{
	return "range";
}
int16_t __stdcall ClassifierRange::classify(const ISequences* sequences, IClassification* result) const
{
	// We'll use the Aroon Oscillator to identify the absense of up/down
	// trend, i.e. a side trend in the data.
	//
	// When the Aroon Up and Aroon Down indicator move towards the centerline (50), 
	// then the data is entering into a consolidation period.
	// A positive or negative threshold can be used to define the strength of the 
	// trend. For example, a surge above +50 would reflect a strong upside move, 
	// while a plunge below -50 would indicate a strong downside move.
	const size_t LOOKBACK = 10;

	// Interate over the sequences
	int16_t mainSeqScore = 0;
	size_t sequencesCount = sequences->size();
	for (size_t s = 0; s < sequencesCount; s++)
	{
		const ISequence* sequence = sequences->get(s);
		if (sequence == nullptr)
			continue; // Something went wrong with this sequence

		size_t valuesCount = sequence->size();
		if (valuesCount<MIN_RANGE)
			return 0; // The sequence is too short

		// We'll have to find the sequence max/min to estimate its spread
		value_t seqMin = numeric_limits<value_t>::max();
		value_t seqMax = numeric_limits<value_t>::lowest();

		// Save the sequence values into an array for faster access
		unique_ptr<value_t[]> values(new value_t[valuesCount]);
		for (size_t i = 0; i < valuesCount; i++)
		{
			// Just copy the value to the buffer of doubles
			values[i] = sequence->getValue(i);

			// Possibly update the sequence min/max value information
			seqMin = std::min(values[i], seqMin);
			seqMax = std::max(values[i], seqMax);
		}

		// Prepare the buffer of double to place Aroon's values
		// It has the same size as the original data
		unique_ptr<value_t[]> aroon(new value_t[valuesCount]);

		// Aroon Oscillator calculation for N=25
		// Aroon Up = 100 x (25 - Items Since 25-item High)/25
		// Aroon Down = 100 x(25 - Items Since 25-item Low)/25
		// Aroon Oscillator = AroonUp - AroonDown
		for (size_t i = 0; i < valuesCount; i++)
		{
			size_t minIndex = i;
			size_t maxIndex = i;
			value_t runMinimum = numeric_limits<value_t>::max();
			value_t runMaximum = numeric_limits<value_t>::lowest();
			size_t lookBackIndex = (i >= LOOKBACK) ? (i - LOOKBACK) : 0;

			// For every value do a bad thing - look into the past
			// for N elements and find the distance to this window's
			// max/min values. Could be optimized a bit to get less 
			// back loops
			for (size_t j = lookBackIndex; j <= i; j++)
			{
				value_t curValue = values[j];
				if (curValue >= runMaximum)
				{
					runMaximum = curValue;
					maxIndex = j;
				}
				if (curValue <= runMinimum)
				{
					runMinimum = curValue;
					minIndex = j;
				}
			}
			value_t AroonUp = 100.0 * ((double)LOOKBACK - (i - maxIndex)) / (double)LOOKBACK;
			value_t AroonDown = 100.0 * ((double)LOOKBACK - (i - minIndex)) / (double)LOOKBACK;
			aroon[i] = (AroonUp - AroonDown);
		}

		// Based on the values calculated by Aroon build the resulting intervals
		int16_t seqScore = fillIntervals(sequence, seqMax, seqMin, values.get(), aroon.get(), valuesCount, result);
		if(s==0)
			mainSeqScore = seqScore;
	}
	return mainSeqScore;
}
/**
Summary:
	This method build a classification based on the calculated Aroon values
*/
int16_t ClassifierRange::fillIntervals(const ISequence* sequence, value_t seqMax, value_t seqMin,
									value_t* values, value_t* aroon, size_t valuesCount,
									IClassification* result) const
{
	// Reduce the aroon values by eliminating the meaningless points, ie noise
	// Present the vector of Aroon values as the 2d geometrical line
	std::vector<Point2D> points, reducedPoints;
	for (size_t i = 0; i< valuesCount; i++)
	{
		points.push_back({ (value_t)i, aroon[i]});
	}

	// Reduce this line with Douglas-Peucker leaving only
	// its 'significant' points
	/*Algorithms::douglasPeuckerReduction(points, (value_t)2, reducedPoints);

	// Check the resulting reduced line
	size_t reducedCount = reducedPoints.size();
	if (reducedCount<2)
		return;

	// Based on the reduced line adjust the original Aroon
	// values array by replacing the meaningless points with
	// with the previous value of the meaningful point ie making
	// 'steps'
	size_t prevIndex = (size_t)reducedPoints[0].x;
	value_t prevValue = aroon[prevIndex];
	for (size_t i = 1; i < reducedCount; i++)
	{
		Point2D &point = reducedPoints[i];
		size_t curIndex = (size_t)point.x;
		for (size_t j = prevIndex; j < curIndex; j++)
		{
			aroon[j] = prevValue;
		}
		prevIndex = curIndex;
		prevValue = aroon[curIndex];
	}*/
	/* Aroon values smoothing
	value_t prevValue = aroon[0];
	for (size_t i = 1; i < valuesCount-1; i++)
	{
		value_t curValue = aroon[i];
		value_t nextValue = aroon[i+1];
		aroon[i] = (prevValue + curValue + nextValue)/3.0;

		prevValue = curValue;
		result->append(sequence, i - 1, i, (int16_t)aroon[i], 1, "");
	}
	*/

	// Now build the intervals based on the Aroon 
	// oscillator values
	size_t leftIndex = -1;
	value_t leftValue = 0.0;
	value_t seqStagnationStrength = 0;
	size_t sumIntervalLen = 0;
	for (size_t i = 0; i< valuesCount; i++)
	{
		// Take the Aroon indicator value
		int16_t arValue = (int16_t)floor(aroon[i]);

		// Take the sequence value at this position
		value_t value = values[i];

		// Only |Aroon|<50% show the ranging behaviour
		// otherwise it might be a trend
		if (abs(arValue) < 50)
		{
			// We'll need the interval's min/max to 
			// estimate it's spread
			value_t intervalMin = value;
			value_t intervalMax = value;

			// Left edge of the interval is set
			leftIndex = i;

			// Now look for the right edge
			size_t rightIndex = i;
			i++;
			for (; i < valuesCount; i++)
			{
				value = values[i];
				arValue = (int16_t)floor(aroon[i]);

				// Are we still outside the trend?
				if (abs(arValue) >= 50)
					break; // The end of the 'valley'

				intervalMin = std::min(value, intervalMin);
				intervalMax = std::max(value, intervalMax);

				// Move the right edge
				rightIndex = i;
			}
            while (rightIndex - leftIndex >= MIN_RANGE)
            {
			    size_t segLength = rightIndex - leftIndex;

			    // How 'fat' is this range? We don't want a volatile hell to get classified
			    // Only nice and slim guys will pass
			    value_t segSpread = 100 * ((seqMax == seqMin) ? 1 : ((intervalMax - intervalMin) / (seqMax - seqMin)));

			    if (segLength >= MIN_RANGE && segSpread < 20.0) // Channel is long enough and slim
			    {
				    int16_t score = (int16_t)ceil(100.0 - segSpread); // The slimmer the better
				    int16_t groupId = (int16_t)round(score / 50);
				    seqStagnationStrength += abs(score)*segLength;
				    sumIntervalLen += segLength;

				    result->append(sequence, leftIndex, rightIndex, 
								    score, groupId, 
								    feature, FK_FROMTO | FK_MEDIAN | FK_PCT_CHNG,
								    nullptr, nullptr, 0);
                    break;
			    }
                else
                {
                    rightIndex--; // try to make the interval smaller by more tight
                    intervalMin = numeric_limits<value_t>::max();
                    intervalMax = numeric_limits<value_t>::lowest();
                    for (size_t j = leftIndex; j <= rightIndex; j++)
                    {
                        // Possibly update the sequence min/max value information
                        intervalMin = std::min(values[j], intervalMin);
                        intervalMax = std::max(values[j], intervalMax);
                    }
                }
            }
		}
	}

	if (sumIntervalLen>0)
	{
		seqStagnationStrength /= sumIntervalLen;
		seqStagnationStrength *= (2*(value_t)sumIntervalLen / (value_t)valuesCount);
		seqStagnationStrength = std::max(seqStagnationStrength, 10.0);
	}
	return (int16_t)(seqStagnationStrength);
}
}}