// $Id: ChartHypothesisCollection.h 3360 2010-07-17 23:23:09Z hieuhoang1972 $
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
#pragma once

#include <set>
#include "ChartHypothesis.h"
#include "QueueEntry.h"


namespace MosesChart
{
	
// order by descending score
class ChartHypothesisScoreOrderer
	{
	public:
		bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
		{
			return hypoA->GetTotalScore() > hypoB->GetTotalScore();
		}
	};
	
class HypothesisRecombinationOrderer
{
public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		// assert in same cell
		const Moses::WordsRange &rangeA	= hypoA->GetCurrSourceRange()
		, &rangeB	= hypoB->GetCurrSourceRange();
		assert(rangeA == rangeB);
		
		/*
		int ret = Moses::Word::Compare(hypoA->GetTargetLHS(), hypoB->GetTargetLHS());
		if (ret != 0)
			return (ret < 0);
		*/
		
		// shouldn't be mixing hypos with different lhs
		assert(hypoA->GetTargetLHS() == hypoB->GetTargetLHS());
		
		int ret = hypoA->LMContextCompare(*hypoB);
		if (ret != 0)
			return (ret < 0);
		
		return false;
	}
};

// order by descending score
class HypothesisScoreOrderer
{
public:
	bool operator()(const Hypothesis* hypoA, const Hypothesis* hypoB) const
	{
		return hypoA->GetTotalScore() > hypoB->GetTotalScore();
	}
};

// 1 of these for each target LHS in each cell
class HypothesisCollection
{
	friend std::ostream& operator<<(std::ostream&, const HypothesisCollection&);

protected:
	typedef std::set<Hypothesis*, HypothesisRecombinationOrderer> HCType;
	HCType m_hypos;
	HypoList m_hyposOrdered;
	
	float m_bestScore; /**< score of the best hypothesis in collection */
	float m_beamWidth; /**< minimum score due to threashold pruning */
	size_t m_maxHypoStackSize; /**< maximum number of hypothesis allowed in this stack */
	bool m_nBestIsEnabled; /**< flag to determine whether to keep track of old arcs */
	
	/** add hypothesis to stack. Prune if necessary.
	 * Returns false if equiv hypo exists in collection, otherwise returns true
	 */
	std::pair<HCType::iterator, bool> Add(Hypothesis *hypo, Manager &manager);

public:
	typedef HCType::iterator iterator;
	typedef HCType::const_iterator const_iterator;
	//! iterators
	const_iterator begin() const { return m_hypos.begin(); }
	const_iterator end() const { return m_hypos.end(); }
	
	HypothesisCollection();
	~HypothesisCollection();
	bool AddHypothesis(Hypothesis *hypo, Manager &manager);

	//! remove hypothesis pointed to by iterator but don't delete the object
	void Detach(const HCType::iterator &iter);
	/** destroy Hypothesis pointed to by iterator (object pool version) */
	void Remove(const HCType::iterator &iter);
	
	void PruneToSize(Manager &manager);

	size_t GetSize() const
	{ return m_hypos.size(); }
	size_t GetHypo() const
	{ return m_hypos.size(); }

	void SortHypotheses();
	void CleanupArcList();

	const HypoList &GetSortedHypotheses() const
	{ return m_hyposOrdered; }
	
	void GetSearchGraph(long translationId, std::ostream &outputSearchGraphStream) const;

};

} // namespace

