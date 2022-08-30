#ifndef _COMM_COMMON_HPP
#define _COMM_COMMON_HPP



// failed attempt to replace this massive amount of signatures in the
// communicators with a macro... would obfuscate the code quite a lot
// and still requires manual labor =(

// keeping it around for a second attempt though 




// begin foreach 
// from
// http://stackoverflow.com/questions/1872220/is-it-possible-to-iterate-over-arguments-in-variadic-macros
// Make a FOREACH macro
#define FE_1(WHAT, A,B, X) WHAT(A,B,X) 
#define FE_2(WHAT, A,B,X, ...) WHAT(A,B,X)FE_1(WHAT,A,B, __VA_ARGS__)
#define FE_3(WHAT, A,B,X, ...) WHAT(A,B,X)FE_2(WHAT,A,B, __VA_ARGS__)
#define FE_4(WHAT, A,B,X, ...) WHAT(A,B,X)FE_3(WHAT,A,B, __VA_ARGS__)
#define FE_5(WHAT, A,B,X, ...) WHAT(A,B,X)FE_4(WHAT,A,B, __VA_ARGS__)
//... repeat as needed

#define GET_MACRO(_1,_2,_3,_4,_5,NAME,...) NAME 
#define FOR_EACH(action, TOK,CLASS,...)					\
  GET_MACRO(__VA_ARGS__,FE_5,FE_4,FE_3,FE_2,FE_1)(action, TOK, CLASS,__VA_ARGS__)

// end 




// #define INSTANTIATE(TOKEN, CLASS, TYPE)  template TOKEN##_SIG(CLASS,TYPE);
#define INSTANTIATE(TOKEN, CLASS, TYPE) template auto CLASS::##TOKEN(TYPE);


#define ALLREDUCE(T) \
  allreduce(std::vector<T> myValues) -> std::vector<T>


// #define ALLREDUCE_SIG(CLASS, T)					\
//   CLASS::allReduce(std::vector<T> myValues) -> std::vector<T>

#define SCATTER_VARIABLE_KNOWN_LENGTH(TYPE) \
  scatterVariableKnownLength( std::vector<TYPE>   &myData,  std::vector<int> &countsPerProc,  std::vector<int> &displPerProc,  int root) const -> std::vector<TYPE>

#define CLASS_DECL(TOKEN,TYPE)			\
  template<typename T> auto TOKEN(TYPE)

#define IMPLEMENT(TOKEN, CLASS, CODE, ...)     \
  template<typename T> auto CLASS::##TOKEN(T)				\
  CODE									\
  FOR_EACH(INSTANTIATE, TOKEN, CLASS, __VA_ARGS__)			\
  
#endif

