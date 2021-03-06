#ifndef STACKIOHELPERS_HHP_
#define STACKIOHELPERS_HHP_

#include <assert.h>
#include "boost/filesystem.hpp"

#include "itkTransformFileReader.h"
#include "itkTransformFileWriter.h"
#include "itkTranslationTransform.h"
#include "itkTransformFactory.h"

#include "Stack.hpp"
#include "StackTransforms.hpp"
#include "Dirs.hpp"
#include "IOHelpers.hpp"

using namespace boost::filesystem;

// Stack Persistence
template <typename StackType>
void Save(StackType& stack, const string& directory)
{
  vector< string > transformPaths = constructPaths(directory, stack.GetBasenames());
  
  for(int slice_number=0; slice_number < stack.GetSize(); ++slice_number)
	{
	  writeTransform(stack.GetTransform(slice_number), transformPaths[slice_number]);
  }
  
}

template <typename StackType>
void Load(StackType& stack, const string& directory)
{
  vector< string > transformPaths = constructPaths(directory, stack.GetBasenames());
  
  // Some transforms might not be registered
  // with the factory so we add them manually
  itk::TransformFactoryBase::RegisterDefaultTransforms();
  // TODO: below registers a new version of transform every time Load() is called
  itk::TransformFactory< itk::TranslationTransform< double, 2 > >::RegisterTransform();
  
  typename StackType::TransformVectorType newTransforms;
  
  for(unsigned int slice_number=0; slice_number<stack.GetSize(); ++slice_number)
  {
    itk::TransformBase::Pointer transformBase = readTransform( transformPaths[slice_number] );
    typename StackType::TransformType::Pointer transform = static_cast<typename StackType::TransformType*>( transformBase.GetPointer() );
    newTransforms.push_back( transform );
  }
  
  stack.SetTransforms(newTransforms);
}

template <typename StackType>
void ApplyAdjustments(StackType& stack, const string& directory)
{
  // construct path to config transform file
  //  e.g. config/Rat28/LoRes_adustments/0053.meta
  vector< string > transformPaths = constructPaths(directory, stack.GetBasenames());
  
  // Some transforms might not be registered
  // with the factory so we add them manually
  itk::TransformFactoryBase::RegisterDefaultTransforms();
  itk::TransformFactory< itk::TranslationTransform< double, 2 > >::RegisterTransform();
  
  typename StackType::TransformVectorType newTransforms;
  
  for(unsigned int slice_number=0; slice_number<stack.GetSize(); ++slice_number)
  {
    if( exists(transformPaths[slice_number]) )
    {
      itk::TransformBase::Pointer transform = readTransform( transformPaths[slice_number] );
      
      // convert Array to Vector
      itk::Array< double > parameters( transform->GetParameters() );
      itk::Vector< double, 2 > translation;
      translation[0] = parameters[0];
      translation[1] = parameters[1];
      
      // translate block image
      StackTransforms::Translate<StackType>(stack, translation, slice_number );
    }
  }
}

template <typename DataType>
void saveVectorToFiles(const vector< DataType >& values, const string& dirName, const vector< string >& fileNames)
{
  assert(values.size() == fileNames.size());
  
  path dirPath = Dirs::ResultsDir() + dirName;
  create_directory(dirPath);
  
  for(unsigned int i=0; i<values.size(); ++i)
  {
    path leafName = basename( path(fileNames[i]).leaf() );
    path outPath = dirPath / leafName;
    ofstream outFile(outPath.string().c_str());
    outFile << values[i] << endl;
  }
}

template <typename DataType>
vector< DataType > loadVectorFromFiles(const string& dirName, const vector< string >& fileNames)
{
  path dirPath = Dirs::ResultsDir() + dirName;
  typename vector< DataType >::size_type numberOfFiles = fileNames.size();
  vector< DataType > values(numberOfFiles);
  
  for(unsigned int i=0; i<numberOfFiles; ++i)
  {
    path inPath = dirPath / basename( path(fileNames[i]).leaf() );
    ifstream inFile(inPath.string().c_str());
    assert(inFile.is_open());
    inFile >> values[i];
  }
  
  return values;
}


#endif
