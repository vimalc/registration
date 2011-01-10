#ifndef FRAMEWORK2DBASE_HPP_
#define FRAMEWORK2DBASE_HPP_

// ITK includes
#include "itkImageRegistrationMethod.h"
#include "itkRegularStepGradientDescentOptimizer.h"
#include "itkGradientDescentOptimizer.h"
#include "itkMeanSquaresImageToImageMetric.h"
#include "itkMattesMutualInformationImageToImageMetric.h"
#include "itkLinearInterpolateImageFunction.h"

// my files
#include "StdOutIterationUpdate.hpp"
#include "FileIterationUpdate.hpp"
#include "NormalizedDifferenceIterationUpdate.hpp"
#include "Stack.hpp"


class Framework2DBase {
public:
  typedef short PixelType;
  typedef itk::Image< PixelType, 2 > SliceType;
  typedef itk::LinearInterpolateImageFunction< SliceType, double > LinearInterpolatorType;
	typedef itk::ImageRegistrationMethod< SliceType, SliceType > RegistrationType;
	typedef itk::ImageMaskSpatialObject< 2 > MaskType;
  typedef itk::SingleValuedNonLinearOptimizer OptimizerType;
	
	RegistrationType::Pointer registration;
	itk::ImageToImageMetric< SliceType, SliceType >::Pointer metric;
	OptimizerType::Pointer optimizer;
	LinearInterpolatorType::Pointer interpolator;
  
  Framework2DBase();
  
  void buildRegistrationComponents();
  
  void buildMetric();
  
  void buildOptimizer();
  
  void wireUpRegistrationComponents();
  
  void setUpObservers();
	
	itk::SingleValuedNonLinearOptimizer::Pointer GetOptimizer() {
    return optimizer;
	}
	
	// explicitly declare virtual destructor,
  // so that base pointers to derived classes will be destroyed fully
  // Made pure virtual to make class abstract
  virtual ~Framework2DBase()=0;
  
private:
  // Copy constructor and copy assignment operator Made private
  // so that no subclasses or clients can use them,
  // deliberately not implemented so not even class methods can use them
  Framework2DBase(const Framework2DBase&);
  Framework2DBase& operator=(const Framework2DBase&);
	
};

#endif