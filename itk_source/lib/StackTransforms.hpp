#ifndef STACKTRANSFORMS_HPP_
#define STACKTRANSFORMS_HPP_


#include "boost/filesystem.hpp"

#include "itkIdentityTransform.h"
#include "itkTranslationTransform.h"
#include "itkCenteredRigid2DTransform.h"
#include "itkCenteredAffineTransform.h"
#include "itkSingleValuedNonLinearOptimizer.h"
#include "itkBSplineDeformableTransform.h"
#include "itkBSplineDeformableTransformInitializer.h"
#include "itkLBFGSBOptimizer.h"


#include "Stack.hpp"
#include "Dirs.hpp"
#include "Parameters.hpp"
#include "StdOutIterationUpdate.hpp"

using namespace std;

namespace StackTransforms {
  typedef itk::MatrixOffsetTransformBase< double, 2, 2 > LinearTransformType;
  typedef itk::TranslationTransform< double, 2 > TranslationTransformType;
  
  itk::Vector< double, 2 > GetLoResTranslation(const string& roi) {
    itk::Vector< double, 2 > LoResTranslation;
    boost::shared_ptr< YAML::Node > roiNode = config(Dirs::GetDataSet() + "/ROIs/" + roi + ".yml");
    for(unsigned int i=0; i<2; i++) {
      (*roiNode)["Translation"][i] >> LoResTranslation[i];
    }
    return LoResTranslation;
  }
  
  template <typename StackType>
  void InitializeToIdentity(StackType& stack) {
    typedef itk::IdentityTransform< double, 2 > TransformType;
    typename StackType::TransformVectorType newTransforms;
    
    for(unsigned int i=0; i<stack.GetSize(); i++)
		{
      typename StackType::TransformType::Pointer transform( TransformType::New() );
      newTransforms.push_back( transform );
		}
		
    stack.SetTransforms(newTransforms);
    
  }
  
  template <typename StackType>
  void InitializeWithTranslation(StackType& stack, const itk::Vector< double, 2 > &translation) {
    typedef itk::TranslationTransform< double, 2 > TransformType;
    typename StackType::TransformVectorType newTransforms;
    TransformType::ParametersType parameters(2);
    
    parameters[0] = translation[0];
    parameters[1] = translation[1];
    
    for(unsigned int i=0; i<stack.GetSize(); i++)
		{
      typename StackType::TransformType::Pointer transform( TransformType::New() );
      transform->SetParametersByValue( parameters );
      newTransforms.push_back( transform );
		}
		
    stack.SetTransforms(newTransforms);
    
  }
  
  template <typename StackType>
  void InitializeToCommonCentre(StackType& stack) {
    typedef itk::CenteredRigid2DTransform< double > TransformType;
    typename StackType::TransformVectorType newTransforms;
    TransformType::ParametersType parameters(5);
    
    for(unsigned int i=0; i<stack.GetSize(); i++)
		{
			const typename StackType::SliceType::SizeType &originalSize( stack.GetOriginalImage(i)->GetLargestPossibleRegion().GetSize() ),
                                       &resamplerSize( stack.GetResamplerSize() );
      const typename StackType::SliceType::SpacingType &originalSpacings( stack.GetOriginalSpacings() );
      const typename StackType::VolumeType::SpacingType &spacings( stack.GetSpacings() );
			
			// rotation in radians
			parameters[0] = 0;
			// translation, applied after rotation.
			parameters[3] = ( originalSpacings[0] * (double)originalSize[0] - spacings[0] * (double)resamplerSize[0] ) / 2.0;
			parameters[4] = ( originalSpacings[1] * (double)originalSize[1] - spacings[1] * (double)resamplerSize[1] ) / 2.0;
			
			// set them to new transform
      typename StackType::TransformType::Pointer transform( TransformType::New() );
      transform->SetParametersByValue( parameters );
      newTransforms.push_back( transform );
		}
		
    stack.SetTransforms(newTransforms);
  }
  
  // Moves centre of rotation without changing the transform
  void MoveCenter(LinearTransformType * transform, const LinearTransformType::CenterType& newCenter)
  {
    LinearTransformType::OffsetType offset = transform->GetOffset();
    transform->SetCenter(newCenter);
    transform->SetOffset(offset);
  }
  
  template <typename StackType>
  void SetMovingStackCenterWithFixedStack( StackType& fixedStack, StackType& movingStack )
  {
    const typename StackType::TransformVectorType& movingTransforms = movingStack.GetTransforms();
    
    // set the moving slices' centre of rotation to the centre of the fixed image
    for(unsigned int i=0; i<movingStack.GetSize(); ++i)
    {
      LinearTransformType::Pointer transform = dynamic_cast< LinearTransformType* >( movingTransforms[i].GetPointer() );
      if(transform)
      {
        const typename StackType::SliceType::SizeType &resamplerSize( fixedStack.GetResamplerSize() );
        const typename StackType::VolumeType::SpacingType &spacings( fixedStack.GetSpacings() );
        LinearTransformType::CenterType center;

        center[0] = spacings[0] * (double)resamplerSize[0] / 2.0;
        center[1] = spacings[1] * (double)resamplerSize[1] / 2.0;
        
        MoveCenter(transform, center);
      }
      else
      {
        cerr << "Matrix isn't a MatrixOffsetTransformBase :-(\n";
        std::abort();
      }
      
    }
    
  }
  
  template <typename StackType, typename NewTransformType>
  void InitializeFromCurrentTransforms(StackType& stack)
  {
    typename StackType::TransformVectorType newTransforms;
    
    for(unsigned int i=0; i<stack.GetSize(); i++)
    {
      typename NewTransformType::Pointer newTransform = NewTransformType::New();
      newTransform->SetIdentity();
      // specialize from vanilla Transform to lowest common denominator in order to call GetCenter()
      LinearTransformType::Pointer oldTransform( dynamic_cast< LinearTransformType* >( stack.GetTransform(i).GetPointer() ) );
      newTransform->SetCenter( oldTransform->GetCenter() );
      newTransform->Compose( oldTransform );
      typename StackType::TransformType::Pointer baseTransform( newTransform );
      newTransforms.push_back( baseTransform );
    }
    
    // set stack's transforms to newTransforms
    stack.SetTransforms(newTransforms);
    
  }
  
  template <typename StackType>
  void InitializeBSplineDeformableFromBulk(StackType& LoResStack, StackType& HiResStack)
  {
    // Perform non-rigid registration
    typedef double CoordinateRepType;
    const unsigned int SpaceDimension = 2;
    const unsigned int SplineOrder = 3;
    typedef itk::BSplineDeformableTransform< CoordinateRepType, SpaceDimension, SplineOrder > TransformType;
    typedef itk::BSplineDeformableTransformInitializer< TransformType, typename StackType::SliceType > InitializerType;
    typename StackType::TransformVectorType newTransforms;
    
    for(unsigned int slice_number=0; slice_number<HiResStack.GetSize(); slice_number++)
    {
      // instantiate transform
      TransformType::Pointer transform( TransformType::New() );
      
      // initialise transform
      typename InitializerType::Pointer initializer = InitializerType::New();
      initializer->SetTransform( transform );
      initializer->SetImage( LoResStack.GetResampledSlice(slice_number) );
      unsigned int gridSize;
      boost::shared_ptr<YAML::Node> deformableParameters = config("deformable_parameters.yml");
      (*deformableParameters)["bsplineTransform"]["gridSize"] >> gridSize;
      TransformType::RegionType::SizeType gridSizeInsideTheImage;
      gridSizeInsideTheImage.Fill(gridSize);
      initializer->SetGridSizeInsideTheImage( gridSizeInsideTheImage );
      
      initializer->InitializeTransform();
      
      transform->SetBulkTransform( HiResStack.GetTransform(slice_number) );
      
      // set initial parameters to zero
      TransformType::ParametersType initialDeformableTransformParameters( transform->GetNumberOfParameters() );
      initialDeformableTransformParameters.Fill( 0.0 );
      transform->SetParametersByValue( initialDeformableTransformParameters );
      
      // add transform to vector
      typename StackType::TransformType::Pointer baseTransform( transform );
      newTransforms.push_back( baseTransform );
    }
    HiResStack.SetTransforms(newTransforms);
  }
  
  template <typename StackType>
  void Translate(StackType& stack, itk::Vector< double, 2 > translation)
  {
    // attempt to cast stack transforms and apply translation
    for(unsigned int i=0; i<stack.GetSize(); i++)
    {
      LinearTransformType::Pointer linearTransform
        = dynamic_cast< LinearTransformType* >( stack.GetTransform(i).GetPointer() );
      TranslationTransformType::Pointer translationTransform
        = dynamic_cast< TranslationTransformType* >( stack.GetTransform(i).GetPointer() );
      if(linearTransform)
      {
        linearTransform->SetOffset(linearTransform->GetOffset() + translation);
      }
      else if(translationTransform)
      {
        translationTransform->SetOffset(translationTransform->GetOffset() + translation);
      }
      else
      {
        cerr << "Matrix isn't a MatrixOffsetTransformBase or a TranslationTransform :-(\n";
        std::abort();
      }
    }
  }
  
  void SetOptimizerScalesForCenteredRigid2DTransform(itk::SingleValuedNonLinearOptimizer::Pointer optimizer)
  {
    double translationScale, rotationScale;
    registrationParameters()["optimizer"]["scale"]["translation"] >> translationScale;
    registrationParameters()["optimizer"]["scale"]["rotation"] >> rotationScale;
  	itk::Array< double > scales( 5 );
    scales[0] = 1.0;
    scales[1] = translationScale;
    scales[2] = translationScale;
    scales[3] = translationScale;
    scales[4] = translationScale;
    optimizer->SetScales( scales );
  }
  
  void SetOptimizerScalesForCenteredSimilarity2DTransform(itk::SingleValuedNonLinearOptimizer::Pointer optimizer)
  {
    double translationScale, rotationScale, sizeScale;
    registrationParameters()["optimizer"]["scale"]["translation"] >> translationScale;
    registrationParameters()["optimizer"]["scale"]["rotation"] >> rotationScale;
    registrationParameters()["optimizer"]["scale"]["size"] >> sizeScale;
  	itk::Array< double > scales( 6 );
    scales[0] = sizeScale;
    scales[1] = rotationScale;
    scales[2] = translationScale;
    scales[3] = translationScale;
    scales[4] = translationScale;
    scales[5] = translationScale;
    optimizer->SetScales( scales );
  }
  
  void SetOptimizerScalesForCenteredAffineTransform(itk::SingleValuedNonLinearOptimizer::Pointer optimizer)
  {
    double translationScale, sizeScale;
    registrationParameters()["optimizer"]["scale"]["translation"] >> translationScale;
    registrationParameters()["optimizer"]["scale"]["size"] >> sizeScale;
  	itk::Array< double > scales( 8 );
  	// four matrix elements
    scales[0] = sizeScale;
    scales[1] = sizeScale;
    scales[2] = sizeScale;
    scales[3] = sizeScale;
  	// two centre coordinates
    scales[4] = translationScale;
    scales[5] = translationScale;
  	// two translation coordinates
    scales[6] = translationScale;
    scales[7] = translationScale;
    optimizer->SetScales( scales );
  }

  template <typename StackType>
  void SetOptimizerScalesForBSplineDeformableTransform(StackType &stack, itk::SingleValuedNonLinearOptimizer::Pointer optimizer)
  {
    typedef itk::SingleValuedNonLinearOptimizer::ScalesType ScalesType;
    ScalesType optimizerScales = ScalesType( stack.GetTransform(0)->GetNumberOfParameters() );
    optimizerScales.Fill( 1.0 );
    
    optimizer->SetScales( optimizerScales );
    
  }
  
  template <typename StackType>
  void ConfigureLBFGSBOptimizer(unsigned int numberOfParameters, itk::LBFGSBOptimizer::Pointer optimizer)
  {
    // From Example
    itk::LBFGSBOptimizer::BoundSelectionType boundSelect( numberOfParameters );
    itk::LBFGSBOptimizer::BoundValueType upperBound( numberOfParameters );
    itk::LBFGSBOptimizer::BoundValueType lowerBound( numberOfParameters );
    
    boundSelect.Fill( 0 );
    upperBound.Fill( 0.0 );
    lowerBound.Fill( 0.0 );
    
    optimizer->SetBoundSelection( boundSelect );
    optimizer->SetUpperBound( upperBound );
    optimizer->SetLowerBound( lowerBound );
    
    optimizer->SetCostFunctionConvergenceFactor( 1e+12 );
    optimizer->SetProjectedGradientTolerance( 1.0 );
    optimizer->SetMaximumNumberOfIterations( 500 );
    optimizer->SetMaximumNumberOfEvaluations( 500 );
    optimizer->SetMaximumNumberOfCorrections( 5 );
    
    // Create an observer and register it with the optimizer
    typedef StdOutIterationUpdate< itk::LBFGSBOptimizer > StdOutObserverType;
    StdOutObserverType::Pointer stdOutObserver = StdOutObserverType::New();
    optimizer->AddObserver( itk::IterationEvent(), stdOutObserver );
    
  }
  
}

#endif
