//	Copyright (C) 1999-2003 Paul O. Lewis
//
//	This file is part of NCL (Nexus Class Library) version 2.0.
//
//	NCL is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	NCL is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with NCL; if not, write to the Free Software Foundation, Inc.,
//	59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//

#ifndef NCL_NXSTREESBLOCK_H
#define NCL_NXSTREESBLOCK_H
#include <climits>
#include <cfloat>
#include "ncl/nxsdefs.h"
#include "ncl/nxstaxablock.h"

class NxsTreesBlockAPI
  : public NxsBlock, public NxsLabelToIndicesMapper
	{
 	public:
		virtual unsigned	GetNumDefaultTree() = 0;
		virtual unsigned	GetNumTrees() = 0;
		virtual NxsString	GetTreeName(unsigned i) = 0;
		virtual NxsString	GetTreeDescription(unsigned i) = 0;
		virtual NxsString	GetTranslatedTreeDescription(unsigned i) = 0;
		virtual bool		IsDefaultTree(unsigned i) = 0;
		virtual bool		IsRootedTree(unsigned i) = 0;
	};
/*! This function provides rudimentary support for parsing of NHX comments.
	It is called during the creation of a NxsSimpleTree to handle any NHX comments

	It fills `infoMap` with the key value pairs parsed from a comment that starts with
		&&NHX
	\returns the unparsed portion of the comment
*/
std::string parseNHXComment(const std::string comment, /*! the comment without the [] braces. If the comment does not start with &&NHX then the entire comment will be returned*/
			std::map<std::string, std::string> *infoMap); /*!< the destination for key value pairs parsed out of the NHX comment */
class NxsFullTreeDescription;
class NxsSimpleNode;
/*! The edge used by the NxsSimpleTree class.
*/
class NxsSimpleEdge
	{
	public:
		bool EdgeLenIsDefaultValue() const
			{
			return defaultEdgeLen;
			}

		bool IsIntEdgeLen() const
			{
			return hasIntEdgeLens;
			}

		double GetDblEdgeLen() const
			{
			return hasIntEdgeLens ? (double) iEdgeLen : dEdgeLen ;
			}

		int GetIntEdgeLen() const
			{
			return hasIntEdgeLens ? iEdgeLen : (int) dEdgeLen ;
			}

		std::vector<NxsComment> GetUnprocessedComments() const
			{
			return unprocessedComments;
			}

		/*! \returns true if `key` was processed from a comment.
			If the key was found and `value` pointer is not NULL, then the
				*value will hold the value on exit
		*/
		bool GetInfo(const std::string &key, std::string *value) const
			{
			std::map<std::string, std::string>::const_iterator kvit = parsedInfo.find(key);
			if (kvit == parsedInfo.end())
				return false;
			if (value != NULL)
				*value = kvit->second;
			return true;
			}
		/*! Returns a reference to the map that stores information in a generic
			key to value mapping where both elements are strings.

			This map is populated by the information from NHX comments during the creation of
			a NxsSimpleTree.
		*/
		const std::map<std::string, std::string> & GetInfo() const
			{
			return parsedInfo;
			}
		const NxsSimpleNode * GetParent() const
			{
			return parent;
			}
		const NxsSimpleNode * GetChild() const
			{
			return child;
			}

		void SetDblEdgeLen(double e, const char *asString)
			{
			defaultEdgeLen = false;
			hasIntEdgeLens = false;
			dEdgeLen = e;
			if (asString)
				lenAsString.assign(asString);

			}

		void SetIntEdgeLen(int e, const char *asString)
			{
			defaultEdgeLen = false;
			hasIntEdgeLens = true;
			iEdgeLen = e;
			if (asString)
				lenAsString.assign(asString);
			}
		mutable void * scratch;
		void SetParent(NxsSimpleNode *p) 
		    {
		    this->parent = p;
		    }
	private:
		void WriteAsNewick(std::ostream &out, bool nhx) const;
		void DealWithNexusComments(const std::vector<NxsComment> & ecs, bool NHXComments);

		NxsSimpleEdge(NxsSimpleNode  *par, NxsSimpleNode * des, double edgeLen)
			:scratch(0L),
			parent(par),
			child(des),
			defaultEdgeLen(true),
			hasIntEdgeLens(false),
			dEdgeLen(edgeLen)
			{
			}

		NxsSimpleEdge(int edgeLen, NxsSimpleNode *par, NxsSimpleNode * des)
			:scratch(0L),
			parent(par),
			child(des),
			defaultEdgeLen(true),
			hasIntEdgeLens(true),
			iEdgeLen(edgeLen)
			{
			}

		NxsSimpleNode * GetMutableParent() const
			{
			return parent;
			}

		NxsSimpleNode * parent;
		NxsSimpleNode * child;
		bool			defaultEdgeLen;
		bool			hasIntEdgeLens;
		int				iEdgeLen;
		double			dEdgeLen;
		std::string		lenAsString; /*easy (but inefficient) means of preserving the formatting of the input branch length */
		std::vector<NxsComment> unprocessedComments;
		std::map<std::string, std::string> parsedInfo;
		friend class NxsSimpleTree;
		friend class NxsSimpleNode;
	};

/*! The node used by the NxsSimpleTree class.
*/
class NxsSimpleNode
	{
	public:
		NxsSimpleEdge GetEdgeToParent() const
			{
			return edgeToPar;
			}

		const NxsSimpleEdge & GetEdgeToParentRef() const
			{
			return edgeToPar;
			}

		NxsSimpleEdge & GetMutableEdgeToParentRef()
			{
			return edgeToPar;
			}

		bool IsTip() const
			{
			return (lChild == 0L);
			}
		NxsSimpleNode *GetFirstChild() const
			{
			return lChild;
			}
		NxsSimpleNode * GetNextSib() const
			{
			return rSib;
			}
		NxsSimpleNode * GetLastChild() const
			{
			NxsSimpleNode * currNode = GetFirstChild();
			if (!currNode)
				return NULL;
			NxsSimpleNode * nextNd = currNode->GetNextSib();
			while (nextNd)
				{
				currNode = nextNd;
				nextNd = currNode->GetNextSib();
				}
			return currNode;
			}

		std::vector<NxsSimpleNode *> GetChildren() const
			{
			std::vector<NxsSimpleNode *> children;
			NxsSimpleNode * currNode = GetFirstChild();
			while(currNode)
				{
				children.push_back(currNode);
				currNode = currNode->GetNextSib();
				}
			return children;
			}
		// present for every leaf. UINT_MAX for internals labeled with taxlabels
		unsigned GetTaxonIndex() const
			{
			return taxIndex;
			}

		// present for every leaf. UINT_MAX for internals labeled with taxlabels
		void SetTaxonIndex(unsigned i)
			{
			taxIndex = i;
			}

		// non-empty only for internals that are labelled with names that are NOT taxLabels
		std::string GetName() const
			{
			return name;
			}
		void SetName(const std::string &n)
			{
			name = n;
			}
		mutable void * scratch;

		NxsSimpleNode(NxsSimpleNode *par, double edgeLen)
			:scratch(0L),
			lChild(0L),
			rSib(0L),
			edgeToPar(par, 0L, edgeLen),
			taxIndex(UINT_MAX)
			{
			edgeToPar.child = this;
			}


	public:
		void WriteAsNewick(std::ostream &out, bool nhx, bool useLeafNames, bool escapeNames, const NxsTaxaBlockAPI *taxa=0L) const;


		NxsSimpleNode(int edgeLen, NxsSimpleNode *par)
			:scratch(0L),
			lChild(0L),
			rSib(0L),
			edgeToPar(edgeLen, par, 0L),
			taxIndex(UINT_MAX)
			{
			edgeToPar.child = this;
			}

		NxsSimpleNode * GetParent() const
			{
			return edgeToPar.GetMutableParent();
			}

		void AddSib(NxsSimpleNode *n)
			{
			if (rSib)
				rSib->AddSib(n);
			else
				rSib = n;
			}
		void AddChild(NxsSimpleNode *n)
			{
			if (lChild)
				lChild->AddSib(n);
			else
				lChild = n;
			}

		bool RemoveChild(NxsSimpleNode *n)
			{
			if (n == 0L || lChild == 0L)
			    return false;			
			if (lChild == n)
				lChild = lChild->rSib;
			else 
			    {
			    NxsSimpleNode * c = lChild;
				for (;;) 
				    {
				    if (c->rSib == n)
				        {
				        c->rSib = n->rSib;
				        break;
				        }
				    if (c->rSib == 0L)
				        return false;
	    			}
		    	}
			n->edgeToPar.parent = 0L;
			return true;
			}
		void AddSelfAndDesToPreorder(std::vector<const NxsSimpleNode *> &p) const;
		NxsSimpleNode * FindTaxonIndex(unsigned leafIndex);

        void LowLevelSetFirstChild(NxsSimpleNode *nd) {
            lChild = nd;
        }
        void LowLevelSetNextSib(NxsSimpleNode *nd) {
            rSib = nd;
        }
    private:
		NxsSimpleNode * lChild;
		NxsSimpleNode * rSib;
		NxsSimpleEdge edgeToPar;
		std::string name; // non-empty only for internals that are labelled with names that are NOT taxLabels
		unsigned taxIndex; // present for every leaf. UINT_MAX for internals labeled with taxlabels
		friend class NxsSimpleTree;
	};
/*! A simple tree class.
	Internally NCL stores trees as newick strings with metadata (see the NxsFullTreeDescription class)
	but you can create a NxsSimpleTree
*/
class NxsSimpleTree
	{
	public:
		NxsSimpleTree(const NxsFullTreeDescription &ftd, const int defaultIntEdgeLen, const double defaultDblEdgeLen)
			:defIntEdgeLen(defaultIntEdgeLen),
			defDblEdgeLen(defaultDblEdgeLen),
			realEdgeLens(false)
			{
			Initialize(ftd);
			}
		NxsSimpleTree(const int defaultIntEdgeLen, const double defaultDblEdgeLen)
			:defIntEdgeLen(defaultIntEdgeLen),
			defDblEdgeLen(defaultDblEdgeLen),
			realEdgeLens(false)
			{}
		~NxsSimpleTree()
			{
			Clear();
			}
		void Initialize(const NxsFullTreeDescription &);


		std::vector<const NxsSimpleNode *> GetPreorderTraversal() const;
		std::vector<NxsSimpleNode *> & GetLeavesRef()
			{
			return leaves;
			}
		std::vector<std::vector<int> > GetIntPathDistances(bool toMRCA=false) const;
		std::vector<std::vector<double> > GetDblPathDistances(bool toMRCA=false) const;

		/** Writes just the newick description with numbers for leaf labels.
			Neither the tree name or NEXUS ; are written
		*/
		void WriteAsNewick(std::ostream &out, bool nhx, bool useLeafNames, bool escapeNames, const NxsTaxaBlockAPI * taxa) const
			{
			if (root)
				root->WriteAsNewick(out, nhx, useLeafNames, escapeNames, taxa);
			}
		NxsSimpleNode * RerootAt(unsigned leafIndex);
        NxsSimpleNode * RerootAtNode(NxsSimpleNode *newRoot);

		const NxsSimpleNode * GetRootConst() const
			{
			return root;
			}
	protected:
		std::vector<NxsSimpleNode *> allNodes;
		std::vector<NxsSimpleNode *> leaves;
		NxsSimpleNode * root;
		int defIntEdgeLen;
		double defDblEdgeLen;
		bool realEdgeLens;
	public:
		NxsSimpleNode * AllocNewNode(NxsSimpleNode *p)
			{
			NxsSimpleNode * nd;
			if (realEdgeLens)
				nd = new NxsSimpleNode(p, defDblEdgeLen);
			else
				nd = new NxsSimpleNode(defIntEdgeLen, p);
			allNodes.push_back(nd);
			return nd;
			}

		void Clear()
			{
			root = NULL;
			for (std::vector<NxsSimpleNode *>::iterator nIt = allNodes.begin(); nIt != allNodes.end(); ++nIt)
				delete *nIt;
			allNodes.clear();
			leaves.clear();
			}
		void FlipRootsChildToRoot(NxsSimpleNode *subRoot);
		NxsSimpleTree(const NxsSimpleTree &); //not defined.  Not copyable
		NxsSimpleTree & operator=(const NxsSimpleTree &); //not defined.  Not copyable
	};

/*! A class that encapsulates a newick string description of a tree and metadata about the tree.

	the NxsTreesBlock stores the trees as NxsFullTreeDescription because during its parse
	and validation of a tree string.
	By default, NCL will "process" each tree -- converting the taxon labels to
		numbers for the taxa (the number will be 1 + the taxon index).
		During this processing, the trees block detects things about the tree such as whether
		there are branch lengths on the tree, whether there are polytomies...

	This data about the tree is then stored in a NxsFullTreeDescription
	so that the client code can access some information about a tree before it parses
	the newick string.

	If you do not want to parse the newick string yourself, you can construct a
		NxsSimpleTree object from a NxsFullTreeDescription object if the NxsFullTreeDescription
		is "processed"

	If the NxsTreesBlock is configured NOT to process trees (see NxsTreesBlock::SetProcessAllTreesDuringParse())
*/
class NxsFullTreeDescription
	{
	public:
		enum TreeDescFlags
			{ 	NXS_IS_ROOTED_BIT					= 0x0001,
				NXS_HAS_SOME_EDGE_LENGTHS_BIT		= 0x0002,
				NXS_MISSING_SOME_EDGE_LENGTHS_BIT	= 0x0004,
				NXS_EDGE_LENGTH_UNION 				= 0x0006,
				NXS_INT_EDGE_LENGTHS_BIT 			= 0x0008,
				NXS_HAS_ALL_TAXA_BIT				= 0x0010,
				NXS_HAS_NHX_BIT 					= 0x0020,
				NXS_HAS_DEG_TWO_NODES_BIT			= 0x0040,
				NXS_HAS_POLYTOMY_BIT				= 0x0080,
				NXS_HAS_INTERNAL_NAMES_BIT			= 0x0100,
				NXS_HAS_NEW_INTERNAL_NAMES_BIT		= 0x0200,
				NXS_KNOWN_INTERNAL_NAMES_BIT		= 0x0400,
				NXS_SOME_ZERO_EDGE_LEN_BIT			= 0x0800,
				NXS_SOME_NEGATIVE_EDGE_LEN_BIT		= 0x1000,
				NXS_TREE_PROCESSED 					= 0x2000
			};
		/*! Creates a Tree description from a newick string, name and int with bits that indicate
			some metadata about the tree.
		*/
		NxsFullTreeDescription(const std::string & newickStr, /*!< the newick string */
				const std::string &treeName, /*!< the name of the tree */
				int infoFlags) /*!< union of the relevant bits from TreeDescFlags */
			:newick(newickStr),
			name(treeName),
			flags(infoFlags),
			minIntEdgeLen(INT_MAX),
			minDblEdgeLen(DBL_MAX),
			requireNewickNameTokenizing(false)
			{}
		/*! Tokenizes the tree into a vector of NEXUS tokens.
			This makes it easier for to parse.
		*/
		std::vector<std::string> GetTreeTokens() const;

		/** returns a newick string.
			If the NxsFullTreeDescription is processed, then the string will have
				1-based numbers corresponding to (1 + Taxa block's index of taxon)
			If it is not processed, then it will correspond with the exact string
				in the file. Handling unprocessed newick strings requires that the
				client code consult the Translation table and implement NEXUS'
				numeric interpretation of labels in order to decode correctly
				decode all taxon labels
		*/
		const std::string &	GetNewick() const
			{
			return newick;
			}
		/*! \returns the name of the tree */
		const std::string &	GetName() const
			{
			return name;
			}
		/*! \returns true if the newick string has been processed. */
		bool IsProcessed() const
			{
			return (flags&NXS_TREE_PROCESSED) != 0;
			}
		/*! \throws a NxsNCLAPIException if the tree has not been "processed" */
		void AssertProcessed() const
			{
			if (!IsProcessed())
				throw NxsNCLAPIException("Tree description queries are only supported on processed tree descriptions.");
			}
		/*! \returns true if the tree was rooted.  */
		bool IsRooted() const
			{
			AssertProcessed();
			return (flags&NXS_IS_ROOTED_BIT) != 0;
			}
		/*! \returns true all of the edges in the tree have edge length.
			\raises a NxsNCLAPIException if the tree has not been processed!
		*/
		bool AllEdgesHaveLengths() const
			{
			AssertProcessed();
			return (flags&NXS_EDGE_LENGTH_UNION) == NXS_HAS_SOME_EDGE_LENGTHS_BIT;
			}
		/*! \returns true at least one edge in the tree have edge length
			\raises a NxsNCLAPIException if the tree has not been processed!
		*/
		bool SomeEdgesHaveLengths() const
			{
			AssertProcessed();
			return (flags&NXS_HAS_SOME_EDGE_LENGTHS_BIT) != 0;
			}
		/*! \returns true all of the edge lengths that are specified can be read as integers
			\raises a NxsNCLAPIException if the tree has not been processed!
		*/
		bool EdgeLengthsAreAllIntegers() const
			{
			AssertProcessed();
			return (flags&NXS_INT_EDGE_LENGTHS_BIT) != 0;
			}
		/*! \returns true if the tree contains all of the taxa listed in the NxsTaxaBlock associated with the trees block that generated this NxsFullTreeDescription
			\raises a NxsNCLAPIException if the tree has not been processed!
		*/
		bool AllTaxaAreIncluded() const
			{
			AssertProcessed();
			return (flags&NXS_HAS_ALL_TAXA_BIT) != 0;
			}
		/*! \returns true if some of the edges in the tree have New Hampshire Extended style comments  (see http://www.phylosoft.org/NHX)
			\raises a NxsNCLAPIException if the tree has not been processed!
		*/
		bool HasNHXComments() const
			{
			AssertProcessed();
			return (flags&NXS_HAS_NHX_BIT) != 0;
			}
		/*! \returns true if the tree has polytomies
			\raises a NxsNCLAPIException if the tree has not been processed!
		*/
		bool HasPolytomies() const
			{
			AssertProcessed();
			return (flags&NXS_HAS_POLYTOMY_BIT) != 0;
			}
		/*! \returns true if the tree some internal nodes that only have one child.
			\raises a NxsNCLAPIException if the tree has not been processed!
		*/
		bool HasDegreeTwoNodes() const
			{
			AssertProcessed();
			return (flags&NXS_HAS_DEG_TWO_NODES_BIT) != 0;
			}
		/*! If EdgeLengthsAreAllIntegers returns true then this will return the
			shortest edge length in the tree (useful as means of checking for
			constraints by programs that prohibit 0 or negative branch lengths)
		*/
		int smallestIntEdgeLength() const
			{
			return minIntEdgeLen;
			}
		/*!	If EdgeLengthsAreAllIntegers returns false then this will return the
			shortest edge length in the tree (useful as means of checking for
			constraints by programs that prohibit 0 or negative branch lengths)
		*/
		double smallestRealEdgeLength() const
			{
			return minDblEdgeLen;
			}
		bool RequiresNewickNameTokenizing() const 
		    {
		    return this->requireNewickNameTokenizing;
		    }
		void SetRequiresNewickNameTokenizing(bool v) 
		    {
		    this->requireNewickNameTokenizing = v;
		    }
	private:
		std::string newick; /*with 1-based numbers corresponding to (1 + Taxa block's index of taxon)*/
		std::string name;
		int flags;
		int minIntEdgeLen; /* if EdgeLengthsAreAllIntegers returns true then this will hold shortest edge length in the tree (useful as means of checking for constraints by programs that prohibit 0 or negative branch lengths)*/
		double minDblEdgeLen; /* if EdgeLengthsAreAllIntegers returns false then this will hold shortest edge length in the tree (useful as means of checking for constraints by programs that prohibit 0 or negative branch lengths)*/
		bool requireNewickNameTokenizing;  /* False by default. If true, then newick rather than NEXUS tokenizing rules should be used for the taxa names */

	friend class NxsTreesBlock;
	};
class NxsTreesBlock;
typedef bool (* ProcessedTreeValidationFunction)(NxsFullTreeDescription &, void *, NxsTreesBlock *);
/*!
	This class handles reading and storage for the NEXUS block TREES.
	The class can  read the TRANSLATE and TREE commands.

	The tree is validated during the parse and then stored as a NxsFullTreeDescription
		object which will hold the newick string. This newick string will have
		numbers rather than names. The numbers in the tree string start at 1 (like other NEXUS numbering),
		but they are simply 1 + the taxon index.

	In previous versions of NCL (before v2.1), the client code would have to use the translate
		table to convert the newick string into the taxon numbers.

	As of v2.1, NCL now does this translation.

*/
class NxsTreesBlock
  : public NxsTreesBlockAPI, public NxsTaxaBlockSurrogate
	{
 	public:
							NxsTreesBlock(NxsTaxaBlockAPI *tb);
		virtual				~NxsTreesBlock();

		void		ReplaceTaxaBlockPtr(NxsTaxaBlockAPI *tb);
		unsigned GetIndexSet(const std::string &label, NxsUnsignedSet * toFill) const
			{
			return NxsLabelToIndicesMapper::GetIndicesFromSets(label, toFill, treeSets);
			}

		/*! \returns the index of the default tree (the last tree in the TREES block with a * before its name)
				if no default tree was specified than the first index (0) will be returned
		*/
		unsigned	GetNumDefaultTree();
		/*! \returns the number of trees stored */
		unsigned	GetNumTrees();
		/*! \returns the number of trees stored */
		unsigned	GetNumTrees() const;
		/*! \returns the NxsFullTreeDescription for tree with index `i`
		`i` should be in the range [0, num_trees)

		If the NxsFullTreeDescription is processed (see NxsFullTreeDescription::IsProcessed())
			then its newick string will have numbers rather than names. The numbers in the tree
			string start at 1 (like other NEXUS numbering), but they are simply 1 + the taxon index.

		In previous versions of NCL (before v2.1), the client code would have to use the translate
			table to convert the newick string into the taxon numbers.


		*/
		const NxsFullTreeDescription & GetFullTreeDescription(unsigned i) const;
		/*! \returns a 1-based number for the last tree read that has the name `name` */
		unsigned	TreeLabelToNumber(const std::string & name) const;
		/*! \returns the tree name for the tree with index `i`
		i should be in the range [0, ntrees)
		*/
		NxsString	GetTreeName(unsigned i);
		/*! \returns the tree description object for the tree with index `i`
		i should be in the range [0, ntrees)
		*/
		NxsString	GetTreeDescription(unsigned i);
		/*! \returns the newick string for the tree with index i. The string will have
			the taxon names rather than numbers (or other translate table keys) in it.
		i should be in the range [0, ntrees)
		*/
		NxsString	GetTranslatedTreeDescription(unsigned i);
		/*! \returns true if the tree with index i is the default tree
		i should be in the range [0, ntrees)
		*/
		bool		IsDefaultTree(unsigned i);
		/*! \returns true if the tree is thought to be rooted (could be rooted
			because this is NCL's default, or it could indicate that a [&R]
			comment was encountered.
		i should be in the range [0, ntrees)
		*/
		bool		IsRootedTree(unsigned i);
		virtual void		Report(std::ostream &out) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		virtual void		BriefReport(NxsString &s) NCL_COULD_BE_CONST ; /*v2.1to2.2 1 */
		virtual void		Reset();
		void				SetNexus(NxsReader *nxsptr)
			{
			NxsBlock::SetNexus(nxsptr);
			NxsTaxaBlockSurrogate::SetNexusReader(nxsptr);
			}
		/*! \ref BlockTypeIDDiscussion */
        virtual const std::string & GetBlockName() const
            {
            return NCL_BLOCKTYPE_ATTR_NAME;
            }

		void WriteAsNexus(std::ostream &out) const;

		virtual VecBlockPtr	GetImpliedBlocks()
			{
			return GetCreatedTaxaBlocks();
			}

		/*only used it the linkAPI is enabled*/
		virtual void		HandleLinkCommand(NxsToken & token)
			{
			HandleLinkTaxaCommand(token);
			}
		virtual void		WriteLinkCommand(std::ostream &out) const
			{
			WriteLinkTaxaCommand(out);
			}

		unsigned GetMaxIndex() const;
		unsigned GetIndicesForLabel(const std::string &label, NxsUnsignedSet *inds) const;
		bool AddNewIndexSet(const std::string &label, const NxsUnsignedSet & inds);
		bool AddNewPartition(const std::string &label, const NxsPartition & inds);

		bool GetAllowImplicitNames() const
			{
			return allowImplicitNames;
			}
		bool GetUseNewickTokenizingDuringParse() const 
		    {
		    return useNewickTokenizingDuringParse;
		    }
		/*! \returns true if the block uses the v2.1 style of parsing in which the tree is interpretted and converted into
				a newick string with standard taxon numbering
			If false, then the NxsTreesBlock uses the v2.0 API in which the tree reader simply stores the tree string
				as written in the file (so the client code has to check the translate table in order to interpret
				the newick stream).
			true by default.
		*/
		bool GetProcessAllTreesDuringParse() const
			{
			return processAllTreesDuringParse;
			}
		void SetAllowImplicitNames(bool s)
			{
			allowImplicitNames = s;
			}
		void SetUseNewickTokenizingDuringParse(bool v) 
		    {
		    useNewickTokenizingDuringParse = v; 
		    }
		void SetTreatIntegerLabelsAsNumbers(bool s) 
		    {
		    treatIntegerLabelsAsNumbers = s;
		    }
		/*! If true then the block will use the v2.1 style of parsing in which the tree is interpretted and converted into
				a newick string with standard taxon numbering
			If false, then the NxsTreesBlock will use the v2.0 API in which the tree reader simply stores the tree string
				as written in the file (so the client code has to check the translate table in order to interpret
				the newick stream).
			true by default.
		*/
		void SetProcessAllTreesDuringParse(bool s)
			{
			processAllTreesDuringParse = s;
			}
		/* Interprets the newick string as a tree. This converts the newick string
			into one in which 1-based numbers are used for taxon labels (raw newick
			strings can contain numbers, taxon labels, tax set names or translate
			table keys as taxon identifiers).

			\raises NxsException
			This function builds trees as in memory. It may  reveal illegal newick strings that were not
			detected as illegal on the parse, so NxsExceptions may  be raised.

			Explicitly calling this function is not necessary unless
			processAllTreesDuringParse is false (because of a previous call to
			SetProcessAllTreesDuringParse()).
		*/
		void ProcessTree(NxsFullTreeDescription &treeDesc) const;
		/* Convenience function that calls ProcessTree() one each stored
			NxsFullTreeDescription instance.

			\raises NxsException
			This function builds trees as in memory. It may  reveal illegal newick strings that were not
			detected as illegal on the parse, so NxsExceptions may  be raised.

			Explicitly calling this function is not necessary unless
			processAllTreesDuringParse is false (because of a previous call to
			SetProcessAllTreesDuringParse()).
		*/
		void ProcessAllTrees() const
			{
			std::vector<NxsFullTreeDescription>::iterator trIt = trees.begin();
			for (; trIt != trees.end(); ++trIt)
				ProcessTree(*trIt);
			}


		/*---------------------------------------------------------------------------------------
		| Results in aliasing of the taxa, assumptionsBlock blocks!
		*/
		NxsTreesBlock & operator=(const NxsTreesBlock &other)
			{
			Reset();
			CopyBaseBlockContents(static_cast<const NxsBlock &>(other));
			CopyTaxaBlockSurrogateContents(other);
			CopyTreesBlockContents(other);
			return *this;
			}

		/*---------------------------------------------------------------------------------------
		| Results in aliasing of the taxa, assumptionsBlock blocks!
		*/
		virtual void CopyTreesBlockContents(const NxsTreesBlock &other)
			{
			allowImplicitNames = other.allowImplicitNames;
			useNewickTokenizingDuringParse = other.useNewickTokenizingDuringParse;
			treatIntegerLabelsAsNumbers = other.treatIntegerLabelsAsNumbers;
			processAllTreesDuringParse = other.processAllTreesDuringParse;
			writeFromNodeEdgeDataStructure = other.writeFromNodeEdgeDataStructure;
			validateInternalNodeLabels = other.validateInternalNodeLabels;
			allowNumericInterpretationOfTaxLabels = other.allowNumericInterpretationOfTaxLabels;
			constructingTaxaBlock = other.constructingTaxaBlock;
			newtaxa = other.newtaxa;
			trees = other.trees;
			capNameToInd = other.capNameToInd;
			defaultTreeInd = other.defaultTreeInd;
			writeTranslateTable = other.writeTranslateTable;
			treeSets = other.treeSets;
			treePartitions = other.treePartitions;
			processedTreeValidationFunction = other.processedTreeValidationFunction;
			ptvArg = other.ptvArg;
			treatAsRootedByDefault = other.treatAsRootedByDefault;
			}
        bool GetTreatAsRootedByDefault() const {
            return treatAsRootedByDefault;
        }
        void SetTreatAsRootedByDefault(bool v) {
            this->treatAsRootedByDefault = v;
        }
		virtual NxsTreesBlock * Clone() const
			{
			NxsTreesBlock * a = new NxsTreesBlock(taxa);
			*a = *this;
			return a;
			}
		static void ProcessTokenVecIntoTree(const ProcessedNxsCommand & token, 
		                                    NxsFullTreeDescription & ftd,
		                                    NxsLabelToIndicesMapper *,
		                                    std::map<std::string, unsigned> &capNameToInd,
		                                    bool allowNewTaxa,
		                                    NxsReader * nexusReader,
		                                    const bool respectCase=false,
		                                    const bool validateInternalNodeLabels=true,
		                                    const bool treatIntegerLabelsAsNumbers=false,
		                                    const bool allowNumericInterpretationOfTaxLabels=true);
		static void ProcessTokenStreamIntoTree(NxsToken & token, NxsFullTreeDescription & ftd,
		                                      NxsLabelToIndicesMapper *,
		                                      std::map<std::string, unsigned> &capNameToInd,
		                                      bool allowNewTaxa,
		                                      NxsReader * nexusReader,
		                                      const bool respectCase=false,
		                                      const bool validateInternalNodeLabels=true,
		                                      const bool treatIntegerLabelsAsNumbers=false,
		                                      const bool allowNumericInterpretationOfTaxLabels=true);

		void SetWriteFromNodeEdgeDataStructure(bool v)
			{
			writeFromNodeEdgeDataStructure = v;
			}
		/* 	Processes all trees and then
			Provides lowlevel access to the "raw" vector of trees stored in the trees block
		*/
		std::vector<NxsFullTreeDescription> & GetProcessedTrees()
			{
			ProcessAllTrees();
			return trees;
			}

		/*! This function allows you to register a callback function that is called after each tree is parsed.

			The signature of your function should be:\n
				\code
				bool someFunctionName(NxsFullTreeDescription &treeDesc, void * blob, NxsTreesBlock * treesB);
				\endcode
			where:
				- treeDesc is the NxsFullTreeDescription for the tree that was just read.
				- blob is pointer to any object or 0L. You supply this blob of data as an argument in
					setValidationCallbacks and the NxsTreesBlock passes it every time that it calls the callback.
					By passing in your own object, you can do bookkeeping between calls without using global variables
					(though you will have to cast the pointer to the blob of data, of course).
				- treesB is a pointer to the block that is conducting the parse.

			If your function returns false, then the trees block will not store.
			If your callback function returns true, then the tree will be stored.
			In either case the NxsTreesBlock will continue parsing after your function returns.

			This Callback hook is convenient for rejecting unwanted trees to save on memory, but it can also
			be used as an optimization.

			See the example executable in example/splitsinfile.  This NCL client, uses this callback to store
			the splits from a trees as the TREES block is being parsed.  It returns false in each case, so that
			the trees are not stored after they are used.
		*/

		void setValidationCallbacks(
			ProcessedTreeValidationFunction func, /*!< your pointer to your callback function */
			void * blob) /*!< pointer to any object that you would like to access during parse */
			{
			this->processedTreeValidationFunction = func;
			this->ptvArg = blob;
			}
		bool 		SwapEquivalentTaxaBlock(NxsTaxaBlockAPI * tb)
		{
			return SurrogateSwapEquivalentTaxaBlock(tb);
		}
		void ReadPhylipTreeFile(NxsToken & token);
		void setWriteTranslateTable(bool wtt)
		{
			this->writeTranslateTable = wtt;
		}
		void setAllowNumericInterpretationOfTaxLabels(bool x) {
			this->allowNumericInterpretationOfTaxLabels = x; 
		}
		/*! Sets the boolean field that determines whether or not the trees
			block will validate treat internal node labels
			as taxon labels during the parse. In this case the labels will
			checked against the taxa block (true is the default).

			This can cause problems if the internal node names are integers that
			are not intended to be taxon labels (eg. support statements for the
			subtending branches).
		*/
		void setValidateInternalNodeLabels(bool x) {
			this->validateInternalNodeLabels = x; /** if true then labels that occur for internal nodes will be validated via the taxa block (true is the default).  This can cause problems if the internal node names are integer that are not intended to be taxon labels. */
		}
		/*! \returns true if the block will validate treat internal node labels
			as taxon labels during the parse. In this case the labels will
			checked against the taxa block (true is the default).

			This can cause problems if the internal node names are integers that
			are not intended to be taxon labels (eg. support statements for the
			subtending branches).
		*/
		bool getValidateInternalNodeLabels() const {
			return this->validateInternalNodeLabels;
		}
		void WriteTranslateCommand(std::ostream & out) const;
	protected :
		void ReadTreeFromOpenParensToken(NxsFullTreeDescription &td, NxsToken & token);

		void WriteTreesCommand(std::ostream & out) const;
		void ConstructDefaultTranslateTable(NxsToken &token, const char * cmd);

		bool allowImplicitNames; /** false by default, true causes the trees block to create a taxa block from the labels found in the trees. */
		bool useNewickTokenizingDuringParse; /** false by default */
		bool treatIntegerLabelsAsNumbers; // if true and allowImplicitNames is true, then new taxon labels that are integers will be treated as the taxon number (rather than arbitrary labels)
		bool processAllTreesDuringParse; /** true by default, false speeds processing but disables detection of errors*/
		bool constructingTaxaBlock; /** true if new names are being tolerated */
		bool writeFromNodeEdgeDataStructure; /**this will probably only ever be set to true in testing code. If true the WriteTrees function will convert each tree to NxsSimpleTree object to write the newick*/
		bool validateInternalNodeLabels; /** if true then labels that occur for internal nodes will be validated via the taxa block (true is the default).  This can cause problems if the internal node names are integer that are not intended to be taxon labels. */
		bool allowNumericInterpretationOfTaxLabels;

		mutable std::vector<NxsFullTreeDescription> trees;
		mutable std::map<std::string, unsigned> capNameToInd;
		unsigned			defaultTreeInd;		/* 0-offset index of default tree specified by user, or 0 if user failed to specify a default tree using an asterisk in the NEXUS data file */
		NxsUnsignedSetMap 	treeSets;
		NxsPartitionsByName treePartitions;

		bool writeTranslateTable ; // only affects writing to NEXUS. Default is true

		ProcessedTreeValidationFunction processedTreeValidationFunction;
		void * ptvArg;
        bool treatAsRootedByDefault; /* true by default */
		virtual	void		Read(NxsToken &token);
		void				HandleTranslateCommand(NxsToken &token);
		void				HandleTreeCommand(NxsToken &token, bool rooted);

		friend class PublicNexusReader;
	};

typedef NxsTreesBlock TreesBlock;
class NxsTreesBlockFactory
	:public NxsBlockFactory
	{
	public:
		virtual NxsTreesBlock  *	GetBlockReaderForID(const std::string & NCL_BLOCKTYPE_ATTR_NAME, NxsReader *reader, NxsToken *token);
	};

#endif
