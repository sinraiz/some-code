#pragma once
#include <seq2text.h>
#include <vector>


namespace s2t{
namespace impl{

class ClassifierRange : public IClassifier
{
public:
	ClassifierRange();
public:
	virtual ctext_t __stdcall name() const override;
	virtual int16_t __stdcall classify(const ISequences* sequences, IClassification* result) const override;
protected:
	virtual int16_t fillIntervals(const ISequence* sequence, value_t seqMax, value_t seqMin,
								value_t* values, value_t* aroon, size_t valuesCount, 
								IClassification* result) const;
private:
	const size_t MIN_RANGE = 5;
	const ctext_t feature = "{{stagnation}}";
};

}}