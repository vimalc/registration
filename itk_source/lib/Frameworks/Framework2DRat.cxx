#ifndef __FRAMEWORK2DRAT_CXX_
#define __FRAMEWORK2DRAT_CXX_

#include "Framework2DRat.hpp"
#include "helper_functions.hpp"

void Framework2DRat::StartRegistration() {
  unsigned int number_of_slices = LoResStack->GetSize();
  
  for(unsigned int slice_number=0; slice_number < number_of_slices; slice_number++) {
    cout << "slice number: " << slice_number << endl;
    cout << "file name: " << LoResStack->GetFileName(slice_number) << endl;
    
    if( bothImagesExist(slice_number) ) {
      registration->SetFixedImage( LoResStack->GetResampledSlice(slice_number) );
      registration->SetMovingImage( HiResStack->GetOriginalImage(slice_number) );
      
      // TEST TO SEE IF THIS MAKES ANY DIFFERENCE
      // registration->SetFixedImageRegion( LoResStack->originalImages[slice_number]->GetLargestPossibleRegion() );
      // TEST TO SEE IF THIS MAKES ANY DIFFERENCE
      
      metric->SetFixedImageMask( LoResStack->GetResampled2DMask(slice_number) );
      metric->SetMovingImageMask( HiResStack->GetOriginal2DMask(slice_number) );
      
      registration->SetTransform( HiResStack->GetTransform(slice_number) );
      registration->SetInitialTransformParameters( HiResStack->GetTransform(slice_number)->GetParameters() );
      
      // halve the width and height of the LoRes mask for each slice
      // until Mattes mutual information stops throwing errors
      cout << "Trying registration..." << endl;
      while( !tryRegistration() ) {
        LoResStack->ShrinkSliceMask(slice_number);
        writeImage< Stack::MaskSliceType >( LoResStack->GetResampled2DMask(slice_number)->GetImage(), "/Users/matthewgibb/Desktop/TestSliceAfter.mhd" );
      }
    }
  }
  cout << "Finished registration." << endl;
}

bool Framework2DRat::bothImagesExist(unsigned int slice_number) {
  return (LoResStack->ImageExists(slice_number) &&
          HiResStack->ImageExists(slice_number) );
}

bool Framework2DRat::tryRegistration() {
  try {
    registration->StartRegistration();
    cout << "Optimizer stop condition: "
         << registration->GetOptimizer()->GetStopConditionDescription() << endl << endl;
    return true;
  }
  catch( itk::ExceptionObject & err ) {
    cerr << "ExceptionObject caught, halving block image width and height." << endl;
    cerr << err << endl;
    cerr << err.GetNameOfClass() << endl;
    return false;
  }
}


#endif