/////////////////////////////////////////////////////////////////
// MakeGnuPlot.cc
/////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>

using namespace std;

int main (int argc, char **argv){
  
  if (argc == 1 || argc > 3){
    cerr << "Usage: makegnuplot annotscores [refscores]" << endl;
    exit (1);
  }

  ifstream data (argv[1]);

  if (data.fail()){
    cerr << "ERROR: Could not open file " << argv[1] << endl;
    exit (1);
  }
  
  int x, ct = 0;
  while (data >> x) ct++;
  data.close();
  
  ofstream out ("temporary_gnuplot_script");
  
  if (out.fail()){
    cerr << "ERROR: Could not create temporary file." << endl;
    exit (1);
  }

  out << "set title \"Column Reliability Scores\"" << endl
      << "set xlabel \"Alignment Position\"" << endl
      << "set ylabel \"Column Reliability\"" << endl
      << "set xr [1:" << ct << "]" << endl
      << "set term postscript enhanced color" << endl
      << "set output \"reliability.ps\"" << endl;
  
  if (argc == 3){
    out << "set style fill solid 0.5 noborder" << endl
	<< "plot \"" << argv[2] << "\" title \"actual\" with boxes lt 2, \\" << endl
	<< "     \"" << argv[1] << "\" title \"predicted\" with histeps lt 1 lw 3" << endl;
  }
  else 
    out << "plot \"" << argv[1] << "\" title \"predicted\" with histeps lt 1 lw 3" << endl;

  out.close();

  if (system ("gnuplot temporary_gnuplot_script") == -1){
    cerr << "ERROR: Could not run Gnuplot correctly." << endl;
    exit (1);
  }
  
  //system ("rm temporary_gnuplot_script");
}
