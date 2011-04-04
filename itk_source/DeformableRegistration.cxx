#include "itkNormalizedCorrelationImageToImageMetric.h"

// my files
#include "Stack.hpp"
#include "RegistrationBuilder.hpp"
#include "StackAligner.hpp"
#include "IOHelpers.hpp"
#include "StackIOHelpers.hpp"
#include "StackTransforms.hpp"
#include "Dirs.hpp"
#include "Parameters.hpp"
#include "Profiling.hpp"

void checkUsage(int argc, char const *argv[]) {
  if( argc != 3 )
  {
    cerr << "\nUsage: " << endl;
    cerr << argv[0] << " dataSet resultsDir\n\n";
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char const *argv[]) {
	// Verify the number of parameters in the command line
	checkUsage(argc, argv);
	
	// Process command line arguments
  Dirs::SetDataSet(argv[1]);
  string outputDir(Dirs::ResultsDir() + argv[2] + "/");
	
	// initialise stack objects
	typedef Stack< float > StackType;
  StackType::VolumeType::SpacingType LoResSpacings, HiResSpacings;
	for(unsigned int i=0; i<3; i++) {
    imageDimensions()["LoResSpacings"][i] >> LoResSpacings[i];
    imageDimensions()["HiResSpacings"][i] >> HiResSpacings[i];
  }
  
  StackType::SliceType::SizeType LoResSize;
  itk::Vector< double, 2 > LoResTranslation;
  for(unsigned int i=0; i<2; i++) {
    imageDimensions()["LoResSize"][i] >> LoResSize[i];
    imageDimensions()["LoResTranslation"][i] >> LoResTranslation[i];
  }
  
  StackType LoResStack( getFileNames(Dirs::BlockDir(), Dirs::SliceFile()), LoResSpacings , LoResSize);
  
  StackType::SliceType::SpacingType HiResOriginalSpacings;
  for(unsigned int i=0; i<2; i++) HiResOriginalSpacings[i] = HiResSpacings[i];
  
  StackType HiResStack(getFileNames(Dirs::SliceDir(), Dirs::SliceFile()), HiResOriginalSpacings,
            LoResStack.GetSpacings(), LoResStack.GetResamplerSize());
  
  // initialise stacks' transforms with saved transform files
  Load(LoResStack, outputDir + "LoResTransforms");
  Load(HiResStack, outputDir + "HiResTransforms");  
  
  // shrink mask slices
  cout << "Test mask load.\n";
  loadNumberOfTimesTooBig(HiResStack, outputDir + "numberOfTimesTooBig.txt");
  
  // Generate fixed images to register against
  LoResStack.updateVolumes();
  writeImage< StackType::VolumeType >( LoResStack.GetVolume(), outputDir + "LoResPersistedStack.mha" );
  
  HiResStack.updateVolumes();
  writeImage< StackType::VolumeType >( HiResStack.GetVolume(), outputDir + "HiResPersistedStack.mha" );
  
  // initialise registration framework
  boost::shared_ptr<YAML::Node> pDeformableParameters = config("deformable_parameters.yml");
  typedef RegistrationBuilder< StackType > RegistrationBuilderType;
  RegistrationBuilderType registrationBuilder(*pDeformableParameters);
  RegistrationBuilderType::RegistrationType::Pointer registration = registrationBuilder.GetRegistration();
  StackAligner< StackType > stackAligner(LoResStack, HiResStack, registration);
  
  // Perform non-rigid registration
  StackTransforms::InitializeBSplineDeformableFromBulk(LoResStack, HiResStack);
  StackTransforms::SetOptimizerScalesForBSplineDeformableTransform(HiResStack, registration->GetOptimizer());
  
  // typedef itk::LBFGSBOptimizer DeformableOptimizerType;
  // DeformableOptimizerType::Pointer deformableOptimizer = DeformableOptimizerType::New();
  // 
  // StackTransforms::ConfigureLBFGSBOptimizer(LoResStack.GetTransform(0)->GetNumberOfParameters(), deformableOptimizer);
  // unsigned int numberOfParameters = HiResStack.GetTransform(0)->GetNumberOfParameters();
  // itk::LBFGSBOptimizer::BoundSelectionType boundSelect( numberOfParameters );
  // itk::LBFGSBOptimizer::BoundValueType upperBound( numberOfParameters );
  // itk::LBFGSBOptimizer::BoundValueType lowerBound( numberOfParameters );
  // 
  // boundSelect.Fill( 0 );
  // upperBound.Fill( 0.0 );
  // lowerBound.Fill( 0.0 );
  // 
  // deformableOptimizer->SetBoundSelection( boundSelect );
  // deformableOptimizer->SetUpperBound( upperBound );
  // deformableOptimizer->SetLowerBound( lowerBound );
  // 
  // deformableOptimizer->SetCostFunctionConvergenceFactor( 1e+7 );
  // deformableOptimizer->SetProjectedGradientTolerance( 0.0000001 );
  // deformableOptimizer->SetMaximumNumberOfIterations( 500 );
  // deformableOptimizer->SetMaximumNumberOfEvaluations( 500 );
  // deformableOptimizer->SetMaximumNumberOfCorrections( 10 );
  // deformableOptimizer->MaximizeOn();
  // 
  // Create an observer and register it with the optimizer
  // typedef StdOutIterationUpdate< itk::LBFGSBOptimizer > StdOutObserverType;
  // StdOutObserverType::Pointer stdOutObserver = StdOutObserverType::New();
  // deformableOptimizer->AddObserver( itk::IterationEvent(), stdOutObserver );
  // 
  // registration->SetOptimizer( deformableOptimizer );
  
  itkProbesCreate();
  itkProbesStart( "Aligning stacks" );
  stackAligner.Update();
  itkProbesStop( "Aligning stacks" );
  itkProbesReport( std::cout );
  
  HiResStack.updateVolumes();
  
  writeImage< StackType::VolumeType >( HiResStack.GetVolume(), outputDir + "HiResDeformedStack.mha" );
  writeImage< StackType::MaskVolumeType >( HiResStack.Get3DMask()->GetImage(), outputDir + "HiResSimilarityMask.mha" );
  
  return EXIT_SUCCESS;
}
