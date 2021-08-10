/////////////////////////////////////////////////////////////
// StemCandidate.hpp
// Profile Stem Candidate calcurated by profile base pairing
// probability matrix 
////////////////////////////////////////////////////////////

#ifndef __STEMCANDIDATE_HPP__
#define __STEMCANDIDATE_HPP__

#include <string>
#include <vector>

using namespace std;

namespace MXSCARNA {
class StemCandidate {
private:
    int numSeq;                      /* the number of sequences in the profile */
    int length;                      /* length of profile stem candidate of fixed length */
    int position;                    /* 5' start position of SC in profile */
    int distance;
    std::vector<std::string> substr; /* profile base string of SC */
    std::vector<std::string> rvstr;  /* profile base string of stem partner of SC */
    int rvposition;                  /* 3' end position of stem partner of SC */
    int rvscnumber;                  /* SC number of stem partner */
    int contPos;       	             /* previous stem that corresponds continuous stem and has position -1. */
    int beforePos;    	             /* most recent stem that doesn't overlap to SC and has position -len. */
    float score;                     /* score of the sum of base pairing probability matrix */
    std::vector<float> baseScore;
    float stacking;                  /* the mean of stacking energy */
    float stemStacking;              /* the mean of 1-continuous stacking energy */

public:
    StemCandidate() : numSeq(0), length(0), position(0), distance(0), 
                      rvposition(0), rvscnumber(0), contPos(-1), beforePos(0),
                      score(0), stacking(0), stemStacking(0) {}
    StemCandidate(int numSeq, int length) : numSeq(numSeq), length(length), 
					    substr(numSeq), rvstr(numSeq), 
					    contPos(-1) { }

    void SetNumSeq(int num) { numSeq = num; }
    void SetLength(int len) { length = len; }
    void SetNumSubstr(int num) {
        substr.resize(num);
	for(int i = 0; i < num; i++) {
	    string &tmpStr = substr[i];
	    tmpStr = "";
	    substr[i] = tmpStr;
	}
    }
    void SetNumRvstr(int num) {
        rvstr.resize(num);
	
	for(int i = 0; i < num; i++) {
	    string &tmpStr = rvstr[i];
	    tmpStr = "";
	    rvstr[i] = tmpStr;
	}
    }
    void SetPosition(int pos) { position = pos; }

    void AddSubstr(int num, char word) {
	std::string &tmpStr = substr[num];
	tmpStr += word;
	substr[num] = tmpStr;
    }

    void AddRvstr(int num, char word) {
	std::string &tmpStr = rvstr[num];
	tmpStr += word;
	rvstr[num] = tmpStr;
    }

    void AddBaseScore(float score) {
        baseScore.push_back(score);
    }

    void SetRvposition(int pos) { rvposition = pos; }
    void SetRvscnumber(int num) { rvscnumber = num; }
    void SetContPos(int pos) { contPos = pos; }
    void SetBeforePos(int pos) { beforePos = pos; }
    void SetDistance(int d) { distance = d; }
    void SetScore(float s) { score = s; }
    void AddScore(float s) { score += s; }
    void SetStacking(float s) { stacking = s; }
    void AddStacking(float s) { stacking += s; }
    void SetStemStacking(float s) { stemStacking = s; }
    int  GetNumSeq() const { return numSeq; }
    int  GetLength() const { return length; }
    int  GetPosition() const { return position; }
    
    string GetSubstr(int num) const { 
	const std::string &tmpStr = substr[num];
	return tmpStr;
    }
    string GetRvstr(int num) const {
	const std::string &tmpStr = rvstr[num];
	return tmpStr;
    }
    float GetBaseScore(int i) const  {
        return baseScore[i];
    }
    int GetRvposition() const {
	return rvposition;
    }
    int GetRvscnumber() const {
	return rvscnumber;
    }
    int GetContPos() const {
	return contPos;
    }
    int GetBeforePos() const {
	return beforePos;
    }
    int GetDistance() const {
        return distance;
    }
    float GetScore() const {
	return score;
    }
    float GetStacking() const  {
	return stacking;
    }
    float GetStemStacking() const {
	return stemStacking;
    }
};
}
#endif // __STEMCANDIDATE_HPP__
