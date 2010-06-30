// YAML config reader
#include "yaml.h"

// ITK includes
#include "itkImage.h"
#include "itkImageRegistrationMethod.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkImage.h"
#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkImageMaskSpatialObject.h"

// File IO
#include "itkRegularExpressionSeriesFileNames.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkTransformFileWriter.h"
#include "itkTransformFactory.h"

// my files
#include "StdOutIterationUpdate.hpp"
#include "FileIterationUpdate.hpp"
#include "MultiResRegistrationCommand.hpp"
#include "Stack.hpp"
#include "Framework3D.hpp"
#include "Framework2DRat.hpp"
#include "helper_functions.hpp"

void checkUsage(int argc, char const *argv[]) {
  if( argc != 5 )
  {
    cerr << "\nUsage: " << endl;
    cerr << argv[0] << " LoResDir HiResDir registrationParamsFile outputDir\n\n";
    exit(EXIT_FAILURE);
  }
  
}

int main (int argc, char const *argv[]) {
	// Verify the number of parameters in the command line
	checkUsage(argc, argv);
  
  string LoResDir(argv[1]), HiResDir(argv[2]), registrationParamsFile(argv[3]), outputDir(argv[4]);
	
	// read registration parameters
  YAML::Node registrationParameters;
  readRegistrationParameters(registrationParameters, registrationParamsFile);
	
	// initialise stack objects
  Stack::VolumeType::SpacingType LoResSpacings, HiResSpacings;
	for(unsigned int i=0; i<3; i++) {
    registrationParameters["LoResSpacings"][i] >> LoResSpacings[i];
    registrationParameters["HiResSpacings"][i] >> HiResSpacings[i];
  }
  
	Stack LoResStack( getFileNames(LoResDir, outputDir + "/picked_files.txt"), LoResSpacings );
  Stack HiResStack( getFileNames(HiResDir, outputDir + "/picked_files2.txt"), HiResSpacings );
  
  // Write final transform to file
  // writeData< itk::TransformFileWriter, Framework3D::TransformType3D >( framework3D.transform3D, (outputDir + "/finalParameters3D.transform").c_str() );
  
	// perform 2-D registration
  Framework2DRat framework2DRat(&LoResStack, &HiResStack, registrationParameters);
  framework2DRat.StartRegistration( outputDir + "/output2D.txt" );
  //
	
	// perform non-rigid registration
	// check out itkMultiResolutionPDEDeformableRegistration
	
	// write volume and mask to disk
  // LoResStack.UpdateVolumes();
	writeImage< Stack::VolumeType >( LoResStack.GetVolume(), outputDir + "/LoResStack.mhd" );
  // writeImage< Stack::MaskVolumeType >( LoResStack.GetMaskVolume(), outputDir + "/LoResMask.mhd" );
	writeImage< Stack::VolumeType >( HiResStack.GetVolume(), outputDir + "/HiResStack.mhd" );
  // writeImage< Stack::MaskVolumeType >( HiResStack.GetMaskVolume(), outputDir + "/HiResMask.mhd" );
		
  return EXIT_SUCCESS;
}