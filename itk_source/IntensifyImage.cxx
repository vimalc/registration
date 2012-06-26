// Increase Seg3D's 0 or 1 segmentation and scale it to 255

#include "boost/program_options.hpp"

#include "itkInvertIntensityImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"

#include "IOHelpers.hpp"
#include "ImageStats.hpp"

using namespace std;
namespace po = boost::program_options;

po::variables_map parse_arguments(int argc, char *argv[]);

int main( int argc, char * argv[] )
{
  // Parse command line arguments
  po::variables_map vm = parse_arguments(argc, argv);
  
  typedef unsigned char PixelType;
  typedef itk::Image< PixelType, 2 > ImageType;
  
  ImageType::Pointer input = readImage< ImageType >( vm["inputFile"].as<string>() );
  
  // invert intensity
  typedef itk::InvertIntensityImageFilter <ImageType> InverterType;
 
  InverterType::Pointer inverter = InverterType::New();
  inverter->SetInput(input);
  // inverter->SetMaximum(50); // default 255
  
  // rescale intensity
  typedef itk::RescaleIntensityImageFilter< ImageType, ImageType > IntensifierType;
  IntensifierType::Pointer intensifier = IntensifierType::New();
  intensifier->SetInput( inverter->GetOutput() );
  intensifier->SetOutputMinimum(0);
  intensifier->SetOutputMaximum(255);
  
  ImageType::Pointer output = intensifier->GetOutput();
  writeImage< ImageType >( output, vm["outputFile"].as<string>() );
  
  printImageStats<ImageType>(output);
  
  return EXIT_SUCCESS;
}

po::variables_map parse_arguments(int argc, char *argv[])
{
  // Declare the supported options.
  po::options_description opts("Options");
  opts.add_options()
      ("help,h", "produce help message")
      ("inputFile", po::value<string>(), "input image")
      ("outputFile", po::value<string>(), "output image")
      ("shift,h", po::value<int>()->default_value(127), "constant to add to intensities")
      ("scale,c", po::value<int>()->default_value(127), "coefficient to scale intensities")
  ;
  
  po::positional_options_description p;
  p.add("inputFile", 1)
   .add("outputFile", 1);
  
  // parse command line
  po::variables_map vm;
	try
	{
  po::store(po::command_line_parser(argc, argv)
            .options(opts)
            .positional(p)
            .run(),
            vm);
	}
	catch (std::exception& e)
	{
	  cerr << "caught command-line parsing error" << endl;
    std::cerr << e.what() << std::endl;
    exit(EXIT_FAILURE);
  }
  po::notify(vm);
  
  // if help is specified, or positional args aren't present,
  // or more than one loadX flag
  if(    vm.count("help")
     || !vm.count("inputFile")
     || !vm.count("outputFile")
    )
  {
    cerr << "Usage: "
      << argv[0] << " [--inputFile=]dark.bmp [--outputFile=]bright.bmp [Options]"
      << endl << endl;
    cerr << opts << "\n";
    exit(EXIT_FAILURE);
  }
  
  return vm;
}
