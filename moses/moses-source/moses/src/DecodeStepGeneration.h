// $Id: DecodeStepGeneration.h 2939 2010-02-24 11:15:44Z jfouet $

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_DecodeStepGeneration_h
#define moses_DecodeStepGeneration_h

#include "DecodeStep.h"

namespace Moses
{

class GenerationDictionary;
class Phrase;
class ScoreComponentCollection;

//! subclass of DecodeStep for generation step
class DecodeStepGeneration : public DecodeStep
{
public:
	DecodeStepGeneration(GenerationDictionary* dict, const DecodeStep* prev);

  //! returns phrase table (dictionary) for translation step 
  const GenerationDictionary &GetGenerationDictionary() const;

  virtual void Process(const TranslationOption &inputPartialTranslOpt
                              , const DecodeStep &decodeStep
                              , PartialTranslOptColl &outputPartialTranslOptColl
                              , TranslationOptionCollection *toc
                              , bool adhereTableLimit) const;

private:
	/*! create new TranslationOption from merging oldTO with mergePhrase
		This function runs IsCompatible() to ensure the two can be merged
	*/
  TranslationOption *MergeGeneration(const TranslationOption& oldTO, Phrase &mergePhrase
                                  , const ScoreComponentCollection& generationScore) const;

};


}
#endif
