//	Copyright (C) 1999-2003 Paul O. Lewis and Mark T. Holder
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

#ifndef NCL_NXSMULTIFORMAT_H
#define NCL_NXSMULTIFORMAT_H
#include <iostream>

#include "ncl/nxsdefs.h"
#include "ncl/nxspublicblocks.h"
class FileToCharBuffer;
/*!
	A special class of PublicNexusReader, that can parse
		\li PHYLIP,
		\li relaxed PHYLIP,
		\li FASTA, and
		\li ALN
	formatted files in addition to NEXUS.  Non-NEXUS files are parsed and the
	information from these files is added to the appropriate NxsBlock object.
	So the parser essentially creates a the normal NCL interface even if the
	input is not NEXUS
*/
class MultiFormatReader: public PublicNexusReader
{
	public:
		/*! enumeration of all of the formats supported by MultiFormatReader

			This enumeration type is used in calls to ReadStream and ReadFilepath
			so that the reader knows what type of data to expect.
		*/
		enum DataFormatType
			{
				NEXUS_FORMAT,
				FASTA_DNA_FORMAT,
				FASTA_AA_FORMAT,
				FASTA_RNA_FORMAT,
				PHYLIP_DNA_FORMAT,
				PHYLIP_RNA_FORMAT,
				PHYLIP_AA_FORMAT,
				PHYLIP_DISC_FORMAT,
				INTERLEAVED_PHYLIP_DNA_FORMAT,
				INTERLEAVED_PHYLIP_RNA_FORMAT,
				INTERLEAVED_PHYLIP_AA_FORMAT,
				INTERLEAVED_PHYLIP_DISC_FORMAT,
				RELAXED_PHYLIP_DNA_FORMAT,
				RELAXED_PHYLIP_RNA_FORMAT,
				RELAXED_PHYLIP_AA_FORMAT,
				RELAXED_PHYLIP_DISC_FORMAT,
				INTERLEAVED_RELAXED_PHYLIP_DNA_FORMAT,
				INTERLEAVED_RELAXED_PHYLIP_RNA_FORMAT,
				INTERLEAVED_RELAXED_PHYLIP_AA_FORMAT,
				INTERLEAVED_RELAXED_PHYLIP_DISC_FORMAT,
				ALN_DNA_FORMAT,
				ALN_RNA_FORMAT,
				ALN_AA_FORMAT,
				PHYLIP_TREE_FORMAT,
				RELAXED_PHYLIP_TREE_FORMAT,
				NEXML_FORMAT,
				FIN_DNA_FORMAT,
				FIN_AA_FORMAT,
				FIN_RNA_FORMAT,
				UNSUPPORTED_FORMAT // keep this last
			};


        void SetCoerceUnderscoresToSpaces(bool v) 
            {
            this->coerceUnderscoresToSpaces = v;
            }

        bool GetCoerceUnderscoresToSpaces() const
            {
            return this->coerceUnderscoresToSpaces;
            }
		
		/*! \returns a vector with the "official" format names that can be used with formatNameToCode

		Currently this list is:  {"nexus", "dnafasta", "aafasta", "rnafasta", "dnaphylip", "rnaphylip", "aaphylip", "discretephylip", "dnaphylipinterleaved", "rnaphylipinterleaved", "aaphylipinterleaved", "discretephylipinterleaved", "dnarelaxedphylip", "rnarelaxedphylip", "aarelaxedphylip", "discreterelaxedphylip", "dnarelaxedphylipinterleaved", "rnarelaxedphylipinterleaved", "aarelaxedphylipinterleaved", "discreterelaxedphylipinterleaved", "dnaaln", "rnaaln", "aaaln", "phyliptree", "relaxedphyliptree", "nexml"}

		*/
		static std::vector<std::string> getFormatNames();
		/*! Converts a string such as "nexus" to the corresponding facet of the DataForamType enum.

			Format names are not case sensitive
		*/
		static DataFormatType formatNameToCode(const std::string &);


		/*!	Creates a new MultiFormatReader
			\arg blocksToRead -1 indicates that every block type should be read.
				alternatively, the caller can OR-together bits of the PublicNexusReader::NexusBlocksToRead enum
				to indicate which blocks should be processed.
			\arg mode should be a facet of the NxsReader::WarningHandlingMode enum
				that indicates where warning messages should be directed.
		*/
		MultiFormatReader(const int blocksToRead = -1, NxsReader::WarningHandlingMode mode=NxsReader::WARNINGS_TO_STDERR)
			:PublicNexusReader(blocksToRead, mode),
			coerceUnderscoresToSpaces(false)
			{}
		virtual ~MultiFormatReader(){}
		/*! Read the specified format
			\arg inp the input stream
			\arg formatName the "official" format name (list of legal choices is available from getFormatNames())
		*/
		void ReadStream(std::istream & inp, const char * formatName);
		/*! Read the specified format
			\arg inp the input stream
			\arg format a facet of DataFormatType indicating the file format
		*/
		void ReadStream(std::istream & inp, DataFormatType format, const char * filepath=0L);

		/*! Read a file of the specified format
			\arg filepath the file path to open and read
			\arg formatName the "official" format name (list of legal choices is available from getFormatNames())
		*/
		void ReadFilepath(const char * filepath, const char * formatName);
		/*! Read a file of the specified format
			\arg filepath the file path to open and read
			\arg format a facet of DataFormatType indicating the file format
		*/
		void ReadFilepath(const char * filepath, DataFormatType format);

		/*! A convenience function for reading FASTA files
			\arg inf the input stream to read
			\arg dt a facet of  NxsCharactersBlock::DataTypesEnum that indicates the expected datatype
		*/
		void readFastaFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt);

	private:
		void addTaxaNames(const std::list<std::string> & taxaName, NxsTaxaBlockAPI * taxa);
		void moveDataToDataBlock(const std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, const unsigned nchar, NxsDataBlock * dataB);
		void moveDataToMatrix(std::list<NxsDiscreteStateRow> & matList,  NxsDiscreteStateMatrix &mat);
		void moveDataToUnalignedBlock(const std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, NxsUnalignedBlock * uB);
		bool readFastaSequences(FileToCharBuffer & ftcb, const NxsDiscreteDatatypeMapper &dm, std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, size_t & longest);
		bool readFinSequences(FileToCharBuffer & ftcb, NxsDiscreteDatatypeMapper &dm, std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, size_t & longest);
		void readPhylipFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt, bool relaxedNames, bool interleaved);
		void readPhylipTreeFile(std::istream & inf, bool relaxedNames);
		void readAlnFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt);
		bool readAlnData(FileToCharBuffer & ftcb, const NxsDiscreteDatatypeMapper &dm, std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList);

		unsigned readPhylipHeader(std::istream & inf, unsigned & ntax, unsigned & nchar);
		void readPhylipData(FileToCharBuffer & ftcb, const NxsDiscreteDatatypeMapper &dm, std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, const unsigned n_taxa, const unsigned n_char, bool relaxedNames);
		void readInterleavedPhylipData(FileToCharBuffer & ftcb, const NxsDiscreteDatatypeMapper &dm, std::list<std::string> & taxaNames, std::list<NxsDiscreteStateRow> & matList, const unsigned n_taxa, const unsigned n_char, bool relaxedNames);
		std::string readPhylipName(FileToCharBuffer & ftcb, unsigned i, bool relaxedNames);

		/*! A convenience function for reading .fin files
			\arg inf the input stream to read
			\arg dt a facet of  NxsCharactersBlock::DataTypesEnum that indicates the expected datatype
		*/
		void readFinFile(std::istream & inf, NxsCharactersBlock::DataTypesEnum dt);
		
		bool coerceUnderscoresToSpaces;

};

/*! \enum MultiFormatReader::DataFormatType
An enumeration of all of the formats supported by MultiFormatReader

This enumeration type is used in calls to ReadStream and ReadFilepath
so that the reader knows what type of data to expect.
*/
/*! var MultiFormatReader::NEXUS_FORMAT
 read any NCL supported NEXUS block
*/
/*! var MultiFormatReader::FASTA_DNA_FORMAT
 DNA sequence data in FASTA format
*/
/*! var MultiFormatReader::FASTA_AA_FORMAT
 amino acid sequence data in FASTA format
*/
/*! var MultiFormatReader::FASTA_RNA_FORMAT
 RNA sequence data in FASTA format
*/
/*! var MultiFormatReader::PHYLIP_DNA_FORMAT
 DNA sequence data in non-interleaved PHYLIP format
*/
/*! var MultiFormatReader::PHYLIP_RNA_FORMAT
 RNA sequence data in non-interleaved PHYLIP format
*/
/*! var MultiFormatReader::PHYLIP_AA_FORMAT
 amino acid sequence data in non-interleaved PHYLIP format
*/
/*! var MultiFormatReader::PHYLIP_DISC_FORMAT
 Discrete data (like the NEXUS "standard" format) in non-interleaved PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_PHYLIP_DNA_FORMAT
 DNA sequence data in interleaved PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_PHYLIP_RNA_FORMAT
 RNA sequence data in interleaved PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_PHYLIP_AA_FORMAT
 amino acid sequence data in interleaved PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_PHYLIP_DISC_FORMAT
 Discrete data (like the NEXUS "standard" format) data in interleaved PHYLIP format
*/
/*! var MultiFormatReader::RELAXED_PHYLIP_DNA_FORMAT
 DNA sequence data in non-interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::RELAXED_PHYLIP_RNA_FORMAT
 RNA sequence data in non-interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::RELAXED_PHYLIP_AA_FORMAT
 amino acid sequence data in non-interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::RELAXED_PHYLIP_DISC_FORMAT
 Discrete data (like the NEXUS "standard" format) data in non-interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_RELAXED_PHYLIP_DNA_FORMAT
 DNA sequence data in interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_RELAXED_PHYLIP_RNA_FORMAT
 RNA sequence data in interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_RELAXED_PHYLIP_AA_FORMAT
 Amino acid sequence data in interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::INTERLEAVED_RELAXED_PHYLIP_DISC_FORMAT
 Discrete data (like the NEXUS "standard" format) data in interleaved relaxed PHYLIP format
*/
/*! var MultiFormatReader::ALN_DNA_FORMAT
 DNA sequence data in ALN format
*/
/*! var MultiFormatReader::ALN_RNA_FORMAT
 RNA sequence data in ALN format
*/
/*! var MultiFormatReader::ALN_AA_FORMAT
 Amino acid sequence data in ALN format
*/
/*! var MultiFormatReader::PHYLIP_TREE_FORMAT
 Trees in NEWICK (PHYLIP) format
*/
/*! var MultiFormatReader::RELAXED_PHYLIP_TREE_FORMAT
 Trees in NEWICK  format with relaxed phylip names
*/
/*! var MultiFormatReader::NEXML_FORMAT
 NEXML formatted file currently unsupported, but support is planned
*/
/*! var MultiFormatReader::UNSUPPORTED_FORMAT
For NCL internal use only ( to mark the end of the DataFormatType enum).
*/

#endif

