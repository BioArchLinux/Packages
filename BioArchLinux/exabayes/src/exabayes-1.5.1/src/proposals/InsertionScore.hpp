#ifndef _INSERTION_SCORE 
#define _INSERTION_SCORE 


class  InsertionScore
{
public: 
  InsertionScore(BranchPlain _b, std::vector<nat> _tmp) : b(_b), partitionParsimony(_tmp){}  
  BranchPlain getBranch() const  {return b; }

  double getWeight() const {return  logProb; }
  void setWeight(double w) { logProb = w; }

  nat getScore() const
  {
    nat result = 0; 
    for(auto b : partitionParsimony)
      result += b; 
    return result; 
  }

  nat getPartitionScore(int model) const{return partitionParsimony[model] ; }


private: 
  BranchPlain b; 
  std::vector<nat> partitionParsimony; 
  double logProb;

  friend std::ostream& operator<< (std::ostream &out, const InsertionScore &rhs) { 
    out <<  "(" << rhs.b << "=" ; 
    for(auto elem : rhs.partitionParsimony)
      out << elem << "," ; 
    return out; 
  }
} ; 


#endif
