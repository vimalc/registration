// Class heirarchy in charge of constructing and configuring Stack objects
// Subclasses specify the following:
//
// * image load dir (default HiRes/LoRes, then setter for e.g. resampled HiRes pairs, or segmented images)
// * basenames (default list is image_list.txt, then either vector<string> or single string setters)
// * image scaling (default HiRes/LoRes)
// * resampler spacings
// * resampler size
// * masks


#ifndef STACKBUILDERBASE_HPP_
#define STACKBUILDERBASE_HPP_

#include "Parameters.hpp"
#include "Dirs.hpp"
#include "PathHelpers.hpp"
#include "NormalizeImages.hpp"

class StackBuilderBase
{
public:
  StackBuilderBase();
  
  // constructs stack through base class interface
  void buildStack();
  
  // abstract polymorphic base
  // see Effective C++ item 7
  virtual ~StackBuilderBase()=0;
  
  void setBasenames(const vector<string>& basenames);
  
  void setBasename(const string& basename);
  
  void setImageLoadDir(const string& imageLoadDir);
  
  void setNormalizeOriginals(bool normalizeSlices)
  { m_normalizeSlices = normalizeSlices; }
  
protected:
  // subclasses must supply a default image load dir
  // in case setImageLoadDir isn't used by client
  // pure virtual function in order to fail at compile
  // time instead of runtime
  virtual string getDefaultImageLoadDir()=0;
  
  string getImageLoadDir();
  vector< string > m_basenames;
  bool m_normalizeSlices;
  
private:
  // Copy constructor and copy assignment operator Made private
  // so that no subclasses or clients can use them,
  // deliberately not implemented so not even class methods can use them
  StackBuilderBase(const StackBuilderBase&);
  StackBuilderBase& operator=(const StackBuilderBase&);
  string m_imageLoadDir;
};

// Constructor
// Sets sensible defaults
StackBuilderBase::StackBuilderBase():
  m_basenames( getBasenames(Dirs::ImageList()) )
{
  // test if configured to normalise images
  registrationParameters()["normalizeImages"] >> m_normalizeSlices;
  cout << "normalizeSlices: " << m_normalizeSlices << endl;
}

StackBuilderBase::~StackBuilderBase() {}

void StackBuilderBase::setBasenames(const vector<string>& basenames)
{
  m_basenames = basenames;
}

void StackBuilderBase::setBasename(const string& basename)
{
  m_basenames = vector< string >(1, basename);
}

void StackBuilderBase::setImageLoadDir(const string& imageLoadDir)
{
  m_imageLoadDir = imageLoadDir;
}

string StackBuilderBase::getImageLoadDir()
{
  if ( m_imageLoadDir.empty() )
    return getDefaultImageLoadDir();
  else
    return m_imageLoadDir;
}

#endif
