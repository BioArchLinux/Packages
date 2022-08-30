/*
  all communicators (impls or wrapper classes) have to implement these declarations
 */ 



template<typename T> 
auto gather(std::vector<T> myData, nat root = 0 )  
  -> std::vector<T>; 

template<typename T> 
auto gatherVariableLength(std::vector<T> myData, int root = 0)  
  -> std::vector<T>; 

template<typename T>
auto gatherVariableKnownLength(std::vector<T> myData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc , int root = 0)   
  -> std::vector<T>; 


template<typename T> 
auto  scatterVariableKnownLength( std::vector<T> allData, std::vector<int> &countsPerProc, std::vector<int> &displPerProc, int root)    
  -> std::vector<T>; 

template<typename T> 		// done
auto broadcast(std::vector<T> array, int root = 0 )   
  -> std::vector<T>; 

template<typename T>		// ok
auto broadcastVar(T var, int root = 0 )    
  -> T   ; 

template<typename T> 
auto allReduce( std::vector<T> myValues)  
  -> std::vector<T>; 


template<typename T>
std::vector<T> reduce(std::vector<T> data, int root )  ; 

  
template<typename T> 
auto  receive( int source, int tag )  
  -> T ; 

template<typename T>  
auto  send( T elem, int dest, int tag ) 
  -> void ; 

int getRank() const;
size_t size() const; 
bool isValid() const ;

bool haveThreadSupport() const ; 

static void finalize();  
static void initComm(int argc, char **argv);
static void abort(int code, bool waitForAll );


SELF split(const std::vector<int> &color, const std::vector<int> &rank) const; 


void waitAtBarrier() ; 




