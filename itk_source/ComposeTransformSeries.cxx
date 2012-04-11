// Composes two transform sets.
// Output transform is the result of applying the first input transform, followed by the second.

#include <assert.h>

#include "itkTransformFileReader.h"
#include "itkTransformFileWriter.h"
#include "itkTransformFactory.h"
#include "itkAffineTransform.h"

#include "IOHelpers.hpp"
#include "Dirs.hpp" 


void checkUsage(int argc, char const *argv[]) {
  if( argc < 3 )
  {
    cerr << "\nUsage: " << endl;
    cerr << argv[0] << " originalDir adjustmentDir outputDir\n\n";
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char const *argv[]) {
	// Verify the number of parameters in the command line
	checkUsage(argc, argv);
	
	// Generate file lists
  vector< string > basenames = directoryContents(argv[1]);
  vector< string > originalPaths   = constructPaths(argv[1], basenames);
  vector< string > adjustmentPaths = constructPaths(argv[2], basenames);
  vector< string > outputPaths     = constructPaths(argv[3], basenames);
  
	// clear results directory
  remove_all(argv[3]);
  create_directory(argv[3]);
	
	// Some transforms might not be registered
  // with the factory so we add them manually
  itk::TransformFactoryBase::RegisterDefaultTransforms();
  // itk::TransformFactory< itk::TranslationTransform< double, 2 > >::RegisterTransform();
  
	// Generate new transforms
	typedef itk::TransformFileReader ReaderType;
	typedef itk::TransformFileWriter WriterType;
	// TranslationTransform also has a Compose() interface, but only with other TranslationTransforms
  typedef itk::MatrixOffsetTransformBase< double, 2, 2 > ComposableTransformType;
  typedef itk::AffineTransform< double, 2 > AffineTransformType;
  
  // the first and last slices should not have adjustments
  // apply adjustments for every other slice
  for(unsigned int i=1; i < originalPaths.size() - 1; ++i)
  {
    // Load input transforms
    ReaderType::Pointer originalReader = ReaderType::New();
    ReaderType::Pointer adjustmentReader = ReaderType::New();
    originalReader->SetFileName(  originalPaths[i].c_str()  );
    adjustmentReader->SetFileName( adjustmentPaths[i].c_str() );
    originalReader->Update();
    adjustmentReader->Update();
    
    // check that transforms are of the right dynamic type
    ComposableTransformType *pOriginalTransform  = dynamic_cast<ComposableTransformType*>( originalReader->GetTransformList()->begin()->GetPointer() );
    ComposableTransformType *pAdjustmentTransform = dynamic_cast<ComposableTransformType*>( adjustmentReader->GetTransformList()->begin()->GetPointer() );
    assert( pOriginalTransform != 0 && pAdjustmentTransform != 0 );
    
    // compose transforms
    // If the argument pre is true (default false), then other is precomposed with self; that is, the resulting transformation consists of first
    // applying other to the source, followed by self. If pre is false or omitted, then other is post-composed with self; that is the resulting
    // transformation consists of first applying self to the source, followed by other. This updates the Translation based on current center.
    AffineTransformType::Pointer outputTransform = AffineTransformType::New();
    outputTransform->SetIdentity();
    outputTransform->Compose(pOriginalTransform);
    outputTransform->Compose(pAdjustmentTransform);
    
    // save output transform
    WriterType::Pointer writer = WriterType::New();
    writer->SetFileName( outputPaths[i].c_str() );
    writer->AddTransform( outputTransform );
    writer->Update();
    
  }
  
  // write out the unaltered first and last transforms
  copy_file( *(originalPaths.begin()), *(outputPaths.begin()) );
  copy_file( *(--originalPaths.end()), *(--outputPaths.end()) );
  
  
  return EXIT_SUCCESS;
}
