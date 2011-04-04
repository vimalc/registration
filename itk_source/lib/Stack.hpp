// This object is built with some original images.
// Its job is to take transform parameters,
// then build a volume and an associated mask.

#ifndef STACK_HPP_
#define STACK_HPP_

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkTileImageFilter.h"
#include "itkNormalizeImageFilter.h"
#include "itkResampleImageFilter.h"
#include "itkTransform.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkChangeInformationImageFilter.h"
#include "itkImageMaskSpatialObject.h"
#include "itkImageRegionIterator.h"


using namespace std;

template <typename TPixel>
class Stack : public itk::Object {
public:
  typedef TPixel PixelType;
	typedef itk::Image< PixelType, 2 > SliceType;
	typedef itk::Image< unsigned char, 2 > MaskSliceType;
	typedef itk::ImageRegionIterator< MaskSliceType > MaskSliceIteratorType;
	typedef itk::Image< PixelType, 3 > VolumeType;
	typedef itk::Image< unsigned char, 3 > MaskVolumeType;
	typedef vector< typename SliceType::Pointer > SliceVectorType;
  typedef vector< MaskSliceType::Pointer > MaskSliceVectorType;
  typedef itk::ImageFileReader< SliceType > ReaderType;
	typedef itk::Transform< double, 2, 2 > TransformType;
	typedef vector< TransformType::Pointer > TransformVectorType;
  typedef itk::LinearInterpolateImageFunction< SliceType, double > LinearInterpolatorType;
  typedef itk::NearestNeighborInterpolateImageFunction< MaskSliceType, double > NearestNeighborInterpolatorType;
  typedef itk::NormalizeImageFilter< SliceType, SliceType > NormalizerType;
	typedef itk::ResampleImageFilter< SliceType, SliceType > ResamplerType;
	typedef itk::ResampleImageFilter< MaskSliceType, MaskSliceType > MaskResamplerType;
  typedef itk::TileImageFilter< SliceType, VolumeType > TileFilterType;
  typedef itk::TileImageFilter< MaskSliceType, MaskVolumeType > MaskTileFilterType;
  typedef itk::ChangeInformationImageFilter< SliceType > XYScaleType;
  typedef itk::ChangeInformationImageFilter< VolumeType > ZScaleType;
  typedef itk::ChangeInformationImageFilter< MaskVolumeType > MaskZScaleType;
	typedef itk::ImageMaskSpatialObject< 3 > MaskType3D;
  typedef itk::ImageMaskSpatialObject< 2 > MaskType2D;
	typedef vector< MaskType2D::Pointer > MaskVectorType2D;
	
private:
  const vector< string > fileNames;
	SliceVectorType originalImages;
	typename XYScaleType::Pointer xyScaler;
  SliceVectorType slices;
	typename VolumeType::Pointer volume;
  typename SliceType::SizeType maxSize;
	typename SliceType::SizeType resamplerSize;
	typename SliceType::SpacingType originalSpacings;
	typename VolumeType::SpacingType spacings;
	typename MaskType3D::Pointer mask3D;
	MaskVectorType2D original2DMasks;
	MaskVectorType2D resampled2DMasks;
	TransformVectorType transforms;
	typename LinearInterpolatorType::Pointer linearInterpolator;
	typename NearestNeighborInterpolatorType::Pointer nearestNeighborInterpolator;
	typename NormalizerType::Pointer normalizer;
	typename ResamplerType::Pointer resampler;
	typename MaskResamplerType::Pointer maskResampler;
	typename TileFilterType::Pointer tileFilter;
	typename TileFilterType::LayoutArrayType layout;
	typename MaskTileFilterType::Pointer maskTileFilter;
	typename ZScaleType::Pointer zScaler;
	typename MaskZScaleType::Pointer maskZScaler;
  vector< unsigned int > numberOfTimesTooBig;
	
public:
  // constructor to center images and size stack to fit in the longest and widest image
  Stack(const vector< string >& inputFileNames, const typename VolumeType::SpacingType& inputSpacings);

	// constructor to specify size and start index explicitly
  Stack(const vector< string >& inputFileNames, const typename VolumeType::SpacingType& inputSpacings,
        const typename SliceType::SizeType& inputSize);
	
	// constructor to specify stack size and spacing, and spacing of original images
  Stack(const vector< string >& inputFileNames, const typename SliceType::SpacingType& inputOriginalSpacings,
        const typename VolumeType::SpacingType& inputSpacings, const typename SliceType::SizeType& inputSize);
	
protected:
  void readImages();
  
  void normalizeImages();
  
  void initializeVectors();
  
  void scaleOriginalSlices();
	
  void buildOriginalMaskSlices();
	
  void calculateMaxSize();
	
	// Stack is just big enough to fit the longest and widest slices in
  void setResamplerSizeToMaxSize();
  
  void initializeFilters();
	
public:	
  void updateVolumes();
	
protected:
  void buildSlices();
	
  void buildVolume();
  
  void buildMaskSlices();
	
  void buildMaskSlice(unsigned int slice_number);
	
  void buildMaskVolume();
	
  void checkSliceNumber(unsigned int slice_number) const;
	
public:
  // Getter methods
  const string& GetFileName(unsigned int slice_number) const {
    checkSliceNumber(slice_number);
    return fileNames[slice_number];
  }
    
  unsigned short GetSize() const { return originalImages.size(); }
  
  const typename SliceType::SizeType& GetMaxSize() const { return maxSize; }

  const typename SliceType::SizeType& GetResamplerSize() const { return resamplerSize; }
  
  const typename VolumeType::SpacingType& GetSpacings() const { return spacings; }

  const typename SliceType::SpacingType& GetOriginalSpacings() const { return originalSpacings; }
          
  typename SliceType::Pointer GetOriginalImage(unsigned int slice_number) {
    checkSliceNumber(slice_number);
  	return originalImages[slice_number];
  }
	
  typename MaskType2D::Pointer GetOriginal2DMask(unsigned int slice_number) {
  	checkSliceNumber(slice_number);
    return original2DMasks[slice_number];
  }
	
  typename SliceType::Pointer GetResampledSlice(unsigned int slice_number) {
    checkSliceNumber(slice_number);
    return slices[slice_number];
  }
  
  typename MaskType2D::Pointer GetResampled2DMask(unsigned int slice_number) {
    checkSliceNumber(slice_number);
    return resampled2DMasks[slice_number];
  }
  
  typename VolumeType::Pointer GetVolume() { return volume; }
	
  typename MaskType3D::Pointer Get3DMask() { return mask3D; }
	
  TransformType::Pointer GetTransform(unsigned int slice_number) {
    checkSliceNumber(slice_number);
    return transforms[slice_number];
  }
	
	const string& GetFileName(unsigned int slice_number) {
    return fileNames[slice_number];
	}
	
  const TransformVectorType& GetTransforms() { return transforms; }
	
  void SetTransforms(const TransformVectorType& inputTransforms) { transforms = inputTransforms; }
  
  bool ImageExists(unsigned int slice_number) {
    return GetOriginalImage(slice_number)->GetLargestPossibleRegion().GetSize()[0];
  }
  
  void ShrinkMaskSlice(unsigned int slice_number);
  
  const vector< unsigned int >& GetNumberOfTimesTooBig() { return numberOfTimesTooBig; }
  
protected:
  void GenerateMaskSlice(unsigned int slice_number);
	
  static bool fileExists(const string& strFilename);
  
  typename SliceType::SpacingType spacings2D() const {
    typename SliceType::SpacingType spacings2D;
    for(unsigned int i=0; i<2; i++) spacings2D[i] = spacings[i];
    return spacings2D;
  }
  
private:
  // Copy constructor and copy assignment operator deliberately not implemented
  // Made private so that nobody can use them
  Stack(const Stack&);
  Stack& operator=(const Stack&);

};

#include "Stack.txx"
#endif
