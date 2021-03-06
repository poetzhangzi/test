// $Id: PhraseDictionarySCFGChart.cpp 3360 2010-07-17 23:23:09Z hieuhoang1972 $
// vim:tabstop=2
/***********************************************************************
 Moses - factored phrase-based language decoder
 Copyright (C) 2010 Hieu Hoang
 
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

#include "PhraseDictionarySCFG.h"
#include "FactorCollection.h"
#include "InputType.h"
#include "ChartRuleCollection.h"
#include "CellCollection.h"
#include "DotChart.h"
#include "StaticData.h"
#include "TreeInput.h"

using namespace std;
using namespace Moses;

Word PhraseDictionarySCFG::CreateCoveredWord(const Word &origSourceLabel, const InputType &src, const WordsRange &range) const
{
	string coveredWordsString = origSourceLabel.GetFactor(0)->GetString();
	
	for (size_t pos = range.GetStartPos(); pos <= range.GetEndPos(); ++pos)
	{
		const Word &word = src.GetWord(pos);
		coveredWordsString += "_" + word.GetFactor(0)->GetString();
	}
	
	FactorCollection &factorCollection = FactorCollection::Instance();
	
	Word ret;
	
	const Factor *factor = factorCollection.AddFactor(Input, 0, coveredWordsString);
	ret.SetFactor(0, factor);
	
	return ret;
}

const ChartRuleCollection *PhraseDictionarySCFG::GetChartRuleCollection(
																																							 InputType const& src
																																							 ,WordsRange const& range
																																							 ,bool adhereTableLimit
																																							 ,const CellCollection &cellColl) const
{
	ChartRuleCollection *ret = new ChartRuleCollection();
	m_chartTargetPhraseColl.push_back(ret);
	
	size_t relEndPos = range.GetEndPos() - range.GetStartPos();
	size_t absEndPos = range.GetEndPos();
	
	// MAIN LOOP. create list of nodes of target phrases
	ProcessedRuleStack &runningNodes = *m_runningNodesVec[range.GetStartPos()];
	
	const ProcessedRuleStack::SavedNodeColl &savedNodeColl = runningNodes.GetSavedNodeColl();
	for (size_t ind = 0; ind < savedNodeColl.size(); ++ind)
	{
		const SavedNode &savedNode = *savedNodeColl[ind];
		const ProcessedRule &prevProcessedRule = savedNode.GetProcessedRule();
		const PhraseDictionaryNodeSCFG &prevNode = static_cast<const PhraseDictionaryNodeSCFG &>(prevProcessedRule.GetLastNode());
		const WordConsumed *prevWordConsumed = prevProcessedRule.GetLastWordConsumed();
		size_t startPos = (prevWordConsumed == NULL) ? range.GetStartPos() : prevWordConsumed->GetWordsRange().GetEndPos() + 1;
		
		// search for terminal symbol
		if (startPos == absEndPos)
		{
			const Word &sourceWord = src.GetWord(absEndPos);
			const PhraseDictionaryNodeSCFG *node = prevNode.GetChild(sourceWord, sourceWord);
			if (node != NULL)
			{
				const Word &sourceWord = node->GetSourceWord();
				WordConsumed *newWordConsumed = new WordConsumed(absEndPos, absEndPos
																												 , sourceWord
																												 , prevWordConsumed);
				ProcessedRule *processedRule = new ProcessedRule(*node, newWordConsumed);
				runningNodes.Add(relEndPos+1, processedRule);
			}
		}
		
		// search for non-terminals
		size_t endPos, stackInd;
		if (startPos > absEndPos)
			continue;
		else if (startPos == range.GetStartPos() && range.GetEndPos() > range.GetStartPos())
		{ // start.
			endPos = absEndPos - 1;
			stackInd = relEndPos;
		}
		else
		{
			endPos = absEndPos;
			stackInd = relEndPos + 1;
		}
		
		// get headwords in this span from chart
		const vector<Word> &headWords = cellColl.GetHeadwords(WordsRange(startPos, endPos));
		
		// go thru each source span
		const LabelList &labelList = src.GetLabelList(startPos, endPos);
		
		LabelList::const_iterator iterLabelList;
		for (iterLabelList = labelList.begin(); iterLabelList != labelList.end(); ++iterLabelList)
		{
			const Word &sourceLabel = *iterLabelList;
			
			// go thru each headword & see if in phrase table
			vector<Word>::const_iterator iterHeadWords;
			for (iterHeadWords = headWords.begin(); iterHeadWords != headWords.end(); ++iterHeadWords)
			{
				const Word &headWord = *iterHeadWords;
				
				const PhraseDictionaryNodeSCFG *node = prevNode.GetChild(headWord, sourceLabel);
				if (node != NULL)
				{
					//const Word &sourceWord = node->GetSourceWord();
					WordConsumed *newWordConsumed = new WordConsumed(startPos, endPos
																													 , headWord
																													 , prevWordConsumed);
					
					ProcessedRule *processedRule = new ProcessedRule(*node, newWordConsumed);
					runningNodes.Add(stackInd, processedRule);
				}
			} // for (iterHeadWords
		} // for (iterLabelList 
	}
	
	// return list of target phrases
	ProcessedRuleColl &nodes = runningNodes.Get(relEndPos + 1);
	//DeleteDuplicates(nodes);
	
	size_t rulesLimit = StaticData::Instance().GetRuleLimit();
	ProcessedRuleColl::const_iterator iterNode;
	for (iterNode = nodes.begin(); iterNode != nodes.end(); ++iterNode)
	{
		const ProcessedRule &processedRule = **iterNode;
		const PhraseDictionaryNodeSCFG &node = static_cast<const PhraseDictionaryNodeSCFG &>(processedRule.GetLastNode());
		const WordConsumed *wordConsumed = processedRule.GetLastWordConsumed();
		assert(wordConsumed);
		
		const TargetPhraseCollection *targetPhraseCollection = node.GetTargetPhraseCollection();
		
		if (targetPhraseCollection != NULL)
		{
			ret->Add(*targetPhraseCollection, *wordConsumed, adhereTableLimit, rulesLimit);
		}
	}
	ret->CreateChartRules(rulesLimit);
	
	return ret;
}

void PhraseDictionarySCFG::DeleteDuplicates(ProcessedRuleColl &nodes) const
{
	map<size_t, float> minEntropy;
	map<size_t, float>::iterator iterEntropy;
	
	// find out min entropy for each node id
	ProcessedRuleColl::iterator iter;
	for (iter = nodes.begin(); iter != nodes.end(); ++iter)
	{
		const ProcessedRule *processedRule = *iter;
		const PhraseDictionaryNodeSCFG &node = static_cast<const PhraseDictionaryNodeSCFG&> (processedRule->GetLastNode());
		size_t nodeId = node.GetId();
		float entropy = node.GetEntropy();
		
		iterEntropy = minEntropy.find(nodeId);
		if (iterEntropy == minEntropy.end())
		{
			minEntropy[nodeId] = entropy;
		}
		else
		{
			float origEntropy = minEntropy[nodeId];
			if (entropy < origEntropy)
			{
				minEntropy[nodeId] = entropy;
			}
		}
	}
	
	// delete nodes which are over min entropy
	size_t ind = 0;
	while (ind < nodes.GetSize())
	{
		const ProcessedRule &processedRule = nodes.Get(ind);
		const PhraseDictionaryNodeSCFG &node = static_cast<const PhraseDictionaryNodeSCFG&> (processedRule.GetLastNode());
		size_t nodeId = node.GetId();
		float entropy = node.GetEntropy();
		float minEntropy1 = minEntropy[nodeId];
		
		if (entropy > minEntropy1)
		{
			nodes.Delete(ind);
		}
		else
		{
			ind++;
		}
	}
}


